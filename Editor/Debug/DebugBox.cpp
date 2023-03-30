#include "Framework.h"
#include "DebugBox.h"

DebugBox::DebugBox(ID3D11Device* device,const Vector3 & boxMin, const Vector3 & boxMax)
	: DebugVertexBuffer(nullptr), DebugIndexBuffer(nullptr), debugBuffer(nullptr)
{
	D3DXMatrixIdentity(&boxWorld);
	VertexColor temp[8];
	for (uint i = 0; i < 8; i++)
	{
		temp[i].color = Color(1, 0, 0, 1);
	}
	temp[0].Position = Vector3(boxMin.x, boxMin.y, boxMax.z);
	temp[1].Position = Vector3(boxMax.x, boxMin.y, boxMax.z);
	temp[2].Position = Vector3(boxMin.x, boxMax.y, boxMax.z);
	temp[3].Position = boxMax;
	temp[4].Position = boxMin;
	temp[5].Position = Vector3(boxMax.x, boxMin.y, boxMin.z);
	temp[6].Position = Vector3(boxMin.x, boxMax.y, boxMin.z);
	temp[7].Position = Vector3(boxMax.x, boxMax.y, boxMin.z);

	

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(VertexColor) * 8;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = &temp[0];
	Check(device->CreateBuffer(&desc, &subResource, &DebugVertexBuffer));

	const uint cubeIndices[] = {
	0,1,1,3,3,2,2,0,4,5,5,7,7,6,6,4,0,4,1,5,2,6,3,7
	};
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(uint) * 24;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = &cubeIndices[0];
	Check(device->CreateBuffer(&desc, &subResource, &DebugIndexBuffer));


	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.ByteWidth = sizeof(Matrix);
	Check(device->CreateBuffer(&desc, NULL, &debugBuffer));
}

DebugBox::~DebugBox()
{
}

void DebugBox::Render(ID3D11DeviceContext * context,const Matrix& VP)
{
	{
		Matrix temp;
		D3DXMatrixTranspose(&temp, &(boxWorld*VP));
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(debugBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &(temp), sizeof(Matrix));
		context->Unmap(debugBuffer, 0);
		context->VSSetConstantBuffers(0, 1, &debugBuffer);
	}
	uint debugSlot = 0;
	uint debugOffset = 0;
	uint debugStride = sizeof(VertexColor);
	context->IASetVertexBuffers(debugSlot, 1, &DebugVertexBuffer, &debugStride, &debugOffset);
	
	context->IASetIndexBuffer(DebugIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	context->DrawIndexed(24, 0, 0);
}
