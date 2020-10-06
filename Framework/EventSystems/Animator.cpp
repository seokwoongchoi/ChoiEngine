#include "Framework.h"
#include "Animator.h"
#include "Renderer.h"
#include "Resources/Mesh.h"
#include "Resources/TextureTransforms.h"
#include "Viewer/Frustum.h"
#include "Transforms.h"
#include "PhysicsSystem.h"
#include "Utility/QuadTree.h"
Animator::Animator(ID3D11Device * device)
	:ColliderSystem(device, "../_Shaders/ComputeShaders/ColliderCS.hlsl", "SkeletalColliderCS"),device(device),
	texture(nullptr), srv(nullptr), maxkeyframe(0), maxBoneCount(0), tweenBuffer(nullptr), tweenData{},
	boneBoxTexture(nullptr), boneBoxSrv(nullptr), transforms(nullptr), tree(nullptr)
{
	tree = new class QuadTree();

	//collider = new ColliderSystem(device, "../_Shaders/ComputeShaders/ColliderCS.hlsl", "SkeletalColliderCS");
	boneTransfomrs = new SkinnedTransform[MAX_ACTOR_COUNT];


	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(TweenDesc)*20;
	Check(device->CreateBuffer(&bufferDesc, NULL, &tweenBuffer));


	
	
	tweenDesc.assign(20, TweenDesc());
	colliderBoxData.assign(30, BoneBoxDesc());

	transforms = new Transforms(device,this);
	
	for (uint i = 0; i < 20; i++)
	{
		tweenData[i].speed = 1.0f;
		
	}
	

}

Animator::~Animator()
{
	
}

void Animator::UpdateInstBuffer(ID3D11DeviceContext * context)
{
	if (totalCount < 1)return;

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(InstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, instTransforms, sizeof(Matrix)*MAX_ACTOR_COUNT*MAX_MODEL_INSTANCE);
	context->Unmap(InstBuffer, 0);
}

bool Animator::ComputeBarier(ID3D11DeviceContext * context)
{
	totalCount = 0;
	uint size = renderers.size();
	for (uint i = 0; i < size; i++)
	{
		totalCount += renderers[i]->drawCount;
	}
	{

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(tweenBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, tweenDesc.data(), sizeof(TweenDesc)*totalCount);
		context->Unmap(tweenBuffer, 0);
	}

	for (uint i = 0; i < totalCount; i++)
	{
		if (tweenDesc[i].state == ActorState::Attack )
			return true;

		
	}
	return false;
}


void Animator::CreateQuadTree()
{
	tree->CreateQuadTree(Vector2(0.0f, 0.0f), Vector2(512.0f, 512.0f),true);
}


void Animator::BoxRender()
{
	tree->BoxRender();
}

const uint & Animator::DrawCount(const uint & actorIndex)
{
	
	return renderers[actorIndex]->drawCount;
}

const uint & Animator::PrevDrawCount(const uint & actorIndex)
{
	return renderers[actorIndex]->prevDrawCount;
}

void Animator::FrustumCulling(const uint & index)
{
	frustum->Update();


	const uint& count = renderers[index]->drawCount;
	

	bool bChanged = false;
    uint start = 0;
	if (index == 0)
		start = 1;

	
	for (uint i = start; i < count; i++)
	{
		
		
		D3DXVec3TransformCoord(&min, &renderers[index]->boxMin, &instTransforms[index][i]);
		D3DXVec3TransformCoord(&max, &renderers[index]->boxMax, &instTransforms[index][i]);

		bool inFrustum = frustum->ContainRect((max+ min)*0.5f, (max - min)*0.5f);
		if (inFrustum == false)
		{
			
			
		
			uint lastIndex = renderers[index]->drawCount-1;

		//	cout << "culled: ";
		//	cout << to_string(i) << endl;


			if (i == lastIndex)
			{
				renderers[index]->drawCount--;

				culledCount[index]++;
				break;
			}
			

			{
				memcpy(&temp, &instTransforms[index][i], sizeof(Matrix));
				memcpy(&instTransforms[index][i], &instTransforms[index][lastIndex], sizeof(Matrix));
				memcpy(&instTransforms[index][lastIndex], &temp, sizeof(Matrix));

				uint animIndex = 0;
				if (index == 0)
				{
					animIndex = i;
				}
				else
				{
					animIndex = renderers[index]->prevDrawCount + i;
				}

				TweenDesc save = tweenDesc[animIndex];
				tweenDesc[i] = tweenDesc[lastIndex];
				tweenDesc[lastIndex] = save;
				
			}

			
			renderers[index]->drawCount--;

			culledCount[index]++;
			
			bChanged = true;
		
			
			
			break;
		
		}
	}
	
	const uint& totalCount = renderers[index]->drawCount + culledCount[index];
	
	
	for (uint i = count; i < totalCount ; i++)
	{
		

		D3DXVec3TransformCoord(&min, &renderers[index]->boxMin, &instTransforms[index][i]);
		D3DXVec3TransformCoord(&max, &renderers[index]->boxMax, &instTransforms[index][i]);

		bool inFrustum = frustum->ContainRect((max + min)*0.5f, (max - min)*0.5f);
		if (inFrustum == true)
		{
			//if (i >= renderers[index]->drawCount)
			{
				


				//cout << "InFrustum: ";
				//cout << to_string(i) << endl;

				if (i == count)
				{
					renderers[index]->drawCount++;
					culledCount[index]--;

					break;
				}
					
				{
					memcpy(&temp, &instTransforms[index][count], sizeof(Matrix));

					memcpy(&instTransforms[index][count], &instTransforms[index][i], sizeof(Matrix));
					memcpy(&instTransforms[index][i], &temp, sizeof(Matrix));
					uint animIndex = 0;
					if (index == 0)
					{
						animIndex = count;
					}
					else
					{
						animIndex = renderers[index]->prevDrawCount + count;
					}

			
					TweenDesc save = tweenDesc[count];
					tweenDesc[animIndex] = tweenDesc[i];
					tweenDesc[i] = save;
					
					bChanged = true;
				}
				
				renderers[index]->drawCount++;
				culledCount[index]--;

				break;
			}
		}
	

	}

	if (bChanged)
	{
		//UpdateInstBuffer();

	}
	
}
	
void Animator::BindPipeline(ID3D11DeviceContext * context)
{
	

	context->VSSetConstantBuffers(3, 1, &tweenBuffer);
	ID3D11ShaderResourceView* srvArray[2] = { InstBufferSRV,srv };
	context->VSSetShaderResources(0, 2, srvArray);
	
}

void Animator::Update(const uint& actorIndex)
{
	

	//if (renderers[actorIndex]->drawCount < 1)return;


	FrustumCulling(actorIndex);

	if (actorIndex > 0)
		renderers[actorIndex]->prevDrawCount = renderers[actorIndex - 1]->drawCount;
	uint start = renderers[actorIndex]->prevDrawCount;
	uint end = renderers[actorIndex]->prevDrawCount + renderers[actorIndex]->drawCount;
	for (uint index = start; index < end; index++)
	{
		auto& tween = tweenDesc[index];
	

		PlayNextClip(index, static_cast<uint>(tween.state));
		if (tween.state == ActorState::Die)
		{
			uint currentClipNum = static_cast<uint>(tween.state);
			if (tween.Curr.NextFrame >= clips[currentClipNum]->Duration() - 1)
			{
				tween.Curr.CurrFrame = static_cast<uint>(clips[currentClipNum]->Duration()) - 2;
				tween.Curr.NextFrame = static_cast<uint>(clips[currentClipNum]->Duration()) - 1;
			
				continue;
			}
		
		}
		else
		{
		
			if (tween.Curr.NextFrame >= clips[tween.Curr.Clip]->Duration() - 1)
			{
				tweenData[index].speed = 1.0f;
				tween.state = ActorState::Idle;
			}
		
		}
		
		const auto& clip = clips[tween.Curr.Clip];

		
		{
			tweenData[index].RunningTime += Time::Delta();
		

			float time = 1.0f / clip->FrameRate() / tweenData[index].speed;
			if (tweenData[index].RunningTime >= time)
			{
				tweenData[index].RunningTime = 0.0f;

				tween.Curr.CurrFrame = (tween.Curr.CurrFrame + 1) % clip->FrameCount();
				tween.Curr.NextFrame = (tween.Curr.CurrFrame + 1) % clip->FrameCount();
			}
			tween.Curr.Time = tweenData[index].RunningTime / time;
		}

		
		if (tween.Next.Clip > -1)
		{
			const auto& nextClip = clips[tween.Next.Clip];

			tween.ChangeTime += Time::Delta();
			tween.TweenTime = tween.ChangeTime / tween.TakeTime;

			if (tween.TweenTime >= 1.0f)
			{
				tween.Curr = tween.Next;

				tween.Next.Clip = -1;
				tween.Next.CurrFrame = 0;
				tween.Next.NextFrame = 0;
				tween.Next.Time = 0;
				tweenData[index].RunningTime = 0.0f;

				tween.ChangeTime = 0.0f;
				tween.TweenTime = 0.0f;
			}
			else
			{
				if (tweenData[index].bContinue == true)
				{
					
					float frameCount = static_cast<float>(clip->FrameCount());
					float temp = static_cast<float>(tween.Curr.CurrFrame) / frameCount;
					float temp2 = static_cast<float>(nextClip->FrameCount());
					float temp3 = temp * temp2;
					tween.Next.CurrFrame = static_cast<uint>(temp3);
					tweenData[index].bContinue = false;
				}
				tweenData[index].RunningTime += Time::Delta();

				float time = 1.0f / nextClip->FrameRate() / tweenData[index].speed;
				if (tween.Next.Time >= 1.0f)
				{
					tweenData[index].RunningTime = 0;

					tween.Next.CurrFrame = (tween.Next.CurrFrame + 1) % nextClip->FrameCount();
					tween.Next.NextFrame = (tween.Next.CurrFrame + 1) % nextClip->FrameCount();
				}
				tween.Next.Time = tweenData[index].RunningTime / time;
			}
		}
	}
	

	transforms->Update(actorIndex);
	const uint& drawCount = renderers[actorIndex]->drawCount;
	
	for (uint i = 0; i < drawCount; i++)
	{
		
		
		tree->Intersection(&instTransforms[actorIndex][i]);
	}
}

void Animator::Compute(ID3D11DeviceContext * context,ID3D11UnorderedAccessView* physicsUAV)
{
	totalCount = 0;
	uint size = renderers.size();
	for (uint i = 0; i < size; i++)
	{
		totalCount += renderers[i]->drawCount;
	}
	
	
	if (totalCount <1)return;

	
	{

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(tweenBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, tweenDesc.data(), sizeof(TweenDesc)*totalCount);
		context->Unmap(tweenBuffer, 0);
	}
	
	
	ID3D11Buffer* bufferArray[1] = { tweenBuffer };
	context->CSSetConstantBuffers(2, 1, bufferArray);

	ID3D11ShaderResourceView* srvArray[3] = { InstBufferSRV,srv,boneBoxSrv };
	context->CSSetShaderResources(0, 3, srvArray);

	context->CSSetUnorderedAccessViews(0, 1, &physicsUAV,nullptr);

	context->CSSetShader(ColliderCS, nullptr, 0);

	
	context->Dispatch(actorCount, 1, 1);


	ZeroMemory(&bufferArray, sizeof(bufferArray));
	context->CSSetConstantBuffers(2, 1, bufferArray);
	context->CSSetShader(nullptr, nullptr, 0);
	ZeroMemory(srvArray, sizeof(srvArray));
	context->CSSetShaderResources(0, 3, srvArray);
	ID3D11UnorderedAccessView* nullUav = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUav, nullptr);

	
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
	//clips.clear();
	//clips.shrink_to_fit();
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

void Animator::ReadBoneBox(BinaryReader * r,const uint & actorIndex)
{
	uint  tempBoxCount = r->UInt();
	if (tempBoxCount > 0)
	{
	
		for (uint i = 0; i < tempBoxCount; i++)
		{
			BoneBoxDesc temp;
			uint index= r->UInt();
			temp.factors.x = static_cast<float>(index);
			temp.ColliderBoxWorld = r->Matrix();
			//D3DXMatrixTranspose(&temp.ColliderBoxWorld, &tempMat);
			temp.local = r->Matrix();
			//D3DXMatrixTranspose(&temp.local, &tempMat2);
			//temp.actorIndex = actorIndex;
			colliderBoxData[(actorIndex*3) + i] = temp;
		}

		
	}


      SafeRelease(boneBoxTexture);
      SafeRelease(boneBoxSrv);
      //CreateTexture
      {
          	D3D11_TEXTURE2D_DESC desc;
          	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
          	desc.Width = 9*3;
          	desc.Height = MAX_ACTOR_COUNT;
          
          	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
          	desc.Usage = D3D11_USAGE_IMMUTABLE;
          	//desc.Usage = D3D11_USAGE_DYNAMIC;
          //	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
          	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
          	desc.MipLevels = 1;
          	desc.ArraySize = 1;
          	desc.SampleDesc.Count = 1;
          
          
          
          
          
          	D3D11_SUBRESOURCE_DATA subResource;
          	subResource.pSysMem = colliderBoxData.data();
          	subResource.SysMemPitch = sizeof(BoneBoxDesc)*3;
          	subResource.SysMemSlicePitch = 0;
          
          	Check(device->CreateTexture2D(&desc, &subResource, &boneBoxTexture));
      }
      
      
      //Create SRV
      {
         	D3D11_TEXTURE2D_DESC desc;
         	boneBoxTexture->GetDesc(&desc);
         
         	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
         	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
         	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
         	srvDesc.Texture2D.MipLevels = 1;
         	srvDesc.Format = desc.Format;
         
         	Check(device->CreateShaderResourceView(boneBoxTexture, &srvDesc, &boneBoxSrv));
      }
}

void Animator::ReadBehaviorTree(BinaryReader * r, const uint & actorIndex)
{
	transforms->ReadBehaviorTree(r, actorIndex);
}
