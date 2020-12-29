#pragma once
#include "Animator.h"
class ActorController
{
public:
	ActorController(class Animator* animator);
	~ActorController(); 


	void Start();
	void Update();
	void Stop() ;

//inline bool AttackNotify1()
//	{
//		if (tweenDesc.state != ActorState::Attack)return false;
//
//		auto currFrame = tweenDesc.Curr.CurrFrame;
//
//	
//			
//		if (currFrame > 40)
//		{
//			OnEndAttack();
//			
//		}
//			
//		else if (currFrame > 5)
//		{
//			OnCheckCombo(1);
//			
//		}
//
//	}
//inline bool AttackNotify2()
//{
//	if (tweenDesc.state != ActorState::Attack2)
//		return false;
//
//	auto currFrame = tweenDesc.Curr.CurrFrame;
//
//	if (currFrame > 40)
//	{
//		OnEndAttack();
//		
//	}
//
//	else if (currFrame > 5)
//	{
//		OnCheckCombo(2);
//		
//	}
//
//}
//
//inline bool AttackNotify3()
//{
//	if (tweenDesc.state != ActorState::Attack3)return false;
//
//	auto currFrame = tweenDesc.Curr.CurrFrame;
//
//	if (currFrame > 40)
//	{
//		OnFinishCombo();
//		
//	}
//	
//	
//}
private:
	void Attacking();

	inline void OnEndAttack()
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

	inline void OnCheckCombo(uint count)
	{
		if (bNextCombo)
			ComboCount= count;
	}

	inline void OnFinishCombo()
	{
		bAttack = false;
		bNextCombo = false;

		ComboCount = 0;
	}


	bool bAttack;
	bool bNextCombo;
	uint ComboCount;


private:
	vector<DWORD> keys;
	Vector3 position = Vector3(0, 0, 0);
	Vector3 Forward = Vector3(0, 0, 1);
	
	Vector3 Right = Vector3(1, 0, 0);
	Matrix R, result;
	Matrix instMatrix;
	Vector3 rotation = Vector3(0, 0, 0);

	Matrix S, T;
	Vector3  s;
	Quaternion q;

	float speed = 200.0f;
	uint count = 0;
	
	float velocity;
	

	D3DDesc desc;
	POINT m_pt;
	Vector3 prevPosition = Vector3(0, 0, 0);
	float prevRotation =0.0f;
	bool bPause;
	bool bStart;
	class Animator* animator;
	class Orbit* orbit;
	struct TweenData tweenData;
};

