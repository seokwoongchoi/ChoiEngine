#pragma once
#include "ParticleSimulation.h"
class Sparks:public ParticleSimulation
{

#ifdef EDITORMODE
	friend class ParticleEditor;

#endif
	friend class EffectSystem;
public:
	Sparks(ID3D11Device* device, uint ID);
	~Sparks();

public:
	void InitBodies(const wstring& path);
	void InitBodies();
	void Update(ID3D11DeviceContext* context)override;
	void Render(ID3D11DeviceContext* context, ID3D11ShaderResourceView* positionSRV)override;
	void PreviewRender(ID3D11DeviceContext* context)override;
	void Reset(const uint& drawCount)override;
	
private:
	void InitBasicSpark();
	 void CreateShaders(ID3D11Device* device)override;
	 void CreateBuffers(ID3D11Device* device)override;
private:
	struct BodyData
	{
		uint Bodies;
		float       *position;
		float       *velocity;
	}bodyData;
	uint readBuffer;
private:
	struct SimulationParametersDesc
	{
		float timestep = 0.0f;
		float softeningSquared = 0.1f;
		uint numParticles = 0;
		uint readOffset = 0;
		uint writeOffset = 0;
		float distance = 20.0f;
		float timer = 0.0f;
		float runningTime = 10.0f;

	}simulateDesc;

	ID3D11Buffer* simulateBuffer;
	Vector4 *particleArray;

	bool bRendered;
	uint drawCount;
private:
	struct WVPDesc
	{
		Matrix WVP;
		float pontSize = 0.1f;
		uint readOffset = 0;
		uint positionIndex = 0;
		float intensity=1.0f;

	}wvpDesc;

	ID3D11Buffer* wvpBuffer;
private:
	float clusterScale = 0.1f;
	float velocityScale = 0.1f;
private :
   ID3D11BlendState * AdditiveBlendState;
   ID3D11DepthStencilState*  NoDepth;
   ID3D11RasterizerState* rsState;

  

};

