#include "Framework.h"
#include "PhysicsSystem.h"
#include "Core/D3D11/D3D11_Helper.h"
#include "Animator.h"
#include "EffectSystem.h"
PhysicsSystem::PhysicsSystem(ID3D11Device* device, Animator* animator, EffectSystem* effects):
	modelData_StructuredBuffer(nullptr),
	modelData_StructuredBufferSRV(nullptr),
	modelData_StructuredBufferUAV(nullptr),
	CollisonCS(nullptr), csAnimBoneBoxData(nullptr),
	copy_StructuredBuffer(nullptr),copy_StructuredBufferUAV(nullptr), copyBuffer(nullptr),
	drawBuffer(nullptr), animator(animator), effects(effects), IsHitted(false), effectCounts{}
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
}


PhysicsSystem::~PhysicsSystem()
{
}

void PhysicsSystem::ComputeEditor(ID3D11DeviceContext * context, ID3D11UnorderedAccessView * effectPositionUAV, const uint & skeletalCount, const uint & staticCount)
{
	if (IsHitted)
	{
		if (Mouse::Get()->Down(0))
		{
			if (Time::Get()->Stopped())
			{
				if (hittedIndex > -1)
				{
					animator->tweenData[hittedIndex].IsStopped = false;
					animator->tweenData[hittedIndex].state = ActorState::Die;
				}
				Time::Get()->Start();
				IsHitted = false;
				return;
			}
		}
		return;
	}
	if (skeletalCount < 2 ) return;

	auto& tweenVector = animator->tweenData;
	
	auto Check = all_of(tweenVector.begin(), tweenVector.end(), [](const TweenData& desc)
	{
		return desc.state != ActorState::Attack;
	});

	if (Check)
		return;

	
		
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

		ID3D11UnorderedAccessView* uavArray[2] = { copy_StructuredBufferUAV ,effectPositionUAV };
		context->CSSetUnorderedAccessViews(0, 2, uavArray, nullptr);
		context->CSSetShader(CollisonCS, nullptr, 0);
		context->Dispatch(skeletalCount , 1, 1);

		//INT64 currentTime;
		//QueryPerformanceCounter((LARGE_INTEGER *)&currentTime);
		//if (chrono::_Is_even<INT64>(currentTime))
		{
			
			  context->CopyResource(copyBuffer, copy_StructuredBuffer);
			  {
			      D3D11_MAPPED_SUBRESOURCE subResource;
			      context->Map(copyBuffer, 0, D3D11_MAP_READ, 0, &subResource);
			      {
			      	memcpy(&Result, subResource.pData, sizeof(Vector4) *skeletalCount);
			      }
			      context->Unmap(copyBuffer, 0);
			  }
		}

		ZeroMemory(&bufferArray, sizeof(bufferArray));
		context->CSSetConstantBuffers(0, 1, bufferArray);
		context->CSSetShader(nullptr, nullptr, 0);
		ZeroMemory(srvArray, sizeof(srvArray));
		context->CSSetShaderResources(0, 1, srvArray);
		ZeroMemory(uavArray, sizeof(uavArray));
		context->CSSetUnorderedAccessViews(0, 2, uavArray, nullptr);

	

	
		
		static bool bFisrt = true;

		if (bFisrt)
		{
			for (uint i = 0; i < 10; i++)
			{
				Result[i] = Vector4(-1, -1, -1, -1);
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
				targetIndex = tweenIndex[ static_cast<uint>(Result[i].w)].index;

				if (tweenDesc[targetIndex].state == ActorState::Die)continue;

				tweenDesc[targetIndex].state = ActorState::StandingReaction;
				cout << to_string(attackerIndex) + " Actor Collison  :" + to_string(targetIndex) + " actor" << endl;
			
				uint attackActorIndex = animator->GetActorIndex(attackerIndex);
				uint temp = attackerIndex;
				if (attackActorIndex > 0)
				{
					for(uint t=0;t< attackActorIndex;t++)
					temp -= animator->DrawCount(t);
				}
				
				animator->GetFoward(&dir, attackActorIndex, temp);

				uint targetActorIndex =  animator->GetActorIndex(targetIndex);
				uint temp2 = targetIndex;
				if (targetActorIndex > 0)
				{
					for (uint t = 0; t < targetActorIndex; t++)
						temp2 -= animator->DrawCount(t);
				}
				animator->SetPosition(dir,5.0f, targetActorIndex, temp2);
				
			
			}
		
			if (animator->IsAtacking(attackerIndex) == false)
				continue;

			if (Result[i].z > -1)
			{
				targetIndex = tweenIndex[static_cast<uint>(Result[i].z)].index;

				if (tweenDesc[targetIndex].state == ActorState::Die || tweenDesc[targetIndex].state == ActorState::BlockingReaction)
				{
					continue;
				}
				cout << to_string(attackerIndex) + " Actor Hit the  :" + to_string(targetIndex) + " sword" << endl;
				if (tweenDesc[targetIndex].state == ActorState::MoveSide ||
					tweenDesc[targetIndex].state == ActorState::Move)
				{
					tweenDesc[targetIndex].state = ActorState::BlockingReaction;

					uint attackActorIndex = animator->GetActorIndex(attackerIndex);
					uint temp = attackerIndex;
					if (attackActorIndex > 0)
					{
						for (uint t = 0; t < attackActorIndex; t++)
							temp -= animator->DrawCount(t);
					}

					animator->GetFoward(&dir, attackActorIndex, temp);

					uint targetActorIndex = animator->GetActorIndex(targetIndex);
					uint temp2 = targetIndex;
					if (targetActorIndex > 0)
					{
						for (uint t = 0; t < targetActorIndex; t++)
							temp2 -= animator->DrawCount(t);
					}
					animator->SetPosition(dir, 30.0f, targetActorIndex, temp2);
				}
				if (swordEffectIndex > -1)
					effectCounts[swordEffectIndex]++;
				Result[i].z = -1;
			}
			else if (Result[i].x > -1)
			{
				targetIndex = tweenIndex[static_cast<uint>(Result[i].x)].index;
			
				if (tweenDesc[targetIndex].state == ActorState::MoveSide || tweenDesc[targetIndex].state == ActorState::Die)
				{
					continue;
				}
			
				cout << to_string(attackerIndex) + " Actor Hit the  :" + to_string(targetIndex) + " body" << endl;
				tweenDesc[targetIndex].state = ActorState::StandingReaction;
				if (bodyEffectIndex > -1)
				effectCounts[bodyEffectIndex]++;
				Result[i].x = -1;
			}

			if (Result[i].y > -1)
			{
				targetIndex = tweenIndex[static_cast<uint>(Result[i].y)].index;
			
				if (tweenDesc[targetIndex].state == ActorState::MoveSide ||
					tweenDesc[targetIndex].state == ActorState::BlockingReaction
					)
				{
					continue;
				}
				cout << to_string(attackerIndex) + " Actor Hit the  :" + to_string(targetIndex) + " head" << endl;
				animator->tweenData[targetIndex].IsStopped = true;
				hittedIndex = targetIndex;
				
		
				Result[i].y = -1;
				if (headEffectIndex > -1)
				effectCounts[headEffectIndex]++;
				
				
				IsHitted = true;
				
			}
		
		}
		

		
		if (bodyEffectIndex > -1 && effectCounts[bodyEffectIndex] > 0)
		{
			effects->ResetBodies(bodyEffectIndex, effectCounts[bodyEffectIndex]);
			effectCounts[bodyEffectIndex] = 0;
		}
			


		if (swordEffectIndex > -1 && effectCounts[swordEffectIndex] > 0)
		{
			effects->ResetBodies(swordEffectIndex, effectCounts[swordEffectIndex]);
			effectCounts[swordEffectIndex] = 0;
		}
		
		if (headEffectIndex > -1 && effectCounts[headEffectIndex] > 0)
		{
			effects->ResetBodies(headEffectIndex, effectCounts[headEffectIndex]);
			effectCounts[headEffectIndex] = 0;
		}
			
	
}

void PhysicsSystem::Compute(ID3D11DeviceContext * context, ID3D11UnorderedAccessView* effectPositionUAV, const uint& skeletalCount, const uint& staticCount)
{
	if (IsHitted)
	{
		if (Mouse::Get()->Down(0))
		{
			if (Time::Get()->Stopped())
			{
				if (hittedIndex > -1)
				{
					animator->tweenData[hittedIndex].IsStopped = false;
					animator->tweenData[hittedIndex].state = ActorState::Die;
				}
				Time::Get()->Start();
				IsHitted = false;
				return;
			}
		}
		return;
	}
	if (skeletalCount < 2) return;

	auto& tweenVector = animator->tweenData;

	auto Check = all_of(tweenVector.begin(), tweenVector.end(), [](const TweenData& desc)
	{
		return desc.state != ActorState::Attack;
	});

	if (Check)
		return;



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

	ID3D11UnorderedAccessView* uavArray[2] = { copy_StructuredBufferUAV ,effectPositionUAV };
	context->CSSetUnorderedAccessViews(0, 2, uavArray, nullptr);
	context->CSSetShader(CollisonCS, nullptr, 0);
	context->Dispatch(skeletalCount, 1, 1);

	//INT64 currentTime;
	//QueryPerformanceCounter((LARGE_INTEGER *)&currentTime);
	//if (chrono::_Is_even<INT64>(currentTime))
	{

		context->CopyResource(copyBuffer, copy_StructuredBuffer);
		{
			D3D11_MAPPED_SUBRESOURCE subResource;
			context->Map(copyBuffer, 0, D3D11_MAP_READ, 0, &subResource);
			{
				memcpy(&Result, subResource.pData, sizeof(Vector4) *skeletalCount);
			}
			context->Unmap(copyBuffer, 0);
		}
	}

	ZeroMemory(&bufferArray, sizeof(bufferArray));
	context->CSSetConstantBuffers(0, 1, bufferArray);
	context->CSSetShader(nullptr, nullptr, 0);
	ZeroMemory(srvArray, sizeof(srvArray));
	context->CSSetShaderResources(0, 1, srvArray);
	ZeroMemory(uavArray, sizeof(uavArray));
	context->CSSetUnorderedAccessViews(0, 2, uavArray, nullptr);





	static bool bFisrt = true;

	if (bFisrt)
	{
		for (uint i = 0; i < 10; i++)
		{
			Result[i] = Vector4(-1, -1, -1, -1);
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
			targetIndex = tweenIndex[static_cast<uint>(Result[i].w)].index;

			if (tweenDesc[targetIndex].state == ActorState::Die)continue;

			tweenDesc[targetIndex].state = ActorState::StandingReaction;
		

			uint attackActorIndex = animator->GetActorIndex(attackerIndex);
			uint temp = attackerIndex;
			if (attackActorIndex > 0)
			{
				for (uint t = 0; t < attackActorIndex; t++)
					temp -= animator->DrawCount(t);
			}

			animator->GetFoward(&dir, attackActorIndex, temp);

			uint targetActorIndex = animator->GetActorIndex(targetIndex);
			uint temp2 = targetIndex;
			if (targetActorIndex > 0)
			{
				for (uint t = 0; t < targetActorIndex; t++)
					temp2 -= animator->DrawCount(t);
			}
			animator->SetPosition(dir, 5.0f, targetActorIndex, temp2);


		}

		if (animator->IsAtacking(attackerIndex) == false)
			continue;

		if (Result[i].z > -1)
		{
			targetIndex = tweenIndex[static_cast<uint>(Result[i].z)].index;

			if (tweenDesc[targetIndex].state == ActorState::Die || tweenDesc[targetIndex].state == ActorState::BlockingReaction)
			{
				continue;
			}
		
			if (tweenDesc[targetIndex].state == ActorState::MoveSide ||
				tweenDesc[targetIndex].state == ActorState::Move)
			{
				tweenDesc[targetIndex].state = ActorState::BlockingReaction;

				uint attackActorIndex = animator->GetActorIndex(attackerIndex);
				uint temp = attackerIndex;
				if (attackActorIndex > 0)
				{
					for (uint t = 0; t < attackActorIndex; t++)
						temp -= animator->DrawCount(t);
				}

				animator->GetFoward(&dir, attackActorIndex, temp);

				uint targetActorIndex = animator->GetActorIndex(targetIndex);
				uint temp2 = targetIndex;
				if (targetActorIndex > 0)
				{
					for (uint t = 0; t < targetActorIndex; t++)
						temp2 -= animator->DrawCount(t);
				}
				animator->SetPosition(dir, 30.0f, targetActorIndex, temp2);
			}
			if (swordEffectIndex > -1)
				effectCounts[swordEffectIndex]++;
			Result[i].z = -1;
		}
		else if (Result[i].x > -1)
		{
			targetIndex = tweenIndex[static_cast<uint>(Result[i].x)].index;

			if (tweenDesc[targetIndex].state == ActorState::MoveSide || tweenDesc[targetIndex].state == ActorState::Die)
			{
				continue;
			}

	
			tweenDesc[targetIndex].state = ActorState::StandingReaction;
			if (bodyEffectIndex > -1)
				effectCounts[bodyEffectIndex]++;
			Result[i].x = -1;
		}

		if (Result[i].y > -1)
		{
			targetIndex = tweenIndex[static_cast<uint>(Result[i].y)].index;

			if (tweenDesc[targetIndex].state == ActorState::MoveSide ||
				tweenDesc[targetIndex].state == ActorState::BlockingReaction
				)
			{
				continue;
			}
		
			animator->tweenData[targetIndex].IsStopped = true;
			hittedIndex = targetIndex;


			Result[i].y = -1;
			if (headEffectIndex > -1)
				effectCounts[headEffectIndex]++;


			IsHitted = true;

		}

	}

	if (bodyEffectIndex > -1 && effectCounts[bodyEffectIndex] > 0)
	{
		effects->ResetBodies(bodyEffectIndex, effectCounts[bodyEffectIndex]);
		effectCounts[bodyEffectIndex] = 0;
	}



	if (swordEffectIndex > -1 && effectCounts[swordEffectIndex] > 0)
	{
		effects->ResetBodies(swordEffectIndex, effectCounts[swordEffectIndex]);
		effectCounts[swordEffectIndex] = 0;
	}

	if (headEffectIndex > -1 && effectCounts[headEffectIndex] > 0)
	{
		effects->ResetBodies(headEffectIndex, effectCounts[headEffectIndex]);
		effectCounts[headEffectIndex] = 0;
	}



	
}

void PhysicsSystem::CreateCopyBuffer(ID3D11Device * device)
{

	UINT outSize = 10;
	
	//ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));


	SafeRelease(copy_StructuredBuffer);
	SafeRelease(copy_StructuredBufferUAV);

	// Create Structured Buffer
	D3D11_BUFFER_DESC sbDesc;


	sbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	sbDesc.CPUAccessFlags = 0;
	sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbDesc.StructureByteStride = sizeof(Vector4);
	sbDesc.ByteWidth = sizeof(Vector4) *outSize;
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
