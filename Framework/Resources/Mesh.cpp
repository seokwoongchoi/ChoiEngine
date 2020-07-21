#include "Framework.h"
#include "Mesh.h"


Mesh::Mesh()
	:material(nullptr),bone(nullptr),nullBuffer(nullptr), vertexCount(0), startVertexIndex(0), name(""),
	startIndex(0), indexCount(0), materialName(""), minPos(Vector3(0, 0, 0)), maxPos(Vector3(0, 0, 0)), boneIndex(0), boneDesc{},
	 boneBuffer(nullptr)
{


}


Mesh::~Mesh()
{
	
	//SafeDelete(vertexBuffer);
	//SafeDelete(indexBuffer);
	SafeRelease(boneBuffer);
}

void Mesh::CreateBuffer(ID3D11Device* device)
{
	SafeRelease(boneBuffer);
	
	// Allocate constant buffers
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(BoneDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &boneBuffer));
	
}


void Mesh::ApplyPipeline(ID3D11DeviceContext * context)
{
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(boneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		
	memcpy(MappedResource.pData, &boneIndex, sizeof(boneIndex));
	
	context->Unmap(boneBuffer, 0);
	context->VSSetConstantBuffers(2, 1, &boneBuffer);

	//Render Material
	
	material->ApplyMaterial(context);


	
}

void Mesh::ApplyPipelineNoMaterial(ID3D11DeviceContext * context)
{
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(boneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &boneIndex, sizeof(int));
	context->Unmap(boneBuffer, 0);
	context->VSSetConstantBuffers(2, 1, &boneBuffer);
}

void Mesh::ApplyPipelineReflectionMaterial(ID3D11DeviceContext * context)
{
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(boneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	
	memcpy(MappedResource.pData, &boneIndex, sizeof(boneIndex));


	context->Unmap(boneBuffer, 0);
	context->VSSetConstantBuffers(2, 1, &boneBuffer);

	//Render Material

	material->ApplyRefeltionMaterial(context);

}

void Mesh::ClearPipeline(ID3D11DeviceContext * context)
{
	
	context->VSSetConstantBuffers(2, 1, &nullBuffer);
	
}

void Mesh::ClearPipelineMaterial(ID3D11DeviceContext * context)
{
	material->ClearMaterial(context);
}

void Mesh::ClearPipelineReflection(ID3D11DeviceContext * context)
{
	material->ClearReflectionMaterial(context);
}

