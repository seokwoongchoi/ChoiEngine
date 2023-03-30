#include "Framework.h"
#include "SSAO.h"

SSAO::SSAO(ID3D11Device* device, uint width, uint height)
	:device(device),DownscaleCB(NULL),
	SSAO_RT(NULL), SSAO_SRV(NULL), SSAO_UAV(NULL),
	MiniDepthBuffer(NULL), MiniDepthUAV(NULL), MiniDepthSRV(NULL),
	DepthDownscaleCS(NULL), ComputeCS(NULL), DownscaleDesc{}, Params{}
{
	Params.Radius = 0.2f;
	Params.SSAOSampRadius = 0.9f;

	this->width = width / 2;
	this->height = height / 2;

	downScaleGroups = (UINT)ceil((float)(this->width * this->height) / 1024);
	
	
	CreateSSAOTexture();

	


	// Allocate down scale depth constant buffer
	D3D11_BUFFER_DESC CBDesc;
	ZeroMemory(&CBDesc, sizeof(CBDesc));
	CBDesc.Usage = D3D11_USAGE_DYNAMIC;
	CBDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	CBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	CBDesc.ByteWidth = sizeof(DownscaleDesc);
	Check(device->CreateBuffer(&CBDesc, NULL, &DownscaleCB));
	//DXUT_SetDebugName(DownscaleCB, "SSAO - Downscale Depth CB");

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Compile the shaders
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;// | D3DCOMPILE_WARNINGS_ARE_ERRORS;


	ID3DBlob* error;
	ID3DBlob* pShaderBlob = NULL;
	//V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"SSAO.hlsl"));
	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/SSAO.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "DepthDownscale", "cs_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));
	//V_RETURN(CompileShader(str, NULL, "DepthDownscale", "cs_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), NULL, &DepthDownscaleCS));
	SafeRelease(pShaderBlob);
	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/SSAO.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "SSAOCompute", "cs_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));
	//V_RETURN(CompileShader(str, NULL, "SSAOCompute", "cs_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreateComputeShader(pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize(), NULL, &ComputeCS));
	SafeRelease(pShaderBlob);

	
}

SSAO::~SSAO()
{
	

	SafeRelease(DownscaleCB);
	SafeRelease(SSAO_RT);
	SafeRelease(SSAO_SRV);
	SafeRelease(SSAO_UAV);
	SafeRelease(MiniDepthBuffer);
	SafeRelease(MiniDepthUAV);
	SafeRelease(MiniDepthSRV);
	SafeRelease(DepthDownscaleCS);
	SafeRelease(ComputeCS);

	
}

void SSAO::Compute(ID3D11DeviceContext* DC, ID3D11ShaderResourceView * DepthSRV, ID3D11ShaderResourceView * NormalsSRV)
{
	DownscaleDepth(DC,DepthSRV, NormalsSRV);
	ComputeSSAO(DC);
}

void SSAO::Resize(const uint &width,const uint& height)
{
	this->width = width / 2;
	this->height = height / 2;

	downScaleGroups = (UINT)ceil((float)(this->width * this->height) / 1024.0f);


	CreateSSAOTexture();
}

void SSAO::DownscaleDepth(ID3D11DeviceContext* DC,ID3D11ShaderResourceView * DepthSRV, ID3D11ShaderResourceView * NormalsSRV)
{

	DownscaleDesc.nWidth = width;
	DownscaleDesc.nHeight = height;
	DownscaleDesc.fHorResRcp = 1.0f / static_cast<float>(DownscaleDesc.nWidth);
	DownscaleDesc.fVerResRcp = 1.0f / static_cast<float>(DownscaleDesc.nHeight);
	 GlobalData::GetProj(&proj);
	DownscaleDesc.ProjParams.x = 1.0f / proj.m[0][0];
	DownscaleDesc.ProjParams.y = 1.0f / proj.m[1][1];
	const float& fQ = 1000.0f / (1000.0f - 0.1f);
	DownscaleDesc.ProjParams.z = -0.1f * fQ;
	DownscaleDesc.ProjParams.w = -fQ;
	GlobalData::GetView(&view);
	D3DXMatrixTranspose(&DownscaleDesc.ViewMatrix, &view);
	//pDownscale->ViewMatrix= GlobalData::GetView();
	//ImGui::SliderFloat("SSAOSampRadius", (float*)&SSAOSampRadius, 0.0f, 20.0f);
	DownscaleDesc.fOffsetRadius = Params.SSAOSampRadius;
	//ImGui::SliderFloat("Radius", (float*)&Radius, 0.0f, 20.0f);
	DownscaleDesc.fRadius = Params.Radius;
	DownscaleDesc.fMaxDepth = 1000.0f;
	// Constants
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	DC->Map(DownscaleCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &DownscaleDesc, sizeof(DownscaleDesc));
	DC->Unmap(DownscaleCB, 0);
	ID3D11Buffer* arrConstBuffers[1] = { DownscaleCB };
	DC->CSSetConstantBuffers(0, 1, arrConstBuffers);

	// Output
	ID3D11UnorderedAccessView* arrUAVs[1] = { MiniDepthUAV };
	DC->CSSetUnorderedAccessViews(0, 1, arrUAVs, NULL);

	// Input
	ID3D11ShaderResourceView* arrViews[2] = { DepthSRV, NormalsSRV };
	DC->CSSetShaderResources(0, 2, arrViews);

	// Shader
	DC->CSSetShader(DepthDownscaleCS, NULL, 0);

	// Execute the downscales first pass with enough groups to cover the entire full res HDR buffer
	DC->Dispatch(downScaleGroups, 1, 1);

	// Cleanup
	DC->CSSetShader(NULL, NULL, 0);
	ZeroMemory(arrViews, sizeof(arrViews));
	DC->CSSetShaderResources(0, 2, arrViews);
	ZeroMemory(arrUAVs, sizeof(arrUAVs));
	DC->CSSetUnorderedAccessViews(0, 1, arrUAVs, NULL);
	ZeroMemory(arrConstBuffers, sizeof(arrConstBuffers));
	DC->CSSetConstantBuffers(0, 1, arrConstBuffers);
}

void SSAO::ComputeSSAO(ID3D11DeviceContext* DC)
{
	
	ID3D11Buffer* arrConstBuffers[1] = { DownscaleCB };
	DC->CSSetConstantBuffers(0, 1, arrConstBuffers);

	// Output
	ID3D11UnorderedAccessView* arrUAVs[1] = { SSAO_UAV };
	DC->CSSetUnorderedAccessViews(0, 1, arrUAVs, NULL);

	// Input
	ID3D11ShaderResourceView* arrViews[1] = { MiniDepthSRV };
	DC->CSSetShaderResources(0, 1, arrViews);

	// Shader
	DC->CSSetShader(ComputeCS, NULL, 0);

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

void SSAO::CreateSSAOTexture()
{

	SafeRelease(SSAO_RT);
	SafeRelease(SSAO_UAV);
	SafeRelease(SSAO_SRV);
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Allocate SSAO
	D3D11_TEXTURE2D_DESC t2dDesc = {
		width, //UINT Width;
		height, //UINT Height;
		1, //UINT MipLevels;
		1, //UINT ArraySize;
		DXGI_FORMAT_R32_TYPELESS,//DXGI_FORMAT_R8_TYPELESS, //DXGI_FORMAT Format;
		1, //DXGI_SAMPLE_DESC SampleDesc;
		0,
		D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
		D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
		0,//UINT CPUAccessFlags;
		0//UINT MiscFlags;    
	};

	Check(device->CreateTexture2D(&t2dDesc, NULL, &SSAO_RT));


	// Create the UAVs
	D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
	ZeroMemory(&UAVDesc, sizeof(UAVDesc));
	UAVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;


	Check(device->CreateUnorderedAccessView(SSAO_RT, &UAVDesc, &SSAO_UAV));


	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory(&SRVDesc, sizeof(SRVDesc));
	SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;
	Check(device->CreateShaderResourceView(SSAO_RT, &SRVDesc, &SSAO_SRV));



	//////////////////////////////////////////////////////////////////////////////////////////////////////

	SafeRelease(MiniDepthBuffer);
	SafeRelease(MiniDepthUAV);
	SafeRelease(MiniDepthSRV);
	// Allocate down scaled depth buffer
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.StructureByteStride = 4 * sizeof(float);
	bufferDesc.ByteWidth = this->width * this->height * bufferDesc.StructureByteStride;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Check(device->CreateBuffer(&bufferDesc, NULL, &MiniDepthBuffer));


	ZeroMemory(&UAVDesc, sizeof(UAVDesc));
	UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	UAVDesc.Buffer.FirstElement = 0;
	UAVDesc.Buffer.NumElements = this->width * this->height;
	Check(device->CreateUnorderedAccessView(MiniDepthBuffer, &UAVDesc, &MiniDepthUAV));

	
	ZeroMemory(&SRVDesc, sizeof(SRVDesc));
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.NumElements = this->width * this->height;
	Check(device->CreateShaderResourceView(MiniDepthBuffer, &SRVDesc, &MiniDepthSRV));

	//////////////////////////////////////////////////////////////////////////////////////////////////////
}
