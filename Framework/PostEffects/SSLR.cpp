#include "Framework.h"
#include "SSLR.h"

SSLR::SSLR(ID3D11Device* device, uint width,uint height)
	:device(device),occlusionDesc{}, lightRayDesc{}, oldvp{},ShowRayTraceRes(false), factor(-1), OcclusionTex(NULL), OcclusionUAV(NULL), OcclusionSRV(NULL),
	LightRaysTex(NULL), LightRaysRTV(NULL), LightRaysSRV(NULL),
	 OcclusionCB(NULL),  OcclusionCS(NULL),  RayTraceCB(NULL),  FullScreenVS(NULL),  RayTracePS(NULL),  CombinePS(NULL),
	AdditiveBlendState(NULL), yellow(Vector3(0.9f, 0.87f, 0.6f)), Params{}
{
	
	/*Params.MaxSunDist = 5.641f;											   
	Params.intensity = 0.5f;
	Params.decay = 0.2f;
	Params.ddecay = 0.3f;
	Params.dist = 100.0f;
	Params.MaxDeltaLen = 0.005f;*/

	Params.MaxSunDist = 5.641f;
	Params.intensity = 0.246f;
	Params.decay = 0.044f;
	Params.ddecay = 0.1f;
	Params.dist = 1000.0f;
	Params.MaxDeltaLen = 0.005f; 

	Resize(width, height);
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////
	// Allocate the occlussion constant buffer
	D3D11_BUFFER_DESC CBDesc;
	ZeroMemory(&CBDesc, sizeof(CBDesc));
	CBDesc.Usage = D3D11_USAGE_DYNAMIC;
	CBDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	CBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	CBDesc.ByteWidth = sizeof(CB_OCCLUSSION);
	Check(device->CreateBuffer(&CBDesc, NULL, &OcclusionCB));
	//DXUT_SetDebugName(OcclusionCB, "SSLR - Occlussion CB");

	CBDesc.ByteWidth = sizeof(CB_LIGHT_RAYS);
	Check(device->CreateBuffer(&CBDesc, NULL, &RayTraceCB));
	//DXUT_SetDebugName(RayTraceCB, "SSLR - Ray Trace CB");
	

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;// | D3DCOMPILE_WARNINGS_ARE_ERRORS;


	
	ID3DBlob* pShaderBlob = NULL;
	ID3DBlob* error;
	
	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/SSLR.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "Occlussion", "cs_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));

	Check(device->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), NULL, &OcclusionCS));
	//DXUT_SetDebugName( OcclusionCS, "SSLR - Occlussion CS");
	SafeRelease(pShaderBlob);

	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/SSLR.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "RayTraceVS", "vs_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));
	//V_RETURN(CompileShader(str, NULL, "RayTraceVS", "vs_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreateVertexShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, & FullScreenVS));
//	DXUT_SetDebugName( FullScreenVS, "SSLR - Full Screen VS");
	SafeRelease(pShaderBlob);
	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/SSLR.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "RayTracePS", "ps_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));
	
	Check(device->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, & RayTracePS));
	//DXUT_SetDebugName( RayTracePS, "SSLR - Ray Trace PS");
	SafeRelease(pShaderBlob);
	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/SSLR.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "CombinePS", "ps_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));

//	V_RETURN(CompileShader(str, NULL, "CombinePS", "ps_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, & CombinePS));
	//DXUT_SetDebugName( CombinePS, "SSLR - Combine PS");
	SafeRelease(pShaderBlob);



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
	Check(device->CreateBlendState(&descBlend, &AdditiveBlendState));

	scatterTexture = new Texture();
	scatterTexture->Load(device, L"Particles/scattering.jpg", nullptr);
}

SSLR::~SSLR()
{
	
	SafeRelease( OcclusionTex);
	SafeRelease( OcclusionUAV);
	SafeRelease( OcclusionSRV);
	SafeRelease( LightRaysTex);
	SafeRelease( LightRaysRTV);
	SafeRelease( LightRaysSRV);
	SafeRelease( OcclusionCB);
	SafeRelease( OcclusionCS);
	SafeRelease( RayTraceCB);
	SafeRelease( FullScreenVS);
	SafeRelease( RayTracePS);
	SafeRelease( CombinePS);
	SafeRelease( AdditiveBlendState);
		
}

void SSLR::Render(ID3D11DeviceContext* DC, ID3D11ShaderResourceView * ssaoSRV, ID3D11RenderTargetView* pLightAccumRTV)
{
	
	dir = GlobalData::LightDirection();
	const Vector3& forward = GlobalData::Forward();
	const float& dotCamSun = -D3DXVec3Dot(&forward, &dir);
	Params.intensity = (dotCamSun*0.18f);
	Params.decay =0.080f-(dotCamSun/26.8f);
	Params.ddecay = dotCamSun * 0.1f;
	if (dotCamSun <= 0.0f || dir.y > 0.1)
	{
		return;
	}
	//if (dir.y > -0.56f)
	//{
	//	dir.y = -0.56f;
	//}


	EyePos= GlobalData::Position();
	
	SunPos = -1 * Params.dist* dir;
	

	SunPos += EyePos + forward;
	

	GlobalData::GetView(&View);
	GlobalData::GetProj(&Proj);
	
	ViewProjection = View * Proj;

	
    D3DXVec3TransformCoord(&SunPosSS, &SunPos, &ViewProjection);

	
	if (abs(SunPosSS.x) >= Params.MaxSunDist || abs(SunPosSS.y) >= Params.MaxSunDist)
	{
		return;
	}

	 Vector3 sunColor;

	D3DXVec3Lerp(&sunColor, &yellow, &white, GlobalData::factor);
	
	
	sunColor*= Params.intensity;



	PrepareOcclusion(DC,ssaoSRV);
	RayTrace(DC,Vector2(SunPosSS.x, SunPosSS.y), sunColor);
	//RayBlur(DC);
	if (!ShowRayTraceRes)
	Combine(DC,pLightAccumRTV);
}

void SSLR::PrepareOcclusion(ID3D11DeviceContext* DC, ID3D11ShaderResourceView * ssaoSRV)
{
	
	// Constants
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	DC->Map( OcclusionCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	occlusionDesc.Width = width;
	occlusionDesc.Height = height;
	occlusionDesc.occlusionFlag = 0.99f;
	
	memcpy(MappedResource.pData, &occlusionDesc, sizeof(occlusionDesc));
	DC->Unmap( OcclusionCB, 0);
	ID3D11Buffer* arrConstBuffers[1] = {  OcclusionCB };
	DC->CSSetConstantBuffers(0, 1, arrConstBuffers);

	// Output
	ID3D11UnorderedAccessView* arrUAVs[1] = {  OcclusionUAV };
	DC->CSSetUnorderedAccessViews(0, 1, arrUAVs, NULL);

	// Input
	ID3D11ShaderResourceView* arrViews[1] = { ssaoSRV };
	DC->CSSetShaderResources(0, 1, arrViews);

	// Shader
	DC->CSSetShader( OcclusionCS, NULL, 0);

	// Execute the downscales first pass with enough groups to cover the entire full res HDR buffer
	DC->Dispatch(downScaleGroups, 1, 1);

	// Cleanup
	DC->CSSetShader(NULL, NULL, 0);
	ZeroMemory(arrViews, sizeof(arrViews));
	DC->CSSetShaderResources(0, 1, arrViews);
	ZeroMemory(arrUAVs, sizeof(arrUAVs));
	DC->CSSetUnorderedAccessViews(0, 1, arrUAVs, NULL);
	ZeroMemory(arrConstBuffers, sizeof(arrConstBuffers));
	DC->CSSetConstantBuffers(0, 1, arrConstBuffers);
}

void SSLR::RayTrace(ID3D11DeviceContext* DC, const Vector2 & SunPosSS, const Vector3 & SunColor)
{
	

	
	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	DC->ClearRenderTargetView( LightRaysRTV, ClearColor);

	D3D11_VIEWPORT oldvp;
	UINT num = 1;
	DC->RSGetViewports(&num, &oldvp);
	if (!ShowRayTraceRes)
	{
		D3D11_VIEWPORT vp[1] = { { 0, 0, (float)width, (float)height, 0.0f, 1.0f } };
		DC->RSSetViewports(1, vp);

		DC->OMSetRenderTargets(1, & LightRaysRTV, NULL);
	}

	// Constants
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	DC->Map( RayTraceCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	
	lightRayDesc.SunPos = Vector2(0.5f * SunPosSS.x + 0.5f, -0.5f * SunPosSS.y + 0.5f);
	lightRayDesc.InitDecay = Params.decay;
	lightRayDesc.DistDecay = Params.ddecay;
	lightRayDesc.RayColor = SunColor;
	lightRayDesc.MaxDeltaLen = Params.MaxDeltaLen;
	memcpy(MappedResource.pData, &lightRayDesc, sizeof(lightRayDesc));
	DC->Unmap( RayTraceCB, 0);
	ID3D11Buffer* arrConstBuffers[1] = {  RayTraceCB };
	DC->PSSetConstantBuffers(0, 1, arrConstBuffers);

	// Input
	ID3D11ShaderResourceView* arrViews[2] = {  OcclusionSRV ,scatterTexture->SRV()	};
	DC->PSSetShaderResources(0, 2, arrViews);

	// Primitive settings
	DC->IASetInputLayout(NULL);
	DC->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	DC->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Set the shaders
	DC->VSSetShader( FullScreenVS, NULL, 0);
	DC->GSSetShader(NULL, NULL, 0);
	DC->PSSetShader( RayTracePS, NULL, 0);

	DC->Draw(4, 0);

	// Cleanup
	arrViews[0] = NULL;
	DC->PSSetShaderResources(0, 1, arrViews);
	DC->VSSetShader(NULL, NULL, 0);
	DC->PSSetShader(NULL, NULL, 0);
	DC->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DC->RSSetViewports(1, &oldvp);
	
}

void SSLR::Combine(ID3D11DeviceContext* DC, ID3D11RenderTargetView* LightAccumRTV)
{

	
	ID3D11BlendState* PrevBlendState;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	DC->OMGetBlendState(&PrevBlendState, prevBlendFactor, &prevSampleMask);
	DC->OMSetBlendState( AdditiveBlendState, prevBlendFactor, prevSampleMask);

	// Restore the light accumulation view
	DC->OMSetRenderTargets(1, &LightAccumRTV, NULL);

	// Constants
	ID3D11Buffer* arrConstBuffers[1] = {  RayTraceCB };
	DC->PSSetConstantBuffers(0, 1, arrConstBuffers);

	// Input
	ID3D11ShaderResourceView* arrViews[1] = { LightRaysSRV };
	DC->PSSetShaderResources(0, 1, arrViews);

	// Primitive settings
	DC->IASetInputLayout(NULL);
	DC->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	DC->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// Set the shaders
	DC->VSSetShader( FullScreenVS, NULL, 0);
	DC->GSSetShader(NULL, NULL, 0);
	DC->PSSetShader( CombinePS, NULL, 0);

	DC->Draw(4, 0);

	// Cleanup
	arrViews[0] = NULL;
	DC->PSSetShaderResources(0, 1, arrViews);
	DC->VSSetShader(NULL, NULL, 0);
	DC->PSSetShader(NULL, NULL, 0);
	DC->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DC->OMSetBlendState(PrevBlendState, prevBlendFactor, prevSampleMask);
}

void SSLR::Resize(const uint & width, const uint & height)
{

	this->width = width / 2;
	this->height = height / 2;

	downScaleGroups = (UINT)ceil((float)(width * height / 4) / 1024);

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Allocate the occlusion resources
	D3D11_TEXTURE2D_DESC t2dDesc = {
		this->width , //UINT Width;
		this->height , //UINT Height;
		1, //UINT MipLevels;
		1, //UINT ArraySize;
		DXGI_FORMAT_R8_TYPELESS, //DXGI_FORMAT Format;
		1, //DXGI_SAMPLE_DESC SampleDesc;
		0,
		D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
		D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
		0,//UINT CPUAccessFlags;
		0//UINT MiscFlags;    
	};
	Check(device->CreateTexture2D(&t2dDesc, NULL, &OcclusionTex));



	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	ZeroMemory(&UAVDesc, sizeof(UAVDesc));
	UAVDesc.Format = DXGI_FORMAT_R8_UNORM;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	Check(device->CreateUnorderedAccessView(OcclusionTex, &UAVDesc, &OcclusionUAV));


	D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd =
	{
		DXGI_FORMAT_R8_UNORM,
		D3D11_SRV_DIMENSION_TEXTURE2D,
		0,
		0
	};
	dsrvd.Texture2D.MipLevels = 1;
	Check(device->CreateShaderResourceView(OcclusionTex, &dsrvd, &OcclusionSRV));



	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Allocate the light rays resources
	t2dDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	Check(device->CreateTexture2D(&t2dDesc, NULL, &LightRaysTex));


	D3D11_RENDER_TARGET_VIEW_DESC rtsvd =
	{
		DXGI_FORMAT_R8_UNORM,
		D3D11_RTV_DIMENSION_TEXTURE2D
	};
	Check(device->CreateRenderTargetView(LightRaysTex, &rtsvd, &LightRaysRTV));

	Check(device->CreateShaderResourceView(LightRaysTex, &dsrvd, &LightRaysSRV));
}
