#include "Framework.h"
#include "Scattering.h"
#include "Core/D3D11/D3D11_Helper.h"

Scattering::Scattering(ID3D11Device* device)
	:device(nullptr),world{},worldDesc {}, pixelDesc{}, worldBuffer(nullptr), pixelBuffer(nullptr), LinearSampler(nullptr), texture(nullptr),
	inputLayout(nullptr), vs(nullptr), ps(nullptr), domeVertexBuffer(nullptr), domeIndexBuffer(nullptr), 
	domeVertexCount(0),	domeIndexCount(0), domeCount(32), theta(1.676f), nullBuffer(nullptr),  AdditiveBlendState(nullptr)
{
	this-> device = device;
	CreateDoom();
	CreateShader();

	// Allocate constant buffers
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(Matrix);
	Check(device->CreateBuffer(&bufferDesc, NULL, &worldBuffer));

	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(Vector4);
	Check(device->CreateBuffer(&bufferDesc, NULL, &pixelBuffer));

	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &LinearSampler));

	D3D11_DEPTH_STENCIL_DESC descDepth;
	descDepth.DepthEnable = false;
	descDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	descDepth.DepthFunc = D3D11_COMPARISON_LESS;
	descDepth.StencilEnable = TRUE;
	descDepth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	descDepth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	const D3D11_DEPTH_STENCILOP_DESC stencilMarkOp = { D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_GREATER };
	descDepth.FrontFace = stencilMarkOp;
	descDepth.BackFace = stencilMarkOp;
	Check(device->CreateDepthStencilState(&descDepth, &depthStencilState));

	texture = new Texture();
	texture->Load(device,L"Environment/Starfield.png", nullptr);
	
	D3DXMatrixScaling(&world,1, 1, 1);
	//D3DXMatrixTranspose(&world, &world);
	

	SafeRelease(AdditiveBlendState);
	// Create the additive blend state
	D3D11_BLEND_DESC descBlend;
	descBlend.AlphaToCoverageEnable = true;
	descBlend.IndependentBlendEnable = true;
	const D3D11_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		TRUE,
		D3D11_BLEND_SRC_COLOR, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_OP_ADD, //srcBlend,descBlend,BlendOp
		D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,//srcBlendAlpha,destBlendAlpha,BlendOpAlpha
		D3D11_COLOR_WRITE_ENABLE_ALL,//rendertargetWriteMask
	};


	for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		descBlend.RenderTarget[i] = defaultRenderTargetBlendDesc;
	}



	Check(device->CreateBlendState(&descBlend, &AdditiveBlendState));
}


Scattering::~Scattering()
{
	SafeDelete(texture);
	SafeDelete(domeVertexBuffer);
	SafeDelete(domeIndexBuffer);
}



void Scattering::Render(ID3D11DeviceContext* context)
{
	Vector3 camPos = GlobalData::Position();
	world._41 = camPos.x;
	world._42 = camPos.y;
	world._43 = camPos.z;
	GlobalData::GetVP(&vp);
	
	D3DXMatrixTranspose(&worldDesc.WVP, &(world*vp));

	if (Keyboard::Get()->Down('H'))
	{
		theta += 0.1f;

	}
	float x = sinf(theta);
	float y = cosf(theta);
	GlobalData::LightColor(Math::Abs(y));


	GlobalData::LightDirection(Vector3(x, y, 1.0f));
	ID3D11ShaderResourceView* srvArray[1] = { *texture };
		context->PSSetShaderResources(0, 1, srvArray);
		context->PSSetSamplers(0, 1, &LinearSampler);
	{
		
			D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(worldBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &worldDesc, sizeof(Matrix));
		context->Unmap(worldBuffer, 0);
		context->VSSetConstantBuffers(0, 1, &worldBuffer);
	}
	
	
	{
		
		pixelDesc.LightDir = Vector3(x,y,1.0f);
		pixelDesc.Time = Time::Delta();
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(pixelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &pixelDesc, sizeof(Vector4));
		context->Unmap(pixelBuffer, 0);
		context->PSSetConstantBuffers(0, 1, &pixelBuffer);
	}
	//ID3D11BlendState* pPrevBlendState;
	//FLOAT prevBlendFactor[4];
	//UINT prevSampleMask;
	//context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);
	// Set the depth state for the directional light
	//context->OMSetBlendState(AdditiveBlendState, prevBlendFactor, prevSampleMask);
	context->OMSetDepthStencilState(depthStencilState,1);
	uint offset = 0;
	
	context->IASetVertexBuffers(slot, 1, &domeVertexBuffer, &stride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(domeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->VSSetShader(vs, nullptr, 0);
	context->PSSetShader(ps, nullptr, 0);
	context->DrawIndexed(domeIndexCount,0,0);

	//context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
	

	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->PSSetShaderResources(0, 1, &nullSRV);
	context->VSSetConstantBuffers(0, 1, &nullBuffer);
	context->PSSetConstantBuffers(0, 1, &nullBuffer);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);


}

void Scattering::CreateDoom()
{
	UINT latitude = domeCount / 2; // 위도
	UINT longitude = domeCount; // 경도

	domeVertexCount = longitude * latitude * 2;
	domeIndexCount = (longitude - 1) * (latitude - 1) * 2 * 8;


	VertexTexture* vertices = new VertexTexture[domeVertexCount];

	UINT index = 0;
	for (UINT i = 0; i < longitude; i++)
	{
		float xz = 100.0f * (i / (longitude - 1.0f)) * Math::PI / 180.0f;

		for (UINT j = 0; j < latitude; j++)
		{
			float y = Math::PI * j / (latitude - 1);

			vertices[index].Position.x = sinf(xz) * cosf(y);
			vertices[index].Position.y = cosf(xz);
			vertices[index].Position.z = sinf(xz) * sinf(y);

			vertices[index].Uv.x = 0.5f / (float)longitude + i / (float)longitude;
			vertices[index].Uv.y = 0.5f / (float)latitude + j / (float)latitude;

			index++;
		} // for(j)
	}  // for(i)

	for (UINT i = 0; i < longitude; i++)
	{
		float xz = 100.0f * (i / (longitude - 1.0f)) * Math::PI / 180.0f;

		for (UINT j = 0; j < latitude; j++)
		{
			float y = (Math::PI * 2.0f) - (Math::PI * j / (latitude - 1));

			vertices[index].Position.x = sinf(xz) * cosf(y);
			vertices[index].Position.y = cosf(xz);
			vertices[index].Position.z = sinf(xz) * sinf(y);

			vertices[index].Uv.x = 0.5f / (float)longitude + i / (float)longitude;
			vertices[index].Uv.y = 0.5f / (float)latitude + j / (float)latitude;

			index++;
		} // for(j)
	}  // for(i)


	index = 0;
	UINT* indices = new UINT[domeIndexCount * 3];

	for (UINT i = 0; i < longitude - 1; i++)
	{
		for (UINT j = 0; j < latitude - 1; j++)
		{
			indices[index++] = i * latitude + j;
			indices[index++] = (i + 1) * latitude + j;
			indices[index++] = (i + 1) * latitude + (j + 1);

			indices[index++] = (i + 1) * latitude + (j + 1);
			indices[index++] = i * latitude + (j + 1);
			indices[index++] = i * latitude + j;
		}
	}

	UINT offset = latitude * longitude;
	for (UINT i = 0; i < longitude - 1; i++)
	{
		for (UINT j = 0; j < latitude - 1; j++)
		{
			indices[index++] = offset + i * latitude + j;
			indices[index++] = offset + (i + 1) * latitude + (j + 1);
			indices[index++] = offset + (i + 1) * latitude + j;

			indices[index++] = offset + i * latitude + (j + 1);
			indices[index++] = offset + (i + 1) * latitude + (j + 1);
			indices[index++] = offset + i * latitude + j;
		}
	}
	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(VertexTexture) * domeVertexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = vertices;
	Check(device->CreateBuffer(&desc, &subResource, &domeVertexBuffer));


	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(uint) * domeIndexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = indices;
	Check(device->CreateBuffer(&desc, &subResource, &domeIndexBuffer));


	SafeDeleteArray(vertices);
	SafeDeleteArray(indices);
}

void Scattering::CreateShader()
{
	ID3DBlob* ShaderBlob = NULL;
	auto path = "../_Shaders/Environment/Scattering.hlsl";
	auto entryPoint = "VS_Dome";
	auto shaderModel = "vs_5_0";

	auto result = D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	);

	if (!result)
		assert(false);

	auto hr = device->CreateVertexShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&vs
	);
	assert(SUCCEEDED(hr));
	inputLayout = new InputLayout();
	inputLayout->Create(device, ShaderBlob);
	SafeRelease(ShaderBlob);


	entryPoint = "PS_Dome";
	shaderModel = "ps_5_0";
	result = D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	);

	if (!result)
		assert(false);

	hr = device->CreatePixelShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&ps
	);
	assert(SUCCEEDED(hr));
	SafeRelease(ShaderBlob);
}

