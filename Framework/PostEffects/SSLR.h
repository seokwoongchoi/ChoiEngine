#pragma once
class SSLR
{
public:
	explicit	SSLR(ID3D11Device* device,uint widht=1280,uint height=720);
	~SSLR();
	// Render the screen space light rays on top of the scene
	void Render(ID3D11DeviceContext* DC, ID3D11ShaderResourceView* ssaoSRV, ID3D11RenderTargetView* LightAccumRTV);
	


	struct PARAMETERS_SSLR
	{
		float MaxSunDist;
		float intensity;
		float decay;
		float ddecay;

		float dist;
		float MaxDeltaLen;
	}Params;
	PARAMETERS_SSLR* GetParams() { return &Params; }
private:

	// Prepare the occlusion map
	void PrepareOcclusion(ID3D11DeviceContext* DC, ID3D11ShaderResourceView* ssaoSRV);

	// Ray trace the occlusion map to generate the rays
	void RayTrace(ID3D11DeviceContext* DC, const Vector2& SunPosSS, const Vector3& SunColor);


	// Combine the rays with the scene
	void Combine(ID3D11DeviceContext* DC, ID3D11RenderTargetView* LightAccumRTV);

private:
	// Shaders
	ID3D11Buffer*  OcclusionCB;
	ID3D11ComputeShader*  OcclusionCS;
	ID3D11Buffer*  RayTraceCB;
	ID3D11VertexShader*  FullScreenVS;
	ID3D11PixelShader*  RayTracePS;
	ID3D11PixelShader*  CombinePS;


	uint downScaleGroups;


private:

	struct CB_OCCLUSSION
	{
		UINT Width;
		UINT Height;
		UINT pad[2];
	}occlusionDesc;
	
	struct CB_LIGHT_RAYS
	{
		Vector2 SunPos;
		float InitDecay;
		float DistDecay;
		Vector3 RayColor;
		float MaxDeltaLen;
	}lightRayDesc;
	
private:
	bool ShowRayTraceRes;
	uint width;
	uint height;

	ID3D11Texture2D* OcclusionTex;
	ID3D11UnorderedAccessView* OcclusionUAV; 
	ID3D11ShaderResourceView* OcclusionSRV;


	ID3D11Texture2D* LightRaysTex;
	ID3D11RenderTargetView* LightRaysRTV;
	ID3D11ShaderResourceView* LightRaysSRV;
private:

	ID3D11BlendState* AdditiveBlendState;

	
	Vector3 dir;
	Vector3 SunPos;
	Vector3 EyePos;
	Matrix View;
	Matrix Proj;
	Matrix ViewProjection;
	Vector3 SunPosSS;
	


	D3D11_VIEWPORT oldvp;
	UINT num = 1;
	Vector3 yellow = Vector3(0.8f, 0.77f, 0.6f);
	Vector3 white=Vector3(0.93f, 0.9f, 0.73f);
	
	float factor;
};

