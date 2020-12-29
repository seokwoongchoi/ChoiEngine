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
	inline void Stop()
	{
		if (IsHitted)
		{
			Time::Get()->Stop();
		}
	}
	inline ID3D11UnorderedAccessView* UAV()
	{
		return modelData_StructuredBufferUAV;
	}
	void ComputeEditor(ID3D11DeviceContext* context,ID3D11UnorderedAccessView* effectPositionUAV,  const uint& skeletalCount, const uint& staticCount);
	void Compute(ID3D11DeviceContext* context, ID3D11UnorderedAccessView* effectPositionUAV, const uint& skeletalCount, const uint& staticCount);
	
	void EffectIndex(int index, int effect)
	{
		switch (index)
		{
		case 0:
		{
			bodyEffectIndex = effect;
		}
		break;
		case 1:
		{
			headEffectIndex = effect;
		}
		break;
		case 2:
		{
			swordEffectIndex = effect;
		}
		break;
		}
	}
private:
	void CreateCopyBuffer(ID3D11Device* device);
	void CreateAnimBoneBuffer(ID3D11Device* device);
private:
	Vector4 Result[10];
	Matrix  T;
	Vector3 dir;
	uint effectCounts[3];
	int bodyEffectIndex = -1;
	int headEffectIndex = -1;
	int swordEffectIndex = -1;
	
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
};

