#pragma once
#include "ColliderSystem.h"
class Animator:public ColliderSystem
{
public:
	Animator(ID3D11Device* device);
	~Animator();


public:
	void BindPipeline(ID3D11DeviceContext* context);
	void Update(const uint& index);
public:
	void AnimTransformSRV();
	void ReadBone(BinaryReader* r);
	void ReadClip(const wstring& name);
	vector<shared_ptr<class ModelClip>> clips;

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
		float RunningTime = 0.0f;
		Vector3 pad = Vector3(0, 0, 0);
	};

	struct TweenDesc
	{
		float TakeTime = 0.2f;
		float TweenTime = 0.0f;
		float ChangeTime = 0.0f;
		float pad;

		KeyframeDesc Curr;
		KeyframeDesc Next;

		TweenDesc()
		{
			Curr.Clip = 0;
			Next.Clip = -1;
		}
	};

	TweenDesc tweenDesc[20];

	ID3D11Buffer* tweenBuffer;
	class SkinnedTransform* boneTransfomrs;


	uint maxkeyframe;
	uint maxBoneCount ;

};

