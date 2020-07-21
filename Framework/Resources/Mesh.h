#pragma once

class Mesh
{
public:
	friend class Renderer; 
	

#ifdef EDITORMODE
	friend class PreviewRenderer;
	friend class ActorEditor;
#endif
public:
	Mesh();
	~Mesh();

	void CreateBuffer(ID3D11Device* device);
	void ApplyPipeline(ID3D11DeviceContext* context);
	void ApplyPipelineNoMaterial(ID3D11DeviceContext* context);
	void ApplyPipelineReflectionMaterial(ID3D11DeviceContext* context);
	void ClearPipeline(ID3D11DeviceContext* context);
	void ClearPipelineMaterial(ID3D11DeviceContext* context);
	void ClearPipelineReflection(ID3D11DeviceContext* context);
	
protected:
	string name;
	shared_ptr<class Material> material;
	
protected:
	

	uint vertexCount;
	uint startVertexIndex;
	uint startIndex;
	uint indexCount;
protected:
	string materialName;
	
	Vector3 minPos;
	Vector3 maxPos;

protected:
	shared_ptr<class ModelBone> bone;
	uint boneIndex;
	struct BoneDesc
	{
		int Index = -1;
		Vector3 lightDirection = Vector3(1, -1, 1);
	} boneDesc;
	ID3D11Buffer* boneBuffer;
	ID3D11Buffer* nullBuffer;
};

class ModelBone
{
public:
	friend class Renderer;
#ifdef EDITORMODE
	friend class PreviewRenderer;
	friend class ActorEditor;
#endif
public:
	ModelBone()
		:parent(nullptr),index(-1), parentIndex(-1)
	{

	}
	~ModelBone()
	{
		childs.clear();
		childs.shrink_to_fit();
		
	}

	int BoneIndex() { return index; }
	int ParentIndex() { return parentIndex; }
	shared_ptr<ModelBone> Parent() { return parent; }
	string Name() { return name; }
public:
	const Matrix& Transform() { return transform; }
	void Transform(const Matrix& matrix) { transform = matrix; }
public:
	vector<shared_ptr<ModelBone>> GetChilds() { return childs; }
	shared_ptr<ModelBone>* ChildsData() { return childs.data(); }
	
protected:
	int index;
	string name;

	int parentIndex;
	shared_ptr<ModelBone> parent;

	Matrix transform;
	
	vector<shared_ptr<ModelBone>> childs;


};


struct ModelKeyframeData
{
	float Time;


	Vector3 Scale;
	Quaternion Rotation;
	Vector3 Translation;
};

struct ModelKeyframe
{
	wstring BoneName;
	vector<ModelKeyframeData> Transforms;
};

class ModelClip
{
public:
	friend class Renderer;
#ifdef EDITORMODE
	friend class PreviewRenderer;
	friend class ActorEditor;
#endif

private:
	ModelClip() :name(L""), duration(0.0f), frameRate(0.0f), frameCount(0)
	{}
	~ModelClip() = default;

public:
	float Duration() { return duration; }
	inline float FrameRate() { return frameRate; }
	inline UINT FrameCount() { return frameCount; }

	ModelKeyframe* Keyframe(const wstring& name)
	{
		if (keyframeMap.count(name) < 1)
			return nullptr;

		return keyframeMap[name];
	}


private:
	wstring name;

	float duration;
	float frameRate;
	UINT frameCount;

	unordered_map<wstring, ModelKeyframe *> keyframeMap;

};