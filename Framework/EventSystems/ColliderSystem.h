#pragma once
#include "Renderers/Renderer.h"
#define MAX_BONE_TRANSFORMS 100
#define MAX_ACTOR_COUNT 5
#define MAX_MODEL_INSTANCE 10

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
	inline ID3D11ShaderResourceView* SRV() {return srv; }
	
	inline ID3D11ShaderResourceView* GetShadowInstBufferSRV() { return ShadowInstBufferSRV; }
	inline ID3D11ShaderResourceView* GetInstBufferSRV() { return InstBufferSRV; }
public:
	uint FrustumCulling(const uint& index);
public:
	inline void SetPosition(const Vector3& pos,float delta, const uint& actorIndex, const uint& index)
	{
		
		instTransforms[actorIndex][index].m[3][0] -= pos.x*delta;
		instTransforms[actorIndex][index].m[3][2] -= pos.z*delta;
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
	 inline void SetInstMatrix(Matrix* const world,const uint& actorIndex, const uint& index)
	 {
		
		 memcpy(&instTransforms[actorIndex][index], world,sizeof(Matrix));
	 }
public:
	inline const uint& DrawCount(const uint& actorIndex)
	{

		return  renderDatas[actorIndex].drawCount;
	 }
	inline const uint& PrevDrawCount(const uint& actorIndex)
	{
		return  renderDatas[actorIndex].prevDrawCount;
	 }
	inline void SaveCount(const uint& actorIndex,uint& count)
	{
		if (actorIndex < 1)
		{
			count = 0;
		}
		uint temp = 0;
		for (uint i = 0; i < actorIndex; i++)
		{
			temp+= renderDatas[i].drawCount;
		}
		count=  temp;
	}
	inline void GetActorIndex(const uint& index,uint& actorIndex,uint& instanceIndex)
	{
		instanceIndex = index;
		if (actorCount < 2)
		{
			actorIndex= 0;
			
			return;
		}

	
		for (uint i = actorCount-1; i > 0; i--)
		{
			uint count = 0;
		
			for (uint y = 0; y < i; y++)
				count += renderDatas[y].drawCount;


			if (index >= count)
			{
				actorIndex =i;
				
				
				for (uint t = 0; t < actorIndex; t++)
					instanceIndex -= DrawCount(t);

				return;
				
			}
			
		}
		actorIndex = 0;
		
		return;
		
	}
	inline const uint& TotalCount() { return totalCount; }
public:
	void BindPipeline(ID3D11DeviceContext* context);
	void UpdateInstTransformSRV(ID3D11DeviceContext* context);
public:
	void ActorCount(uint& actorCount) {
		 for (uint i = 0; i < this->actorCount; i++)
		 {
			 if (renderDatas[i].drawCount < 1)
			 {
				 actorCount = (this->actorCount-1);
				 return;
			 }
		 }
		 actorCount= this->actorCount;
		 return;
	 }
     void SetActorCount(const uint& actorCount)	 { this->actorCount = actorCount; }
public:
	 void RegisterRenderData(const uint& index, class Renderer* renderer);
     void PushDrawCount(const uint& index, const Matrix& world,bool bCrateShadowBuffer=false);
	 vector<Vector3> GetBoxMinMax(const uint& actorIndex, const uint& drawCount);
public:
	void CreateInstTransformSRV();
	void CreateShadowInstTransformSRV();
	void ClearTextureTransforms();
	void ReadBone(BinaryReader* r,uint actorIndex);
protected:
	void CreateModelTransformSRV();
private:
	void BindingBone();
protected:
	vector<vector<Matrix>>instTransforms;
	Matrix OneDimensionalInstTransforms[MAX_MODEL_INSTANCE+5];
	struct RenderData
	{
		uint drawCount = 0;
		uint prevDrawCount = 0;
		uint saveCount = 0;
		
		Vector3 boxMin = Vector3(0, 0, 0);
		Vector3 boxMax = Vector3(0, 0, 0);
	};
	vector<RenderData>renderDatas;
	uint totalCount;
	uint actorCount;
	
protected:
	class Frustum* frustum;
	Vector3 max;
	Vector3 min;
	
protected:
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* srv;
	
	ID3D11Texture2D			  *InstBuffer;
	ID3D11ShaderResourceView  *InstBufferSRV;


	ID3D11Texture2D			  *ShadowInstBuffer;
	ID3D11ShaderResourceView  *ShadowInstBufferSRV;
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

	ID3D11ComputeShader* ColliderCS;
protected:
	ID3D11Device* device;
	uint actorIndex;
	vector<Matrix>shadowMtrix;
private:
	class BoneTransform* boneTexture;
	
};
