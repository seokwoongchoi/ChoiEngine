#pragma once
#define MAX_ACTOR_COUNT 10
#define MAX_THREAD_COUNT 5

class Engine final
{

#ifdef EDITORMODE
	friend class Editor;
#endif

public:
	Engine();
	~Engine();
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	void Load(const uint& index, const wstring& modelName, const ReadMeshType& meshTyp);
	void PusInstance(const uint & index, const Vector3& pos, const Vector3& scale);
public:

	void Resize(uint width, uint height);
	void Update();
	void Render();
	
public:
	void Begin();
	inline void Present(){
		immediateContext->SetPredication(predicate, true);
		swapChain->Present(bVsync == true ? 1 : 0, 0); 
	}
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
	class Renderer* renderer;
private:
	class Camera* mainCamera;
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
	ID3D11CommandList* commadList[MAX_ACTOR_COUNT];
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

	float ClearColor[4];
private:
	uint actorCount;

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

