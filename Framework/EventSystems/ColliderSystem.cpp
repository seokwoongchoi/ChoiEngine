#include "Framework.h"
#include "ColliderSystem.h"
#include "Resources/Mesh.h"
#include "Resources/TextureTransforms.h"
#include "Core/D3D11/D3D11_Helper.h"

#include "Viewer/Frustum.h"
#include "PhysicsSystem.h"

ColliderSystem::ColliderSystem(ID3D11Device * device, const string& path, const string& entryPoint)
	:device(device),  texture(nullptr), srv(nullptr), actorCount(0), boneTexture(nullptr), ColliderCS(nullptr),
	InstBuffer(nullptr), InstBufferSRV(nullptr), ShadowInstBuffer(nullptr), ShadowInstBufferSRV(nullptr), frustum(nullptr),drawBuffer(nullptr), totalCount(0)
{
	frustum = new Frustum();
	
	instTransforms.assign(MAX_ACTOR_COUNT, vector<Matrix>());
	//for (uint a = 0; a < MAX_ACTOR_COUNT; a++)
	//	for (UINT i = 0; i < MAX_MODEL_INSTANCE; i++)
	//	{
	//		D3DXMatrixIdentity(&instTransforms[a][i]);

	//	}//for(i)

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

	CreateInstTransformSRV();
}

uint ColliderSystem::FrustumCulling(const uint& index)
{
	 renderDatas[index].prevDrawCount = 0;
	 frustum->Update();
     for (uint i = 0; i < renderDatas[index].drawCount; i++)
	 {
		 D3DXVec3TransformCoord(&min, & renderDatas[index].boxMin, &instTransforms[index][i]);
		 D3DXVec3TransformCoord(&max, & renderDatas[index].boxMax, &instTransforms[index][i]);

		 bool inFrustum = frustum->ContainRect((max + min)*0.5f, (max - min)*0.5f);
		 if (inFrustum)
		 {
			 uint one_Index = renderDatas[index].prevDrawCount;
			 renderDatas[index].prevDrawCount++;
			
			 if (index > 0)
			 {
				 for (uint p = 0; p < index; p++)
				 {
					 one_Index += renderDatas[p].prevDrawCount;
				 }
				
			 }

			 if (one_Index >= (MAX_MODEL_INSTANCE+5)) break;
			 memcpy(&OneDimensionalInstTransforms[one_Index], &instTransforms[index][i], sizeof(Matrix));
			
		 }
	 }
	 return renderDatas[index].prevDrawCount;
}

vector<Vector3> ColliderSystem::GetBoxMinMax(const uint & actorIndex, const uint & drawCount)
{
	vector<Vector3> dest;
	Vector3  temp[8];
	Vector3& min = renderDatas[actorIndex].boxMin;
	Vector3& max = renderDatas[actorIndex].boxMax;
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
		D3DXVec3TransformCoord(&temp[b], &temp[b], &instTransforms[actorIndex][drawCount]);
		dest.emplace_back(temp[b]);
	}
	
	return dest;
}





void ColliderSystem::CreateModelTransformSRV()
{
	if (actorCount == 0) return;

	SafeRelease(texture);
	SafeRelease(srv);

	
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
		
	
	
		for (UINT i = 0; i < actorCount; i++)
		{
			for (UINT b = 0; b < boneTexture->bones[i].size(); b++)
			{
				auto& bone = boneTexture->bones[i][b];

				Matrix parent;
				int parentIndex = bone->ParentIndex();

				if (parentIndex < 0)
					D3DXMatrixIdentity(&parent);

				else
					parent = boneTexture->temp[parentIndex];

				Matrix matrix = bone->Transform();
				boneTexture->temp[b] = parent;
				
				boneTexture->boneTransforms[i][b] = matrix * boneTexture->temp[b];

			}//for(b)
		}//for(i)

	
		


		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = boneTexture->boneTransforms;
		
		//subResource.SysMemPitch =  sizeof(Matrix);
		//subResource.SysMemSlicePitch = 0;
		subResource.SysMemPitch = MAX_BONE_TRANSFORMS * sizeof(Matrix) ;
		subResource.SysMemSlicePitch = 0;

		Check(device->CreateTexture2D(&desc, &subResource, &texture));
	
		//boneTexture->bones[actorIndex].clear();
		//boneTexture->bones[actorIndex].shrink_to_fit();
	
		

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
		desc.Width = (MAX_MODEL_INSTANCE+5) *4;
		desc.Height = 1;

		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		//desc.Usage = D3D11_USAGE_DEFAULT;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;


	
		Check(device->CreateTexture2D(&desc, nullptr, &InstBuffer));
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

void ColliderSystem::CreateShadowInstTransformSRV()
{
	shadowMtrix.clear();
	shadowMtrix.shrink_to_fit();
	SafeRelease(ShadowInstBuffer);
	SafeRelease(ShadowInstBufferSRV);

	
	uint temptotalCount = 0;
	for (uint i = 0; i < renderDatas.size(); i++)
	{
		temptotalCount += renderDatas[i].drawCount;
	}

	for (uint a = 0; a < actorCount; a++)
	{
		for (uint i = 0; i < renderDatas[a].drawCount; i++)
		{
			shadowMtrix.emplace_back(instTransforms[a][i]);
		}
	}

	
	
	//CreateTexture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.Width = temptotalCount * 4;
		desc.Height = 1;

		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;



		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = shadowMtrix.data();
		subResource.SysMemPitch = sizeof(Matrix)*temptotalCount;
		subResource.SysMemSlicePitch = 0;

		Check(device->CreateTexture2D(&desc, &subResource, &ShadowInstBuffer));
	}

	//Create SRV
	{
		D3D11_TEXTURE2D_DESC desc;
		ShadowInstBuffer->GetDesc(&desc);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = desc.Format;

		Check(device->CreateShaderResourceView(ShadowInstBuffer, &srvDesc, &ShadowInstBufferSRV));
	}

}

void ColliderSystem::BindingBone()
{
   for_each(boneTexture->bones[actorIndex].begin(), boneTexture->bones[actorIndex].end(),[&](shared_ptr<ModelBone>bone)
   {
		if (bone->parentIndex > -1&& bone->parent == nullptr)
		{
			
			bone->parent = boneTexture->bones[actorIndex][bone->parentIndex];
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
	shadowMtrix.clear();
	shadowMtrix.shrink_to_fit();
}



void ColliderSystem::BindPipeline(ID3D11DeviceContext * context)
{
	

	ID3D11ShaderResourceView* srvArray[2] = { InstBufferSRV,srv };
	context->VSSetShaderResources(0, 2, srvArray);
	
}

void ColliderSystem::UpdateInstTransformSRV(ID3D11DeviceContext * context)
{
	totalCount = 0;
	for (uint i = 0; i < actorCount; i++)
	{
		totalCount += renderDatas[i].prevDrawCount;
	}

	if (totalCount < 1)return;

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(InstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, OneDimensionalInstTransforms, sizeof(Matrix) * totalCount);
	context->Unmap(InstBuffer, 0);
}

void ColliderSystem::ReadBone(BinaryReader * r, uint actorIndex)
{
	if (boneTexture == nullptr)
		boneTexture = new BoneTransform();

	boneTexture->bones[actorIndex].clear();
	boneTexture->bones[actorIndex].shrink_to_fit();

	this->actorIndex = actorIndex;
	uint tempBoneCount = r->UInt();

	for (UINT i = 0; i < tempBoneCount; i++)
	{
		const auto& bone = make_shared<ModelBone>();
		bone->index = r->Int();
		bone->name = r->String();
		bone->parentIndex = r->Int();
		bone->transform = r->Matrix();
		boneTexture->bones[this->actorIndex].emplace_back(bone);
		
	}
	
	BindingBone();

}


void ColliderSystem::RegisterRenderData(const uint& index,class Renderer* renderer)
{
	RenderData data;
	if (renderer)
	{
		data.boxMin = renderer->boxMin;
		data.boxMax = renderer->boxMax;
	}
	
	
	if (renderDatas.empty()&&index==0)
	{
		
		renderDatas.emplace_back(data);
	}
	else if(renderDatas.size()>index)
	{
		data.drawCount = renderDatas[index].drawCount;
		data.prevDrawCount = renderDatas[index].prevDrawCount;
		data.saveCount = renderDatas[index].saveCount;
	
		renderDatas[index] = data;
	}
	else 
	{
		renderDatas.emplace_back(data);
	}
	
}

void ColliderSystem::PushDrawCount(const uint & index, const Matrix & world, bool bCrateShadowBuffer)
{
	if (renderDatas.empty()|| renderDatas.size()<=index|| renderDatas[index].drawCount >= MAX_MODEL_INSTANCE) return;

	instTransforms[index].emplace_back(world);
	
	renderDatas[index].drawCount++;
	
	if(bCrateShadowBuffer)
	CreateShadowInstTransformSRV();
	

}
