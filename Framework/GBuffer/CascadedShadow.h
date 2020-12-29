#pragma once
class CascadedShadow
{
	friend class LightManager;
public:
	CascadedShadow();
	~CascadedShadow();

	bool PrepareNextShadowLight(ID3D11DeviceContext* context);
	void CascadedShadowsGen(ID3D11DeviceContext* context);




	void Init(Vector2 iShadowMapSize);
	void SetShadowMapSize(Vector2 iShadowMapSize);
	void Update(const Vector3& vDirectionalDir);
	void SetAntiFlicker(bool bIsOn) {  bAntiFlickerOn = bIsOn; }

	static const int  TotalCascades = 3;
protected:
	Matrix  arrWorldToCascadeProj[TotalCascades];
	Matrix  WorldToShadowSpace;
	
	Vector4  ToCascadeOffsetX;
	Vector4  ToCascadeOffsetY;
	Vector4  ToCascadeScale;

private:


	void ExtractFrustumPoints(float fNear, float fFar, Vector3* arrFrustumCorners);
	void ExtractFrustumBoundSphere(float fNear, float fFar, Vector3& vBoundCenter, float& fBoundRadius);
	bool CascadeNeedsUpdate(const Matrix& mShadowView, int iCascadeIdx, const Vector3& newCenter, Vector3& vOffset);

	bool  bAntiFlickerOn;
	Vector2  ShadowMapSize;
	float  CascadeTotalRange;
	float  arrCascadeRanges[4];

	Vector3  ShadowBoundCenter;
	float  ShadowBoundRadius;
	Vector3  arrCascadeBoundCenter[ TotalCascades];
	float  arrCascadeBoundRadius[ TotalCascades];

	Vector3 arrFrustumPoints[8];
private:
	Matrix view;
	Vector3 camPos;
	Vector3 camForward;
	Vector3 camUp;
	Vector3 camRight;

	Vector3 worldCenter;
	Vector3 pos;
	Vector3 lookAt;
	Vector3 right;
	Vector3 up; ;

	Matrix shadowView;
	Matrix shadowViewInv;

	Matrix cascadeTrans;
	Matrix cascadeScale;
	Vector3 newCenter;

	Vector3 offset;

	Vector3 cascadeCenterShadowSpace;

	D3D11_VIEWPORT vp[3];
	Vector3 oldCenterInCascade;
	Vector3 newCenterInCascade;
	Vector3 centerDiff;



	Vector3 vBoundSpan;
public:
	void CreateCascadedShadowBuffers(ID3D11Device* device,const Vector2& size);
	ID3D11ShaderResourceView* CascadedDepthStencilSRV;
private:
	ID3D11DepthStencilState*  ShadowGenDepthState;
	ID3D11RasterizerState*  CascadedShadowGenRS;

	ID3D11Texture2D* CascadedDepthStencilRT;
	ID3D11DepthStencilView* CascadedDepthStencilDSV;
	



	ID3D11GeometryShader*  CascadedShadowGenGeometryShader;
	ID3D11Buffer*  CascadedShadowGenGeometryCB;

	D3D11_VIEWPORT shadowVP[3];

};

