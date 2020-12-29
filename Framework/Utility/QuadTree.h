#pragma once
#define terrain_gridpoints					512
struct QuadTreeNode
{
	bool Intersection(const Vector3& org, const Vector3& dir, Vector3& Pos);
	bool IntersectionAABB(const Vector3& org, const Vector3& dir, Vector3& Pos, float& d);

	Vector3 boundsMin;
	Vector3 boundsMax;
	Vector2 minMaxY;
	vector< shared_ptr<QuadTreeNode>>childs;
	vector<shared_ptr<QuadTreeNode>> hittedChilds;
	
	bool hitted = false;

	float dist = -1.0f;

	float minX;
	float maxX;
	float minZ;
	float maxZ;
};

class QuadTree
{
public:
	QuadTree();
	~QuadTree();


	bool Intersection(Vector3& Pos);
	void Intersection(Matrix * const  matrix);
	bool InBounds(uint row, uint col);
	void CreateQuadTree(Vector2 lt,Vector2 rb,bool calcHeight=false);
	void BoxRender();
	void BoxRender(shared_ptr<QuadTreeNode> node);
	void GetRay(OUT Vector3* position, OUT Vector3* direction, const Matrix& w, const Matrix& v, const Matrix& p);
	shared_ptr<QuadTreeNode> GetRoot(){	return root;}

public:
	float	heightMap[terrain_gridpoints + 1][terrain_gridpoints + 1];
private:
	void CreateQuadTree(shared_ptr< QuadTreeNode> node, Vector2 leftTop, Vector2 rightBottom, bool calHeight=false);

private:
	
	shared_ptr< QuadTreeNode> root;
private:
	Vector3 org, dir;
	Matrix V;
	Matrix P;
	Matrix W;
	Vector3 pos;

	float height;
	float width;
};

