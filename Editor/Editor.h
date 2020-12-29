#pragma once
#include "Utility/QuadTree.h"
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
	void Save(const wstring& fileName);
	void Load(const wstring& fileName);
	void LoadThread(const wstring& fileName);
public:
	void RenderTexture(Vector2 position, Vector2 scale, Color color, Texture* texture);
private:
	void ActorAsset();
	void ParticleAsset();
	void BehaviorTreeAsset();
	void MenuBar();
	void ToolBar();
	class Texture* buttonTextures[3];
private:
	void EditorMenu();
private:
	void AddTrasform(const Vector3& pos);
	void DropParticle(const Vector3& pos);
	void DebugRender();

	void QuadTreeRender(shared_ptr<QuadTreeNode> node);
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
	ImGuiWindowFlags flags;
	class QuadTree* tree;
	Vector3 pos;
	 

	bool IsClicked;
	int clickedActorIndex;
	int clickedInstanceIndex;
	ReadMeshType clickedMeshType;
protected:
	uint staticActorCount;
	uint skeletalActorCount;

	Matrix identity;
	bool bShowCollider;

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

	
};