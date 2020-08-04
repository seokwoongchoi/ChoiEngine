#include "Framework.h"
#include "Frustum.h"

Frustum::Frustum()
{
	
}

Frustum::~Frustum()
{
}

void Frustum::Update()
{
	//camera->GetMatrix();
	V = GlobalData::GetView();
	P = GlobalData::GetProj();
	//perspective->GetMatrix(&P);
	//P._22 -= 0.3f;
	//P._11 -= 0.3f;

	/*static float t = -1.0f;
	ImGui::InputFloat("fov", (float*)&t, -2.0f, 2.0f);
	P._22 += t;
	P._11 += t;*/

	D3DXMatrixMultiply(&W, &V, &P);

	//left
	planes[0].a = W._14 + W._11;
	planes[0].b = W._24 + W._21;
	planes[0].c = W._34 + W._31;
	planes[0].d = W._44 + W._41;
	//right
	planes[1].a = W._14 - W._11;
	planes[1].b = W._24 - W._21;
	planes[1].c = W._34 - W._31;
	planes[1].d = W._44 - W._41;

	//bottom
	planes[2].a = W._14 + W._12;
	planes[2].b = W._24 + W._22;
	planes[2].c = W._34 + W._32;
	planes[2].d = W._44 + W._42;
	//top
	planes[3].a = W._14 - W._12;
	planes[3].b = W._24 - W._22;
	planes[3].c = W._34 - W._32;
	planes[3].d = W._44 - W._42;

	//near
	planes[4].a = W._13;
	planes[4].b = W._23;
	planes[4].c = W._33;
	planes[4].d = W._43;
	//far
	planes[5].a = W._14 - W._13;
	planes[5].b = W._24 - W._23;
	planes[5].c = W._34 - W._33;
	planes[5].d = W._44 - W._43;

	for (int i = 0; i < 6; i++)
		D3DXPlaneNormalize(&planes[i], &planes[i]);


}

void Frustum::Render(Plane * plane)
{
	memcpy(plane, this->planes, sizeof(Plane) * 6);
}

bool Frustum::ContainPoint(Vector3 & position)
{
	for (int i = 0; i < 6; i++)
	{
		if (D3DXPlaneDotCoord(&planes[i], &position) < 0.0f)
		{
			return false;
		}
	}

	return true;
}

bool Frustum::ContainRect(float xCenter, float yCenter, float zCenter, float xSize, float ySize, float zSize)
{
	for (int i = 0; i < 6; i++)
	{
		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter - xSize), (yCenter - ySize), (zCenter - zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter + xSize), (yCenter - ySize), (zCenter - zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter - xSize), (yCenter + ySize), (zCenter - zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter - xSize), (yCenter - ySize), (zCenter + zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter + xSize), (yCenter + ySize), (zCenter - zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter + xSize), (yCenter - ySize), (zCenter + zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter - xSize), (yCenter + ySize), (zCenter + zSize))) >= 0.0f)
			continue;

		if (D3DXPlaneDotCoord(&planes[i], &Vector3((xCenter + xSize), (yCenter + ySize), (zCenter + zSize))) >= 0.0f)
			continue;

		return false;
	}

	return true;
}

bool Frustum::ContainRect(Vector3 center, Vector3 size)
{
	return ContainRect(center.x, center.y, center.z, size.x, size.y, size.z);
}

bool Frustum::ContainCube(Vector3 & center, float radius)
{


	for (int i = 0; i < 6; i++)
	{

		check.x = center.x - radius;
		check.y = center.y - radius;
		check.z = center.z - radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x + radius;
		check.y = center.y - radius;
		check.z = center.z - radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x - radius;
		check.y = center.y + radius;
		check.z = center.z - radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x + radius;
		check.y = center.y + radius;
		check.z = center.z - radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x - radius;
		check.y = center.y - radius;
		check.z = center.z + radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x + radius;
		check.y = center.y - radius;
		check.z = center.z + radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x - radius;
		check.y = center.y + radius;
		check.z = center.z + radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;

		check.x = center.x + radius;
		check.y = center.y + radius;
		check.z = center.z + radius;
		if (D3DXPlaneDotCoord(&planes[i], &check) >= 0.0f)
			continue;


		return false;
	}
	return true;


}
