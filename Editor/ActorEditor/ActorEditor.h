#pragma once

enum class EditMode :uint
{
	Animator,
	Render,

};
class ActorEditor
{
public:
	explicit ActorEditor(ID3D11Device* device);
	~ActorEditor();


	ActorEditor(const ActorEditor&) = delete;
	ActorEditor& operator=(const ActorEditor&) = delete;

	void Render(ID3D11DeviceContext* context);
public:
	bool IsDragged() { return bDrag; }
	void IsDragged(bool bDrag) { this-> bDrag= bDrag; }

	bool IsComiled() { return bCompiled; }
public:
	void ActorIndex(const uint& actorIndex){this->actorIndex = actorIndex;}
	const uint& ActorIndex(){return  actorIndex;}

public:

	void SetDragDropPayload(const string& data);


public:
	void ImageButton(class Engine* engine);
	bool EditingMode();
	void Compile();
	void Editor();
	void ShowFrame(const ImVec2 & size);

private:
	void ShowComponents(const ImVec2 & size);
	void ShowComponentPopUp();
	void ShowComponentListPopUp(const string& componentName);
	vector<string>componentList; string componentName = "N/A";
private:
	void ShowMaterial(const ImVec2 & size);
	void ShowHierarchy();
	void ShowBone(shared_ptr<class ModelBone> bone);
	void ShowChild(shared_ptr<class ModelBone> bone);
	void ShowHierarcyPopup();
	void BlendMesh();

	shared_ptr<class ModelBone> currentBone;
private:

	void LoadSkeletalMesh(const wstring& file);
	void LoadStaticMesh(const wstring& file);
	void LoadMaterial(function<void(wstring, uint, Material*)> f, uint num, shared_ptr<class Material> material);
	void SetMaterial(wstring& file, uint textureType, shared_ptr<class Material> material);

private:
	ID3D11Device* device;
	wstring modelName;
private:
	class PreviewRenderer* previewRender;
private:
	ID3D11Texture2D* backBuffer;
	ID3D11RenderTargetView* rtv;
	ID3D11ShaderResourceView* srv;

	ID3D11Texture2D* depthBackBuffer;
	ID3D11DepthStencilView* dsv;
	
private:
	bool bActive;
	bool bEditing;
	bool bFirst;
	bool bDrag;
	bool bLoaded;
	bool bModelLoaded;
	bool bBone;
	bool bBlend;
	bool bCompiled;
	bool bHasEffect;
	ReadMeshType meshType;
private:
	uint actorIndex;
	ImVec2 size;
};

