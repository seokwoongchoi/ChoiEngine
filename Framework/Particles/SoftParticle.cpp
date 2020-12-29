#include "Framework.h"
#include "SoftParticle.h"
#include "Core/D3D11/D3D11_Helper.h"
float articleVel = 3.0f;


SoftParticle::SoftParticle(ID3D11Device * device, uint ID)
	:ParticleSimulation(device), CB_PSBuffer(nullptr), CB_GSBuffer(nullptr),
	blendState(nullptr),sampLinearClamp(nullptr),sampVolume(nullptr), sampPoint(nullptr),
    depthState(nullptr),CPUParticles(nullptr),  CPUParticleIndices(nullptr),ParticleDepthArray(nullptr),
    ParticleVB(nullptr),ParticleIB(nullptr),VS(nullptr),GS(nullptr),PS(nullptr)
{
	id = ID;
	

	
	CreateParticleBuffers(device);

	CreateShaders(device);
	CreateBuffers(device);

	smokevol = new Texture();
	smokevol->Load(device, L"SoftParticle/smokevol1.dds");
	gradient = new Texture();
	gradient->Load(device, L"SoftParticle/colorgradient.dds");

	D3DXMatrixIdentity(&gsDesc.World);
	D3DXMatrixScaling(&gsDesc.World, 10, 10, 10);

}

SoftParticle::~SoftParticle()
{
}

void SoftParticle::Update(ID3D11DeviceContext* context)
{
	Vector3 Eye= GlobalData::Position();
	Vector3 Dir;
	D3DXVec3Normalize(&Dir, &GlobalData::Forward());

	simulateDesc.runningTime += Time::Delta();
	AdvanceParticles(simulateDesc.runningTime, Time::Delta());
	SortParticleBuffer(Eye, Dir);
	UpdateParticleBuffers(context);

}

void SoftParticle::Render(ID3D11DeviceContext* context, ID3D11ShaderResourceView* srv)
{
	Update(context);
	
		
	View = GlobalData::GetView();
	Proj = GlobalData::GetProj();
	
	D3DXMatrixTranspose(&gsDesc.WVP, &(gsDesc.World * View*Proj));
	D3DXMatrixTranspose(&gsDesc.World, &gsDesc.World);
	D3DXMatrixInverse(&gsDesc.InvView, nullptr, &View);
	D3DXMatrixTranspose(&gsDesc.InvView, &gsDesc.InvView);
	D3DXMatrixInverse(&psDesc.InvProj, nullptr, &Proj);
	D3DXMatrixTranspose(&psDesc.InvProj, &psDesc.InvProj);

	
	//D3DXVec3Normalize((Vector3*)&psDesc.lightDirectional, &GlobalData::LightDirection());

	{

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(CB_GSBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &gsDesc, sizeof(CB_GS));
		context->Unmap(CB_GSBuffer, 0);
		context->GSSetConstantBuffers(0, 1, &CB_GSBuffer);
	}
	{

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(CB_PSBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &psDesc, sizeof(CB_PS));
		context->Unmap(CB_PSBuffer, 0);
		context->PSSetConstantBuffers(1, 1, &CB_PSBuffer);
	}

	ID3D11BlendState* pPrevBlendState;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);

	ID3D11DepthStencilState* PrevDepthState;
	UINT PrevStencil;
	context->OMGetDepthStencilState(&PrevDepthState, &PrevStencil);



	context->OMSetDepthStencilState(depthState, 2);
	context->OMSetBlendState(blendState, prevBlendFactor, prevSampleMask);

	ID3D11ShaderResourceView* srvs[2] = { gradient->SRV(),  nullptr };
	context->GSSetShaderResources(0, 1, &srvs[0]);
	srvs[0] = smokevol->SRV();
	srvs[1] = srv;
	context->PSSetShaderResources(1, 2, srvs);

	context->VSSetShader(VS, nullptr, 0);
	context->GSSetShader(GS, nullptr, 0);
	context->PSSetShader(PS, nullptr, 0);


	
	ID3D11SamplerState* sampleState[2] = { sampVolume ,sampPoint };
	context->PSSetSamplers(1, 2, sampleState);
	context->GSSetSamplers(0, 1, &sampLinearClamp);
		
	

	ID3D11Buffer *pBuffers[1] = { ParticleVB };
	UINT stride[1] = { sizeof(PARTICLE_VERTEX) };
	UINT offset[1] = { 0 };
	context->IASetVertexBuffers(0, 1, pBuffers, stride, offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->IASetIndexBuffer(ParticleIB, DXGI_FORMAT_R32_UINT, 0);

		
	context->DrawIndexed(MAX_PARTICLES, 0, 0);

	context->VSSetShader(nullptr, nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);

	ZeroMemory(&sampleState, sizeof(sampleState));
	context->PSSetSamplers(0, 2, sampleState);
	context->GSSetSamplers(0, 1, &sampleState[0]);
	ZeroMemory(&pBuffers, sizeof(pBuffers));
	context->GSSetConstantBuffers(0, 1, &pBuffers[0]);
	context->PSSetConstantBuffers(0, 1, &pBuffers[0]);
	context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
	context->OMSetDepthStencilState(PrevDepthState, PrevStencil);


	ZeroMemory(&srvs, sizeof(srvs));
	context->GSSetShaderResources(0, 1, &srvs[0]);
	context->PSSetShaderResources(0, 2, srvs);

	
	
}
void SoftParticle::PreviewRender(ID3D11DeviceContext * context, const Matrix & view, const Matrix & proj)
{
	Update(context);
	
	View = view;
	Proj = proj;

	D3DXMatrixTranspose(&gsDesc.WVP, &(gsDesc.World * View*Proj));
	
	
	D3DXMatrixInverse(&gsDesc.InvView, NULL, &View);
	D3DXMatrixInverse(&psDesc.InvProj, NULL, &Proj);
	
	//psDesc.lightDirectional = Vector4(GlobalData::LightDirection());


	// Render the particles

	ID3D11Buffer *pBuffers[1] = { ParticleVB };
	UINT stride[1] = { sizeof(PARTICLE_VERTEX) };
	UINT offset[1] = { 0 };
	context->IASetVertexBuffers(0, 1, pBuffers, stride, offset);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->IASetIndexBuffer(ParticleIB, DXGI_FORMAT_R32_UINT, 0);

	context->DrawIndexed(MAX_PARTICLES, 0, 0);
	// unbind the depth from the resource so we can set it as depth next time around
	ID3D11ShaderResourceView* Nulls[2] = { NULL,NULL };
	context->PSSetShaderResources(0, 2, Nulls);
}
void SoftParticle::Reset(const uint & drawCount)
{
}
void SoftParticle::CreateShaders(ID3D11Device * device)
{
	SafeRelease(VS);
	SafeRelease(GS);
	SafeRelease(PS);

	ID3DBlob* ShaderBlob = nullptr;
	auto path = "../_Shaders/Particles/SoftParticle/SoftParticles.hlsl";
	auto entryPoint = "VS";
	auto shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &VS));

	inputLayout = make_shared<InputLayout>();
	inputLayout->Create(device, ShaderBlob);
	SafeRelease(ShaderBlob);


	


		entryPoint = "GS";
		shaderModel = "gs_5_0";
		Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
		Check(device->CreateGeometryShader(ShaderBlob->GetBufferPointer(),
			ShaderBlob->GetBufferSize(), nullptr, &GS));
		SafeRelease(ShaderBlob);
	
	
		entryPoint = "PS";
		shaderModel = "ps_5_0";
		Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
		Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
			ShaderBlob->GetBufferSize(), nullptr, &PS));
		SafeRelease(ShaderBlob);
	


}
void SoftParticle::CreateBuffers(ID3D11Device * device)
{
	/////////////////////////////////////////////////////////////////////////////////////

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(CB_GS);
	Check(device->CreateBuffer(&bufferDesc, NULL, &CB_GSBuffer));
	bufferDesc.ByteWidth = sizeof(CB_PS);
	Check(device->CreateBuffer(&bufferDesc, NULL, &CB_PSBuffer));


	/////////////////////////////////////////////////////

	SafeRelease(blendState);
	D3D11_BLEND_DESC descBlend;
	descBlend.AlphaToCoverageEnable = FALSE;
	descBlend.IndependentBlendEnable = FALSE;
	D3D11_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		TRUE,
			D3D11_BLEND_SRC_ALPHA,  D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, //srcBlend,descBlend,BlendOp
			D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,//srcBlendAlpha,destBlendAlpha,BlendOpAlpha
			D3D11_COLOR_WRITE_ENABLE_ALL,//rendertargetWriteMask
	};
	for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		descBlend.RenderTarget[i] = defaultRenderTargetBlendDesc;

	Check(device->CreateBlendState(&descBlend, &blendState));




	////////////////////////////////////////////////////////////////////////////////////////y
	D3D11_DEPTH_STENCIL_DESC descDepth;
	descDepth.DepthEnable = TRUE;
	descDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	descDepth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	descDepth.StencilEnable = false;
	descDepth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	descDepth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	const D3D11_DEPTH_STENCILOP_DESC stencilMarkOp = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_LESS_EQUAL };
	descDepth.FrontFace = stencilMarkOp;
	descDepth.BackFace = stencilMarkOp;

	Check(device->CreateDepthStencilState(&descDepth, &depthState));

	/////////////////////////////////////////////////////////////////////////////////////////

	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.MaxAnisotropy = 16;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &sampLinearClamp));



	samDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	Check(device->CreateSamplerState(&samDesc, &sampVolume));


	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	Check(device->CreateSamplerState(&samDesc, &sampPoint));


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
	Check(pd3dDevice->CreateBuffer(&vbdesc, NULL, &ParticleVB));

	vbdesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
	vbdesc.ByteWidth = MAX_PARTICLES * sizeof(DWORD);
	Check(pd3dDevice->CreateBuffer(&vbdesc, NULL, &ParticleIB));

	CPUParticles = new PARTICLE_VERTEX[MAX_PARTICLES];
	
	for (UINT i = 0; i < MAX_PARTICLES; i++)
	{
		CPUParticles[i].Life = -1;	//kill all particles
	}

	CPUParticleIndices = new DWORD[MAX_PARTICLES];
	
	ParticleDepthArray = new float[MAX_PARTICLES];

	
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
	pParticle->Vel *= articleVel;

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
	if (!ParticleDepthArray || !CPUParticleIndices)
		return;

	// assume vDir is normalized
	D3DXVECTOR3 vToParticle;
	//init indices and depths
	for (UINT i = 0; i < MAX_PARTICLES; i++)
	{
		CPUParticleIndices[i] = i;
		vToParticle = CPUParticles[i].Pos - vEye;
		ParticleDepthArray[i] = D3DXVec3Dot(&vDir, &vToParticle);
	}

	// Sort
	QuickDepthSort(CPUParticleIndices, ParticleDepthArray, 0, MAX_PARTICLES - 1);
}

void SoftParticle::AdvanceParticles( double fTime, float fTimeDelta)
{
	//emit new particles
	static double fLastEmitTime = 0;
	static UINT iLastParticleEmitted = 0;

	
	
	float fEmitRate = simulateDesc.EmitRate;
	
	float fParticleMaxSize = simulateDesc.ParticleMaxSize;
	float fParticleMinSize = simulateDesc.ParticleMinSize;


	UINT NumParticlesToEmit = (UINT)((fTime - fLastEmitTime) / fEmitRate);
	if (NumParticlesToEmit > 0)
	{
		for (UINT i = 0; i < NumParticlesToEmit; i++)
		{
			EmitParticle(&CPUParticles[iLastParticleEmitted], i);
			iLastParticleEmitted = (iLastParticleEmitted + 1) % MAX_PARTICLES;
		}
		fLastEmitTime = fTime;
	}

	D3DXVECTOR3 vel;
	float lifeSq = 0;

	static float dir = 0.5f;
	
	for (UINT i = 0; i < MAX_PARTICLES; i++)
	{
		if (CPUParticles[i].Life > -1)
		{
			// squared velocity falloff
			lifeSq = CPUParticles[i].Life*CPUParticles[i].Life;

			// Slow down by 50% as we age
			vel = CPUParticles[i].Vel * (1 - 0.5f*lifeSq);
			
			vel.y += dir;	//(add some to the up direction, becuase of buoyancy)
			
			CPUParticles[i].Pos += vel * fTimeDelta;
			CPUParticles[i].Life += fTimeDelta / simulateDesc.ParticleLifeSpan;
			CPUParticles[i].Size = fParticleMinSize + (fParticleMaxSize - fParticleMinSize) * CPUParticles[i].Life;

			if (CPUParticles[i].Life > 0.99f)
				CPUParticles[i].Life = -1;
		}
	}
}

void SoftParticle::UpdateParticleBuffers(ID3D11DeviceContext* context)
{

	context->UpdateSubresource(ParticleVB, NULL, NULL, CPUParticles, 0, 0);
	context->UpdateSubresource(ParticleIB, NULL, NULL, CPUParticleIndices, 0, 0);
}



