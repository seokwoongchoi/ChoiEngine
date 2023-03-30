#pragma once

class GBufferData
{
public:
	GBufferData() :width(1280), height(720), device(nullptr),
		nullBuffer(nullptr) , depthStencilTexture(nullptr), diffuseTexture(nullptr), normalTexture(nullptr), specularTexture(nullptr), depthStencilDSV(nullptr),
		depthStencilReadOnlyDSV(nullptr), diffuseRTV(nullptr), normalRTV(nullptr), specularRTV(nullptr), depthStencilSRV(nullptr),
		diffuseSRV(nullptr), normalSRV(nullptr), specularSRV(nullptr), depthStencilState(nullptr), packingVP{}
	
	{}
	~GBufferData() {}

	void Init(ID3D11Device* device,uint width = 1280, uint height = 720);
	void Destroy();
	void Update();
	void Resize(uint width, uint height);
	
	void PrepareForPacking(ID3D11DeviceContext* context);
	void PrepareForWaterPacking(ID3D11DeviceContext* context);
	void PrepareForUnpack(ID3D11DeviceContext* context);
	void PostRender(ID3D11DeviceContext* context);
public:
	inline ID3D11ShaderResourceView* DiffuseSRV(){return diffuseSRV;}
	inline ID3D11ShaderResourceView* SpecularSRV(){return specularSRV;}
	
	inline ID3D11DepthStencilView* DepthstencilDSV(){return depthStencilDSV;}
	inline ID3D11DepthStencilView* DepthstencilDSVReadOnly(){return depthStencilReadOnlyDSV;}
public:
	inline ID3D11ShaderResourceView* NormalSRV(){return normalSRV;	}
	inline ID3D11ShaderResourceView* DepthstencilSRV(){	return depthStencilSRV;	}
public:
	inline ID3D11Texture2D* DiffuseTexture(){	return diffuseTexture;}
	inline ID3D11Texture2D* NormalTexture() { return normalTexture; }
	inline ID3D11Texture2D* PBRTexture() { return specularTexture; }
private:
	ID3D11Device* device;
private:

	
	void CreateViews();
	// GBuffer textures
	ID3D11Texture2D* depthStencilTexture;
	ID3D11Texture2D* diffuseTexture;
	ID3D11Texture2D* normalTexture;
	ID3D11Texture2D* specularTexture;
	

	// GBuffer render views
	ID3D11DepthStencilView* depthStencilDSV;
	ID3D11DepthStencilView* depthStencilReadOnlyDSV;
	ID3D11RenderTargetView* diffuseRTV;
	ID3D11RenderTargetView* normalRTV;
	ID3D11RenderTargetView* specularRTV;


	// GBuffer shader resource views
	ID3D11ShaderResourceView* depthStencilSRV;
	ID3D11ShaderResourceView* diffuseSRV;
	ID3D11ShaderResourceView* normalSRV;
	ID3D11ShaderResourceView* specularSRV;

	

	ID3D11DepthStencilState * depthStencilState;

protected:
	struct CB_GBufferUnpack
	{
		Vector4 PerspectiveValuse;
		Matrix ViewInv;
	};
	CB_GBufferUnpack pGBufferUnpackCB;
	ID3D11Buffer* unpackBuffer;
	ID3D11Buffer* nullBuffer;
	
private:
	Matrix proj,view;

	uint width;
	uint height;

	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	D3D11_VIEWPORT packingVP;
};

