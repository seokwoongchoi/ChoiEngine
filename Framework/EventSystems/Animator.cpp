#include "Framework.h"
#include "Animator.h"
#include "Renderer.h"
#include "Resources/Mesh.h"
#include "Resources/TextureTransforms.h"


Animator::Animator(ID3D11Device * device)
	:ColliderSystem(device, "../_Shaders/ComputeShaders/ColliderCS.hlsl", "SkeletalColliderCS"),device(device),
	texture(nullptr), srv(nullptr), maxkeyframe(0), maxBoneCount(0), tweenBuffer(nullptr), RunningTime{}
{
	//collider = new ColliderSystem(device, "../_Shaders/ComputeShaders/ColliderCS.hlsl", "SkeletalColliderCS");
	boneTransfomrs = new SkinnedTransform[MAX_ACTOR_COUNT];


	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(TweenDesc)*20;
	Check(device->CreateBuffer(&bufferDesc, NULL, &tweenBuffer));


	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(BoneBoxDesc) * 20;
	Check(device->CreateBuffer(&bufferDesc, NULL, &boneBoxBuffer));
	
	tweenDesc.assign(20, TweenDesc());
	boneBoxDesc.assign(20, BoneBoxDesc());
	for (uint i = 0; i < 20; i++)
	{
		tweenDesc[i].index = i;
	}
	for(uint i=0;i<6;i++)
	tweenDesc[i].Curr.Clip = i;
}

Animator::~Animator()
{
	
}

void Animator::PushDrawCount(const uint & index, const Matrix & world)
{
	instTexture->instTransforms[index][renderers[index]->drawCount] = world;
	CreateInstTransformSRV();
	renderers[index]->drawCount++;
}

void Animator::BindPipeline(ID3D11DeviceContext * context)
{
	uint totalCount = 0;
	for (uint i = 0; i < renderers.size(); i++)
	{
		totalCount += renderers[i]->drawCount;
	}
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(tweenBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, tweenDesc.data(), sizeof(TweenDesc)*totalCount);
	context->Unmap(tweenBuffer, 0);
	context->VSSetConstantBuffers(3, 1, &tweenBuffer);

	ID3D11ShaderResourceView* srvArray[2] = { InstBufferSRV,srv };
	context->VSSetShaderResources(0, 2, srvArray);
	
}

void Animator::Update(const uint& actorIndex)
{

	int culledIndex = -1;
	culledIndex = FrustumCulling(actorIndex);

	if (culledIndex >= 0)
	{
		SortTweenBuffer(actorIndex, culledIndex);
	}


	
			
	
	if(actorIndex >0)
	renderers[actorIndex]->prevDrawCount = renderers[actorIndex - 1]->drawCount;
	

	uint start = renderers[actorIndex]->prevDrawCount;
	uint end = renderers[actorIndex]->prevDrawCount + renderers[actorIndex]->drawCount;
	for (uint index = start; index < end; index++)
	{
	    //tweenDesc[index].Curr.Clip = tweenDesc[index].index;
		const auto& clip = clips[tweenDesc[index].Curr.Clip];

		static const float speed = 1.0f;
		//현재 애니메이션
		{
			RunningTime[index] += Time::Delta();
		

			float time = 1.0f / clip->FrameRate() / speed;
			if (RunningTime[index] >= time)
			{
				RunningTime[index] = 0.0f;

				tweenDesc[index].Curr.CurrFrame = (tweenDesc[index].Curr.CurrFrame + 1) % clip->FrameCount();
				tweenDesc[index].Curr.NextFrame = (tweenDesc[index].Curr.CurrFrame + 1) % clip->FrameCount();
			}
			tweenDesc[index].Curr.Time = RunningTime[index] / time;
		}

		//바뀔 애니메이션이 존재함
		if (tweenDesc[index].Next.Clip > -1)
		{
			const auto& nextClip = clips[tweenDesc[index].Next.Clip];

			tweenDesc[index].ChangeTime += Time::Delta();
			tweenDesc[index].TweenTime = tweenDesc[index].ChangeTime / tweenDesc[index].TakeTime;

			if (tweenDesc[index].TweenTime >= 1.0f)
			{
				tweenDesc[index].Curr = tweenDesc[index].Next;

				tweenDesc[index].Next.Clip = -1;
				tweenDesc[index].Next.CurrFrame = 0;
				tweenDesc[index].Next.NextFrame = 0;
				tweenDesc[index].Next.Time = 0;
				RunningTime[index] = 0.0f;

				tweenDesc[index].ChangeTime = 0.0f;
				tweenDesc[index].TweenTime = 0.0f;
			}
			else
			{
				RunningTime[index] += Time::Delta();

				float time = 1.0f / nextClip->FrameRate() / speed;
				if (tweenDesc[index].Next.Time >= 1.0f)
				{
					RunningTime[index] = 0;

					tweenDesc[index].Next.CurrFrame = (tweenDesc[index].Next.CurrFrame + 1) % nextClip->FrameCount();
					tweenDesc[index].Next.NextFrame = (tweenDesc[index].Next.CurrFrame + 1) % nextClip->FrameCount();
				}
				tweenDesc[index].Next.Time = RunningTime[index] / time;
			}
		}
	}
	
}

void Animator::AnimTransformSRV()
{
	if (srv) return;
	
	const uint& clipSize = clips.size();
	const uint& boneCount = boneTransfomrs->bones.size();
	auto& saveParentMatrix = boneTransfomrs->saveTransforms;

	
	if (!clips.empty())
	{
		for (uint i = 0; i < clipSize; i++)
		{
			boneTransfomrs[i].CreateTransforms(clips[i]->FrameCount(), boneCount);
			maxkeyframe = max(maxkeyframe, clips[i]->FrameCount());
		
		}

		maxBoneCount = max(maxBoneCount, boneCount);
		
		for (UINT i = 0; i < clipSize; i++)
		{
			for (UINT f = 0; f < clips[i]->FrameCount(); f++)
			{
				for (UINT b = 0; b < boneCount; b++)
				{
					auto& bone = boneTransfomrs->bones[b];
					
					Matrix invGlobal;
					D3DXMatrixInverse(&invGlobal, nullptr, &bone->Transform());

					int parentIndex = bone->ParentIndex();
					Matrix parent;
					if (parentIndex < 0)
						D3DXMatrixIdentity(&parent);
					else
						parent = saveParentMatrix[parentIndex];

					const auto& frame = clips[i]->Keyframe(bone->Name());

					if (frame != nullptr)
					{
						ModelKeyframeData data = frame->Transforms[f];
						Matrix S, R, T;
						D3DXMatrixScaling(&S, data.Scale.x, data.Scale.y, data.Scale.z);
						D3DXMatrixRotationQuaternion(&R, &data.Rotation);
						D3DXMatrixTranslation(&T, data.Translation.x, data.Translation.y, data.Translation.z);

						const Matrix& animation = S * R * T;

						saveParentMatrix[b] = animation * parent;

						boneTransfomrs[i].Transform[f][b] = invGlobal * saveParentMatrix[b];
					}
					else
					{
						saveParentMatrix[b] = parent;

						boneTransfomrs[i].Transform[f][b] = bone->Transform()* saveParentMatrix[b];
					}
				}
			}
		}
		

		//Create Texture
		{
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
			desc.Width = maxBoneCount * 4;
			desc.Height = maxkeyframe;
			desc.MipLevels = 1;
			desc.ArraySize = clipSize;
			desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			desc.SampleDesc.Count = 1;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;



			uint pageSize = maxBoneCount * 4 * 16 * maxkeyframe;
			void* p = malloc(pageSize * clipSize);


			for (UINT c = 0; c < clipSize; c++)
			{
				for (UINT y = 0; y < clips[c]->FrameCount(); y++)
				{
					UINT start = c * pageSize;
					void* temp = (BYTE *)p + maxBoneCount * y * sizeof(Matrix) + start;

					memcpy(temp, boneTransfomrs[c].Transform[y], sizeof(Matrix) * maxBoneCount);
				}
			}

			D3D11_SUBRESOURCE_DATA* subResource = new D3D11_SUBRESOURCE_DATA[clipSize];
			for (UINT c = 0; c < clipSize; c++)
			{
				void* temp = (BYTE *)p + c * pageSize;

				subResource[c].pSysMem = temp;
				subResource[c].SysMemPitch = boneCount * sizeof(Matrix);
				subResource[c].SysMemSlicePitch = pageSize;
			}



			SafeRelease(texture);
			SafeRelease(srv);
			Check(device->CreateTexture2D(&desc, subResource, &texture));

			SafeDeleteArray(subResource);
			free(p);
		
		}

		//Create SRV
		{
			D3D11_TEXTURE2D_DESC desc;
			texture->GetDesc(&desc);

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Format = desc.Format;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.ArraySize = clipSize;

			HRESULT hr = device->CreateShaderResourceView(texture, &srvDesc, &srv);
			Check(hr);


		//// Create the UAVs
		//D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
		//ZeroMemory(&DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
		//DescUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
		//DescUAV.Format = desc.Format;
		//
		//DescUAV.Texture2DArray.ArraySize = model->ClipCount() + 1;
		//
		//
		//Check(D3D::GetDevice()->CreateUnorderedAccessView(texture, &DescUAV, &uav));
		}
	}
}

void Animator::ReadClip(const wstring & name)
{
	wstring filePath = L"../_Models/SkeletalMeshes/" + name + L"/";
	vector<wstring> files;

	wstring filter = L"*.clip";
	Path::GetFiles(&files, filePath, filter, false);

	for (uint i = 0; i < files.size(); i++)
	{
		auto clipfileName = Path::GetFileName(files[i]);
		const auto& file = L"../_Models/SkeletalMeshes/" + name+L"/"+ clipfileName;

		BinaryReader* r = new BinaryReader();
		r->Open(file);
		const auto& clip = make_shared< ModelClip>();

		clip->name =r->String();
		clip->duration = r->Float();
		clip->frameRate = r->Float();
		clip->frameCount = r->UInt();

		UINT keyframesCount = r->UInt();
		for (UINT i = 0; i < keyframesCount; i++)
		{
			const auto& keyframe = make_shared< ModelKeyframe>();
			keyframe->BoneName = r->String();

			UINT size = r->UInt();
			if (size > 0)
			{
				keyframe->Transforms.assign(size, ModelKeyframeData());

				void* ptr = reinterpret_cast<void *>(&keyframe->Transforms[0]);
				r->Byte(&ptr, sizeof(ModelKeyframeData) * size);
			}

			clip->keyframeMap[keyframe->BoneName] = keyframe;

		}

		r->Close();
		SafeDelete(r);

		clips.emplace_back(clip);
	}


	AnimTransformSRV();
}

void Animator::SortTweenBuffer(const uint & actorIndex, const uint & drawIndex)
{
	uint index = 0;
	if (actorIndex == 0)
	{
		index = drawIndex;
	}
	else
	{
		index = renderers[actorIndex]->prevDrawCount + drawIndex;
	}

	
	TweenDesc temp = tweenDesc[index];

	
	uint count = renderers[actorIndex]->drawCount;
	
	for (uint i = index; i < count; i++)
		tweenDesc[i] = tweenDesc[i + 1];

	tweenDesc[count] = temp;
    
}

void Animator::ReadBone(BinaryReader * r)
{
	uint tempBoneCount = r->UInt();

	for (UINT i = 0; i < tempBoneCount; i++)
	{
		const auto& bone = make_shared<ModelBone>();
		bone->index = r->Int();
		bone->name = r->String();
		bone->parentIndex = r->Int();
		bone->transform = r->Matrix();
		boneTransfomrs->bones.emplace_back(bone);

	}

	

	for_each(boneTransfomrs->bones.begin(), boneTransfomrs->bones.end(), [&](shared_ptr<ModelBone>bone)
	{
		if (bone->parentIndex > -1 && bone->parent == nullptr)
		{

			bone->parent = boneTransfomrs->bones[bone->parentIndex];
			bone->parent->childs.push_back(bone);


		}
		else
			bone->parent = NULL;

	});
}
