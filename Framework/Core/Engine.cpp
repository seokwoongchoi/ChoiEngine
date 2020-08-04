#include "Framework.h"
#include "Engine.h"

#include "Viewer/Freedom.h"

//Deferrd
#include "GBuffer/GBufferData.h"
#include "GBuffer/LightManager.h"
#include "GBuffer/CascadedShadow.h"
//Event
#include "EventSystems/ColliderSystem.h"
#include "EventSystems/Animator.h"
#include "EventSystems/Renderer.h"

//PostEffects
#include "PostEffects/HDR.h"
#include "PostEffects/SSAO.h"
#include "PostEffects/SSLR.h"
//Env
#include "Environment/Island11.h"
#include "Environment/Sky/Scattering.h"
class GBufferData GBuffer;
class LightManager Lighting;
class CascadedShadow  CascadedMatrixSet;


Engine::Engine()
	:HDRTexture(nullptr), HDRRTV(nullptr), HDRSRV(nullptr), bShowCascadedDebug(false),
	viewport{}, predicate(nullptr),device(nullptr),immediateContext(nullptr),swapChain(nullptr),bVsync(false), backBuffer(nullptr), depthStencilView(nullptr), renderTargetView(nullptr),
	island11(nullptr),sky(nullptr), mainCamera(nullptr),mainViewBuffer(nullptr), skeletalRenderer(nullptr), staticRenderer(nullptr),ssao(nullptr),sslr(nullptr),hdr(nullptr),
	ClearColor{ 0.5f, 0.6f, 0.3f, 0.0f }, VP{}, deferredContext{}, commadList{}, numerator(0), denominator(1), staticActorCount(0), skeletalActorCount(0),
	animator(nullptr), collider(nullptr)

{
	uint width = static_cast<uint>(D3D::Width());
	uint height = static_cast<uint>(D3D::Height());
	bVsync = D3D::GetDesc().bVsync;
	{
		
		SetGpuInfo(static_cast<uint>(width), static_cast<uint>(height));

		CreateSwapChainAndDevice(D3D::GetDesc().Handle, D3D::GetDesc().bFullScreen);
		CreateBackBuffer(width, height);
	

		D3D11_FEATURE_DATA_THREADING ThreadingOptions;
		Check(device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &ThreadingOptions, sizeof(D3D11_FEATURE_DATA_THREADING)));
		UINT contextFlags = immediateContext->GetContextFlags();

		for (uint i = 0; i < MAX_THREAD_COUNT; i++)
		{
			Check(device->CreateDeferredContext(contextFlags, &deferredContext[i]));
		}
	}
	

	{
		mainCamera = new Freedom();
		static_cast<Freedom*>(mainCamera)->Speed(30, 0.3f);
		// Allocate constant buffers
		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.ByteWidth = sizeof(Matrix);
		Check(device->CreateBuffer(&bufferDesc, NULL, &mainViewBuffer));
	}
	

	{
		GBuffer.Init(device);
		Lighting.Init(device);
	}
	
	{




		collider = new ColliderSystem(device, "../_Shaders/ComputeShaders/ColliderCS.hlsl", "StaticColliderCS");
		staticRenderer = new Renderer[MAX_ACTOR_COUNT];


		
		animator = new Animator(device);
		skeletalRenderer = new Renderer[MAX_ACTOR_COUNT];
		
	/*	for (uint i = 0; i < 7; i++)
			Load(i, L"VerdeResidence", ReadMeshType::StaticMesh);

		Matrix world;
		for (uint i = 0; i < 7; i++)
		for (uint d = 0; d < 7; d++)
		{
			D3DXMatrixScaling(&world, 0.01f, 0.01f, 0.01f);
			world._41 = d * 15;
			world._42 = 0.0f;
			world._43 = i * 15;
			PusInstance(i, world,ReadMeshType::StaticMesh);
			
		}
		collider->ClearTextureTransforms();*/
	}
	
	
	{
		

		ssao = new SSAO(device, width, height);
		sslr = new SSLR(device, width, height);
		hdr = new HDR(device, width, height);
		CreateHDRRTV(width, height);
	}
	
	
	
	{
		island11 = new Island11(device);
		sky = new Scattering(device);
	}
	{
		
		preintegratedFG = make_shared< Texture>();
		preintegratedFG->Load(device,L"PBR/PreintegratedFG.bmp");
	

		const wstring& temp = L"../../_Textures/Environment/SunsetCube1024.dds";
		//wstring temp = L"../../_Textures/Environment/sky_ocean.dds";
		
		D3DX11CreateShaderResourceViewFromFile
		(
			device, temp.c_str(), NULL, NULL, &skyIRSRV, NULL
		);

	
	
		//SafeRelease(skyIRSRV);
	}

	D3D11_QUERY_DESC queryDesc;
	queryDesc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
	queryDesc.MiscFlags = D3D11_QUERY_MISC_PREDICATEHINT;
	device->CreatePredicate(&queryDesc, &predicate);

	{
		//// Create constant buffers
		//D3D11_BUFFER_DESC cbDesc;
		//ZeroMemory(&cbDesc, sizeof(cbDesc));
		//cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		//cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		//cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		//cbDesc.ByteWidth = sizeof(CB_FOG);
		//Check(device()->CreateBuffer(&cbDesc, NULL, &fogBuffer));

		//// Prepare the fog parameters
		//D3D11_MAPPED_SUBRESOURCE MappedResource;
		//immediateContext->Map(fogBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		//memcpy(MappedResource.pData, &fogDesc, sizeof(fogDesc));

		//fogDesc.fogColor = Vector3(0.1f, 0.1f, 0.1f);
		//fogDesc.HighlightColor = Vector3(0.1f, 0.1f, 0.1f);
		//fogDesc.StartDepth = 0.0f;
		//fogDesc.GlobalDensity = 0.1f;
		//fogDesc.dir = -GlobalData::LightDirection();
		//fogDesc.HeightFalloff = 0.1f;
		//immediateContext->Unmap(fogBuffer, 0);
		//immediateContext->PSSetConstantBuffers(2, 1, &fogBuffer);
	}
	

	
}

Engine::~Engine()
{
	DeleteBackBuffer();

	if (swapChain != NULL)
		swapChain->SetFullscreenState(false, NULL);

	SafeRelease(immediateContext);
	SafeRelease(device);
	SafeRelease(swapChain);
	SafeDelete(mainCamera);
	
}

void Engine::Update()
{
	if (Keyboard::Get()->Down('O'))
	{
		bShowCascadedDebug == true ? bShowCascadedDebug = false : bShowCascadedDebug = true;
	}

	mainCamera->Update();

	D3DXMatrixTranspose(&VP, &GlobalData::GetVP());
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	deferredContext[1]->Map(mainViewBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, VP, sizeof(Matrix));
	deferredContext[1]->Unmap(mainViewBuffer, 0);

	deferredContext[1]->VSSetConstantBuffers(1, 1, &mainViewBuffer);

	for (uint i = 0; i < skeletalActorCount; i++)
	animator->Update(i);
	
	for (uint i = 0; i < staticActorCount; i++)
	{
		collider->FrustumCulling(i);
	}

	
}

void Engine::Render()
{

	
	     auto mainDepthSRV = GBuffer.DepthstencilSRV();
		
		 island11->Update(deferredContext[1]);
		 island11->Caustics(deferredContext[1]);
		

	     
		 Lighting.PrepareNextShadowLight(deferredContext[0]);
		 GBuffer.PrepareForPacking(deferredContext[1]);

		
		 island11->Reflection(deferredContext[2]);

		 //skeletal Model Render
		 {
			 for (uint i = 0; i < 3; i++)
		     {
		    	 animator->BindPipeline(deferredContext[i]);
		    	
		     }

			 for (uint i = 0; i < skeletalActorCount; i++)
			 {
				 //skeletalRenderer[i].Depth_PreRender(deferredContext[0]);
				 skeletalRenderer[i].Render(deferredContext[1]);
				 skeletalRenderer[i].Reflection_PreRender(deferredContext[2]);
			 }

		 }
		
		 //static Model Render
		 {
			 for (uint i = 0; i < 4; i++)
			 {
				 collider->BindPipeline(deferredContext[i]);
				// deferredContext[5]->FinishCommandList(false, &commadList[5]);
			 }
			/* for (uint i = 0; i < 3; i++)
			 {
				 deferredContext[i]->ExecuteCommandList(commadList[5], true);
			 }*/
			 for (uint i = 0; i < staticActorCount; i++)
			 {

				 staticRenderer[i].Depth_PreRender(deferredContext[0]);
				 staticRenderer[i].Render(deferredContext[1]);
				 staticRenderer[i].Reflection_PreRender(deferredContext[2]);
			 }

		 }
		
		
		
		 island11->Terrain(deferredContext[1]);

		 island11->Refraction(deferredContext[1], mainDepthSRV, GBuffer.DiffuseTexture());
		 GBuffer.PrepareForWaterPacking(deferredContext[1]);
		 island11->Water(deferredContext[1]);


		
		 deferredContext[0]->FinishCommandList(false, &commadList[0]);
		 deferredContext[1]->FinishCommandList(false, &commadList[1]);
		 deferredContext[2]->FinishCommandList(false, &commadList[2]);
	
		
		 {
			
    		 deferredContext[3]->RSSetViewports(1, &viewport);
			 deferredContext[3]->OMSetRenderTargets(1, &HDRRTV, GBuffer.DepthstencilDSVReadOnly());
			 deferredContext[3]->ClearDepthStencilView(GBuffer.DepthstencilDSVReadOnly(), D3D11_CLEAR_DEPTH, 1.0, 0);


			 sky->Render(deferredContext[3]);

			 D3DXMatrixTranspose(&VP, &GlobalData::GetVP());
			 D3D11_MAPPED_SUBRESOURCE MappedResource;
			 deferredContext[3]->Map(mainViewBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
			 memcpy(MappedResource.pData, VP, sizeof(Matrix));
			 deferredContext[3]->Unmap(mainViewBuffer, 0);
			
			 deferredContext[3]->VSSetConstantBuffers(1, 1, &mainViewBuffer);

			
			
			 for (uint i = 0; i < staticActorCount; i++)
			 {
				 
				 staticRenderer[i].Forward_Render(deferredContext[3]);
				 
			 }
			 
			 
			
			 deferredContext[3]->FinishCommandList(false, &commadList[3]);
			
		 }
		
		
		 {//Execute packinig& shadow
			 
			
			 immediateContext->ExecuteCommandList(commadList[0], false);
			 SafeRelease(commadList[0]);
			 immediateContext->ExecuteCommandList(commadList[2], false);
			 SafeRelease(commadList[2]);
			 immediateContext->ExecuteCommandList(commadList[1], false);
			 SafeRelease(commadList[1]);
			 
			 
		 }
		

		 {
			 immediateContext->RSSetViewports(1, &viewport);
			 immediateContext->OMSetRenderTargets(1, &HDRRTV, GBuffer.DepthstencilDSVReadOnly());
			 immediateContext->ClearRenderTargetView(HDRRTV, ClearColor);
			 immediateContext->ClearDepthStencilView(GBuffer.DepthstencilDSVReadOnly(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
			
			 {//SSAO
				 ssao->Compute(immediateContext, mainDepthSRV, GBuffer.NormalSRV());
				
			 }
			
			 
			 //Unpack
			 GBuffer.PrepareForUnpack(immediateContext);
			 immediateContext->Begin(predicate);
			 {
				 ID3D11ShaderResourceView* arrSRV[3] = { ssao->GetSSAOSRV(),*preintegratedFG ,skyIRSRV };
				 immediateContext->PSSetShaderResources(5, 3, arrSRV);
				 Lighting.DoLighting(immediateContext);
				 if (bShowCascadedDebug)
					Lighting.DoDebugCascadedShadows(immediateContext);
			 }
			 immediateContext->SetPredication(predicate, true);
			 immediateContext->End(predicate);
			
			

			 
		 }	
		
		
	
		
		 //SSLR Execute & Referation
		 {
			  immediateContext->SetPredication(predicate, false);
			  {	
				
     		     
				  sslr->Render(immediateContext, ssao->GetMiniDepthSRV(), HDRRTV);
				 
				
			  }

			 // immediateContext->SetPredication(nullptr, false);
			
		 }
		 //Sky & water
		 {
		     immediateContext->ExecuteCommandList(commadList[3], false);
			 SafeRelease(commadList[3]);
			 immediateContext->SetPredication(nullptr, false);
		 }
		 //PostEffect
		 {
			 immediateContext->RSSetViewports(1, &viewport);
			 hdr->PostProcessing(immediateContext, HDRSRV, renderTargetView, mainDepthSRV);
			
		 }
		
				
	
	
		

		
	
		
		
		
		
		{
			 //Lighting.DoDebugCascadedShadows(immediateContext);
			// float ClearColor[4] = { 0.6f, 0.62f, 1.0f, 0.0f };
			//D3D::ImmediateContext()->ClearRenderTargetView(old_target, ClearColor);
			//D3D::ImmediateContext()->ClearDepthStencilView(old_depth, D3D11_CLEAR_DEPTH, 1.0, 0); 
			//immediateContext->OMGetDepthStencilState(&pPrevDepthState, &nPrevStencil);
			/*immediateContext->OMSetDepthStencilState(pPrevDepthState, nPrevStencil);
			SafeRelease(pPrevDepthState);*/
		}
}

void Engine::Begin()
{

	immediateContext->RSSetViewports(1, &viewport);
	immediateContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
	immediateContext->ClearRenderTargetView(renderTargetView, ClearColor);
	immediateContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
}

void Engine::CreateHDRRTV(uint width, uint height)
{
	SafeRelease(HDRTexture);
	SafeRelease(HDRRTV);
	SafeRelease(HDRSRV);

	// Create the HDR render target
	D3D11_TEXTURE2D_DESC dtd = {
		width, //UINT Width;
		height, //UINT Height;
		1, //UINT MipLevels;
		1, //UINT ArraySize;
		DXGI_FORMAT_R16G16B16A16_TYPELESS, //DXGI_FORMAT Format;
		1, //DXGI_SAMPLE_DESC SampleDesc;
		0,
		D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
		0,//UINT CPUAccessFlags;
		0//UINT MiscFlags;    
	};
	Check(device->CreateTexture2D(&dtd, NULL, &HDRTexture));


	D3D11_RENDER_TARGET_VIEW_DESC rtsvd =
	{
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		D3D11_RTV_DIMENSION_TEXTURE2D
	};
	Check(device->CreateRenderTargetView(HDRTexture, &rtsvd,&HDRRTV));


	D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd =
	{
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		D3D11_SRV_DIMENSION_TEXTURE2D,
		0,
		0
	};
	dsrvd.Texture2D.MipLevels = 1;
	Check(device->CreateShaderResourceView(HDRTexture, &dsrvd, &HDRSRV));
}

void Engine::SetGpuInfo(const uint & width, const uint & height)
{
	HRESULT hr;

	IDXGIFactory* factory;
	hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&factory);
	Check(hr);

	IDXGIAdapter* adapter;
	hr = factory->EnumAdapters(0, &adapter);
	Check(hr);

	IDXGIOutput* adapterOutput;
	hr = adapter->EnumOutputs(0, &adapterOutput);
	Check(hr);

	uint modeCount=0;
	hr = adapterOutput->GetDisplayModeList
	(
		DXGI_FORMAT_R8G8B8A8_UNORM
		, DXGI_ENUM_MODES_INTERLACED
		, &modeCount
		, NULL
	);
	Check(hr);

	DXGI_MODE_DESC* displayModeList = new DXGI_MODE_DESC[modeCount];
	hr = adapterOutput->GetDisplayModeList
	(
		DXGI_FORMAT_R8G8B8A8_UNORM
		, DXGI_ENUM_MODES_INTERLACED
		, &modeCount
		, displayModeList
	);
	Check(hr);

	for (UINT i = 0; i < modeCount; i++)
	{
		bool isCheck = true;
		isCheck &= displayModeList[i].Width == width;
		isCheck &= displayModeList[i].Height == height;

		if (isCheck == true)
		{
			numerator = displayModeList[i].RefreshRate.Numerator;
			denominator = displayModeList[i].RefreshRate.Denominator;
		}
	}

	DXGI_ADAPTER_DESC adapterDesc;
	hr = adapter->GetDesc(&adapterDesc);
	Check(hr);

	gpuMemorySize = adapterDesc.DedicatedVideoMemory / 1024 / 1024;
	gpuDescription = adapterDesc.Description;

	SafeDeleteArray(displayModeList);


	SafeRelease(adapterOutput);
	SafeRelease(adapter);
	SafeRelease(factory);
}

void Engine::CreateSwapChainAndDevice(HWND Handle, bool IsFullScrenn)
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = 0;
	swapChainDesc.BufferDesc.Height = 0;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	if (bVsync == true)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = Handle;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = !IsFullScrenn;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	swapChainDesc.Flags = 0;


	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	//creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		//D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	HRESULT hr = D3D11CreateDeviceAndSwapChain
	(
		NULL
		, D3D_DRIVER_TYPE_HARDWARE
		, NULL
		, creationFlags
		, featureLevels
		, 1
		, D3D11_SDK_VERSION
		, &swapChainDesc
		, &swapChain
		, &device
		, NULL
		, &immediateContext
	);

	Check(hr);

}

void Engine::CreateBackBuffer(const uint & width, const uint & height)
{
	HRESULT hr;

	//GetBackBuffer And Create RTVdeviceContext
	{
		ID3D11Texture2D* backbufferPointer;
		hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbufferPointer);
		Check(hr);

		hr = device->CreateRenderTargetView(backbufferPointer, NULL, &renderTargetView);
		Check(hr);

		SafeRelease(backbufferPointer);
	}

	//Create Texture - DSV
	{
		D3D11_TEXTURE2D_DESC desc = { 0 };
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		hr = device->CreateTexture2D(&desc, NULL, &backBuffer);
		Check(hr);
	}

	//Create DSV
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;

		hr = device->CreateDepthStencilView(backBuffer, &desc, &depthStencilView);
		Check(hr);

		
	}
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	Begin();
}

void Engine::DeleteBackBuffer()
{
	SafeRelease(depthStencilView);
	SafeRelease(renderTargetView);
	SafeRelease(backBuffer);
}

void Engine::Load(const uint& index, const wstring& modelName, const ReadMeshType& meshType)
{
	BinaryReader* r = new BinaryReader();
	switch (meshType)
	{
	case ReadMeshType::StaticMesh:
	{
		staticRenderer[index].Initiallize(device, index);
		staticRenderer[index].ReadMaterial(modelName);
	
		collider->SetActorCount(index + 1);
		r->Open(L"../_Models/StaticMeshes/" + modelName + L"/" + modelName + L"_Edit" + L".mesh");
		{
			collider->ReadBone(r);
			staticRenderer[index].ReadMesh(r, meshType);
		}
		r->Close();
	
		staticRenderer[index].CreateShader("../_Shaders/StaticMesh.hlsl");
		collider->RegisterRenderer(&staticRenderer[index],index);
	}
	break;
	case ReadMeshType::SkeletalMesh:
	{
		skeletalRenderer[index].Initiallize(device, index);
		skeletalRenderer[index].ReadMaterial(modelName);
	
		animator->SetActorCount(index + 1);
		r->Open(L"../_Models/SkeletalMeshes/" + modelName + L"/" + modelName + L"_Edit" + L".mesh");
		{
			animator->ReadBone(r);
			skeletalRenderer[index].ReadMesh(r, meshType);
		}
		r->Close();

		animator->ReadClip(modelName);

		skeletalRenderer[index].CreateShader("../_Shaders/SkeletalMesh.hlsl");
		animator->RegisterRenderer(&skeletalRenderer[index], index);
	}
	break;
	}

	SafeDelete(r);

}

void Engine::PusInstance(const uint & index, const Matrix & world, const ReadMeshType& meshType)
{
	
	switch (meshType)
	{
	case ReadMeshType::StaticMesh:
		
		
		collider->PushDrawCount(index, world);
		
		staticActorCount = index + 1;
		break;
	case ReadMeshType::SkeletalMesh:

		
		animator->PushDrawCount(index, world);
		
		skeletalActorCount= index +1;
		break;

	}

}

void Engine::Resize(uint width, uint height)
{


	D3D::Width(width);
	D3D::Height(height);

	DeleteBackBuffer();
	{
		HRESULT hr = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
		Check(hr);
	}
	CreateBackBuffer(width, height);
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);


	Lighting.Resize(Vector2(static_cast<float>(width), static_cast<float>(height)));

	GBuffer.Resize(width,height);
	ssao->Resize(width, height);
}