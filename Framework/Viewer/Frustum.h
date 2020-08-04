#pragma once
class Frustum
{
public:
	Frustum();
	~Frustum();

	void Update();
	void Render(Plane* plane);

	bool ContainPoint(Vector3& position);
	bool ContainRect(float xCenter, float yCenter, float zCenter, float xSize, float ySize, float zSize);
	bool ContainRect(Vector3 center, Vector3 size);
	bool ContainCube(Vector3& center, float radius);
private:
	Plane planes[6];

	

	Matrix W, V, P;
	Vector3 check;
};

