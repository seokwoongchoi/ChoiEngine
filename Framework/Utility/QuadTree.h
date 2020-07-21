#pragma once

struct QuadTreeNode
{
	float minX;
	float maxX;
	float minZ;
	float maxZ;

	Vector3 boundsMin;
	Vector3 boundsMax;

	Vector2 minMaxY;
	vector< shared_ptr<QuadTreeNode>>childs;
	vector<shared_ptr<QuadTreeNode>> hittedChilds;

	bool Intersection(const Vector3& org, const Vector3& dir, Vector3& Pos);


	bool IntersectionAABB(const Vector3& org, const Vector3& dir, Vector3& Pos, float& d);
	bool hitted = false;

	float dist = -1.0f;



};

class QuadTree
{
public:
	QuadTree();
	~QuadTree();


	bool Intersection(Vector3& Pos);

private:
	void CreateQuadTree(shared_ptr< QuadTreeNode> node, Vector2 leftTop, Vector2 rightBottom);

private:
	void GetRay(OUT Vector3* position, OUT Vector3* direction, const Matrix& w, const Matrix& v, const Matrix& p);
	shared_ptr< QuadTreeNode> root;
private:
	Vector3 org, dir;
	Matrix V;
	Matrix P;
	Matrix W;
	Vector3 pos;
};

