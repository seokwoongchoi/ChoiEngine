#pragma once
struct CB_PSDesc
{
	float Time = 0.0f;
	float Cover = 3.0f;
	float Sharpness = 0.928f;
	float Speed = 0.004f;
	Vector2 SecondOffset = Vector2(0.65f, 0.34f);
	Vector2 CloudTiles = Vector2(2.0f, 2.0f);

};


class Cloud
{
public:
	explicit Cloud(ID3D11Device* device);
	~Cloud();
	Cloud(const Cloud&) = delete;
	Cloud& operator=(const Cloud&) = delete;
	
	void Render(ID3D11DeviceContext* context);
	void Render(ID3D11DeviceContext* context,const Matrix& VP);
	void Create();
	void CreateShader();
	CB_PSDesc* GetParams()
	{
		return &psDesc;
	}
	float* GetScale()
	{
		return &scale;
	}

private:
	Matrix S, T,VP;
	Vector3 position = Vector3(0, 0, 0);
	uint skyPlaneResolution = 1024;
	float textureRepeat = 1.5f;
	float skyPlaneWidth = 128.0f;
	float skyPlaneTop = 15.0f;
	float skyPlaneBottom = -15.0f;
	

	
	ID3D11Texture2D* texture;

	Texture* noiseTexture;
	Texture* cloudTexture1;
	Texture* cloudTexture2;
	
	ID3D11ShaderResourceView* srv;

	ID3D11Buffer* cloudVertexBuffer;
	ID3D11Buffer* cloudIndexBuffer;
	uint cloudVertexCount;
	uint cloudIndexCount;

	
	struct CB_VSDesc
	{
		Matrix wvp;

	} vsDesc;
	
	ID3D11Buffer* vsBuffer;

	
	CB_PSDesc psDesc;

	ID3D11Buffer* psBuffer;
	
	float scale = 20.0f;
	ID3D11Device* device;
	ID3D11SamplerState* LinearSampler;
	ID3D11BlendState*  AdditiveBlendState;
	ID3D11DepthStencilState*  depthStencilState;
private:
	ID3D11VertexShader* vs;
	ID3D11PixelShader* ps;
	InputLayout* inputLayout;
};

