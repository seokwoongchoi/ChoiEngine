#pragma once
#include "Renders/Shader.h"
#define MAX_PARTICLES 500
#include "ParticleSimulation.h"
struct PARTICLE_VERTEX
{
	D3DXVECTOR3	 Pos;
	D3DXVECTOR3  Vel;
	float		 Life;
	float		 Size;
};

class SoftParticle :public ParticleSimulation
{
public:
	SoftParticle(ID3D11Device* device, uint ID);
	~SoftParticle();

	
public:
	void Update(ID3D11DeviceContext* context)override;
	void Render(ID3D11DeviceContext* context, ID3D11ShaderResourceView* positionSRV)override;
	void PreviewRender(ID3D11DeviceContext* context, const Matrix & view, const Matrix & proj)override;
	void Reset(const uint& drawCount)override;

private:
	void CreateShaders(ID3D11Device* device)override;
	void CreateBuffers(ID3D11Device* device)override;
	
private:
	void CreateParticleBuffers(ID3D11Device* pd3dDevice);
	void CreateNoiseVolume(ID3D11Device* pd3dDevice, UINT VolumeSize);
	void SortParticleBuffer(D3DXVECTOR3 vEye, D3DXVECTOR3 vDir);
	void AdvanceParticles(double fTime, float fTimeDelta);
	void UpdateParticleBuffers(ID3D11DeviceContext* context);
private:
	struct SimulationParametersDesc
	{
		INT Width = 640;
		INT Height = 480;
		INT SampleCount = 1;

	
		float ParticleLifeSpan = 5.0f;
		float EmitRate = 0.015f;


		float ParticleMaxSize = 1.0f;
		float ParticleMinSize = 1.0f;

		float runningTime = 0.0f;

	}simulateDesc;
private:

	struct CB_GS
	{
		Matrix WVP;
		Matrix World;
		Matrix InvView;
	}gsDesc;
	ID3D11Buffer* CB_GSBuffer;
	struct CB_PS
	{
		Matrix InvProj;
		//Vector4 lightDirectional;
	//	float  SizeZScale=1.0f;
	//	float  FadeDistance=1.0f;

		//Vector2 pad;
	}psDesc;
	ID3D11Buffer* CB_PSBuffer;


	Matrix View;
	Matrix Proj;


private:
	
	PARTICLE_VERTEX*		CPUParticles;
	DWORD*					CPUParticleIndices;
	float*					ParticleDepthArray;
	shared_ptr<class InputLayout> inputLayout;
	ID3D11Buffer*           ParticleVB;
	ID3D11Buffer*           ParticleIB ;


	Texture* smokevol;
	Texture* gradient;

private:
	ID3D11BlendState*        blendState;
	ID3D11SamplerState*      sampLinearClamp;
	ID3D11SamplerState*      sampVolume;
	ID3D11SamplerState*      sampPoint;
	ID3D11DepthStencilState* depthState;
private:
	ID3D11VertexShader*   VS;
	ID3D11GeometryShader* GS;
	ID3D11PixelShader*    PS;
};