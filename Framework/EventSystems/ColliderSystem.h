#pragma once
#include "Renderers/Renderer.h"

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
	inline ID3D11ShaderResourceView* GetInstBufferSRV() {
		return InstBufferSRV;
	}
	inline ID3D11ShaderResourceView* SRV() {
		return srv;
	}
	//void UpdateInstBuffer();

	uint FrustumCulling(const uint& index);
public:
	 Vector3* GetBoxMinMax(const uint& actorIndex, const uint& drawCount);
	
	 inline void GetInstMatrix(Matrix* const pointer, const uint& actorIndex)
	 {
		 *pointer = instTransforms[actorIndex][renderDatas[actorIndex].drawCount - 1];
	 }
	 inline void GetInstMatrix(Matrix* const pointer,const uint& actorIndex, const uint& index)
	 {
		 *pointer= instTransforms[actorIndex][index];
	 }

	 inline void GetPosition(Vector3* const pointer,const uint& actorIndex, const uint& index)
	 {
		 *pointer = instTransforms[actorIndex][index].m[3];
	 }
	 inline void GetFoward( Vector3 * const pointer,const uint& actorIndex, const uint& index)
	 {
		*pointer = instTransforms[actorIndex][index].m[2];
	 }
	
	 inline void SetInstMatrix(Matrix* const world, const uint& index)
	 {
		 memcpy(instTransforms[index][renderDatas[index].drawCount - 1], world, sizeof(Matrix));
		
		 // UpdateInstBuffer();
	 }
	 inline void SetInstMatrix(Matrix* const world,const uint& actorIndex, const uint& index)
	 {
		 memcpy(instTransforms[actorIndex][index], world,sizeof(Matrix));
		// UpdateInstBuffer();
	 }
public:
	 const uint& ActorCount() {	 return actorCount; }
     void SetActorCount(const uint& actorCount)	 { this->actorCount = actorCount; }
	 const uint& DrawCount(const uint& index);
	 void RenderandCulledCount(const uint& index, uint& CulledCount);
	inline  const uint& TotalCount() { return totalCount; }

	
	 void RegisterRenderData(const uint& index, class Renderer* renderer);
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
	struct RenderData
	{
		uint drawCount = 0;
		uint culledCount = 0;
		uint prevDrawCount = 0;
		int btIndex = -1;
		Vector3 boxMin = Vector3(0, 0, 0);
		Vector3 boxMax = Vector3(0, 0, 0);
	};
	vector<RenderData>renderDatas;
	//unordered_map<uint, class Renderer*>renderers;
	//unordered_map<uint,uint>culledCount;

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
