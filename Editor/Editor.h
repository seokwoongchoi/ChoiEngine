#pragma once

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
private:
	void ActorAsset();
	void MenuBar();
	void ToolBar();
	class Texture* buttonTextures[3];
private:
	void EditorMenu();
private:
	void AddTrasform(const Vector3& pos);
	void DebugRender();
private:
	
	class Engine* engine;
	void ApplyStyle();

private:
	bool bStart;
	
	vector<shared_ptr<class ActorEditor>> actors;
	ImGuiWindowFlags flags;
	class QuadTree* tree;
	Vector3 pos;
	 

	bool IsPushed;
	int pushedActorIndex;
	ReadMeshType pushedMeshType;
protected:
	uint staticActorCount;
	uint skeletalActorCount;

	Matrix identity;
	bool bShowCollider;
};