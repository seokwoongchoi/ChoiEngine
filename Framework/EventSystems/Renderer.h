#pragma once
struct shadowDesc
{
	Vector3 Position;
	Vector4 Indices;
	Vector4 Weights;
};
class Renderer 
{
	friend class Transforms;
	friend class ColliderSystem;
	friend class Animator;
public:
	explicit Renderer()
		:device(nullptr),vertexBuffer(nullptr), indexBuffer(nullptr), shadowVertexBuffer(nullptr), reflectionPS(nullptr),vs(nullptr), ps(nullptr),
		inputLayout(nullptr), rsState(nullptr), mesh(nullptr),  sampLinear(nullptr), 
		stride(0), offset(0), slot(0), meshCount(0), ID(0), materials{},
		AdditiveBlendState(nullptr), forwardPS(nullptr), blendMesh(nullptr), blendCount(0), blendMeshIndex(0), drawCount(0), sinputLayout(nullptr),
		shadowVS(nullptr), boxMin(0,0,0), boxMax(0,0,0), prevDrawCount(0), shadowStride(0), btIndex(-1), reflectionVS(nullptr), bMaintainShadow(false)
	{}
	~Renderer() = default;

	void SetMaintainShadow(bool bMaintainShadow)
	{
		this->bMaintainShadow = bMaintainShadow;
	}
private:

	Renderer(const Renderer &) = delete;
	Renderer & operator= (const Renderer &) = delete;

protected:
	uint drawCount;
	uint prevDrawCount;
public:
	bool ReadMesh(BinaryReader* r, const ReadMeshType& meshType);
	
	void BindingStaticMesh(BinaryReader* r);
	void BindingSkeletalMesh(BinaryReader* r);

	
	

	void ReadMaterial(const wstring& name);
	void BindMaterial();
	void CreateShader(const string& file);
public:
	void Initiallize(ID3D11Device* device,const uint& ID);

	void Depth_PreRender(ID3D11DeviceContext* context);
	void Reflection_PreRender(ID3D11DeviceContext* context);
	void Render(ID3D11DeviceContext* context);
	void Forward_Render(ID3D11DeviceContext* context);
protected://Meshs
	class Mesh* mesh;
	class Mesh* blendMesh;
	uint blendMeshIndex;

	vector< shared_ptr<class Material>>materials;
	uint meshCount;
	uint blendCount;
protected:
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* shadowVertexBuffer;
	ID3D11Buffer* indexBuffer;
	
	shared_ptr<class InputLayout> inputLayout;
	shared_ptr<class InputLayout> sinputLayout;
	uint shadowStride;
	uint stride;
	uint slot;
	uint offset;
protected:
	ID3D11Device* device;
private:
    ID3D11VertexShader* vs;
	ID3D11VertexShader* shadowVS;
	ID3D11PixelShader* ps;

	ID3D11VertexShader* reflectionVS;
	ID3D11PixelShader* reflectionPS; 
	ID3D11PixelShader* forwardPS;



private://RasterizerState
	void CreateStates();
	ID3D11RasterizerState* rsState;
	ID3D11BlendState*  AdditiveBlendState;
	ID3D11SamplerState*    sampLinear;

		
private:
	Vector3 boxMin;
	Vector3 boxMax;


private:
	int btIndex;
	bool bMaintainShadow;
	uint ID;
};

