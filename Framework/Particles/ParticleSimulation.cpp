#include "Framework.h"
#include "ParticleSimulation.h"


ParticleSimulation::ParticleSimulation(ID3D11Device * device):
	device(device),
	StructuredBuffer(nullptr),
	StructuredBufferSRV(nullptr),
	StructuredBufferUAV(nullptr),
	updateCS(nullptr), type(ReadParticleType::Default),
	VS(nullptr), GS(nullptr), PS(nullptr), sampler(nullptr), particleTexture(nullptr), preivewVS(nullptr), maintainVS(nullptr)
	
{

	SafeRelease(sampler);

	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &sampler));
}

ParticleSimulation::~ParticleSimulation()
{
}

