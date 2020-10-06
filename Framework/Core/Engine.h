#pragma once

#define MAX_THREAD_COUNT 6

class Engine final
{
#ifdef EDITORMODE
	friend class Editor;
	friend class ActorEditor;
#endif


public:
	explicit Engine();
	~Engine();

	
private:

	
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

public:
	void Load();
	void Load(const uint& index, const wstring& modelName, const uint& actorCount,const ReadMeshType& meshType);
	void LoadParticle(const uint& index, const wstring& path,const ReadParticleType& particleType);
	void PusInstance(const uint & index, const  Matrix& world, const ReadMeshType& meshType);
public:

	void Resize(uint width, uint height);
	void Update(bool bStart);
	void Update();
	void Render();

public:
	void Begin();
	inline void Present(){
		immediateContext->SetPredication(predicate, true);
		swapChain->Present(bVsync == true ? 1 : 0, 0); 
	}

private:
	float ClearColor[4];
	uint staticActorCount;
	uint skeletalActorCount;

protected:
	class SSAO* ssao;
	class SSLR* sslr;
	class HDR* hdr;
protected:
	class Island11* island11;
	class Scattering* sky;
	shared_ptr<Texture> preintegratedFG;
	ID3D11ShaderResourceView* skyIRSRV;
protected:

	class EffectSystem* effects;
	class Animator* animator;
	class PhysicsSystem* physics;
	class ColliderSystem* collider;
	class Renderer* staticRenderer;
	class Renderer* skeletalRenderer;
private:
	class ActorController* actorController;
	Matrix VP;
	ID3D11Buffer* mainViewBuffer;
private:
	void CreateHDRRTV(uint width, uint height);
	ID3D11Texture2D*          HDRTexture;
	ID3D11RenderTargetView*   HDRRTV;
	ID3D11ShaderResourceView* HDRSRV;
private:
	ID3D11Predicate* predicate;
protected:
	ID3D11Device* device;
	ID3D11DeviceContext* immediateContext;
private:
	ID3D11DeviceContext* deferredContext[MAX_THREAD_COUNT];
	ID3D11CommandList* commadList[MAX_THREAD_COUNT];
	IDXGISwapChain* swapChain;
	bool bVsync;

private:
	ID3D11Debug* debugDevice;
	UINT gpuMemorySize;
	wstring gpuDescription;
	UINT numerator;
	UINT denominator;
private:
	ID3D11Texture2D* backBuffer;
	ID3D11DepthStencilView* depthStencilView;
	ID3D11RenderTargetView* renderTargetView;
	D3D11_VIEWPORT viewport;
private:
	void SetGpuInfo(const uint & width, const uint & height);
	void CreateSwapChainAndDevice(HWND Handle, bool IsFullScrenn);
	void CreateBackBuffer(const uint & width, const uint & height);
	void DeleteBackBuffer();
private:
	bool bShowCascadedDebug;

//private:
//	struct CB_FOG
//	{
//		Vector3 fogColor;
//		float StartDepth;
//		Vector3 HighlightColor;
//		float GlobalDensity;
//		Vector3 dir;
//	    float HeightFalloff;
//	}fogDesc;
//
//	ID3D11Buffer* fogBuffer;
};

