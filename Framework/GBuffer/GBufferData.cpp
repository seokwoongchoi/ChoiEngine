#include "Framework.h"
#include "GBufferData.h"

void GBufferData::Init(ID3D11Device* device, uint width, uint height)
	
{
	this->device = device;
	this->width = width;
	this->height = height;

	packingVP.Width = static_cast<float>(width);
	packingVP.Height = static_cast<float>(height);
	packingVP.MaxDepth = 1.0f;
	packingVP.MinDepth = 0.0f;
	packingVP.TopLeftX = 0.0f;
	packingVP.TopLeftY = 0.0f;

	

	CreateViews();

	// Allocate constant buffers
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(CB_GBufferUnpack);
	Check(device->CreateBuffer(&bufferDesc, NULL, &unpackBuffer));
}

void GBufferData::Destroy()
{
	// Clear all allocated targets
	SafeRelease(depthStencilTexture);
	SafeRelease(diffuseTexture);
	SafeRelease(normalTexture);
	SafeRelease(specularTexture);

	// Clear all views
	SafeRelease(depthStencilDSV);
	SafeRelease(depthStencilReadOnlyDSV);
	SafeRelease(diffuseRTV);
	SafeRelease(normalRTV);
	SafeRelease(specularRTV);
	SafeRelease(depthStencilSRV);
	SafeRelease(diffuseSRV);
	SafeRelease(normalSRV);
	SafeRelease(specularSRV);
	

	// Clear the depth stencil state
	SafeRelease(depthStencilState);
}

void GBufferData::Update()
{
	 GlobalData::GetProj(&proj);

	//D3DXMatrixTranspose(&proj, &proj);
	pGBufferUnpackCB.PerspectiveValuse.x = 1 / proj._11;
	pGBufferUnpackCB.PerspectiveValuse.y = 1 / proj._22;
	pGBufferUnpackCB.PerspectiveValuse.z = proj._43;
	pGBufferUnpackCB.PerspectiveValuse.w = -proj._33;

	GlobalData::GetView(&view);
	D3DXMatrixInverse(&pGBufferUnpackCB.ViewInv, nullptr, &view);
	D3DXMatrixTranspose(&pGBufferUnpackCB.ViewInv, &pGBufferUnpackCB.ViewInv);
}

void GBufferData::Resize(uint width, uint height)
{
	this->width = width;
	this->height = height;
	packingVP.Width =static_cast<float>( width);
	packingVP.Height = static_cast<float>(height);

	CreateViews();
}


void GBufferData::PrepareForPacking(ID3D11DeviceContext* context)
{
	uint num = 1;
	context->RSSetViewports(num, &packingVP);
	
	ID3D11RenderTargetView* rt[3] = { diffuseRTV,  specularRTV,	normalRTV };
	context->OMSetRenderTargets(3, rt, depthStencilDSV);
	context->ClearDepthStencilView(depthStencilDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	context->ClearRenderTargetView(diffuseRTV, ClearColor);
	context->ClearRenderTargetView(specularRTV, ClearColor);
	context->ClearRenderTargetView(normalRTV, ClearColor);



	UINT8 SceneStencilFlag = 2;
	context->OMSetDepthStencilState(depthStencilState, SceneStencilFlag);
}

void GBufferData::PrepareForWaterPacking(ID3D11DeviceContext * context)
{
	uint num = 1;
	context->RSSetViewports(num, &packingVP);

	ID3D11RenderTargetView* rt[3] = { diffuseRTV,  specularRTV,	normalRTV };
	context->OMSetRenderTargets(3, rt, depthStencilDSV);
	
	UINT8 SceneStencilFlag = 2;
	context->OMSetDepthStencilState(depthStencilState, SceneStencilFlag);
}

void GBufferData::PrepareForUnpack(ID3D11DeviceContext* context)
{
	// Fill the GBuffer unpack constant buffer

	


	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(unpackBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &pGBufferUnpackCB, sizeof(CB_GBufferUnpack));
	context->Unmap(unpackBuffer, 0);
	context->PSSetConstantBuffers(0, 1, &unpackBuffer);
}

void GBufferData::PostRender(ID3D11DeviceContext* context)
{
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// Little cleanup
	ID3D11RenderTargetView* rt[3] = { NULL, NULL, NULL };
	context->OMSetRenderTargets(3, rt, depthStencilReadOnlyDSV);
}



void GBufferData::CreateViews()
{
	// Clear all allocated targets
	SafeRelease(depthStencilTexture);
	SafeRelease(diffuseTexture);
	SafeRelease(normalTexture);
	SafeRelease(specularTexture);

	// Clear all views
	SafeRelease(depthStencilDSV);
	SafeRelease(depthStencilReadOnlyDSV);
	SafeRelease(diffuseRTV);
	SafeRelease(normalRTV);
	SafeRelease(specularRTV);
	SafeRelease(depthStencilSRV);
	SafeRelease(diffuseSRV);
	SafeRelease(normalSRV);
	SafeRelease(specularSRV);

	// Clear the depth stencil state
	SafeRelease(depthStencilState);


	// Texture formats
	static const DXGI_FORMAT depthStencilTextureFormat = DXGI_FORMAT_R24G8_TYPELESS;
	static const DXGI_FORMAT basicColorTextureFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	static const DXGI_FORMAT normalTextureFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	static const DXGI_FORMAT specPowTextureFormat = DXGI_FORMAT_R16G16_UNORM;

	// Render view formats
	static const DXGI_FORMAT depthStencilRenderViewFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	static const DXGI_FORMAT basicColorRenderViewFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	static const DXGI_FORMAT normalRenderViewFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	static const DXGI_FORMAT specPowRenderViewFormat = DXGI_FORMAT_R16G16_UNORM;

	// Resource view formats
	static const DXGI_FORMAT depthStencilResourceViewFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	static const DXGI_FORMAT basicColorResourceViewFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	static const DXGI_FORMAT normalResourceViewFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
	static const DXGI_FORMAT specPowResourceViewFormat = DXGI_FORMAT_R16G16_UNORM;

	// Allocate the depth stencil target
	D3D11_TEXTURE2D_DESC dtd = {
		width, //UINT Width;
		height, //UINT Height;
		1, //UINT MipLevels;
		1, //UINT ArraySize;
		DXGI_FORMAT_UNKNOWN, //DXGI_FORMAT Format;
		1, //DXGI_SAMPLE_DESC SampleDesc;
		0,
		D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
		0,//UINT CPUAccessFlags;
		0//UINT MiscFlags;    
	};
	dtd.Format = depthStencilTextureFormat;
	Check(device->CreateTexture2D(&dtd, NULL, &depthStencilTexture));
	
	// Allocate the base color with specular intensity target
	dtd.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	


	// Allocate the base color with specular intensity target
	dtd.Format = normalTextureFormat;
	Check(device->CreateTexture2D(&dtd, NULL, &normalTexture));
	
	// Allocate the specular power target
	dtd.Format = specPowTextureFormat;
	Check(device->CreateTexture2D(&dtd, NULL, &specularTexture));

	dtd.Format = basicColorTextureFormat;
	dtd.SampleDesc.Count = 1;
	dtd.SampleDesc.Quality = 0;
	Check(device->CreateTexture2D(&dtd, NULL, &diffuseTexture));

	
	// Create the render target views
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd =
	{
		depthStencilRenderViewFormat,
		D3D11_DSV_DIMENSION_TEXTURE2D,
		0
	};
	Check(device->CreateDepthStencilView(depthStencilTexture, &dsvd, &depthStencilDSV));


	dsvd.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
	Check(device->CreateDepthStencilView(depthStencilTexture, &dsvd, &depthStencilReadOnlyDSV));

	D3D11_RENDER_TARGET_VIEW_DESC rtsvd =
	{
		basicColorRenderViewFormat,
		D3D11_RTV_DIMENSION_TEXTURE2D
	};
	Check(device->CreateRenderTargetView(diffuseTexture, &rtsvd, &diffuseRTV));
	

	rtsvd.Format = normalRenderViewFormat;
	Check(device->CreateRenderTargetView(normalTexture, &rtsvd, &normalRTV));
	

	rtsvd.Format = specPowRenderViewFormat;
	Check(device->CreateRenderTargetView(specularTexture, &rtsvd, &specularRTV));
	

	// Create the resource views
	D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd =
	{
		depthStencilResourceViewFormat,
		D3D11_SRV_DIMENSION_TEXTURE2DMS,
		0,
		0
	};
	dsrvd.Texture2D.MipLevels = 1;
	Check(device->CreateShaderResourceView(depthStencilTexture, &dsrvd, &depthStencilSRV));
	dsrvd.Format = basicColorResourceViewFormat;
	Check(device->CreateShaderResourceView(diffuseTexture, &dsrvd, &diffuseSRV));
		

	dsrvd.Format = normalResourceViewFormat;
	Check(device->CreateShaderResourceView(normalTexture, &dsrvd, &normalSRV));
	
	dsrvd.Format = specPowResourceViewFormat;
	Check(device->CreateShaderResourceView(specularTexture, &dsrvd, &specularSRV));
	



	D3D11_DEPTH_STENCIL_DESC descDepth;
	descDepth.DepthEnable = TRUE;
	descDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	descDepth.DepthFunc = D3D11_COMPARISON_LESS;
	descDepth.StencilEnable = TRUE;
	descDepth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	descDepth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	const D3D11_DEPTH_STENCILOP_DESC stencilMarkOp = { D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_REPLACE, D3D11_COMPARISON_ALWAYS };
	descDepth.FrontFace = stencilMarkOp;
	descDepth.BackFace = stencilMarkOp;

	
	Check(device->CreateDepthStencilState(&descDepth, &depthStencilState));
	

	
}
