#pragma once

#define MAX_BONE_TRANSFORMS 100
#define MAX_ACTOR_COUNT 10
#define MAX_MODEL_INSTANCE 20

class InstTransform
{
public:
	InstTransform() = default;
	~InstTransform() = default;

	
	Matrix instTransforms[MAX_ACTOR_COUNT][MAX_MODEL_INSTANCE];
	
};
class BoneTransform
{
public:
	BoneTransform()=default;
	~BoneTransform() = default;
	
	vector<shared_ptr<class ModelBone>> bones;
	Matrix boneTransforms[MAX_ACTOR_COUNT][MAX_BONE_TRANSFORMS];
	Matrix temp[MAX_BONE_TRANSFORMS];
};

class SkinnedTransform
{
public:
	SkinnedTransform() = default;
	
	~SkinnedTransform()
	{
		for (UINT i = 0; i < keyframe; i++)
			SafeDeleteArray(Transform[i]);

		SafeDeleteArray(Transform);
	};

	void CreateTransforms(const uint& keyframe, const uint& boneCount)
	{
		this->keyframe = keyframe;
		Transform = new Matrix*[keyframe];

		for (UINT i = 0; i < keyframe; i++)
			Transform[i] = new Matrix[boneCount];

		saveTransforms = new Matrix[boneCount];
	}

	void CreateTransforms()
	{
		this->keyframe = 300;
		Transform = new Matrix*[keyframe];

		for (UINT i = 0; i < keyframe; i++)
			Transform[i] = new Matrix[MAX_BONE_TRANSFORMS];

		saveTransforms = new Matrix[MAX_BONE_TRANSFORMS];
	}

	vector<shared_ptr<class ModelBone>> bones;
	Matrix** Transform;
	Matrix* saveTransforms;
	
private:
	uint keyframe;

};

