#include "Framework.h"
#include "Behavior.h"
#include "EventSystems/Animator.h"

//std::random_device rd;
//std::default_random_engine engine(rd());
//std::uniform_int_distribution<> dis(1, 100);
////auto dice= std::bind(dis, engine);
//std::_Binder<std::_Unforced, std::uniform_int_distribution<>&, std::default_random_engine&> dice = std::bind(dis, engine);


Animator* Behavior::animator = nullptr;

Vector3 Behavior::position = Vector3(0.0f, 0.0f, 0.0f);
Quaternion Behavior::quat = Quaternion(0.0f, 0.0f, 0.0f, 0.0f);
 Vector3 Behavior::scale = Vector3(1.0f, 1.0f, 1.0f);
  uint Behavior::findedActorIndex=0;
  uint Behavior::findedInstIndex =0;
void Behavior::SetAnimator(Animator * animator)
{
	this->animator = animator;
}

EStatus  Behavior::Tick(const uint & actorIndex, const uint& instanceIndex)
{
	 
	if (Status != EStatus::Running)
	{
		OnInitialize();
	}

	Status = Update(actorIndex,instanceIndex);
	

	if (Status != EStatus::Running)
	{
		OnTerminate(Status);
	}

	return Status;
}



EStatus  Repeat::Update(const uint & actorIndex, const uint& instanceIndex)
{
	while (true)
	{
		Child->Tick(actorIndex, instanceIndex);
		if (Child->IsRunning())return EStatus::Success;
		if (Child->IsFailuer())return EStatus::Failure;
		if (++Count == Limited)return EStatus::Success;
		Child->Reset();
	}
	return EStatus::Invalid;
}

void  Composite::RemoveChild(Behavior * InChild)
{
	auto it = std::find(Children.begin(), Children.end(), InChild);
	if (it != Children.end())
	{
		Children.erase(it);
	}
}

EStatus  Sequence::Update(const uint & actorIndex, const uint& instanceIndex)
{
	while (true)
	{
		EStatus s = (*CurrChild)->Tick(actorIndex, instanceIndex);
	
		if (s != EStatus::Success)
			return s;
		if (++CurrChild == Children.end())
			return EStatus::Success;
	}
	return EStatus::Invalid; 
}



EStatus  Selector::Update(const uint & actorIndex, const uint& instanceIndex)
{
	while (true)
	{
		EStatus s = (*CurrChild)->Tick(actorIndex, instanceIndex);
		if (s != EStatus::Failure)
			return s;
		if (++CurrChild == Children.end())
			return EStatus::Failure;
	}
	return EStatus::Invalid; 
}



EStatus  Parallel::Update(const uint & actorIndex, const uint& instanceIndex)
{
	int SuccessCount = 0, FailureCount = 0;
	int ChildrenSize = static_cast<uint>(Children.size());
	for (auto it : Children)
	{
		
		if (!it->IsTerminate())
			it->Tick(actorIndex, instanceIndex);

		if (it->IsSuccess())
		{
			++SuccessCount;
			if (SucessPolicy == EPolicy::RequireOne)
			{
				it->Reset();
				return EStatus::Success;
			}

		}

		if (it->IsFailuer())
		{
			++FailureCount;
			if (FailurePolicy == EPolicy::RequireOne)
			{
				it->Reset();
				return EStatus::Failure;
			}
		}
	}

	if (FailurePolicy == EPolicy::RequireAll&&FailureCount == ChildrenSize)
	{
		for (auto it : Children)
		{
			it->Reset();
		}

		return EStatus::Failure;
	}
	if (SucessPolicy == EPolicy::RequireAll&&SuccessCount == ChildrenSize)
	{
		for (auto it : Children)
		{
			it->Reset();
		}
		return EStatus::Success;
	}

	return EStatus::Running;
}

void  Parallel::OnTerminate(EStatus InStatus)
{
	for (auto it : Children)
	{
		if (it->IsRunning())
			it->Abort();
	}
}


EStatus  ActiveSelector::Update(const uint & actorIndex, const uint& instanceIndex)
{

	Behaviors::iterator Previous = CurrChild;
	
	Selector::OnInitialize();
	EStatus result = Selector::Update(actorIndex, instanceIndex);
	
	if (Previous != Children.end()&CurrChild != Previous)
	{
		(*Previous)->Abort();
	}

	return result;
}



EStatus  Condition_IsSeeEnemy::Update(const uint & actorIndex, const uint& instanceIndex)
{
	auto& tweenData = animator->tweenData;
	uint count;
	animator->SaveCount(actorIndex, count);
	uint index = count + instanceIndex;

	uint quadTreeID = tweenData[index].quadTreeID;
	auto find = find_if(tweenData.begin(), tweenData.end(), [&index, &quadTreeID](const TweenData& desc)
	{
		if (index != desc.index)
		{
			//int temp = desc.quadTreeID-quadTreeID;
			/*if (temp < 0)
			{
				if (temp == -2 || temp == -4 ||
					temp == -1028 || temp == -1026 ||
					temp == -1024 || temp == -1022 ||
					temp == -1020 || temp == -2050 ||
					temp == -2048 || temp == -2046)
					return true;
			
			}
			if (temp == 0    ||
				temp == 2    || temp == 4    ||
				temp == 1028 || temp == 1026 ||
				temp == 1024 || temp == 1022 ||
				temp == 1020 || temp == 2050 ||
				temp == 2048 || temp == 2046 )
				return true;*/

			int temp = desc.quadTreeID - quadTreeID;
			if (temp < 0)
			{
				if (temp == -2 * 4 ||
					temp == -1026 * 4 ||
					temp == -1024 * 4 ||
					temp == -1022 * 4)
					return true;

			}
			else
			{
				if (temp == 0 ||
					temp == 2 * 4 ||
					temp == 1026 * 4 ||
					temp == 1024 * 4 ||
					temp == 1022 * 4)
					return true;
			}
			
		}
		return false;
	});

	if (find != tweenData.end())
	{
		animator->GetActorIndex(find->index, findedActorIndex, findedInstIndex);
		return !IsNegation ? EStatus::Success : EStatus::Failure;
	}

	return !IsNegation ? EStatus::Failure : EStatus::Success;





	//uint actorCount;
	//animator->ActorCount(actorCount);
	//
	//
	//const float& dist = 5.0f;
	//	
	//for (uint a = 0; a < actorCount; a++)
	//{
	//	const uint& count = animator->DrawCount(a);
	//	for (uint i = 0; i < count; i++)
	//	{
	//		if (actorIndex == a && instanceIndex == i) continue;

	//		uint count;
	//		animator->SaveCount(a, count);
	//		uint index = count + i;
	//		if (animator->tweenData[index].state == ActorState::Die)
	//			continue;

	//		Vector3 findPosition;
	//		animator->GetPosition(&findPosition, a, i);
	//		float temp = (findPosition.x - position.x)*(findPosition.x - position.x) +
	//			(findPosition.z - position.z)*(findPosition.z - position.z);


	//		if (temp < dist*dist)
	//		{
	//			

	//		}

	//	}
	//}
	//	

	// return !IsNegation ? EStatus::Failure : EStatus::Success;
	
}

EStatus  Condition_IsHealthLow::Update(const uint & actorIndex, const uint& instanceIndex)
{
	Vector3 findPosition;
	animator->GetPosition(&findPosition,findedActorIndex, findedInstIndex);

	float length = D3DXVec3Length(&(Vector3(position.x,0.0f,position.z) - Vector3(findPosition.x, 0.0f, findPosition.z)));
  
	uint count;
	animator->SaveCount(findedActorIndex, count);
	
	uint index = count + findedInstIndex;
    //if(length<2.0f|| length < 3.0f&& animator->tweenDesc[index].state == ActorState::Idle)
	//{
	//	return !IsNegation ? EStatus::Failure : EStatus::Success;
	//}
	//else
	{
		
		return !IsNegation ? EStatus::Success : EStatus::Failure;
	}
}

EStatus  Condition_IsEnemyDead::Update(const uint & actorIndex, const uint& instanceIndex)
{
	
	uint count;
	animator->SaveCount(findedActorIndex, count);
	uint index = count + findedInstIndex;

	if(	animator->tweenData[index].state == ActorState::Die)
	{
		//std::cout << "Enemy is Dead" << std::endl;
		return !IsNegation ? EStatus::Success : EStatus::Failure;
	}

	else
	{
		//std::cout << "Enemy is not Dead" << std::endl;
		return !IsNegation ? EStatus::Failure : EStatus::Success;
	}
}

EStatus  Action_Attack::Update(const uint & actorIndex, const uint& instanceIndex)
{
	uint count;
	animator->SaveCount(actorIndex, count);
	uint index = count + instanceIndex;
	auto& tweenData = animator->tweenData[index];
	if (tweenData.state == ActorState::Die)
		return EStatus::Failure;

	Vector3 findPosition;
	animator->GetPosition(&findPosition,findedActorIndex, findedInstIndex);
	Vector3 dir = position - findPosition > 0 ? position - findPosition : (position - findPosition)*-1;
	float x = dir.x;
	float z = dir.z;
	D3DXVec3Normalize(&dir, &Vector3(x, 0.0f, z));
	float line = D3DXVec3Length(&dir);

	float anglez = (acosf(dir.z / line));
	float anglex = (acosf(dir.x / line));
	if (findPosition.x<position.x&& findPosition.z>position.z)
	{
		anglez -= static_cast<float>(D3DXToRadian(360.0));
		angle = anglez;
	}
	else if (findPosition.x > position.x&& findPosition.z < position.z)
	{
		anglex -= static_cast<float>(D3DXToRadian(90.0));
		angle = -anglex;
	}
	else if (findPosition.x < position.x&& findPosition.z < position.z)
	{
		anglez -= static_cast<float>(D3DXToRadian(360.0));
		angle = anglez;
	}
	else if (findPosition.x > position.x&& findPosition.z > position.z)
	{
		anglex -= static_cast<float>(D3DXToRadian(270.0));
		angle = anglex;
	}

	float length = D3DXVec3Length(&(Vector3(position.x, 0.0f, position.z) - Vector3(findPosition.x, 0.0f, findPosition.z)));
	if (saveAngle != angle)
	{
		D3DXMatrixScaling(&S, scale.x, scale.y, scale.z);
		D3DXMatrixTranslation(&T, position.x, position.y, position.z);
		D3DXMatrixRotationY(&R, angle);
		D3DXMatrixMultiply(&world, &S, &R);
		D3DXMatrixMultiply(&world, &world, &T);
		animator->SetInstMatrix(&world,actorIndex, instanceIndex);
		saveAngle = angle;
	}

	if (length < 1.5f)
	{
		
		animator->SetPosition(-dir, 2.0f * Time::Delta(),actorIndex, instanceIndex);
		cout << "Back!!!" << endl;
	}
	else if (length > 1.8f)
	{
		animator->SetPosition(dir, 2.0f * Time::Delta(), actorIndex, instanceIndex);
		
	}

	
	tweenData.speed = 1.5f;
	tweenData.state=ActorState::Attack;
	
	return EStatus::Success;
}

EStatus  Action_Runaway::Update(const uint & actorIndex, const uint& instanceIndex)
{
	
	//if (EventSystem::Get()->IsEndAnimation(actorIndex, instanceIndex, 3))
	{

		//return EStatus::Success;
	}
		
	
	return EStatus::Failure;
}

EStatus  Action_Patrol::Update(const uint & actorIndex, const uint& instanceIndex)
{ 
	/*
	Vectorww3 randomDir;
	randomDir.x= Math::Random(-1.0f, 1.0f);
	randomDir.y = 0;
	randomDir.z = Math::Random(-1.0f, 1.0f);
	
	position = actorData->GetPosition(instanceIndex);
	finish = position + distance * nor;

	Vector3 currPosition = actorData->GetPosition(instanceIndex);
	Vector3 delta = finish - currPosition > 0 ?
		finish - currPosition : currPosition;
	float line = D3DXVec3Length(&delta);
	float anglez = acosf(delta.z / line);
	float anglex = acosf(delta.x / line);

	if (finish.x<currPosition.x&& finish.z>currPosition.z)
	{
		anglez -= D3DXToRadian(180);
		rotation.y = anglez;
	}
	else if (finish.x > currPosition.x&& finish.z < currPosition.z)
	{
		anglex += D3DXToRadian(90);
		rotation.y = -anglex;
	}
	else if (finish.x < currPosition.x&& finish.z < currPosition.z)
	{
		anglez -= D3DXToRadian(180);
		rotation.y = anglez;
	}
	else if (finish.x > currPosition.x&& finish.z > currPosition.z)
	{
		anglex += D3DXToRadian(270);
		rotation.y = anglex;
	}
	
	
	EventSystem::Get()->Events["Move"](actorData->ActorIndex(), instanceIndex);
	
	while (!IsArrive)
	{
		if (static_cast<uint>(position.x) != static_cast<uint>(finish.x))
			position += speed * nor;
		else
		{
			IsArrive = true;
			break;
		}
			
	}

	actorData->SetPosition(position, instanceIndex);
	actorData->SetRotation(rotation, instanceIndex); */
	
	//EventSystem::Get()->Events["Move"](actorIndex, instanceIndex);
	//std::cout << "Action_Patrol" << std::endl;
	return EStatus::Success;

}

EStatus Condition_IsInRange::Update(const uint & actorIndex, const uint & instanceIndex)
{
	Vector3 findPosition;
	animator->GetPosition(&findPosition,findedActorIndex, findedInstIndex);

	float length = D3DXVec3Length(&(Vector3(position.x, 0.0f, position.z) - Vector3(findPosition.x, 0.0f, findPosition.z)));

	uint count;
	animator->SaveCount(findedActorIndex, count);
	uint index = count + findedInstIndex;
	if (length < 2.0f )
	{
		return !IsNegation ? EStatus::Failure : EStatus::Success;
	}
	else
	{

		return !IsNegation ? EStatus::Success : EStatus::Failure;
	}
}

EStatus Action_Strafe::Update(const uint & actorIndex, const uint & instanceIndex)
{
	uint count;
	animator->SaveCount(actorIndex, count);
	
	uint index = count + instanceIndex;
	auto& tweenData = animator->tweenData[index];
	if (tweenData.state == ActorState::Die)
		return EStatus::Failure;
	else
	{

		
		Vector3 findPosition;
		animator->GetPosition(&findPosition,findedActorIndex, findedInstIndex);
		Vector3 dir = position - findPosition > 0 ? position - findPosition : (position - findPosition)*-1;
		float x = dir.x;
		float z = dir.z;
		D3DXVec3Normalize(&dir, &Vector3(x, 0.0f, z));
		float line = D3DXVec3Length(&dir);

		float anglez = (acosf(dir.z / line));
		float anglex = (acosf(dir.x / line));
		if (findPosition.x<position.x&& findPosition.z>position.z)
		{
			anglez -= static_cast<float>(D3DXToRadian(360.0));
			angle = anglez;
		}
		else if (findPosition.x > position.x&& findPosition.z < position.z)
		{
			anglex -= static_cast<float>(D3DXToRadian(90.0));
			angle = -anglex;
		}
		else if (findPosition.x < position.x&& findPosition.z < position.z)
		{
			anglez -= static_cast<float>(D3DXToRadian(360.0));
			angle = anglez;
		}
		else if (findPosition.x > position.x&& findPosition.z > position.z)
		{
			anglex -= static_cast<float>(D3DXToRadian(270.0));
			angle = anglex;
		}

		D3DXMatrixScaling(&S, scale.x, scale.y, scale.z);
		D3DXMatrixTranslation(&T, position.x, position.y, position.z);
		D3DXMatrixRotationY(&R, angle);
		if (saveAngle != angle)
		{
			D3DXMatrixMultiply(&world, &S, &R);
			D3DXMatrixMultiply(&world, &world, &T);
			animator->SetInstMatrix(&world, actorIndex, instanceIndex);
			saveAngle = angle;
		}
		

		Vector2 random = Vector2(0, 0);
		 random.x = Math::Random(-1.0f, 1.0f);
		D3DXVec2Normalize(&random,&random);
		float speed = Math::Random(0.1f, 5.0f);
		Vector3 Right;
		D3DXVec3Cross(&Right, &dir, &Vector3(0, 1, 0));
		Right *= random.x;
		position += 3.5f* Right * 0.004f;
	

		float length = D3DXVec3Length(&(Vector3(position.x, 0.0f, position.z) - Vector3(findPosition.x, 0.0f, findPosition.z)));
		if (random.x > 0.0f&&length>1.5f)
		{
			
			animator->SetPosition(-dir, -3.5f * Time::Delta(), actorIndex, instanceIndex);
			//position += 3.5f * -dir * Time::Delta();
			
		}
		else if (length < 1.5f)
		{
			animator->SetPosition(-dir, 3.5f * Time::Delta(), actorIndex, instanceIndex);
			//position -= 3.5f * -dir * Time::Delta();
		
		}
		
		//animator->tweenData[index].speed = 1.0f;
		if (tweenData.state!=ActorState::Attack)
		//if (!animator->IsAtacking(index))
		{
			tweenData.state = ActorState::Move;
		
	    }
	
		return EStatus::Success;
	}

}
