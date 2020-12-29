#include "Framework.h"
#include "QuadTree.h"
//#include "../Editor/Debug/DebugLine.h"

QuadTree::QuadTree()
	:org(0,0,0),dir(0,0,0),pos(0,0,0), height(512.0f),width(512.0f)
{
	D3DXMatrixIdentity(&W);
	root = make_shared< QuadTreeNode>();

}


QuadTree::~QuadTree()
{
}

void QuadTree::BoxRender()
{
	BoxRender(root);
}

void QuadTree::BoxRender(shared_ptr<QuadTreeNode> node)
{
	//D3DXVECTOR3 dest[8];

	////cout << "boundsMax.y:";
	////cout << node->boundsMax.y << endl;

	//dest[0] = Vector3(node->boundsMin.x, node->boundsMin.y, node->boundsMax.z);
	//dest[1] = Vector3(node->boundsMax.x, node->boundsMin.y, node->boundsMax.z);
	//dest[2] = Vector3(node->boundsMin.x, node->boundsMax.y, node->boundsMax.z);
	//dest[3] = Vector3(node->boundsMax.x, node->boundsMax.y, node->boundsMax.z);
	//dest[4] = Vector3(node->boundsMin);
	//dest[5] = Vector3(node->boundsMax.x, node->boundsMin.y, node->boundsMin.z);
	//dest[6] = Vector3(node->boundsMin.x, node->boundsMax.y, node->boundsMin.z);
	//dest[7] = Vector3(node->boundsMax.x, node->boundsMax.y, node->boundsMin.z);

	////
	////D3DXMATRIX world = transform->World();
	//////D3DXMatrixTranspose(&world, &transform->World());
	////for (UINT i = 0; i < 8; i++)
	////	D3DXVec3TransformCoord(&dest[i], &dest[i], &world);

	//Color color = node->hitted ? Color(1, 0, 0, 1) : Color(0, 0, 1, 1);
	////Front
	//DebugLine::Get()->RenderLine(dest[0], dest[1], color);
	//DebugLine::Get()->RenderLine(dest[1], dest[3], color);
	//DebugLine::Get()->RenderLine(dest[3], dest[2], color);
	//DebugLine::Get()->RenderLine(dest[2], dest[0], color);

	////Backward
	//DebugLine::Get()->RenderLine(dest[4], dest[5], color);
	//DebugLine::Get()->RenderLine(dest[5], dest[7], color);
	//DebugLine::Get()->RenderLine(dest[7], dest[6], color);
	//DebugLine::Get()->RenderLine(dest[6], dest[4], color);

	////Side
	//DebugLine::Get()->RenderLine(dest[0], dest[4], color);
	//DebugLine::Get()->RenderLine(dest[1], dest[5], color);
	//DebugLine::Get()->RenderLine(dest[2], dest[6], color);
	//DebugLine::Get()->RenderLine(dest[3], dest[7], color);

	//for (auto& child : node->childs)
	//{
	//	BoxRender(child);
	//}
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
void QuadTree::Intersection(Matrix * const  matrix)
{
	
	Vector3 org = Vector3(matrix->_41, matrix->_42 + 5, matrix->_43);
	Vector3 actorPos;
	if (!root->Intersection(org, Vector3(-0.0f, -1.0f, -0.000001f), actorPos))
	{
		return;
	}
	Vector2 lerp;
	D3DXVec2Lerp(&lerp, &Vector2(0, matrix->_42), &Vector2(0,actorPos.y), Time::Delta()*5.0f);
	matrix->_42 = lerp.y;
}
bool QuadTree::InBounds(uint row, uint col)
{
	return row >= 0 && row < height && col >= 0 && col < width;
}
void QuadTree::CreateQuadTree(Vector2 lt, Vector2 rb, bool calcHeight)
{
	CreateQuadTree(root, lt, rb,calcHeight);
}

void QuadTree::CreateQuadTree(shared_ptr<QuadTreeNode> node, Vector2 leftTop, Vector2 rightBottom,bool calHeight)
{
	static const float TileSize = 2.0f;
	static const float  tolerance = 0.01f;
	
	
	node->minX = leftTop.x ;
	node->maxX = rightBottom.x;



	node->minZ = leftTop.y ;
	node->maxZ = rightBottom.y;

	
	node->minX -= tolerance;
	node->maxX += tolerance;
	node->minZ += tolerance;
	node->maxZ -= tolerance;

	if (calHeight == true)
	{
		float minY = FLT_MAX;
		float maxY = -FLT_MAX;

		
		uint lt = ((uint)leftTop.y-512)*-1;
		uint rb =((uint)rightBottom.y-512)*-1;
			for (uint x = (uint)leftTop.x; x < (uint)rightBottom.x; x++)
			for (uint y = rb; y < lt; y++)
            {
             
             
            		float data = 0.0f;
             
            		data = heightMap[x][y];
             
            		minY = min(minY, data);
            		maxY = max(maxY, data);
            		
            	
            }
                
		node->minMaxY = Vector2(minY, maxY);
		
	}
	else
	{
		node->minMaxY = Vector2(0.0f, 0.0f);
	}



	node->boundsMin = Vector3(node->minX, node->minMaxY.x, node->minZ);
	node->boundsMax = Vector3(node->maxX, node->minMaxY.y, node->maxZ);
	



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


		CreateQuadTree(node->childs[0],  leftTop, Vector2(leftTop.x + nodeWidth, leftTop.y + nodeDepth), calHeight);
		CreateQuadTree(node->childs[1],  Vector2(leftTop.x + nodeWidth, leftTop.y), Vector2(rightBottom.x, leftTop.y + nodeDepth), calHeight);
		CreateQuadTree(node->childs[2],  Vector2(leftTop.x, leftTop.y + nodeDepth), Vector2(leftTop.x + nodeDepth, rightBottom.y), calHeight);
		CreateQuadTree(node->childs[3],  Vector2(leftTop.x + nodeWidth, leftTop.y + nodeDepth), rightBottom, calHeight);



	}

}
void QuadTree::GetRay(OUT Vector3 * position, OUT Vector3 * direction, const Matrix & w, const Matrix & v, const Matrix & p)
{
	Vector3 mouse = Mouse::Get()->GetPosition();

	Vector2 point;
	//Inv Viewport
	{
		point.x = (((2.0f*mouse.x) / D3D::Width()) - 1.0f);
		point.y = (((2.0f*mouse.y) / D3D::Height()) - 1.0f)*-1.0f;

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
bool Compare(shared_ptr<QuadTreeNode>& a, shared_ptr<QuadTreeNode>& b)
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
