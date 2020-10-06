#pragma once
#include "Renderer.h"
class ColliderSystem
{
#ifdef EDITORMODE

	friend class ActorEditor;
#endif
public:
	explicit ColliderSystem(ID3D11Device* device,const string& path,const string& entryPoint);
	~ColliderSystem()
	{
		SafeRelease(srv);
		SafeRelease(texture);
	};

	 
private:

	ColliderSystem(const ColliderSystem & )=delete;
	ColliderSystem & operator= (const ColliderSystem & ) = delete;


public:
	
	//void UpdateInstBuffer();

	void FrustumCulling(const uint& index);
public:
	 Vector3* GetBoxMinMax(const uint& actorIndex, const uint& drawCount);
	 inline const Matrix& GetInstMatrix(const uint& index)
	 {
		 return instTransforms[index][renderers[index]->drawCount - 1];
	 }
	 inline const Matrix& GetInstMatrix(const uint& actorIndex, const uint& index)
	 {
		 return instTransforms[actorIndex][index];
	 }
	 inline const Vector3& GetPosition(const uint& actorIndex, const uint& index)
	 {
		 return Vector3(instTransforms[actorIndex][index]._41, instTransforms[actorIndex][index]._42, instTransforms[actorIndex][index]._43);
	 }
	 inline void SetInstMatrix(const uint& index, const Matrix& world)
	 {
		 instTransforms[index][renderers[index]->drawCount - 1] = world;
		// UpdateInstBuffer();
	 }
	 inline  void SetInstMatrix(const uint& actorIndex, const uint& index, const Matrix& world)
	 {
		 instTransforms[actorIndex][index] = world;
		// UpdateInstBuffer();
	 }
public:
	 const uint& ActorCount() {	 return actorCount; }
     void SetActorCount(const uint& actorCount)	 { this->actorCount = actorCount; }
	 const uint& DrawCount(const uint& index);
	 void RenderandCulledCount(const uint& index, uint& CulledCount);
	inline  const uint& TotalCount() { return totalCount; }

	
	 void RegisterRenderer(class Renderer* renderer, const uint& index);
     void PushDrawCount(const uint& index, const Matrix& world);
public:
	void ClearTextureTransforms();
	
	void BindPipeline(ID3D11DeviceContext* context);
	void ReadBone(BinaryReader* r);
public:
	void CreateInstTransformSRV();

protected:
	void CreateModelTransformSRV();
private:
	void BindingBone();
protected:
	Matrix instTransforms[3][10];
	unordered_map<uint, class Renderer*>renderers;
	unordered_map<uint,uint>culledCount;

	ID3D11Texture2D			  *InstBuffer;
	ID3D11ShaderResourceView  *InstBufferSRV;
	uint totalCount;
	uint actorCount;

protected:
	struct CB_DrawCount
	{
		uint staticDrawCount = 0;
		uint skeletalDrawCount = 0;
		uint particleIndex = 0;
		float pad = 0;
	}drawCountDesc;

	ID3D11Buffer* drawBuffer;
	
protected:
	class Frustum* frustum;
	Vector3 max;
	Vector3 min;
	Vector3 dest[8];
	Vector3  temp[8];

protected:
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* srv;
protected:
	
	class BoneTransform* boneTexture;
	ID3D11ComputeShader* ColliderCS;
protected:
	ID3D11Device* device;
};

