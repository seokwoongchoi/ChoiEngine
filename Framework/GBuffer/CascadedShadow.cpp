#include "Framework.h"
#include "CascadedShadow.h"
#include "Core/D3D11/D3D11_Helper.h"
CascadedShadow::CascadedShadow()
	: arrCascadeRanges{ 0,0,0,0 }, CascadeTotalRange(100.0f), ShadowMapSize(Vector2(1280.0f,720.0f)), ShadowBoundCenter(Vector3(0,0,0)), ShadowBoundRadius(0),	  arrCascadeBoundCenter{ Vector3 (0,0,0), Vector3(0,0,0) , Vector3(0,0,0) },
	arrCascadeBoundRadius{ 0,0,0 },	
	camPos(Vector3(0, 0, 0)),camForward(Vector3(0, 0, 0)),camUp(Vector3(0, 0, 0)),camRight(Vector3(0, 0, 0)),worldCenter(Vector3(0, 0, 0)),
	lookAt(Vector3(0, 0, 0)),right(Vector3(0, 0, 0)),up(Vector3(0, 0, 0)), newCenter(Vector3(0, 0, 0)), offset(Vector3(0, 0, 0)), cascadeCenterShadowSpace(Vector3(0, 0, 0)),
	oldCenterInCascade(Vector3(0, 0, 0)), newCenterInCascade(Vector3(0, 0, 0)), centerDiff(Vector3(0, 0, 0)),	 vBoundSpan(Vector3(0, 0, 0)),
	view{ 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f,   0.0f ,0.0f ,0.0f ,0.0f },
	shadowView{ 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f },
	shadowViewInv{ 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f },
    cascadeTrans{ 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f },
	cascadeScale{ 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f },
	shadowVP{}, CascadedShadowGenGeometryShader(nullptr), CascadedShadowGenGeometryCB(nullptr), ShadowGenDepthState(nullptr),
	CascadedShadowGenRS(nullptr), CascadedDepthStencilRT(nullptr), CascadedDepthStencilDSV(nullptr), CascadedDepthStencilSRV(nullptr)

{

	right = Vector3(1.0f, 0.0f, 0.0f);
}


CascadedShadow::~CascadedShadow()
{
	SafeRelease(CascadedShadowGenGeometryShader);
	SafeRelease(CascadedShadowGenGeometryCB);
	SafeRelease(ShadowGenDepthState);
	SafeRelease(CascadedShadowGenRS);
	SafeRelease(CascadedDepthStencilRT);
	SafeRelease(CascadedDepthStencilDSV);
	SafeRelease(CascadedDepthStencilSRV);
}

void CascadedShadow::Init(Vector2 iShadowMapSize)
{
	 this->ShadowMapSize = iShadowMapSize;

	// Set the range values
	 arrCascadeRanges[0] = 0.1f;
	 arrCascadeRanges[1] = 3.0f;
	 arrCascadeRanges[2] = 10.0f;
	 arrCascadeRanges[3] = CascadeTotalRange;

	for (int i = 0; i <  TotalCascades; i++)
	{
		 arrCascadeBoundCenter[i] = Vector3(0.0f, 0.0f, 0.0f);
		 arrCascadeBoundRadius[i] = 0.0f;
	}
}

void CascadedShadow::SetShadowMapSize(Vector2 iShadowMapSize)
{
	this->ShadowMapSize = iShadowMapSize;
}

void CascadedShadow::Update()
{
	GlobalData::GetView(&view);
	
	camPos = GlobalData::Position();
	camForward = GlobalData::Forward();
	worldCenter = camPos + camForward *  CascadeTotalRange * 0.5f;
	lookAt = worldCenter + GlobalData::LightDirection() * 1000.0f;
	D3DXVec3Cross(&up, &GlobalData::LightDirection(), &right);
	D3DXVec3Normalize(&up, &up);
	D3DXMatrixLookAtLH(&shadowView, &worldCenter, &lookAt, &up);
		
	float fRadius;
	ExtractFrustumBoundSphere( arrCascadeRanges[0],  arrCascadeRanges[3], ShadowBoundCenter, fRadius);
	ShadowBoundRadius = max( ShadowBoundRadius, fRadius); // Expend the radius to compensate for numerical errors
	
  	D3DXMatrixOrthoLH(&shadowProj, ShadowBoundRadius, ShadowBoundRadius, -ShadowBoundRadius, ShadowBoundRadius);
	D3DXMatrixMultiply(&WorldToShadowSpace, &shadowView, &shadowProj);
	D3DXMatrixTranspose(&shadowViewInv, &shadowView);
	for (int iCascadeIdx = 0; iCascadeIdx < TotalCascades; iCascadeIdx++)
	{
		ExtractFrustumBoundSphere( arrCascadeRanges[iCascadeIdx],  arrCascadeRanges[iCascadeIdx + 1], newCenter, fRadius);
		arrCascadeBoundRadius[iCascadeIdx] = max( arrCascadeBoundRadius[iCascadeIdx],fRadius); 
	
		 
		if (CascadeNeedsUpdate(&shadowView, iCascadeIdx, newCenter,offset))
		{
			Vector3 offsetOut;
			D3DXVec3TransformNormal(&offsetOut, &offset, &shadowViewInv);
			arrCascadeBoundCenter[iCascadeIdx] += offsetOut;
		}

		// Get the cascade center in shadow space
		D3DXVec3TransformCoord(&cascadeCenterShadowSpace, &arrCascadeBoundCenter[iCascadeIdx], &WorldToShadowSpace);
		
		// Update the translation from shadow to cascade space
		 ToCascadeOffsetX[iCascadeIdx] = -cascadeCenterShadowSpace.x;
		 ToCascadeOffsetY[iCascadeIdx] = -cascadeCenterShadowSpace.y;
		 D3DXMatrixTranslation(&cascadeTrans, ToCascadeOffsetX[iCascadeIdx], ToCascadeOffsetY[iCascadeIdx], 0.0f);
	
		 ToCascadeScale[iCascadeIdx] =  ShadowBoundRadius /  arrCascadeBoundRadius[iCascadeIdx];
		 D3DXMatrixScaling(&cascadeScale, ToCascadeScale[iCascadeIdx], ToCascadeScale[iCascadeIdx], 1.0f);
		// Combine the matrices to get the transformation from world to cascade space
		 D3DXMatrixTranspose(&arrWorldToCascadeProj[iCascadeIdx], &(WorldToShadowSpace * cascadeTrans * cascadeScale));
	}

	// Set the values for the unused slots to someplace outside the shadow space
	for (int i =  TotalCascades; i < 4; i++)
	{
		 ToCascadeOffsetX[i] =250.0f;
		 ToCascadeOffsetY[i] =250.0f;
		 ToCascadeScale[i] = 0.1f;
	}
}


void CascadedShadow::CascadedShadowsGen(ID3D11DeviceContext * context)
{
	context->OMSetDepthStencilState(ShadowGenDepthState, 2);
	context->RSSetState(CascadedShadowGenRS);

	context->RSSetViewports(3, shadowVP);

	// Set the depth target
	ID3D11RenderTargetView* nullRT = nullptr;
	context->OMSetRenderTargets(1, &nullRT, CascadedDepthStencilDSV);
	context->ClearDepthStencilView(CascadedDepthStencilDSV, D3D11_CLEAR_DEPTH, 1.0, 0);


	// Get the cascade matrices for the current camera configuration
	
	// Fill the shadow generation matrices constant buffer
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(CascadedShadowGenGeometryCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &arrWorldToCascadeProj, sizeof(Matrix)*TotalCascades);
	context->Unmap(CascadedShadowGenGeometryCB, 0);


	context->GSSetConstantBuffers(6, 1, &CascadedShadowGenGeometryCB);


	// Set the shadow generation shaders
	context->GSSetShader(CascadedShadowGenGeometryShader, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}


void CascadedShadow::ExtractFrustumBoundSphere(float fNear, float fFar, Vector3 & vBoundCenter, float & fBoundRadius)
{
	Matrix viewInv;
	D3DXMatrixInverse(&viewInv, nullptr, &view);

	camUp = Vector3(viewInv._21, viewInv._22, viewInv._23);
	camRight = Vector3(viewInv._11, viewInv._12, viewInv._13);

	const float& aspectRatio =static_cast<float>( D3D::Width()) / static_cast<float>(D3D::Height());
	
	const float& fTanFOVX = tanf(aspectRatio * Math::PI*0.25f);
	const float& fTanFOVY = tanf(aspectRatio);

	// The center of the sphere is in the center of the frustum
	vBoundCenter = camPos + camForward * (fNear + 0.5f * (fNear + fFar));

	// Radius is the distance to one of the frustum far corners
	vBoundSpan = camPos + (-camRight * fTanFOVX + camUp * fTanFOVY + camForward) * fFar - vBoundCenter;
	 fBoundRadius = D3DXVec3Length(&vBoundSpan);
}

bool CascadedShadow::CascadeNeedsUpdate(const Matrix * const mShadowView, int iCascadeIdx, const Vector3 & newCenter, Vector3 & vOffset)
{
	// Find the offset between the new and old bound ceter
	
	D3DXVec3TransformCoord(&oldCenterInCascade, &arrCascadeBoundCenter[iCascadeIdx], mShadowView);
	D3DXVec3TransformCoord(&newCenterInCascade, &newCenter, mShadowView);

	centerDiff = newCenterInCascade - oldCenterInCascade;

	// Find the pixel size based on the diameters and map pixel size
	Vector2 fPixelSize;
	fPixelSize.x= static_cast<float>(D3D::Width()) / (2.0f *  arrCascadeBoundRadius[iCascadeIdx]);
	fPixelSize.y = static_cast<float>(D3D::Height()) / (2.0f *  arrCascadeBoundRadius[iCascadeIdx]);
	const float& fPixelOffX = centerDiff.x * fPixelSize.x;
	const float& fPixelOffY = centerDiff.y * fPixelSize.y;

	// Check if the center moved at least half a pixel unit
	bool bNeedUpdate = abs(fPixelOffX) > 0.5f || abs(fPixelOffY) > 0.5f;
	if (bNeedUpdate)
	{
		// Round to the 
		vOffset.x = floorf(0.5f + fPixelOffX) / fPixelSize.x;
		vOffset.y = floorf(0.5f + fPixelOffY) / fPixelSize.y;
		vOffset.z = centerDiff.z;
	}

	return bNeedUpdate;
}

void CascadedShadow::CreateCascadedShadowBuffers(ID3D11Device* device,const Vector2 & size)
{
	D3D11_DEPTH_STENCIL_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.DepthEnable = TRUE;
	descDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	descDepth.DepthFunc = D3D11_COMPARISON_LESS;
	
   const D3D11_DEPTH_STENCILOP_DESC noSkyStencilOp = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_EQUAL };
	descDepth.FrontFace = noSkyStencilOp;
	descDepth.BackFace = noSkyStencilOp;

	
	descDepth.StencilEnable = true;
	descDepth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	descDepth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	Check(device->CreateDepthStencilState(&descDepth, &ShadowGenDepthState));

	D3D11_RASTERIZER_DESC descRast;
	ZeroMemory(&descRast, sizeof(descRast));
	descRast.CullMode = D3D11_CULL_BACK;
	descRast.FillMode = D3D11_FILL_SOLID;
	descRast.DepthBias = 6;

	descRast.SlopeScaledDepthBias = 3.0f;
	descRast.DepthClipEnable = true;
	Check(device->CreateRasterizerState(&descRast, &CascadedShadowGenRS));

	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory(&cbDesc, sizeof(cbDesc));
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.ByteWidth = 3 * sizeof(Matrix);
	Check(device->CreateBuffer(&cbDesc, nullptr, &CascadedShadowGenGeometryCB));

	ID3DBlob* ShaderBlob = nullptr;
	auto path = "../_Shaders/Lighting/ShadowGen.hlsl";
	auto entryPoint = "CascadedShadowMapsGenGS";
	auto shaderModel = "gs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateGeometryShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &CascadedShadowGenGeometryShader));
	SafeRelease(ShaderBlob);


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

	Check(device->CreateTexture2D(&dtd, nullptr, &CascadedDepthStencilRT));

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


	Init(size);

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
