#include "Framework.h"
#include "CascadedShadow.h"

CascadedShadow::CascadedShadow()
	: arrCascadeRanges{ 0,0,0,0 }, bAntiFlickerOn(true), CascadeTotalRange(50.0f), ShadowMapSize(Vector2(1280.0f,720.0f)), ShadowBoundCenter(Vector3(0,0,0)), ShadowBoundRadius(0),	  arrCascadeBoundCenter{ Vector3 (0,0,0), Vector3(0,0,0) , Vector3(0,0,0) },
	arrCascadeBoundRadius{ 0,0,0 },	arrFrustumPoints{ Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) , Vector3(0,0,0) },
	camPos(Vector3(0, 0, 0)),camForward(Vector3(0, 0, 0)),camUp(Vector3(0, 0, 0)),camRight(Vector3(0, 0, 0)),worldCenter(Vector3(0, 0, 0)),pos(Vector3(0, 0, 0)),
	lookAt(Vector3(0, 0, 0)),right(Vector3(0, 0, 0)),up(Vector3(0, 0, 0)), newCenter(Vector3(0, 0, 0)), offset(Vector3(0, 0, 0)), cascadeCenterShadowSpace(Vector3(0, 0, 0)),
	oldCenterInCascade(Vector3(0, 0, 0)), newCenterInCascade(Vector3(0, 0, 0)), centerDiff(Vector3(0, 0, 0)),	 vBoundSpan(Vector3(0, 0, 0)),
	view{ 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f,   0.0f ,0.0f ,0.0f ,0.0f },
	shadowView{ 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f },
	shadowViewInv{ 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f },
    cascadeTrans{ 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f },
	cascadeScale{ 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f, 0.0f ,0.0f ,0.0f ,0.0f },
	vp{}
{
}


CascadedShadow::~CascadedShadow()
{
}

void CascadedShadow::Init(Vector2 iShadowMapSize)
{
	 this->ShadowMapSize = iShadowMapSize;

	// Set the range values
	 arrCascadeRanges[0] = 0.1f;
	 arrCascadeRanges[1] = 10.0f;
	 arrCascadeRanges[2] = 25.0f;
	 arrCascadeRanges[3] = CascadeTotalRange;

	for (int i = 0; i <  TotalCascades; i++)
	{
		 arrCascadeBoundCenter[i] = Vector3(0.0f, 0.0f, 0.0f);
		 arrCascadeBoundRadius[i] = 0.0f;
	}
}

void CascadedShadow::SetShadowMapSize(Vector2 iShadowMapSize)
{
	this->ShadowMapSize = iShadowMapSize;
}

void CascadedShadow::Update(const Vector3 & vDirectionalDir)
{
	view = GlobalData::GetView();
	Matrix viewInv;
	D3DXMatrixInverse(&viewInv, nullptr,&view);
	camPos = GlobalData::Position();
	camForward = GlobalData::Forward();
	camUp = Vector3(viewInv._21, viewInv._22, viewInv._23);
	camRight = Vector3(viewInv._11, viewInv._12, viewInv._13);
	
	worldCenter = camPos + camForward *  CascadeTotalRange * 0.5f;
	pos = worldCenter;
	lookAt = worldCenter + vDirectionalDir * 1000.0f;
	right = Vector3(1.0f, 0.0f, 0.0f);
	D3DXVec3Cross(&up, &vDirectionalDir, &right);
	D3DXVec3Normalize(&up, &up);
	
	D3DXMatrixLookAtLH(&shadowView, &pos, &lookAt, &up);
		
	float fRadius;
	ExtractFrustumBoundSphere( arrCascadeRanges[0],  arrCascadeRanges[3], ShadowBoundCenter, fRadius);
	ShadowBoundRadius = max( ShadowBoundRadius, fRadius); // Expend the radius to compensate for numerical errors
	Matrix shadowProj;
	 D3DXMatrixOrthoLH(&shadowProj, ShadowBoundRadius, ShadowBoundRadius, -ShadowBoundRadius, ShadowBoundRadius);
	 WorldToShadowSpace = shadowView * shadowProj;

	
	D3DXMatrixTranspose(&shadowViewInv, &shadowView);
	for (int iCascadeIdx = 0; iCascadeIdx < TotalCascades; iCascadeIdx++)
	{
		
		if ( bAntiFlickerOn)
		{
			
			ExtractFrustumBoundSphere( arrCascadeRanges[iCascadeIdx],  arrCascadeRanges[iCascadeIdx + 1], newCenter, fRadius);
			arrCascadeBoundRadius[iCascadeIdx] = max( arrCascadeBoundRadius[iCascadeIdx],fRadius); 
		
			 
			if (CascadeNeedsUpdate(shadowView, iCascadeIdx, newCenter,offset))
			{
				Vector3 offsetOut;
				D3DXVec3TransformNormal(&offsetOut, &offset, &shadowViewInv);
				arrCascadeBoundCenter[iCascadeIdx] += offsetOut;
			}

			// Get the cascade center in shadow space
			D3DXVec3TransformCoord(&cascadeCenterShadowSpace, &arrCascadeBoundCenter[iCascadeIdx], &WorldToShadowSpace);
			
			// Update the translation from shadow to cascade space
			 ToCascadeOffsetX[iCascadeIdx] = -cascadeCenterShadowSpace.x;
			 ToCascadeOffsetY[iCascadeIdx] = -cascadeCenterShadowSpace.y;
			 D3DXMatrixTranslation(&cascadeTrans, ToCascadeOffsetX[iCascadeIdx], ToCascadeOffsetY[iCascadeIdx], 0.0f);
		
			 ToCascadeScale[iCascadeIdx] =  ShadowBoundRadius /  arrCascadeBoundRadius[iCascadeIdx];
			 D3DXMatrixScaling(&cascadeScale, ToCascadeScale[iCascadeIdx], ToCascadeScale[iCascadeIdx], 1.0f);
			
		}
		else
		{
			
			ExtractFrustumPoints( arrCascadeRanges[iCascadeIdx],  arrCascadeRanges[iCascadeIdx + 1], arrFrustumPoints);

			 Vector3& vMin = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
			 Vector3& vMax = Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			for (int i = 0; i < 8; i++)
			{
				Vector3 PointInShadowSpace;
				D3DXVec3TransformCoord(&PointInShadowSpace, &arrFrustumPoints[iCascadeIdx], &WorldToShadowSpace);
				

				for (int j = 0; j < 3; j++)
				{
					if ((&vMin.x)[j] > (&PointInShadowSpace.x)[j])
						(&vMin.x)[j] = (&PointInShadowSpace.x)[j];
					if ((&vMax.x)[j] < (&PointInShadowSpace.x)[j])
						(&vMax.x)[j] = (&PointInShadowSpace.x)[j];
				}
			}

			 Vector3 CascadeCenterShadowSpace = 0.5f * (vMin + vMax);

			// Update the translation from shadow to cascade space
			 ToCascadeOffsetX[iCascadeIdx] = -CascadeCenterShadowSpace.x;
			 ToCascadeOffsetY[iCascadeIdx] = -CascadeCenterShadowSpace.y;
			 D3DXMatrixTranslation(&cascadeTrans, ToCascadeOffsetX[iCascadeIdx], ToCascadeOffsetY[iCascadeIdx], 0.0f);
			

			// Update the scale from shadow to cascade space
			 ToCascadeScale[iCascadeIdx] = 2.0f / max(vMax.x - vMin.x, vMax.y - vMin.y);
			 D3DXMatrixScaling(&cascadeScale, ToCascadeScale[iCascadeIdx], ToCascadeScale[iCascadeIdx], 1.0f);
			
		}

		// Combine the matrices to get the transformation from world to cascade space
		 arrWorldToCascadeProj[iCascadeIdx] =  WorldToShadowSpace * cascadeTrans * cascadeScale;
		 D3DXMatrixTranspose(&arrWorldToCascadeProj[iCascadeIdx], &arrWorldToCascadeProj[iCascadeIdx]);
	}

	// Set the values for the unused slots to someplace outside the shadow space
	for (int i =  TotalCascades; i < 4; i++)
	{
		 ToCascadeOffsetX[i] = 400.0f;
		 ToCascadeOffsetY[i] = 400.0f;
		 ToCascadeScale[i] = 0.1f;
	}
}

void CascadedShadow::ExtractFrustumPoints(float fNear, float fFar, Vector3 * arrFrustumCorners)
{
	// Get the camera bases
	

	// Calculate the tangent values (this can be cached
	const float& aspectRatio = static_cast<float>(D3D::Width()) / static_cast<float>(D3D::Height());
	const float& fTanFOVX = tanf(aspectRatio * Math::PI*0.25f);
	const float& fTanFOVY = tanf(aspectRatio);

	// Calculate the points on the near plane
	arrFrustumCorners[0] = camPos + (-camRight * fTanFOVX + camUp * fTanFOVY + camForward) * fNear;
	arrFrustumCorners[1] = camPos + (camRight * fTanFOVX + camUp * fTanFOVY + camForward) * fNear;
	arrFrustumCorners[2] = camPos + (camRight * fTanFOVX - camUp * fTanFOVY + camForward) * fNear;
	arrFrustumCorners[3] = camPos + (-camRight * fTanFOVX - camUp * fTanFOVY + camForward) * fNear;

	// Calculate the points on the far plane
	arrFrustumCorners[4] = camPos + (-camRight * fTanFOVX + camUp * fTanFOVY + camForward) * fFar;
	arrFrustumCorners[5] = camPos + (camRight * fTanFOVX + camUp * fTanFOVY + camForward) * fFar;
	arrFrustumCorners[6] = camPos + (camRight * fTanFOVX - camUp * fTanFOVY + camForward) * fFar;
	arrFrustumCorners[7] = camPos + (-camRight * fTanFOVX - camUp * fTanFOVY + camForward) * fFar;
}

void CascadedShadow::ExtractFrustumBoundSphere(float fNear, float fFar, Vector3 & vBoundCenter, float & fBoundRadius)
{
	const float& aspectRatio =static_cast<float>( D3D::Width()) / static_cast<float>(D3D::Height());
	
	const float& fTanFOVX = tanf(aspectRatio * Math::PI*0.25f);
	const float& fTanFOVY = tanf(aspectRatio);

	// The center of the sphere is in the center of the frustum
	vBoundCenter = camPos + camForward * (fNear + 0.5f * (fNear + fFar));

	// Radius is the distance to one of the frustum far corners
	vBoundSpan = camPos + (-camRight * fTanFOVX + camUp * fTanFOVY + camForward) * fFar - vBoundCenter;
	 fBoundRadius = D3DXVec3Length(&vBoundSpan);
}

bool CascadedShadow::CascadeNeedsUpdate(const Matrix & mShadowView, int iCascadeIdx, const Vector3 & newCenter, Vector3 & vOffset)
{
	// Find the offset between the new and old bound ceter
	
	D3DXVec3TransformCoord(&oldCenterInCascade, &arrCascadeBoundCenter[iCascadeIdx], &mShadowView);
	D3DXVec3TransformCoord(&newCenterInCascade, &newCenter, &mShadowView);

	centerDiff = newCenterInCascade - oldCenterInCascade;

	// Find the pixel size based on the diameters and map pixel size
	Vector2 fPixelSize;
	fPixelSize.x= static_cast<float>(D3D::Width()) / (2.0f *  arrCascadeBoundRadius[iCascadeIdx]);
	fPixelSize.y = static_cast<float>(D3D::Height()) / (2.0f *  arrCascadeBoundRadius[iCascadeIdx]);
	const float& fPixelOffX = centerDiff.x * fPixelSize.x;
	const float& fPixelOffY = centerDiff.y * fPixelSize.y;

	// Check if the center moved at least half a pixel unit
	bool bNeedUpdate = abs(fPixelOffX) > 0.5f || abs(fPixelOffY) > 0.5f;
	if (bNeedUpdate)
	{
		// Round to the 
		vOffset.x = floorf(0.5f + fPixelOffX) / fPixelSize.x;
		vOffset.y = floorf(0.5f + fPixelOffY) / fPixelSize.y;
		vOffset.z = centerDiff.z;
	}

	return bNeedUpdate;
}
