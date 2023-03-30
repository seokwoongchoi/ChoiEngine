#pragma once
class PhysicsSystem
{
	friend class Animator;
#ifdef EDITORMODE
	friend class Editor;
#endif
public:
	PhysicsSystem(ID3D11Device* device,class Animator* animator,class  EffectSystem* effects);
	~PhysicsSystem();
private:
	void CollisonSword(const uint& attackerIndex, const uint& targetIndex);
	void CollisonBody(const uint& attackerIndex, const uint& targetIndex);
	void CollisonHead(const uint& attackerIndex, const uint& targetIndex);
	void CollisonBodyBody(const uint& attackerIndex, const uint& targetIndex);
	bool CheckCollision();

	
public:
	void Collison(const uint& skeletalCount);
	void Stop();
	
	inline ID3D11UnorderedAccessView* UAV()
	{
		return modelData_StructuredBufferUAV;
	}
	
	void Compute(ID3D11DeviceContext* context, ID3D11UnorderedAccessView* effectPositionUAV, const uint& skeletalCount, const uint& staticCount);
	
	void EffectIndex(int index, int effect)
	{
		switch (index)
		{
		case 0:
		{
			bodyEffectIndex = effect;
			SetIndirectUAV(bodyEffectIndex);
		}
		break;
		case 1:
		{
			headEffectIndex = effect;
			SetIndirectUAV(headEffectIndex);
		}
		break;
		case 2:
		{
			swordEffectIndex = effect;
			SetIndirectUAV(swordEffectIndex);
		}
		break;
		}
	}

	void SetIndirectUAV(const int& effectIndex);
private:
	void CreateCopyBuffer(ID3D11Device* device);
	void CreateAnimBoneBuffer(ID3D11Device* device);
private:
	//uint Result[15*4];
	Vector4 Result[15];
	Matrix  T;
	Vector3 dir;
	
	int bodyEffectIndex = -1;
	int headEffectIndex = -1;
	int swordEffectIndex = -1;
	
	vector< ID3D11UnorderedAccessView*>uavArray;
private:
	class Animator* animator;
	class EffectSystem* effects;
protected:
	ID3D11Texture2D			  *modelData_StructuredBuffer;
	ID3D11ShaderResourceView  *modelData_StructuredBufferSRV ;
	ID3D11UnorderedAccessView *modelData_StructuredBufferUAV ;
private:
	ID3D11Buffer			   *copy_StructuredBuffer;
	ID3D11UnorderedAccessView  *copy_StructuredBufferUAV;
	ID3D11Buffer		       *copyBuffer;



private:
	ID3D11ComputeShader* CollisonCS;
	struct CB_DrawCount
	{
		uint drawCount;
		uint skeletalCount;
		uint staticCount;
		uint pad;
	}drawDesc;
	
	ID3D11Buffer* drawBuffer;
	
	bool IsHitted;
	int hittedActorIndex = -1;
	int hittedIndex = -1;

protected:
	struct CS_AnimBoneBoxDesc
	{
		Matrix boxes[3];
	};
	CS_AnimBoneBoxDesc* csAnimBoneBoxData;

	bool bCheck = false;


	uint countResult = 0;
	int lastResult = -1;
};

