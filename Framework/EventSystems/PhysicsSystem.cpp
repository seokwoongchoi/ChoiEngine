#include "Framework.h"
#include "PhysicsSystem.h"
#include "Core/D3D11/D3D11_Helper.h"
#include "Animator.h"
#include "EffectSystem.h"
#include "GBuffer/LightManager.h"
extern class LightManager Lighting;
PhysicsSystem::PhysicsSystem(ID3D11Device* device, Animator* animator, EffectSystem* effects):
	modelData_StructuredBuffer(nullptr),
	modelData_StructuredBufferSRV(nullptr),
	modelData_StructuredBufferUAV(nullptr),
	CollisonCS(nullptr), csAnimBoneBoxData(nullptr),
	copy_StructuredBuffer(nullptr),copy_StructuredBufferUAV(nullptr),// copyBuffer(nullptr),
	drawBuffer(nullptr), animator(animator), effects(effects), IsHitted(false)
	
{

	ID3DBlob* ShaderBlob = nullptr;
	auto& path = "../_Shaders/ComputeShaders/CollisonCS.hlsl";
	auto& entryPoint = "CS";
	auto& shaderModel = "cs_5_0";
	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));


	Check(device->CreateComputeShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&CollisonCS
	));

	SafeRelease(ShaderBlob);

	CreateAnimBoneBuffer(device);
	CreateCopyBuffer(device);


	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(drawDesc) ;
	Check(device->CreateBuffer(&bufferDesc, NULL, &drawBuffer));

	uavArray.resize(5,nullptr);
	
}


PhysicsSystem::~PhysicsSystem()
{
}

void PhysicsSystem::CollisonSword(const uint & attackerIndex, const uint& targetIndex)
{
	animator->tweenData[targetIndex].state= ActorState::BlockingReaction;
	uint attackActorIndex,instanceIndex;
	animator->GetActorIndex(attackerIndex, attackActorIndex, instanceIndex);
	animator->GetFoward(&dir, attackActorIndex, instanceIndex);

	uint targetActorIndex, targetinstanceIndex;;
	animator->GetActorIndex(targetIndex, targetActorIndex, targetinstanceIndex);
	//맞은 타켓이 밀려남
	animator->SetPosition(dir, 30.0f, targetActorIndex, targetinstanceIndex);

	
	if (swordEffectIndex > -1)
	{
		effects->ResetBodies(swordEffectIndex);
		Vector3 targetpos;
		animator->GetPosition(&targetpos, targetActorIndex, targetinstanceIndex);
		Lighting.AddPointLight(targetpos, 20.0f, Vector3(0.8f, 0.6f, 0.6f), 1.5f, false, true);
	}
}

void PhysicsSystem::CollisonBody(const uint & attackerIndex, const uint& targetIndex)
{
	if (bodyEffectIndex > -1)
	{
		
		effects->ResetBodies(bodyEffectIndex);

		uint targetActorIndex,instanceIndex;
		animator->GetActorIndex(targetIndex, targetActorIndex, instanceIndex);
		Vector3 targetpos;
		animator->GetPosition(&targetpos, targetActorIndex, instanceIndex);
		Lighting.AddPointLight(targetpos, 20.0f, Vector3(0.8f, 0.2f, 0.1f), 1.5f, false, true);
	}

}

void PhysicsSystem::CollisonHead(const uint & attackerIndex, const uint& targetIndex)
{
	if (headEffectIndex > -1)
	{
		
		effects->ResetBodies(headEffectIndex);

		uint targetActorIndex,instanceIndex;
		animator->GetActorIndex(targetIndex, targetActorIndex, instanceIndex);
		Vector3 targetpos;
		animator->GetPosition(&targetpos, targetActorIndex, instanceIndex);

		Lighting.AddPointLight(targetpos, 20.0f, Vector3(0.8f, 0.2f, 0.1f), 1.5f, false, true);
	}
}

void PhysicsSystem::CollisonBodyBody(const uint & attackerIndex, const uint& targetIndex)
{
	uint attackActorIndex,attackInstanceIndex;
	animator->GetActorIndex(attackerIndex, attackActorIndex, attackInstanceIndex);
	Vector3 attackpos;
	animator->GetPosition(&attackpos, attackActorIndex, attackInstanceIndex);

	uint targetActorIndex,targetInstanceIndex;
	animator->GetActorIndex(targetIndex, targetActorIndex, targetInstanceIndex);
	Vector3 targetpos;
	animator->GetPosition(&targetpos, targetActorIndex, targetInstanceIndex);

	dir = targetpos - attackpos;
	D3DXVec3Normalize(&dir, &dir);
	animator->SetPosition(dir, -0.3f, targetActorIndex, targetInstanceIndex);
}

bool PhysicsSystem::CheckCollision()
{
	auto& tweenData = animator->tweenData;

	auto find = find_if(tweenData.begin(), tweenData.end(), [](const TweenData& desc)
	{
		return desc.state == ActorState::Attack || desc.state == ActorState::Attack2 ||
			desc.state == ActorState::Move || desc.state == ActorState::Run;
	});

	if (find == tweenData.end()) return false;

	if (find->state == ActorState::Move || find->state == ActorState::Run)
	{
		uint index = find->index;
		uint quadTreeID = find->quadTreeID;
		auto check2 = any_of(tweenData.begin(), tweenData.end(), [&index, &quadTreeID](const TweenData& desc)
		{
			if (index != desc.index)
			{
				int temp = desc.quadTreeID - quadTreeID;
				if (temp < 0)
				{
					if (temp == -2 ||
						temp == -1026 ||
						temp == -1024 ||
						temp == -1022)
						return true;

				}
				else
				{
					if (temp == 0 ||
						temp == 2 ||
						temp == 1026 ||
						temp == 1024 ||
						temp == 1022)
						return true;
				}

			}
			return false;
		});


		if (check2 == false)
			return false;
	}

	return true;

}

void PhysicsSystem::Collison(const uint & skeletalCount)
{

	static bool bFisrt = true;
	if (bFisrt)
	{
		for (uint i = 0; i < 15; i++)
		{
			Result[i] = Vector4(-1,-1,-1,-1);
			
		}
		bFisrt = false;
		return;
	}

	auto tweenDesc = &animator->tweenData[0];
	auto tweenIndex = &animator->tweenDesc[0];

	uint targetIndex = 0;
	
	for (uint i = 0; i < skeletalCount; i++)
	{
		uint attackerIndex = tweenIndex[i].index;

		if (tweenDesc[attackerIndex].state == ActorState::Die)
			continue;

		if (Result[i].w > -1)
		{
			if (countResult > 0 && lastResult == Result[i].w)
			{
				countResult = 0;
				continue;
			}


			targetIndex = tweenIndex[static_cast<uint>(Result[i].w)].index;

			if (tweenDesc[targetIndex].state == ActorState::Die)continue;

			tweenDesc[targetIndex].state = ActorState::StandingReaction;
			cout << to_string(attackerIndex) + " Actor Collison  :" + to_string(targetIndex) + " actor" << endl;

			CollisonBodyBody(attackerIndex, targetIndex);
			

			lastResult = static_cast<uint>(Result[i].w);
			countResult++;
		}

		if (animator->IsAtacking(attackerIndex) == false)
		{
			continue;
		}


		if (Result[i].x > -1 )
		{
			if (countResult > 0 && lastResult == Result[i].x)
			{
				countResult = 0;
				continue;
			}

			targetIndex = tweenIndex[static_cast<uint>(Result[i].x)].index;
			if (tweenDesc[targetIndex].Curr.Clip == static_cast<uint>(ActorState::Die))
			{
				continue;
			}
			else if (tweenDesc[targetIndex].Curr.Clip == static_cast<uint>(ActorState::Move))
			{
				
				CollisonSword(attackerIndex, targetIndex);
				//Result[i].z = -1;
				continue;
			}

			cout << to_string(attackerIndex) + " Actor Hit the  :" + to_string(targetIndex) + " body" << endl;
			tweenDesc[targetIndex].state = ActorState::StandingReaction;
			CollisonBody(attackerIndex, targetIndex);

			lastResult = static_cast<uint>(Result[i].x);
			countResult++;
		}
		if (Result[i].y > -1 )
		{
		
			if (countResult > 0 && lastResult == Result[i].y)
			{
				countResult = 0;
				continue;
			}

			targetIndex = tweenIndex[static_cast<uint>(Result[i].y)].index;
			if (tweenDesc[targetIndex].Curr.Clip == static_cast<uint>(ActorState::Move) ||
				tweenDesc[targetIndex].Curr.Clip == static_cast<uint>(ActorState::BlockingReaction)
				)
			{
				continue;
			}
			hittedActorIndex = attackerIndex;

		
			cout << to_string(attackerIndex) + " Actor Hit the  :" + to_string(targetIndex) + " head" << endl;
			
			hittedIndex = targetIndex;
			CollisonHead(attackerIndex, targetIndex);

			lastResult = static_cast<uint>(Result[i].y);
			countResult++;

			IsHitted = true;

		}
		if (Result[i].z > -1)
		{
			if (countResult > 0 && lastResult == Result[i].z)
			{
				countResult = 0;
				continue;
			}

			targetIndex = tweenIndex[static_cast<uint>(Result[i].z)].index;
			if (tweenDesc[targetIndex].state == ActorState::Die)
			{
				continue;
			}
			cout << to_string(attackerIndex) + " Actor Hit the  :" + to_string(targetIndex) + " sword" << endl;
			if (tweenDesc[targetIndex].Curr.Clip == static_cast<uint>(ActorState::Move))
			{
				
				CollisonSword(attackerIndex, targetIndex);
			}
				
	
			lastResult = static_cast<uint>(Result[i].z);
			countResult++;
		}

		
	}



}

void PhysicsSystem::Stop()
{
	if (IsHitted)
	{
		Time::Get()->Stop();
		animator->tweenData[hittedActorIndex].IsStopped = true;
	}
}




void PhysicsSystem::Compute(ID3D11DeviceContext * context, ID3D11UnorderedAccessView* effectPositionUAV, const uint& skeletalCount, const uint& staticCount)
{
	

	if (IsHitted)
	{
		if (Mouse::Get()->Down(1))
		{

			if (Time::Get()->Stopped())
			{
				if (hittedIndex > -1)
				{
					animator->tweenData[hittedActorIndex].IsStopped = false;
					animator->tweenData[hittedIndex].state = ActorState::Die;
				}
				Time::Get()->Start();
				IsHitted = false;
				return;
			}
		}
		return;
	}
	if (skeletalCount < 2 || CheckCollision() == false) return;


	{
		drawDesc.drawCount = skeletalCount + staticCount;
		drawDesc.skeletalCount = skeletalCount;
		drawDesc.staticCount = staticCount;
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(drawBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &drawDesc, sizeof(drawDesc));
		context->Unmap(drawBuffer, 0);
		ID3D11Buffer* bufferArray[1] = { drawBuffer };
		context->CSSetConstantBuffers(0, 1, bufferArray);

		ID3D11ShaderResourceView* srvArray[1] = { modelData_StructuredBufferSRV };
		context->CSSetShaderResources(0, 1, srvArray);


		if (uavArray[0] == nullptr)
		{
			//uavArray[0] = indirectBufferUAV;
			uavArray[0] = copy_StructuredBufferUAV;
			uavArray[1] = effectPositionUAV;
		}

		context->CSSetUnorderedAccessViews(0, 5, uavArray.data(), 0);
		context->CSSetShader(CollisonCS, nullptr, 0);
		context->Dispatch(skeletalCount, 1, 1);


		{
			
			
			//for (uint i = 0; i < 4; i++)
			{
				context->CopyResource(copyBuffer, copy_StructuredBuffer);
				//context->CopyStructureCount(copyBuffer,0, indirectBufferUAV[i]);
				D3D11_MAPPED_SUBRESOURCE subResource;
				context->Map(copyBuffer, 0, D3D11_MAP_READ, 0, &subResource);
				{

					memcpy(&Result, subResource.pData, sizeof(Vector4) * skeletalCount);
				}
				context->Unmap(copyBuffer, 0);
			}
			

		}
		ZeroMemory(&bufferArray, sizeof(bufferArray));
		context->CSSetConstantBuffers(0, 1, bufferArray);
		context->CSSetShader(nullptr, nullptr, 0);
		ZeroMemory(srvArray, sizeof(srvArray));
		context->CSSetShaderResources(0, 1, srvArray);
		//	ZeroMemory(uavArray, sizeof(uavArray));
		ID3D11UnorderedAccessView* nullUavArray[5] = { nullptr,nullptr,nullptr,nullptr,nullptr };
		context->CSSetUnorderedAccessViews(0, 5, nullUavArray, nullptr);

	}

	Collison(skeletalCount);


}




void PhysicsSystem::SetIndirectUAV(const int & effectIndex)
{
	uavArray[effectIndex + 2] = effects->IndirectUAV()[effectIndex];
}

void PhysicsSystem::CreateCopyBuffer(ID3D11Device * device)
{

	UINT outSize = 15;
	
	//ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));


	SafeRelease(copy_StructuredBuffer);
	SafeRelease(copy_StructuredBufferUAV);

	

	// Create Structured Buffer
	D3D11_BUFFER_DESC sbDesc;


	sbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS ;
	sbDesc.CPUAccessFlags = 0;
	sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbDesc.StructureByteStride = sizeof(Vector4);
	sbDesc.ByteWidth = sizeof(Vector4)*outSize;
	sbDesc.Usage = D3D11_USAGE_DEFAULT;
	Check(device->CreateBuffer(&sbDesc, nullptr, &copy_StructuredBuffer));

	
	// create the UAV for the structured buffer
	D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
	sbUAVDesc.Buffer.FirstElement = 0;
	sbUAVDesc.Buffer.Flags = 0;
	sbUAVDesc.Buffer.NumElements = outSize;
	sbUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	Check(device->CreateUnorderedAccessView(copy_StructuredBuffer, &sbUAVDesc, &copy_StructuredBufferUAV));



	

	

	D3D11_BUFFER_DESC desc;
	copy_StructuredBuffer->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	Check(device->CreateBuffer(&desc, nullptr, &copyBuffer));
}

void PhysicsSystem::CreateAnimBoneBuffer(ID3D11Device * device)
{
	//////////////////////////////////////////////////////////////////////////////
	const uint& maxHeight = 10;
	SafeRelease(modelData_StructuredBuffer);
	SafeRelease(modelData_StructuredBufferSRV);
	SafeRelease(modelData_StructuredBufferUAV);


	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = 12;
	desc.Height = maxHeight;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;

	UINT outSize = maxHeight;
	if (csAnimBoneBoxData == nullptr)
	{
		csAnimBoneBoxData = new CS_AnimBoneBoxDesc[outSize];

		for (UINT i = 0; i < outSize; i++)
		{
			for (uint b = 0; b < 3; b++)
				D3DXMatrixIdentity(&csAnimBoneBoxData[i].boxes[b]);

		}
	}


	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = csAnimBoneBoxData;
	subResource.SysMemPitch = sizeof(CS_AnimBoneBoxDesc);
	subResource.SysMemSlicePitch = sizeof(CS_AnimBoneBoxDesc) * maxHeight;

	Check(device->CreateTexture2D(&desc, &subResource, &modelData_StructuredBuffer));

	//Create SRV
	{


		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = desc.Format;

		Check(device->CreateShaderResourceView(modelData_StructuredBuffer, &srvDesc, &modelData_StructuredBufferSRV));
	}


	// create the UAV for the structured buffer
	D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
	sbUAVDesc.Buffer.FirstElement = 0;
	sbUAVDesc.Buffer.Flags = 0;
	sbUAVDesc.Buffer.NumElements = maxHeight * 12;
	sbUAVDesc.Format = desc.Format;
	sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	Check(device->CreateUnorderedAccessView(modelData_StructuredBuffer, &sbUAVDesc, &modelData_StructuredBufferUAV));

	SafeDeleteArray(csAnimBoneBoxData);
}
