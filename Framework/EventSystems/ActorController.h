#pragma once
class ActorController
{
public:
	ActorController(class Animator* animator);
	~ActorController(); 


	void Start();
	void Update();
	void Stop() ;

private:
	void Attacking();
	void OnEndAttack();
	void OnCheckCombo();
	void OnFinishCombo();
	bool bAttack;
	bool bNextCombo;
	uint ComboCount;
private:
	class Animator* animator;
	class Orbit* orbit;

private:
	Vector3 position = Vector3(0, 0, 0);
	Vector3 Forward = Vector3(0, 0, 1);
	
	Vector3 Right = Vector3(1, 0, 0);
	Matrix R;

	Vector3 rotation = Vector3(0, 0, 0);

	Matrix S, T;
	Vector3 p, s, r;
	Quaternion q;
	Vector3 prevPosition = Vector3(0, 0, 0);
	Vector2 moveValue;
	
	float velocity;
	
	float xValue = 0;

	D3DDesc desc;
	POINT m_pt;


	bool bPause;
	bool bStart;
	
};

