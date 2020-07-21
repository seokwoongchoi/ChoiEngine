#pragma once
class CascadedShadow
{
	friend class LightManager;
public:
	CascadedShadow();
	~CascadedShadow();

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
};

