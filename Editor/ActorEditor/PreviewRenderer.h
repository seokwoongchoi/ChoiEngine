#pragma once


class PreviewRenderer
{
	friend class ActorEditor;
public:
	PreviewRenderer(ID3D11Device* device);
		
	~PreviewRenderer();
public:
	void Update(const Vector2& size);
	void Render(ID3D11DeviceContext* context);
public:
	void SaveMeshFile(const wstring& name, const ReadMeshType& meshType);
public:
	void CreateSahders(const string& file);
	void ReadMesh(const wstring& name, const ReadMeshType& meshType);
	void ReadMaterial(const wstring& name);
	void ReadClip(const wstring& name);
public:

	void DeleteMesh(const uint& index);
	void BlendMesh(const uint& index);

private:
	void BindingBone(const ReadMeshType& meshType);
	void BindingStaticMesh();
	void BindingSkeletaMesh();

private:
	shared_ptr<class ProgressBar> progress;
private:
	ID3D11Device * device;
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
	vector<shared_ptr<class Mesh>> meshes;
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
	Vector2 R;
	Matrix view;
	Matrix proj;
	Vector3 viewRight;
	Vector3 viewUp;
private://Cbuffer
	void CreateConstantBuffers();
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
private:
	void CreateModelTransformSRV();
	void CreateAnimTransformSRV();
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* srv;
	ID3D11ShaderResourceView* nullSRV;
	bool bLoaded;
	bool bLoadingStart;
};

