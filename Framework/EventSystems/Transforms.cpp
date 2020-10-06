#include "Framework.h"
#include "Transforms.h"
#include "Animator.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Renderer.h"
Transforms::Transforms(ID3D11Device * device,Animator* animator)
	:animator(animator), position(Vector3(0.0f, 0.0f, 0.0f)),quat ( Quaternion(0.0f, 0.0f, 0.0f, 0.0f)),
     scale( Vector3(1.0f, 1.0f, 1.0f))
{
}

Transforms::~Transforms()
{
}

void Transforms::Update(const uint & index)
{
	uint drawCount = animator->renderers[index]->drawCount;
	int btIndex= animator->renderers[index]->btIndex;
	if (btIndex > -1)
	{
		for (uint i = 0; i < drawCount; i++)
		{
			if (index == 0 && i == 0)continue;

			D3DXMatrixDecompose(&scale, &quat, &position, &animator->GetInstMatrix(index, i));
			behaviorTrees[btIndex]->SetTransform(position,quat,scale);
			behaviorTrees[btIndex]->Tick(index, i);
		}
		
	}
	
}


void Transforms::ReadBehaviorTree(BinaryReader * r, const uint & actorIndex)
{
	int behaviorTreeNum = r->Int();
	if (behaviorTreeNum > -1)
	{
		animator->renderers[actorIndex]->btIndex = behaviorTreeNum;
		BinaryReader * BehaviorRead = new BinaryReader();
		wstring path = L"../_BehaviorTreeDatas/BehaviorTree" + to_wstring(behaviorTreeNum) + L".behaviortree";
		BehaviorRead->Open(path);
		uint count = BehaviorRead->UInt();
		BehaviorTreeBuilder* Builder = new BehaviorTreeBuilder();
		for (uint i = 0; i < count; i++)
		{
		

			string node = BehaviorRead->String();
			if (node == "Back")
			{
				count++;
				cout << node << endl;
				Builder->Back();
				continue;

			}
			
			{
				Vector2 pos = Vector2(BehaviorRead->Float(), BehaviorRead->Float());
				uint parentIndex = BehaviorRead->UInt();
			}

			if (node == "Selector")
			{
				Builder->ActiveSelector();
				cout << node << endl;
			}
			else if (node == "Sequence")
			{
				Builder->Sequence();
				cout << node << endl;
			}
			else if (node == "SimpleParallel")
			{
				Builder->Parallel(EPolicy::RequireAll, EPolicy::RequireOne);
				cout << node << endl;
			}
			else if (node == "Condition1")
			{
				Builder->Condition(EConditionMode::IsSeeEnemy, false);
			
				cout << node << endl;
			}
			else if (node == "Condition2")
			{
				Builder->Condition(EConditionMode::IsHealthLow, false);
				cout << node << endl;
			}
			else if (node == "Condition3")
			{
				Builder->Condition(EConditionMode::IsEnemyDead, true);
				cout << node << endl;
			}
			else if (node == "Condition4")
			{
				Builder->Condition(EConditionMode::IsInRange, false);
				cout << node << endl;
			}
			else if (node == "Action1")
			{
				Builder->Action(EActionMode::Attack);
				
			}
			else if (node == "Action2")
			{
				Builder->Action(EActionMode::Patrol);
				cout << node << endl;
			}
			else if (node == "Action3")
			{
				Builder->Action(EActionMode::Runaway);
				cout << node << endl;
			}
			else if (node == "Action4")
			{
				Builder->Action(EActionMode::Strafe);
				cout << node << endl;
			}
		

			
		}
		BehaviorRead->Close();
		SafeDelete(BehaviorRead);
		behaviorTrees.emplace_back(Builder->End());
		behaviorTrees.back()->SetAnimator(animator);
	}
}
