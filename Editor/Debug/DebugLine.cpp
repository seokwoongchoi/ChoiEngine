#include "stdafx.h"
#include "DebugLine.h"
#include "Core/D3D11/D3D11_Helper.h"
DebugLine* DebugLine::instance = NULL;

void DebugLine::Create()
{
	assert(instance == NULL);

	instance = new DebugLine();
}

void DebugLine::Delete()
{
	SafeDelete(instance);
}

DebugLine * DebugLine::Get()
{
	assert(instance != NULL);

	return instance;
}

void DebugLine::RenderLine(Vector3 & start, Vector3 & end)
{
	RenderLine(start, end, D3DXCOLOR(0, 1, 0, 1));
}

void DebugLine::RenderLine(Vector3 & start, Vector3 & end, float r, float g, float b)
{
	RenderLine(start, end, D3DXCOLOR(r, g, b, 1));
}

void DebugLine::RenderLine(float x, float y, float z, float x2, float y2, float z2)
{
	RenderLine(Vector3(x, y, z), Vector3(x2, y2, z2), D3DXCOLOR(0, 1, 0, 1));
}

void DebugLine::RenderLine(float x, float y, float z, float x2, float y2, float z2, float r, float g, float b)
{
	RenderLine(Vector3(x, y, z), Vector3(x2, y2, z2), D3DXCOLOR(r, g, b, 1));
}

void DebugLine::RenderLine(float x, float y, float z, float x2, float y2, float z2, D3DXCOLOR & color)
{
	RenderLine(Vector3(x, y, z), Vector3(x2, y2, z2), color);
}

void DebugLine::RenderLine(Vector3 & start, Vector3 & end, Color & color , uint ID)
{
	
	vertices[drawCount].color = color;
	vertices[drawCount].color.a = static_cast<float>(ID);
	vertices[drawCount++].Position = start;

	vertices[drawCount].color = color;
	vertices[drawCount].color.a = static_cast<float>(ID);
	vertices[drawCount++].Position = end;
	
}

void DebugLine::RenderLine(const Vector3 & start, const Vector3 & end, uint ID)
{
	vertices[drawCount].color = Color(1, 0, 0, 1);
	vertices[drawCount].color.a = static_cast<float>(ID);
	vertices[drawCount++].Position = start;

	vertices[drawCount].color = Color(1, 0, 0, 1);
	vertices[drawCount].color.a = static_cast<float>(ID);
	vertices[drawCount++].Position = end;
}





void DebugLine::Render(ID3D11DeviceContext* context)
{
	context->VSSetShader(DebugVS, nullptr,0);
	context->PSSetShader(DebugPS, nullptr, 0);
	{
		Matrix temp;
		GlobalData::GetVP(&temp);
		D3DXMatrixTranspose(&temp, &temp);
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(debugBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &(temp), sizeof(Matrix));
		context->Unmap(debugBuffer, 0);
		context->VSSetConstantBuffers(0, 1, &debugBuffer);
	}
	D3D11_MAPPED_SUBRESOURCE subResource;
	context->Map(DebugVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		memcpy(subResource.pData, vertices.data(), sizeof(VertexColor) * MAX_LINE_VERTEX );
	}
	context->Unmap(DebugVertexBuffer, 0);

	
	context->IASetInputLayout(*debugInputLayout);
	uint debugSlot = 0;
	uint debugOffset = 0;
	uint debugStride = sizeof(VertexColor);
	context->IASetVertexBuffers(debugSlot, 1, &DebugVertexBuffer, &debugStride, &debugOffset);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	context->Draw(drawCount * 2, 0);
	
	drawCount = 0;
	ZeroMemory(vertices.data(), sizeof(VertexColor) * MAX_LINE_VERTEX);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

void DebugLine::BoneBoxRender(ID3D11DeviceContext * context, ID3D11ShaderResourceView * srv)
{
	context->VSSetShader(BoneBoxVS, nullptr, 0);
	context->PSSetShader(DebugPS, nullptr, 0);
	{
		Matrix temp;
		GlobalData::GetVP(&temp);
		D3DXMatrixTranspose(&temp, &temp);
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(debugBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &(temp), sizeof(Matrix));
		context->Unmap(debugBuffer, 0);
		context->VSSetConstantBuffers(0, 1, &debugBuffer);
	}
	D3D11_MAPPED_SUBRESOURCE subResource;
	context->Map(DebugVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		memcpy(subResource.pData, vertices.data(), sizeof(VertexColor) * MAX_LINE_VERTEX);
	}
	context->Unmap(DebugVertexBuffer, 0);

	context->VSSetShaderResources(0, 1, &srv);

	context->IASetInputLayout(*debugInputLayout);
	uint debugSlot = 0;
	uint debugOffset = 0;
	uint debugStride = sizeof(VertexColor);
	context->IASetVertexBuffers(debugSlot, 1, &DebugVertexBuffer, &debugStride, &debugOffset);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	context->DrawInstanced(drawCount * 2,3, 0,0);

	drawCount = 0;
	ZeroMemory(vertices.data(), sizeof(VertexColor) * MAX_LINE_VERTEX);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

DebugLine::DebugLine()
	: drawCount(0), DebugVS(nullptr), DebugPS(nullptr), debugInputLayout(nullptr), debugBuffer(nullptr), DebugVertexBuffer(nullptr),
	BoneBoxVS(nullptr)

{

}

DebugLine::~DebugLine()
{
	//SafeDeleteArray(vertices);
	SafeRelease(DebugVS);
	SafeRelease(DebugPS);
	SafeDelete(debugInputLayout);
}

void DebugLine::Initiallize(ID3D11Device * device)
{	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	SafeRelease(DebugVS);

	SafeRelease(DebugPS);

	ID3DBlob* ShaderBlob = NULL;

	auto path = "../_Shaders/PreviewDebug.hlsl";
	auto entryPoint = "DebugVS";
	auto shaderModel = "vs_5_0";

	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));



	Check(device->CreateVertexShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&DebugVS
	));

	
	SafeRelease(ShaderBlob);


	
	 entryPoint = "BoneBoxVS";
	

	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));



	Check(device->CreateVertexShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&BoneBoxVS
	));
	debugInputLayout = new InputLayout();
	debugInputLayout->Create(device, ShaderBlob);
	SafeRelease(ShaderBlob);
	//////////////////////////////////////////////////////////////////////////
	entryPoint = "DebugPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));



	Check(device->CreatePixelShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&DebugPS
	));

	SafeRelease(ShaderBlob);
	vertices.assign(MAX_LINE_VERTEX, VertexColor());

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(VertexColor) * MAX_LINE_VERTEX;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = vertices.data();
	Check(device->CreateBuffer(&desc, &subResource, &DebugVertexBuffer));

	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.ByteWidth = sizeof(Matrix);
	Check(device->CreateBuffer(&desc, NULL, &debugBuffer));



}

void DebugLine::PreviewRender(ID3D11DeviceContext * context, const Matrix & matrix)
{
	context->VSSetShader(DebugVS, nullptr, 0);
	context->PSSetShader(DebugPS, nullptr, 0);
	{
		Matrix temp;
		D3DXMatrixTranspose(&temp, &matrix);
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(debugBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &(temp), sizeof(Matrix));
		context->Unmap(debugBuffer, 0);
		context->VSSetConstantBuffers(0, 1, &debugBuffer);
	}
	D3D11_MAPPED_SUBRESOURCE subResource;
	context->Map(DebugVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
	{
		memcpy(subResource.pData, vertices.data(), sizeof(VertexColor) * MAX_LINE_VERTEX);
	}
	context->Unmap(DebugVertexBuffer, 0);

	context->IASetInputLayout(*debugInputLayout);
	uint debugSlot = 0;
	uint debugOffset = 0;
	uint debugStride = sizeof(VertexColor);
	context->IASetVertexBuffers(debugSlot, 1, &DebugVertexBuffer, &debugStride, &debugOffset);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	context->Draw(drawCount * 2, 0);

	drawCount = 0;
	//vertices.clear();
	ZeroMemory(vertices.data(), sizeof(VertexColor) * MAX_LINE_VERTEX);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}
