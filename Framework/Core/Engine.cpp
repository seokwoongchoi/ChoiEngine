#include "Framework.h"
#include "Engine.h"



//Deferrd
#include "GBuffer/GBufferData.h"
#include "GBuffer/LightManager.h"
#include "GBuffer/CascadedShadow.h"
//Event

#include "EventSystems/ColliderSystem.h"
#include "EventSystems/Animator.h"
#include "EventSystems/Renderers/Renderer.h"
#include "EventSystems/Renderers/ShadowRenderer.h"

#include "EventSystems/PhysicsSystem.h"
#include "EventSystems/EffectSystem.h"

//PostEffects
#include "PostEffects/HDR.h"
#include "PostEffects/SSAO.h"
#include "PostEffects/SSLR.h"
//Env

#include "Environment/Island11_HLSL.h"
#include "Environment/Sky/Scattering.h"
#include "Environment/Sky/Cloud.h"
#include "EventSystems/ActorController.h"
#include <stack>
class GBufferData GBuffer;
class LightManager Lighting;
class CascadedShadow  CascadedMatrixSet;

#define MAX_THREAD_COUNT 4
ID3D11DeviceContext* deferredContext[MAX_THREAD_COUNT];
ID3D11CommandList* commandList[MAX_THREAD_COUNT];


Engine::Engine()
	:HDRTexture(nullptr), HDRRTV(nullptr), HDRSRV(nullptr), bShowCascadedDebug(false),
	viewport{}, predicate(nullptr),device(nullptr),immediateContext(nullptr),swapChain(nullptr),bVsync(false), backBuffer(nullptr), depthStencilView(nullptr), renderTargetView(nullptr),
	sky(nullptr),mainViewBuffer(nullptr), skeletalRenderer(nullptr), skeletalShadowRenderer(nullptr),
	staticRenderer(nullptr), staticShadowRenderer(nullptr),	ssao(nullptr),sslr(nullptr),hdr(nullptr),
	ClearColor{ 0.5f, 0.6f, 0.3f, 0.0f },  numerator(0), denominator(1), staticActorCount(0), skeletalActorCount(0),
	animator(nullptr), collider(nullptr),effects(nullptr),island11(nullptr)

{  


	
	unsigned char option0 = 1 << 0;//0000 0001
	unsigned char option1 = 1 << 1;//0000 0010
	unsigned char option2 = 'g';// << 2;//0000 0100
	unsigned char option3 = 1 << 3;//0000 1000


	
	uint check = static_cast<uint>(option0);
	uint check1 = static_cast<uint>(option1);
	uint check2 = static_cast<uint>(option2);
	uint check3 = static_cast<uint>(option3);


	
	
	
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
		


		// Allocate constant buffers
		D3D11_BUFFER_DESC bufferDesc;
		ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.ByteWidth = sizeof(MainViewData);
		Check(device->CreateBuffer(&bufferDesc, NULL, &mainViewBuffer));
	}
	

	{
		GBuffer.Init(device,width,height);
		Lighting.Init(device);
		CascadedMatrixSet.CreateCascadedShadowBuffers(device, Vector2(static_cast<float>(width), static_cast<float>(height)));
	}
	
	//EventSystems
	{
		
		
		effects = new EffectSystem(device);
		animator = new Animator(device);
		physics = new PhysicsSystem(device, animator, effects);
		effects->RegisterPhysics(physics);
		actorController = new ActorController(animator);

		collider = new ColliderSystem(device, "../_Shaders/ComputeShaders/ColliderCS.hlsl", "StaticColliderCS");
		
		staticRenderer = new Renderer[5];
		staticShadowRenderer = new ShadowRenderer[5];
		skeletalRenderer = new Renderer[MAX_SKELETAL_ACTOR_COUNT];
		skeletalShadowRenderer = new ShadowRenderer[MAX_SKELETAL_ACTOR_COUNT];

	
	/*	for (uint i = 0; i < 7; i++)
			Load(i, L"VerdeResidence", i+1,ReadMeshType::StaticMesh);

		Matrix world;
		for (uint i = 0; i < 7; i++)
		for (uint d = 0; d < 7; d++)
		{
			D3DXMatrixScaling(&world, 0.01f, 0.01f, 0.01f);
			world._41 = d * 15;
			world._42 = 0.0f;
			world._43 = i * 15;
			PusInstance(i, world,ReadMeshType::StaticMesh);
			
		}*/
		//collider->ClearTextureTransforms();
	}
	
	//Post Effects
	{
		

		ssao = new SSAO(device, width, height);
		sslr = new SSLR(device, width, height);
		hdr = new HDR(device, width, height);
		CreateHDRRTV(width, height);
	}
	
	
	
	{
	
		island11 = new Island11(device,animator->QuadTree());
		
		sky = new Scattering(device);
		cloud = new Cloud(device);
	}
	{
		
		preintegratedFG = make_shared< Texture>();
		preintegratedFG->Load(device,L"PBR/PreintegratedFG.bmp", nullptr);
	

		const wstring& temp = L"../_Textures/Environment/SunsetCube1024.dds";
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

	
		// Create constant buffers
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.ByteWidth = sizeof(CB_FOG);
		Check(device->CreateBuffer(&cbDesc, NULL, &fogBuffer));



	
	
	
}

Engine::~Engine()
{
	DeleteBackBuffer();

	if (swapChain != NULL)
		swapChain->SetFullscreenState(false, NULL);

	SafeRelease(immediateContext);
	SafeRelease(device);
	SafeRelease(swapChain);
	
	
}




void Engine::Update(bool bStart)
{
	physics->Stop();
	if (Keyboard::Get()->Down('C'))
	{
		bShowCascadedDebug == true ? bShowCascadedDebug = false : bShowCascadedDebug = true;
		
	}
	
	{
		for (uint i = 0; i < staticActorCount; i++)
		{
			drawCount[skeletalActorCount + i] = collider->FrustumCulling(i);
		}
		collider->UpdateInstTransformSRV(immediateContext);
	}
	
	{
		//if (bStart)
		
		
		animator->Update(bStart);

	
		
		for (uint i = 0; i < skeletalActorCount; i++)
		{
			drawCount[i] = animator->AnimationUpdate(i);
		}
		actorController->Update();

		for (uint i = 0; i < skeletalActorCount; i++)
		animator->UpdateNextAnim(i);

		animator->Compute(immediateContext, physics->UAV());
		
		if (bStart)
		{
			//Thread::Get()->AddTask([&]() {
			
			physics->Compute(immediateContext, effects->UAV(), animator->TotalCount(), collider->TotalCount());
			
			//});
		}
	     

		for (uint i = 0; i < skeletalActorCount; i++)
			animator->UpdateNextAnim(i);

		animator->Compute(immediateContext, physics->UAV());

	}
	
	CascadedMatrixSet.Update();
	GBuffer.Update();
	Lighting.Update();
	GlobalData::GetVP(&mainViewData.VP);
	D3DXMatrixTranspose(&mainViewData.VP, &mainViewData.VP);
	//mainViewData.EyePos = Vector4(GlobalData::Position(), 1.0f);
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	immediateContext->Map(mainViewBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &mainViewData, sizeof(MainViewData));
	immediateContext->Unmap(mainViewBuffer, 0);
}


void Engine::Update()
{

	physics->Stop();
	

	{
		for (uint i = 0; i < staticActorCount; i++)
		{
			drawCount[skeletalActorCount + i] = collider->FrustumCulling(i);
		}
		collider->UpdateInstTransformSRV(immediateContext);
	}

	{
		
		animator->Update();


		for (uint i = 0; i < skeletalActorCount; i++)
		{
			drawCount[i] = animator->AnimationUpdate(i);
		}

		animator->Compute(immediateContext, physics->UAV());

		
		physics->Compute(immediateContext, effects->UAV(), animator->TotalCount(), collider->TotalCount());
		actorController->Update();
	

	}

	CascadedMatrixSet.Update();
	GBuffer.Update();
	Lighting.Update();
	GlobalData::GetVP(&mainViewData.VP);
	D3DXMatrixTranspose(&mainViewData.VP, &mainViewData.VP);
	//mainViewData.EyePos = Vector4(GlobalData::Position(), 1.0f);
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	immediateContext->Map(mainViewBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &mainViewData, sizeof(MainViewData));
	immediateContext->Unmap(mainViewBuffer, 0);

}

struct ShadowDatas
{
	uint shadowSkeletalActorCount ;
	uint shadowStaticActorCount ;

	vector<uint>drawCount;
	

	ID3D11ShaderResourceView* staticInstBuffer; 
	ID3D11ShaderResourceView* staticBoneSRV;

	ID3D11ShaderResourceView* skeletalInstBuffer;
	ID3D11ShaderResourceView* skeletalBoneSRV;

	ShadowRenderer* skeletalShadowRendererThread;
	ShadowRenderer* staticShadowRendererThread;
}shadowDatas;

struct DrawDatas
{
	uint SkeletalActorCount;
	uint StaticActorCount;

	vector<uint>drawCount;


	ID3D11ShaderResourceView* staticInstBuffer;
	ID3D11ShaderResourceView* staticBoneSRV;

	ID3D11ShaderResourceView* skeletalInstBuffer;
	ID3D11ShaderResourceView* skeletalBoneSRV;

	Renderer* skeletalRendererThread;
	Renderer* staticRendererThread;

	Island11* island11Thread;
	Scattering* skyThread;
	Cloud* cloudThread;

	ID3D11Buffer* viewBuffer;
}drawDatas;


void Engine::Render()
{
	{
		
		shadowDatas.shadowSkeletalActorCount = skeletalActorCount;
		shadowDatas.shadowStaticActorCount = staticActorCount;
		shadowDatas.drawCount = drawCount;
		//memcpy(shadowDatas.drawCount.data(), drawCount.data(), sizeof(uint)*drawCount.size());
		shadowDatas.staticInstBuffer = collider->GetShadowInstBufferSRV();
		shadowDatas.staticBoneSRV = collider->SRV();
		shadowDatas.skeletalInstBuffer = animator->GetInstBufferSRV();
		shadowDatas.skeletalBoneSRV = animator->SRV();
		shadowDatas.skeletalShadowRendererThread = skeletalShadowRenderer;
		shadowDatas.staticShadowRendererThread = staticShadowRenderer;
		//INT64 currentTime;
		//QueryPerformanceCounter((LARGE_INTEGER *)&currentTime);
		//if (chrono::_Is_even<INT64>(currentTime))
		//Thread::Get()->AddTask([]() {
		
			{
				CascadedMatrixSet.CascadedShadowsGen(deferredContext[0]);
				ID3D11ShaderResourceView* srvArray[2] = { shadowDatas.skeletalInstBuffer,shadowDatas.skeletalBoneSRV };
				deferredContext[0]->VSSetShaderResources(0, 2, srvArray);
				for (uint i = 0; i < shadowDatas.shadowSkeletalActorCount; i++)
				{
					uint prevCount = 0;
					if (i > 0)
					{
						for (uint p = 0; p < i; p++)
						{
							prevCount += shadowDatas.drawCount[p];
						}
					}
					shadowDatas.skeletalShadowRendererThread[i].Depth_PreRender(deferredContext[0], shadowDatas.drawCount[i], prevCount);
				}
				srvArray[0] = { shadowDatas.staticInstBuffer };
				srvArray[1] = { shadowDatas.staticBoneSRV };
				deferredContext[0]->VSSetShaderResources(0, 2, srvArray);
				for (uint i = 0; i < shadowDatas.shadowStaticActorCount; i++)
				{
					uint prevCount = 0;
					if (i > 0)
					{
						for (uint p = 0; p < i; p++)
						{
							prevCount += shadowDatas.staticShadowRendererThread[p].DrawCount();
						}
					}
					shadowDatas.staticShadowRendererThread[i].Depth_PreRender(deferredContext[0], prevCount);
				}

			}
			
			deferredContext[0]->FinishCommandList(false, &commandList[0]);
		
	//	});
	}

	drawDatas.SkeletalActorCount = skeletalActorCount;
	drawDatas.StaticActorCount = staticActorCount;
	drawDatas.drawCount = drawCount;
	drawDatas.staticInstBuffer = collider->GetInstBufferSRV();
	drawDatas.staticBoneSRV = collider->SRV();
	drawDatas.skeletalInstBuffer = animator->GetInstBufferSRV();
	drawDatas.skeletalBoneSRV = animator->SRV();
	drawDatas.skeletalRendererThread = skeletalRenderer;
	drawDatas.staticRendererThread = staticRenderer;
	drawDatas.island11Thread = island11;
	drawDatas.skyThread = sky;
	drawDatas.cloudThread = cloud;
	drawDatas.viewBuffer = mainViewBuffer;

	
	//Thread::Get()->AddTask([]() {
		drawDatas.island11Thread->Update(deferredContext[1]);
		drawDatas.island11Thread->Caustics(deferredContext[1]);
		GBuffer.PrepareForPacking(deferredContext[1]);
		deferredContext[1]->VSSetConstantBuffers(1, 1, &drawDatas.viewBuffer);
		{
			ID3D11DepthStencilState* PrevDepthState;
			UINT PrevStencil;
			deferredContext[1]->OMGetDepthStencilState(&PrevDepthState, &PrevStencil);

			drawDatas.skyThread->Render(deferredContext[1]);
			deferredContext[1]->OMSetDepthStencilState(PrevDepthState, PrevStencil);
			drawDatas.cloudThread->Render(deferredContext[1]);
			drawDatas.island11Thread->Reflection(deferredContext[2]);
		}

		
		ID3D11ShaderResourceView* srvArray[2] = { drawDatas.skeletalInstBuffer,drawDatas.skeletalBoneSRV };
		deferredContext[1]->VSSetShaderResources(0, 2, srvArray);
		deferredContext[2]->VSSetShaderResources(0, 2, srvArray);
		for (uint i = 0; i < drawDatas.SkeletalActorCount; i++)
		{
			uint prevCount = 0;
			if (i > 0)
			{
				for (uint p = 0; p < i; p++)
					prevCount += drawDatas.drawCount[p];
			}
			drawDatas.skeletalRendererThread[i].Render(deferredContext[1], drawDatas.drawCount[i], prevCount);
			drawDatas.skeletalRendererThread[i].Reflection_PreRender(deferredContext[2], drawDatas.drawCount[i], prevCount);
		}
		srvArray[0] = { drawDatas.staticInstBuffer };
		srvArray[1] = { drawDatas.staticBoneSRV };
		deferredContext[1]->VSSetShaderResources(0, 2, srvArray);
		deferredContext[2]->VSSetShaderResources(0, 2, srvArray);
		for (uint i = 0; i < drawDatas.StaticActorCount; i++)
		{
			uint prevCount = 0;
			if (i > 0)
			{
				for (uint p = 0; p < i; p++)
					prevCount += drawDatas.drawCount[drawDatas.SkeletalActorCount + p];
			}
			drawDatas.staticRendererThread[i].Render(deferredContext[1], drawDatas.drawCount[(drawDatas.SkeletalActorCount + i)], prevCount);
			drawDatas.staticRendererThread[i].Reflection_PreRender(deferredContext[2], drawDatas.drawCount[(drawDatas.SkeletalActorCount + i)], prevCount);
		}
		drawDatas.island11Thread->Terrain(deferredContext[1]);
		drawDatas.island11Thread->Refraction(deferredContext[1], GBuffer.DepthstencilSRV(), GBuffer.DiffuseTexture());
		GBuffer.PrepareForWaterPacking(deferredContext[1]);
		drawDatas.island11Thread->Water(deferredContext[1]);

		

		deferredContext[1]->FinishCommandList(false, &commandList[1]);
		deferredContext[2]->FinishCommandList(false, &commandList[2]);
		
	//});
	immediateContext->ExecuteCommandList(commandList[0], false);
	SafeRelease(commandList[0]);
	immediateContext->ExecuteCommandList(commandList[2], false);
	SafeRelease(commandList[2]);
	immediateContext->ExecuteCommandList(commandList[1], false);
	SafeRelease(commandList[1]);
	
	ssao->Compute(immediateContext, GBuffer.DepthstencilSRV(), GBuffer.NormalSRV());

	
		{
			immediateContext->ClearRenderTargetView(HDRRTV, ClearColor);
			immediateContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1, 0);

		
			//Unpack
			immediateContext->RSSetViewports(1, &viewport);
			immediateContext->OMSetRenderTargets(1, &HDRRTV, GBuffer.DepthstencilDSVReadOnly());
			GBuffer.PrepareForUnpack(immediateContext);

			{
				
				
				ID3D11ShaderResourceView* arrSRV[8] = { GBuffer.DepthstencilSRV(), GBuffer.DiffuseSRV(), GBuffer.NormalSRV() ,GBuffer.SpecularSRV(),
					CascadedMatrixSet.CascadedDepthStencilSRV, ssao->GetSSAOSRV(),*preintegratedFG ,skyIRSRV };
				immediateContext->PSSetShaderResources(0, 8, arrSRV);



				{//Fog

					fogDesc.dir = -GlobalData::LightDirection();

					D3D11_MAPPED_SUBRESOURCE MappedResource;
					immediateContext->Map(fogBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
					memcpy(MappedResource.pData, &fogDesc, sizeof(fogDesc));
					immediateContext->Unmap(fogBuffer, 0);
					immediateContext->PSSetConstantBuffers(2, 1, &fogBuffer);
				}


				immediateContext->Begin(predicate);
				{
					
					Lighting.DoLighting(immediateContext);
					//Lighting.DoDebugLightVolume(immediateContext);
					if (bShowCascadedDebug)
						Lighting.DoDebugCascadedShadows(immediateContext, GBuffer.DepthstencilSRV());
				}
				immediateContext->SetPredication(predicate, true);
				immediateContext->End(predicate);

			
				ZeroMemory(arrSRV, sizeof(arrSRV));
				immediateContext->PSSetShaderResources(0, 8, arrSRV);
			}

		}
			
			 
	
		
		
		 //SSLR Execute & Referation
		 { 
			
		    immediateContext->SetPredication(predicate, false);
			
			deferredContext[3]->RSSetViewports(1, &viewport);
			deferredContext[3]->OMSetRenderTargets(1, &HDRRTV, GBuffer.DepthstencilDSVReadOnly()); 
			//effects->Render(deferredContext[3]);
			sslr->Render(deferredContext[3], ssao->GetMiniDepthSRV(), HDRRTV);
			deferredContext[3]->FinishCommandList(false, &commandList[3]);
			immediateContext->ExecuteCommandList(commandList[3], false);
			SafeRelease(commandList[3]);
		 }
	
		
		
		 //Sky & water
		 {
			 immediateContext->RSSetViewports(1, &viewport);
			 immediateContext->OMSetRenderTargets(1, &HDRRTV, GBuffer.DepthstencilDSVReadOnly());
				
			 {
				 collider->BindPipeline(immediateContext);
				 immediateContext->VSSetConstantBuffers(1, 1, &mainViewBuffer);

				 for (uint i = 0; i < staticActorCount; i++)
				 {
					 uint prevCount = 0;
					 if (i > 0)
					 {
						 for (uint p = 0; p < i; p++)
							 prevCount += drawCount[skeletalActorCount + p];
					 }
					 staticRenderer[i].Forward_Render(immediateContext, drawCount[skeletalActorCount + i], prevCount);
				 }
				 ID3D11ShaderResourceView* srvArray[2] = { nullptr,nullptr };
				 immediateContext->VSSetShaderResources(0, 2, srvArray);
				 ID3D11Buffer* nullBuffer = nullptr;
				 immediateContext->VSSetConstantBuffers(1, 1, &nullBuffer);

			 }
			
			 effects->Render(immediateContext,GBuffer.DepthstencilSRV());
			
			 //effects->Swap();
		 }


		 //PostEffect
		 {
		
			 hdr->PostProcessing(immediateContext, HDRSRV, renderTargetView, GBuffer.DepthstencilSRV());
			 immediateContext->OMSetRenderTargets(1, &renderTargetView, GBuffer.DepthstencilDSVReadOnly());
		 }
		

		 
		   immediateContext->SetPredication(nullptr, false);
		
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

void Engine::RenderEditor1()
{
	{

		shadowDatas.shadowSkeletalActorCount = skeletalActorCount;
		shadowDatas.shadowStaticActorCount = staticActorCount;
		shadowDatas.drawCount = drawCount;
		//memcpy(shadowDatas.drawCount.data(), drawCount.data(), sizeof(uint)*drawCount.size());
		shadowDatas.staticInstBuffer = collider->GetShadowInstBufferSRV();
		shadowDatas.staticBoneSRV = collider->SRV();
		shadowDatas.skeletalInstBuffer = animator->GetInstBufferSRV();
		shadowDatas.skeletalBoneSRV = animator->SRV();
		shadowDatas.skeletalShadowRendererThread = skeletalShadowRenderer;
		shadowDatas.staticShadowRendererThread = staticShadowRenderer;
		//INT64 currentTime;
		//QueryPerformanceCounter((LARGE_INTEGER *)&currentTime);
		//if (chrono::_Is_even<INT64>(currentTime))
		//Thread::Get()->AddTask([]() {

			{
				CascadedMatrixSet.CascadedShadowsGen(deferredContext[0]);
				ID3D11ShaderResourceView* srvArray[2] = { shadowDatas.skeletalInstBuffer,shadowDatas.skeletalBoneSRV };
				deferredContext[0]->VSSetShaderResources(0, 2, srvArray);
				for (uint i = 0; i < shadowDatas.shadowSkeletalActorCount; i++)
				{
					uint prevCount = 0;
					if (i > 0)
					{
						for (uint p = 0; p < i; p++)
						{
							prevCount += shadowDatas.drawCount[p];
						}
					}
					shadowDatas.skeletalShadowRendererThread[i].Depth_PreRender(deferredContext[0], shadowDatas.drawCount[i], prevCount);
				}
				srvArray[0] = { shadowDatas.staticInstBuffer };
				srvArray[1] = { shadowDatas.staticBoneSRV };
				deferredContext[0]->VSSetShaderResources(0, 2, srvArray);
				for (uint i = 0; i < shadowDatas.shadowStaticActorCount; i++)
				{
					uint prevCount = 0;
					if (i > 0)
					{
						for (uint p = 0; p < i; p++)
						{
							prevCount += shadowDatas.staticShadowRendererThread[p].DrawCount();
						}
					}
					shadowDatas.staticShadowRendererThread[i].Depth_PreRender(deferredContext[0], prevCount);
				}

			}

			deferredContext[0]->FinishCommandList(false, &commandList[0]);

		//});
	}

	drawDatas.SkeletalActorCount = skeletalActorCount;
	drawDatas.StaticActorCount = staticActorCount;
	drawDatas.drawCount = drawCount;
	drawDatas.staticInstBuffer = collider->GetInstBufferSRV();
	drawDatas.staticBoneSRV = collider->SRV();
	drawDatas.skeletalInstBuffer = animator->GetInstBufferSRV();
	drawDatas.skeletalBoneSRV = animator->SRV();
	drawDatas.skeletalRendererThread = skeletalRenderer;
	drawDatas.staticRendererThread = staticRenderer;
	drawDatas.island11Thread = island11;
	drawDatas.skyThread = sky;
	drawDatas.cloudThread = cloud;
	drawDatas.viewBuffer = mainViewBuffer;


	//Thread::Get()->AddTask([]() {
	drawDatas.island11Thread->Update(deferredContext[1]);
	drawDatas.island11Thread->Caustics(deferredContext[1]);
	GBuffer.PrepareForPacking(deferredContext[1]);
	deferredContext[1]->VSSetConstantBuffers(1, 1, &drawDatas.viewBuffer);
	{
		ID3D11DepthStencilState* PrevDepthState;
		UINT PrevStencil;
		deferredContext[1]->OMGetDepthStencilState(&PrevDepthState, &PrevStencil);

		drawDatas.skyThread->Render(deferredContext[1]);
		deferredContext[1]->OMSetDepthStencilState(PrevDepthState, PrevStencil);
		drawDatas.cloudThread->Render(deferredContext[1]);
		drawDatas.island11Thread->Reflection(deferredContext[2]);
	}


	ID3D11ShaderResourceView* srvArray[2] = { drawDatas.skeletalInstBuffer,drawDatas.skeletalBoneSRV };
	deferredContext[1]->VSSetShaderResources(0, 2, srvArray);
	deferredContext[2]->VSSetShaderResources(0, 2, srvArray);
	for (uint i = 0; i < drawDatas.SkeletalActorCount; i++)
	{
		uint prevCount = 0;
		if (i > 0)
		{
			for (uint p = 0; p < i; p++)
				prevCount += drawDatas.drawCount[p];
		}
		drawDatas.skeletalRendererThread[i].Render(deferredContext[1], drawDatas.drawCount[i], prevCount);
		drawDatas.skeletalRendererThread[i].Reflection_PreRender(deferredContext[2], drawDatas.drawCount[i], prevCount);
	}
	srvArray[0] = { drawDatas.staticInstBuffer };
	srvArray[1] = { drawDatas.staticBoneSRV };
	deferredContext[1]->VSSetShaderResources(0, 2, srvArray);
	deferredContext[2]->VSSetShaderResources(0, 2, srvArray);
	for (uint i = 0; i < drawDatas.StaticActorCount; i++)
	{
		uint prevCount = 0;
		if (i > 0)
		{
			for (uint p = 0; p < i; p++)
				prevCount += drawDatas.drawCount[drawDatas.SkeletalActorCount + p];
		}
		drawDatas.staticRendererThread[i].Render(deferredContext[1], drawDatas.drawCount[(drawDatas.SkeletalActorCount + i)], prevCount);
		drawDatas.staticRendererThread[i].Reflection_PreRender(deferredContext[2], drawDatas.drawCount[(drawDatas.SkeletalActorCount + i)], prevCount);
	}
	drawDatas.island11Thread->Terrain(deferredContext[1]);
	drawDatas.island11Thread->Refraction(deferredContext[1], GBuffer.DepthstencilSRV(), GBuffer.DiffuseTexture());
	GBuffer.PrepareForWaterPacking(deferredContext[1]);
	drawDatas.island11Thread->Water(deferredContext[1]);



	deferredContext[1]->FinishCommandList(false, &commandList[1]);
	deferredContext[2]->FinishCommandList(false, &commandList[2]);

	//});
	immediateContext->ExecuteCommandList(commandList[0], false);
	SafeRelease(commandList[0]);
	immediateContext->ExecuteCommandList(commandList[2], false);
	SafeRelease(commandList[2]);
	immediateContext->ExecuteCommandList(commandList[1], false);
	SafeRelease(commandList[1]);

	ssao->Compute(immediateContext, GBuffer.DepthstencilSRV(), GBuffer.NormalSRV());
	GBuffer.PostRender(immediateContext);

	{
		immediateContext->ClearRenderTargetView(HDRRTV, ClearColor);
		immediateContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);


		//Unpack
		immediateContext->RSSetViewports(1, &viewport);
		immediateContext->OMSetRenderTargets(1, &HDRRTV, GBuffer.DepthstencilDSVReadOnly());
		GBuffer.PrepareForUnpack(immediateContext);

		{


			ID3D11ShaderResourceView* arrSRV[8] = { GBuffer.DepthstencilSRV(), GBuffer.DiffuseSRV(), GBuffer.NormalSRV() ,GBuffer.SpecularSRV(),
				CascadedMatrixSet.CascadedDepthStencilSRV, ssao->GetSSAOSRV(),nullptr ,nullptr };
			immediateContext->PSSetShaderResources(0, 8, arrSRV);



			{//Fog

				fogDesc.dir = -GlobalData::LightDirection();

				D3D11_MAPPED_SUBRESOURCE MappedResource;
				immediateContext->Map(fogBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
				memcpy(MappedResource.pData, &fogDesc, sizeof(fogDesc));
				immediateContext->Unmap(fogBuffer, 0);
				immediateContext->PSSetConstantBuffers(2, 1, &fogBuffer);
			}


			//immediateContext->Begin(predicate);
			{

				Lighting.DoLighting(immediateContext);
				//Lighting.DoDebugLightVolume(immediateContext);
				if (bShowCascadedDebug)
					Lighting.DoDebugCascadedShadows(immediateContext, GBuffer.DepthstencilSRV());
			}

			ZeroMemory(arrSRV, sizeof(arrSRV));
			immediateContext->PSSetShaderResources(0, 8, arrSRV);
		}

	}


}

void Engine::RenderEditor2()
{
//	immediateContext->SetPredication(predicate, true);
	


		//SSLR Execute & Referation
		 

		 //immediateContext->SetPredication(predicate, false);
		// immediateContext->End(predicate);

		
		 deferredContext[3]->RSSetViewports(1, &viewport);
		 deferredContext[3]->OMSetRenderTargets(1, &HDRRTV, GBuffer.DepthstencilDSVReadOnly());
		
		 sslr->Render(deferredContext[3], ssao->GetMiniDepthSRV(), HDRRTV);
		 deferredContext[3]->FinishCommandList(false, &commandList[3]);
		 immediateContext->ExecuteCommandList(commandList[3], false);
		 SafeRelease(commandList[3]);
		 



		 //Sky & water
		 
			 immediateContext->RSSetViewports(1, &viewport);
			 immediateContext->OMSetRenderTargets(1, &HDRRTV, GBuffer.DepthstencilDSVReadOnly());

			 {
				 collider->BindPipeline(immediateContext);
				 immediateContext->VSSetConstantBuffers(1, 1, &mainViewBuffer);

				 for (uint i = 0; i < staticActorCount; i++)
				 {
					 uint prevCount = 0;
					 if (i > 0)
					 {
						 for (uint p = 0; p < i; p++)
							 prevCount += drawCount[skeletalActorCount + p];
					 }
					 staticRenderer[i].Forward_Render(immediateContext, drawCount[skeletalActorCount + i], prevCount);
				 }
				 ID3D11ShaderResourceView* srvArray[2] = { nullptr,nullptr };
				 immediateContext->VSSetShaderResources(0, 2, srvArray);
				 ID3D11Buffer* nullBuffer = nullptr;
				 immediateContext->VSSetConstantBuffers(1, 1, &nullBuffer);

			 }

			 effects->Render(immediateContext, GBuffer.DepthstencilSRV());

			 //effects->Swap();
		 


		 //PostEffect
		 {

			 hdr->PostProcessing(immediateContext, HDRSRV, renderTargetView, GBuffer.DepthstencilSRV());
			 immediateContext->OMSetRenderTargets(1, &renderTargetView, GBuffer.DepthstencilDSVReadOnly());
		 }



		// immediateContext->SetPredication(nullptr, false);

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
	immediateContext->ClearRenderTargetView(renderTargetView, ClearColor);
	immediateContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

	immediateContext->RSSetViewports(1, &viewport);
	immediateContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
	
}

void Engine::CreateHDRRTV(uint width, uint height)
{
	//DXGI_FORMAT_R32G32B32A32_FLOAT
	//DXGI_FORMAT_R32G32B32A32_FLOAT
	DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	
	SafeRelease(HDRTexture);
	SafeRelease(HDRRTV);
	SafeRelease(HDRSRV);

	// Create the HDR render target
	D3D11_TEXTURE2D_DESC dtd = {
		width, //UINT Width;
		height, //UINT Height;
		1, //UINT MipLevels;
		1, //UINT ArraySize;
		DXGI_FORMAT_R32G32B32A32_TYPELESS, //DXGI_FORMAT Format;
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
		format,
		D3D11_RTV_DIMENSION_TEXTURE2D
	};
	Check(device->CreateRenderTargetView(HDRTexture, &rtsvd, &HDRRTV));


	D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd =
	{
		format,
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

	gpuMemorySize = static_cast<uint>(adapterDesc.DedicatedVideoMemory) / 1024 / 1024;
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
	
	UINT m4xMassQuality;
	device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMassQuality);
	assert(m4xMassQuality > 0);
	int a = 0;

}

void Engine::CreateBackBuffer(const uint & width, const uint & height)
{
	HRESULT hr;

	SafeRelease(renderTargetView);
	

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

void Engine::Load()
{
	BinaryReader* r = new BinaryReader();

	auto fileName = L"../_Levels/Level6.Level";
	r->Open(fileName);

	uint count = r->UInt();
	if (count > 0)
	{
		for (uint i = 0; i < count; i++)
		{
			uint actorIndex = r->UInt();
			uint actorCount = r->UInt();
			actorCount = actorIndex+1;
			uint tempmeshType = r->UInt();
			string tempModelName = r->String();
			wstring tempwString = String::ToWString(tempModelName);
			ReadMeshType meshType = tempmeshType > 1 ? ReadMeshType::SkeletalMesh : ReadMeshType::StaticMesh;
			Load(actorIndex, tempwString, actorCount, meshType);

			uint drawCount = r->UInt();

			if (drawCount > 0)
				switch (tempmeshType)
				{
				case 1:
				{
					if (actorIndex == 0)
					{
						staticShadowRenderer[actorIndex].SetMaintainShadow(true);
					}
					for (uint i = 0; i < drawCount; i++)
					{
						Matrix temp = r->Matrix();
						collider->PushDrawCount(actorIndex, temp,true);
						staticShadowRenderer[actorIndex].PushInstance();
					}
				}
					
					break;
				case 2:
					for (uint i = 0; i < drawCount; i++)
					{
						Matrix temp = r->Matrix();
						animator->PushDrawCount(actorIndex, temp);
						skeletalShadowRenderer[actorIndex].PushInstance();
					}
					break;

				}
			
		}
	}
	uint particleCount = r->UInt();
	if (particleCount > 0)
	{
		for (uint i = 0; i < particleCount; i++)
		{
			uint type = r->UInt();
			auto path = r->String();
			switch (type)
			{
			case 1:
			{
				
				LoadParticle(i, String::ToWString(path), ReadParticleType::Spark);
			}
			break;
			case 2:
				break;
			case 3:
				
				LoadParticle(i, String::ToWString(path), ReadParticleType::Smoke);
				break;
			}
		}
		
		
	}

	uint treeCount = r->UInt();
	if (treeCount > 0)
	{
		auto path = r->String();
	}
	
	uint pointCount = r->UInt();
	if (pointCount > 0)
	{
		for (uint i = 0; i < pointCount; i++)
		{
			Vector3 Color = r->Vector3();
			Vector3 pos = r->Vector3();
			float intencity = r->Float();
			float Range = r->Float();
			Lighting.AddPointLight(pos, Range, Color, intencity, false);
		}
		

	}

	r->Close();
	SafeDelete(r);

	collider->ClearTextureTransforms();
	animator->ClearTextureTransforms();
	actorController->Start();
}

void Engine::Load(const uint& index, const wstring& modelName, const uint& actorCount, const ReadMeshType& meshType)
{
	BinaryReader* r = new BinaryReader();
	switch (meshType)
	{
	case ReadMeshType::StaticMesh:
	{
		
		staticRenderer[index].Initiallize(device, index);
		staticShadowRenderer[index].Initiallize(device, index);

		staticRenderer[index].ReadMaterial(modelName);
	
		staticActorCount = actorCount;
		collider->SetActorCount(staticActorCount);
		r->Open(L"../_Models/StaticMeshes/" + modelName + L"/" + modelName + L"_Edit" + L".mesh");
		{
			collider->ReadBone(r, index);
			staticRenderer[index].ReadMesh(r, meshType);
		}
		r->Close();
	
		bool isTree = false;
		if (modelName == L"tree")
		{
			isTree = true;
		}
		staticRenderer[index].CreateShader("../_Shaders/StaticMesh.hlsl", isTree);

		staticShadowRenderer[index].SetMesh(staticRenderer[index].GetMesh(), staticRenderer[index].GetMeshCount());
		staticShadowRenderer[index].SetShadowVertexBufferData(staticRenderer[index].GetShadowBuffer(), staticRenderer[index].GetIndexBuffer());
		staticShadowRenderer[index].CreateShader("../_Shaders/StaticMeshCSO");
		staticShadowRenderer[index].SetShadowStride(meshType);

		collider->RegisterRenderData(index,&staticRenderer[index]);
	
		
	}
	break;
	case ReadMeshType::SkeletalMesh:
	{
		skeletalRenderer[index].Initiallize(device, index);
		skeletalShadowRenderer[index].Initiallize(device, index);

		skeletalRenderer[index].ReadMaterial(modelName);
		skeletalRenderer[index].CreateShader("../_Shaders/SkeletalMesh.hlsl",false);
		

		skeletalActorCount = actorCount;
		animator->SetActorCount(skeletalActorCount);

		r->Open(L"../_Models/SkeletalMeshes/" + modelName + L"/" + modelName + L"_Edit" + L".mesh");
		{
		
			
				animator->ReadBone(r,index);
			

			
			skeletalRenderer[index].ReadMesh(r, meshType);
			animator->ReadBoneBox(r,index);
			animator->ReadBehaviorTree(r, index);
		}
		r->Close();
		
		animator->ReadClip(modelName, index);
		

		

		skeletalShadowRenderer[index].SetMesh(skeletalRenderer[index].GetMesh(), skeletalRenderer[index].GetMeshCount());
		skeletalShadowRenderer[index].SetShadowVertexBufferData(skeletalRenderer[index].GetShadowBuffer(), skeletalRenderer[index].GetIndexBuffer());
		skeletalShadowRenderer[index].CreateShader("../_Shaders/SkeletalMeshCSO");
		skeletalShadowRenderer[index].SetShadowStride(meshType);

		animator->RegisterRenderData(index, &skeletalRenderer[index]);
	}
	break;
	}

	SafeDelete(r);

	drawCount.clear();
	drawCount.shrink_to_fit();
	drawCount.assign((staticActorCount + skeletalActorCount),uint());
	

}

void Engine::LoadParticle(const uint & index, const wstring & path, const ReadParticleType & particleType)
{
	
	effects->LoadParticle(index, path ,particleType);
	
}

void Engine::PusInstance(const uint & index, const Matrix & world, const ReadMeshType& meshType)
{
	
	switch (meshType)
	{
	case ReadMeshType::StaticMesh:
		
		
		staticShadowRenderer[index].PushInstance();
		collider->PushDrawCount(index, world, true);
		
		
		break;
	case ReadMeshType::SkeletalMesh:

		skeletalShadowRenderer[index].PushInstance();
		animator->PushDrawCount(index, world);
		
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
	CreateHDRRTV(width, height);
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);


	Lighting.Resize(Vector2(static_cast<float>(width), static_cast<float>(height)));

	CascadedMatrixSet.CreateCascadedShadowBuffers(device, Vector2(static_cast<float>(width), static_cast<float>(height)));
	CascadedMatrixSet.SetShadowMapSize( Vector2(static_cast<float>(width), static_cast<float>(height)));
	
	GBuffer.Resize(width,height);
	hdr->Resize(width, height);
	ssao->Resize(width, height);
	sslr->Resize(width, height);
}


