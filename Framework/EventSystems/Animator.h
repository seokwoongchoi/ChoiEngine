#pragma once
#include "ColliderSystem.h"
class Animator:public ColliderSystem
{
public:
	Animator(ID3D11Device* device);
	~Animator();



	void PushDrawCount(const uint& index, const Matrix& world) override;
public:
	void BindPipeline(ID3D11DeviceContext* context);
	void Update(const uint& actorIndex);
public:
	void AnimTransformSRV();
	void ReadBone(BinaryReader* r);
	void ReadClip(const wstring& name);
	vector<shared_ptr<class ModelClip>> clips;
private:
	void SortTweenBuffer(const uint & actorIndex, const uint & drawIndex);
private:
	ID3D11Device* device;
private:
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* srv;
private:

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
		uint index=0;

		KeyframeDesc Curr;
		KeyframeDesc Next;

		TweenDesc()
		{
			Curr.Clip = 0;
			Next.Clip = -1;
		}
	};

	float RunningTime[20];
	vector<TweenDesc> tweenDesc;

	ID3D11Buffer* tweenBuffer;
	class SkinnedTransform* boneTransfomrs;


	uint maxkeyframe;
	uint maxBoneCount ;
private:
	struct BoneBoxDesc
	{
		Matrix loca[3];
		Matrix BoneScale[3];
		int Index[3];
		float pad;
	};
	vector<BoneBoxDesc> boneBoxDesc;
	ID3D11Buffer* boneBoxBuffer;
};

