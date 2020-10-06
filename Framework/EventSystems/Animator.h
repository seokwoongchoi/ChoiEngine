#pragma once

#include "ColliderSystem.h"

enum class ActorState
{
	Idle,
	Move,
	MoveSide,
	Attack,
	//Attack2, 
	//Attack3,
	//Run,
	Jump,
	StandingReaction,
	Blocking,
	BlockingReaction,
	Die,
	
};
struct KeyframeDesc
{

	int Clip = 0;
	uint CurrFrame = 0;
	uint NextFrame = 0;
	float Time = 0.0f;

};
struct TweenDesc
{
	float TakeTime = 0.2f;
	float TweenTime = 0.0f;
	float ChangeTime = 0.0f;
	ActorState state = ActorState::Idle;

	KeyframeDesc Curr;
	KeyframeDesc Next;

	TweenDesc()
	{
		Curr.Clip = 0;
		Next.Clip = -1;
	}
};
class Animator:public ColliderSystem
{
	friend class Transforms;
	friend class PhysicsSystem;
	friend class Action_Attack;
	friend class Action_Strafe;
	friend class Condition_IsInRange;
	friend class Condition_IsEnemyDead;
	friend class Condition_IsSeeEnemy;
	friend class ActorController;

#ifdef EDITORMODE

	friend class ActorEditor;
#endif
public:
	explicit Animator(ID3D11Device* device);
	~Animator();
private:
	
	Animator(const Animator &) = delete;
	Animator & operator= (const Animator &) = delete;
public:
	void UpdateInstBuffer(ID3D11DeviceContext* context);
	bool ComputeBarier(ID3D11DeviceContext * context);
public:
	class QuadTree* QuadTree()
	{
		return tree;
	}
	void CreateQuadTree();
public:
	//inline bool IsAtacking(const uint& index)
	//{
	//	if (tweenDesc[index].Curr.Clip != 3 && tweenDesc[index].Curr.Clip != 4)return false;
	//	auto& curr = tweenDesc[index].Curr;
	//	const uint& frameCount = clips[curr.Clip]->FrameCount();
	//
	//	if (curr.CurrFrame > 10&& curr.CurrFrame<frameCount-5)
	//	{
	//		return true;

	//	}
	//	return false;
	//}
	//
	 void BoxRender();
public:
	const uint& DrawCount(const uint& actorIndex);
	const uint& PrevDrawCount(const uint& actorIndex);
private:
	Matrix temp;
	void FrustumCulling(const uint& index);
public:
	void BindPipeline(ID3D11DeviceContext* context);
	void Update(const uint& actorIndex);
	void Compute(ID3D11DeviceContext* context, ID3D11UnorderedAccessView* physicsUAV);
	
public:
	void AnimTransformSRV();
	void ReadBone(BinaryReader* r);
	void ReadBoneBox(BinaryReader* r, const uint & actorIndex);
	void ReadBehaviorTree(BinaryReader* r, const uint & actorIndex);
	void ReadClip(const wstring& name);

private:
	class QuadTree* tree;
private:
	class Transforms* transforms;
private:



	inline void PlayNextClip(int instance, int clip, float speed = 1.0f)
	{
		if (tweenDesc[instance].Curr.Clip == clip) return;
		tweenDesc[instance].Next.Clip = clip;
	}

	int saveSword = -1;


	struct TweenData
	{
		float RunningTime;
		float speed;
		bool bContinue;
	}tweenData[20];

	vector<TweenDesc> tweenDesc;

	ID3D11Buffer* tweenBuffer;
private:
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* srv;
private:
	ID3D11Device* device;



	class SkinnedTransform* boneTransfomrs;


	uint maxkeyframe;
	uint maxBoneCount ;
private:
	struct BoneBoxDesc
	{
		Matrix local;
		Matrix ColliderBoxWorld;
		Vector4 factors=Vector4(0,0,0,0);
		
	};
	vector<BoneBoxDesc> colliderBoxData;
	

	ID3D11Texture2D*          boneBoxTexture;
	ID3D11ShaderResourceView* boneBoxSrv;

	vector<shared_ptr<class ModelClip>> clips;
};

