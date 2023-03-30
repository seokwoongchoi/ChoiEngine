#pragma once
#include<stack>
#include "Behavior.h"
//namespace BT

	enum class EActionMode
	{
		Attack,
		Patrol,
		Runaway,
		Strafe
	};

	enum class EConditionMode
	{
		IsSeeEnemy,
		IsHealthLow,
		IsEnemyDead,
		IsInRange
	};

	class BehaviorTree
	{
	public:
		explicit BehaviorTree(Behavior* InRoot) :Root(InRoot) {}

		
		BehaviorTree(const BehaviorTree&) = delete;
		BehaviorTree& operator=(const BehaviorTree&) = delete;

		
		inline void Tick(const uint & actorIndex, const uint& instIndex)
		{
			Root->Tick(actorIndex, instIndex);
			
		}
		inline void SetTransform(const  Vector3& pos, const  Quaternion& q, const  Vector3& scale)
		{
			Root->SetTransform(pos, q, scale);
		}
		bool HaveRoot() { return Root ? true : false; }
		void SetRoot(Behavior* InNode) { Root = InNode; }
		
		void Release() { Root->Release(); }
	
		void SetAnimator(class Animator* animator);
	private:
		Behavior* Root;
		
	};

	
	class BehaviorTreeBuilder
	{
	public:
		BehaviorTreeBuilder() {}
		~BehaviorTreeBuilder() { }
		
		BehaviorTreeBuilder* Sequence();
		BehaviorTreeBuilder* Action(EActionMode ActionModes);
		BehaviorTreeBuilder* Condition(EConditionMode ConditionMode, bool IsNegation);
		BehaviorTreeBuilder* Selector();
		BehaviorTreeBuilder* Repeat(int RepeatNum);
		BehaviorTreeBuilder* ActiveSelector();
		BehaviorTreeBuilder* Filter();
		BehaviorTreeBuilder* Parallel(EPolicy InSucess, EPolicy InFailure);
		BehaviorTreeBuilder* Monitor(EPolicy InSucess, EPolicy InFailure);
		BehaviorTreeBuilder* Back();
		BehaviorTree* End();

	private:
		void AddBehavior(Behavior* NewBehavior);

	private:
		Behavior* TreeRoot = nullptr;
		
		std::stack<Behavior*> NodeStack;
	};


