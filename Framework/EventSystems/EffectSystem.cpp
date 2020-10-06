#include "Framework.h"
#include "EffectSystem.h"
#include "Particles/Sparks.h"

EffectSystem::EffectSystem(ID3D11Device * device)
	:device(device), bLoaded(false), 
	position_StructuredBuffer(nullptr),
	position_StructuredBufferSRV(nullptr), 
	position_StructuredBufferUAV(nullptr)
{									

	
	CreatePositionBuffer(device);

	particles.clear();
	particles.shrink_to_fit();

}

EffectSystem::~EffectSystem()
{
}

void EffectSystem::LoadParticle(const uint & index, const wstring & path, const ReadParticleType & particleType)
{
	
	switch (particleType)
	{

	case ReadParticleType::Spark:
	{
	    if(particles.size()>index)
		{
			static_cast<Sparks*>(particles[index])->InitBodies(path);
		}
		else  
		{
			auto spark = new Sparks(device, index);
			spark->InitBodies(path);
			particles.emplace_back(spark);
			
		}
	}
	break;
	case ReadParticleType::Blood:
		break;
	case ReadParticleType::Smoke:
		break;

	}

}

void EffectSystem::Render(ID3D11DeviceContext * context)
{
	if (!bLoaded)return;
	
	for (auto& particle : particles)
	{
		
		particle->Render(context, position_StructuredBufferSRV);
	}
	
}

void EffectSystem::ResetBodies(const uint & index,const uint& effectCount)
{
	if (particles.size() <= index)return;
	particles[index]->Reset(effectCount);
	bLoaded = true;
}

void EffectSystem::CreatePositionBuffer(ID3D11Device * device)
{
	//////////////////////////////////////////////////////////////////////////////
	const uint& maxHeight = 10;
	SafeRelease(position_StructuredBuffer);
	SafeRelease(position_StructuredBufferSRV);
	SafeRelease(position_StructuredBufferUAV);


	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = 2;
	desc.Height = maxHeight;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;

	Vector4 temp[20];
	for (uint i = 0; i < maxHeight*2; i++)
	{
		temp[i] = Vector4(0, 0, 0, 0);
	}



	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = temp;
	subResource.SysMemPitch = sizeof(Vector4)*2;
	subResource.SysMemSlicePitch = 0;

	Check(device->CreateTexture2D(&desc, &subResource, &position_StructuredBuffer));

	//Create SRV
	{


		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = desc.Format;

		Check(device->CreateShaderResourceView(position_StructuredBuffer, &srvDesc, &position_StructuredBufferSRV));
	}


	// create the UAV for the structured buffer
	D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
	sbUAVDesc.Buffer.FirstElement = 0;
	sbUAVDesc.Buffer.Flags = 0;
	sbUAVDesc.Buffer.NumElements = maxHeight*2;
	sbUAVDesc.Format = desc.Format;
	sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	Check(device->CreateUnorderedAccessView(position_StructuredBuffer, &sbUAVDesc, &position_StructuredBufferUAV));

}
