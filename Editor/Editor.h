#pragma once

class Editor final
{
	
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
private:

	class Engine* engine;
	void ApplyStyle();

private:
	bool bStart;
	
	vector<class ActorEditor*> actors;
	ImGuiWindowFlags flags;
	class QuadTree* tree;
	Vector3 pos;
};