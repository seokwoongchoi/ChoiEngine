#include "Framework.h"
#include "Mesh.h"


Mesh::Mesh()
	:material(nullptr),nullBuffer(nullptr), vertexCount(0), startVertexIndex(0), name(""),
	startIndex(0), indexCount(0), materialName(""), boneDesc{},
	 boneBuffer(nullptr), bHasMaterial(false)
{}


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
	memcpy(MappedResource.pData, &boneDesc, sizeof(boneDesc));
	context->Unmap(boneBuffer, 0);
	context->VSSetConstantBuffers(2, 1, &boneBuffer);

	//Render Material
	if (bHasMaterial)
		material->ApplyMaterial(context);
}


void Mesh::ApplyPipeline(ID3D11DeviceContext * context,const uint& prevDrawCount, const uint& actorIndex)
{
	boneDesc.prevDrawCount = prevDrawCount;
	boneDesc.actorIndex = actorIndex;
	
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(boneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &boneDesc, sizeof(boneDesc));
	context->Unmap(boneBuffer, 0);
	context->VSSetConstantBuffers(2, 1, &boneBuffer);

	//Render Material
	if(bHasMaterial)
	material->ApplyMaterial(context);
}

void Mesh::ApplyPipelineNoMaterial(ID3D11DeviceContext * context, const uint& prevDrawCount, const uint& actorIndex)
{
	boneDesc.prevDrawCount = prevDrawCount;
	
	boneDesc.actorIndex = actorIndex;
	
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(boneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &boneDesc, sizeof(boneDesc));
	context->Unmap(boneBuffer, 0);
	context->VSSetConstantBuffers(2, 1, &boneBuffer);
}

void Mesh::ApplyPipelineReflectionMaterial(ID3D11DeviceContext * context, const uint& prevDrawCount, const uint& actorIndex)
{
	boneDesc.prevDrawCount = prevDrawCount;
	boneDesc.actorIndex = actorIndex;
	

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(boneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &boneDesc, sizeof(boneDesc));
	context->Unmap(boneBuffer, 0);
	context->VSSetConstantBuffers(2, 1, &boneBuffer);

	//Render Material
	if (bHasMaterial)
	material->ApplyRefeltionMaterial(context);
}

void Mesh::ClearPipeline(ID3D11DeviceContext * context)
{
	context->VSSetConstantBuffers(2, 1, &nullBuffer);
}

void Mesh::ClearPipelineMaterial(ID3D11DeviceContext * context)
{
	if (bHasMaterial)
	material->ClearMaterial(context);
	context->VSSetConstantBuffers(2, 1, &nullBuffer);
}

void Mesh::ClearPipelineReflection(ID3D11DeviceContext * context)
{
	if (bHasMaterial)
	material->ClearReflectionMaterial(context);
	context->VSSetConstantBuffers(2, 1, &nullBuffer);
}

