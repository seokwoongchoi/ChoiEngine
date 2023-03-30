#pragma once
class BehaviorTreeEditor
{
public:
	BehaviorTreeEditor(ID3D11Device* device);
	~BehaviorTreeEditor();

	void Save(BinaryWriter* w);
	void Initiallize(ID3D11Device* device);
	void Render();
	void ImageButton();

	class Texture* behaviorTreeTexture;
	 bool bShowBehaviorTree;


	 void LoadAndCompile(const string& path);
};

