#include "Framework.h"
#include "LightManager.h"
#include "CascadedShadow.h"
#include "GBufferData.h"
#include "Core/D3D11/D3D11_Helper.h"


extern GBufferData GBuffer;
extern CascadedShadow  CascadedMatrixSet;
UINT8 SceneStencilFlag = 2;


#pragma pack(push,1)
struct CB_DIRECTIONAL
{

	Vector3 DirToLight;
	float pad;
	Vector3 DirectionalColor;
	float pad2;
	Matrix ToShadowSpace;
	Vector4 ToCascadeSpace[3];
};

struct CB_POINT_LIGHT_DOMAIN
{
	Matrix WolrdViewProj;
};

struct CB_POINT_LIGHT_PIXEL
{
	Vector3 PointLightPos;
	float PointLightRangeRcp;
	Vector3 PointColor;
	float pad;
	Vector2 LightPerspectiveValues;
	float pad2[2];
};

struct CB_SPOT_LIGHT_DOMAIN
{
	Matrix WolrdViewProj;
	float fSinAngle;
	float fCosAngle;
	float pad[2];
};

struct CB_SPOT_LIGHT_PIXEL
{
	Vector3 SpotLightPos;
	float SpotLightRangeRcp;
	Vector3 DirToLight;
	float SpotCosOuterCone;
	Vector3 SpotColor;
	float SpotCosConeAttRange;
	Matrix ToShadowmap;
};

struct CB_CAPSULE_LIGHT_DOMAIN
{
	Matrix WolrdViewProj;
	float HalfCapsuleLen;
	float CapsuleRange;
	float pad[2];
};

struct CB_CAPSULE_LIGHT_PIXEL
{
	Vector3 CapsuleLightPos;
	float CapsuleLightRangeRcp;
	Vector3 CapsuleDir;
	float CapsuleLen;
	Vector3 CapsuleColor;
	float pad;
};



#pragma pack(pop)
LightManager::LightManager()
	:device(nullptr),shadowVP{},ShowLightVolume(false), LastShadowLight(-1), NextFreeSpotShadowmap(-1), NextFreePointShadowmap(-1),
 DirLightVertexShader(NULL),  DirLightPixelShader(NULL),  DirLightCB(NULL),
 PointLightVertexShader(NULL),  PointLightHullShader(NULL),  PointLightDomainShader(NULL),  PointLightPixelShader(NULL),  PointLightShadowPixelShader(NULL),
 PointLightDomainCB(NULL),  PointLightPixelCB(NULL),
 SpotLightVertexShader(NULL),  SpotLightHullShader(NULL),  SpotLightDomainShader(NULL), SpotLightPixelShader(NULL),  SpotLightShadowPixelShader(NULL),
 SpotLightDomainCB(NULL),  SpotLightPixelCB(NULL),
 CapsuleLightVertexShader(NULL),  CapsuleLightHullShader(NULL),  CapsuleLightDomainShader(NULL),  CapsuleLightPixelShader(NULL),
 CapsuleLightDomainCB(NULL),  CapsuleLightPixelCB(NULL),
// pShadowGenVSLayout(NULL),
 CascadedShadowGenGeometryShader(NULL),
 SpotShadowGenVertexShader(NULL),  SpotShadowGenVertexCB(NULL),
 PointShadowGenVertexShader(NULL),  PointShadowGenGeometryShader(NULL),  PointShadowGenGeometryCB(NULL),
  CascadedShadowGenGeometryCB(NULL), DebugCascadesPixelShader(NULL),
 NoDepthWriteLessStencilMaskState(NULL),  NoDepthWriteGreatherStencilMaskState(NULL),  ShadowGenDepthState(NULL),
AdditiveBlendState(NULL), NoDepthClipFrontRS(NULL),  WireframeRS(NULL),
 ShadowGenRS(NULL),  CascadedShadowGenRS(NULL),  PCFSamplerState(NULL),
CascadedDepthStencilRT(NULL), CascadedDepthStencilDSV(NULL), CascadedDepthStencilSRV(NULL)
{
	for (int i = 0; i <  iTotalSpotShadowmaps; i++)
	{
		 SpotDepthStencilRT[i] = NULL;
		 SpotDepthStencilDSV[i] = NULL;
		 SpotDepthStencilSRV[i] = NULL;
	}

	for (int i = 0; i < TotalPointShadowmaps; i++)
	{
		 PointDepthStencilRT[i] = NULL;
		 PointDepthStencilDSV[i] = NULL;
		 PointDepthStencilSRV[i] = NULL;
	}
}


LightManager::~LightManager()
{
}

void LightManager::Init(ID3D11Device* device)
{
	this->device = device;




	CreateBuffers(device);
	CreateShaders(device);


	D3D11_DEPTH_STENCIL_DESC descDepth;
	descDepth.DepthEnable = TRUE;
	descDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	descDepth.DepthFunc = D3D11_COMPARISON_LESS;
	descDepth.StencilEnable = TRUE;
	descDepth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	descDepth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	const D3D11_DEPTH_STENCILOP_DESC noSkyStencilOp = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_EQUAL };
	descDepth.FrontFace = noSkyStencilOp;
	descDepth.BackFace = noSkyStencilOp;
	Check(device->CreateDepthStencilState(&descDepth, & NoDepthWriteLessStencilMaskState));
	
	descDepth.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
	Check(device->CreateDepthStencilState(&descDepth, & NoDepthWriteGreatherStencilMaskState));
	

	descDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	descDepth.DepthFunc = D3D11_COMPARISON_LESS;
	descDepth.StencilEnable = true;
	descDepth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	descDepth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	
	Check(device->CreateDepthStencilState(&descDepth, & ShadowGenDepthState));
	
	// Create the additive blend state
	D3D11_BLEND_DESC descBlend;
	descBlend.AlphaToCoverageEnable = FALSE;
	descBlend.IndependentBlendEnable = FALSE;
	const D3D11_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		TRUE,
		D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		descBlend.RenderTarget[i] = defaultRenderTargetBlendDesc;
	Check(device->CreateBlendState(&descBlend, & AdditiveBlendState));
	
	D3D11_RASTERIZER_DESC descRast = {
		D3D11_FILL_SOLID,
		D3D11_CULL_FRONT,
		FALSE,
		D3D11_DEFAULT_DEPTH_BIAS,
		D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
		D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		TRUE,
		FALSE,
		FALSE,
		FALSE
	};
	Check(device->CreateRasterizerState(&descRast, & NoDepthClipFrontRS));

	descRast.CullMode = D3D11_CULL_BACK;
	descRast.FillMode = D3D11_FILL_WIREFRAME;
	Check(device->CreateRasterizerState(&descRast, & WireframeRS));
	

	descRast.FillMode = D3D11_FILL_SOLID;
	descRast.DepthBias = 6;
	descRast.SlopeScaledDepthBias = 1.0f;
	Check(device->CreateRasterizerState(&descRast, & ShadowGenRS));
	
	descRast.DepthBias =6;
	descRast.CullMode = D3D11_CULL_BACK;
	descRast.SlopeScaledDepthBias = 3;
	descRast.DepthClipEnable = false;
	Check(device->CreateRasterizerState(&descRast, & CascadedShadowGenRS));
	

	// Create the PCF sampler state
	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &sampLinear));

	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, & PCFSamplerState));
	
	// Allocate the depth stencil target
	D3D11_TEXTURE2D_DESC dtd = {
		 1024, //UINT Width;
		 1024, //UINT Height;
		1, //UINT MipLevels;
		1, //UINT ArraySize;
		//DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT_R32_TYPELESS, //DXGI_FORMAT Format;
		1, //DXGI_SAMPLE_DESC SampleDesc;
		0,
		D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
		0,//UINT CPUAccessFlags;
		0//UINT MiscFlags;    
	};
	D3D11_DEPTH_STENCIL_VIEW_DESC descDepthView =
	{
		// DXGI_FORMAT_D24_UNOR S8_UINT,
		DXGI_FORMAT_D32_FLOAT,
		D3D11_DSV_DIMENSION_TEXTURE2D,
		0
	};
	D3D11_SHADER_RESOURCE_VIEW_DESC descShaderView =
	{
		//DXGI_FORMAT_R24_UNOR X8_TYPELESS,
		DXGI_FORMAT_R32_FLOAT,
		D3D11_SRV_DIMENSION_TEXTURE2D,
		0,
		0
	};
	descShaderView.Texture2D.MipLevels = 1;

	char strResName[32];
	for (int i = 0; i <  iTotalSpotShadowmaps; i++)
	{
		sprintf_s(strResName, "Spot Shadowmap Target %d", i);
		Check(device->CreateTexture2D(&dtd, NULL, & SpotDepthStencilRT[i]));
		

		sprintf_s(strResName, "Spot Shadowmap Depth View %d", i);
		Check(device->CreateDepthStencilView( SpotDepthStencilRT[i], &descDepthView, & SpotDepthStencilDSV[i]));
		
		sprintf_s(strResName, "Spot Shadowmap Resource View %d", i);
		Check(device->CreateShaderResourceView( SpotDepthStencilRT[i], &descShaderView, & SpotDepthStencilSRV[i]));
		
	}

	// Allocate the point shadow targets and views
	
	dtd.ArraySize = 6;
	dtd.MiscFlags = D3D10_RESOURCE_MISC_TEXTURECUBE;
	descDepthView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	descDepthView.Texture2DArray.FirstArraySlice = 0;
	descDepthView.Texture2DArray.ArraySize = 6;
	descShaderView.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	descShaderView.TextureCube.MipLevels = 1;
	descShaderView.TextureCube.MostDetailedMip = 0;
	for (int i = 0; i < TotalPointShadowmaps; i++)
	{
		sprintf_s(strResName, "Point Shadowmap Target %d", i);
		Check(device->CreateTexture2D(&dtd, NULL, & PointDepthStencilRT[i]));
		

		sprintf_s(strResName, "Point Shadowmap Depth View %d", i);
		Check(device->CreateDepthStencilView( PointDepthStencilRT[i], &descDepthView, & PointDepthStencilDSV[i]));
		
		sprintf_s(strResName, "Point Shadowmap Resource View %d", i);
		Check(device->CreateShaderResourceView( PointDepthStencilRT[i], &descShaderView, & PointDepthStencilSRV[i]));
		
	}

	float width = static_cast<float>(D3D::Width());
    float height = static_cast<float>(D3D::Height());
	CreateCascadedShadowBuffers(Vector2(width,height));
	
	

}

void LightManager::CreateShaders(ID3D11Device* device)
{
	// Compile the shaders
	//DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	//dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	auto path = "../_Shaders/Lighting/DirLight.hlsl";
	auto entryPoint = "DirLightVS";
	auto shaderModel = "vs_5_0";
	// Load the directional light shaders
	ID3DBlob* ShaderBlob = NULL;
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	/*HANDLE fileHandle = CreateFile(L"../_Shaders/Lighting/DirLight_vs.cso", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	bool bChecked = fileHandle != INVALID_HANDLE_VALUE;
	assert(bChecked);

	DWORD dataSize = GetFileSize(fileHandle, NULL);
	assert(dataSize != 0xFFFFFFFF);

	void* data = malloc(dataSize);
	DWORD readSize;
	Check(ReadFile(fileHandle, data, dataSize, &readSize, NULL));

	CloseHandle(fileHandle);
	fileHandle = NULL;

	D3DCreateBlob(dataSize, &ShaderBlob
	);
	memcpy(ShaderBlob->GetBufferPointer(), data, dataSize);*/
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &DirLightVertexShader));
	SafeRelease(ShaderBlob);

	entryPoint = "DirLightPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &DirLightPixelShader));
	SafeRelease(ShaderBlob);


	entryPoint = "DirLightNOAOPS";
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
		&NOAOps
	));

	SafeRelease(ShaderBlob);
	entryPoint = "CascadeShadowDebugPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &DebugCascadesPixelShader));
	SafeRelease(ShaderBlob);

	// Load the point light shaders
	path = "../_Shaders/Lighting/PointLight.hlsl";
	entryPoint = "PointLightVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &PointLightVertexShader));
	SafeRelease(ShaderBlob);

	entryPoint = "PointLightHS";
	shaderModel = "hs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateHullShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &PointLightHullShader));
	SafeRelease(ShaderBlob);

	entryPoint = "PointLightDS";
	shaderModel = "ds_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateDomainShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &PointLightDomainShader));
	SafeRelease(ShaderBlob);

	entryPoint = "PointLightPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &PointLightPixelShader));
	SafeRelease(ShaderBlob);

	entryPoint = "PointLightShadowPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &PointLightShadowPixelShader));
	SafeRelease(ShaderBlob);

	// Load the spot light shaders
	path = "../_Shaders/Lighting/SpotLight.hlsl";
	entryPoint = "SpotLightVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &SpotLightVertexShader));
	SafeRelease(ShaderBlob);

	entryPoint = "SpotLightHS";
	shaderModel = "hs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateHullShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &SpotLightHullShader));
	SafeRelease(ShaderBlob);

	entryPoint = "SpotLightDS";
	shaderModel = "ds_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateDomainShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &SpotLightDomainShader));
	SafeRelease(ShaderBlob);

	entryPoint = "SpotLightPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &SpotLightPixelShader));
	SafeRelease(ShaderBlob);


	entryPoint = "SpotLightShadowPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &SpotLightShadowPixelShader));
	SafeRelease(ShaderBlob);

	// Load the spot capsule shaders
	path = "../_Shaders/Lighting/CapsuleLight.hlsl";
	entryPoint = "CapsuleLightVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &CapsuleLightVertexShader));
	SafeRelease(ShaderBlob);

	entryPoint = "CapsuleLightHS";
	shaderModel = "hs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateHullShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &CapsuleLightHullShader));
	SafeRelease(ShaderBlob);

	entryPoint = "CapsuleLightDS";
	shaderModel = "ds_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateDomainShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &CapsuleLightDomainShader));
	SafeRelease(ShaderBlob);

	entryPoint = "CapsuleLightPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &CapsuleLightPixelShader));
	SafeRelease(ShaderBlob);

	// Load the shadow generation shaders
	path = "../_Shaders/Lighting/ShadowGen.hlsl";
	entryPoint = "SpotShadowGenVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &SpotShadowGenVertexShader));

	//// Create a layout for the object data
	//const D3D11_INPUT_ELEMENT_DESC layout[] =
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//};
	//Check(device->CreateInputLayout(layout, ARRAYSIZE(layout), ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(), & pShadowGenVSLayout));
	

	SafeRelease(ShaderBlob);


	entryPoint = "PointShadowGenVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &PointShadowGenVertexShader));
	SafeRelease(ShaderBlob);

	entryPoint = "PointShadowGenGS";
	shaderModel = "gs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateGeometryShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &PointShadowGenGeometryShader));
	SafeRelease(ShaderBlob);

	

	entryPoint = "CascadedShadowMapsGenGS";
	shaderModel = "gs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateGeometryShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &CascadedShadowGenGeometryShader));
	SafeRelease(ShaderBlob);

	
}

void LightManager::CreateBuffers(ID3D11Device* device)
{
	// Create constant buffers
	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory(&cbDesc, sizeof(cbDesc));
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.ByteWidth = sizeof(CB_DIRECTIONAL);
	Check(device->CreateBuffer(&cbDesc, NULL, &DirLightCB));
	//DXUT_SetDebugName( pDirLightCB, "Directional Light CB");

	cbDesc.ByteWidth = sizeof(CB_POINT_LIGHT_DOMAIN);
	Check(device->CreateBuffer(&cbDesc, NULL, &PointLightDomainCB));
	//DXUT_SetDebugName( pPointLightDomainCB, "Point Light Domain CB");

	cbDesc.ByteWidth = sizeof(CB_POINT_LIGHT_PIXEL);
	Check(device->CreateBuffer(&cbDesc, NULL, &PointLightPixelCB));
	//DXUT_SetDebugName( pPointLightPixelCB, "Point Light Pixel CB");

	cbDesc.ByteWidth = sizeof(CB_SPOT_LIGHT_DOMAIN);
	Check(device->CreateBuffer(&cbDesc, NULL, &SpotLightDomainCB));
	//DXUT_SetDebugName( pSpotLightDomainCB, "Spot Light Domain CB");

	cbDesc.ByteWidth = sizeof(CB_SPOT_LIGHT_PIXEL);
	Check(device->CreateBuffer(&cbDesc, NULL, &SpotLightPixelCB));
	//DXUT_SetDebugName( pSpotLightPixelCB, "Spot Light Pixel CB");

	cbDesc.ByteWidth = sizeof(CB_CAPSULE_LIGHT_DOMAIN);
	Check(device->CreateBuffer(&cbDesc, NULL, &CapsuleLightDomainCB));
	//DXUT_SetDebugName( pCapsuleLightDomainCB, "Capsule Light Domain CB");

	cbDesc.ByteWidth = sizeof(CB_CAPSULE_LIGHT_PIXEL);
	Check(device->CreateBuffer(&cbDesc, NULL, &CapsuleLightPixelCB));
	//DXUT_SetDebugName( pCapsuleLightPixelCB, "Capsule Light Pixel CB");

	cbDesc.ByteWidth = sizeof(Matrix);
	Check(device->CreateBuffer(&cbDesc, NULL, &SpotShadowGenVertexCB));
	//DXUT_SetDebugName( pSpotShadowGenVertexCB, "Spot Shadow Gen Vertex CB");

	cbDesc.ByteWidth = 6 * sizeof(Matrix);
	Check(device->CreateBuffer(&cbDesc, NULL, &PointShadowGenGeometryCB));
	//DXUT_SetDebugName( pPointShadowGenGeometryCB, "Point Shadow Gen Geometry CB");

	cbDesc.ByteWidth = 3 * sizeof(Matrix);
	Check(device->CreateBuffer(&cbDesc, NULL, &CascadedShadowGenGeometryCB));
	//DXUT_SetDebugName( pCascadedShadowGenGeometryCB, "Point Shadow Gen Geometry CB");
}

void LightManager::Deinit()
{
	SafeRelease( DirLightVertexShader);
	SafeRelease( DirLightPixelShader);
	
	SafeRelease( DirLightCB);
	SafeRelease( PointLightVertexShader);
	SafeRelease( PointLightHullShader);
	SafeRelease( PointLightDomainShader);
	SafeRelease( PointLightPixelShader);
	SafeRelease( PointLightShadowPixelShader);
	SafeRelease( PointLightDomainCB);
	SafeRelease( PointLightPixelCB);
	SafeRelease( SpotLightVertexShader);
	SafeRelease( SpotLightHullShader);
	SafeRelease( SpotLightDomainShader);
	SafeRelease( SpotLightPixelShader);
	SafeRelease( SpotLightShadowPixelShader);
	SafeRelease( SpotLightDomainCB);
	SafeRelease( SpotLightPixelCB);
	SafeRelease( CapsuleLightVertexShader);
	SafeRelease( CapsuleLightHullShader);
	SafeRelease( CapsuleLightDomainShader);
	SafeRelease( CapsuleLightPixelShader);
	SafeRelease( CapsuleLightDomainCB);
	SafeRelease( CapsuleLightPixelCB);
	//SafeRelease( ShadowGenVSLayout);
	SafeRelease( SpotShadowGenVertexShader);
	SafeRelease( SpotShadowGenVertexCB);
	SafeRelease( PointShadowGenVertexShader);
	SafeRelease( PointShadowGenGeometryShader);
	SafeRelease( PointShadowGenGeometryCB);

	SafeRelease( CascadedShadowGenGeometryShader);
	SafeRelease( CascadedShadowGenGeometryCB);
	
	SafeRelease( DebugCascadesPixelShader);
	SafeRelease( NoDepthWriteLessStencilMaskState);
	SafeRelease( ShadowGenDepthState);
	SafeRelease( NoDepthWriteGreatherStencilMaskState);
	SafeRelease( AdditiveBlendState);
	SafeRelease( NoDepthClipFrontRS);
	SafeRelease( WireframeRS);
	SafeRelease( ShadowGenRS);
	SafeRelease( CascadedShadowGenRS);
	SafeRelease( PCFSamplerState);

	for (int i = 0; i <  iTotalSpotShadowmaps; i++)
	{
		SafeRelease( SpotDepthStencilRT[i]);
		SafeRelease( SpotDepthStencilDSV[i]);
		SafeRelease( SpotDepthStencilSRV[i]);
	}

	for (int i = 0; i < TotalPointShadowmaps; i++)
	{
		SafeRelease( PointDepthStencilRT[i]);
		SafeRelease( PointDepthStencilDSV[i]);
		SafeRelease( PointDepthStencilSRV[i]);
	}

	SafeRelease(CascadedDepthStencilRT);
	SafeRelease(CascadedDepthStencilDSV);
	SafeRelease(CascadedDepthStencilSRV);
}

void LightManager::Update()
{
}

void LightManager::ClearShadowPipeLine(ID3D11DeviceContext * context)
{
	ID3D11Buffer* nullBuffer = nullptr;
	context->VSSetShader(nullptr, nullptr, 0);
	context->GSSetConstantBuffers(6, 1, &nullBuffer);
	context->GSSetShader(nullptr, nullptr, 0);
	ID3D11DepthStencilState* nullState = nullptr;
	context->OMSetDepthStencilState(nullState, 0);
}

void LightManager::Resize(const Vector2 & size)
{
	CreateCascadedShadowBuffers(size);
	CascadedMatrixSet.SetShadowMapSize(size);
	for (uint i = 0; i < 3; i++)
	{
		shadowVP[i].Width = size.x;
		shadowVP[i].Height = size.y;
		
	}
}


void LightManager::DoLighting(ID3D11DeviceContext * context)
{
	// Set the shadowmapping PCF sampler
	ID3D11SamplerState* arrSamplers[2] = { sampLinear, PCFSamplerState };
	context->PSSetSamplers(0, 2, arrSamplers);

	// Store the previous depth state
	ID3D11DepthStencilState* PrevDepthState;
	UINT PrevStencil;
	context->OMGetDepthStencilState(&PrevDepthState, &PrevStencil);

	// Set the depth state for the directional light
	//context->OMSetDepthStencilState( NoDepthWriteLessStencilMaskState, SceneStencilFlag);

	// Set the GBuffer views
	
	ID3D11ShaderResourceView* arrViews[4] = { GBuffer.DepthstencilSRV(), GBuffer.DiffuseSRV(), GBuffer.NormalSRV() ,GBuffer.SpecularSRV() };
	context->PSSetShaderResources(0, 4, arrViews);

	// Do the directional light
	DirectionalLight(context);

	// Once we are done with the directional light, turn on the blending
	ID3D11BlendState* pPrevBlendState;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);
	context->OMSetBlendState( AdditiveBlendState, prevBlendFactor, prevSampleMask);

	// Set the depth state for the rest of the lights
	context->OMSetDepthStencilState( NoDepthWriteGreatherStencilMaskState, SceneStencilFlag);

	ID3D11RasterizerState* pPrevRSState;
	context->RSGetState(&pPrevRSState);
	context->RSSetState(NoDepthClipFrontRS);

	// Do the rest of the lights
	for (auto& light:arrLights)
	{
		if (light.eLightType == TYPE_POINT)
		{
			PointLight(context, light.Position, light.Range, light.Color, light.iShadowmapIdx, false);
		}
		else if (light.eLightType == TYPE_SPOT)
		{
			SpotLight(context, light.Position, light.Direction, light.Range, light.InnerAngle,
				light.OuterAngle, light.Color, light.iShadowmapIdx, false);
		}
		else if (light.eLightType == TYPE_CAPSULE)
		{
			CapsuleLight(context, light.Position, light.Direction, light.Range, light.Length, light.Color, false);
		}
	}

	// Cleanup
	context->VSSetShader(NULL, NULL, 0);
	context->HSSetShader(NULL, NULL, 0);
	context->DSSetShader(NULL, NULL, 0);
	context->PSSetShader(NULL, NULL, 0);

	// Restore the states
	context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
	SafeRelease(pPrevBlendState);
	context->RSSetState(pPrevRSState);
	SafeRelease(pPrevRSState);
	context->OMSetDepthStencilState(PrevDepthState, PrevStencil);
	SafeRelease(PrevDepthState);

	// Cleanup
	ZeroMemory(arrViews, sizeof(arrViews));
	context->PSSetShaderResources(0, 4, arrViews);

	ZeroMemory(arrSamplers, sizeof(arrSamplers));
	context->PSSetSamplers(0, 2, arrSamplers);

	
}

void LightManager::DoDebugLightVolume(ID3D11DeviceContext * context)
{
	// Set wireframe state
	ID3D11RasterizerState* PrevRSState;
	context->RSGetState(&PrevRSState);
	context->RSSetState( WireframeRS);

	for (auto& light : arrLights)
	{
		if (light.eLightType == TYPE_POINT)
		{
			PointLight(context, light.Position, light.Range, light.Color, light.iShadowmapIdx, true);
		}
		else if (light.eLightType == TYPE_SPOT)
		{
			SpotLight(context, light.Position, light.Direction, light.Range, light.InnerAngle,
				light.OuterAngle, light.Color, light.iShadowmapIdx, true);
		}
		else if (light.eLightType == TYPE_CAPSULE)
		{
			CapsuleLight(context, light.Position, light.Direction, light.Range, light.Length, light.Color, true);
		}
	}

	// Cleanup
	context->VSSetShader(NULL, NULL, 0);
	context->HSSetShader(NULL, NULL, 0);
	context->DSSetShader(NULL, NULL, 0);
	context->PSSetShader(NULL, NULL, 0);

	// Restore the states
	context->RSSetState(PrevRSState);
	SafeRelease(PrevRSState);
}

void LightManager::DoDebugCascadedShadows(ID3D11DeviceContext * context)
{
	

	// Set the depth state for the directional light
	//context->OMSetDepthStencilState( NoDepthWriteLessStencilMaskState, SceneStencilFlag);

	// Once we are done with the directional light, turn on the blending
	ID3D11BlendState* PrevBlendState;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(&PrevBlendState, prevBlendFactor, &prevSampleMask);
	context->OMSetBlendState( AdditiveBlendState, prevBlendFactor, prevSampleMask);

	// Use the same constant buffer values again
	context->PSSetConstantBuffers(1, 1, & DirLightCB);

	// Set the GBuffer views
	ID3D11ShaderResourceView* arrViews[1] = { GBuffer.DepthstencilSRV() };
	context->PSSetShaderResources(0, 1, arrViews);

	// Primitive settings
	context->IASetInputLayout(NULL);
	context->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Set the shaders
	context->VSSetShader( DirLightVertexShader, NULL, 0);
	context->GSSetShader(NULL, NULL, 0);
	context->PSSetShader( DebugCascadesPixelShader, NULL, 0);

	context->Draw(4, 0);

	// Cleanup
	arrViews[0] = NULL;
	context->PSSetShaderResources(0, 1, arrViews);
	context->VSSetShader(NULL, NULL, 0);
	context->PSSetShader(NULL, NULL, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->OMSetBlendState(PrevBlendState, prevBlendFactor, prevSampleMask);
}

bool LightManager::PrepareNextShadowLight(ID3D11DeviceContext * context)
{
	// Set the shadow rasterizer state with the bias
	
	context->OMSetDepthStencilState(ShadowGenDepthState, 0);
	context->RSSetState(CascadedShadowGenRS);

	
	CascadedShadowsGen(context);
	return false;
}

void LightManager::CreateCascadedShadowBuffers(const Vector2 & size)
{
	SafeRelease(CascadedDepthStencilSRV);
	SafeRelease(CascadedDepthStencilDSV);
	SafeRelease(CascadedDepthStencilRT);
	// Allocate the depth stencil target
	D3D11_TEXTURE2D_DESC dtd = {
		 static_cast<uint>(size.x), //UINT Width;
		 static_cast<uint>(size.y), //UINT Height;
		1, //UINT MipLevels;
		CascadedShadow::TotalCascades, //UINT ArraySize;
		//DXGI_FORMAT_R24G8_TYPELESS,
		DXGI_FORMAT_R32_TYPELESS, //DXGI_FORMAT Format;
		1, //DXGI_SAMPLE_DESC SampleDesc;
		0,
		D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
		0,//UINT CPUAccessFlags;
		0//UINT MiscFlags;    
	};

	Check(device->CreateTexture2D(&dtd, NULL, &CascadedDepthStencilRT));

	D3D11_DEPTH_STENCIL_VIEW_DESC descDepthView;
	ZeroMemory(&descDepthView, sizeof(descDepthView));
	descDepthView.Format = DXGI_FORMAT_D32_FLOAT;//;
	descDepthView.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	descDepthView.Texture2DArray.FirstArraySlice = 0;
	descDepthView.Texture2DArray.ArraySize = CascadedShadow::TotalCascades;
	Check(device->CreateDepthStencilView(CascadedDepthStencilRT, &descDepthView, &CascadedDepthStencilDSV));

	D3D11_SHADER_RESOURCE_VIEW_DESC descShaderView;
	ZeroMemory(&descShaderView, sizeof(descShaderView));
	descShaderView.Format = DXGI_FORMAT_R32_FLOAT;
	descShaderView.Texture2D.MipLevels = 1;
	descShaderView.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	descShaderView.Texture2DArray.FirstArraySlice = 0;
	descShaderView.Texture2DArray.ArraySize = CascadedShadow::TotalCascades;
	Check(device->CreateShaderResourceView(CascadedDepthStencilRT, &descShaderView, &CascadedDepthStencilSRV));


	CascadedMatrixSet.Init(size);

	for (uint i = 0; i < CascadedShadow::TotalCascades; i++)
	{
		shadowVP[i].Width = size.x;
		shadowVP[i].Height = size.y;
		shadowVP[i].TopLeftX = 0;
		shadowVP[i].TopLeftY = 0;
		shadowVP[i].MinDepth = 0.0f;
		shadowVP[i].MaxDepth = 1.0f;
	}
}

void LightManager::DirectionalLight(ID3D11DeviceContext * context)
{
	

	
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map( DirLightCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	
	
	{
		CB_DIRECTIONAL pDirectionalValuesCB;
		pDirectionalValuesCB.DirToLight = -GlobalData::LightDirection();
		pDirectionalValuesCB.DirectionalColor = GlobalData::LightColor();
		
		D3DXMatrixTranspose(&pDirectionalValuesCB.ToShadowSpace , &CascadedMatrixSet.WorldToShadowSpace);

		pDirectionalValuesCB.ToCascadeSpace[0] = CascadedMatrixSet.ToCascadeOffsetX;
		pDirectionalValuesCB.ToCascadeSpace[1] = CascadedMatrixSet.ToCascadeOffsetY;
		pDirectionalValuesCB.ToCascadeSpace[2] = CascadedMatrixSet.ToCascadeScale;
		memcpy(MappedResource.pData, &pDirectionalValuesCB, sizeof(pDirectionalValuesCB));
	}
	
	context->Unmap( DirLightCB, 0);
	context->PSSetConstantBuffers(1, 1, & DirLightCB);

	// Set the cascaded shadow map if casting shadows
	{
	
		context->PSSetShaderResources(4, 1, &CascadedDepthStencilSRV);
	}

	// Primitive settings
	context->IASetInputLayout(NULL);
	context->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Set the shaders
	context->VSSetShader( DirLightVertexShader, NULL, 0);
	context->GSSetShader(NULL, NULL, 0);

	static bool bAo = true;
	if (Keyboard::Get()->Down('G'))
	{
		bAo == true ? bAo = false : bAo = true;
	}
	if (bAo)
		context->PSSetShader(DirLightPixelShader, NULL, 0);
	else
		context->PSSetShader(NOAOps, nullptr, 0);

	

	context->Draw(4, 0);

	// Cleanup
	ID3D11ShaderResourceView *arrRV[1] = { NULL };
	context->PSSetShaderResources(4, 1, arrRV);
	context->VSSetShader(NULL, NULL, 0);
	context->PSSetShader(NULL, NULL, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void LightManager::PointLight(ID3D11DeviceContext * context, const Vector3 & Pos, float Range, const Vector3 & Color, int iShadowmapIdx, bool bWireframe)
{
}

void LightManager::SpotLight(ID3D11DeviceContext * context, const Vector3 & Pos, const Vector3 & Dir, float Range, float InnerAngle, float OuterAngle, const Vector3 & Color, int iShadowmapIdx, bool bWireframe)
{
}

void LightManager::CapsuleLight(ID3D11DeviceContext * context, const Vector3 & Pos, const Vector3 & Dir, float Range, float fLen, const Vector3 & Color, bool bWireframe)
{
}

void LightManager::SpotShadowGen(ID3D11DeviceContext * context, const LIGHT & light)
{
}

void LightManager::PointShadowGen(ID3D11DeviceContext * context, const LIGHT & light)
{
}



void LightManager::CascadedShadowsGen(ID3D11DeviceContext * context)
{
	

		context->RSSetViewports(3, shadowVP);

		// Set the depth target
		ID3D11RenderTargetView* nullRT = NULL;
		context->OMSetRenderTargets(1, &nullRT, CascadedDepthStencilDSV);

		// Clear the depth stencil
		context->ClearDepthStencilView(CascadedDepthStencilDSV, D3D11_CLEAR_DEPTH, 1.0, 0);


	

	// Get the cascade matrices for the current camera configuration
	CascadedMatrixSet.Update(GlobalData::LightDirection());
	// Fill the shadow generation matrices constant buffer
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(CascadedShadowGenGeometryCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &CascadedMatrixSet.arrWorldToCascadeProj, sizeof(Matrix)*CascadedShadow::TotalCascades);
	context->Unmap(CascadedShadowGenGeometryCB, 0);
	
	
	context->GSSetConstantBuffers(6, 1, & CascadedShadowGenGeometryCB);


	// Set the shadow generation shaders
	context->GSSetShader( CascadedShadowGenGeometryShader, NULL, 0);
	context->PSSetShader(NULL, NULL, 0);
}
