#pragma once
class EventSystem
{

public:
	EventSystem(ID3D11Device* device);
	~EventSystem();

public:
	void Compute(ID3D11DeviceContext* context);
	void Update(const uint& skeletalActorCount, const uint& staticActorCount);
public:
	void SkeletalData_BindPipeLine(ID3D11DeviceContext* context);
	void StaticData_BindPipeLine(ID3D11DeviceContext* context);
private:
	class EffectSystem* effects;
	class Animator* animator;
	class PhysicsSystem* physics;
	class ColliderSystem* collider;
};

