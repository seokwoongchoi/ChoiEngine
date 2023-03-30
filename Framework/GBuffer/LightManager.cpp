#include "Framework.h"
#include "LightManager.h"
#include "CascadedShadow.h"
#include "GBufferData.h"
#include "Core/D3D11/D3D11_Helper.h"



extern CascadedShadow  CascadedMatrixSet;
UINT8 SceneStencilFlag = 2;


#pragma pack(push,1)




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
	:device(nullptr),ShowLightVolume(false), LastShadowLight(-1), NextFreeSpotShadowmap(-1), NextFreePointShadowmap(-1),
 DirLightVertexShader(nullptr),  DirLightPixelShader(nullptr),  DirLightCB(nullptr),
 PointLightVertexShader(nullptr),  PointLightHullShader(nullptr),  PointLightDomainShader(nullptr),  PointLightPixelShader(nullptr),  PointLightShadowPixelShader(nullptr),
 PointLightDomainCB(nullptr),  PointLightPixelCB(nullptr),
 SpotLightVertexShader(nullptr),  SpotLightHullShader(nullptr),  SpotLightDomainShader(nullptr), SpotLightPixelShader(nullptr),  SpotLightShadowPixelShader(nullptr),
 SpotLightDomainCB(nullptr),  SpotLightPixelCB(nullptr),
 CapsuleLightVertexShader(nullptr),  CapsuleLightHullShader(nullptr),  CapsuleLightDomainShader(nullptr),  CapsuleLightPixelShader(nullptr),
 CapsuleLightDomainCB(nullptr),  CapsuleLightPixelCB(nullptr), RenderBrushPixelShader(nullptr),
// pShadowGenVSLayout(nullptr),
 
 SpotShadowGenVertexShader(nullptr),  SpotShadowGenVertexCB(nullptr),
 PointShadowGenVertexShader(nullptr),  PointShadowGenGeometryShader(nullptr),  PointShadowGenGeometryCB(nullptr),
 DebugCascadesPixelShader(nullptr),
 NoDepthWriteLessStencilMaskState(nullptr),  NoDepthWriteGreatherStencilMaskState(nullptr),
AdditiveBlendState(nullptr), NoDepthClipFrontRS(nullptr),  WireframeRS(nullptr),
 ShadowGenRS(nullptr),   PCFSamplerState(nullptr)
{
	for (int i = 0; i <  iTotalSpotShadowmaps; i++)
	{
		 SpotDepthStencilRT[i] = nullptr;
		 SpotDepthStencilDSV[i] = nullptr;
		 SpotDepthStencilSRV[i] = nullptr;
	}

	for (int i = 0; i < TotalPointShadowmaps; i++)
	{
		 PointDepthStencilRT[i] = nullptr;
		 PointDepthStencilDSV[i] = nullptr;
		 PointDepthStencilSRV[i] = nullptr;
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
	ID3DBlob* ShaderBlob = nullptr;
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));

    ifstream fin(L"", std::ios::binary);



	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &DirLightVertexShader));
	SafeRelease(ShaderBlob);

	entryPoint = "DirLightPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &DirLightPixelShader));
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
		ShaderBlob->GetBufferSize(), nullptr, &DebugCascadesPixelShader));

	SafeRelease(ShaderBlob);
	entryPoint = "RenderBrushPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &RenderBrushPixelShader));


	SafeRelease(ShaderBlob);
	
	// Load the point light shaders
	path = "../_Shaders/Lighting/PointLight.hlsl";
	entryPoint = "PointLightVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &PointLightVertexShader));
	SafeRelease(ShaderBlob);

	entryPoint = "PointLightHS";
	shaderModel = "hs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateHullShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &PointLightHullShader));
	SafeRelease(ShaderBlob);

	entryPoint = "PointLightDS";
	shaderModel = "ds_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateDomainShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &PointLightDomainShader));
	SafeRelease(ShaderBlob);

	entryPoint = "PointLightPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &PointLightPixelShader));
	SafeRelease(ShaderBlob);

	/*entryPoint = "PointLightShadowPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &PointLightShadowPixelShader));
	SafeRelease(ShaderBlob);*/

	// Load the spot light shaders
	path = "../_Shaders/Lighting/SpotLight.hlsl";
	entryPoint = "SpotLightVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &SpotLightVertexShader));
	SafeRelease(ShaderBlob);

	entryPoint = "SpotLightHS";
	shaderModel = "hs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateHullShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &SpotLightHullShader));
	SafeRelease(ShaderBlob);

	entryPoint = "SpotLightDS";
	shaderModel = "ds_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateDomainShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &SpotLightDomainShader));
	SafeRelease(ShaderBlob);

	entryPoint = "SpotLightPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &SpotLightPixelShader));
	SafeRelease(ShaderBlob);


	/*entryPoint = "SpotLightShadowPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &SpotLightShadowPixelShader));
	SafeRelease(ShaderBlob);*/

	// Load the spot capsule shaders
	//path = "../_Shaders/Lighting/CapsuleLight.hlsl";
	//entryPoint = "CapsuleLightVS";
	//shaderModel = "vs_5_0";
	//Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	//Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(), nullptr, &CapsuleLightVertexShader));
	//SafeRelease(ShaderBlob);

	//entryPoint = "CapsuleLightHS";
	//shaderModel = "hs_5_0";
	//Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	//Check(device->CreateHullShader(ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(), nullptr, &CapsuleLightHullShader));
	//SafeRelease(ShaderBlob);

	//entryPoint = "CapsuleLightDS";
	//shaderModel = "ds_5_0";
	//Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	//Check(device->CreateDomainShader(ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(), nullptr, &CapsuleLightDomainShader));
	//SafeRelease(ShaderBlob);

	//entryPoint = "CapsuleLightPS";
	//shaderModel = "ps_5_0";
	//Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	//Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(), nullptr, &CapsuleLightPixelShader));
	//SafeRelease(ShaderBlob);

	//// Load the shadow generation shaders
	//path = "../_Shaders/Lighting/ShadowGen.hlsl";
	//entryPoint = "SpotShadowGenVS";
	//shaderModel = "vs_5_0";
	//Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	//Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(), nullptr, &SpotShadowGenVertexShader));

	//// Create a layout for the object data
	//const D3D11_INPUT_ELEMENT_DESC layout[] =
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//};
	//Check(device->CreateInputLayout(layout, ARRAYSIZE(layout), ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(), & pShadowGenVSLayout));
	

	/*SafeRelease(ShaderBlob);


	entryPoint = "PointShadowGenVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &PointShadowGenVertexShader));
	SafeRelease(ShaderBlob);

	entryPoint = "PointShadowGenGS";
	shaderModel = "gs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateGeometryShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &PointShadowGenGeometryShader));
	SafeRelease(ShaderBlob);
*/
	


	
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
	Check(device->CreateBuffer(&cbDesc, nullptr, &DirLightCB));
	//DXUT_SetDebugName( pDirLightCB, "Directional Light CB");

	cbDesc.ByteWidth = sizeof(CB_POINT_LIGHT_DOMAIN);
	Check(device->CreateBuffer(&cbDesc, nullptr, &PointLightDomainCB));
	//DXUT_SetDebugName( pPointLightDomainCB, "Point Light Domain CB");

	cbDesc.ByteWidth = sizeof(CB_POINT_LIGHT_PIXEL);
	Check(device->CreateBuffer(&cbDesc, nullptr, &PointLightPixelCB));
	//DXUT_SetDebugName( pPointLightPixelCB, "Point Light Pixel CB");

	cbDesc.ByteWidth = sizeof(CB_SPOT_LIGHT_DOMAIN);
	Check(device->CreateBuffer(&cbDesc, nullptr, &SpotLightDomainCB));
	//DXUT_SetDebugName( pSpotLightDomainCB, "Spot Light Domain CB");

	cbDesc.ByteWidth = sizeof(CB_SPOT_LIGHT_PIXEL);
	Check(device->CreateBuffer(&cbDesc, nullptr, &SpotLightPixelCB));
	//DXUT_SetDebugName( pSpotLightPixelCB, "Spot Light Pixel CB");

	cbDesc.ByteWidth = sizeof(CB_CAPSULE_LIGHT_DOMAIN);
	Check(device->CreateBuffer(&cbDesc, nullptr, &CapsuleLightDomainCB));
	//DXUT_SetDebugName( pCapsuleLightDomainCB, "Capsule Light Domain CB");

	cbDesc.ByteWidth = sizeof(CB_CAPSULE_LIGHT_PIXEL);
	Check(device->CreateBuffer(&cbDesc, nullptr, &CapsuleLightPixelCB));
	//DXUT_SetDebugName( pCapsuleLightPixelCB, "Capsule Light Pixel CB");

	cbDesc.ByteWidth = sizeof(Matrix);
	Check(device->CreateBuffer(&cbDesc, nullptr, &SpotShadowGenVertexCB));
	//DXUT_SetDebugName( pSpotShadowGenVertexCB, "Spot Shadow Gen Vertex CB");

	cbDesc.ByteWidth = 6 * sizeof(Matrix);
	Check(device->CreateBuffer(&cbDesc, nullptr, &PointShadowGenGeometryCB));
	//DXUT_SetDebugName( pPointShadowGenGeometryCB, "Point Shadow Gen Geometry CB");

	
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

	
	
	SafeRelease( DebugCascadesPixelShader);
	SafeRelease( NoDepthWriteLessStencilMaskState);
	
	SafeRelease( NoDepthWriteGreatherStencilMaskState);
	SafeRelease( AdditiveBlendState);
	SafeRelease( NoDepthClipFrontRS);
	SafeRelease( WireframeRS);
	SafeRelease( ShadowGenRS);
	
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


}

void LightManager::Update()
{
	pDirectionalValuesCB.Time += Time::Delta();
	pDirectionalValuesCB.DirToLight = -GlobalData::LightDirection();
	pDirectionalValuesCB.DirectionalColor = GlobalData::LightColor();

	D3DXMatrixTranspose(&pDirectionalValuesCB.ToShadowSpace, &CascadedMatrixSet.WorldToShadowSpace);

	pDirectionalValuesCB.ToCascadeSpace[0] = CascadedMatrixSet.ToCascadeOffsetX;
	pDirectionalValuesCB.ToCascadeSpace[1] = CascadedMatrixSet.ToCascadeOffsetY;
	pDirectionalValuesCB.ToCascadeSpace[2] = CascadedMatrixSet.ToCascadeScale;
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
	context->OMSetDepthStencilState( NoDepthWriteLessStencilMaskState, SceneStencilFlag);

	// Set the GBuffer views
	
	

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
	for (auto& light : pointLights)
	{

		PointLight(context, light.Position, light.Range, light.Color, light.Intencity, light.iShadowmapIdx, false);
	}
	for (auto& light : spotLights)
	{


		SpotLight(context, light.Position, light.Direction, light.Range, light.InnerAngle,
			light.OuterAngle, light.Color, light.iShadowmapIdx, false);

	}
	for (auto& light : capsuleLights)
	{

		CapsuleLight(context, light.Position, light.Direction, light.Range, light.Length, light.Color, false);

	}
	

	// Cleanup
	context->VSSetShader(nullptr, nullptr, 0);
	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);

	// Restore the states
	context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
	SafeRelease(pPrevBlendState);
	context->RSSetState(pPrevRSState);
	SafeRelease(pPrevRSState);
	context->OMSetDepthStencilState(PrevDepthState, PrevStencil);
	SafeRelease(PrevDepthState);

	

	ZeroMemory(arrSamplers, sizeof(arrSamplers));
	context->PSSetSamplers(0, 2, arrSamplers);

	
}

void LightManager::DoDebugLightVolume(ID3D11DeviceContext * context)
{
	// Set wireframe state
	ID3D11RasterizerState* PrevRSState;
	context->RSGetState(&PrevRSState);
	context->RSSetState( WireframeRS);

	for (auto& light : pointLights)
	{
		
		PointLight(context, light.Position, light.Range, light.Color, light.Intencity, light.iShadowmapIdx, true);
	}
	for (auto& light : spotLights)
	{

		
		SpotLight(context, light.Position, light.Direction, light.Range, light.InnerAngle,
			light.OuterAngle, light.Color, light.iShadowmapIdx, true);
		
	}
	for (auto& light : capsuleLights)
	{

		CapsuleLight(context, light.Position, light.Direction, light.Range, light.Length, light.Color, true);

	}


	// Cleanup
	context->VSSetShader(nullptr, nullptr, 0);
	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);

	// Restore the states
	context->RSSetState(PrevRSState);
	SafeRelease(PrevRSState);
}

void LightManager::DoDebugCascadedShadows(ID3D11DeviceContext * context, ID3D11ShaderResourceView* srv)
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
	ID3D11ShaderResourceView* arrViews[1] = { srv };
	context->PSSetShaderResources(0, 1, arrViews);

	// Primitive settings
	context->IASetInputLayout(nullptr);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Set the shaders
	context->VSSetShader( DirLightVertexShader, nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader( DebugCascadesPixelShader, nullptr, 0);

	context->Draw(4, 0);

	// Cleanup
	arrViews[0] = nullptr;
	context->PSSetShaderResources(0, 1, arrViews);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->OMSetBlendState(PrevBlendState, prevBlendFactor, prevSampleMask);
}



void LightManager::DirectionalLight(ID3D11DeviceContext * context)
{
	

	
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map( DirLightCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &pDirectionalValuesCB, sizeof(pDirectionalValuesCB));
	context->Unmap( DirLightCB, 0);
	context->PSSetConstantBuffers(1, 1, & DirLightCB);

	// Primitive settings
	context->IASetInputLayout(nullptr);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Set the shaders
	context->VSSetShader( DirLightVertexShader, nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);

	static bool bAo = true;
	if (Keyboard::Get()->Down('G'))
	{
		bAo == true ? bAo = false : bAo = true;
	}
	if (bAo)
		context->PSSetShader(DirLightPixelShader, nullptr, 0);
	else
		context->PSSetShader(NOAOps, nullptr, 0);

	

	context->Draw(4, 0);

	// Cleanup
	ID3D11ShaderResourceView *arrRV[1] = { nullptr };
	context->PSSetShaderResources(4, 1, arrRV);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void LightManager::PointLight(ID3D11DeviceContext * context, const Vector3 & Pos, float Range, const Vector3 & Color, float Intencity,int iShadowmapIdx, bool bWireframe)
{


	D3DXMatrixScaling(&LightWorldScale, Range, Range, Range);
	
	D3DXMatrixTranslation(&LightWorldTrans, Pos.x, Pos.y, Pos.z);
	Matrix vp;
	GlobalData::GetVP(&vp);
	WorldViewProjection = LightWorldScale * LightWorldTrans * vp;


	
	
	
	{
		D3DXMatrixTranspose(&PointLightDomainDesc.WolrdViewProj, &WorldViewProjection);
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(PointLightDomainCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &PointLightDomainDesc, sizeof(PointLightDomainDesc));
		context->Unmap(PointLightDomainCB, 0);
	
	}
	context->DSSetConstantBuffers(0, 1, &PointLightDomainCB);

	{
		PointLightPixelDesc.PointLightPos = Pos;
		PointLightPixelDesc.PointLightRangeRcp =Range;
		PointLightPixelDesc.PointColor = Color;
		PointLightPixelDesc.Intencity = Intencity;
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(PointLightPixelCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &PointLightPixelDesc, sizeof(PointLightPixelDesc));
		context->Unmap(PointLightPixelCB, 0);

	}
	context->PSSetConstantBuffers(1, 1, &PointLightPixelCB);




	context->IASetInputLayout(NULL);
	context->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);

	// Set the shaders
	context->VSSetShader(PointLightVertexShader, NULL, 0);
	context->HSSetShader(PointLightHullShader, NULL, 0);
	context->DSSetShader(PointLightDomainShader, NULL, 0);
	context->GSSetShader(NULL, NULL, 0);
	context->PSSetShader( PointLightPixelShader, NULL, 0);

	context->Draw(2, 0);
}

void LightManager::SpotLight(ID3D11DeviceContext * context, const Vector3 & Pos, const Vector3 & Dir, float Range, float InnerAngle, float OuterAngle, const Vector3 & Color, int iShadowmapIdx, bool bWireframe)
{
}

void LightManager::CapsuleLight(ID3D11DeviceContext * context, const Vector3 & Pos, const Vector3 & Dir, float Range, float fLen, const Vector3 & Color, bool bWireframe)
{
}

void LightManager::SpotShadowGen(ID3D11DeviceContext * context, const SpotLights & light)
{
}

void LightManager::PointShadowGen(ID3D11DeviceContext * context, const PointLights & light)
{
}

void LightManager::RenderBrush(ID3D11DeviceContext * context, ID3D11Buffer* buffer, ID3D11ShaderResourceView* srv)
{

	// Set the depth state for the directional light
	//context->OMSetDepthStencilState( NoDepthWriteLessStencilMaskState, SceneStencilFlag);

	// Once we are done with the directional light, turn on the blending
	ID3D11BlendState* PrevBlendState;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(&PrevBlendState, prevBlendFactor, &prevSampleMask);
	context->OMSetBlendState(AdditiveBlendState, prevBlendFactor, prevSampleMask);

	// Use the same constant buffer values again

	ID3D11Buffer* buffers[2] = { DirLightCB ,buffer };
	context->PSSetConstantBuffers(1, 2, buffers);

	// Set the GBuffer views
	ID3D11ShaderResourceView* arrViews[1] = { srv };
	context->PSSetShaderResources(0, 1, arrViews);

	// Primitive settings
	context->IASetInputLayout(nullptr);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Set the shaders
	context->VSSetShader(DirLightVertexShader, nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(RenderBrushPixelShader, nullptr, 0);

	context->Draw(4, 0);

	// Cleanup
	
	ZeroMemory(&buffers, sizeof(buffers));
	context->PSSetConstantBuffers(1, 2, buffers);
	arrViews[0] = nullptr;
	context->PSSetShaderResources(0, 1, arrViews);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->OMSetBlendState(PrevBlendState, prevBlendFactor, prevSampleMask);
}



