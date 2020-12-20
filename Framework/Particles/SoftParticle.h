#pragma once
#include "Renders/Shader.h"
#define MAX_PARTICLES 500

struct PARTICLE_VERTEX
{
	D3DXVECTOR3	 Pos;
	D3DXVECTOR3  Vel;
	float		 Life;
	float		 Size;
};

class SoftParticle 
{
	

	enum PARTICLE_TECHNIQUE
	{
		PT_BILLBOARD_HARD,
		PT_BILLBOARD_ODEPTH,
		PT_BILLBOARD_SOFT,
		PT_BILLBOARD_ODEPTHSOFT,
		PT_VOLUME_HARD,
		PT_VOLUME_SOFT
	};
public:


	// IExecute을(를) 통해 상속됨
	 void Initialize(ID3D11Device* device) ;
	
	 void Update(ID3D11DeviceContext* context) ;
	 void Render(ID3D11DeviceContext* context, ID3D11ShaderResourceView* srv) ;
	
	

public:
	void CreateParticleBuffers(ID3D11Device* pd3dDevice);
	void CreateNoiseVolume(ID3D11Device* pd3dDevice, UINT VolumeSize);
	void SortParticleBuffer(D3DXVECTOR3 vEye, D3DXVECTOR3 vDir);
	void AdvanceParticles(double fTime, float fTimeDelta);
	void UpdateParticleBuffers(ID3D11DeviceContext* context);
private:
	Shader* shader;

	float runningTime = 0.0f;


	PARTICLE_VERTEX*		g_pCPUParticles = NULL;
	DWORD*					g_pCPUParticleIndices = NULL;
	float*					g_pParticleDepthArray = NULL;

	ID3D11Buffer*           g_pParticleVB = NULL;
	ID3D11Buffer*           g_pParticleIB = NULL;
	
	ID3D11Texture3D*        g_pNoiseVolume = NULL;
	ID3D11ShaderResourceView* g_pNoiseVolumeRV = NULL;



	ID3DX11EffectMatrixVariable* g_pmWorldViewProj = NULL;
	ID3DX11EffectMatrixVariable* g_pmWorldView = NULL;
	ID3DX11EffectMatrixVariable* g_pmWorld = NULL;
	ID3DX11EffectMatrixVariable* g_pmInvView = NULL;
	ID3DX11EffectMatrixVariable* g_pmInvProj = NULL;
	ID3DX11EffectScalarVariable* g_pfFadeDistance = NULL;
	ID3DX11EffectScalarVariable* g_pfSizeZScale = NULL;
	ID3DX11EffectVectorVariable* g_pvViewLightDir1 = NULL;
	ID3DX11EffectVectorVariable* g_pvViewLightDir2 = NULL;
	ID3DX11EffectVectorVariable* g_pvWorldLightDir1 = NULL;
	ID3DX11EffectVectorVariable* g_pvWorldLightDir2 = NULL;
	ID3DX11EffectVectorVariable* g_pvEyePt = NULL;
	ID3DX11EffectVectorVariable* g_pvViewDir = NULL;
	ID3DX11EffectVectorVariable* g_pvOctaveOffsets = NULL;
	ID3DX11EffectVectorVariable* g_pvScreenSize = NULL;
	ID3DX11EffectShaderResourceVariable* g_pDiffuseTex = NULL;
	ID3DX11EffectShaderResourceVariable* g_pNormalTex = NULL;
	ID3DX11EffectShaderResourceVariable* g_pColorGradient = NULL;
	ID3DX11EffectShaderResourceVariable* g_pVolumeDiffTex = NULL;
	ID3DX11EffectShaderResourceVariable* g_pVolumeNormTex = NULL;
	ID3DX11EffectShaderResourceVariable* g_pDepthTex = NULL;
	ID3DX11EffectShaderResourceVariable* g_pDepthMSAATex = NULL;

	/*ID3DX11EffectShaderResourceVariable* diffuseMap = NULL;
	ID3DX11EffectShaderResourceVariable* normalMap = NULL;
	ID3DX11EffectShaderResourceVariable* pbrMap = NULL;*/

	INT g_iWidth = 640;
	INT g_iHeight = 480;
	INT g_iSampleCount = 1;



	float g_fFadeDistance = 1.0f;
	float g_fParticleLifeSpan = 5.0f;
	float g_fEmitRate = 0.015f;

	
	float g_fParticleMaxSize = 1.0f;
	float g_fParticleMinSize =1.0f;
	bool  g_bAnimateParticles = true;


	PARTICLE_TECHNIQUE g_ParticleTechnique = PT_BILLBOARD_ODEPTH;
	D3DXVECTOR3 g_vLightDir1 = D3DXVECTOR3(1.705f, 5.557f, -9.380f);
	D3DXVECTOR3 g_vLightDir2 = D3DXVECTOR3(-5.947f, -5.342f, -5.733f);

	Texture* smokevol;
	Texture* gradient;

	ID3D11Texture2D* depthStencilTexture;
	ID3D11DepthStencilView* depthStencilDSV;
	ID3D11ShaderResourceView* depthStencilSRV;
/*
	
	ID3D11Texture2D			 *diffuseResource=nullptr;
	ID3D11ShaderResourceView *diffuseResourceSRV = nullptr;

	ID3D11Texture2D			* normalResource = nullptr;
	ID3D11ShaderResourceView* normalResourceSRV = nullptr;

	ID3D11Texture2D			 *pbrResource = nullptr;
	ID3D11ShaderResourceView *pbrResourceSRV = nullptr;*/
};