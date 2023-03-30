#pragma once
#define MAX_ACTOR_BONECOLLIDER 3
enum class EditMode :uint
{
	Animator,
	Render,

};

enum class GizmoMode :uint
{
	Default,
	SkeletalBone,
	StaticBone,
	Collider,
	Effect

};
class ActorEditor
{
	
private:
	EditMode mode;
public:
	explicit ActorEditor(ID3D11Device* device,class Engine* engine,class Editor* editor);
	~ActorEditor();


	ActorEditor(const ActorEditor&) = delete;
	ActorEditor& operator=(const ActorEditor&) = delete;

	class Editor* editor;
	class Engine* engine;
	void Save(BinaryWriter* w);
	void Load(BinaryReader* r);
private:
	void Save();
	
public:
	void Render(ID3D11DeviceContext* context);
public:
	bool IsDragged() { return bDrag; }
	void IsDragged(bool bDrag) { this-> bDrag= bDrag; }

	bool IsMove() { return bMove; }
public:
	
	const uint& ActorIndex(){return  actorIndex;}
	
	const ReadMeshType& MeshType() { return meshType; }
public:

	void SetDragDropPayload(const string& data);

	const Matrix& PreviewWorld();
public:
	void BarrierModelUse();
	bool ImageButton();
	void EditingMode();
	void Compile();
	void Editor();
	void ShowFrame(const ImVec2 & size);

	void LoadBehaviorTree();
private:
	GizmoMode gizmoMode;
	void ImGizmo();

	bool PreviewGizmoSet(ImGuizmo::MODE mode, ImGuizmo::OPERATION operation);
	void SkeletalGizmo(ImGuizmo::MODE mode, ImGuizmo::OPERATION operation);
	void StaticGizmo(ImGuizmo::MODE mode, ImGuizmo::OPERATION operation);
	void ColliderGizmo(ImGuizmo::MODE mode, ImGuizmo::OPERATION operation,const int& colliderIndex);
	
	int gizmoColliderIndex;
private:
	void ShowTransfomrs();
private:
	uint currentClipNum;
	void ShowAnimList(const ImVec2 & size);
	void ShowAnimFrame(const ImVec2 & size);
private:
	void ShowComponents(const ImVec2 & size);
	void ShowComponentPopUp();
	void ShowComponentListPopUp(const string& componentName);

	vector<string>componentList; 
	string componentName = "N/A";
private:
	void ShowMaterial(const ImVec2 & size);
	void ShowHierarchy();
	void ShowStaticBones(shared_ptr<class ModelBone> bone);
	void ShowSkeletalBones(shared_ptr<class ModelBone> bone);
	void ShowHierarcyPopup();
private:
	void CreateBox();
private:
	void BlendMesh();
	void CreateAttachMesh();
	void LoadAttachMesh(const wstring& file);
	shared_ptr<class ModelBone> currentBone;
private:
	vector<wstring>clipList;
	void ClipFinder(const wstring& file);
	void LoadSkeletalMesh(const wstring& file);
	void LoadStaticMesh(const wstring& file);
	void LoadMaterial(function<void(wstring, uint, shared_ptr<class Material>)> f, uint num, shared_ptr<class Material> material);
	void SetMaterial(wstring& file, uint textureType, shared_ptr<class Material> material);
private:
	Texture* buttonTextures[3];
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
	D3D11_VIEWPORT viewport;
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
	bool bMove;
	bool bHasEffect;
	bool bBindedTree;
	
	ReadMeshType meshType;
private:
	uint actorIndex;
	ImVec2 size;
	float camSpeed;
	ImGuiWindowFlags windowFlags;

private://Gizmo
	Matrix identity;
	Matrix gizmoWorld;
	Vector3 gizmoSnap;
	Matrix gizmoDelta;
	Matrix previewWorld;


};

