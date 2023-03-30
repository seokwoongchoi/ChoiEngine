#pragma once
class HDR
{
public:
	explicit HDR(ID3D11Device* device, uint width = 1280, uint height = 720);
	~HDR();
	HDR(const HDR &) = delete;
	HDR & operator= (const HDR &) = delete;
	
	void PostProcessing(ID3D11DeviceContext* DC, ID3D11ShaderResourceView* pHDRSRV, ID3D11RenderTargetView * oldTarget, ID3D11ShaderResourceView* dsv);
	void DownScale(ID3D11DeviceContext* DC,ID3D11ShaderResourceView* srv);
	
	void Bloom(ID3D11DeviceContext* DC);
	void BokehHightlightScan(ID3D11ShaderResourceView* pHDRSRV, ID3D11ShaderResourceView* pDepthSRV);
	void Blur(ID3D11DeviceContext* DC,ID3D11ShaderResourceView* pInput, ID3D11UnorderedAccessView* pOutput);
	void FinalPass(ID3D11DeviceContext* DC,ID3D11ShaderResourceView* srv, ID3D11ShaderResourceView* dsv);
	

	void BokehRender();
	void Resize(const uint &width, const uint& height);
public:

	struct PARAMETERS_HDR
	{
		float MiddleGrey;
		float White;
		
		float BloomThreshold;
		float BloomScale;
	}Params;
	PARAMETERS_HDR* GetParams() { return &Params; }

	
private:


	ID3D11Buffer*              downScale1DBuffer;
	ID3D11UnorderedAccessView* downScale1DUAV;
	ID3D11ShaderResourceView*  downScale1DSRV;


	ID3D11Texture2D*           DownScaleRT;
	ID3D11ShaderResourceView*   DownScaleSRV;
	ID3D11UnorderedAccessView* DownScaleUAV;
		

	// Average luminance
	ID3D11Buffer*              avgLumBuffer;
	ID3D11UnorderedAccessView* avgLumUAV;
	ID3D11ShaderResourceView*   avgLumSRV;
	
	// Previous average luminance for adaptation
	//ID3D11Buffer* PrevAvgLumBuffer;
	//ID3D11UnorderedAccessView* PrevAvgLumUAV;
	//ID3D11ShaderResourceView* PrevAvgLumSRV;


	ID3D11DeviceContext* DC;

private:

	
	uint width;
	uint height;

	uint downScaleGroups;

	


private:
	struct TDownScaleCB
	{
		UINT Width;
		UINT Height;
		UINT TotalPixels;
		UINT GroupSize;
	
		float BloomThreshold;
		UINT pad[3];
	}DownScaleDesc;


	ID3D11Buffer* DownScaleCB;

	struct TBlurCB
	{
		UINT Width;
		UINT Height;
		UINT pad[2];
		
	}blurDesc;
	ID3D11Buffer* BlurCB;

	struct TFinalPassCB
	{

		float MiddleGrey;
		float LumWhiteSqr;
		float BloomScale;
		UINT pad;
	}fianlPassDesc;
	
	ID3D11Buffer* FinalPassCB;

	 float DOFFarValues1 = 0.0f;
	 float DOFFarValues2 =  943.0f;

	 float BokehLumThreshold= 7.65f;
	 float BokehBlurThreshold = 0.43f;
	 float BokehRadiusScale = 0.05f;
	 float BokehColorScale = 0.05f;

	
	 ID3D11SamplerState*        SampLinear = NULL;
	 ID3D11SamplerState*        SampPoint = NULL;
	// Bloom texture
	ID3D11Texture2D*          BloomRT;
	ID3D11ShaderResourceView* BloomSRV;
	ID3D11UnorderedAccessView* BloomUAV;
	
	// Temporary texture
	ID3D11Texture2D* TempRT[2];
	ID3D11ShaderResourceView* TempSRV[2];
	ID3D11UnorderedAccessView* TempUAV[2];
private:

	//ID3D11Buffer* BokehBuffer;
	//ID3D11UnorderedAccessView* BokehUAV=nullptr;
	//ID3D11ShaderResourceView* BokehSRV;
	//


	//// Bokeh indirect draw buffer
	//ID3D11Buffer* BokehIndirectDrawBuffer=nullptr;

	//// Bokeh highlight texture view and blend state
	//ID3D11ShaderResourceView* BokehTexView;
	/*ID3DX11EffectUnorderedAccessViewVariable* bokehUAVEffect;
	ID3DX11EffectShaderResourceVariable* bokehHDRTextureEffect;
	ID3DX11EffectShaderResourceVariable* bokehDepthTextureEffect;
	ID3DX11EffectShaderResourceVariable* bokehAvgLumEffect;*/

	/*struct TBokehHightlightScanCB
	{
		UINT nWidth;
		UINT nHeight;
		float ProjectionValues[2];
		float fDOFFarStart;

		float fDOFFarRangeRcp;
		float fMiddleGrey;
		float fLumWhiteSqr;
		float fBokehBlurThreshold;

		float fBokehLumThreshold;
		float fRadiusScale;
		float fColorScale;
		

	} ;
	TBokehHightlightScanCB BokehHightlightScanCB;
	ConstantBuffer* bokehCSBuffer;
	ID3DX11EffectConstantBuffer* sBokehCSBuffer;

	struct TBokehRenderCB
	{
		float AspectRatio[2];
		UINT pad[2];


	};
	TBokehRenderCB BokehRenderCB;
	ConstantBuffer* bokehRenderBuffer;
	ID3DX11EffectConstantBuffer* sBokehRenderBuffer;
*/
	//Matrix proj;
	bool bFirst = false;
	


	// Shaders
	ID3D11ComputeShader* DownScaleFirstPassCS;
	ID3D11ComputeShader* DownScaleSecondPassCS;
	ID3D11ComputeShader* BloomRevealCS;
	ID3D11ComputeShader* HorizontalBlurCS;
	ID3D11ComputeShader* VerticalBlurCS;
	ID3D11VertexShader*  FullScreenQuadVS;
	ID3D11PixelShader*   FinalPassPS;
	ID3D11Device*device;
};
