#include "Framework.h"
#include "Smoke.h"

FLOAT randf()
{
	return FLOAT(rand()) / FLOAT(RAND_MAX);
}
Smoke::Smoke(ID3D11Device * device)
	:bWireframe(FALSE),
	bMeteredOverdraw(FALSE),
	InstanceTextureWH(512),
	InstanceTexture(NULL),
	InstanceTextureSRV(NULL),
	AbsoluteMaxNumActiveParticles(InstanceTextureWH * InstanceTextureWH),
	MaxNumActiveParticles(0),
	NumActiveParticles(0),
	ParticleVertexBuffer(NULL),
	ParticleIndexBuffer(NULL),
	ParticleIndexBufferTessellated(NULL),
	ParticleVertexLayout(NULL),
	ParticleTextureSRV(NULL),
	ParticleLifeTime(3.f),
	TessellationDensity(300.f),
	MaxTessellation(8.f),
	ParticleScale(0.01f),
	SoftParticleFadeDistance(0.25f),
	ParticlePrevBuffer(NULL),
	ParticleCurrBuffer(NULL),
	FirstUpdate(TRUE),
	LastUpdateTime(0.0),
	EmissionRate(0.f)
{
	ParticlePrevBuffer = new ParticleInstance[AbsoluteMaxNumActiveParticles];
	ParticleCurrBuffer = new ParticleInstance[AbsoluteMaxNumActiveParticles];

	
}

Smoke::~Smoke()
{
	delete[] ParticlePrevBuffer;
	delete[] ParticleCurrBuffer;
}

void Smoke::InitPlumeSim()
{
	MaxNumActiveParticles = 128 * 128;
	ParticleLifeTime = 10.f;
	ParticleScale = 0.05f;
	FirstUpdate = true;
	LastUpdateTime = 0;

	EmissionRate = float(MaxNumActiveParticles) / ParticleLifeTime;
}
void randPointInUnitDisk(float& x, float& y)
{
	const float angle = 2.f * float(Math::PI) *randf();
	const float r = sqrtf(randf());

	x = r * cosf(angle);
	y = r * sinf(angle);
}
void Smoke::PlumeSimTick(double Time)
{
	if (FirstUpdate)
	{
		// Try loading the state
		//LoadParticleState(Time);

		LastUpdateTime = Time;
		FirstUpdate = false;
		
	}

	if (LastUpdateTime == Time)
		return ;

	const float timeDelta = float(Time - LastUpdateTime);

	// Prepare to update bounds
	bool boundsFirstUpdate = true;
	MinCorner = D3DXVECTOR3(0.f, 0.f, 0.f);
	MaxCorner = D3DXVECTOR3(0.f, 0.f, 0.f);

	// Flip the buffers ready for the simulation update
	ParticleInstance* Temp = ParticleCurrBuffer;
	ParticleCurrBuffer = ParticlePrevBuffer;
	ParticlePrevBuffer = Temp;

	// First, simulate the existing particles, killing as we go
	const ParticleInstance* pSrcEnd = ParticlePrevBuffer + NumActiveParticles;
	ParticleInstance* pDst = ParticleCurrBuffer;
	for (const ParticleInstance* pSrc = ParticlePrevBuffer; pSrc != pSrcEnd; ++pSrc)
	{
		if (!pSrc->IsDead(Time))
		{
			D3DXVECTOR3 pos(pSrc->positionAndOpacity);
			pos += timeDelta * D3DXVECTOR3(pSrc->velocityAndLifetime);

			// Lifetime-driven opacity fade
			const float lifeParam = pSrc->LifeParam(Time);
			float opacity = 1.f;
			if (lifeParam > 0.9f)
				opacity = 10.f * (1.f - lifeParam);

			pDst->positionAndOpacity = D3DXVECTOR4(pos, opacity);

			// Decel on upward motion
			pDst->velocityAndLifetime = pSrc->velocityAndLifetime;
			pDst->velocityAndLifetime.y -= 0.1f * timeDelta / ParticleLifeTime;

			// Copy remaining properties
			pDst->birthTime = pSrc->birthTime;

			UpdateBounds(boundsFirstUpdate, pDst->positionAndOpacity);

			++pDst;
		}
	}

	const uint DstParticlesLeft = uint((ParticleCurrBuffer + MaxNumActiveParticles) - pDst);
	const uint ParticleToEmitThisTick = min(DstParticlesLeft, uint(EmissionRate * float(timeDelta)));
	for (uint i = 0; i != ParticleToEmitThisTick; ++i, ++pDst)
	{
		float x, z;
		randPointInUnitDisk(x, z);

		pDst->positionAndOpacity.x = 0.1f * x;
		pDst->positionAndOpacity.y = ParticleScale;
		pDst->positionAndOpacity.z = 0.1f * z;
		pDst->positionAndOpacity.w = 1.f;

		float vx, vz;
		randPointInUnitDisk(vx, vz);

		pDst->velocityAndLifetime.x = 0.05f * vx;
		pDst->velocityAndLifetime.y = 0.1f + 0.05f * randf();
		pDst->velocityAndLifetime.z = 0.05f * vz;
		pDst->velocityAndLifetime.w = randf() * ParticleLifeTime;

		pDst->birthTime = Time;

		UpdateBounds(boundsFirstUpdate, pDst->positionAndOpacity);
	}

	NumActiveParticles = uint(pDst - ParticleCurrBuffer);

	LastUpdateTime = Time;

	
}

void Smoke::SortByDepth()
{
	const D3DXVECTOR3 viewDir=GlobalData::Forward();

	sort(ParticleCurrBuffer, ParticleCurrBuffer + NumActiveParticles, DepthSortFunctor(viewDir));

	
}

void Smoke::UpdateD3D11Resources(ID3D11DeviceContext * DC)
{
	
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	DC->Map(InstanceTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);

	BYTE* pCurrRowStart = reinterpret_cast<BYTE*>(MappedResource.pData);
	D3DXFLOAT16* pCurrTexel = reinterpret_cast<D3DXFLOAT16*>(pCurrRowStart);
	D3DXFLOAT16* pCurrRowEnd = pCurrTexel + (4 * InstanceTextureWH);
	const ParticleInstance* pCurrSrc = ParticleCurrBuffer;
	for (UINT ix = 0; ix != NumActiveParticles; ++ix)
	{
		pCurrTexel[0] = pCurrSrc->positionAndOpacity.x;
		pCurrTexel[1] = pCurrSrc->positionAndOpacity.y;
		pCurrTexel[2] = pCurrSrc->positionAndOpacity.z;
		pCurrTexel[3] = pCurrSrc->positionAndOpacity.w;

		++pCurrSrc;
		pCurrTexel += 4;
		if (pCurrTexel == pCurrRowEnd)
		{
			// New row!
			pCurrRowStart = pCurrRowStart + MappedResource.RowPitch;
			pCurrTexel = reinterpret_cast<D3DXFLOAT16*>(pCurrRowStart);
			pCurrRowEnd = pCurrTexel + (4 * InstanceTextureWH);
		}
	}

	DC->Unmap(InstanceTexture, 0);

}

void Smoke::OnD3D11CreateDevice(ID3D11Device * device)
{

	// Set up particle instance buffers
	{
		D3D11_TEXTURE2D_DESC texDesc;
		texDesc.Width = InstanceTextureWH;
		texDesc.Height = InstanceTextureWH;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DYNAMIC;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		texDesc.MiscFlags = 0;

		Check(device->CreateTexture2D(&texDesc, NULL, &InstanceTexture));

		D3D11_SHADER_RESOURCE_VIEW_DESC rvDesc;
		rvDesc.Format = texDesc.Format;
		rvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		rvDesc.Texture2D.MostDetailedMip = 0;
		rvDesc.Texture2D.MipLevels = texDesc.MipLevels;

		Check(device->CreateShaderResourceView(InstanceTexture, &rvDesc, &InstanceTextureSRV));
	}

	// Set up particle vertex/index buffers
	ParticleVertex* pVertices = new ParticleVertex[4 * AbsoluteMaxNumActiveParticles];
	DWORD* pIndices = new DWORD[6 * AbsoluteMaxNumActiveParticles];
	DWORD* pTessellatedIndices = new DWORD[4 * AbsoluteMaxNumActiveParticles];
	{
		ParticleVertex* pCurrVertex = pVertices;
		DWORD* pCurrIndex = pIndices;
		DWORD* pCurrTessellatedIndex = pTessellatedIndices;
		DWORD currBaseIndex = 0;
		for (UINT ix = 0; ix != AbsoluteMaxNumActiveParticles; ++ix)
		{
			pCurrIndex[0] = currBaseIndex + 0;
			pCurrIndex[1] = currBaseIndex + 1;
			pCurrIndex[2] = currBaseIndex + 2;
			pCurrIndex[3] = currBaseIndex + 2;
			pCurrIndex[4] = currBaseIndex + 1;
			pCurrIndex[5] = currBaseIndex + 3;

			pCurrTessellatedIndex[0] = currBaseIndex + 0;
			pCurrTessellatedIndex[1] = currBaseIndex + 1;
			pCurrTessellatedIndex[2] = currBaseIndex + 2;
			pCurrTessellatedIndex[3] = currBaseIndex + 3;

			pCurrVertex[0].instanceIndex = FLOAT(ix);
			pCurrVertex[1].instanceIndex = FLOAT(ix);
			pCurrVertex[2].instanceIndex = FLOAT(ix);
			pCurrVertex[3].instanceIndex = FLOAT(ix);

			pCurrVertex[0].cornerIndex = 0;
			pCurrVertex[1].cornerIndex = 1;
			pCurrVertex[2].cornerIndex = 2;
			pCurrVertex[3].cornerIndex = 3;

			currBaseIndex += 4;
			pCurrVertex += 4;
			pCurrIndex += 6;
			pCurrTessellatedIndex += 4;
		}
	}

	{
		D3D11_BUFFER_DESC vbDesc;
		vbDesc.ByteWidth = 4 * AbsoluteMaxNumActiveParticles * sizeof(ParticleVertex);
		vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbDesc.CPUAccessFlags = 0;
		vbDesc.MiscFlags = 0;
		vbDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vbSRD;
		vbSRD.pSysMem = pVertices;
		vbSRD.SysMemPitch = 0;
		vbSRD.SysMemSlicePitch = 0;

		Check(device->CreateBuffer(&vbDesc, &vbSRD, &ParticleVertexBuffer));
	}

	{
		D3D11_BUFFER_DESC ibDesc;
		ibDesc.ByteWidth = 6 * AbsoluteMaxNumActiveParticles * sizeof(DWORD);
		ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibDesc.CPUAccessFlags = 0;
		ibDesc.MiscFlags = 0;
		ibDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA ibSRD;
		ibSRD.pSysMem = pIndices;
		ibSRD.SysMemPitch = 0;
		ibSRD.SysMemSlicePitch = 0;

		Check(device->CreateBuffer(&ibDesc, &ibSRD, &ParticleIndexBuffer));
	}

	{
		D3D11_BUFFER_DESC ibDesc;
		ibDesc.ByteWidth = 4 * AbsoluteMaxNumActiveParticles * sizeof(DWORD);
		ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
		ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibDesc.CPUAccessFlags = 0;
		ibDesc.MiscFlags = 0;
		ibDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA ibSRD;
		ibSRD.pSysMem = pTessellatedIndices;
		ibSRD.SysMemPitch = 0;
		ibSRD.SysMemSlicePitch = 0;

		Check(device->CreateBuffer(&ibDesc, &ibSRD, &ParticleIndexBufferTessellated));
	}

	delete[] pVertices;
	delete[] pIndices;
	delete[] pTessellatedIndices;

	// Set up particle texture
	{
		WCHAR str[MAX_PATH];
		//Check(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, L"OpacityMapping\\smoke.dds"));

		ID3D11Resource* pParticleTextureRsrc = NULL;
		Check(D3DX11CreateTextureFromFile(device, str, NULL, NULL, &pParticleTextureRsrc, NULL));

		// Downcast to texture 2D
		ID3D11Texture2D* pParticleTexture = NULL;
		Check(pParticleTextureRsrc->QueryInterface(IID_ID3D11Texture2D, (LPVOID*)&pParticleTexture));
		SafeRelease(pParticleTextureRsrc);

		D3D11_TEXTURE2D_DESC texDesc;
		pParticleTexture->GetDesc(&texDesc);

		D3D11_SHADER_RESOURCE_VIEW_DESC rvDesc;
		rvDesc.Format = texDesc.Format;
		rvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		rvDesc.Texture2D.MostDetailedMip = 0;
		rvDesc.Texture2D.MipLevels = texDesc.MipLevels;

		Check(device->CreateShaderResourceView(pParticleTexture, &rvDesc, &ParticleTextureSRV));

		SafeRelease(pParticleTexture);
	}
}

void Smoke::OnD3D11DestroyDevice()
{
	SafeRelease(ParticleVertexLayout);

	SafeRelease(InstanceTexture);
	SafeRelease(InstanceTextureSRV);
	SafeRelease(ParticleVertexBuffer);
	SafeRelease(ParticleIndexBuffer);
	SafeRelease(ParticleIndexBufferTessellated);

	SafeRelease(ParticleTextureSRV);
}

void Smoke::DrawToOpacity(ID3D11DeviceContext * DC)
{
	/*V_RETURN(m_InstanceTexture_EffectVar->SetResource(m_pInstanceTextureSRV));
	V_RETURN(m_ParticleScale_EffectVar->SetFloat(m_ParticleScale));
	V_RETURN(m_TessellationDensity_EffectVar->SetFloat(m_TessellationDensity));
	V_RETURN(m_MaxTessellation_EffectVar->SetFloat(m_MaxTessellation));
	V_RETURN(m_ParticleTexture_EffectVar->SetResource(m_pParticleTextureSRV));

	D3DXMATRIXA16 mIdentity;
	D3DXMatrixIdentity(&mIdentity);
	V_RETURN(m_mLocalToWorld_EffectVar->SetMatrix((FLOAT*)&mIdentity));*/

	//V_RETURN(m_pEffectPass_ToOpacity->Apply(0, pDC));
	
	/*DC->IASetInputLayout(ParticleVertexLayout);

	UINT Stride = sizeof(ParticleVertex);
	UINT Offset = 0;
	DC->IASetVertexBuffers(0, 1, &ParticleVertexBuffer, &Stride, &Offset);

	ID3D11Buffer* pIndexBuffer =  ParticleIndexBufferTessellated ;
	DC->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	D3D11_PRIMITIVE_TOPOLOGY topology = UseTessellation ? D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	DC->IASetPrimitiveTopology(topology);

	UINT NumIndices = UseTessellation ?NumActiveParticles * 4 :NumActiveParticles * 6;
	DC->DrawIndexed(NumIndices, 0, 0);*/
}

void Smoke::DrawSoftToSceneLowRes(ID3D11DeviceContext * DC, ID3D11ShaderResourceView * DepthStencilSRV, float zNear, float zFar)
{

	//BOOL UseTessellation = FALSE;
	//ID3DX11EffectPass* pPass = m_pEffectPass_SoftToScene[m_LightingMode];
	//if (LM_Tessellated == m_LightingMode)
	//{
	//	if (pPass)
	//		UseTessellation = TRUE;
	//	else
	//		pPass = m_pEffectPass_SoftToScene[LM_Pixel];
	//}

	//// Set soft-specific params
	//FLOAT ZLinParams[4] = { zFar / zNear, (zFar / zNear - 1.f), zFar, 0.f };
	//V_RETURN(ZLinParams_EffectVar->SetFloatVector(ZLinParams));
	//V_RETURN(SoftParticlesFadeDistance_EffectVar->SetFloat(SoftParticleFadeDistance * ParticleScale));
	//V_RETURN(SoftParticlesDepthTexture_EffectVar->SetResource(DepthStencilSRV));

	//V_RETURN(SetEffectVars());
	//V_RETURN(pPass->Apply(0, pDC));

	//if (m_bWireframe)
	//{
	//	V_RETURN(m_pEffectPass_ToSceneWireframeOverride->Apply(0, pDC));
	//}

	//V_RETURN(m_pEffectPass_LowResToSceneOverride->Apply(0, pDC));

	//V_RETURN(Draw(pDC, UseTessellation));

}

void Smoke::BindToEffect(ID3D11Device * pd3dDevice)
{

	//InstanceTexture_EffectVar = pEffect->GetVariableByName("g_InstanceTexture")->AsShaderResource();
	//ParticleTexture_EffectVar = pEffect->GetVariableByName("g_ParticleTexture")->AsShaderResource();
	//ParticleScale_EffectVar = pEffect->GetVariableByName("g_ParticleScale")->AsScalar();
	//mLocalToWorld_EffectVar = pEffect->GetVariableByName("g_mLocalToWorld")->AsMatrix();
	//ZLinParams_EffectVar = pEffect->GetVariableByName("g_ZLinParams")->AsVector();
	//SoftParticlesFadeDistance_EffectVar = pEffect->GetVariableByName("g_SoftParticlesFadeDistance")->AsScalar();
	//SoftParticlesDepthTexture_EffectVar = pEffect->GetVariableByName("g_SoftParticlesDepthTexture")->AsShaderResource();

	//TessellationDensity_EffectVar = pEffect->GetVariableByName("g_TessellationDensity")->AsScalar();
	//MaxTessellation_EffectVar = pEffect->GetVariableByName("g_MaxTessellation")->AsScalar();

	//pEffectPass_ToScene[LM_Vertex] = pEffect->GetTechniqueByName("RenderParticlesToScene_LM_Vertex")->GetPassByIndex(0);
	//pEffectPass_SoftToScene[LM_Vertex] = pEffect->GetTechniqueByName("RenderSoftParticlesToScene_LM_Vertex")->GetPassByIndex(0);
	//pEffectPass_ToScene[LM_Pixel] = pEffect->GetTechniqueByName("RenderParticlesToScene_LM_Pixel")->GetPassByIndex(0);
	//pEffectPass_SoftToScene[LM_Pixel] = pEffect->GetTechniqueByName("RenderSoftParticlesToScene_LM_Pixel")->GetPassByIndex(0);
	//pEffectPass_LowResToSceneOverride = pEffect->GetTechniqueByName("RenderLowResParticlesToSceneOverride")->GetPassByIndex(0);
	//pEffectPass_ToSceneWireframeOverride = pEffect->GetTechniqueByName("RenderParticlesToSceneWireframeOverride")->GetPassByIndex(0);
	//pEffectPass_ToSceneOverdrawMeteringOverride = pEffect->GetTechniqueByName("RenderToSceneOverdrawMeteringOverride")->GetPassByIndex(0);

	//m_pEffectPass_ToOpacity = pEffect->GetTechniqueByName("RenderParticlesToOpacity")->GetPassByIndex(0);
	//m_pEffectPass_ToScene[LM_Tessellated] = NULL;
	//m_pEffectPass_SoftToScene[LM_Tessellated] = NULL;
	//if (pd3dDevice->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
	//{
	//	m_pEffectPass_ToScene[LM_Tessellated] = pEffect->GetTechniqueByName("RenderParticlesToScene_LM_Tessellated")->GetPassByIndex(0);
	//	m_pEffectPass_SoftToScene[LM_Tessellated] = pEffect->GetTechniqueByName("RenderSoftParticlesToScene_LM_Tessellated")->GetPassByIndex(0);
	//}

	//SAFE_RELEASE(m_pParticleVertexLayout);
	//D3DX11_PASS_DESC passDesc;
	//m_pEffectPass_ToScene[LM_Vertex]->GetDesc(&passDesc);
	//Check(pd3dDevice->CreateInputLayout(ParticleVertex::GetLayout(), ParticleVertex::NumLayoutElements,
	//	passDesc.pIAInputSignature, passDesc.IAInputSignatureSize,
	//	&ParticleVertexLayout));
}

void Smoke::Draw(ID3D11DeviceContext * DC)
{
	DC->IASetInputLayout(ParticleVertexLayout);

	UINT Stride = sizeof(ParticleVertex);
	UINT Offset = 0;
	DC->IASetVertexBuffers(0, 1, &ParticleVertexBuffer, &Stride, &Offset);

	ID3D11Buffer* pIndexBuffer = ParticleIndexBufferTessellated ;
	DC->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
	DC->IASetPrimitiveTopology(topology);

	UINT NumIndices = NumActiveParticles * 4 ;
	DC->DrawIndexed(NumIndices, 0, 0);
}

void Smoke::UpdateBounds(bool & firstUpdate, const D3DXVECTOR4 & positionAndScale)
{
	Vector3 particleMin, particleMax;

	particleMin.x = positionAndScale.x - ParticleScale;
	particleMin.y = positionAndScale.y - ParticleScale;
	particleMin.z = positionAndScale.z - ParticleScale;

	particleMax.x = positionAndScale.x + ParticleScale;
	particleMax.y = positionAndScale.y + ParticleScale;
	particleMax.z = positionAndScale.z + ParticleScale;

	if (firstUpdate)
	{
		MinCorner = particleMin;
		MaxCorner = particleMax;
		firstUpdate = FALSE;
	}
	else
	{
		MinCorner.x = min(MinCorner.x, particleMin.x);
		MinCorner.y = min(MinCorner.y, particleMin.y);
		MinCorner.z = min(MinCorner.z, particleMin.z);

		MaxCorner.x = max(MaxCorner.x, particleMax.x);
		MaxCorner.y = max(MaxCorner.y, particleMax.y);
		MaxCorner.z = max(MaxCorner.z, particleMax.z);
	}
}
