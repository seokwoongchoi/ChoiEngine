#pragma once


class BoneTransform
{
public:
	BoneTransform()=default;
	~BoneTransform() = default;
	
	//map<uint, vector<shared_ptr<class ModelBone>>> bones;
	map<uint, vector<shared_ptr<class ModelBone>>> bones;
	
	Matrix boneTransforms[MAX_ACTOR_COUNT][MAX_BONE_TRANSFORMS];
	Matrix temp[MAX_BONE_TRANSFORMS];
};

class SkinnedTransform
{
public:
	SkinnedTransform() :keyframe(0)
	{}
	~SkinnedTransform()
	{};

	
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
	map<uint, vector<shared_ptr<class ModelBone>>> bones;

	Matrix** Transform=nullptr;
	Matrix* saveTransforms = nullptr;

	void Clear()
	{
		if (Transform == nullptr|| keyframe==0)return;

		for (UINT i = 0; i < keyframe; i++)
			SafeDeleteArray(Transform[i]);

		SafeDeleteArray(Transform);
		SafeDeleteArray(saveTransforms);
	}


	
private:
	uint keyframe;
	
};

