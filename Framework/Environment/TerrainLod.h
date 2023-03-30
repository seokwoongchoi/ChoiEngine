#pragma once
class TerrainLod
{
public:
	TerrainLod(ID3D11Device* device, ID3D11ShaderResourceView* heightSRV=nullptr);
	~TerrainLod();
public:
	void Update();
	void Render(ID3D11DeviceContext* context);
	void RunComputeShader(ID3D11DeviceContext* context);
private:
	void ReadPixel();
	void UpdateHeightMap();
	void CreateVertexData();
	void CreateIndexData();
	void CreateShaders();
private:
	static const int			TERRAIN_X_LEN = 64;
	static const int			TERRAIN_Z_LEN = 64;
private:

	Matrix world,vp;
	ID3D11Buffer* mainCBuffer;//VS
	struct PatchDesc
	{
		Vector4 cameraPosition; //  w unused 
		Vector4 minMaxDistance; // zw unused
		Vector4 minMaxLOD; // zw unused
		Vector4 heightMapDimensionsHS;
	}patchDesc;
	ID3D11Buffer* patchCBuffer;//HS

	struct SampleparamsDesc
	{
		Matrix ViewProj;
		Matrix InvTposeWorld;
		Vector4 heightMapDimensionsDS; // zw unused
		Vector4 minMaxLOD;
	}sampleparamsDesc;
	ID3D11Buffer* sampleparamsCBuffer;//DS
private:
	ID3D11Texture2D*           computeTexture;
	ID3D11ShaderResourceView*  computeSRV;
	ID3D11UnorderedAccessView* computeUAV;
	ID3D11ShaderResourceView* heightSRV;
	float textureWidth;
	float textureHeight;
private:
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;
	uint indexCount;
	shared_ptr<class InputLayout> inputLayout;
	uint stride;
	uint slot;
	uint offset;
private:
	ID3D11SamplerState*    sampLinear;
	ID3D11RasterizerState* rsState;
private:
	ID3D11VertexShader* vs;
	ID3D11HullShader*   hs;
	ID3D11DomainShader* ds;
	ID3D11GeometryShader* gs;
	ID3D11PixelShader*  ps;
	ID3D11ComputeShader*  cs;
private:
	ID3D11Device* device;
	Texture* heightMap;
};
