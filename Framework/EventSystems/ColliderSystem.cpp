#include "Framework.h"
#include "ColliderSystem.h"
#include "Resources/Mesh.h"
#include "Resources/TextureTransforms.h"
#include "Core/D3D11/D3D11_Helper.h"

#include "Viewer/Frustum.h"
#include "PhysicsSystem.h"

ColliderSystem::ColliderSystem(ID3D11Device * device, const string& path, const string& entryPoint)
	:device(device),  texture(nullptr), srv(nullptr), actorCount(0), boneTexture(nullptr), ColliderCS(nullptr),
	InstBuffer(nullptr), InstBufferSRV(nullptr), frustum(nullptr),drawBuffer(nullptr), totalCount(0)
{
	frustum = new Frustum();
	
	CreateInstTransformSRV();
	
	for (uint a = 0; a < MAX_ACTOR_COUNT; a++)
		for (UINT i = 0; i < MAX_MODEL_INSTANCE; i++)
		{
			D3DXMatrixIdentity(&instTransforms[a][i]);

		}//for(i)
	//////////////////////////////////////////////////////////////////////////
	ID3DBlob* ShaderBlob = nullptr;
	//auto& path = "../_Shaders/ComputeShaders/ColliderCS.hlsl";
	//auto& entryPoint = "StaticColliderCS";
	auto& shaderModel = "cs_5_0";
	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));


	Check( device->CreateComputeShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&ColliderCS
	));
	
	SafeRelease(ShaderBlob);



	///////////////////////////////////////////////////////////////////////////////////////////////////
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(Vector4);
	Check(device->CreateBuffer(&bufferDesc, NULL, &drawBuffer));
}


//void ColliderSystem::UpdateInstBuffer()
//{
//	ID3D11DeviceContext * context;
//	device->GetImmediateContext(&context);
//
//
//	D3D11_MAPPED_SUBRESOURCE MappedResource;
//	context->Map(InstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
//	memcpy(MappedResource.pData, instTransforms, sizeof(Matrix)*actorCount*MAX_MODEL_INSTANCE);
//	context->Unmap(InstBuffer, 0);
//}
//


void ColliderSystem::FrustumCulling(const uint& index)
{
	
	//if (renderers[index]->drawCount == 0)return;

	frustum->Update();

	
	 const uint& count = renderers[index]->drawCount;


	 bool bChanged = false;
	

	 for (uint i = 0; i < count; i++)
	 {


		 D3DXVec3TransformCoord(&min, &renderers[index]->boxMin, &instTransforms[index][i]);
		 D3DXVec3TransformCoord(&max, &renderers[index]->boxMax, &instTransforms[index][i]);

		 bool inFrustum = frustum->ContainRect((max + min)*0.5f, (max - min)*0.5f);
		 if (inFrustum == false)
		 {



			 uint lastIndex = renderers[index]->drawCount - 1;

			 cout << "culled: ";
			 cout << to_string(i) << endl;


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
			
				
			 }


			 renderers[index]->drawCount--;

			 culledCount[index]++;

			 bChanged = true;



			 break;

		 }
	 }

	 const uint& totalCount = renderers[index]->drawCount + culledCount[index];


	 for (uint i = count; i < totalCount; i++)
	 {


		 D3DXVec3TransformCoord(&min, &renderers[index]->boxMin, &instTransforms[index][i]);
		 D3DXVec3TransformCoord(&max, &renderers[index]->boxMax, &instTransforms[index][i]);

		 bool inFrustum = frustum->ContainRect((max + min)*0.5f, (max - min)*0.5f);
		 if (inFrustum == true)
		 {
			 //if (i >= renderers[index]->drawCount)
			 {


				 cout << "InFrustum: ";
				 cout << to_string(i) << endl;

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
		 ID3D11DeviceContext* context;
		 device->GetImmediateContext(&context);
		 D3D11_MAPPED_SUBRESOURCE MappedResource;
		 context->Map(InstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		 memcpy(MappedResource.pData, instTransforms, sizeof(Matrix)*actorCount*MAX_MODEL_INSTANCE);
		 context->Unmap(InstBuffer, 0);
		

	 }
	//bool bChanged = false;
	//
	//for (uint i = 0; i < count; i++)
	//{
	//	D3DXVec3TransformCoord(&min, &renderers[index]->boxMin, &instTransforms[index][i]);
	//	D3DXVec3TransformCoord(&max, &renderers[index]->boxMax, &instTransforms[index][i]);


	//	bool inFrustum = frustum->ContainCube((max + min)*0.5f, (max.x - min.x)*0.5f);
	//	if (inFrustum == false)
	//	{
	//	

	//		if (i >= renderers[index]->drawCount) continue;

	//		cout << "culled: ";
	//		cout << to_string(i) << endl;

	//		uint culledDrawCount = renderers[index]->drawCount - 1;
	//		/*if (i == culledDrawCount)
	//		{
	//			renderers[index]->drawCount--;

	//			culledCount[index]++;
	//			break;
	//		}*/

	//	
	//		
	//		Matrix temp = instTransforms[index][i];
	//		memcpy(instTransforms[index][i], instTransforms[index][culledDrawCount], sizeof(Matrix));
	//		memcpy(instTransforms[index][culledDrawCount], temp, sizeof(Matrix));
	//		
	//				
	//		renderers[index]->drawCount--;
	//		culledCount[index]++;
	//	
	//	
	//		bChanged = true;
	//	}
	//	else
	//	{
	//		if (i >= renderers[index]->drawCount)
	//		{

	//			renderers[index]->drawCount++;
	//			culledCount[index]--;

	//			cout << "InFrustum: ";
	//			cout << to_string(i) << endl;

	//			
	//		}
	//	}

	//}
	//if(bChanged)
	//	UpdateInstBuffer();
	
	
	
}

Vector3 * ColliderSystem::GetBoxMinMax(const uint & actorIndex, const uint & drawCount)
{
	
	Vector3& min = renderers[actorIndex]->boxMin;
	Vector3& max = renderers[actorIndex]->boxMax;
	temp[0] = Vector3(min.x, min.y, max.z);
	temp[1] = Vector3(max.x, min.y, max.z);
	temp[2] = Vector3(min.x, max.y, max.z);
	temp[3] = Vector3(max);
	temp[4] = Vector3(min);
	temp[5] = Vector3(max.x, min.y, min.z);
	temp[6] = Vector3(min.x, max.y, min.z);
	temp[7] = Vector3(max.x, max.y, min.z);

	for (uint b = 0; b < 8; b++)
	{
		D3DXVec3TransformCoord(&dest[b], &temp[b], &instTransforms[actorIndex][drawCount]);
	}
	

	return &dest[0];
}




const uint & ColliderSystem::DrawCount(const uint & index)
{
	
	return renderers[index]->drawCount;
	
}

void ColliderSystem::RenderandCulledCount(const uint & index, uint& CulledCount)
{
	CulledCount = renderers[index]->drawCount + culledCount[index];
	
}



void ColliderSystem::CreateModelTransformSRV()
{
	if (actorCount == 0) return;

	

	SafeRelease(texture);
	SafeRelease(srv);


	const uint& boneCount = boneTexture-> bones.size();
	
	
	//CreateTexture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.Width = MAX_BONE_TRANSFORMS * 4;
		desc.Height = MAX_ACTOR_COUNT;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		//desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		
	
	
		//for (UINT i = 0; i < actorCount; i++)
		{
			for (UINT b = 0; b < boneCount; b++)
			{
				auto& bone = boneTexture->bones[b];

				Matrix parent;
				int parentIndex = bone->ParentIndex();

				if (parentIndex < 0)
					D3DXMatrixIdentity(&parent);

				else
					parent = boneTexture->temp[parentIndex];

				Matrix matrix = bone->Transform();
				boneTexture->temp[b] = parent;
				
				boneTexture->boneTransforms[actorCount-1][b] = matrix * boneTexture->temp[b];

			}//for(b)
		}//for(i)

	
		


		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = boneTexture->boneTransforms;
		
		//subResource.SysMemPitch =  sizeof(Matrix);
		//subResource.SysMemSlicePitch = 0;
		subResource.SysMemPitch = MAX_BONE_TRANSFORMS * sizeof(Matrix) ;
		subResource.SysMemSlicePitch = 0;

		Check(device->CreateTexture2D(&desc, &subResource, &texture));
		boneTexture->bones.clear();
		boneTexture->bones.shrink_to_fit();
	
		

		//free(p);
	}


	//Create SRV
	{
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = desc.Format;

		Check(device->CreateShaderResourceView(texture, &srvDesc, &srv));
	}

}

void ColliderSystem::CreateInstTransformSRV()
{
	SafeRelease(InstBuffer);
	SafeRelease(InstBufferSRV);
	//CreateTexture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.Width = MAX_MODEL_INSTANCE * 4;
		desc.Height = MAX_ACTOR_COUNT;

		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		//desc.Usage = D3D11_USAGE_DEFAULT;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;


	


		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = &instTransforms;
		subResource.SysMemPitch = sizeof(Matrix)*MAX_MODEL_INSTANCE;
		subResource.SysMemSlicePitch = MAX_ACTOR_COUNT;

		Check(device->CreateTexture2D(&desc, &subResource, &InstBuffer));
	}


	//Create SRV
	{
		D3D11_TEXTURE2D_DESC desc;
		InstBuffer->GetDesc(&desc);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = desc.Format;

		Check(device->CreateShaderResourceView(InstBuffer, &srvDesc, &InstBufferSRV));
	}

}

void ColliderSystem::BindingBone()
{
   for_each(boneTexture->bones.begin(), boneTexture->bones.end(),[&](shared_ptr<ModelBone>bone)
   {
		if (bone->parentIndex > -1&& bone->parent == nullptr)
		{
			
			bone->parent = boneTexture->bones[bone->parentIndex];
			bone->parent->childs.push_back(bone);
			
			
		}
		else
			bone->parent = NULL;
		
	});
	
	CreateModelTransformSRV();
}

void ColliderSystem::ClearTextureTransforms()
{
	//SafeDelete(instTexture);
	SafeDelete(boneTexture);
}



void ColliderSystem::BindPipeline(ID3D11DeviceContext * context)
{
	

	ID3D11ShaderResourceView* srvArray[2] = { InstBufferSRV,srv };
	context->VSSetShaderResources(0, 2, srvArray);
	
}

void ColliderSystem::ReadBone(BinaryReader * r)
{
	if (boneTexture == nullptr)
		boneTexture = new BoneTransform();

	uint tempBoneCount = r->UInt();

	for (UINT i = 0; i < tempBoneCount; i++)
	{
		const auto& bone = make_shared<ModelBone>();
		bone->index = r->Int();
		bone->name = r->String();
		bone->parentIndex = r->Int();
		bone->transform = r->Matrix();
		boneTexture->bones.emplace_back(bone);
		
	}
	
	BindingBone();

}

void ColliderSystem::RegisterRenderer(Renderer * renderer, const uint & index)
{
	
	if (renderers[index])
	{
		return;
	}
	renderers[index] = renderer;
	
	
}

void ColliderSystem::PushDrawCount(const uint & index, const Matrix & world)
{
	if (renderers[index]->drawCount >= MAX_MODEL_INSTANCE) return;

	instTransforms[index][renderers[index]->drawCount] = world;
	
	

	CreateInstTransformSRV();
	renderers[index]->drawCount++;
	/*ID3D11DeviceContext * context;
	device->GetImmediateContext(&context);


	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(InstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, instTexture->instTransforms, sizeof(Matrix)*MAX_ACTOR_COUNT*MAX_MODEL_INSTANCE);
	context->Unmap(InstBuffer, 0);*/
}
