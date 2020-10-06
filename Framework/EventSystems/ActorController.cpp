#include "Framework.h"
#include "ActorController.h"
#include "Viewer/Orbit.h"
#include "Animator.h"

ActorController::ActorController(Animator* animator)
	:animator(animator), bStart(false), bPause(false), bAttack(false), bNextCombo(false), ComboCount(0)
{
	orbit = new Orbit();
	desc = D3D::GetDesc();
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
	if (!bStart)
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
	const Matrix& temp = animator->GetInstMatrix(0, 0);

	Forward = Vector3(temp._31, temp._32, temp._33);
	Right = Vector3(temp._11, temp._12, temp._13);

	{
		POINT point;
		GetCursorPos(&point);

		moveValue.x = static_cast<float>(point.x - m_pt.x);
		moveValue.y = static_cast<float>(point.y - m_pt.y);
		orbit->SetMoveValue(moveValue);
		rotation.y += (moveValue.x*0.15f)*0.04f;

		SetCursorPos(m_pt.x, m_pt.y);
	}
	


	D3DXMatrixDecompose(&s, &q, &p, &temp);
	D3DXMatrixScaling(&S, s.x, s.y, s.z);
	D3DXMatrixRotationY(&R, rotation.y);

	prevPosition = p;
	position = p;
	bool bMoveSide = false;
	
	bool bJump = false;
	

	
	if (Keyboard::Get()->Press('W'))
	{

		position -= 200.0f*Forward* Time::Delta();

	}
	else if (Keyboard::Get()->Press('S'))
	{

		position += 200.0f*Forward* Time::Delta();

	}

	if (Keyboard::Get()->Press('A'))
	{


		position += 200.0f*Right* Time::Delta();
		bMoveSide = true;

	}
	else if (Keyboard::Get()->Press('D'))
	{

		position -= 200.0f*Right* Time::Delta();

		bMoveSide = true;
	}	


	
	if (Keyboard::Get()->Press(0x20))
	{
		animator->tweenData[0].speed = 1.1f;
		animator->tweenDesc[0].state = ActorState::Jump;
		bJump = true;
	}

	
	if (Keyboard::Get()->Up('W')|| Keyboard::Get()->Up('S')|| Keyboard::Get()->Up('A')|| Keyboard::Get()->Up('D'))
	{
		animator->tweenDesc[0].state = ActorState::Idle;
	}


	velocity = D3DXVec3Length(&(position - prevPosition));

	
	if (Mouse::Get()->Down(0))
	{
		animator->tweenData[0].speed = 1.5f;
		animator->tweenDesc[0].state = ActorState::Attack;
		bAttack = true;
		//Attacking();
		//OnCheckCombo();

		//OnEndAttack();
	}

	//if(!animator->IsAtacking(0))
	//{
	//	OnFinishCombo();
	//	animator->tweenData[0].speed = 1.0f;
	//}



	if (velocity > 0.0f && !bAttack &&!bJump)
	{
		animator->tweenDesc[0].state = ActorState::Move;
		if(bMoveSide)
			animator->tweenDesc[0].state = ActorState::MoveSide;

		
	}

	bAttack = false;
	
	

	orbit->SetTargetPosition(Vector3(position.x, position.y+2 , position.z));
	D3DXMatrixTranslation(&T, position.x, position.y, position.z);
	const Matrix& result = S * R*T;

	if(Math::Abs(moveValue.x)>0||Math::Abs( moveValue.y)>0||velocity>0)
	animator->SetInstMatrix(0, 0, result);
	orbit->Update();
	
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
	switch (ComboCount)
	{
	case 0:
	{
		animator->tweenData[0].speed = 1.5f;
		animator->tweenDesc[0].state = ActorState::Attack;
	}
		
		break;
	case 1:
	{
		animator->tweenData[0].bContinue = true;
		animator->tweenData[0].speed = 1.2f;
		//animator->tweenDesc[0].state = ActorState::Attack2;
	}
	
		break;
	case 2:
		//animator->tweenDesc[0].state = ActorState::Attack3;
		break;
	}
}

void ActorController::OnEndAttack()
{
	bAttack = false;
	if (bNextCombo)
	{

		Attacking();
		bNextCombo = false;
	}
	else
		ComboCount = 0;
}

void ActorController::OnCheckCombo()
{
	if (bNextCombo)
		ComboCount++;
}

void ActorController::OnFinishCombo()
{
	bAttack = false;
	bNextCombo = false;

	ComboCount = 0;
}
