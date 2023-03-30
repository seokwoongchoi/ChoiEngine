#pragma once

class SSAO
{
public:
	explicit	SSAO(ID3D11Device* device, uint width=1280,uint height=720);
	~SSAO();
	SSAO(const SSAO &) = delete;
	SSAO & operator= (const SSAO &) = delete;
	void Compute(ID3D11DeviceContext* DC, ID3D11ShaderResourceView* DepthSRV, ID3D11ShaderResourceView* NormalsSRV);
	
	inline ID3D11ShaderResourceView* GetSSAOSRV() { return SSAO_SRV; }
	inline ID3D11ShaderResourceView* GetMiniDepthSRV() { return SSAO_SRV; }

	void Resize(const uint &width, const uint& height);


	struct PARAMETERS_SSAO
	{
		float SSAOSampRadius;
		float Radius;
	}Params;
	PARAMETERS_SSAO* GetParams() { return &Params; }
private:

	void DownscaleDepth(ID3D11DeviceContext* DC,ID3D11ShaderResourceView* DepthSRV, ID3D11ShaderResourceView* NormalsSRV);
	
	void ComputeSSAO(ID3D11DeviceContext* DC);
	void CreateSSAOTexture();
	
private:
	uint downScaleGroups;
	UINT width;
	UINT height;

	ID3D11Device* device;
private:
	struct DownscaleDesc
	{
		UINT nWidth;
		UINT nHeight;
		float fHorResRcp;
		float fVerResRcp;
		Vector4 ProjParams;
		Matrix ViewMatrix;
		float fOffsetRadius;
		float fRadius;
		float fMaxDepth;
		UINT pad;
	}DownscaleDesc;
	ID3D11Buffer* DownscaleCB;

private:
	
	// SSAO values for usage with the directional light
	ID3D11Texture2D* SSAO_RT;
	ID3D11UnorderedAccessView* SSAO_UAV;
	ID3D11ShaderResourceView* SSAO_SRV;

	// Downscaled depth buffer (1/4 size)
	ID3D11Buffer* MiniDepthBuffer;
	ID3D11UnorderedAccessView* MiniDepthUAV;
	ID3D11ShaderResourceView* MiniDepthSRV;
	//StructuredBuffer* depthBuffer;
	//StructuredBuffer* ssaoBuffer;
		// Shaders
	ID3D11ComputeShader* DepthDownscaleCS;
	ID3D11ComputeShader* ComputeCS;

	Matrix proj,view;
};
