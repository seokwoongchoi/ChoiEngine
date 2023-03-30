#pragma once

struct ParticleInstance
{
	Vector4 positionAndOpacity;
	Vector4 velocityAndLifetime;
	double birthTime;

	bool IsDead(double fTime) const
	{
		return (fTime - birthTime) > velocityAndLifetime.w;
	}

	float LifeParam(double fTime) const
	{
		return float(fTime - birthTime) / float(velocityAndLifetime.w);
	}
};

struct DepthSortFunctor
{
	DepthSortFunctor(const Vector3& viewDirArg) :
		viewDirection(viewDirArg)
	{
	}

	bool operator()(const ParticleInstance& lhs, const ParticleInstance& rhs) const
	{
		const float lhsDepth = D3DXVec3Dot(&viewDirection, (const Vector3*)&lhs.positionAndOpacity);
		const float rhsDepth = D3DXVec3Dot(&viewDirection, (const Vector3*)&rhs.positionAndOpacity);

		return lhsDepth > rhsDepth;
	}

	Vector3 viewDirection;
};
struct ParticleVertex
{
	FLOAT instanceIndex;
	FLOAT cornerIndex;

	enum { NumLayoutElements = 1 };

	static const D3D11_INPUT_ELEMENT_DESC* GetLayout()
	{
		static const D3D11_INPUT_ELEMENT_DESC theLayout[NumLayoutElements] = {
			// SemanticName  SemanticIndex  Format                        InputSlot    AlignedByteOffset              InputSlotClass               InstanceDataStepRate
			{ "POSITION",    0,             DXGI_FORMAT_R32G32_FLOAT,     0,           D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0                    }
		};

		return theLayout;
	}
};
class Smoke
{
	Smoke(ID3D11Device* device);
	~Smoke();

	void Save(double Time);
	void InitPlumeSim();


    void PlumeSimTick(double Time);
	void SortByDepth();

    void UpdateD3D11Resources(ID3D11DeviceContext* DC);

    void OnD3D11CreateDevice(ID3D11Device* device);
    void OnD3D11DestroyDevice();

    const Vector3& GetBoundsMin() const { return MinCorner; }
    const Vector3& GetBoundsMax() const { return MaxCorner; }

    uint GetNumActiveParticles() const { return NumActiveParticles; }

    void DrawToOpacity(ID3D11DeviceContext* DC);
	void DrawSoftToSceneLowRes(	ID3D11DeviceContext* DC,
									ID3D11ShaderResourceView* DepthStencilSRV,
									float zNear, float zFar
									);

	void DrawToScene(ID3D11DeviceContext* DC);



    void SetMeteredOverdraw(bool b) { bMeteredOverdraw = b; }

	// NB: Effectively measured in 'particle widths'
	void SetSoftParticleFadeDistance(float f) { SoftParticleFadeDistance = f; }
	float GetSoftParticleFadeDistance() const { return SoftParticleFadeDistance; }

	void BindToEffect(ID3D11Device* pd3dDevice);

private:

	void Draw(ID3D11DeviceContext* DC);
	void UpdateBounds(bool& firstUpdate, const D3DXVECTOR4& positionAndScale);
	

    bool bWireframe;
    bool bMeteredOverdraw;

	

 
    ID3D11Texture2D*            InstanceTexture;
    ID3D11ShaderResourceView*   InstanceTextureSRV;

    uint AbsoluteMaxNumActiveParticles;
    uint MaxNumActiveParticles;
    uint NumActiveParticles;
	uint InstanceTextureWH;

    ID3D11Buffer*               ParticleVertexBuffer;
    ID3D11Buffer*               ParticleIndexBuffer;
    ID3D11Buffer*               ParticleIndexBufferTessellated;
    ID3D11InputLayout*          ParticleVertexLayout;

    ID3D11ShaderResourceView*   ParticleTextureSRV;

    float ParticleLifeTime;
    float ParticleScale;
	float SoftParticleFadeDistance;

    float TessellationDensity;
    float MaxTessellation;



    ParticleInstance* ParticlePrevBuffer;
    ParticleInstance* ParticleCurrBuffer;

	bool FirstUpdate;
    double LastUpdateTime;
	float EmissionRate;

 //   ID3DX11EffectScalarVariable* TessellationDensity_EffectVar;
 //   ID3DX11EffectScalarVariable* MaxTessellation_EffectVar;

 //   ID3DX11EffectPass*          pEffectPass_ToScene[NuLightingModes];
	//ID3DX11EffectPass*          pEffectPass_SoftToScene[NuLightingModes];
 //   ID3DX11EffectPass*          pEffectPass_ToSceneWireframeOverride;
 //   ID3DX11EffectPass*          pEffectPass_ToSceneOverdrawMeteringOverride;
 //   ID3DX11EffectPass*          pEffectPass_LowResToSceneOverride;
 //   ID3DX11EffectPass*          pEffectPass_ToOpacity;

 //   ID3DX11EffectShaderResourceVariable* InstanceTexture_EffectVar;
 //   ID3DX11EffectScalarVariable*         ParticleScale_EffectVar;
 //   ID3DX11EffectShaderResourceVariable* ParticleTexture_EffectVar;
 //   ID3DX11EffectMatrixVariable*         mLocalToWorld_EffectVar;
	//ID3DX11EffectVectorVariable*		 ZLinParams_EffectVar;
	//ID3DX11EffectScalarVariable*         SoftParticlesFadeDistance_EffectVar;
	//ID3DX11EffectShaderResourceVariable* SoftParticlesDepthTexture_EffectVar;

    Vector3 MinCorner;
    Vector3 MaxCorner;
};

