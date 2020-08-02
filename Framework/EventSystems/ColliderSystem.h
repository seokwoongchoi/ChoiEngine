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

	 Vector3* GetBoxMinMax(const uint& actorIndex, const uint& drawCount);
	 const Matrix& GetInstMatrix(const uint& index);
	 void SetInstMatrix(const uint& index,const Matrix& world);
	 const uint& ActorCount()
	 {
		 return actorCount;
	 }
	
	void SetActorCount(const uint& actorCount)
	{
		this->actorCount = actorCount;
	}

	 const uint& DrawCount(const uint& index);
	void RegisterRenderer(class Renderer* renderer,const uint& index)
	{
		if (renderers[index])
		{
			SafeDelete(renderers[index]);
		}
		renderers[index] = renderer;
	
	}
	void PushDrawCount(const uint& index, const Matrix& world);
	
public:
	void ClearTextureTransforms();

	void BindPipeline(ID3D11DeviceContext* context);
	void ReadBone(BinaryReader* r);


	void CreateModelTransformSRV();
	void CreateInstTransformSRV();
private:
	void BindingBone();
	

private:
	ID3D11Device* device;

	unordered_map<uint, class Renderer*>renderers;
protected:
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
	Vector3 dest[8];
};

