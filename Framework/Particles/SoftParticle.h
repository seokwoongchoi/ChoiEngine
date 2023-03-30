#pragma once
#include "Renders/Shader.h"

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
	struct CHAR4
	{
		char x, y, z, w;
	};
public:
	SoftParticle(ID3D11Device* device, uint ID);
	~SoftParticle();

	inline float RPercent()
	{
		float ret = (float)((rand() % 20000) - 10000);
		return ret / 10000.0f;
	}
	float GetDensity(int x, int y, int z, CHAR4* pTexels, UINT VolumeSize);
	void SetNormal(D3DXVECTOR3 Normal, int x, int y, int z, CHAR4* pTexels, UINT VolumeSize);
	void QuickDepthSort(DWORD* indices, float* depths, int lo, int hi);
public:
	int InitBodies(const wstring& path)override;
	void Update(ID3D11DeviceContext* context)override;
	void Render(ID3D11DeviceContext* context, ID3D11ShaderResourceView* positionSRV)override;
	void PreviewRender(ID3D11DeviceContext* context, const Matrix & view, const Matrix & proj)override;
	void Reset(const uint& drawCount)override {}

private:
	void CreateShaders(ID3D11Device* device)override;
	void CreateBuffers(ID3D11Device* device)override;
	
private:
	void CreateParticleBuffers(ID3D11Device* pd3dDevice);
	void CreateNoiseVolume(ID3D11Device* pd3dDevice, uint VolumeSize);
	void SortParticleBuffer(D3DXVECTOR3 vEye, D3DXVECTOR3 vDir);
	void AdvanceParticles(double fTime, float fTimeDelta);
	void UpdateParticleBuffers(ID3D11DeviceContext* context);
	void EmitParticle(PARTICLE_VERTEX* pParticle, uint index);
public:

	void SetPosition(bool setPosition){ bSettedPosition= setPosition;}
	void SetWorld(const Matrix& world){simulateDesc.World = world;}
	const Matrix& GetWorld() { return simulateDesc.World; }
	struct SimulationParametersDesc
	{
	

		uint numParticles = 500;
		float ParticleLifeSpan = 5.0f;
		float EmitRate = 0.015f;
		float FadeDistance = 5.0f;

		float ParticleMaxSize = 1.0f;
		float ParticleMinSize = 1.0f;

		float runningTime = 0.0f;
		float articleVel = 3.0f;

		Vector3 factor = Vector3(1.0f, 1.0f, 1.0f);
		Vector3 dir = Vector3(0.0f, 1.0f, 0.0f);

		Matrix World;

	}simulateDesc;
private:
	double fLastEmitTime = 0;
	UINT iLastParticleEmitted = 0;
	float dir = 0.5f;

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
		Matrix WorldView;
		Vector4 OctaveOffsets[4];
		Vector3 ViewDir;
		float FadeDistance;
		//Vector4 lightDirectional;
	//	float  SizeZScale=1.0f;
	//	float  FadeDistance=1.0f;

		//Vector2 pad;
	}psDesc;
	ID3D11Buffer* CB_PSBuffer;


	Matrix View;
	Matrix Proj;
	bool bSettedPosition;

private:
	
	PARTICLE_VERTEX*		CPUParticles;
	DWORD*					CPUParticleIndices;
	float*					ParticleDepthArray;
	shared_ptr<class InputLayout> inputLayout;
	ID3D11Buffer*           ParticleVB;
	ID3D11Buffer*           ParticleIB ;


	Texture* smokevol;
	Texture* gradient;

	//ID3D11Texture3D*        NoiseVolume = NULL;
	//ID3D11ShaderResourceView* NoiseVolumeRV = NULL;

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
	ID3D11PixelShader*    PreviewPS;
};