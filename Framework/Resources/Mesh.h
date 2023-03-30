#pragma once

class Mesh
{
public:
	friend class Renderer;
	friend class ShadowRenderer;
	
#ifdef EDITORMODE
	friend class PreviewRenderer;
	friend class ActorEditor;
#endif
public:
	Mesh();
	~Mesh();

	void CreateBuffer(ID3D11Device* device);
	void ApplyPipeline(ID3D11DeviceContext* context);
	void ApplyPipeline(ID3D11DeviceContext* context, const uint& prevDrawCount, const uint& actorIndex);
	void ApplyPipelineNoMaterial(ID3D11DeviceContext* context,  const uint& prevDrawCount, const uint& actorIndex);
	void ApplyPipelineReflectionMaterial(ID3D11DeviceContext* context, const uint& prevDrawCount, const uint& actorIndex);
	void ClearPipeline(ID3D11DeviceContext* context);
	void ClearPipelineMaterial(ID3D11DeviceContext* context);
	void ClearPipelineReflection(ID3D11DeviceContext* context);
protected:
	struct BoneDesc
	{
		int boneIndex = -1;
		uint actorIndex = 0;
		uint drawCount = 0;
		uint prevDrawCount = 0;
	} boneDesc;
	ID3D11Buffer* boneBuffer;
	uint vertexCount;
	uint startVertexIndex;
	uint startIndex;
	uint indexCount;
protected:
	shared_ptr<class Material> material;
	bool bHasMaterial;
protected:
	string materialName;
	ID3D11Buffer* nullBuffer;
	string name;
};

class ModelBone
{
public:
	friend class ColliderSystem;
	friend class Animator;
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

	inline int BoneIndex() { return index; }
	inline int ParentIndex() { return parentIndex; }
	shared_ptr<ModelBone> Parent() { return parent; }
	string Name() { return name; }
public:
	const Matrix& Transform() { return transform; }
	void Transform(const Matrix& matrix) { transform = matrix; }
public:
	inline const vector<shared_ptr<ModelBone>>& GetChilds() {return childs;}
	inline shared_ptr<ModelBone>* ChildsData() { 
		if (childs.empty()) return nullptr;

		return childs.data(); }
	
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
	string BoneName;
	vector<ModelKeyframeData> Transforms;
};

class ModelClip
{
public:
	friend class Animator;
	
#ifdef EDITORMODE
	friend class PreviewRenderer;
	friend class ActorEditor;
#endif

public:
	ModelClip() :name(""), duration(0.0f), frameRate(0.0f), frameCount(0)
	{}
	~ModelClip() = default;
public:
	float Duration() { return duration; }
	inline float FrameRate() { return frameRate; }
	inline const uint& FrameCount() { return frameCount; }
public:
	shared_ptr<ModelKeyframe> Keyframe(const string& name)
	{
		
		if (keyframeMap.count(name) < 1)
			return nullptr;

		return keyframeMap[name];
	}
private:
    float duration;
	float frameRate;
	UINT frameCount;

	string name;
	unordered_map<string, shared_ptr<ModelKeyframe>> keyframeMap;

	
	
};

