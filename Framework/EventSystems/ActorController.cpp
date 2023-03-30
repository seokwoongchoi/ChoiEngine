#include "Framework.h"
#include "ActorController.h"
#include "Viewer/Orbit.h"


ActorController::ActorController(Animator* animator)
	:animator(animator), bStart(false), bPause(false), bAttack(false), bNextCombo(false), ComboCount(0)
{
	orbit = new Orbit();
	desc = D3D::GetDesc();

	keys.emplace_back('A');
	keys.emplace_back('D');
	keys.emplace_back('W');
	keys.emplace_back('S');
}

ActorController::~ActorController()
{
}

void ActorController::Start()
{
	//Context::Get()->SetCameraIndex(1);
	m_pt.x = desc.Width / 2;
	m_pt.y = desc.Height / 2;
	ClientToScreen(desc.Handle, &m_pt);
	SetCursorPos(m_pt.x, m_pt.y);
	ShowCursor(false);
	bStart = true;
}

void ActorController::Update()
{
	if (!bStart||animator->actorCount<1)
		return;

	if (Keyboard::Get()->Down(27))
	{
		bPause ? bPause = false : bPause = true;
		ShowCursor(bPause);
	}

	if (bPause)
	{
		orbit->SetMoveValue(Vector2(0.0f, 0.0f));
		return;
	}
	
	animator->GetInstMatrix(&instMatrix,0, 0);
	animator->GetFoward(&Forward, 0, 0);
	tweenData = animator->tweenData[0];

	D3DXMatrixDecompose(&s, &q, &position, &instMatrix);
	D3DXMatrixScaling(&S, s.x, s.y, s.z);
	prevPosition = position;
	prevRotation = rotation.y;

	


	bool bJump = false;
	
	bool walking = false;
	float direction = -1.0f;
	if (Keyboard::Get()->Press(&keys[0]))
	{
		speed += Time::Delta()*100.0f;
		walking = true;
	}
	if (speed > 400.0f)
		speed = 400.0f;
	if (Keyboard::Get()->Press('S'))
	{
		direction = 1.0f;
		
	}

	if (Keyboard::Get()->Press('A'))
	{

		count++;
		if (count < 50)
		{
			rotation.y -= Time::Delta()*7.0f;
		}
	}
	else if (Keyboard::Get()->Press('D'))
	{
		count++;
		if (count <50)
		{
			rotation.y += Time::Delta()*7.0f;
		}
		
	}	
	if (walking)
	{
		
		position += (direction*speed * Forward* Time::Delta());
	}
	

	velocity = D3DXVec3Length(&(position - prevPosition));
	if (velocity > 0.0f && !bAttack && !bJump)
	{
		tweenData.state = ActorState::Move;
		if(speed>300.0f)
			tweenData.state = ActorState::Run;
	}

	if (Keyboard::Get()->Press(0x20))
	{
		tweenData.speed = 1.5f;
		tweenData.state = ActorState::Jump;

	/*	float height= animator->tweenDesc[0].Curr.CurrFrame<10?
			1.0f*Time::Delta() :-1.0f*Time::Delta();
		position.y += height;*/
		
		bJump = true;
	}

	
	
	
	if (Mouse::Get()->Down(0))
	{
		
		Attacking();
		
	}
	else if (Mouse::Get()->Down(1))
	{
		if (tweenData.state == ActorState::Attack)
		{
			tweenData.speed = 2.0f;
		}
		tweenData.state = ActorState::Attack2;
		
	}

	/*thread worker1([&]() {
		AttackNotify1();
		AttackNotify2();
		AttackNotify3(); });

	worker1.join();*/

	
	{
		POINT point;
		GetCursorPos(&point);
		Vector2 moveValue;
		moveValue.x = static_cast<float>(point.x - m_pt.x);
		moveValue.y = static_cast<float>(point.y - m_pt.y);
		orbit->SetMoveValue(moveValue);
		orbit->SetTargetPosition(Vector3(position.x, position.y + 2, position.z));
		orbit->Update();
		//rotation.y += (moveValue.x*0.15f)*delta;
     	SetCursorPos(m_pt.x, m_pt.y);
	}
	
	if (velocity > 0.0f || rotation.y != prevRotation)
	{
		D3DXMatrixRotationY(&R, rotation.y);
		D3DXMatrixTranslation(&T, position.x, position.y, position.z);
		 result = S * R*T;
		animator->SetInstMatrix(&result,0, 0);
	}

	if (Keyboard::Get()->Up('A') || Keyboard::Get()->Up('D'))
	{
		count = 0;
	}
	if (!Keyboard::Get()->Press('W')&&Keyboard::Get()->Up(&keys[0]))
	{
		tweenData.state = ActorState::Idle;
		speed = 200.0f;
	}
	if (tweenData.state == ActorState::Idle)
	{
		bAttack = false;
		bNextCombo = false;

		ComboCount = 0;

	}
	animator->tweenData[0] = tweenData;
}

void ActorController::Stop()
{
	ShowCursor(true);
	bStart = false;
}

void ActorController::Attacking()
{
	if (bAttack == true)
	{
		bNextCombo = true;
		return;
	}



	bAttack = true;
	
	if (ComboCount < 1)
	{
		tweenData.speed = 2.0f;
		tweenData.state = ActorState::Attack;
	}
	//else if( ComboCount < 2)
	//{
	//	animator->tweenData[0].bContinue = true;
	//	//animator->tweenData[0].farmeDelta = 20;
	//	animator->tweenData[0].speed = 1.5f;
	//	tweenDesc.state = ActorState::Attack2;
	//}
 //
	//else 
	//{
	//	animator->tweenData[0].bContinue = true;
	//	animator->tweenData[0].farmeDelta =20;
	//	animator->tweenData[0].speed = 2.0f;
	//	tweenDesc.state = ActorState::Attack3;
	//}
	/*switch (ComboCount)
	{
	case 0:
	{
		
	}
		
		break;
	case 1:
	{
		
	}
	
		break;
	case 2:
		animator->tweenData[0].bContinue = true;
		animator->tweenData[0].speed = 1.2f;
		tweenDesc.state = ActorState::Attack3;
		break;
	}*/
}

