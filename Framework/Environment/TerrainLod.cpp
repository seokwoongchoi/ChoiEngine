#include "Framework.h"
#include "TerrainLod.h"
#include "Core/D3D11/D3D11_Helper.h"
#define clamp(value,minimum,maximum) (max(min((value),(maximum)),(minimum)))
TerrainLod::TerrainLod(ID3D11Device* device, ID3D11ShaderResourceView* heightSRV)
	:device(device), heightSRV(heightSRV),
	vs(nullptr), hs(nullptr), ds(nullptr), ps(nullptr),
	vertexBuffer(nullptr), indexBuffer(nullptr), heightMap(nullptr), sampLinear(nullptr), mainCBuffer(nullptr),
	patchCBuffer(nullptr), sampleparamsCBuffer(nullptr), stride(0), offset(0), slot(0), indexCount(0), rsState(nullptr),
	gs(nullptr), cs(nullptr),computeTexture(nullptr),computeSRV(nullptr),computeUAV(nullptr)
{
	CreateVertexData();
	CreateIndexData();
	CreateShaders();
	
	if (heightSRV == nullptr)
	{
		heightMap = new Texture();
		heightMap->Load(device, L"../_Textures/Terrain/TerrainHeightMap.png", nullptr);
		this->heightSRV = *heightMap;
	}
	

	

	SafeRelease(sampLinear);
	SafeRelease(rsState);

	D3D11_SAMPLER_DESC sampDesc;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.BorderColor[0] = sampDesc.BorderColor[1] = sampDesc.BorderColor[2] = sampDesc.BorderColor[3] = 0;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	sampDesc.MinLOD = 0.0f;
	sampDesc.MipLODBias = 0.0f;
	Check(device->CreateSamplerState(&sampDesc, &sampLinear));

	D3D11_RASTERIZER_DESC reasterizerDesc;
	ZeroMemory(&reasterizerDesc, sizeof(reasterizerDesc));
	reasterizerDesc.CullMode = D3D11_CULL_BACK;
	reasterizerDesc.FillMode = D3D11_FILL_SOLID;
	reasterizerDesc.FrontCounterClockwise = true;
	Check(device->CreateRasterizerState(&reasterizerDesc, &rsState));

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(Matrix);
	Check(device->CreateBuffer(&bufferDesc, NULL, &mainCBuffer));
	bufferDesc.ByteWidth = sizeof(PatchDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &patchCBuffer));
	bufferDesc.ByteWidth = sizeof(SampleparamsDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &sampleparamsCBuffer));
	

	D3DXMatrixIdentity(&world);
	Matrix S,R, T;
	D3DXMatrixTranslation(&T, 256, 0, 256);
	D3DXMatrixScaling(&S, 500.0f, 1.0f, 500.0f);
	
	D3DXMatrixRotationY(&R, static_cast<float>(D3DXToRadian(90.0)));
	world = S *R*T;
	D3DXMatrixInverse(&sampleparamsDesc.InvTposeWorld, nullptr, &world);
	D3DXMatrixTranspose(&world, &world);
	D3DXMatrixTranspose(&sampleparamsDesc.InvTposeWorld, &sampleparamsDesc.InvTposeWorld);


	{
		D3D11_TEXTURE2D_DESC desc;
		ID3D11Texture2D* texture;
		this->heightSRV->GetResource((ID3D11Resource**)&texture);
		texture->GetDesc(&desc);
		textureWidth = static_cast<float>(desc.Width);
		textureHeight = static_cast<float>(desc.Height);

		patchDesc.heightMapDimensionsHS = Vector4(static_cast<float>(desc.Width), static_cast<float>(desc.Height), static_cast<float>(TERRAIN_X_LEN), static_cast<float>(TERRAIN_Z_LEN));
		sampleparamsDesc.heightMapDimensionsDS = Vector4(static_cast<float>(desc.Width), static_cast<float>(desc.Height), static_cast<float>(TERRAIN_X_LEN), static_cast<float>(TERRAIN_Z_LEN));
		patchDesc.minMaxDistance = Vector4(1.0f,100.0f, /* unused */ 0.0f, /* unused */ 0.0f);
		patchDesc.minMaxLOD = Vector4(1.0f, 5.0f, /* unused */ 0.0f, /* unused */ 0.0f);
		sampleparamsDesc.minMaxLOD= Vector4(1.0f, 5.0f, /* unused */ 0.0f, /* unused */ 0.0f);


		// Allocate the downscaled target
		D3D11_TEXTURE2D_DESC dtd = {
			TERRAIN_X_LEN,//512, //UINT Width;
			TERRAIN_Z_LEN,//512, //UINT Height;
			1, //UINT MipLevels;
			1, //UINT ArraySize;
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			//DXGI_FORMAT_R8G8B8A8_UNORM,//desc.Format,//DXGI_FORMAT_R32_TYPELESS, //DXGI_FORMAT Format;
			1, //DXGI_SAMPLE_DESC SampleDesc;
			0,
			D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
			D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,//UINT BindFlags;
			0,//UINT CPUAccessFlags;
			0//UINT MiscFlags;    
		};
		Check(device->CreateTexture2D(&dtd, NULL, &computeTexture));

	
		// Create the resource views
		D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd;
		ZeroMemory(&dsrvd, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		dsrvd.Format = dtd.Format;//desc.Format;
	//	dsrvd.Format = DXGI_FORMAT_R32_FLOAT;
		dsrvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		dsrvd.Texture2D.MipLevels = 1;
		Check(device->CreateShaderResourceView(computeTexture, &dsrvd, &computeSRV));

		// Create the UAVs
		D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
		ZeroMemory(&DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
		DescUAV.Format = dtd.Format;// ;
		//DescUAV.Format = DXGI_FORMAT_R32_FLOAT;
		DescUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		DescUAV.Buffer.FirstElement = 0;
		DescUAV.Buffer.NumElements = dtd.Width * dtd.Height;
		Check(device->CreateUnorderedAccessView(computeTexture, &DescUAV, &computeUAV));
	}
	
}

TerrainLod::~TerrainLod()
{
}

void TerrainLod::Update()
{
    patchDesc.cameraPosition=Vector4(GlobalData::Position().x, GlobalData::Position().y, GlobalData::Position().z,1.0f);
	GlobalData::GetVP(&vp);
	D3DXMatrixTranspose(&sampleparamsDesc.ViewProj,&vp);
}

void TerrainLod::Render(ID3D11DeviceContext * context)
{
	
	static bool bFirst = true;
	if (bFirst)
	{
		RunComputeShader(context);
		bFirst = false;
	}



	context->VSSetShader(vs, nullptr, 0);
	context->HSSetShader(hs, nullptr, 0);
	context->DSSetShader(ds, nullptr, 0);
	context->GSSetShader(gs, nullptr, 0);
	context->PSSetShader(ps, nullptr, 0);

	context->DSSetSamplers(0, 1, &sampLinear);
	context->RSSetState(rsState);
	ID3D11ShaderResourceView* srv[2] = { heightSRV ,computeSRV };
	context->DSSetShaderResources(0, 1, &srv[0]);
	context->HSSetShaderResources(1, 1, &srv[1]);

	{
		ZeroMemory(&MappedResource, sizeof(MappedResource));
		context->Map(mainCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &world, sizeof(Matrix));
		context->Unmap(mainCBuffer, 0);
		context->VSSetConstantBuffers(0, 1, &mainCBuffer);
	}
	{
		ZeroMemory(&MappedResource, sizeof(MappedResource));
		context->Map(patchCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &patchDesc, sizeof(PatchDesc));
		context->Unmap(patchCBuffer, 0);
		context->HSSetConstantBuffers(1, 1, &patchCBuffer);
	}
	{
		ZeroMemory(&MappedResource, sizeof(MappedResource));
		context->Map(sampleparamsCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &sampleparamsDesc, sizeof(SampleparamsDesc));
		context->Unmap(sampleparamsCBuffer, 0);
		context->DSSetConstantBuffers(2, 1, &sampleparamsCBuffer);
	}

	context->IASetVertexBuffers(slot, 1, &vertexBuffer, &stride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST);

	context->DrawIndexed(indexCount,0,0);
	


	{//Clear
		context->VSSetShader(nullptr, nullptr, 0);
		context->HSSetShader(nullptr, nullptr, 0);

		context->DSSetShader(nullptr, nullptr, 0);
		context->GSSetShader(nullptr, nullptr, 0);
		context->PSSetShader(nullptr, nullptr, 0);

		context->RSSetState(nullptr);
		ID3D11SamplerState* nullSamp = nullptr;
		context->DSSetSamplers(0, 1, &nullSamp);
		ZeroMemory(&srv, sizeof(srv));
		context->DSSetShaderResources(0, 1, srv);
		context->HSSetShaderResources(1, 1, srv);
		ID3D11Buffer* nullBuffer = nullptr;
		context->VSSetConstantBuffers(0, 1, &nullBuffer);
		context->HSSetConstantBuffers(1, 1, &nullBuffer);
		context->DSSetConstantBuffers(2, 1, &nullBuffer);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	

}

void TerrainLod::RunComputeShader(ID3D11DeviceContext * context)
{
	ID3D11ShaderResourceView* srvArray[1] = { heightSRV };
	context->CSSetShaderResources(0, 1, srvArray);

	ID3D11UnorderedAccessView* uavArray[1] = { computeUAV };
	context->CSSetUnorderedAccessViews(0, 1, uavArray, nullptr);

	context->CSSetShader(cs, nullptr, 0);
    context->Dispatch(static_cast<uint>(textureWidth / 16.0f), static_cast<uint>(textureHeight / 16.0f), 1);


	context->CSSetShader(nullptr, nullptr, 0);
	ZeroMemory(srvArray, sizeof(srvArray));
	context->CSSetShaderResources(0, 1, srvArray);
	ZeroMemory(uavArray, sizeof(uavArray));
	context->CSSetUnorderedAccessViews(0, 1, uavArray, nullptr);
}

void TerrainLod::ReadPixel()
{
}

void TerrainLod::UpdateHeightMap()
{
}

void TerrainLod::CreateVertexData()
{

	SafeRelease(vertexBuffer);

	SafeRelease(indexBuffer);


	struct VertexTerrain
	{
		Vector3 Position;
		Vector2 Uv;
		//Color Alpha;
	};
	stride = sizeof(VertexTerrain);

	float fWidth = static_cast<float>(TERRAIN_X_LEN);
	float fHeight = static_cast<float>(TERRAIN_Z_LEN);
	vector< VertexTerrain> vertices;
	vertices.assign((TERRAIN_X_LEN + 1) * (TERRAIN_Z_LEN + 1), VertexTerrain());

	for (int x = 0; x < TERRAIN_X_LEN + 1; ++x)
	{
		for (int z = 0; z < TERRAIN_Z_LEN + 1; ++z)
		{
			float fX = static_cast<float>(x) / fWidth - 0.5f;
		
			float fZ = static_cast<float>(z) / fHeight - 0.5f;
			
			vertices[x + z * (TERRAIN_X_LEN + 1)].Position = Vector3(fX, 0.0f, fZ);
			vertices[x + z * (TERRAIN_X_LEN + 1)].Uv = Vector2(fX + 0.5f, fZ + 0.5f);
		}
	}

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(VertexTerrain) * (TERRAIN_X_LEN + 1) * (TERRAIN_Z_LEN + 1);
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = vertices.data();
	Check(device->CreateBuffer(&desc, &subResource, &vertexBuffer));


	vector< uint> indices;
	
	// Create index data
	for (int x = 0; x < TERRAIN_X_LEN; ++x)
	{
		for (int z = 0; z < TERRAIN_Z_LEN; ++z)
		{
			// Define 12 control points per terrain quad

			// 0-3 are the actual quad vertices
			indices.emplace_back((z + 0) + (x + 0) * (TERRAIN_X_LEN + 1));
			indices.emplace_back((z + 1) + (x + 0) * (TERRAIN_X_LEN + 1));
			indices.emplace_back((z + 0) + (x + 1) * (TERRAIN_X_LEN + 1));
			indices.emplace_back((z + 1) + (x + 1) * (TERRAIN_X_LEN + 1));

			// 4-5 are +x
			indices.emplace_back(clamp(z + 0, 0, TERRAIN_Z_LEN) + clamp(x + 2, 0, TERRAIN_X_LEN) * (TERRAIN_X_LEN + 1));
			indices.emplace_back(clamp(z + 1, 0, TERRAIN_Z_LEN) + clamp(x + 2, 0, TERRAIN_X_LEN) * (TERRAIN_X_LEN + 1));

			// 6-7 are +z
			indices.emplace_back(clamp(z + 2, 0, TERRAIN_Z_LEN) + clamp(x + 0, 0, TERRAIN_X_LEN) * (TERRAIN_X_LEN + 1));
			indices.emplace_back(clamp(z + 2, 0, TERRAIN_Z_LEN) + clamp(x + 1, 0, TERRAIN_X_LEN) * (TERRAIN_X_LEN + 1));

			// 8-9 are -x
			indices.emplace_back(clamp(z + 0, 0, TERRAIN_Z_LEN) + clamp(x - 1, 0, TERRAIN_X_LEN) * (TERRAIN_X_LEN + 1));
			indices.emplace_back(clamp(z + 1, 0, TERRAIN_Z_LEN) + clamp(x - 1, 0, TERRAIN_X_LEN) * (TERRAIN_X_LEN + 1));

			// 10-11 are -z			
			indices.emplace_back(clamp(z - 1, 0, TERRAIN_Z_LEN) + clamp(x + 0, 0, TERRAIN_X_LEN) * (TERRAIN_X_LEN + 1));
			indices.emplace_back(clamp(z - 1, 0, TERRAIN_Z_LEN) + clamp(x + 1, 0, TERRAIN_X_LEN) * (TERRAIN_X_LEN + 1));
		}
	}
	indexCount = static_cast<uint>(indices.size());
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(uint) *indexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = indices.data();
	Check(device->CreateBuffer(&desc, &subResource, &indexBuffer));



	vertices.clear();
	vertices.shrink_to_fit();

	indices.clear();
	indices.shrink_to_fit();

}

void TerrainLod::CreateIndexData()
{
}

void TerrainLod::CreateShaders()
{
	SafeRelease(vs);
	SafeRelease(hs);
	SafeRelease(ds);
	SafeRelease(gs);
	SafeRelease(ps);
	SafeRelease(cs);
	

	ID3DBlob* ShaderBlob = nullptr;
	auto path = "../_Shaders/Environment/InterlockingTerrainTiles.hlsl";
	auto entryPoint = "vsMain";
	auto shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &vs));

	inputLayout = make_shared<InputLayout>();
	inputLayout->Create(device, ShaderBlob);
	SafeRelease(ShaderBlob);


	//entryPoint = "hsSimple";
	entryPoint = "hsComplex";
	shaderModel = "hs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateHullShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &hs));
	SafeRelease(ShaderBlob);

	entryPoint = "dsMain";
	shaderModel = "ds_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateDomainShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &ds));
	SafeRelease(ShaderBlob);

	entryPoint = "gsMain";
	shaderModel = "gs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateGeometryShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &gs));
	SafeRelease(ShaderBlob);

	
	
	entryPoint = "psMain";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &ps));
	SafeRelease(ShaderBlob);


	path = "../_Shaders/Environment/InterlockingTerrainTilesComputeShader.hlsl";
	entryPoint = "csMain";
	shaderModel = "cs_5_0";
	
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateComputeShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &cs));
	SafeRelease(ShaderBlob);

}
