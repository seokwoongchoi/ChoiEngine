#include "Framework.h"
#include "Material.h"

Material::Material(ID3D11Device* device)
	:device(device), name(L""), arrSRV{}, srvArray{}, colorDesc{}, colorBuffer(nullptr), nullBuffer(nullptr), textures(nullptr)
	, diffuseFile(L""), normalFile(L""),roughnessFile(L""), metallicFile(L"")
{
    SafeRelease(colorBuffer);
    SafeDeleteArray(textures);

	textures = new Texture[MAX_MODEL_TEXTURE];

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(colorDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &colorBuffer));
}


Material::~Material()
{
	SafeDeleteArray(textures);
	SafeRelease(colorBuffer);
}

void Material::ApplyMaterial(ID3D11DeviceContext* context)
{
	Vector3 l = GlobalData::LightDirection();
	colorDesc.Specular = Color(l.x, l.y, l.z, colorDesc.Specular.a);

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(colorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &colorDesc, sizeof(colorDesc));
	context->Unmap(colorBuffer, 0);
	context->PSSetConstantBuffers(0, 1, &colorBuffer);

	for (uint i = 0; i < MAX_MODEL_TEXTURE; i++)
	{
		arrSRV[i] = textures[i];
	}

	context->PSSetShaderResources(0, MAX_MODEL_TEXTURE, arrSRV);
}

void Material::ApplyRefeltionMaterial(ID3D11DeviceContext * context)
{
	
	Vector3 l = GlobalData::LightDirection();
	colorDesc.Specular = Color(l.x, l.y, l.z, colorDesc.Specular.a);

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(colorBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &colorDesc, sizeof(colorDesc));
	context->Unmap(colorBuffer, 0);
	context->PSSetConstantBuffers(0, 1, &colorBuffer);

	srvArray[0] = textures[0];
	srvArray[1] = textures[1];
	

	context->PSSetShaderResources(0,2, srvArray);
}


void Material::ClearMaterial(ID3D11DeviceContext* context)
{
	ZeroMemory(arrSRV, sizeof(arrSRV));
		context->PSSetShaderResources(0, MAX_MODEL_TEXTURE, arrSRV);

		context->PSSetConstantBuffers(0, 1, &nullBuffer);
		
}

void Material::ClearReflectionMaterial(ID3D11DeviceContext * context)
{
	ZeroMemory(srvArray, sizeof(srvArray));
	context->PSSetShaderResources(0, 2, srvArray);

	context->PSSetConstantBuffers(0, 1, &nullBuffer);
}
