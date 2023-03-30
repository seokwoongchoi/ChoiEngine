#pragma once
enum class BrushType
{
	None,
	Raise,
	Smooth,
	Splatting,
	Billboard,
};
class TerrainEditor
{
public:
	TerrainEditor(ID3D11Device* device, class Island11* island11);
	~TerrainEditor();

public:
	void ImGui();
	void Update(const Vector3& mousePos, ID3D11DeviceContext* context);
	void Compute(ID3D11DeviceContext* context);
	ID3D11Buffer* GetBrushBuffer(ID3D11DeviceContext* context);
private:
	void RaiseHeight(ID3D11DeviceContext* context);
	void SmoothBrush(ID3D11DeviceContext* context);
private:
	struct BrushDesc
	{
		Vector3 Color = Vector3(1, 0.2f, 0);
		uint Range = 10;
		Vector3 Location;
		uint BrushShape = 0; //그리지 않음
	}brushDesc;
	ID3D11Buffer* brushDescBuffer;

	struct UpdateDesc
	{

		Vector3 Location;
		uint Range = 10;
		uint BrushShape;
		float raiseSpeed = 0.1f;
		float Padding[2];


	}updateDesc;
	ID3D11Buffer*  updateDescBuffer;

	BrushType brushType;
private:
	ID3D11ComputeShader* UpdateHeightCS;
	ID3D11ComputeShader* SmoothingCS;
private:
	class Island11* island11;
};

