#include "Framework.h"
#include "Cloud.h"
#include "Core/D3D11/D3D11_Helper.h"

Cloud::Cloud(ID3D11Device* device)
	:device(device), texture(nullptr), srv(nullptr), LinearSampler(nullptr),vsBuffer(nullptr),vs(nullptr),ps(nullptr),inputLayout(nullptr)
	, AdditiveBlendState(nullptr), depthStencilState(nullptr)
{
	CreateShader();

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(Matrix);
	Check(device->CreateBuffer(&bufferDesc, NULL, &vsBuffer));

	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(CB_PSDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &psBuffer));

	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &LinearSampler));


	noiseTexture = new Texture();
	noiseTexture->Load(device, L"noise.png", nullptr, true);
	cloudTexture1 = new Texture();
	cloudTexture1->Load(device,L"Environment/cloud001.dds", nullptr, true);
	cloudTexture2 = new Texture();
	cloudTexture2->Load(device,L"Environment/cloud002.dds", nullptr, true);


	Create();
	
	int perm[] =
	{
	   151,160,137,91,90,15,
	   131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
	   190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
	   88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
	   77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
	   102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
	   135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
	   5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
	   223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
	   129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
	   251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
	   49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
	   138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
	};

	int gradValues[] =
	{
	   +1, +1, +0, -1, +1, +0, +1, -1, +0, -1, -1, +0,
	   +1, +0, +1, -1, +0, +1, +1, +0, -1, -1, +0, -1,
	   +0, +1, +1, +0, -1, +1, +0, +1, -1, +0, -1, -1,
	   +1, +1, +0, +0, -1, +1, -1, +1, +0, +0, -1, -1
	};


	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = 256;
	desc.Height = 256;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	Color* pixels = new Color[256 * 256];
	for (int y = 0; y < 256; y++)
	{
		for (int x = 0; x < 256; x++)
		{
			int value = perm[(x + perm[y]) & 0xFF];

			Color color;
			color.r = (float)(gradValues[value & 0x0F] * 64 + 64);
			color.g = (float)(gradValues[value & 0x0F + 1] * 64 + 64);
			color.b = (float)(gradValues[value & 0x0F + 2] * 64 + 64);
			color.a = (float)value;


			UINT index = desc.Width * y + x;
			pixels[index] = color;
		}
	}

	D3D11_SUBRESOURCE_DATA subResource = { 0 };
	subResource.pSysMem = pixels;
	subResource.SysMemPitch = 256 * 4;

	Check(device->CreateTexture2D(&desc, &subResource, &texture));

	

	//Create SRV
	{
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srvDesc.Format = desc.Format;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

		Check(device->CreateShaderResourceView(texture, &srvDesc, &srv));
	}


	
	SafeDeleteArray(pixels);


	SafeRelease(AdditiveBlendState);
	// Create the additive blend state
	D3D11_BLEND_DESC descBlend;
	descBlend.AlphaToCoverageEnable = FALSE;
	descBlend.IndependentBlendEnable = FALSE;
	const D3D11_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		TRUE,
		D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, //srcBlend,descBlend,BlendOp
		D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,//srcBlendAlpha,destBlendAlpha,BlendOpAlpha
		D3D11_COLOR_WRITE_ENABLE_ALL,//rendertargetWriteMask
	};

	

	for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		descBlend.RenderTarget[i] = defaultRenderTargetBlendDesc;
	}


	Check(device->CreateBlendState(&descBlend, &AdditiveBlendState));


	D3D11_DEPTH_STENCIL_DESC descDepth;
	descDepth.DepthEnable = false;
	descDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	descDepth.DepthFunc = D3D11_COMPARISON_LESS;
	descDepth.StencilEnable = TRUE;
	descDepth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	descDepth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	const D3D11_DEPTH_STENCILOP_DESC stencilMarkOp = { D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_LESS };
	descDepth.FrontFace = stencilMarkOp;
	descDepth.BackFace = stencilMarkOp;
	Check(device->CreateDepthStencilState(&descDepth, &depthStencilState));
}


Cloud::~Cloud()
{
	
}



void Cloud::Render(ID3D11DeviceContext* context)
{
	Vector3 camPos = GlobalData::Position();
	
	D3DXMatrixTranslation(&T, camPos.x, camPos.y, camPos.z);
	D3DXMatrixScaling(&S, scale, scale, scale);
	GlobalData::GetVP(&VP);
	D3DXMatrixTranspose(&vsDesc.wvp, &(S*T *VP));
	psDesc.Time += Time::Delta();
	ID3D11ShaderResourceView* srvArray[3] = { *noiseTexture,*cloudTexture1,*cloudTexture2 };
	context->PSSetShaderResources(0, 3, srvArray);
	context->PSSetSamplers(0, 1, &LinearSampler);

	{
		D3D11_MAPPED_SUBRESOURCE MappedResource; 
		context->Map(vsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &vsDesc, sizeof(Matrix));
		context->Unmap(vsBuffer, 0);
		context->VSSetConstantBuffers(0, 1, &vsBuffer);

		
	}
	{
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(psBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &psDesc, sizeof(CB_PSDesc));
		context->Unmap(psBuffer, 0);
		context->PSSetConstantBuffers(0, 1, &psBuffer);
	}



	uint offset = 0;
	uint slot = 0;
	uint stride = sizeof(VertexTexture);
	context->IASetVertexBuffers(slot, 1, &cloudVertexBuffer, &stride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(cloudIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->VSSetShader(vs, nullptr, 0);
	context->PSSetShader(ps, nullptr, 0);

	ID3D11BlendState* pPrevBlendState;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);

	context->OMSetBlendState(AdditiveBlendState, prevBlendFactor, prevSampleMask);
   //context->OMSetDepthStencilState(depthStencilState, 1);
	context->DrawIndexed(cloudIndexCount, 0, 0);

	context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
	
	ZeroMemory(&srvArray, sizeof(srvArray));
	context->PSSetShaderResources(0, 3, srvArray);

	ID3D11Buffer* nullBuffer = nullptr;
	context->VSSetConstantBuffers(0, 1, &nullBuffer);

	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

void Cloud::Render(ID3D11DeviceContext * context, const Matrix & VP)
{
	ID3D11ShaderResourceView* srvArray[3] = { *noiseTexture,*cloudTexture1,*cloudTexture2 };
	context->PSSetShaderResources(0, 3, srvArray);
	context->PSSetSamplers(0, 1, &LinearSampler);

	{

		Vector3 camPos = GlobalData::Position();
		Matrix S, T,proj;
		GlobalData::GetProj(&proj);
		Matrix ViewProj = VP * proj;
		D3DXMatrixTranslation(&T, camPos.x, camPos.y, camPos.z);
		D3DXMatrixScaling(&S, scale, scale, scale);
		D3DXMatrixTranspose(&vsDesc.wvp, &(S*T * ViewProj));

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(vsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &vsDesc, sizeof(Matrix));
		context->Unmap(vsBuffer, 0);
		context->VSSetConstantBuffers(0, 1, &vsBuffer);


	}
	{

		psDesc.Time += Time::Delta();
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(psBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &psDesc, sizeof(CB_PSDesc));
		context->Unmap(psBuffer, 0);
		context->PSSetConstantBuffers(0, 1, &psBuffer);
	}



	uint offset = 0;
	uint slot = 0;
	uint stride = sizeof(VertexTexture);
	context->IASetVertexBuffers(slot, 1, &cloudVertexBuffer, &stride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(cloudIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->VSSetShader(vs, nullptr, 0);
	context->PSSetShader(ps, nullptr, 0);

	ID3D11BlendState* pPrevBlendState;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);

	context->OMSetBlendState(AdditiveBlendState, prevBlendFactor, prevSampleMask);

	context->DrawIndexed(cloudIndexCount, 0, 0);

	context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);

	ZeroMemory(&srvArray, sizeof(srvArray));
	context->PSSetShaderResources(0, 3, srvArray);

	ID3D11Buffer* nullBuffer = nullptr;
	context->VSSetConstantBuffers(0, 1, &nullBuffer);

	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}


void Cloud::Create()
{
	float quadSize = skyPlaneWidth / static_cast<float>(skyPlaneResolution);
	float radius = skyPlaneWidth / 2.0f;
	float constant = (skyPlaneTop - skyPlaneBottom) / (radius*radius);
	float textureDelta = (float)textureRepeat / (float)skyPlaneResolution;


	struct skyPlane
	{
		Vector3 position;
		Vector3 uv;
	};

	skyPlane* plane = new skyPlane[(skyPlaneResolution + 1)*(skyPlaneResolution + 1)];


	for (uint i = 0; i <= skyPlaneResolution; i++)
		for (uint j = 0; j <= skyPlaneResolution; j++)
		{
			Vector3 position;
			Vector3 uv;

			position.x = (-0.5f*skyPlaneWidth) + ((float)j*quadSize);
			position.z = (-0.5f*skyPlaneWidth) + ((float)i*quadSize);
			position.y = skyPlaneTop - (constant*((position.x*position.x) + (position.z*position.z)));

			uv.x = (float)j*textureDelta;
			uv.y = (float)i*textureDelta;

			uint index = i * (skyPlaneResolution + 1) + j;
			plane[index].position = position;
			plane[index].uv = uv;
		}

	UINT index = 0;
	cloudVertexCount = ((skyPlaneResolution + 1)*(skyPlaneResolution + 1)) * 6;
	cloudIndexCount = cloudVertexCount;
	VertexTexture* vertices = new VertexTexture[cloudVertexCount];
	UINT* indices = new UINT[cloudVertexCount];
	for (uint j = 0; j < skyPlaneResolution; j++)
	{
		for (uint i = 0; i < skyPlaneResolution; i++)
		{
			uint index1 = j * (skyPlaneResolution + 1) + i;
			uint index2 = j * (skyPlaneResolution + 1) + (i + 1);
			uint index3 = (j + 1) * (skyPlaneResolution + 1) + i;
			uint index4 = (j + 1) * (skyPlaneResolution + 1) + (i + 1);

			// Triangle 1 - Upper Left
			vertices[index].Position = plane[index1].position;
			vertices[index].Uv = D3DXVECTOR2(plane[index1].uv.x, plane[index1].uv.y);
			indices[index] = index;
			index++;

			// Triangle 1 - Upper Right
			vertices[index].Position = plane[index2].position;
			vertices[index].Uv = D3DXVECTOR2(plane[index2].uv.x, plane[index2].uv.y);
			indices[index] = index;
			index++;

			// Triangle 1 - Bottom Left
			vertices[index].Position = plane[index3].position;
			vertices[index].Uv = D3DXVECTOR2(plane[index3].uv.x, plane[index3].uv.y);
			indices[index] = index;
			index++;

			// Triangle 2 - Bottom Left
			vertices[index].Position = plane[index3].position;
			vertices[index].Uv = D3DXVECTOR2(plane[index3].uv.x, plane[index3].uv.y);
			indices[index] = index;
			index++;

			// Triangle 2 - Upper Right
			vertices[index].Position = plane[index2].position;
			vertices[index].Uv = D3DXVECTOR2(plane[index2].uv.x, plane[index2].uv.y);
			indices[index] = index;
			index++;

			// Triangle 2 - Bottom Right
			vertices[index].Position = plane[index4].position;
			vertices[index].Uv = D3DXVECTOR2(plane[index4].uv.x, plane[index4].uv.y);
			indices[index] = index;
			index++;
		}
	}

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(VertexTexture) * cloudVertexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = vertices;
	Check(device->CreateBuffer(&desc, &subResource, &cloudVertexBuffer));


	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(uint) * cloudIndexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = indices;
	Check(device->CreateBuffer(&desc, &subResource, &cloudIndexBuffer));

	
	SafeDeleteArray(vertices);
	SafeDeleteArray(indices);
}

void Cloud::CreateShader()
{
	ID3DBlob* ShaderBlob = NULL;
	auto path = "../_Shaders/Environment/Cloud.hlsl";
	auto entryPoint = "VS_Cloud";
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


	entryPoint = "PS_Cloud";
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
