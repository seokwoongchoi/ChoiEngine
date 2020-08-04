#pragma once
class ColliderSystem
{
	friend class Animator;
public:
	ColliderSystem(ID3D11Device* device,const string& path,const string& entryPoint);
	~ColliderSystem()
	{
		SafeRelease(srv);
		SafeRelease(texture);
	};


public:
	void SortInstMatrix(const uint & actorIndex, const uint & drawIndex);
	const int& FrustumCulling(const uint& index);
public:
	 Vector3* GetBoxMinMax(const uint& actorIndex, const uint& drawCount);
	 const Matrix& GetInstMatrix(const uint& index);
	 void SetInstMatrix(const uint& index,const Matrix& world);
public:
	 const uint& ActorCount() {	 return actorCount; }
     void SetActorCount(const uint& actorCount)	 { this->actorCount = actorCount; }
	 const uint& DrawCount(const uint& index);
	 void RegisterRenderer(class Renderer* renderer, const uint& index);
     virtual void PushDrawCount(const uint& index, const Matrix& world);
public:
	void ClearTextureTransforms();
	void BindPipeline(ID3D11DeviceContext* context);
	void ReadBone(BinaryReader* r);

private:
	void CreateModelTransformSRV();
	void CreateInstTransformSRV();
private:
	void BindingBone();
private:

private:
	ID3D11Device* device;
protected:
	unordered_map<uint, class Renderer*>renderers;
	ID3D11ShaderResourceView  *InstBufferSRV;
private:
	ID3D11Texture2D			  *InstBuffer;
	class InstTransform* instTexture;
private:
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* srv;
private:
	uint actorCount;
	class BoneTransform* boneTexture;
private:
	ID3D11ComputeShader* ColliderCS;

private:
	class Frustum* frustum;
	Vector3 dest[8];
	Vector3  temp[8];
	unordered_map<uint, uint>culledCount;
protected:
	struct CB_DrawCount
	{
		uint staticDrawCount=0;
		uint skeletalDrawCount=0;
		uint particleIndex =0;
		float pad=0;
	}drawCountDesc;

	ID3D11Buffer* drawBuffer;
};

