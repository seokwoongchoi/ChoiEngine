#pragma once
#define MAX_CLIP_SIZE 10
#include "ColliderSystem.h"
#include "Resources/Mesh.h"
enum class ActorState
{
	Idle,
	Move,
	MoveSide,
	Attack,
	//Attack2, 
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

	bool bContinue;
	int farmeDelta = 0;
	bool IsStopped = false;
	uint drawCount = 0;
	uint maxBoneCount = 0;

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
	if (tweenData[index].state!=ActorState::Attack)return false;
	auto& curr = tweenData[index].Curr;
	

	if (curr.CurrFrame > 16&& curr.CurrFrame<23)
	{
		return true;

	}
	return false;
}
inline void PlayNextClip(int instance, int clip)
{
	if (tweenData[instance].Curr.Clip == clip) return;
	tweenData[instance].Next.Clip = clip;
}
public:
	uint FrustumCulling(const uint& index);
public:
	void BindPipeline(ID3D11DeviceContext* context);
	uint Update(const uint& actorIndex);
	void Compute(ID3D11DeviceContext* context, ID3D11UnorderedAccessView* physicsUAV);
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
	vector<TweenData> tweenData;
	vector<TweenDesc> tweenDesc;
	ID3D11Buffer* tweenBuffer;
private:

	ID3D11Texture2D* outputTexture;
	ID3D11ShaderResourceView* outputSRV;
	ID3D11UnorderedAccessView* outputUAV;
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
	void ReadClip(const wstring& name);
public:
	void IintAnimData();
public:
	class QuadTree* QuadTree() { return tree; }
	void CreateQuadTree();
	void BoxRender();
};

