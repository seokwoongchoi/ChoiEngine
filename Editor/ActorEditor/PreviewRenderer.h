#pragma once

	struct ColliderBoxData
	{
		Matrix matrix;
		Matrix  R, T;
		Vector3 scale, position;
		Quaternion q;
		Matrix ColliderBoxWorld;
		Matrix result;
		int Index=-1;

	};
class PreviewRenderer
{
		friend class ActorEditor;
public:
	PreviewRenderer(ID3D11Device* device);
		
	~PreviewRenderer();
public:
	void AnimationUpdate(ID3D11DeviceContext* context);
	void Update(const Vector2& size, const float& camSpeed);
	void Render(ID3D11DeviceContext* context);

private:
	void BoxRender(ID3D11DeviceContext * context,const Matrix& matrix, const Vector3& min, const Vector3& max, const Color& color);
	void CapsuleRender(ID3D11DeviceContext * context, const Matrix& matrix,  const Color& color);
	void DebugRender(ID3D11DeviceContext* context);
public:
	void SaveMeshFile(const wstring& name, const ReadMeshType& meshType);
	void SaveMaterialFile(const wstring& name);
public:
	void CreateSahders(const string& file);
	void ReadMesh(const wstring& file, const wstring & modelName, const ReadMeshType& meshType);
	void ReadEditedMesh(const wstring& file, const wstring & modelName, const ReadMeshType& meshType);
	void ReadMaterial(const wstring& name);
	void ReadEditedMaterial(const wstring& name);
	void ReadClip(const wstring& name);

	void ReadAttachMesh(const wstring& path, const ReadMeshType& meshType, const uint& parentBoneIndex);
	
public:

	void DeleteMesh(const uint& index);
	void BlendMesh(const uint& index);

private:
	
	void BindingStaticMesh();
	void BindingSkeletalMesh();
	void BindingMaterialBone();


	
private:
	ReadMeshType  meshType;
	ID3D11Device * device;
	shared_ptr<class ProgressBar> progress;
private:
	shared_ptr<Texture> preintegratedFG;
	ID3D11ShaderResourceView* skyIRSRV;
private:
	
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	class InputLayout* inputLayout;
private:
	ID3D11VertexShader* VS;
	ID3D11PixelShader* PS;
public:
	
	
	Matrix boxWorld;
	void GetBoxSize(const Vector3& min,const Vector3& max);
	Vector3 boxMin;
	Vector3 boxMax;
	
	Vector3  dest[8], temp[8];
private:
	uint stride;
	uint slot;
	uint offset;
private://RasterizerState
	void CreateStates();
	ID3D11RasterizerState* rsState;
	ID3D11SamplerState   * sampLinear;
	ID3D11BlendState*  AdditiveBlendState;
protected://Meshs
	vector<shared_ptr<class ModelBone>> attachBones;
	vector<shared_ptr<class Mesh>> attachMeshes;
	vector<shared_ptr<class ModelClip>> clips;
	vector<shared_ptr<class Mesh>> meshes;
	vector<shared_ptr<class Mesh>> blendMeshes;
	uint blendMeshIndex;
	vector<shared_ptr<class ModelBone>> bones;
	vector<shared_ptr<class Material>>materials;
	uint meshCount;
	uint blendCount;
	uint boneCount;

	vector< VertexTextureNormalTangent> staticVertices;
	vector< VertexTextureNormalTangentBlend> skeletalVertices;
	vector< uint>indices;
private:
	Vector3 targetPosition;
	float distance;
	Vector2 rotation;
	Matrix view;
	Matrix proj;
	Vector3 viewRight;
	Vector3 viewUp;
private://Cbuffer
	void CreateConstantBuffers();
	void CreateAnimConstantBuffers();
	struct PreviewDesc
	{
		Matrix W;
		Matrix VP;
		
	}previewDesc;

	ID3D11Buffer* worldBuffer;
	ID3D11Buffer* nullBuffer;

	struct EyeDesc
	{
		Vector3 EyePosition;
		float pad;
	}eyeDesc;
	ID3D11Buffer* eyeBuffer;
protected:
	void UpdateCurrFameAnimTransformSRV();
	
private:
	void CreateModelTransformSRV();
	void CreateAnimTransformSRV();
	
	
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* srv;
	ID3D11ShaderResourceView* nullSRV;
	bool bLoaded;
	
protected:
	bool bPause;
	struct KeyframeDesc
	{
		int Clip = 0;
		uint CurrFrame = 0;
		uint NextFrame = 0;
		float Time = 0.0f;
		float RunningTime = 0.0f;
		Vector3 pad=Vector3(0,0,0);
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
	}tweenDesc;
	


	ID3D11Buffer* tweenBuffer;
	
	const Matrix& GetSkinnedMatrix(const uint& index);

	
private:
	struct BoneTransform
	{
		Matrix** Transform;
		BoneTransform()
		{}

		void CreateTransforms(const uint& keyframe, const uint& boneCount)
		{
			this->keyframe = keyframe;
			Transform = new Matrix*[keyframe];

			for (UINT i = 0; i < keyframe; i++)
				Transform[i] = new Matrix[boneCount];
		}
		~BoneTransform()
		{

			for (UINT i = 0; i < keyframe; i++)
				SafeDeleteArray(Transform[i]);

			SafeDeleteArray(Transform);
		}
		uint keyframe;
	};
	uint nuArmedBoneCount;
	BoneTransform* skinTransforms ;
	Matrix* saveParentMatrix;
	uint maxkeyframe ;
	uint unArmedBoneCount;

	Matrix skinnedTransform;
	Matrix invGlobal;
	Matrix parent;
	Matrix S, R, T;
	Matrix animation;

	UINT pageSize;
	void* p;
protected:

	uint boxCount;

	ColliderBoxData colliderBoxData[3];
protected:

	int behaviorTreeIndex;
	private:
		bool bHasCam;
};




