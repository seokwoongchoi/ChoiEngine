#pragma once
typedef struct
{
	Vector3 Position;
	Vector3 Direction;
	float Range;
	Vector3 Color;
	int iShadowmapIdx;
	float Intencity;

	bool bEffect = false;
	
} PointLights;

typedef struct
{
	Vector3 Position;
	Vector3 Direction;
	float Range;
	float OuterAngle;
	float InnerAngle;
	Vector3 Color;
	int iShadowmapIdx;
	float Intencity;
} SpotLights;

typedef struct
{
	Vector3 Position;
	Vector3 Direction;
	float Range;
	float Length;
	float OuterAngle;
	float InnerAngle;
	Vector3 Color;
	int iShadowmapIdx;
	float Intencity;
} CapsuleLights;

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

	

	void AddPointLight(const Vector3& PointPosition, float PointRange, const Vector3& PointColor, float Intencity, bool bCastShadow=false, bool bEffect = false)
	{
		PointLights pointLight;

	
		pointLight.Position = PointPosition;
		pointLight.Range = PointRange;
		pointLight.Color = PointColor;
		pointLight.Intencity = Intencity;
		pointLight.bEffect = bEffect;
    	pointLight.iShadowmapIdx = bCastShadow ? GetNextFreePointShadowmapIdx() : -1;

		pointLights.emplace_back(pointLight);
	}

	void ClearEffectPointLights()
	{
		auto find = find_if(pointLights.begin(), pointLights.end(), [](const PointLights& point)
		{
			return point.bEffect==true;
		});
		if (find != pointLights.end())
		{
			pointLights.erase(find);
			pointLights.shrink_to_fit();
		}
	}
	PointLights* GetPointLights()
	{
		
		return pointLights.data();
	}

	void AddSpotLight(const Vector3& SpotPosition, const Vector3& SpotDirection, float SpotRange,
		float SpotOuterAngle, float SpotInnerAngle, float Intencity, const Vector3& SpotColor, bool bCastShadow)
	{
		SpotLights spotLight;

		
		spotLight.Position = SpotPosition;
		spotLight.Direction = SpotDirection;
		spotLight.Range = SpotRange;
		spotLight.OuterAngle = Math::PI * SpotOuterAngle / 180.0f;
		spotLight.InnerAngle = Math::PI * SpotInnerAngle / 180.0f;
		spotLight.Color = SpotColor;
		spotLight.iShadowmapIdx = bCastShadow ? GetNextFreeSpotShadowmapIdx() : -1;
		spotLight.Intencity = Intencity;
		spotLights.push_back(spotLight);
	}

	void AddCapsuleLight(const Vector3& CapsulePosition, const Vector3& CapsuleDirection, float CapsuleRange,
		float CapsuleLength, float Intencity, const Vector3& CapsuleColor)
	{
		CapsuleLights capsuleLight;

	
		capsuleLight.Position = CapsulePosition;
		capsuleLight.Direction = CapsuleDirection;
		capsuleLight.Range = CapsuleRange;
		capsuleLight.Length = CapsuleLength;
		capsuleLight.Color = CapsuleColor;
		capsuleLight.iShadowmapIdx = -1;
		capsuleLight.OuterAngle = 0.0f;
		capsuleLight.InnerAngle = 0.0f;
		capsuleLight.Intencity = Intencity;
		capsuleLights.push_back(capsuleLight);
	}

	void DoLighting(ID3D11DeviceContext* context);
	void DoDebugLightVolume(ID3D11DeviceContext* context);
	void DoDebugCascadedShadows(ID3D11DeviceContext* context, ID3D11ShaderResourceView* srv);
	
private:
	ID3D11Device* device;


public:

	void DirectionalLight(ID3D11DeviceContext* context);
	void PointLight(ID3D11DeviceContext* context, const Vector3& Pos, float Range, const Vector3& Color, float Intencity, int iShadowmapIdx, bool bWireframe);
	void SpotLight(ID3D11DeviceContext* context, const Vector3& Pos, const Vector3& Dir, float Range, float InnerAngle, float OuterAngle, const Vector3& Color, int iShadowmapIdx, bool bWireframe);
	void CapsuleLight(ID3D11DeviceContext* context, const Vector3& Pos, const Vector3& Dir, float Range, float fLen, const Vector3& Color, bool bWireframe);
public:

	int GetNextFreeSpotShadowmapIdx() { return (NextFreeSpotShadowmap + 1 <  iTotalSpotShadowmaps) ? ++NextFreeSpotShadowmap : -1; }
    void SpotShadowGen(ID3D11DeviceContext* context, const SpotLights& light);
	int GetNextFreePointShadowmapIdx() { return ( NextFreePointShadowmap + 1 < TotalPointShadowmaps) ? ++ NextFreePointShadowmap : -1; }
	void PointShadowGen(ID3D11DeviceContext* context, const PointLights& light);
	
public:
	void RenderBrush(ID3D11DeviceContext* context,ID3D11Buffer* buffer, ID3D11ShaderResourceView* srv);

private:
	
	struct CB_DIRECTIONAL
{

	Vector3 DirToLight;
	float Time = 0.0f;
	Vector3 DirectionalColor;
	float pad2;
	Matrix ToShadowSpace;
	Vector4 ToCascadeSpace[3];
};

	CB_DIRECTIONAL pDirectionalValuesCB;
	D3DXMATRIX LightWorldScale;
	D3DXMATRIX LightWorldTrans;
	D3DXMATRIX WorldViewProjection;
	struct CB_POINT_LIGHT_DOMAIN
	{
		Matrix WolrdViewProj;
	}PointLightDomainDesc;

	struct CB_POINT_LIGHT_PIXEL
	{
		Vector3 PointLightPos;
		float PointLightRangeRcp;
		Vector3 PointColor;
		float Intencity;
		Vector2 LightPerspectiveValues;
		float pad2[2];
	}PointLightPixelDesc;
		
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





	ID3D11PixelShader*  DebugCascadesPixelShader;
	ID3D11PixelShader*  RenderBrushPixelShader;

	ID3D11DepthStencilState*  NoDepthWriteLessStencilMaskState;
	ID3D11DepthStencilState*  NoDepthWriteGreatherStencilMaskState;

	

	ID3D11BlendState*  AdditiveBlendState;

	ID3D11RasterizerState*  NoDepthClipFrontRS;

	
	ID3D11RasterizerState*  WireframeRS;

	ID3D11RasterizerState*  ShadowGenRS;
	


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



	
	vector<PointLights> pointLights;

	vector<SpotLights> spotLights;


	vector<CapsuleLights> capsuleLights;

	


};

