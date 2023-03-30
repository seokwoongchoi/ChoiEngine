#pragma once
class EffectSystem
{
	friend class Animator;
public:
	EffectSystem(ID3D11Device* device);
	~EffectSystem();
public:
	inline ID3D11UnorderedAccessView* UAV() {
		return position_StructuredBufferUAV;
	}

	inline ID3D11UnorderedAccessView** IndirectUAV() {
		return &indirectBufferUAV[0];
	}
	void LoadParticle(const uint & index, const wstring & path, const ReadParticleType & particleType);

	void Render(ID3D11DeviceContext* context, ID3D11ShaderResourceView* softParticleDepth);
	void ResetBodies(const int& index);
	void RegisterPhysics(class PhysicsSystem*physics)
	{
		this->physics = physics;
	}
private:
	void CreateIndirectIndex(uint index, uint count);

private:
	
	ID3D11Device* device;
	vector <  class ParticleSimulation*>particles;
	vector < uint>instanceCount;
	

private:
	void CreatePositionBuffer(ID3D11Device* device);
	ID3D11Texture2D			  *position_StructuredBuffer;
	ID3D11ShaderResourceView  *position_StructuredBufferSRV;
	ID3D11UnorderedAccessView *position_StructuredBufferUAV;


	ID3D11Buffer		  *indirectBuffer[3];
	ID3D11UnorderedAccessView *indirectBufferUAV[3];

	class PhysicsSystem* physics;

};

