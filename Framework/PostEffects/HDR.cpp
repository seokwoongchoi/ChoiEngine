#include "Framework.h"
#include "HDR.h"

//, fMiddleGrey(6.0), fWhite(6.0f)
HDR::HDR(ID3D11Device* device, uint width, uint height)
	:device(device),fianlPassDesc{}, DownScaleDesc{}, DC(NULL), Params{},
	DownScaleFirstPassCS(nullptr), DownScaleSecondPassCS(nullptr),  BloomRevealCS(nullptr),
    HorizontalBlurCS(nullptr),  VerticalBlurCS(nullptr),  FullScreenQuadVS(nullptr),  FinalPassPS(nullptr),
    BloomRT(nullptr),  BloomSRV(nullptr),  BloomUAV(nullptr),
    downScale1DBuffer(nullptr),  downScale1DUAV(nullptr),  downScale1DSRV(nullptr),  DownScaleRT(nullptr),
    DownScaleSRV(nullptr),  DownScaleUAV(nullptr),  avgLumBuffer(nullptr),  avgLumUAV(nullptr),  avgLumSRV(nullptr),
	BlurCB(nullptr)
{// Find the amount of thread groups needed for the downscale operation
	

	for (uint i = 0; i < 2; i++)
	{
		TempRT[i] = nullptr;
		TempSRV[i] = nullptr;
		TempUAV[i] = nullptr;
	}
	
	Params.BloomScale = 1.1f;
	Params.BloomThreshold = 0.0f;
	Params.White = 3.878f;
	Params.MiddleGrey = 2.459f;
	
	Resize(width, height);


	


	/////////////////////////////////////////////////////////////////////////
	//const UINT nMaxBokehInst = 4056;
	//bufferDesc.StructureByteStride = 7 * sizeof(float);
	//bufferDesc.ByteWidth = nMaxBokehInst * bufferDesc.StructureByteStride;
	//
	//Check(device->CreateBuffer(&bufferDesc, NULL, &BokehBuffer));

	//DescUAV.Buffer.FirstElement = 0;
	//DescUAV.Buffer.NumElements = nMaxBokehInst;
	//DescUAV.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND;
	//Check(device->CreateUnorderedAccessView(BokehBuffer, &DescUAV, &BokehUAV));


	//dsrvd.Buffer.NumElements = nMaxBokehInst;
	//Check(device->CreateShaderResourceView(BokehBuffer, &dsrvd, &BokehSRV));
	//ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	//bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
	//bufferDesc.ByteWidth = 16;

	//D3D11_SUBRESOURCE_DATA initData;
	//UINT bufferInit[4] = { 0, 1, 0, 0 };
	//initData.pSysMem = bufferInit;
	//initData.SysMemPitch = 0;
	//initData.SysMemSlicePitch = 0;

	//Check(device->CreateBuffer(&bufferDesc, &initData, &BokehIndirectDrawBuffer));

	//wstring temp = L"../../_Textures/Environment/Bokeh.dds";
	////wstring temp = L"../../_Textures/Environment/sky_ocean.dds";
	//D3DX11CreateShaderResourceViewFromFile
	//(
	//	device, temp.c_str(), NULL, NULL, &BokehTexView, NULL
	//);


	
	
	// Create the two samplers
	//D3D11_SAMPLER_DESC samDesc;
	//ZeroMemory(&samDesc, sizeof(samDesc));
	//
	//samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	//samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	//
	//samDesc.MaxAnisotropy = 1;
	//samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	//samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	//Check(device->CreateSamplerState(&samDesc, &g_pSampPoint));
	//shader->AsSampler("PointSampler")->SetSampler(0, g_pSampPoint);


	/////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Allocate constant buffers
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(TDownScaleCB);
	Check(device->CreateBuffer(&bufferDesc, NULL, &DownScaleCB));
//	DXUT_SetDebugName(DownScaleCB, "PostFX - Down Scale CB");

	bufferDesc.ByteWidth = sizeof(TFinalPassCB);
	Check(device->CreateBuffer(&bufferDesc, NULL, &FinalPassCB));

	bufferDesc.ByteWidth = sizeof(TBlurCB);
	Check(device->CreateBuffer(&bufferDesc, NULL, &BlurCB));
	//DXUT_SetDebugName(FinalPassCB, "PostFX - Final Pass CB");

	

	// Compile the shaders
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;// | D3DCOMPILE_WARNINGS_ARE_ERRORS;

	ID3DBlob* pShaderBlob = NULL;
	ID3DBlob* error;
	
	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/PostDownScaleFX.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "DownScaleFirstPass", "cs_5_0", dwShaderFlags, NULL, &pShaderBlob,&error));
	//Check(D3DCompileFromFile(str.c_str(), NULL, "DownScaleFirstPass", "cs_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreateComputeShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &DownScaleFirstPassCS));
	//DXUT_SetDebugName(DownScaleFirstPassCS, "Post FX - Down Scale First Pass CS");
	SafeRelease(pShaderBlob);

	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/PostDownScaleFX.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "DownScaleSecondPass", "cs_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));
	//V_RETURN(CompileShader(str, NULL, "DownScaleSecondPass", "cs_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreateComputeShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &DownScaleSecondPassCS));
	//DXUT_SetDebugName(DownScaleSecondPassCS, "Post FX - Down Scale Second Pass CS");
	SafeRelease(pShaderBlob);

	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/PostDownScaleFX.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BloomReveal", "cs_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));
	//V_RETURN(CompileShader(str, NULL, "BloomReveal", "cs_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreateComputeShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &BloomRevealCS));
	//DXUT_SetDebugName(BloomRevealCS, "Post FX - Bloom Reveal CS");
	SafeRelease(pShaderBlob);


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// NVidia Gaussian Blur

	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/Blur.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VerticalFilter", "cs_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));
	//V_RETURN(CompileShader(str, NULL, "VerticalFilter", "cs_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreateComputeShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &VerticalBlurCS));
	//DXUT_SetDebugName(m_VerticalBlurCS, "Post FX - Vertical Blur CS");
	SafeRelease(pShaderBlob);
	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/Blur.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "HorizFilter", "cs_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));
	//V_RETURN(CompileShader(str, NULL, "HorizFilter", "cs_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreateComputeShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &HorizontalBlurCS));
//	DXUT_SetDebugName(m_HorizontalBlurCS, "Post FX - Horizontal Blur CS");
	SafeRelease(pShaderBlob);

	//V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"PostFX.hlsl"));
	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/PostEffect.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "FullScreenQuadVS", "vs_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));
	//V_RETURN(CompileShader(str, NULL, "FullScreenQuadVS", "vs_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreateVertexShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &FullScreenQuadVS));
	//DXUT_SetDebugName(FullScreenQuadVS, "Post FX - Full Screen Quad VS");
	SafeRelease(pShaderBlob);
	Check(D3DCompileFromFile(L"../_Shaders/PostEffects/PostEffect.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "FinalPassPS", "ps_5_0", dwShaderFlags, NULL, &pShaderBlob, &error));
	//V_RETURN(CompileShader(str, NULL, "FinalPassPS", "ps_5_0", dwShaderFlags, &pShaderBlob));
	Check(device->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &FinalPassPS));
	//DXUT_SetDebugName(FinalPassPS, "Post FX - Final Pass PS");
	SafeRelease(pShaderBlob);

	
		

	// Create the two samplers
	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &SampLinear));

	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	Check(device ->CreateSamplerState(&samDesc, &SampPoint));
}

HDR::~HDR()
{
	SafeRelease(downScale1DBuffer);
	SafeRelease(downScale1DUAV);
	SafeRelease(downScale1DSRV);
	//SafeRelease(downScaleCB);
	//SafeRelease(finalPassCB);
	SafeRelease(avgLumBuffer);
	SafeRelease(avgLumUAV);
	SafeRelease(avgLumSRV);
}

void HDR::PostProcessing(ID3D11DeviceContext* DC,ID3D11ShaderResourceView * pHDRSRV, ID3D11RenderTargetView * oldTarget, ID3D11ShaderResourceView* dsv)
{
	{
		DownScaleDesc.Width = width;
		DownScaleDesc.Height = height;
		DownScaleDesc.TotalPixels = DownScaleDesc.Width * DownScaleDesc.Height;
		DownScaleDesc.GroupSize = downScaleGroups;
		DownScaleDesc.BloomThreshold = Params.BloomThreshold;

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		DC->Map(DownScaleCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &DownScaleDesc, sizeof(DownScaleDesc));
		DC->Unmap(DownScaleCB, 0);
	}
	
	ID3D11Buffer* arrConstBuffers[1] = { DownScaleCB };
	DC->CSSetConstantBuffers(0, 1, arrConstBuffers);

	ID3D11RenderTargetView* rt[1] = { NULL };
	DC->OMSetRenderTargets(1, rt, nullptr);
	
	DownScale(DC,pHDRSRV);
	//BokehHightlightScan(pHDRSRV, dsv);
	Bloom(DC);
	Blur(DC,TempSRV[0], BloomUAV);
	
	// Cleanup
	ZeroMemory(&arrConstBuffers, sizeof(arrConstBuffers));
	DC->CSSetConstantBuffers(0, 1, arrConstBuffers);
	
	rt[0] = oldTarget;
	DC->OMSetRenderTargets(1, rt, nullptr);
	FinalPass(DC,pHDRSRV,dsv);
	//ID3D11Buffer* pTempBuffer = avgLumBuffer;
	/*ID3D11UnorderedAccessView* pTempUAV = avgLumUAV;
	ID3D11ShaderResourceView* p_TempSRV = avgLumSRV;
	avgLumBuffer = PrevAvgLumBuffer;
	avgLumUAV = PrevAvgLumUAV;
	avgLumSRV = PrevAvgLumSRV;
	PrevAvgLumBuffer = pTempBuffer;
	PrevAvgLumUAV = pTempUAV;
	PrevAvgLumSRV = p_TempSRV;*/
	//BokehRender();
}

void HDR::DownScale(ID3D11DeviceContext* DC, ID3D11ShaderResourceView * srv)
{
	
	ID3D11UnorderedAccessView* arrUAVs[2] = { downScale1DUAV, DownScaleUAV };
	DC->CSSetUnorderedAccessViews(0, 2, arrUAVs, NULL);
	// Input
	ID3D11ShaderResourceView* arrViews[2] = { srv, NULL };
	DC->CSSetShaderResources(0, 1, arrViews);
	
	DC->CSSetShader(DownScaleFirstPassCS, NULL, 0);
	
	DC->Dispatch(downScaleGroups, 1, 1);


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Second pass - reduce to a single pixel

	// Outoput
	ZeroMemory(arrUAVs, sizeof(arrUAVs));
	arrUAVs[0] = avgLumUAV;
	DC->CSSetUnorderedAccessViews(0, 2, arrUAVs, NULL);

	// Input
	arrViews[0] = downScale1DSRV;
	arrViews[1] = avgLumSRV;
	DC->CSSetShaderResources(0, 2, arrViews);

	// Shader
	DC->CSSetShader(DownScaleSecondPassCS, NULL, 0);

	// Excute with a single group - this group has enough threads to process all the pixels
	DC->Dispatch(1, 1, 1);

	// Cleanup
	DC->CSSetShader(NULL, NULL, 0);
	ZeroMemory(arrViews, sizeof(arrViews));
	DC->CSSetShaderResources(0, 2, arrViews);
	ZeroMemory(arrUAVs, sizeof(arrUAVs));
	DC->CSSetUnorderedAccessViews(0, 2, arrUAVs, NULL);
}



void HDR::Bloom(ID3D11DeviceContext* DC)
{

	// Input
	ID3D11ShaderResourceView* arrViews[2] = { DownScaleSRV, avgLumSRV };
	DC->CSSetShaderResources(0, 2, arrViews);

	// Output
	ID3D11UnorderedAccessView* arrUAVs[1] = { TempUAV[0] };
	DC->CSSetUnorderedAccessViews(0, 1, arrUAVs, NULL);

	// Shader
	DC->CSSetShader(BloomRevealCS, NULL, 0);

	// Execute the downscales first pass with enough groups to cover the entire full res HDR buffer
	DC->Dispatch(downScaleGroups, 1, 1);

	// Cleanup
	DC->CSSetShader(NULL, NULL, 0);
	ZeroMemory(arrViews, sizeof(arrViews));
	DC->CSSetShaderResources(0, 2, arrViews);
	ZeroMemory(arrUAVs, sizeof(arrUAVs));
	DC->CSSetUnorderedAccessViews(0, 1, arrUAVs, NULL);

	
	
	//sfirstDownScaleSRV->SetResource(DownScaleSRV);//0
	//ffirstAvgLumEffect->SetResource(avgLumSRV);//1

	//fBloomEffect->SetUnorderedAccessView(TempUAV[0]);

	//downScaleShader->Dispatch(0, 3, downScaleGroups, 1, 1); 
	////Blur(TempSRV[0], BloomUAV);
}


void HDR::BokehHightlightScan(ID3D11ShaderResourceView * pHDRSRV, ID3D11ShaderResourceView * pDepthSRV)
{
	//bokehUAVEffect->SetUnorderedAccessView(BokehUAV);
	//bokehHDRTextureEffect->SetResource(pHDRSRV);
	//bokehDepthTextureEffect->SetResource(pDepthSRV);
	//bokehAvgLumEffect->SetResource(avgLumSRV);

	//BokehHightlightScanCB.nWidth = width;
	//BokehHightlightScanCB.nHeight = height;

	//proj= Context::Get()->Projection();
	//
	//BokehHightlightScanCB.ProjectionValues[0] = proj.m[3][2];
	//BokehHightlightScanCB.ProjectionValues[1] = -proj.m[2][2];

	//
	//BokehHightlightScanCB.fDOFFarStart = DOFFarValues1;
	//BokehHightlightScanCB.fDOFFarRangeRcp = 1.0f/DOFFarValues2;
	//BokehHightlightScanCB.fMiddleGrey = fMiddleGrey;
	//BokehHightlightScanCB.fLumWhiteSqr = fWhite;
	//BokehHightlightScanCB.fLumWhiteSqr *= BokehHightlightScanCB.fMiddleGrey; // Scale by the middle gray value
	//BokehHightlightScanCB.fLumWhiteSqr *= BokehHightlightScanCB.fLumWhiteSqr; // Square
	//
	//ImGui::SliderFloat("m_fBokehBlurThreshold", (float*)&m_fBokehBlurThreshold, 0.f, 10.0f);
	//BokehHightlightScanCB.fBokehBlurThreshold = m_fBokehBlurThreshold;
	//ImGui::SliderFloat("m_fBokehLumThreshold", (float*)&m_fBokehLumThreshold, 0.f, 10.0f);
	//BokehHightlightScanCB.fBokehLumThreshold = m_fBokehLumThreshold;
	//ImGui::SliderFloat("m_fBokehRadiusScale", (float*)&m_fBokehRadiusScale, 0.f, 10.0f);
	//BokehHightlightScanCB.fRadiusScale = m_fBokehRadiusScale;
	//ImGui::SliderFloat("m_fBokehColorScale", (float*)&m_fBokehColorScale, 0.f, 10.0f);
	//BokehHightlightScanCB.fColorScale = m_fBokehColorScale;
	//

	//bokehCSBuffer->Apply();
	//sBokehCSBuffer->SetConstantBuffer(bokehCSBuffer->Buffer());

	//bokehCSShader->Dispatch(0,0,(UINT)ceil((float)(width * height) /1024.0f), 1, 1);
	//bokehCSShader->Dispatch(0, 0, 1280, 1, 1);
}

void HDR::Blur(ID3D11DeviceContext* DC, ID3D11ShaderResourceView * pInput, ID3D11UnorderedAccessView * pOutput)
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Second pass - horizontal Gaussian filter

	{
		blurDesc.Width = width;
		blurDesc.Height = height;
	
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		DC->Map(BlurCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &blurDesc, sizeof(blurDesc));
		DC->Unmap(BlurCB, 0);
	}

	ID3D11Buffer* arrConstBuffers[1] = { BlurCB };
	DC->CSSetConstantBuffers(0, 1, arrConstBuffers);
	// Output
	ID3D11UnorderedAccessView* arrUAVs[1] = { TempUAV[1] };
	DC->CSSetUnorderedAccessViews(0, 1, arrUAVs, NULL);

	// Input
	ID3D11ShaderResourceView* arrViews[1] = { pInput };
	DC->CSSetShaderResources(0, 1, arrViews);

	// Shader
	DC->CSSetShader(HorizontalBlurCS, NULL, 0);

	// Execute the horizontal filter
	DC->Dispatch(static_cast<uint>(ceil((width) / (128.0f - 12.0f))), static_cast<uint>(ceil(height)), 1);

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// First pass - vertical Gaussian filter

	// Output
	arrUAVs[0] = pOutput;
	DC->CSSetUnorderedAccessViews(0, 1, arrUAVs, NULL);

	// Input
	arrViews[0] = TempSRV[1];
	DC->CSSetShaderResources(0, 1, arrViews);

	// Shader
	DC->CSSetShader(VerticalBlurCS, NULL, 0);

	// Execute the vertical filter
	DC->Dispatch(static_cast<uint>(ceil(width )), static_cast<uint>(ceil((height) / (128.0f - 12.0f))), 1);

	// Cleanup
	ZeroMemory(&arrConstBuffers, sizeof(arrConstBuffers));
	DC->CSSetConstantBuffers(0, 1, arrConstBuffers);
	DC->CSSetShader(NULL, NULL, 0);
	ZeroMemory(arrViews, sizeof(arrViews));
	DC->CSSetShaderResources(0, 1, arrViews);
	ZeroMemory(arrUAVs, sizeof(arrUAVs));
	DC->CSSetUnorderedAccessViews(0, 1, arrUAVs, NULL);

	
}


void HDR::FinalPass(ID3D11DeviceContext* DC, ID3D11ShaderResourceView* srv, ID3D11ShaderResourceView* dsv)
{
	
	ID3D11ShaderResourceView* arrViews[3] = { srv, avgLumSRV, BloomSRV };
	DC->PSSetShaderResources(0, 3, arrViews);
		
	{
		fianlPassDesc.MiddleGrey = Params.MiddleGrey;
		fianlPassDesc.LumWhiteSqr = Params.White;
		fianlPassDesc.LumWhiteSqr *= fianlPassDesc.MiddleGrey; // Scale by the middle grey value
		fianlPassDesc.LumWhiteSqr *= fianlPassDesc.LumWhiteSqr; // Square
		fianlPassDesc.BloomScale = Params.BloomScale;

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		DC->Map(FinalPassCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &fianlPassDesc, sizeof(fianlPassDesc));
		DC->Unmap(FinalPassCB, 0);
	
		DC->PSSetConstantBuffers(0, 1, &FinalPassCB);
	}
	
		DC->IASetInputLayout(NULL);
	DC->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	DC->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);
	DC->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	ID3D11SamplerState* arrSamplers[2] = { SampPoint, SampLinear };
	DC->PSSetSamplers(0, 2, arrSamplers);

	// Set the shaders
	DC->VSSetShader(FullScreenQuadVS, NULL, 0);
	DC->PSSetShader(FinalPassPS, NULL, 0);

	DC->Draw(4, 0);

	// Cleanup
	ZeroMemory(arrViews, sizeof(arrViews));
	DC->PSSetShaderResources(0, 3, arrViews);
	ID3D11Buffer* nullBuffer = nullptr;
	DC->PSSetConstantBuffers(0, 1, &nullBuffer);
	DC->VSSetShader(NULL, NULL, 0);
	DC->PSSetShader(NULL, NULL, 0);
}

void HDR::BokehRender()
{
	
	/*DC->CopyStructureCount(BokehIndirectDrawBuffer, 0, BokehUAV);
	

	BokehRenderCB.AspectRatio[0] = 1.0f;
	BokehRenderCB.AspectRatio[1] = (float)width / (float)height;



	bokehSRVEffect->SetResource(BokehSRV);

	bokehRenderBuffer->Apply();
	sBokehRenderBuffer->SetConstantBuffer(bokehRenderBuffer->Buffer());


	DC->IASetInputLayout(NULL);
	DC->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	DC->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
*/
	

	//bokehShader->DrawInstancedIndirect(0,0, BokehIndirectDrawBuffer,0);
	
}

void HDR::Resize(const uint & width, const uint & height)
{

	this->width = width/4;
	this->height = height/4;
	downScaleGroups = (UINT)ceil((float)(this->width *this->height) /( 1024));

	//downScaleGroups = 1280;


	SafeRelease(DownScaleRT);
	SafeRelease(DownScaleSRV);
	SafeRelease(DownScaleUAV);


	for (uint i = 0; i < 2; i++)
	{
		SafeRelease(TempRT[i]);
		SafeRelease(TempSRV[i]);
		SafeRelease(TempUAV[i]);
	}

	SafeRelease(BloomRT);
	SafeRelease(BloomSRV);
	SafeRelease(BloomUAV);

	SafeRelease(avgLumBuffer);
	SafeRelease(avgLumUAV);
	SafeRelease(avgLumSRV);

	SafeRelease(downScale1DBuffer);
	SafeRelease(downScale1DUAV);
	SafeRelease(downScale1DSRV);


	// Allocate the downscaled target
	D3D11_TEXTURE2D_DESC dtd = {
		this->width , //UINT Width;
		this->height, //UINT Height;
		1, //UINT MipLevels;
		1, //UINT ArraySize;
		DXGI_FORMAT_R32G32B32A32_TYPELESS, //DXGI_FORMAT Format;
		1, //DXGI_SAMPLE_DESC SampleDesc;
		0,
		D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,//UINT BindFlags;
		0,//UINT CPUAccessFlags;
		0//UINT MiscFlags;    
	};
	Check(device->CreateTexture2D(&dtd, NULL, &DownScaleRT));


	// Create the resource views
	D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd;
	ZeroMemory(&dsrvd, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	dsrvd.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	dsrvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	dsrvd.Texture2D.MipLevels = 1;
	Check(device->CreateShaderResourceView(DownScaleRT, &dsrvd, &DownScaleSRV));


	// Create the UAVs
	D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
	ZeroMemory(&DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	DescUAV.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	DescUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	DescUAV.Buffer.FirstElement = 0;
	DescUAV.Buffer.NumElements = this->width * this->height;
	Check(device->CreateUnorderedAccessView(DownScaleRT, &DescUAV, &DownScaleUAV));
	Check(device->CreateTexture2D(&dtd, NULL, &TempRT[0]));
	Check(device->CreateShaderResourceView(TempRT[0], &dsrvd, &TempSRV[0]));
	Check(device->CreateUnorderedAccessView(TempRT[0], &DescUAV, &TempUAV[0]));
	Check(device->CreateTexture2D(&dtd, NULL, &TempRT[1]));
	Check(device->CreateShaderResourceView(TempRT[1], &dsrvd, &TempSRV[1]));
	Check(device->CreateUnorderedAccessView(TempRT[1], &DescUAV, &TempUAV[1]));


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Allocate bloom target
	Check(device->CreateTexture2D(&dtd, NULL, &BloomRT));
	Check(device->CreateShaderResourceView(BloomRT, &dsrvd, &BloomSRV));
	Check(device->CreateUnorderedAccessView(BloomRT, &DescUAV, &BloomUAV));

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Allocate down scaled luminance buffer
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.StructureByteStride = sizeof(float);
	bufferDesc.ByteWidth = downScaleGroups * bufferDesc.StructureByteStride;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	Check(device->CreateBuffer(&bufferDesc, NULL, &downScale1DBuffer));


	//D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
	ZeroMemory(&DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	DescUAV.Format = DXGI_FORMAT_UNKNOWN;
	DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	DescUAV.Buffer.NumElements = downScaleGroups;
	Check(device->CreateUnorderedAccessView(downScale1DBuffer, &DescUAV, &downScale1DUAV));
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Allocate average luminance buffer
	//D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd;
	ZeroMemory(&dsrvd, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	dsrvd.Format = DXGI_FORMAT_UNKNOWN;
	dsrvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	dsrvd.Buffer.NumElements = downScaleGroups;
	Check(device->CreateShaderResourceView(downScale1DBuffer, &dsrvd, &downScale1DSRV));

	bufferDesc.ByteWidth = sizeof(float);
	Check(device->CreateBuffer(&bufferDesc, NULL, &avgLumBuffer));
	//Check(device->CreateBuffer(&bufferDesc, NULL, &PrevAvgLumBuffer));

	DescUAV.Buffer.NumElements = 1;
	Check(device->CreateUnorderedAccessView(avgLumBuffer, &DescUAV, &avgLumUAV));
	//Check(device->CreateUnorderedAccessView(avgLumBuffer, &DescUAV, &PrevAvgLumUAV));

	dsrvd.Buffer.NumElements = 1;
	Check(device->CreateShaderResourceView(avgLumBuffer, &dsrvd, &avgLumSRV));
	//Check(device->CreateShaderResourceView(PrevAvgLumBuffer, &dsrvd, &PrevAvgLumSRV));
}




