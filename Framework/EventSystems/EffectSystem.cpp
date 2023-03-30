#include "Framework.h"
#include "EffectSystem.h"
#include "PhysicsSystem.h"
#include "Particles/Sparks.h"
#include "Particles/SoftParticle.h"

EffectSystem::EffectSystem(ID3D11Device * device)
	:device(device),
	position_StructuredBuffer(nullptr),
	position_StructuredBufferSRV(nullptr),
	position_StructuredBufferUAV(nullptr),
	/*position_StructuredBuffer{ nullptr,nullptr,nullptr,nullptr,nullptr,nullptr },
	position_StructuredBufferSRV{nullptr,nullptr,nullptr,nullptr,nullptr,nullptr },
	position_StructuredBufferUAV{ nullptr,nullptr,nullptr,nullptr,nullptr,nullptr },*/
	indirectBuffer{ nullptr,nullptr,nullptr },
	indirectBufferUAV{ nullptr,nullptr,nullptr },
	physics(nullptr)
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
			int temp = particles[index]->InitBodies(path);
			uint count = static_cast<Sparks*>(particles[index])->GetParticleCount();
			CreateIndirectIndex(index,count);
			physics->EffectIndex(temp, static_cast<Sparks*>(particles[index])->ID());
		}
		else  
		{
			auto spark = new Sparks(device, index);
			spark->SetParticleType(ReadParticleType::Spark);
			int temp =spark->InitBodies(path);
			uint count = spark ->GetParticleCount();
			CreateIndirectIndex(index, count);
			physics->EffectIndex(temp, spark->ID());
			
			particles.emplace_back(spark);
			
		}
	}
	break;
	case ReadParticleType::Blood:
		break;
	case ReadParticleType::Smoke:
		if (particles.size() > index)
		{
		
			
			particles[index]->InitBodies(path);
				
			
		}
		else
	    {		
			auto smoke = new SoftParticle(device, index);
			smoke->SetParticleType(ReadParticleType::Smoke);
			smoke->InitBodies(path);
		    particles.emplace_back(smoke);
		}

		break;

	}

}

void EffectSystem::Render(ID3D11DeviceContext * context,ID3D11ShaderResourceView* softParticleDepth)
{
	
	for (auto& particle : particles)
	{

		ID3D11ShaderResourceView* srv = nullptr;
		if (particle->GetParticleType() == ReadParticleType::Spark)
		{
			
			auto spark = static_cast<Sparks*>(particle);
			uint effectIndex = spark->ID();
			
			
				srv = position_StructuredBufferSRV;
				spark->Render(context, srv, indirectBuffer[effectIndex]);
			
			
		}
		else
		{
			srv = softParticleDepth;
			particle->Render(context, srv);
		}
		
		
		
	}
	
}

void EffectSystem::ResetBodies(const int & index)
{
	if (index<0||particles.size() <= static_cast<uint>(index))return;
	particles[index]->Reset(0);
	
}

void EffectSystem::CreateIndirectIndex(uint index, uint count)
{
	SafeRelease(indirectBuffer[index]);
	SafeRelease(indirectBufferUAV[index]);


	{
		UINT* pInitArgs = new UINT[4];

		pInitArgs[0] = count;
		pInitArgs[1] = 0;
		pInitArgs[2] = 0;
		pInitArgs[3] = 0;
		

		


			D3D11_SUBRESOURCE_DATA InitArgsData;
			InitArgsData.pSysMem = pInitArgs;
			InitArgsData.SysMemPitch = 0;
			InitArgsData.SysMemSlicePitch = 0;

			D3D11_BUFFER_DESC desc;
			desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
			desc.StructureByteStride = 0;
			desc.ByteWidth = 4 * sizeof(UINT);
			desc.Usage = D3D11_USAGE_DEFAULT;

				Check(device->CreateBuffer(&desc, &InitArgsData, &indirectBuffer[index]));




			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			ZeroMemory(&uavDesc, sizeof(uavDesc));
			uavDesc.Format = DXGI_FORMAT_R32_UINT;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.Flags = 0;
			uavDesc.Buffer.NumElements = 4;
			Check(device->CreateUnorderedAccessView(indirectBuffer[index], &uavDesc, &indirectBufferUAV[index]));

		

		SafeDelete(pInitArgs);
	}
}

//void EffectSystem::Swap()
//{
//
//
//	for (uint i = 0; i < 3; i++)
//	{
//		ID3D11Buffer* pTempBuffer = position_StructuredBuffer[i];
//		ID3D11UnorderedAccessView* pTempUAV = position_StructuredBufferUAV[i];
//		ID3D11ShaderResourceView* p_TempSRV = position_StructuredBufferSRV[i];
//		position_StructuredBuffer[i] = position_StructuredBuffer[i+3];
//		position_StructuredBufferUAV[i] = position_StructuredBufferUAV[i + 3];
//		position_StructuredBufferSRV[i] = position_StructuredBufferSRV[i + 3];
//		position_StructuredBuffer[i + 3] = pTempBuffer;
//		position_StructuredBufferUAV[i + 3] = pTempUAV;
//		position_StructuredBufferSRV[i + 3] = p_TempSRV;
//	}
//
//	
//}

void EffectSystem::CreatePositionBuffer(ID3D11Device * device)
{

	const uint& maxHeight = 10;
	SafeRelease(position_StructuredBuffer);
	SafeRelease(position_StructuredBufferSRV);
	SafeRelease(position_StructuredBufferUAV);


	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = 3;
	desc.Height = maxHeight;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;

	Vector4 temp[30];
	for (uint i = 0; i < maxHeight * 3; i++)
	{
		temp[i] = Vector4(0, 0, 0, 0);
	}



	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = temp;
	subResource.SysMemPitch = sizeof(Vector4) * 3;
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
	sbUAVDesc.Buffer.NumElements = maxHeight * 3;
	sbUAVDesc.Format = desc.Format;
	sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	Check(device->CreateUnorderedAccessView(position_StructuredBuffer, &sbUAVDesc, &position_StructuredBufferUAV));
	////////////////////////////////////////////////////////////////////////////
	

	
	
	

	
	
}
