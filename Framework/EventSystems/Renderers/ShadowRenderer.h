#pragma once
class ShadowRenderer
{
public:
	explicit ShadowRenderer():ID(0),slot(0), offset(0), shadowStride(0), meshCount(0), shadowVertexBuffer(nullptr), indexBuffer(nullptr),
		inputLayout(nullptr), mesh(nullptr), shadowVS(nullptr), device(nullptr), bMaintainShadow(false)
	{

	}
	~ShadowRenderer() = default;

private:
	ShadowRenderer(const ShadowRenderer &) = delete;
	ShadowRenderer & operator= (const ShadowRenderer &) = delete;

public:
	void Depth_PreRender(ID3D11DeviceContext* context, const uint& drawCount, const uint&prevCount);
public:
	void Initiallize(ID3D11Device* device, const uint& ID);
	void CreateShader(const string& file);
	void SetMesh(class Mesh* mesh, const uint& meshCount)
	{
		this->meshCount = meshCount;
		this->mesh = mesh;
	}
	void SetShadowVertexBufferData(ID3D11Buffer* shadowVertexBuffer, ID3D11Buffer* indexBuffer)
	{
		this->shadowVertexBuffer = shadowVertexBuffer;
		this->indexBuffer = indexBuffer;
	}
	void SetShadowStride(ReadMeshType meshType)
	{
		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
			shadowStride = sizeof(Vector3);
			break;
		case ReadMeshType::SkeletalMesh:
			shadowStride = sizeof(shadowDesc);
			break;
	
		}
	}

	void SetMaintainShadow(bool bMaintainShadow)
	{
		this->bMaintainShadow = bMaintainShadow;
	}
private:
	class Mesh* mesh;
	ID3D11Buffer* shadowVertexBuffer;
	ID3D11Buffer* indexBuffer;
	shared_ptr<class InputLayout> inputLayout;
	uint slot;
	uint offset;
	uint shadowStride;
    uint  meshCount;

	bool bMaintainShadow;

private:
	uint ID;
	ID3D11VertexShader* shadowVS;
	ID3D11Device* device;
	
};