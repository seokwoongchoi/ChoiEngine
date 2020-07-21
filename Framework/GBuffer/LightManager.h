#pragma once

class LightManager
{
public:
	LightManager();
	~LightManager();
	static const int TotalPointShadowmaps = 3;


public:

	void Init(ID3D11Device* device);
	void CreateShaders(ID3D11Device* device);
	void CreateBuffers(ID3D11Device* device);
	void Deinit();

	void Update();
	void ClearShadowPipeLine(ID3D11DeviceContext * context);
	void Resize(const Vector2& size);


public:
	void ClearLights() { arrLights.clear(); LastShadowLight = -1; NextFreeSpotShadowmap = -1;  NextFreePointShadowmap = -1; }
	void AddPointLight(const Vector3& PointPosition, float PointRange, const Vector3& PointColor, bool bCastShadow)
	{
		LIGHT pointLight;

		pointLight.eLightType = TYPE_POINT;
		pointLight.Position = PointPosition;
		pointLight.Range = PointRange;
		pointLight.Color = PointColor;
    	pointLight.iShadowmapIdx = bCastShadow ? GetNextFreePointShadowmapIdx() : -1;

		arrLights.push_back(pointLight);
	}

	void AddSpotLight(const Vector3& SpotPosition, const Vector3& SpotDirection, float SpotRange,
		float SpotOuterAngle, float SpotInnerAngle, const Vector3& SpotColor, bool bCastShadow)
	{
		LIGHT spotLight;

		spotLight.eLightType = TYPE_SPOT;
		spotLight.Position = SpotPosition;
		spotLight.Direction = SpotDirection;
		spotLight.Range = SpotRange;
		spotLight.OuterAngle = Math::PI * SpotOuterAngle / 180.0f;
		spotLight.InnerAngle = Math::PI * SpotInnerAngle / 180.0f;
		spotLight.Color = SpotColor;
		spotLight.iShadowmapIdx = bCastShadow ? GetNextFreeSpotShadowmapIdx() : -1;

		arrLights.push_back(spotLight);
	}

	void AddCapsuleLight(const Vector3& CapsulePosition, const Vector3& CapsuleDirection, float CapsuleRange,
		float CapsuleLength, const Vector3& CapsuleColor)
	{
		LIGHT capsuleLight;

		capsuleLight.eLightType = TYPE_CAPSULE;
		capsuleLight.Position = CapsulePosition;
		capsuleLight.Direction = CapsuleDirection;
		capsuleLight.Range = CapsuleRange;
		capsuleLight.Length = CapsuleLength;
		capsuleLight.Color = CapsuleColor;
		capsuleLight.iShadowmapIdx = -1;
		capsuleLight.OuterAngle = 0.0f;
		capsuleLight.InnerAngle = 0.0f;
		arrLights.push_back(capsuleLight);
	}

	void DoLighting(ID3D11DeviceContext* context);
	void DoDebugLightVolume(ID3D11DeviceContext* context);
	void DoDebugCascadedShadows(ID3D11DeviceContext* context);
	bool PrepareNextShadowLight(ID3D11DeviceContext* context);
private:
	ID3D11Device* device;
private:
	void CreateCascadedShadowBuffers(const Vector2& size);
private:

	typedef enum
	{
		TYPE_POINT = 0,
		TYPE_SPOT,
		TYPE_CAPSULE
	} LIGHT_TYPE;


	typedef struct
	{
		LIGHT_TYPE eLightType = {};
		Vector3 Position;
		Vector3 Direction;
		float Range;
		float Length;
		float OuterAngle;
		float InnerAngle;
		Vector3 Color;
		int iShadowmapIdx;
	} LIGHT;
public:

	void DirectionalLight(ID3D11DeviceContext* context);
	void PointLight(ID3D11DeviceContext* context, const Vector3& Pos, float Range, const Vector3& Color, int iShadowmapIdx, bool bWireframe);
	void SpotLight(ID3D11DeviceContext* context, const Vector3& Pos, const Vector3& Dir, float Range, float InnerAngle, float OuterAngle, const Vector3& Color, int iShadowmapIdx, bool bWireframe);
	void CapsuleLight(ID3D11DeviceContext* context, const Vector3& Pos, const Vector3& Dir, float Range, float fLen, const Vector3& Color, bool bWireframe);
public:

	int GetNextFreeSpotShadowmapIdx() { return (NextFreeSpotShadowmap + 1 <  iTotalSpotShadowmaps) ? ++NextFreeSpotShadowmap : -1; }
    void SpotShadowGen(ID3D11DeviceContext* context, const LIGHT& light);
	int GetNextFreePointShadowmapIdx() { return ( NextFreePointShadowmap + 1 < TotalPointShadowmaps) ? ++ NextFreePointShadowmap : -1; }
	void PointShadowGen(ID3D11DeviceContext* context, const LIGHT& light);
	void CascadedShadowsGen(ID3D11DeviceContext* context);
public:
	// Directional light
	ID3D11VertexShader*  DirLightVertexShader;
	ID3D11PixelShader*  DirLightPixelShader;
	
	ID3D11Buffer*  DirLightCB;
	ID3D11PixelShader* NOAOps;
	// Point light
	ID3D11VertexShader*  PointLightVertexShader;
	ID3D11HullShader*  PointLightHullShader;
	ID3D11DomainShader*  PointLightDomainShader;
	ID3D11PixelShader*  PointLightPixelShader;
	ID3D11PixelShader*  PointLightShadowPixelShader;
	ID3D11Buffer*  PointLightDomainCB;
	ID3D11Buffer*  PointLightPixelCB;

	// Spot light
	ID3D11VertexShader*  SpotLightVertexShader;
	ID3D11HullShader*  SpotLightHullShader;
	ID3D11DomainShader*  SpotLightDomainShader;
	ID3D11PixelShader* SpotLightPixelShader;
	ID3D11PixelShader*  SpotLightShadowPixelShader;
	ID3D11Buffer*  SpotLightDomainCB;
	ID3D11Buffer*  SpotLightPixelCB;

	// Capsule light
	ID3D11VertexShader*  CapsuleLightVertexShader;
	ID3D11HullShader*  CapsuleLightHullShader;
	ID3D11DomainShader*  CapsuleLightDomainShader;
	ID3D11PixelShader*  CapsuleLightPixelShader;
	ID3D11Buffer*  CapsuleLightDomainCB;
	ID3D11Buffer*  CapsuleLightPixelCB;

	ID3D11VertexShader*  SpotShadowGenVertexShader;
	ID3D11Buffer*  SpotShadowGenVertexCB;

	ID3D11VertexShader*  PointShadowGenVertexShader;
	ID3D11GeometryShader*  PointShadowGenGeometryShader;
	ID3D11Buffer*  PointShadowGenGeometryCB;


	ID3D11GeometryShader*  CascadedShadowGenGeometryShader;
	ID3D11Buffer*  CascadedShadowGenGeometryCB;



	ID3D11PixelShader*  DebugCascadesPixelShader;


	ID3D11DepthStencilState*  NoDepthWriteLessStencilMaskState;
	ID3D11DepthStencilState*  NoDepthWriteGreatherStencilMaskState;

	ID3D11DepthStencilState*  ShadowGenDepthState;

	ID3D11BlendState*  AdditiveBlendState;

	ID3D11RasterizerState*  NoDepthClipFrontRS;

	
	ID3D11RasterizerState*  WireframeRS;

	ID3D11RasterizerState*  ShadowGenRS;
	ID3D11RasterizerState*  CascadedShadowGenRS;


	ID3D11SamplerState*      sampLinear;
	ID3D11SamplerState*  PCFSamplerState;
	
	bool ShowLightVolume;

	static const float ShadowNear;


	int LastShadowLight;

	int NextFreeSpotShadowmap;


	
	static const int  iTotalSpotShadowmaps = 3;

	
	ID3D11Texture2D*  SpotDepthStencilRT[ iTotalSpotShadowmaps];
	ID3D11DepthStencilView*  SpotDepthStencilDSV[ iTotalSpotShadowmaps];
	ID3D11ShaderResourceView*  SpotDepthStencilSRV[ iTotalSpotShadowmaps];

	
	int  NextFreePointShadowmap;

	ID3D11Texture2D*  PointDepthStencilRT[TotalPointShadowmaps];
	ID3D11DepthStencilView*  PointDepthStencilDSV[ iTotalSpotShadowmaps];
	ID3D11ShaderResourceView*  PointDepthStencilSRV[ iTotalSpotShadowmaps];

	ID3D11Texture2D* CascadedDepthStencilRT;
	ID3D11DepthStencilView* CascadedDepthStencilDSV;
	ID3D11ShaderResourceView* CascadedDepthStencilSRV;

	std::vector<LIGHT> arrLights;

private:
	
	D3D11_VIEWPORT shadowVP[3];
	


};

