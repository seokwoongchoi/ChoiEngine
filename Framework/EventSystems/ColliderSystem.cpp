#include "Framework.h"
#include "ColliderSystem.h"
#include "Resources/Mesh.h"
#include "Resources/TextureTransforms.h"
#include "Core/D3D11/D3D11_Helper.h"
#include "Renderer.h"
#include "Viewer/Frustum.h"
ColliderSystem::ColliderSystem(ID3D11Device * device, const string& path, const string& entryPoint)
	:device(device),  texture(nullptr), srv(nullptr), actorCount(0), boneTexture(nullptr), ColliderCS(nullptr),
	InstBuffer(nullptr), InstBufferSRV(nullptr), instTexture(nullptr), frustum(nullptr),drawBuffer(nullptr)
{
	frustum = new Frustum();
	instTexture= new InstTransform();
	CreateInstTransformSRV();
	
	for (uint a = 0; a < MAX_ACTOR_COUNT; a++)
		for (UINT i = 0; i < MAX_MODEL_INSTANCE; i++)
		{
			D3DXMatrixIdentity(&instTexture->instTransforms[a][i]);

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

void ColliderSystem::SortInstMatrix(const uint & actorIndex, const uint & drawIndex)
{
	Matrix temp = instTexture->instTransforms[actorIndex][drawIndex];
	uint count = renderers[actorIndex]->drawCount>0? renderers[actorIndex]->drawCount - 1:0;
	for(uint i= drawIndex;i< count;i++)
	instTexture->instTransforms[actorIndex][i] = instTexture->instTransforms[actorIndex][i+1];

	instTexture->instTransforms[actorIndex][count] = temp;

	ID3D11DeviceContext* context = nullptr;
	device->GetImmediateContext(&context);

	CreateInstTransformSRV();
}

const int& ColliderSystem::FrustumCulling(const uint& index)
{
	frustum->Update();
	const Vector3& min = renderers[index]->boxMin;
	const Vector3& max = renderers[index]->boxMax;
	temp[0] = Vector3(min.x, min.y, max.z);
	temp[1] = Vector3(max.x, min.y, max.z);
	temp[2] = Vector3(min.x, max.y, max.z);
	temp[3] = Vector3(max);
	temp[4] = Vector3(min);
	temp[5] = Vector3(max.x, min.y, min.z);
	temp[6] = Vector3(min.x, max.y, min.z);
	temp[7] = Vector3(max.x, max.y, min.z);

	uint count = renderers[index]->drawCount+ culledCount[index];
	for (uint i = 0; i < count; i++)
	{
		for (uint b = 0; b < 8; b++)
		{
			D3DXVec3TransformCoord(&dest[b], &temp[b], &instTexture->instTransforms[index][i]);
		}

		bool inFrustum = frustum->ContainCube((dest[4] + dest[3])*0.5f, (dest[3].x - dest[4].x)*0.5f);
		if (inFrustum == false)
		{
			if (i >= renderers[index]->drawCount) continue;

			culledCount[index]++;
			
			SortInstMatrix(index,i);
			renderers[index]->drawCount--;
			cout << "culled" << endl;
			return i;
		}
		else
		{
			if(i>= renderers[index]->drawCount)
			{
				renderers[index]->drawCount++;
				culledCount[index]--;
				cout << "InFrustum" << endl;
				return -2;
			}
		}
		
	}
	return -1;
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
		D3DXVec3TransformCoord(&dest[b], &temp[b], &instTexture->instTransforms[actorIndex][drawCount]);
	}
	

	return &dest[0];
}

const Matrix & ColliderSystem::GetInstMatrix(const uint & index)
{
	return instTexture->instTransforms[index][renderers[index]->drawCount - 1];
}

void ColliderSystem::SetInstMatrix(const uint & index, const Matrix & world)
{
	instTexture->instTransforms[index][renderers[index]->drawCount-1] = world;
	CreateInstTransformSRV();
}

const uint & ColliderSystem::DrawCount(const uint & index)
{
	
	return renderers[index]->drawCount;
	
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
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
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
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;


	


		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = instTexture->instTransforms;
		subResource.SysMemPitch = sizeof(Matrix)*MAX_MODEL_INSTANCE;
		subResource.SysMemSlicePitch = 0;

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
	SafeDelete(instTexture);
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
	instTexture->instTransforms[index][renderers[index]->drawCount] = world;
	
	

	CreateInstTransformSRV();
	renderers[index]->drawCount++;
	/*ID3D11DeviceContext * context;
	device->GetImmediateContext(&context);


	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(InstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, instTexture->instTransforms, sizeof(Matrix)*MAX_ACTOR_COUNT*MAX_MODEL_INSTANCE);
	context->Unmap(InstBuffer, 0);*/
}
