#pragma once
#define MAX_MODEL_INSTANCE 20

class Renderer 
{
public:
	Renderer()
		:device(nullptr),vertexBuffer(nullptr), indexBuffer(nullptr), shadowVertexBuffer(nullptr), reflectionPS(nullptr),nullSRV(nullptr), nullBuffer(nullptr), vs(nullptr), ps(nullptr),
		inputLayout(nullptr), rsState(nullptr), mesh(nullptr), texture(nullptr), srv(nullptr), sampLinear(nullptr), 
		worldBuffer(nullptr), stride(0), offset(0), slot(0), meshCount(0), boneCount(0), ID(0), modelDesc{}, bones{}, materials{},
		AdditiveBlendState(nullptr), forwardPS(nullptr), blendMesh(nullptr), blendCount(0), blendMeshIndex(0), drawCount(0), sinputLayout(nullptr),
		shadowVS(nullptr)
	{}
	~Renderer() = default;

	void PushDrawCount(const Vector3& pos,const Vector3& scale)
	{
		Matrix S, T;
		D3DXMatrixScaling(&S, scale.x, scale.y, scale.z);
		D3DXMatrixTranslation(&T, pos.x, pos.y, pos.z);
		modelDesc.instTransform[drawCount] = S * T;
		D3DXMatrixTranspose(&modelDesc.instTransform[drawCount], &modelDesc.instTransform[drawCount]);
		drawCount++;

	
	}
	

	bool ReadMesh(const wstring& name,const ReadMeshType& meshType);
	void ReadMaterial(const wstring& name);
	void CreateShader(const string& file);
public:
	void Initiallize(ID3D11Device* device,const uint& ID);
	void Update(ID3D11DeviceContext* context);
	void Depth_PreRender(ID3D11DeviceContext* context);
	void Reflection_PreRender(ID3D11DeviceContext* context);
	void Render(ID3D11DeviceContext* context);
	void Forward_Render(ID3D11DeviceContext* context);
private:
	uint drawCount;
	uint ID;

private://Shaders
	ID3D11Device* device;
private:
	ID3D11VertexShader* vs;
	ID3D11VertexShader* shadowVS;
	ID3D11PixelShader* ps;
	
	ID3D11PixelShader* reflectionPS; 
	ID3D11PixelShader* forwardPS;
private://IA Stage

	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* shadowVertexBuffer;
	ID3D11Buffer* indexBuffer;
private:
	shared_ptr<class InputLayout> inputLayout;
	shared_ptr<class InputLayout> sinputLayout;
	uint stride;
	uint slot;
	uint offset;

private://RasterizerState
	void CreateStates();
	ID3D11RasterizerState* rsState;
	ID3D11BlendState*  AdditiveBlendState;
	ID3D11SamplerState*    sampLinear;
private://Meshs
	class Mesh* mesh;
	class Mesh* blendMesh;
	uint blendMeshIndex;
	vector<shared_ptr<class ModelBone>> bones;
	vector< shared_ptr<class Material>>materials;
	uint meshCount;
	uint blendCount;
	uint boneCount;

	
	
	void BindingBone(const ReadMeshType& meshType);
	
	void BindingStaticMesh(BinaryReader* r);
	void BindingSkeletalMesh(BinaryReader* r);

private://Vertex Resources
	void CreateModelTransformSRV();
	void CreateModelAnimationSRV();
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* srv;
	ID3D11ShaderResourceView* nullSRV;
	

private://Cbuffer
	void CreateConstantBuffers();
	struct ModelData
	{
		Matrix instTransform[MAX_MODEL_INSTANCE];
  	}modelDesc;
	
	ID3D11Buffer* worldBuffer;
	ID3D11Buffer* nullBuffer;
};

