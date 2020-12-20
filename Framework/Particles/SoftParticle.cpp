#include "Framework.h"
#include "SoftParticle.h"
#include "Renders/Shader.h"

float g_ParticleVel = 3.0f;

void SoftParticle::Initialize(ID3D11Device* device)
{
	/*D3D11_TEXTURE2D_DESC tex_desc;
	D3D11_SHADER_RESOURCE_VIEW_DESC textureSRV_desc;
	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	ZeroMemory(&tex_desc, sizeof(tex_desc));

	tex_desc.Width = (UINT)(D3D::Width());
	tex_desc.Height = (UINT)(D3D::Height());
	tex_desc.MipLevels = (UINT)max(1, log(max((float)tex_desc.Width, (float)tex_desc.Height)) / (float)log(2.0f));
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	textureSRV_desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	textureSRV_desc.Texture2D.MostDetailedMip = 0;

	Check(device->CreateTexture2D(&tex_desc, NULL, &diffuseResource));
	Check(device->CreateShaderResourceView(diffuseResource, &textureSRV_desc, &diffuseResourceSRV));

	Check(device->CreateTexture2D(&tex_desc, NULL, &normalResource));
	Check(device->CreateShaderResourceView(normalResource, &textureSRV_desc, &normalResourceSRV));

	Check(device->CreateTexture2D(&tex_desc, NULL, &pbrResource));
	Check(device->CreateShaderResourceView(pbrResource, &textureSRV_desc, &pbrResourceSRV));*/
	
	shader = new Shader(device,L"Particles/SoftParticle/SoftParticles.fx");

	// Obtain the parameter handles
	g_pmWorldViewProj = shader->AsMatrix("g_mWorldViewProj");
	g_pmWorldView = shader->AsMatrix("g_mWorldView");
	g_pmWorld = shader->AsMatrix("g_mWorld");
	g_pmInvView = shader->AsMatrix("g_mInvView");
	g_pmInvProj = shader->AsMatrix("g_mInvProj");
	g_pfFadeDistance = shader->AsScalar("g_fFadeDistance");
	g_pfSizeZScale = shader->AsScalar("g_fSizeZScale");
	g_pvViewLightDir1 = shader->AsVector("g_vViewLightDir1");
	g_pvViewLightDir2 = shader->AsVector("g_vViewLightDir2");
	g_pvWorldLightDir1 = shader->AsVector("g_vWorldLightDir1");
	g_pvWorldLightDir2 = shader->AsVector("g_vWorldLightDir2");
	g_pvEyePt = shader->AsVector("g_vEyePt");
	g_pvViewDir = shader->AsVector("g_vViewDir");
	g_pvOctaveOffsets = shader->AsVector("g_OctaveOffsets");
	g_pvScreenSize = shader->AsVector("g_vScreenSize");
	g_pDiffuseTex = shader->AsSRV("g_txDiffuse");
	g_pNormalTex = shader->AsSRV("g_txNormal");
	g_pColorGradient = shader->AsSRV("g_txColorGradient");
	g_pVolumeDiffTex = shader->AsSRV("g_txVolumeDiff");
	g_pVolumeNormTex = shader->AsSRV("g_txVolumeNorm");
	g_pDepthTex = shader->AsSRV("g_txDepth");
	g_pDepthMSAATex = shader->AsSRV("g_txDepthMSAA");

	//diffuseMap = shader->AsSRV("diffuseMap");
	//normalMap = shader->AsSRV("normapMap");
	//pbrMap = shader->AsSRV("pbrMap");

	
	// Create the particles
	CreateParticleBuffers(device);

	// Create the noise volume
	CreateNoiseVolume(device, 32);

	// Load the Particle Texture
	
	smokevol = new Texture();
	smokevol->Load(device,L"SoftParticle/smokevol1.dds");
	gradient = new Texture();
	gradient->Load(device, L"SoftParticle/colorgradient.dds");
	
	g_pfFadeDistance->SetFloat(g_fFadeDistance);

	D3D11_TEXTURE2D_DESC dtd = {
			D3D::GetDesc().Width, //UINT Width;
			D3D::GetDesc().Height, //UINT Height;
		1, //UINT MipLevels;
		1, //UINT ArraySize;
		DXGI_FORMAT_R32_TYPELESS, //DXGI_FORMAT Format;
		1, //DXGI_SAMPLE_DESC SampleDesc;
		0,
		D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
		0,//UINT CPUAccessFlags;
		0//UINT MiscFlags;    
	};
	
	Check(device->CreateTexture2D(&dtd, NULL, &depthStencilTexture));

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd =
	{
		DXGI_FORMAT_D32_FLOAT,
			D3D11_DSV_DIMENSION_TEXTURE2D,
			0
	};
	Check(device->CreateDepthStencilView(depthStencilTexture, &dsvd, &depthStencilDSV));

	D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd =
	{
		DXGI_FORMAT_R32_FLOAT,
		D3D11_SRV_DIMENSION_TEXTURE2DMS,
		0,
		0
	};
	dsrvd.Texture2D.MipLevels = 1;
	Check(device->CreateShaderResourceView(depthStencilTexture, &dsrvd, &depthStencilSRV));
}
void SoftParticle::Update(ID3D11DeviceContext* context)
{
	
	D3DXVECTOR3 vEye= GlobalData::Position();


	D3DXVECTOR3 vDir;
	D3DXVec3Normalize(&vDir, &GlobalData::Forward());

	runningTime += Time::Delta();
	AdvanceParticles(runningTime, Time::Delta());
	SortParticleBuffer(vEye, vDir);
	UpdateParticleBuffers(context);

	// Update the movement of the noise octaves
	D3DXVECTOR4 OctaveOffsets[4];
	for (int i = 0; i < 4; i++)
	{
		OctaveOffsets[i].x = -(float)(runningTime*0.05);
		OctaveOffsets[i].y = 0;
		OctaveOffsets[i].z = 0;
		OctaveOffsets[i].w = 0;
	}
	g_pvOctaveOffsets->SetFloatVectorArray((float*)OctaveOffsets, 0, 4);

}

void SoftParticle::Render(ID3D11DeviceContext* context, ID3D11ShaderResourceView* srv)
{
	/*context->ResolveSubresource(diffuseResource, 0, diffuse, 0, DXGI_FORMAT_R10G10B10A2_UNORM);
	context->ResolveSubresource(normalResource, 0, normal, 0, DXGI_FORMAT_R10G10B10A2_UNORM);
	context->ResolveSubresource(pbrResource, 0, PBR, 0, DXGI_FORMAT_R10G10B10A2_UNORM);

	diffuseMap->SetResource(diffuseResourceSRV);
	normalMap->SetResource(normalResourceSRV);
	pbrMap->SetResource(pbrResourceSRV);*/
	// Get the projection & view matrix from the camera class
	D3DXMATRIX mWorld;
	D3DXMATRIX mView = GlobalData::GetView();
	D3DXMATRIX mProj = GlobalData::GetProj();
	D3DXMATRIX mInvView;
	D3DXMATRIX mInvProj,S;
	D3DXVECTOR3 vec3 = GlobalData::Position();
	D3DXVECTOR3 viewDir = GlobalData::Forward();
  // Vector3 pos = vec3 + viewDir*10;
	Vector3 pos = Vector3(256, 0, 256);
	D3DXMatrixTranslation(&mWorld, pos.x, pos.y, pos.z);
	D3DXMatrixScaling(&S,10,10, 10);
	//D3DXMatrixIdentity(&mWorld);
	
	D3DXMATRIX mWorldViewProj = S*mWorld * mView*mProj;
	D3DXMATRIX mWorldView = mWorld * mView;
	D3DXMatrixInverse(&mInvView, NULL, &mView);
	D3DXMatrixInverse(&mInvProj, NULL, &mProj);
	D3DXVECTOR4 vViewLightDir1;
	D3DXVECTOR4 vWorldLightDir1;
	D3DXVECTOR4 vViewLightDir2;
	D3DXVECTOR4 vWorldLightDir2;
	D3DXVec3Normalize((D3DXVECTOR3*)&vWorldLightDir1, &g_vLightDir1);
	D3DXVec3TransformNormal((D3DXVECTOR3*)&vViewLightDir1, &g_vLightDir1, &mView);
	D3DXVec3Normalize((D3DXVECTOR3*)&vViewLightDir1, (D3DXVECTOR3*)&vViewLightDir1);
	D3DXVec3Normalize((D3DXVECTOR3*)&vWorldLightDir2, &g_vLightDir2);
	D3DXVec3TransformNormal((D3DXVECTOR3*)&vViewLightDir2, &g_vLightDir2, &mView);
	D3DXVec3Normalize((D3DXVECTOR3*)&vViewLightDir2, (D3DXVECTOR3*)&vViewLightDir2);

	
	D3DXVec3Normalize(&viewDir, &viewDir);
	
	
	D3DXVECTOR4 vEyePt;
	vEyePt.x = vec3.x;
	vEyePt.y = vec3.y;
	vEyePt.z = vec3.z;
	FLOAT fScreenSize[2] = { (FLOAT)g_iWidth, (FLOAT)g_iHeight };

	g_pmWorldViewProj->SetMatrix((float*)&mWorldViewProj);
	g_pmWorldView->SetMatrix((float*)&mWorldView);
	g_pmWorld->SetMatrix((float*)&mWorld);
	g_pmInvView->SetMatrix((float*)&mInvView);
	g_pmInvProj->SetMatrix((float*)&mInvProj);
	g_pvViewLightDir1->SetFloatVector((float*)&vViewLightDir1);
	g_pvWorldLightDir1->SetFloatVector((float*)&vWorldLightDir1);
	g_pvViewLightDir2->SetFloatVector((float*)&vViewLightDir2);
	g_pvWorldLightDir2->SetFloatVector((float*)&vWorldLightDir2);
	g_pvViewDir->SetFloatVector((float*)&viewDir);
	g_pvEyePt->SetFloatVector((float*)&vEyePt);
	g_pvScreenSize->SetFloatVector(fScreenSize);

	

	uint pass = 0;

	if (1 == g_iSampleCount) {
		switch (g_ParticleTechnique)
		{
		case PT_BILLBOARD_HARD:
			pass = 0;
			break;
		case PT_BILLBOARD_ODEPTH:
			pass = 1;
			break;
		case PT_BILLBOARD_SOFT:
			pass = 2;
			break;
		case PT_BILLBOARD_ODEPTHSOFT:
			pass = 3;
			break;
		case PT_VOLUME_HARD:
			pass = 4;
			break;
		case PT_VOLUME_SOFT:
			pass = 5;
			break;
		};
	}
	/*else {
		switch (g_ParticleTechnique)
		{
		case PT_BILLBOARD_HARD:
			pParticleTech = g_pRenderBillboardParticlesHard;
			break;
		case PT_BILLBOARD_ODEPTH:
			pParticleTech = g_pRenderBillboardParticlesODepth;
			break;
		case PT_BILLBOARD_SOFT:
			pParticleTech = g_pRenderBillboardParticlesSoftMSAA;
			break;
		case PT_BILLBOARD_ODEPTHSOFT:
			pParticleTech = g_pRenderBillboardParticlesODepthSoftMSAA;
			break;
		case PT_VOLUME_HARD:
			pParticleTech = g_pRenderVolumeParticlesHardMSAA;
			break;
		case PT_VOLUME_SOFT:
			pParticleTech = g_pRenderVolumeParticlesSoftMSAA;
			break;
		};
	}*/

	

	if (PT_BILLBOARD_HARD != g_ParticleTechnique &&
		PT_BILLBOARD_ODEPTH != g_ParticleTechnique)
	{
		// Unbind the depth stencil texture from the device
		//context->OMSetRenderTargets(1, &rtv, nullptr);
		// Bind it instead as a shader resource view
		g_pDepthTex->SetResource(srv);
	}

	// Render the particles
	
	ID3D11Buffer *pBuffers[1] = { g_pParticleVB };
	UINT stride[1] = { sizeof(PARTICLE_VERTEX) };
	UINT offset[1] = { 0 };
	context->IASetVertexBuffers(0, 1, pBuffers, stride, offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->IASetIndexBuffer(g_pParticleIB, DXGI_FORMAT_R32_UINT, 0);

	if (PT_VOLUME_HARD == g_ParticleTechnique ||
		PT_VOLUME_SOFT == g_ParticleTechnique)
	{
		g_pVolumeDiffTex->SetResource(g_pNoiseVolumeRV);
		g_pVolumeNormTex->SetResource(NULL);
	}
	else
	{
		g_pVolumeDiffTex->SetResource(smokevol->SRV());
	}
	g_pColorGradient->SetResource(gradient->SRV());

	
	shader->DrawIndexed(context,0, pass, MAX_PARTICLES, 0, 0);
	

	// unbind the depth from the resource so we can set it as depth next time around
	ID3D11ShaderResourceView* Nulls[2] = { NULL,NULL };
	context->PSSetShaderResources(0, 2, Nulls);

	
	
}
struct CHAR4
{
	char x, y, z, w;
};
void SoftParticle::CreateParticleBuffers(ID3D11Device * pd3dDevice)
{
	D3D11_BUFFER_DESC vbdesc;
	vbdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbdesc.ByteWidth = MAX_PARTICLES * sizeof(PARTICLE_VERTEX);
	vbdesc.CPUAccessFlags = 0;
	vbdesc.MiscFlags = 0;
	vbdesc.Usage = D3D11_USAGE_DEFAULT;
	Check(pd3dDevice->CreateBuffer(&vbdesc, NULL, &g_pParticleVB));

	vbdesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
	vbdesc.ByteWidth = MAX_PARTICLES * sizeof(DWORD);
	Check(pd3dDevice->CreateBuffer(&vbdesc, NULL, &g_pParticleIB));

	g_pCPUParticles = new PARTICLE_VERTEX[MAX_PARTICLES];
	
	for (UINT i = 0; i < MAX_PARTICLES; i++)
	{
		g_pCPUParticles[i].Life = -1;	//kill all particles
	}

	g_pCPUParticleIndices = new DWORD[MAX_PARTICLES];
	
	g_pParticleDepthArray = new float[MAX_PARTICLES];
	
}
float RPercent()
{
	float ret = (float)((rand() % 20000) - 10000);
	return ret / 10000.0f;
}
void EmitParticle(PARTICLE_VERTEX* pParticle,uint index)
{
	uint width = 20;
	uint height = 20;
	uint distance = 40;
	pParticle->Pos.x = static_cast<float>((index% width)*distance);
	pParticle->Pos.y = 0.7f;
	pParticle->Pos.z = 0;

	pParticle->Vel.x = 1.0f;
	pParticle->Vel.y = 0.3f*RPercent();
	pParticle->Vel.z = 0.3f*RPercent();

	D3DXVec3Normalize(&pParticle->Vel, &pParticle->Vel);
	pParticle->Vel *= g_ParticleVel;

	pParticle->Life = 0.0f;
	pParticle->Size = 0.0f;
}

float GetDensity(int x, int y, int z, CHAR4* pTexels, UINT VolumeSize)
{
	if (x < 0)
		x += VolumeSize;
	if (y < 0)
		y += VolumeSize;
	if (z < 0)
		z += VolumeSize;

	x = x % VolumeSize;
	y = y % VolumeSize;
	z = z % VolumeSize;

	int index = x + y * VolumeSize + z * VolumeSize*VolumeSize;

	return (float)pTexels[index].w / 128.0f;
}

void SetNormal(D3DXVECTOR3 Normal, int x, int y, int z, CHAR4* pTexels, UINT VolumeSize)
{
	if (x < 0)
		x += VolumeSize;
	if (y < 0)
		y += VolumeSize;
	if (z < 0)
		z += VolumeSize;

	x = x % VolumeSize;
	y = y % VolumeSize;
	z = z % VolumeSize;

	int index = x + y * VolumeSize + z * VolumeSize*VolumeSize;

	pTexels[index].x = (char)(Normal.x * 128.0f);
	pTexels[index].y = (char)(Normal.y * 128.0f);
	pTexels[index].z = (char)(Normal.z * 128.0f);
}
void SoftParticle::CreateNoiseVolume(ID3D11Device * pd3dDevice, UINT VolumeSize)
{
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = new CHAR4[VolumeSize*VolumeSize*VolumeSize];
	InitData.SysMemPitch = VolumeSize * sizeof(CHAR4);
	InitData.SysMemSlicePitch = VolumeSize * VolumeSize * sizeof(CHAR4);

	// Gen a bunch of random values
	CHAR4* pData = (CHAR4*)InitData.pSysMem;
	for (UINT i = 0; i < VolumeSize*VolumeSize*VolumeSize; i++)
	{
		pData[i].w = (char)(RPercent() * 128.0f);
	}

	// Generate normals from the density gradient
	float heightAdjust = 0.5f;
	D3DXVECTOR3 Normal;
	D3DXVECTOR3 DensityGradient;
	for (UINT z = 0; z < VolumeSize; z++)
	{
		for (UINT y = 0; y < VolumeSize; y++)
		{
			for (UINT x = 0; x < VolumeSize; x++)
			{
				DensityGradient.x = GetDensity(x + 1, y, z, pData, VolumeSize) - GetDensity(x - 1, y, z, pData, VolumeSize) / heightAdjust;
				DensityGradient.y = GetDensity(x, y + 1, z, pData, VolumeSize) - GetDensity(x, y - 1, z, pData, VolumeSize) / heightAdjust;
				DensityGradient.z = GetDensity(x, y, z + 1, pData, VolumeSize) - GetDensity(x, y, z - 1, pData, VolumeSize) / heightAdjust;

				D3DXVec3Normalize(&Normal, &DensityGradient);
				SetNormal(Normal, x, y, z, pData, VolumeSize);
			}
		}
	}

	D3D11_TEXTURE3D_DESC desc;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.Depth = desc.Height = desc.Width = VolumeSize;
	desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	Check(pd3dDevice->CreateTexture3D(&desc, &InitData, &g_pNoiseVolume));

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory(&SRVDesc, sizeof(SRVDesc));
	SRVDesc.Format = desc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	SRVDesc.Texture3D.MipLevels = desc.MipLevels;
	SRVDesc.Texture3D.MostDetailedMip = 0;
	Check(pd3dDevice->CreateShaderResourceView(g_pNoiseVolume, &SRVDesc, &g_pNoiseVolumeRV));

	SafeDeleteArray(InitData.pSysMem);
}

void QuickDepthSort(DWORD* indices, float* depths, int lo, int hi)
{
	//  lo is the lower index, hi is the upper index
	//  of the region of array a that is to be sorted
	int i = lo, j = hi;
	float h;
	int index;
	float x = depths[(lo + hi) / 2];

	//  partition
	do
	{
		while (depths[i] > x) i++;
		while (depths[j] < x) j--;
		if (i <= j)
		{
			h = depths[i]; depths[i] = depths[j]; depths[j] = h;
			index = indices[i]; indices[i] = indices[j]; indices[j] = index;
			i++; j--;
		}
	} while (i <= j);

	//  recursion
	if (lo < j) QuickDepthSort(indices, depths, lo, j);
	if (i < hi) QuickDepthSort(indices, depths, i, hi);
}
void SoftParticle::SortParticleBuffer(D3DXVECTOR3 vEye, D3DXVECTOR3 vDir)
{
	if (!g_pParticleDepthArray || !g_pCPUParticleIndices)
		return;

	// assume vDir is normalized
	D3DXVECTOR3 vToParticle;
	//init indices and depths
	for (UINT i = 0; i < MAX_PARTICLES; i++)
	{
		g_pCPUParticleIndices[i] = i;
		vToParticle = g_pCPUParticles[i].Pos - vEye;
		g_pParticleDepthArray[i] = D3DXVec3Dot(&vDir, &vToParticle);
	}

	// Sort
	QuickDepthSort(g_pCPUParticleIndices, g_pParticleDepthArray, 0, MAX_PARTICLES - 1);
}

void SoftParticle::AdvanceParticles( double fTime, float fTimeDelta)
{
	//emit new particles
	static double fLastEmitTime = 0;
	static UINT iLastParticleEmitted = 0;

	if (!g_bAnimateParticles)
	{
		fLastEmitTime = fTime;
		return;
	}

	
	
	
	float fEmitRate = g_fEmitRate;
	
	float fParticleMaxSize = g_fParticleMaxSize;
	float fParticleMinSize = g_fParticleMinSize;

	if (PT_VOLUME_HARD == g_ParticleTechnique ||
		PT_VOLUME_SOFT == g_ParticleTechnique)
	{
		fEmitRate *= 3.0f;	//emit 1/3 less particles if we're doing volume
		fParticleMaxSize *= 1.5f;	//1.5x the max radius
		fParticleMinSize *= 1.5f;	//1.5x the min radius
	}

	UINT NumParticlesToEmit = (UINT)((fTime - fLastEmitTime) / fEmitRate);
	if (NumParticlesToEmit > 0)
	{
		for (UINT i = 0; i < NumParticlesToEmit; i++)
		{
			EmitParticle(&g_pCPUParticles[iLastParticleEmitted], i);
			iLastParticleEmitted = (iLastParticleEmitted + 1) % MAX_PARTICLES;
		}
		fLastEmitTime = fTime;
	}

	D3DXVECTOR3 vel;
	float lifeSq = 0;

	static float dir = 0.5f;
	
	for (UINT i = 0; i < MAX_PARTICLES; i++)
	{
		if (g_pCPUParticles[i].Life > -1)
		{
			// squared velocity falloff
			lifeSq = g_pCPUParticles[i].Life*g_pCPUParticles[i].Life;

			// Slow down by 50% as we age
			vel = g_pCPUParticles[i].Vel * (1 - 0.5f*lifeSq);
			
			vel.y += dir;	//(add some to the up direction, becuase of buoyancy)
			
			g_pCPUParticles[i].Pos += vel * fTimeDelta;
			g_pCPUParticles[i].Life += fTimeDelta / g_fParticleLifeSpan;
			g_pCPUParticles[i].Size = fParticleMinSize + (fParticleMaxSize - fParticleMinSize) * g_pCPUParticles[i].Life;

			if (g_pCPUParticles[i].Life > 0.99f)
				g_pCPUParticles[i].Life = -1;
		}
	}
}

void SoftParticle::UpdateParticleBuffers(ID3D11DeviceContext* context)
{

	context->UpdateSubresource(g_pParticleVB, NULL, NULL, g_pCPUParticles, 0, 0);
	context->UpdateSubresource(g_pParticleIB, NULL, NULL, g_pCPUParticleIndices, 0, 0);
}



