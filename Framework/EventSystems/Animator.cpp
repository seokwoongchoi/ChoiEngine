#include "Framework.h"
#include "Animator.h"
#include "Renderers/Renderer.h"

#include "Resources/TextureTransforms.h"
#include "Viewer/Frustum.h"
#include "Transforms.h"
#include "PhysicsSystem.h"
#include "Utility/QuadTree.h"
Animator::Animator(ID3D11Device * device)
	:ColliderSystem(device, "../_Shaders/ComputeShaders/ColliderCS.hlsl", "SkeletalColliderCS"),device(device),
	 maxkeyframe(0), maxBoneCount(0), tweenBuffer(nullptr), tweenData{},
	boneBoxTexture(nullptr), boneBoxSrv(nullptr), transforms(nullptr), tree(nullptr),
	outputTexture(nullptr),outputSRV(nullptr),outputUAV(nullptr),
	 IsEdittedClip{}

{
	for (uint i = 0; i < MAX_SKELETAL_ACTOR_COUNT; i++)
	{
		IsFirstClipRead[i] = true;
	}
	tree = new class QuadTree();

	//collider = new ColliderSystem(device, "../_Shaders/ComputeShaders/ColliderCS.hlsl", "SkeletalColliderCS");
	boneTransfomrs = new SkinnedTransform[MAX_SKELETAL_ACTOR_COUNT* MAX_CLIP_SIZE];

	
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(TweenDesc)*MAX_MODEL_INSTANCE;
	Check(device->CreateBuffer(&bufferDesc, NULL, &tweenBuffer));


	
	
	tweenDesc.assign(MAX_MODEL_INSTANCE+5, TweenDesc());
	tweenData.assign(MAX_MODEL_INSTANCE+5, TweenData());
	
	colliderBoxData.assign(MAX_SKELETAL_ACTOR_COUNT*3, BoneBoxDesc());

	transforms = new Transforms(device,this);
	
	for (uint i = 0; i < MAX_MODEL_INSTANCE; i++)
	{
		
		tweenData[i].index = i;
	
		
	}
	


	//SafeRelease(animationBuffer);
	//SafeRelease(animationSRV);
	//SafeRelease(animationUAV);


	//// Create Structured Buffer
	//D3D11_BUFFER_DESC sbDesc;

	//sbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	//sbDesc.CPUAccessFlags = 0;
	//sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	//sbDesc.StructureByteStride = sizeof(CS_AnimOutputDesc);
	//sbDesc.ByteWidth = sizeof(CS_AnimOutputDesc) *outSize;
	//sbDesc.Usage = D3D11_USAGE_DEFAULT;
	//Check(D3D::GetDevice()->CreateBuffer(&sbDesc, &Data, &out_StructuredBuffer));

	//// create the Shader Resource View (SRV) for the structured buffer
	//D3D11_SHADER_RESOURCE_VIEW_DESC sbSRVDesc;

	//sbSRVDesc.Buffer.ElementOffset = 0;
	//sbSRVDesc.Buffer.ElementWidth = sizeof(CS_AnimOutputDesc);
	//sbSRVDesc.Buffer.FirstElement = 0;
	//sbSRVDesc.Buffer.NumElements = outSize;
	//sbSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	//sbSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	//Check(D3D::GetDevice()->CreateShaderResourceView(out_StructuredBuffer, &sbSRVDesc, &out_StructuredBufferSRV));
	//
	//// create the UAV for the structured buffer
	//D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
	//sbUAVDesc.Buffer.FirstElement = 0;
	//sbUAVDesc.Buffer.Flags = 0;
	//sbUAVDesc.Buffer.NumElements = outSize;
	//sbUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	//sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	//Check(D3D::GetDevice()->CreateUnorderedAccessView(out_StructuredBuffer, &sbUAVDesc, &out_StructuredBufferUAV));

}

Animator::~Animator()
{
}





void Animator::PushDrawCount(const uint & index, const Matrix & world)
{
	if (renderDatas.empty() || renderDatas.size() <= index || renderDatas[index].drawCount >= MAX_MODEL_INSTANCE) return;
	uint temp = 0;
	for (uint a = 0; a < actorCount; a++)
	{

		temp += renderDatas[a].drawCount;

	}
	if (temp >= 15)return;
	instTransforms[index].emplace_back(world);


	
	tree->Intersection(&instTransforms[index][renderDatas[index].drawCount], temp);
	renderDatas[index].drawCount++;
}




uint Animator::FrustumCulling(const uint & index)
{
	renderDatas[index].prevDrawCount = 0;
	frustum->Update();

	
	
	
	for (uint i = 0; i < renderDatas[index].drawCount; i++)
	{
		D3DXVec3TransformCoord(&min, &renderDatas[index].boxMin, &instTransforms[index][i]);
		D3DXVec3TransformCoord(&max, &renderDatas[index].boxMax, &instTransforms[index][i]);

		bool inFrustum = frustum->ContainRect((max + min)*0.5f, (max - min)*0.5f);
		if (inFrustum)
		{

			uint one_Index = renderDatas[index].prevDrawCount;
			uint tempIndex = i;
			renderDatas[index].prevDrawCount++;

			if (index > 0)
			{
				for (uint p = 0; p < index; p++)
				{
					one_Index += renderDatas[p].prevDrawCount;
					tempIndex += renderDatas[p].drawCount;
				}
			}


			memcpy(&OneDimensionalInstTransforms[one_Index], &instTransforms[index][i], sizeof(Matrix));


			tweenDesc[one_Index].TweenTime = tweenData[tempIndex].TweenTime;
			tweenDesc[one_Index].index = tweenData[tempIndex].index;

			tweenDesc[one_Index].Curr = tweenData[tempIndex].Curr;
			tweenDesc[one_Index].Next = tweenData[tempIndex].Next;
			
		}
	}
	return   renderDatas[index].prevDrawCount;
	
}
	
void Animator::BindPipeline(ID3D11DeviceContext * context)
{
	ID3D11ShaderResourceView* srvArray[2] = { InstBufferSRV,outputSRV };
	context->VSSetShaderResources(0, 2, srvArray);
}

uint Animator::AnimationUpdate(const uint& actorIndex)
{
	uint start = 0;
	if (actorIndex > 0)
	{
		for (uint i = 0; i < actorIndex; i++)
		{
			start += renderDatas[i].drawCount;
		}
		
	}
	uint end = start+renderDatas[actorIndex].drawCount;
	for (uint index = start; index < end; index++)
	{
		auto& tween = tweenData[index];
		const uint& currAnimation = tween.Curr.Clip;
		const uint& clipIndex = actorIndex * MAX_CLIP_SIZE + currAnimation;
		const auto& clip = clips[clipIndex];

		if (tween.IsStopped == true)
			continue;
		

		if (tween.Curr.Clip == static_cast<uint>(ActorState::Die))
		{

			auto currClip = actorIndex * MAX_CLIP_SIZE + static_cast<uint>(tween.state);
			if (tween.Curr.NextFrame >= clips[currClip]->Duration() - 1)
			{
				tween.Curr.CurrFrame = static_cast<uint>(clips[currClip]->Duration()) - 2;
				tween.Curr.NextFrame = static_cast<uint>(clips[currClip]->Duration()) - 1;
				tween.Curr.Clip = 9;
				continue;
			}
		}
		else
		{
			if (tween.Curr.NextFrame >= clip->Duration() - 1)
			{
				tween.speed = 1.0f;
				tween.state = ActorState::Idle;
			}
		}

	

		{
			tween.RunningTime += Time::Delta();
			
			float time = 1.0f / clip->FrameRate() / tween.speed;
			if (tween.RunningTime >= time)
			{
				tween.RunningTime = 0.0f;

				tween.Curr.CurrFrame = (tween.Curr.CurrFrame + 1) % clip->FrameCount();
				tween.Curr.NextFrame = (tween.Curr.CurrFrame + 1) % clip->FrameCount();
			}
			tween.Curr.Time = tween.RunningTime / time;
	
		}
		
		
		

	
		
	}
	


	
		
	return FrustumCulling(actorIndex);
}

void Animator::UpdateNextAnim(const uint & actorIndex)
{
	uint start = 0;
	if (actorIndex > 0)
	{
		for (uint i = 0; i < actorIndex; i++)
		{
			start += renderDatas[i].drawCount;
		}

	}
	uint end = start + renderDatas[actorIndex].drawCount;
	for (uint index = start; index < end; index++)
	{
		auto& tween = tweenData[index];
		const uint& currAnimation = tween.Curr.Clip;
		const uint& clipIndex = actorIndex * MAX_CLIP_SIZE + currAnimation;
		const auto& clip = clips[clipIndex];

		PlayNextClip(index, static_cast<uint>(tween.state));
	
		if (tween.Next.Clip > -1)
		{
			const uint& nextClipIndex = actorIndex * MAX_CLIP_SIZE + tween.Next.Clip;
			const auto& nextClip = clips[nextClipIndex];

			tween.RunningTime = 0.0f;
			tween.TweenTime += Time::Delta();
			tween.TweenTime *= 1.8f;
			if (tween.TweenTime >= 1.0f)
			{
				tween.Curr = tween.Next;

				tween.Next.Clip = -1;
				tween.Next.CurrFrame = 0;
				tween.Next.NextFrame = 0;
				tween.Next.Time = 0;
				tween.RunningTime = 0.0f;


				tween.TweenTime = 0.0f;
			}
			else
			{
				tween.RunningTime += Time::Delta();

				float time = 1.0f / nextClip->FrameRate() / tween.speed;
				if (tween.Next.Time >= 1.0f)
				{
					tween.RunningTime = 0;

					tween.Next.CurrFrame = (tween.Next.CurrFrame + 1) % nextClip->FrameCount();
					tween.Next.NextFrame = (tween.Next.CurrFrame + 1) % nextClip->FrameCount();
				}
				tween.Next.Time = tween.RunningTime / time;
			}
		}



	}


}

void Animator::Update()
{
	for(uint a=0;a<actorCount;a++)
	for (uint i = 0; i < renderDatas[a].drawCount; i++)
	{
		uint index = i;
		if (a > 0)
		{
			for (uint t = 0; t < a; t++)
			{
				index += renderDatas[t].drawCount;
			}

		}
		if (tweenData[index].state == ActorState::Die)continue;
		tree->Intersection(&instTransforms[a][i], tweenData[index].quadTreeID);
		if (a < 1 && i < 1)continue;
		transforms->Update(a, i);
	}
	
}

void Animator::Update(bool bstart)
{
	for (uint a = 0; a < actorCount; a++)
		for (uint i = 0; i < renderDatas[a].drawCount; i++)
		{
			uint index = i;
			if (a > 0)
			{
				for (uint t = 0; t < a; t++)
				{
					index += renderDatas[t].drawCount;
				}

			}
			if (tweenData[index].state == ActorState::Die)continue;
			tree->Intersection(&instTransforms[a][i], tweenData[index].quadTreeID);

			if (bstart)
			{
				if (a < 1 && i < 1)continue;
				transforms->Update(a, i);
			}
			
		}
}

void Animator::Compute(ID3D11DeviceContext * context,ID3D11UnorderedAccessView* physicsUAV)
{
	if (Keyboard::Get()->Down('K'))
	{
		cout << "" << endl;
		uint temp = 0;
		for (uint a = 0; a < actorCount; a++)
		{
			temp += renderDatas[a].drawCount;
		}

		for (uint a = 0; a < actorCount; a++)
		{
			for (uint i = 0; i < renderDatas[a].drawCount; i++)
			{
				uint index = i;
				if (a > 0)
				{
					for (uint t = 0; t < a; t++)
					{
						index += renderDatas[t].drawCount;
					}

				}

				

				cout << to_string(index) + "Actor : ";
				cout << to_string(tweenData[index].quadTreeID);
				if (index == temp - 1)
				{
					cout << "" << endl;
				}
				else
				{
					cout << ", ";
				}
			}

		}

	}


	totalCount = 0;
	for (uint i = 0; i < actorCount; i++)
	{
		tweenDesc[i].drawCount = renderDatas[i].prevDrawCount;
		tweenDesc[i].maxBoneCount = tweenData[i].maxBoneCount;

		totalCount += renderDatas[i].prevDrawCount;
		
	}
	
	
	if (totalCount <1)return;

	{
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(InstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &OneDimensionalInstTransforms[0], sizeof(Matrix) * totalCount);
		context->Unmap(InstBuffer, 0);
	}
		
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

	ID3D11UnorderedAccessView* uavArray[2] = { outputUAV,physicsUAV };
	context->CSSetUnorderedAccessViews(0, 2, uavArray,nullptr);

	context->CSSetShader(ColliderCS, nullptr, 0);
	context->Dispatch(1, actorCount, 1);


	ZeroMemory(&bufferArray, sizeof(bufferArray));
	context->CSSetConstantBuffers(2, 1, bufferArray);
	context->CSSetShader(nullptr, nullptr, 0);
	ZeroMemory(srvArray, sizeof(srvArray));
	context->CSSetShaderResources(0, 3, srvArray);
	ZeroMemory(uavArray, sizeof(uavArray));

	context->CSSetUnorderedAccessViews(0, 2, uavArray, nullptr);
	
}

void Animator::ClearTextureTransforms()
{
	for (UINT i = 0; i < clips.size(); i++)
		boneTransfomrs[i].Clear();

	boneTransfomrs[0].bones.clear();
	SafeDeleteArray(boneTransfomrs);

	shadowMtrix.clear();
	shadowMtrix.shrink_to_fit();
	
	for (uint i = 0; i < clips.size(); i++)
	{
		clips[i]->keyframeMap.clear();
		
	}
}


void Animator::AnimTransformSRV()
{
	
	for (UINT i = 0; i < clips.size(); i++)
		boneTransfomrs[i].Clear();


	//Thread::Get()->AddTask([this]()
	//{
		SafeRelease(texture);
		SafeRelease(srv);

		SafeRelease(outputTexture);
		SafeRelease(outputSRV);
		SafeRelease(outputUAV);


		const uint& clipSize = static_cast<uint>(clips.size());
		uint boneCount = 0; 
	

			
			if (!clips.empty())
			{
				for (uint i = 0; i < clipSize; i++)
				{
					uint boneActorIndex = static_cast<uint>(i / MAX_CLIP_SIZE);
					boneCount = static_cast<uint>(boneTransfomrs[0].bones[boneActorIndex].size());
					maxBoneCount = max(maxBoneCount, boneCount);
					
				}
				for (uint i = 0; i < clipSize; i++)
				{
					uint boneActorIndex = static_cast<uint>(i / MAX_CLIP_SIZE);
					boneTransfomrs[i].CreateTransforms(clips[i]->FrameCount(), maxBoneCount);
					maxkeyframe = max(maxkeyframe, clips[i]->FrameCount());
					
					tweenData[boneActorIndex].maxBoneCount = static_cast<uint>(boneTransfomrs[0].bones[boneActorIndex].size());
				}
				
				Matrix* saveTransforms = new Matrix[boneCount];
			

				for (UINT i = 0; i < clipSize; i++)
				{
					auto& saveParentMatrix = boneTransfomrs[i].saveTransforms;
					uint boneActorIndex = static_cast<uint>(i / MAX_CLIP_SIZE);
					for (UINT f = 0; f < clips[i]->FrameCount(); f++)
					{
						for (UINT b = 0; b < boneTransfomrs[0].bones[boneActorIndex].size(); b++)
						{
							auto& bone = boneTransfomrs[0].bones[boneActorIndex][b];

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
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;



				uint pageSize = maxBoneCount * 4 * 16 * maxkeyframe;
				void* p = malloc(pageSize * clipSize);

				//for(uint a=0;a<actorCount;a++)
				for (UINT c = 0; c <clips.size(); c++)
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
					subResource[c].SysMemPitch = maxBoneCount * sizeof(Matrix);
					subResource[c].SysMemSlicePitch = pageSize;
				}





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


				Check(device->CreateShaderResourceView(texture, &srvDesc, &srv));
					
			}
		}

		//Create Texture
			{
		        D3D11_TEXTURE2D_DESC desc;
		        ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		        desc.Width =  4*maxBoneCount;
		        desc.Height = MAX_MODEL_INSTANCE;
		        desc.MipLevels = 1;
		        desc.ArraySize = 1;
		        desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		        desc.SampleDesc.Count = 1;
		        desc.Usage = D3D11_USAGE_DEFAULT;
		        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		        
		        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

				

			    Check(device->CreateTexture2D(&desc, nullptr, &outputTexture));
			
			}

			//Create SRV
			{
				D3D11_TEXTURE2D_DESC desc;
				outputTexture->GetDesc(&desc);

				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
				ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				srvDesc.Format = desc.Format;
				
			
				Check(device->CreateShaderResourceView(outputTexture, &srvDesc, &outputSRV));


				// Create the UAVs

				D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
				sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
				sbUAVDesc.Buffer.FirstElement = 0;
				sbUAVDesc.Buffer.Flags = 0;
				sbUAVDesc.Buffer.NumElements = maxBoneCount * 4 * MAX_MODEL_INSTANCE;
				sbUAVDesc.Format = desc.Format;
				
				Check(device->CreateUnorderedAccessView(outputTexture, &sbUAVDesc, &outputUAV));
			}
}

void Animator::ReadClip(const wstring & name, const uint& actorIndex)
{
	
	if (IsFirstClipRead[actorIndex]==true|| IsEdittedClip[actorIndex]==true)
	{
		
		wstring filePath = L"../_Models/SkeletalMeshes/" + name + L"/";
		vector<wstring> files;

		wstring filter = L"*.clip";
		Path::GetFiles(&files, filePath, filter, false);

		for (uint i = 0; i < files.size(); i++)
		{
			auto clipfileName = Path::GetFileName(files[i]);
			const auto& file = L"../_Models/SkeletalMeshes/" + name + L"/" + clipfileName;

			BinaryReader* r = new BinaryReader();
			r->Open(file);
			const auto& clip = make_shared< ModelClip>();

			clip->name = r->String();
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

			if (IsEdittedClip[actorIndex] == true)
			{
				uint start = MAX_CLIP_SIZE * actorIndex;
				clips[start+i] = clip;
			}
			else
			{
				clips.emplace_back(clip);
			}
			
		}


		
		
	}

	bool temp =false;
	for (uint i = 0; i < actorCount; i++)
	{
		temp |= IsEdittedClip[actorIndex];
	}

	if (IsFirstClipRead[actorIndex] == true || temp == true)
	{
		AnimTransformSRV();
		IsFirstClipRead[actorIndex] = false;
	}
	
}

void Animator::IintAnimData()
{
	uint temp = 0;
	for (uint a = 0; a < actorCount; a++)
	{
		
		temp += renderDatas[a].drawCount;
		
	}
	for (uint i = 0; i < temp; i++)
	{
		tweenData[i].state = ActorState::Idle;
	
		tweenData[i].Curr.Clip = 0;
		tweenData[i].Curr.CurrFrame = 0;
		tweenData[i].Curr.NextFrame = 0;
		tweenData[i].Curr.Time = 0;
		
		tweenData[i].Next.Clip = -1;
		tweenData[i].Next.CurrFrame = 0;
		tweenData[i].Next.NextFrame = 0;
		tweenData[i].Next.Time = 0;
		

		tweenData[i].TweenTime = 0.0f;
	}
}



void Animator::ReadBone(BinaryReader * r,uint actorIndex)
{
	uint tempBoneCount = r->UInt();

	this->actorIndex = actorIndex;
	boneTransfomrs[0].bones[actorIndex].clear();
	boneTransfomrs[0].bones[actorIndex].shrink_to_fit();

	for (UINT i = 0; i < tempBoneCount; i++)
	{
		const auto& bone = make_shared<ModelBone>();
		bone->index = r->Int();
		bone->name = r->String();
		bone->parentIndex = r->Int();
		bone->transform = r->Matrix();
		boneTransfomrs[0].bones[actorIndex].emplace_back(bone);

	}

	

	for_each(boneTransfomrs[0].bones[actorIndex].begin(), boneTransfomrs[0].bones[actorIndex].end(), [&](shared_ptr<ModelBone>bone)
	{
		if (bone->parentIndex > -1 && bone->parent == nullptr)
		{

			bone->parent = boneTransfomrs[0].bones[actorIndex][bone->parentIndex];
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
			temp.local = r->Matrix();
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
          	desc.Height = actorCount;
          
          	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
          	desc.Usage = D3D11_USAGE_IMMUTABLE;
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
