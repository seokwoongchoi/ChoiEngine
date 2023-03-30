#pragma once
class Scattering
{
public:
	explicit Scattering(ID3D11Device* device);
	~Scattering();
	Scattering(const Scattering&) = delete;
	Scattering& operator=(const Scattering&) = delete;

	
	void Render(ID3D11DeviceContext* context);

	
private:
	void CreateDoom();
	void CreateShader();
private:
	ID3D11Buffer* domeVertexBuffer;
	ID3D11Buffer* domeIndexBuffer;
	uint domeVertexCount;
	uint domeIndexCount;
	uint domeCount;
private:
	struct CB_World
	{
		Matrix WVP;
	
	}worldDesc;
	Matrix world;
	Matrix vp;
	struct CB_Pixel
	{
		Vector3 LightDir;
		float Time=0.0f;
	}pixelDesc;
	
	ID3D11Buffer* worldBuffer;
	ID3D11Buffer* nullBuffer;
	ID3D11Buffer* pixelBuffer;
	ID3D11SamplerState* LinearSampler;
private:
	ID3D11VertexShader* vs;
	ID3D11PixelShader* ps;
	InputLayout* inputLayout;
private:
	Texture* texture;
	float theta;
	uint slot = 0;
	uint stride = sizeof(VertexTexture);

private:
	ID3D11DepthStencilState*  depthStencilState;
	ID3D11Device* device;
	ID3D11BlendState*  AdditiveBlendState;
};

