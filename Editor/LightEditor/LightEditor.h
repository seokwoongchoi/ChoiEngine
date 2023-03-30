#pragma once
enum class LightType :uint
{
	unKnown,
	pointLight,
	spotLight,
	capsuleLight,
};
class LightEditor
{
public:
	LightEditor(ID3D11Device* device, class Editor* editor);
	~LightEditor();
	class Editor* editor;
	void Update(const Vector3& mousePos);
	void ImageButton();
	void SetDragDropPayload(const LightType& type, const wstring& data);
	void Save(BinaryWriter* w);
	void SetControllLightIndex(int index)
	{
		controllLightIndex = index;
	}
	void PushPointCount()
	{
		pointCount++;
	}
public:
	void GridPointLight();
	void GridSpotLight();
	void GridCapsuleLight();
public:
	int LightList();
	void GetLightPosition(Vector3& pos);
	void SetLightPosition(const Vector3& pos);
private:
	
	
	
	void PointLightRender();
	void SpotLightRender();
	void CapsuleLightRender();
private:
	void PointLightControll(int index);
	void SpotLightControll(int index);

private:
	void Projection(OUT Vector3* position, Vector3& source,const  Matrix& W,const  Matrix& V,const Matrix& P);
private:

	ID3D11Device* device;
	LightType lightType;
private:

	Texture* pointTexture;
	Texture* spotTexture;
	Texture* capsuleTexture;
private:
	bool bDrag;

	uint pointCount;
	uint spotCount;
	uint capsuleCount;
	Matrix world;

	vector<Vector3> v, v2, v3;

    int controllLightIndex;
};

