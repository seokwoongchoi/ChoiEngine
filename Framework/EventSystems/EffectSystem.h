#pragma once
class EffectSystem
{
	friend class Animator;
public:
	EffectSystem(ID3D11Device* device);
	~EffectSystem();
public:
	inline ID3D11UnorderedAccessView* UAV() {
		return position_StructuredBufferUAV
			;
	}
	void LoadParticle(const uint & index, const wstring & path, const ReadParticleType & particleType);

	void Render(ID3D11DeviceContext* context);
	void ResetBodies(const uint& index, const uint& effectCount);
private:

	ID3D11Device* device;
	vector <  class ParticleSimulation*>particles;
	vector < uint>instanceCount;
	
	bool bLoaded;
private:
	void CreatePositionBuffer(ID3D11Device* device);
	ID3D11Texture2D			  *position_StructuredBuffer;
	ID3D11ShaderResourceView  *position_StructuredBufferSRV;
	ID3D11UnorderedAccessView *position_StructuredBufferUAV;
};

