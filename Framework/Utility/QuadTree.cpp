#include "Framework.h"
#include "QuadTree.h"


QuadTree::QuadTree()
	:org(0,0,0),dir(0,0,0),pos(0,0,0)
{
	D3DXMatrixIdentity(&W);
	root = make_shared< QuadTreeNode>();
	CreateQuadTree(root, Vector2(-256.0f, -256.0f), Vector2(256.0f, 256.0f));
}


QuadTree::~QuadTree()
{
}
bool QuadTree::Intersection(Vector3 & Pos)
{
	V = GlobalData::GetView();
	P = GlobalData::GetProj();


	GetRay(&org, &dir, W, V, P);

	if (!root->Intersection(org, dir, pos))
	{
		return false;
	}

	
	Pos = pos;

	return true;
}
void QuadTree::CreateQuadTree(shared_ptr<QuadTreeNode> node, Vector2 leftTop, Vector2 rightBottom)
{
	static const float TileSize = 2;
	static const float  tolerance = 0.01f;
	static const float  width = 512.0f;
	static const float  height = 512.0f;

	// convert the heightmap index bounds into world-space coordinates
	/*minX = leftTop.x * CellsPerPatch - Width / 2;
	maxX = rightBottom.x * CellsPerPatch - Width / 2;
	minZ = -leftTop.y * CellsPerPatch + Depth / 2;
	maxZ = -rightBottom.y * CellsPerPatch + Depth / 2;*/
	node->minX = leftTop.x + width / 2;
	node->maxX = rightBottom.x + width / 2;



	node->minZ = -leftTop.y  + height/2 ;
	node->maxZ = -rightBottom.y+ height/2;

	//cout << node->minX << endl;

	/*node->minX = leftTop.x ;
	node->maxX = rightBottom.x;
	node->minZ = leftTop.y ;
	node->maxZ = rightBottom.y;
*/
// adjust the bounds to get a very slight overlap of the bounding boxes
	node->minX -= tolerance;
	node->maxX += tolerance;
	node->minZ += tolerance;
	node->maxZ -= tolerance;
	//UINT patchID = leftTop.x * (patchVertexCols - 1) + leftTop.y;

	node->minMaxY = Vector2(0.0f,0.0f);



	node->boundsMin = Vector3(node->minX, node->minMaxY.x, node->minZ);
	node->boundsMax = Vector3(node->maxX, node->minMaxY.y, node->maxZ);
	/*
	Matrix world = D3DXMATRIX(
		0, 0, 1, 0,
		0, 1, 0, 0,
		1, 0, 0, 0,
		0, 0, 0, 1);

	D3DXVec3TransformCoord(&node->boundsMin, &Vector3(node->minX, node->minMaxY.x, node->minZ), &world);
	D3DXVec3TransformCoord(&node->boundsMax, &Vector3(node->maxX, node->minMaxY.y, node->maxZ), &world);*/
	// construct the new node and assign the world-space bounds of the terrain region



	float nodeWidth = (rightBottom.x - leftTop.x)*0.5f;
	float nodeDepth = (rightBottom.y - leftTop.y)*0.5f;


	// we will recurse until the terrain regions match our logical terrain tile sizes

	node->childs.clear();
	if (nodeWidth >= TileSize && nodeWidth >= TileSize)
	{
		shared_ptr<QuadTreeNode> child1 = make_shared<QuadTreeNode>();
		shared_ptr<QuadTreeNode> child2 = make_shared<QuadTreeNode>();
		shared_ptr<QuadTreeNode> child3 = make_shared<QuadTreeNode>();
		shared_ptr<QuadTreeNode> child4 = make_shared<QuadTreeNode>();

		node->childs.emplace_back(child1);
		node->childs.emplace_back(child2);
		node->childs.emplace_back(child3);
		node->childs.emplace_back(child4);


		CreateQuadTree(node->childs[0],  leftTop, Vector2(leftTop.x + nodeWidth, leftTop.y + nodeDepth));
		CreateQuadTree(node->childs[1],  Vector2(leftTop.x + nodeWidth, leftTop.y), Vector2(rightBottom.x, leftTop.y + nodeDepth));
		CreateQuadTree(node->childs[2],  Vector2(leftTop.x, leftTop.y + nodeDepth), Vector2(leftTop.x + nodeDepth, rightBottom.y));
		CreateQuadTree(node->childs[3],  Vector2(leftTop.x + nodeWidth, leftTop.y + nodeDepth), rightBottom);



	}

}
void QuadTree::GetRay(OUT Vector3 * position, OUT Vector3 * direction, const Matrix & w, const Matrix & v, const Matrix & p)
{
	Vector3 mouse = Mouse::Get()->GetPosition();

	Vector2 point;
	//Inv Viewport
	{
		point.x = (((2.0f*mouse.x) / 1280.0f) - 1.0f);
		point.y = (((2.0f*mouse.y) / 720) - 1.0f)*-1.0f;

	}

	//Inv Projection
	{
		point.x = point.x / p._11;
		point.y = point.y / p._22;
	}

	Vector3 cameraPosition;
	//inv View
	{
		Matrix invView;
		D3DXMatrixInverse(&invView, nullptr, &v);

		cameraPosition = Vector3(invView._41, invView._42, invView._43);

		D3DXVec3TransformNormal(direction, &Vector3(point.x, point.y, 1), &invView);
		D3DXVec3Normalize(direction, direction);
	}
	//inv world
	{
		Matrix invWorld;
		D3DXMatrixInverse(&invWorld, nullptr, &w);

		D3DXVec3TransformCoord(position, &cameraPosition, &invWorld); //직선이 맞을 물체와 카메라의 공간을 일치시켜주기위해서 
		D3DXVec3TransformNormal(direction, direction, &invWorld);//직선이 맞을 물체와 카메라의 공간을 일치시켜주기위해서
		D3DXVec3Normalize(direction, direction);
		D3DXVec3TransformCoord(position, position, &invWorld);
	}
}
bool Compare(shared_ptr<QuadTreeNode> a, shared_ptr<QuadTreeNode> b)
{
	return a->dist > b->dist;
}
bool QuadTreeNode::Intersection(const Vector3 & org, const Vector3 & dir, Vector3 & Pos)
{
	Pos = Vector3(FLT_MAX, FLT_MAX, FLT_MAX);

	if (childs.empty())
	{
		float d;
		if (!IntersectionAABB(org, dir, Pos, d))
		{

			//cout << "Not Intersection" << endl;
			return false;
		}
		Pos = (boundsMin + boundsMax)*0.5f;


		return true;
	}
	
	//priority_queue<float> pq;
	for (auto& child : childs)
	{
		float cd = 0;
		if (child->IntersectionAABB(org, dir, Pos, child->dist))
		{
			child->dist += Math::Random(-0.001f, 0.001f);

			//pq.push(hittedChilds.back()->dist);
			hittedChilds.emplace_back(child);
		}
	}


	if (hittedChilds.empty())
	{

		return false;
	}


	sort(hittedChilds.begin(), hittedChilds.end(), Compare);

	bool intersect = false;
	Vector3 bestHit = org + 1000 * dir;


	for (auto& p : hittedChilds)
	{
		Vector3 thisHit;

		bool wasHit = p->Intersection(org, dir, thisHit);
		if (!wasHit)
		{
			//cout << "Not was Hit" << endl;
			continue;
		}
		/*Vector3 nor;
		D3DXVec3Normalize(&nor, &(thisHit - org));
		Vector3 dotDir = dir;
		float dot = D3DXVec3Dot(&nor,&dir);
		if (dot > 1.4f||dot<0.6f) {
			cout << "False due to dot" << endl;
			continue;
		}*/
		float l1 = D3DXVec3LengthSq(&(org - thisHit));
		float l2 = D3DXVec3LengthSq(&(org - bestHit));
		// check that the intersection is closer than the nearest intersection found thus far
		if (!(l1 < l2))
		{
			//cout << "check that the intersection LengthSquard" << endl;
			continue;
		}


		bestHit = thisHit;

		//SafeDelete(thisNode);
		intersect = true;
	}

	Pos = bestHit;
	hittedChilds.clear();
	hittedChilds.shrink_to_fit();

	return intersect;
}

bool QuadTreeNode::IntersectionAABB(const Vector3 & org, const Vector3 & dir, Vector3 & Pos, float & d)
{
	float t_min = 0;
	float t_max = FLT_MAX;


	for (int i = 0; i < 3; i++)
	{
		if (abs(dir[i]) < Math::EPSILON)
		{
			if (org[i] < boundsMin[i] ||
				org[i] >boundsMax[i])
			{
				hitted = false;
				return false;
			}

		}
		else
		{
			float denom = 1.0f / dir[i];
			float t1 = (boundsMin[i] - org[i]) * denom;
			float t2 = (boundsMax[i] - org[i]) * denom;

			if (t1 > t2)
			{
				swap(t1, t2);
			}

			t_min = max(t_min, t1);
			t_max = min(t_max, t2);

			if (t_min > t_max)
			{
				hitted = false;
				return false;
			}


		}
	}
	hitted = true;
	//Vector3 hit =  org + t_min * dir;


	//Pos = (boundsMin + boundsMax)*0.5f;
	//;
	d = t_min;
	return true;
}
