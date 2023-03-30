#pragma once
#include "Utility/QuadTree.h"

enum class GizmoType :uint
{
	Default,
	StaticMesh,
	SkeletalMesh,
	Light,
	Particle,
	DebugFigures
};
struct BoxRayStruct
{
	Vector3 max;
	Vector3 min;

	uint actorIndex;
	uint instanceIndex;
};

struct DebugCube
{
	Matrix world;


	DebugCube()
	{
		D3DXMatrixIdentity(&world);
	}
};
class Editor final
{
	friend class ActorEditor;
public:
	Editor();
	~Editor();

	Editor(const Editor&) = delete;
	Editor& operator=(const Editor&) = delete;
	
	void Resize(const uint& width, const uint& height);
	void Update();
	void Render();
	void PostEffects();
	void Cloud();
	void Colliders();
	void Save(const wstring& fileName);
	void LoadText(const wstring& fileName);
	void Load(const wstring& fileName);
	void LoadThread(const wstring& fileName);
public:
	void RenderTexture(Vector2 position, Vector2 scale, Color color, Texture* texture);
private:
	void Gizmo();
	void ActorAsset();
	void ParticleAsset();
	int SpaceParticleList();
	void BehaviorTreeAsset();
	void MenuBar();
	void ToolBar();
	class Texture* buttonTextures[5];
private:
	void EditorMenu();
private:
	void AddTrasform(const Vector3& pos);
	void DropParticle(const Vector3& pos);
	void DropCube(const Vector3& pos);
private:
	void DebugRender();

	void QuadTreeRender(shared_ptr<QuadTreeNode> node);
	void QuadTreeRender(shared_ptr<QuadTreeNode> node,uint NodeID);

	void QuadTreeEditor();
	void RenderDebugCube();
	
private:
	void BoxRayTracing();
	bool IntersectionAABB(const Vector3& org, const Vector3& dir, Vector3 & Pos, const Vector3& boundsMin, const Vector3& boundsMax);
private:
	
	class Engine* engine;
	void ApplyStyle();
private:
	class Camera* mainCamera;
	
private:
	bool bStart;
	
	vector<shared_ptr<class ActorEditor>> actors;
	vector<shared_ptr<class ParticleEditor>> particles;
	vector<shared_ptr<class BehaviorTreeEditor>> behaviorTrees;
	shared_ptr<class LightEditor>lights;
	shared_ptr<class TerrainEditor>terrainEditor;
	ImGuiWindowFlags flags;
	class QuadTree* tree;
	Vector3 pos;
	Matrix v, p;

	
	int clickedActorIndex;
	int clickedInstanceIndex;
	GizmoType clickedMeshType;
protected:
	uint staticActorCount;
	uint skeletalActorCount;

	Matrix identity;
	bool bShowCollider;
	bool bShowQuadTree;

	Vector3 boneBoxCollider[8];

private:
	struct GuiTexture
	{
		Vector2 Position;
		Vector2 scale;
		class Texture* texture;
		Color color;

		GuiTexture()
		{

		}
	};

	vector<GuiTexture> textures;

	vector<string>spaceParticles;
	vector<uint>keys;
	int controllParticleIndex;
	bool IsClicked;
	bool bNeedUpdate = false;
	bool isMove = false;


	vector<BoxRayStruct> boxSort;

	class TextReader* reader;


	vector<DebugCube>debugCubes;
	bool bSelectedCube;
	int controllCubeIndex;
};