#pragma once
#define MAX_CLIP_SIZE 10
#include "ColliderSystem.h"
#include "Resources/Mesh.h"
enum class ActorState:uint
{
	Idle,
	Move,
	Attack2,
	Attack,
	//Attack3,
	Run,
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
struct TweenData
{
	
	float TweenTime = 0.0f;
	ActorState state = ActorState::Idle;
	float RunningTime = 0.0f;
	float speed = 1.0f;

	KeyframeDesc Curr;
	KeyframeDesc Next;


	uint index = 0;

	bool IsStopped =false;
	int farmeDelta = 0;
	uint quadTreeID = 0;

	uint drawCount = 0;
	uint maxBoneCount = 0;
	uint ThirdquadTreeID = 0;
	uint LastquadTreeID = 0;
	TweenData()
	{
		Curr.Clip = 0;
		Next.Clip = -1;
	}

};


class Animator :public ColliderSystem
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
	inline ID3D11Buffer* TweenBuffer() {return tweenBuffer;}
	inline ID3D11ShaderResourceView* SRV() { return outputSRV; }
inline bool IsAtacking(const uint& index)
{
	/*if (tweenData[index].Curr.Clip!=static_cast<uint>(ActorState::Attack)
		&& tweenData[index].state != ActorState::Attack2)return false;*/

	if (tweenData[index].Curr.Clip == static_cast<uint>(ActorState::Attack))
	{
		auto& curr = tweenData[index].Curr;
		if (curr.CurrFrame > 17 && curr.CurrFrame < 23)
		{
		
			return true;

		}
		return false;
	}
	/*else
	{
		auto& curr = tweenData[index].Curr;
		if ( curr.CurrFrame > 39&& curr.CurrFrame < 43)
		{
			return true;

		}
		return false;
	}*/
	return false;
	
}
inline void PlayNextClip(int instance, int clip)
{
	auto& tween = tweenData[instance];

	if (tween.Curr.Clip == clip) return;

	tween.Next.Clip = clip;

	
}
public:
	uint FrustumCulling(const uint& index);
public:
	void BindPipeline(ID3D11DeviceContext* context); 
	uint AnimationUpdate(const uint& actorIndex);
	void UpdateNextAnim(const uint& actorIndex);
	void Update();
	void Update(bool bstart);
	void Compute(ID3D11DeviceContext* context, ID3D11UnorderedAccessView* physicsUAV);
	vector<TweenData> tweenData;
private:
	vector<shared_ptr<class ModelClip>> clips;
	struct TweenDesc
	{
		float TweenTime = 0.0f;
		uint drawCount = 0;
		uint maxBoneCount = 0;
		uint index = 0;

		KeyframeDesc Curr;
		KeyframeDesc Next;

		TweenDesc()
		{
			Curr.Clip = 0;
			Next.Clip = -1;
		}
	};
	
	vector<TweenDesc> tweenDesc;
	ID3D11Buffer* tweenBuffer;
private:

	ID3D11Texture2D* outputTexture;
	ID3D11ShaderResourceView* outputSRV;
	ID3D11UnorderedAccessView* outputUAV;
//private:
//	ID3D11Buffer			  *animationBuffer = nullptr;
//	ID3D11ShaderResourceView* animationSRV = nullptr;
	//ID3D11UnorderedAccessView* animationUAV = nullptr;
private:
	class QuadTree* tree;
	class Transforms* transforms;
private:
	ID3D11Device* device;
	class SkinnedTransform* boneTransfomrs;
	uint maxkeyframe;
	uint maxBoneCount;

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
public:
	void AnimTransformSRV();
	void ClearTextureTransforms();
public:
	void ReadBone(BinaryReader* r, uint actorIndex);
	void ReadBoneBox(BinaryReader* r, const uint & actorIndex);
	void ReadBehaviorTree(BinaryReader* r, const uint & actorIndex);
	void ReadClip(const wstring& name,const uint& actorIndex);
public:
	void IintAnimData();
public:
	class QuadTree* QuadTree() { return tree; }

	void PushDrawCount(const uint& index, const Matrix& world);

private:
	bool IsFirstClipRead[MAX_SKELETAL_ACTOR_COUNT];
	bool IsEdittedClip[MAX_SKELETAL_ACTOR_COUNT];
};

