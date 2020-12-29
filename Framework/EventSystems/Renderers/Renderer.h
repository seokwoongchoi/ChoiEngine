#pragma once

class Renderer 
{
	friend class Transforms;
	friend class ColliderSystem;
	friend class Animator;
public:
	explicit Renderer()
		:device(nullptr),vertexBuffer(nullptr), shadowVertexBuffer(nullptr),indexBuffer(nullptr), reflectionPS(nullptr),vs(nullptr), ps(nullptr),
		inputLayout(nullptr), rsState(nullptr), mesh(nullptr),  sampLinear(nullptr), 
		stride(0), offset(0), slot(0), meshCount(0), ID(0), materials{},
		AdditiveBlendState(nullptr), forwardPS(nullptr), blendMesh(nullptr), blendCount(0), blendMeshIndex(0),
		boxMin(0,0,0), boxMax(0,0,0), btIndex(-1), reflectionVS(nullptr), AlphaToCoverageEnable(nullptr)
	{}
	~Renderer() = default;

	
private:

	Renderer(const Renderer &) = delete;
	Renderer & operator= (const Renderer &) = delete;
public:
	void Reflection_PreRender(ID3D11DeviceContext* context, const uint& drawCount, const uint& prevDrawCount);
	void Render(ID3D11DeviceContext* context, const uint& drawCount, const uint& prevDrawCount);
	void Forward_Render(ID3D11DeviceContext* context, const uint& drawCount, const uint& prevDrawCount);
public:
	bool ReadMesh(BinaryReader* r, const ReadMeshType& meshType);
	
	void BindingStaticMesh(BinaryReader* r);
	void BindingSkeletalMesh(BinaryReader* r);
	void ReadMaterial(const wstring& name);
	void BindMaterial();
	void CreateShader(const string& file,bool IsTree);

	void Initiallize(ID3D11Device* device,const uint& ID);

public:
	class Mesh* GetMesh();
	
	const uint& GetMeshCount()
	{
		return meshCount;
	}

	ID3D11Buffer* GetShadowBuffer()
	{
		return shadowVertexBuffer;
	}
	ID3D11Buffer* GetIndexBuffer()
	{
		return indexBuffer;
	}
protected://Meshs
	class Mesh* mesh;
	class Mesh* blendMesh;
	uint blendMeshIndex;

	vector< shared_ptr<class Material>>materials;
	uint meshCount;
	uint blendCount;
protected:
	ID3D11Buffer* shadowVertexBuffer;
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	
	shared_ptr<class InputLayout> inputLayout;
	
	
	uint stride;
	uint slot;
	uint offset;
private:
	ID3D11VertexShader* vs;

	ID3D11PixelShader* ps;

	ID3D11VertexShader* reflectionVS;
	ID3D11PixelShader* reflectionPS;
	ID3D11PixelShader* forwardPS;


private://RasterizerState
	void CreateStates();
	ID3D11RasterizerState* rsState;
	ID3D11BlendState*  AdditiveBlendState;
	ID3D11BlendState*  AlphaToCoverageEnable;
	ID3D11SamplerState*    sampLinear;

		
private:
	Vector3 boxMin;
	Vector3 boxMax;

protected:
	ID3D11Device* device;
	int btIndex;
	uint ID;
	bool bComplete = false;
	bool bAlphaToCoverage = false;
};

