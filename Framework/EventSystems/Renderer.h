#pragma once

class Renderer 
{	
	friend class ColliderSystem;
	friend class Animator;
public:
	Renderer()
		:device(nullptr),vertexBuffer(nullptr), indexBuffer(nullptr), shadowVertexBuffer(nullptr), reflectionPS(nullptr),vs(nullptr), ps(nullptr),
		inputLayout(nullptr), rsState(nullptr), mesh(nullptr),  sampLinear(nullptr), 
		stride(0), offset(0), slot(0), meshCount(0), ID(0), materials{},
		AdditiveBlendState(nullptr), forwardPS(nullptr), blendMesh(nullptr), blendCount(0), blendMeshIndex(0), drawCount(0), sinputLayout(nullptr),
		shadowVS(nullptr), boxMin(0,0,0), boxMax(0,0,0), prevDrawCount(0)
	{}
	~Renderer() = default;

protected:
	uint drawCount;
	uint prevDrawCount;
public:
	bool ReadMesh(BinaryReader* r, const ReadMeshType& meshType);
	template<typename T>
	void BindingMesh(BinaryReader* r);
	

	void ReadMaterial(const wstring& name);
	void BindMaterial();
	void CreateShader(const string& file);
public:
	void Initiallize(ID3D11Device* device,const uint& ID);

	void Depth_PreRender(ID3D11DeviceContext* context);
	void Reflection_PreRender(ID3D11DeviceContext* context);
	void Render(ID3D11DeviceContext* context);
	void Forward_Render(ID3D11DeviceContext* context);
private:
	
	uint ID;

protected:
	ID3D11Device* device;
private:
    ID3D11VertexShader* vs;
	ID3D11VertexShader* shadowVS;
	ID3D11PixelShader* ps;
	
	ID3D11PixelShader* reflectionPS; 
	ID3D11PixelShader* forwardPS;
protected://IA Stage

	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* shadowVertexBuffer;
	ID3D11Buffer* indexBuffer;
protected:
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
protected://Meshs
	class Mesh* mesh;
	class Mesh* blendMesh;
	uint blendMeshIndex;

	vector< shared_ptr<class Material>>materials;
	uint meshCount;
	uint blendCount;
		
private:
	Vector3 boxMin;
	Vector3 boxMax;


};

template<typename T>
inline void Renderer::BindingMesh(BinaryReader * r)
{
	vector< T> vertices;
	stride = sizeof(T);
	vector< Vector3> shadowVertices;
	vector< uint> indices;

	uint totalCount = r->UInt();
	uint totalIndexCount = r->UInt();

	vertices.assign(totalCount, T());
	shadowVertices.assign(totalCount, Vector3());
	indices.assign(totalIndexCount, uint());


	{
		void* ptr = reinterpret_cast<void*>(vertices.data());
		r->Byte(&ptr, sizeof(T) * totalCount);

	}





	{
		void* ptr = reinterpret_cast<void*>(indices.data());
		r->Byte(&ptr, sizeof(uint) *totalIndexCount);

	}


	SafeRelease(vertexBuffer);
	SafeRelease(shadowVertexBuffer);
	SafeRelease(indexBuffer);


	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(T) * totalCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = vertices.data();
	Check(device->CreateBuffer(&desc, &subResource, &vertexBuffer));



	//Thread::Get()->AddTask([&]()
	{
		for (uint i = 0; i < totalCount; i++)
		{
			memcpy(shadowVertices[i], vertices[i].Position, sizeof(Vector3));
		}

		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
		desc.ByteWidth = sizeof(Vector3) * totalCount;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.Usage = D3D11_USAGE_DEFAULT;


		D3D11_SUBRESOURCE_DATA subResource;
		ZeroMemory(&subResource, sizeof(subResource));
		subResource.pSysMem = shadowVertices.data();
		Check(device->CreateBuffer(&desc, &subResource, &shadowVertexBuffer));
	}
	//);



	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(uint) * totalIndexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = indices.data();
	Check(device->CreateBuffer(&desc, &subResource, &indexBuffer));




	vertices.clear();
	vertices.shrink_to_fit();
	shadowVertices.clear();
	shadowVertices.shrink_to_fit();
	indices.clear();
	indices.shrink_to_fit();
}
