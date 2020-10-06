#pragma once
enum class ReadMeshType:uint
{
	Default,
	StaticMesh,
	SkeletalMesh,
};

enum class ReadParticleType :uint
{
	Default,
	Spark,
	Blood,
	Smoke
};

struct GlobalViewData
{
	Matrix view;
	Matrix proj;
	
	Vector3 pos;
	Vector3 dir;
	

	Vector3 lookAt;
};
class GlobalData
{
public:
	inline static const void SetWorldViewData(GlobalViewData* data)
	{
		viewData = *data;
		
	}
	inline static const Matrix& GetView() { return GlobalData::viewData.view; }
	inline static const Matrix& GetProj() { return GlobalData::viewData.proj; }
	inline static const Matrix& GetVP() { GlobalData::VP = GlobalData::viewData.view * GlobalData::viewData.proj;	return GlobalData::VP; }
	
	inline static const Vector3& Position() { return GlobalData::viewData.pos; }
	inline static const Vector3& Forward() { return GlobalData::viewData.dir; }
	inline static const Vector3& LookAt() { return GlobalData::viewData.lookAt; }
	

	inline static const Vector3& LightDirection() { return GlobalData::LightDir; }
	inline static const Vector3& LightColor() 
	{
	
		D3DXVec3Lerp(&GlobalData::LightCol, &GlobalData::yellow, &GlobalData::white, GlobalData::factor);

		return GlobalData::LightCol;
	}

	inline static const void LightDirection(const Vector3&LightDir) { GlobalData::LightDir=LightDir; }
	inline static const void LightColor(const float&factor) { GlobalData::factor =  factor; }

	static GlobalViewData viewData;
	static Matrix VP;

	static Vector3 yellow;
	static Vector3 white;
	static Vector3 LightDir;
	static Vector3 LightCol;
	static float factor;
};

