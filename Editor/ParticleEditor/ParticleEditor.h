#pragma once
class ParticleEditor
{
public:
	ParticleEditor(ID3D11Device* device, class Engine* engine, class Editor* editor,uint ID);
	~ParticleEditor();
public:
	bool IsDragged() { return bDrag; }
	void IsDragged(bool bDrag) { this->bDrag = bDrag; }
	void SetWorld(const Matrix& world);
	
	const Matrix& GetWorld();
	uint GetID() {
		return ID;
	}
	ReadParticleType GetParticleType()
	{
		return particleType;
	}

public:

	void PreviewRender(ID3D11DeviceContext* context);
	void Save(BinaryWriter* w);
private:
	class Editor* editor;
	class Engine* engine;
	uint ID;
private:
	void Editor();
	void ShowFrame(const ImVec2& size);
	void SelectParticleType(const ImVec2 & size);

	void CamUpdate();
public:
	void ImageButton();
	bool bEditing;
	void Compile();
	void Load(const wstring& file,bool IsLevel=false);
	void SetTexture(const wstring& file);
private:
	ID3D11Device* device;
	class ParticleSimulation* particle;

private:
	ID3D11Texture2D* backBuffer;
	ID3D11RenderTargetView* rtv;
	ID3D11ShaderResourceView* srv;

	ID3D11Texture2D* depthBackBuffer;
	ID3D11DepthStencilView* dsv;
	D3D11_VIEWPORT viewport;
private:
	Texture* buttonTextures[3];
	Texture* particleTexture;
	wstring textureName;

private:
	
	bool bSmokeSetted;
	bool bStart;
	bool bDrag;
private:
	Vector3 targetPosition;
	float distance;
	Vector2 rotation;
	Matrix view;
	Matrix proj;
	Vector3 viewRight;
	Vector3 viewUp;
	ImVec2 size;
	Vector3 EyePosition;
	ReadParticleType particleType;
	int BindEffectBone=0;
	string BoneCollider = "BodyCollider";
};

