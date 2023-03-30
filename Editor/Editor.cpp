#include "stdafx.h"
#include "Editor.h"
#include "Core/Engine.h"
#include "GBuffer/LightManager.h"
#include "GBuffer/GBufferData.h"
#include "./ImGui/imgui_impl_win32.h"
#include "./ImGui/imgui_impl_dx11.h"
#include "ImGui/ImGuizmo.h"

#include "ActorEditor/ActorEditor.h"
#include "PostEffects/HDR.h"
#include "PostEffects/SSLR.h"
#include "PostEffects/SSAO.h"

#include "Viewer/Freedom.h"
#include "Viewer/Orbit.h"

//Event
#include "EventSystems/ColliderSystem.h"
#include "EventSystems/Animator.h"
#include "EventSystems/Renderers/Renderer.h"
#include "EventSystems/Renderers/ShadowRenderer.h"
#include "EventSystems/PhysicsSystem.h"
//Particle
#include "ParticleEditor/ParticleEditor.h"
//Lights
#include "LightEditor/LightEditor.h"
#include "BehaviorTreeEditor.h"
#include "EventSystems/ActorController.h"

#include "Environment/Sky/Cloud.h"

#include "ProgressBar/ProgressReport.h"
#include "ProgressBar/ProgressBar.h"
//Terrain
#include "TerrainEditor/TerrainEditor.h"

#include "TextReader/TextReader.h"
extern class LightManager Lighting; 
extern class GBufferData GBuffer;


ID3D11DeviceContext* deferredContext;
ID3D11CommandList* commadList;

Editor::Editor()
	:bStart(false), flags(0),  clickedActorIndex(-1), clickedMeshType(GizmoType::Default),
	staticActorCount(0),skeletalActorCount(0), bShowCollider(false), mainCamera(nullptr), clickedInstanceIndex(-1)
, controllParticleIndex(-1), IsClicked(false), bShowQuadTree(false), 
   bSelectedCube(false), controllCubeIndex(-1)
{



    engine= new Engine();

	mainCamera = new Freedom();
	
	static_cast<Freedom*>(mainCamera)->Speed(2.5f, 0.1f);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	/*io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigWindowsResizeFromEdges = true;
    io.ConfigViewportsNoTaskBarIcon = true;*/

	ImGui_ImplWin32_Init(D3D::GetDesc().Handle);

	
	ImGui_ImplDX11_Init(engine->device, engine->immediateContext);
	ApplyStyle();

  
	flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
	


	{
		for (uint i = 0; i < 5; i++)
		{
			buttonTextures[i] = new Texture();
		}
	
		buttonTextures[0]->Load(engine->device, L"playButton.png",nullptr,true);
	    buttonTextures[1]->Load(engine->device, L"pauseButton.png",nullptr, true);
		buttonTextures[2]->Load(engine->device, L"stopButton.png", nullptr, true);
		
		buttonTextures[3]->Load(engine->device, L"cubeIcon.png", nullptr, true);
		buttonTextures[4]->Load(engine->device, L"CapsuleIcon.png", nullptr, true);
	}
	tree = new QuadTree();
	tree->CreateQuadTree(Vector2(0.0f,0.0f),Vector2(512.0f,512.0f));
	DebugLine::Get()->Initiallize(engine->device);
	
	D3DXMatrixIdentity(&identity);

	Vector3 boneBoxMin = Vector3(-0.5f, -0.5f, -0.5f);
	Vector3 boneBoxMax = Vector3(0.5f, 0.5f, 0.5f);

	boneBoxCollider[0] = Vector3(boneBoxMin.x, boneBoxMin.y, boneBoxMax.z);
	boneBoxCollider[1] = Vector3(boneBoxMax.x, boneBoxMin.y, boneBoxMax.z);
	boneBoxCollider[2] = Vector3(boneBoxMin.x, boneBoxMax.y, boneBoxMax.z);
	boneBoxCollider[3] = Vector3(boneBoxMax);
	boneBoxCollider[4] = Vector3(boneBoxMin);
	boneBoxCollider[5] = Vector3(boneBoxMax.x, boneBoxMin.y, boneBoxMin.z);
	boneBoxCollider[6] = Vector3(boneBoxMin.x, boneBoxMax.y, boneBoxMin.z);
	boneBoxCollider[7] = Vector3(boneBoxMax.x, boneBoxMax.y, boneBoxMin.z);

	lights = make_shared<LightEditor>(engine->device,this);

	terrainEditor = make_shared<TerrainEditor>(engine->device,engine->island11);

	UINT contextFlags = engine->immediateContext->GetContextFlags();
	Check(engine->device->CreateDeferredContext(contextFlags, &deferredContext));


	reader = new TextReader();
}

Editor::~Editor()
{


	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	SafeDelete(mainCamera);
  
}

void Editor::Resize(const uint & width, const uint & height)
{
	engine->Resize(width,height);

	ImGui_ImplDX11_InvalidateDeviceObjects();
	ImGui_ImplDX11_CreateDeviceObjects();
}

void Editor::Update()
{

	if (!ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemHovered())
	{
		Keyboard::Get()->Update();
		Mouse::Get()->Update();
	}
	
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();


	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::SetNextWindowBgAlpha(0.0f);
	
	ImGui::SetNextWindowBgAlpha(0.0f);
	
	ImGui::Begin
	(
		"TextWindow", NULL,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus
	);
	

	for (GuiTexture gTexture : textures)
	{
		ImVec2 position = ImVec2(gTexture.Position.x, gTexture.Position.y);
		ImVec2 size = ImVec2(gTexture.scale.x, gTexture.scale.y);
		ImColor color = ImColor(gTexture.color.r, gTexture.color.g, gTexture.color.b, gTexture.color.a);


		ImGui::GetWindowDrawList()->AddImage(gTexture.texture ? gTexture.texture->SRV() : nullptr, ImVec2(position.x - size.x / 2, position.y - size.y / 2),
			ImVec2(position.x + size.x / 2, position.y + size.y / 2), ImVec2(0, 0), ImVec2(1, 1), color);


	}
	textures.clear();

	
	
	
	if (IsClicked)
	{
		Gizmo();
	}
	
		


	
	ImGui::End();

	
	if (!bStart&&tree->Intersection(pos))
	{
		AddTrasform(pos);
		DropParticle(pos);
		DropCube(pos);
		terrainEditor->Update(pos,engine->immediateContext);

	}

	if (!bStart)
	{
		lights->Update(pos);
		mainCamera->Update();
	}
	
	

	
	
	engine->Update(bStart);

	if (Mouse::Get()->Down(0))
	BoxRayTracing();
	
}

void Editor::Render()
{
	auto context = engine->immediateContext;
	engine->Begin();
	engine->RenderEditor1();

	Lighting.RenderBrush(context, terrainEditor->GetBrushBuffer(context),GBuffer.DepthstencilSRV());
	
	engine->RenderEditor2();

	if (bShowCollider)
		DebugRender();
	if (bShowQuadTree)
	{
		auto tree = engine->animator->QuadTree();
		auto root = tree->GetRoot();
		uint id;
		
		auto node = tree->Intersection(id);
		if (node)
		{

			QuadTreeRender(tree->FindNode(root,id));
		}
		
		DebugLine::Get()->Render(context);

	}
		

	MenuBar();
	ToolBar();
	
	for (auto& actor : actors)
	{
		actor->Render(deferredContext);
	}
	for (auto& particle : particles)
	{
		particle->PreviewRender(deferredContext);
	}
	deferredContext->FinishCommandList(false, &commadList);
	context->ExecuteCommandList(commadList, true);
	SafeRelease(commadList);
	
	Cloud();
	
	PostEffects();
	BehaviorTreeAsset();
	ParticleAsset();
	QuadTreeEditor();
	
	ActorAsset();

	if (!bStart)
	terrainEditor->ImGui();
	
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	engine->Present();
	
}

void Editor::PostEffects()
{
	if (bStart)return;

	auto HDR = engine->hdr;
	auto parms = HDR->GetParams();
	ImGui::Begin("HDR", 0,flags);
	{

		ImGui::SliderFloat("BloomScale", (float*)&parms->BloomScale, 0.0f, 2.0f);
		ImGui::SliderFloat("BloomThreshold", (float*)&parms->BloomThreshold, 0.0f, 2.0f);
		ImGui::SliderFloat("MiddleGrey", (float*)&parms->MiddleGrey, 0.0f, 9.0f);
		ImGui::SliderFloat("White", (float*)&parms->White, 0.0f, 9.0f);
	}
	ImGui::End();


	auto SSLR = engine->sslr;
	auto sslrparms = SSLR->GetParams();
	ImGui::Begin("SSLR", 0,flags);
	{
		ImGui::SliderFloat("MaxSunDist", (float*)&sslrparms->MaxSunDist, 0.0f, 20.0f);
		ImGui::SliderFloat("intensity", (float*)&sslrparms->intensity, 0.01f, 10.0f);
		ImGui::SliderFloat("decay", (float*)&sslrparms->decay, 0.01f, 0.5f);
		ImGui::SliderFloat("ddecay", (float*)&sslrparms->ddecay, 0.01f, 0.5f);
		ImGui::SliderFloat("dist", (float*)&sslrparms->dist, 50.0f, 1000.0f);
		ImGui::SliderFloat("MaxDeltaLen", (float*)&sslrparms->MaxDeltaLen, -1.0f, 5.0f);
	}
	ImGui::End();



	auto SSAO = engine->ssao;
	auto ssaoparms = SSAO->GetParams();
	ImGui::Begin("SSAO", 0, flags);
	{
		ImGui::SliderFloat("Radius", (float*)&ssaoparms->Radius, 0.0f, 20.0f);
		ImGui::SliderFloat("SSAOSampRadius", (float*)&ssaoparms->SSAOSampRadius, 0.01f, 20.0f);
		
	}
	ImGui::End();



	lights->ImageButton();
	if (lights->LightList() > -1&& clickedMeshType != GizmoType::Light)
	{
		clickedMeshType = GizmoType::Light;
		controllParticleIndex = -1;
		IsClicked = true;
	}
	auto fogDesc = &engine->fogDesc;
	ImGui::Begin("FOG", 0, flags);
	{
		ImGui::ColorEdit3("fogColor", (float*)&fogDesc->fogColor);
		ImGui::ColorEdit3("HighlightColor", (float*)&fogDesc->HighlightColor);
		ImGui::SliderFloat("StartDepth", (float*)&fogDesc->StartDepth, 0.0f, 100.0f);
		ImGui::SliderFloat("GlobalDensity", (float*)&fogDesc->GlobalDensity, 0.0f, 0.1f);
		ImGui::SliderFloat("HeightFalloff", (float*)&fogDesc->HeightFalloff, 0.0f, 0.1f);

	}
	ImGui::End();
}

void Editor::Cloud()
{
	if (bStart)return;
	auto cloud = engine->cloud;
	auto parms = cloud->GetParams();
	
	ImGui::Begin("Cloud", 0, flags);
	{

		ImGui::SliderFloat("Cover", &parms->Cover, -3.0f,3.0f);
		ImGui::SliderFloat("Sharpness", &parms->Sharpness, 0.0f, 1.0f);
		ImGui::SliderFloat("TilesX", &parms->CloudTiles.x, 0.0f, 2.0f);
		ImGui::SliderFloat("TilesY", &parms->CloudTiles.y, 0.0f, 2.0f);
		ImGui::SliderFloat("Scale", cloud->GetScale(), 1.0f, 20.0f);
		
		
	}
	ImGui::End();
}

void Editor::Colliders()
{
	ImGui::Begin("Colliders", 0);
	{
		if (ImGui::ImageButton(buttonTextures[3]->SRV(), ImVec2(50.0f, 50.0f)))//pause
		{


		}

		if (ImGui::ImageButton(buttonTextures[4]->SRV(), ImVec2(50.0f, 50.0f)))//pause
		{


		}
	}
}

void Editor::Save(const wstring & fileName)
{
	BinaryWriter* w = new BinaryWriter();

	auto ext = ".Level";
	string sname = String::ToString(fileName);
	if (String::Contain(String::ToString(fileName), ext))
	{
		auto lastIndex = fileName.find_last_of('.');
		sname = String::ToString(fileName).substr(0, lastIndex);
	}

	w->Open(String::ToWString(sname + ext));
	w->UInt(actors.size());
	for (auto& actor : actors)
	{
		actor->Save(w);
	}
	w->UInt(particles.size());
	for (auto& particle : particles)
	{
		particle->Save(w);
	}
	w->UInt(behaviorTrees.size());
	for (auto& tree : behaviorTrees)
	{
		tree->Save(w);
	}

	lights->Save(w);

	w->Close();
	SafeDelete(w);
}

void Editor::LoadText(const wstring & fileName)
{
	reader->ReadText(fileName);
}

void Editor::Load(const wstring & fileName)
{
	

	
	LoadThread(fileName);
}

void Editor::LoadThread(const wstring & fileName)
{
	
//	Thread::Get()->AddTask([this, fileName]()
//	{
		BinaryReader* r = new BinaryReader();
		r->Open(fileName);
		ProgressReport::Get().SetJobCount(ProgressReport::Model, 8000);


		uint count = r->UInt();
		if (count > 0)
		{
			for (uint i = 0; i < count; i++)
			{
				auto& actor = make_shared<ActorEditor>(engine->device, engine, this);
				actor->Load(r);
				actors.emplace_back(actor);
			}
		}
		for (uint i = 0; i < 5000; i++)
		{
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}


		uint particleCount = r->UInt();
		if (particleCount > 0)
		{
			for (uint i = 0; i < particleCount; i++)
			{
				auto& particle = make_shared<ParticleEditor>(engine->device, engine, this, particles.size());
				uint type = r->UInt();
				auto path = r->String();
				particle->Load(String::ToWString(path), true);
				if (particle->GetParticleType() == ReadParticleType::Smoke)
				{
					auto find = find_if(keys.begin(), keys.end(), [particle](uint index)
					{
						return index == particle->GetID();
					});
					if (find == keys.end())
					{
						keys.emplace_back(particle->GetID());
						string data = "";
						data = "Smoke" + to_string(particle->GetID());
						spaceParticles.emplace_back(data);
					}
				}
				particles.emplace_back(particle);
			}


		}
		for (uint i = 0; i < 2000; i++)
		{
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}
		uint treeCount = r->UInt();
		if (treeCount > 0)
		{
			auto& tree = make_shared<BehaviorTreeEditor>(engine->device);
			behaviorTrees.emplace_back(tree);
			auto path = r->String();
			behaviorTrees.front()->LoadAndCompile(path);

			actors[0]->LoadBehaviorTree();
		}
		for (uint i = 0; i < 1000; i++)
		{
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
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
				lights->PushPointCount();
			}


		}

		r->Close();
		SafeDelete(r);

	//});
}


void Editor::RenderTexture(Vector2 position, Vector2 scale, Color color, Texture * texture)
{
	GuiTexture gTexture;
	gTexture.Position = position;
	gTexture.scale = scale;
	gTexture.color = color;
	gTexture.texture = texture;


	textures.emplace_back(gTexture);
}

void Editor::Gizmo()
{
	isMove = false;
	static ImGuizmo::OPERATION operation(ImGuizmo::TRANSLATE);
	//static ImGuizmo::MODE mode(ImGuizmo::WORLD);
	static ImGuizmo::MODE mode(ImGuizmo::LOCAL);
	if (ImGui::IsKeyPressed('T'))//w
		operation = ImGuizmo::TRANSLATE;
	if (clickedMeshType!=GizmoType::DebugFigures&&ImGui::IsKeyPressed('Y'))//e
		operation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed('U'))//r
		operation = ImGuizmo::SCALE;

	switch (clickedMeshType)
	{
	case GizmoType::StaticMesh:
	{

		Matrix gizmo;
		engine->collider->GetInstMatrix(&gizmo, clickedActorIndex, clickedInstanceIndex);
		Matrix gizmoDelta;
		D3DXMatrixIdentity(&gizmoDelta);

		const	ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		GlobalData::GetView(&v); GlobalData::GetProj(&p);
		ImGuizmo::Manipulate(v,p, operation, mode,
			gizmo, gizmoDelta, nullptr, &isMove);

	
		if (gizmoDelta != identity)
		{
			
			engine->collider->SetInstMatrix(&gizmo, clickedActorIndex, clickedInstanceIndex);
			engine->collider->CreateShadowInstTransformSRV();
		}
	}
	break;
	case GizmoType::SkeletalMesh:
	{
		Matrix gizmo;
		engine->animator->GetInstMatrix(&gizmo, clickedActorIndex, clickedInstanceIndex);

		Matrix gizmoDelta;
		D3DXMatrixIdentity(&gizmoDelta);

		const	ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
		GlobalData::GetView(&v); GlobalData::GetProj(&p);
		ImGuizmo::Manipulate(v,p, operation, mode,
			gizmo, gizmoDelta, nullptr, &isMove);

		
		if (gizmoDelta != identity)
		{
			
			engine->animator->SetInstMatrix(&gizmo, clickedActorIndex, clickedInstanceIndex);
		}

	}
	break;
	case GizmoType::Particle:
	{


		Matrix gizmo = particles[keys[controllParticleIndex]]->GetWorld();

		Matrix gizmoDelta;
		D3DXMatrixIdentity(&gizmoDelta);

		const	ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
		GlobalData::GetView(&v); GlobalData::GetProj(&p);
		ImGuizmo::Manipulate(v,p, operation, mode,
			gizmo, gizmoDelta, nullptr, &isMove);

		
		if (gizmoDelta != identity)
		{
			auto key = keys[controllParticleIndex];
			auto find = find_if(particles.begin(), particles.end(), [key](shared_ptr<class ParticleEditor> particle)
			{
				return particle->GetID() == key;
			});
			if (find != particles.end())
			{

				auto particle = *find;
				particle->SetWorld(gizmo);
				bNeedUpdate = true;
			}
	
		}
			
		if (Mouse::Get()->Up(0) && bNeedUpdate)
		{
			auto key = keys[controllParticleIndex];
			auto find = find_if(particles.begin(), particles.end(), [key](shared_ptr<class ParticleEditor> particle)
			{
				return particle->GetID() == key;
			});
			if (find != particles.end())
			{
				auto particle = *find;
				particle->Compile();
				bNeedUpdate = false;
			}
			
		}
	
		
	}
	break;
	case GizmoType::Light:
	{


		Matrix gizmo;
		D3DXMatrixIdentity(&gizmo);
		Vector3 pos;
		lights->GetLightPosition(pos);
		for (uint i = 0; i < 3; i++)
		{
			gizmo.m[3][i] = pos[i];
		}

		Matrix gizmoDelta;
		D3DXMatrixIdentity(&gizmoDelta);

		const	ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
		GlobalData::GetView(&v); GlobalData::GetProj(&p);
		ImGuizmo::Manipulate(v,p, operation, mode,
			gizmo, gizmoDelta, nullptr, &isMove);

	
		if (gizmoDelta != identity)
		{
			lights->SetLightPosition(gizmo.m[3]);
		}


	}
	break;

	case GizmoType::DebugFigures:
	{
		if (controllCubeIndex < 0)return;

		Matrix gizmo = debugCubes[controllCubeIndex].world;

		Matrix gizmoDelta;
		D3DXMatrixIdentity(&gizmoDelta);

		const	ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
		GlobalData::GetView(&v); GlobalData::GetProj(&p);
		ImGuizmo::Manipulate(v,p, operation, mode,
			gizmo, gizmoDelta, nullptr, &isMove);


		if (gizmoDelta != identity)
		{
			
			

			debugCubes[controllCubeIndex].world = gizmo;
			

		}


	}
	break;


	}
}

void Editor::ActorAsset()
{
	if (bStart)return;

	ImGui::Begin("Assets", 0, flags);
	{

		static bool isEditActor = false;
	
		static vector< shared_ptr<ActorEditor>>::iterator saveIter= actors.end();
		vector< shared_ptr<ActorEditor>>::iterator find = find_if(actors.begin(), actors.end(), [](shared_ptr<ActorEditor>actor)
		{
			return actor->ImageButton();
		});
		
		if (find != actors.end())
		{
			
			isEditActor = true;
			if (ImGui::GetIO().MouseDown[1])
				ImGui::OpenPopup("actorPopUp");

			saveIter = find;

			for (vector<shared_ptr<ActorEditor>>::iterator iter = find+1; iter != actors.end(); iter++)
			{
				shared_ptr < ActorEditor>& actor = *iter;
				actor->ImageButton();
			}
		}
		else
		{
			
			isEditActor = false;
		}

		if (ImGui::BeginPopup("actorPopUp"))
		{
			if (ImGui::MenuItem("Copy")) {}
			if (ImGui::MenuItem("Delete"))
			{
				actors.erase(saveIter);
			}
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		
		if (ImGui::IsWindowHovered())
		{
			if (!isEditActor&&ImGui::GetIO().MouseDown[1])
				ImGui::OpenPopup("Popup");
		}
		if (ImGui::BeginPopup("Popup"))
		{
			if (ImGui::MenuItem("Copy")) {}
			if (ImGui::MenuItem("Delete"))
			{
				
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Add Actor"))
			{
				auto& actor = make_shared<ActorEditor>(engine->device,engine,this);
				actors.emplace_back(actor);

			}
		

			ImGui::EndPopup();
		}


	}
	ImGui::End();
}

void Editor::ParticleAsset()
{
	if (bStart)return;
	
	ImGui::Begin("ParticleAsset", 0, flags);
	{

		for (auto& particle : particles)
		{
			particle->ImageButton();
			
		}
		ImGui::SameLine();

		if (ImGui::IsWindowHovered())
		{
			if (ImGui::GetIO().MouseDown[1])
				ImGui::OpenPopup("ParticlePopup");
		}
		if (ImGui::BeginPopup("ParticlePopup"))
		{
			if (ImGui::MenuItem("Copy")) {}
			if (ImGui::MenuItem("Delete")){}

			ImGui::Separator();

			if (ImGui::MenuItem("Add Particle"))
			{
				auto& particle = make_shared<ParticleEditor>(engine->device,engine,this,particles.size());
				particles.emplace_back(particle);

			}


			ImGui::EndPopup();
		}


	}
	ImGui::End();

	if (SpaceParticleList() > -1 && clickedMeshType != GizmoType::Particle)
	{
		clickedMeshType = GizmoType::Particle;
		
		lights->SetControllLightIndex(-1);
		IsClicked = true;
	}
}

int Editor::SpaceParticleList()
{

	ImGui::Begin("SpaceParticleList", 0, flags);
	{
		ImGui::Columns(1, "mycolumns1", false);  // 3-ways, no border
		ImGui::Separator();

		{

			for (uint p = 0; p < spaceParticles.size(); p++)
			{
				const string& name = spaceParticles[p];
				const char* label = name.c_str();

				ImGuiTreeNodeFlags  flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

				if (ImGui::TreeNodeEx(label, flags))
				{
					if (ImGui::IsItemClicked())
					{
						controllParticleIndex = p;
					}
					ImGui::TreePop();

				}
			}



		}


	}
	ImGui::End();

	return controllParticleIndex;
}

void Editor::BehaviorTreeAsset()
{
	
	if (bStart)return;

	ImGui::Begin("BehaviorTreeAsset", 0, flags);
	{

		for (auto& tree : behaviorTrees)
		{
			tree->ImageButton();
		}
		ImGui::SameLine();

		if (ImGui::IsWindowHovered())
		{
			if (ImGui::GetIO().MouseDown[1])
				ImGui::OpenPopup("TreePopup");
		}
		if (ImGui::BeginPopup("TreePopup"))
		{
			if (ImGui::MenuItem("Copy")) {}
			if (ImGui::MenuItem("Delete")) {}

			ImGui::Separator();

			if (ImGui::MenuItem("Add BehaviorTree"))
			{
				auto& tree = make_shared<BehaviorTreeEditor>(engine->device);
				behaviorTrees.emplace_back(tree);

			}


			ImGui::EndPopup();
		}


	}
	ImGui::End();
}

void Editor::MenuBar()
{
	if (bStart)return;
	if (ImGui::BeginMainMenuBar())
	{
		//File Save & Load
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Load"))
			{
				HWND hWnd = NULL;
				function<void(wstring)> f = bind(&Editor::Load, this, placeholders::_1);
			    Path::OpenFileDialog(L"", Path::LevelFilter, L"../_Levels/", f, hWnd);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save")) 
			{
				//ShellExecute(D3D::GetDesc().Handle, L"open", L"../Game/Game.exe",nullptr , nullptr, SW_SHOWNORMAL);
			}
			if (ImGui::MenuItem("Save as..."))
			{
				HWND hWnd = NULL;
				function<void(wstring)> f = bind(&Editor::Save, this, placeholders::_1);
				Path::SaveFileDialog(L"", Path::LevelFilter, L"../_Levels/", f, hWnd);
			}
			if (ImGui::MenuItem("LoadText"))
			{
				HWND hWnd = NULL;
				function<void(wstring)> f = bind(&Editor::LoadText, this, placeholders::_1);
				Path::OpenFileDialog(L"", Path::EveryFilter, L"../input_files/", f, hWnd);
				
			}

			ImGui::EndMenu();
		}

		//Tools
		if (ImGui::BeginMenu("Tools"))
		{
			

			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

}

void Editor::ToolBar()
{
	ImGui::Begin("ToolBar",0,flags);
	{
		if (ImGui::ImageButton(buttonTextures[0]->SRV(), ImVec2(20.0f, 20.0f)))//play
		{
			engine->actorController->Start();
			bStart = true;
			if (Time::Get()->Stopped())
			{
				Time::Get()->Start();
			}
		}
	    ImGui::SameLine();
		if (ImGui::ImageButton(buttonTextures[1]->SRV(), ImVec2(20.0f, 20.0f)))//pause
		{
			engine->actorController->Stop();
			bStart = false;
			
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(buttonTextures[2]->SRV(), ImVec2(20.0f, 20.0f)))//stop
		{
			bStart = false;
			engine->animator->IintAnimData();
			Time::Get()->Stop();
		}
		
		ImGui::Checkbox("Show ColliderBox", &bShowCollider);

		bShowQuadTree = bShowCollider == true ? false : bShowQuadTree;
		ImGui::Checkbox("Show QuadTree", &bShowQuadTree);
	
		const string& str = string("Frame Rate : ") + to_string(ImGui::GetIO().Framerate);
		ImGui::Text(str.c_str());

		Vector3 pos;
		mainCamera->Position(&pos);

		string str2 = "Cam Position : ";
		str2 += to_string((int)pos.x) + ", " + to_string((int)pos.y) + ", " + to_string((int)pos.z);
		ImGui::Text(str2.c_str());

		Vector3 rotation;
		static_cast<Freedom*>(mainCamera)->Rotation(&rotation);
			
		string str1 = "Cam Rotation : ";
		str1 += to_string((int)rotation.x) + ", " + to_string((int)rotation.y) + ", " + to_string((int)rotation.z);
		ImGui::Text(str1.c_str());
		
	}
	ImGui::End();
}

void Editor::EditorMenu()
{
}

void Editor::AddTrasform(const Vector3 & pos)
{
	//for_each(actors.begin(), actors.end(), [&pos](Actor* actor) {actor->AddTransform(pos); });
	if (bStart) return;

	auto darggedActor = std::find_if(actors.begin(), actors.end(),
		[](shared_ptr<ActorEditor> actor) {return actor->IsDragged() == true; });
	if (darggedActor != actors.end())
	{
		auto actor = *darggedActor;
		if (actor->IsMove() == false) return;
		if (Mouse::Get()->Up(0))
		{
			Matrix previewWorld;
			D3DXMatrixTranspose(&previewWorld, &actor->PreviewWorld());
			previewWorld._41 = pos.x;
			previewWorld._42 = pos.y;
			previewWorld._43 = pos.z;

			engine->PusInstance(actor->ActorIndex(), previewWorld, actor->MeshType());

		//	uint boxCallIndex = actor->BoxCallIndex();
		//	DebugLine::Get()->SetInstPointer(engine->collider->GetInstMatrixPointer(pushedActorIndex), boxCallIndex,0);
		//	DebugLine::Get()->DrawCount(boxCallIndex);

			actor->IsDragged(false);
			
		}

	}

}

void Editor::DropParticle(const Vector3 & pos)
{
	if (bStart) return;

	auto darggedParticle = std::find_if(particles.begin(), particles.end(),
		[](shared_ptr<ParticleEditor> particle) {return particle->IsDragged() == true; });
	if (darggedParticle != particles.end())
	{
		auto particle = *darggedParticle;
	
		if (Mouse::Get()->Up(0))
		{
			auto particleType = particle->GetParticleType();
			if (particleType != ReadParticleType::Smoke) return;

			auto find = find_if(keys.begin(), keys.end(), [particle](uint index)
			{
				return index == particle->GetID();
			});
			if (find == keys.end())
			{
				

				Matrix world = particle->GetWorld();

				for (uint i = 0; i < 3; i++)
					world.m[3][i] = pos[i];

				particle->SetWorld(world);

				keys.emplace_back(particle->GetID());
			
				string data = "";
				data = "Smoke" + to_string(particle->GetID());
				
				spaceParticles.emplace_back(data);
				particle->Compile();
			}
		

			
			
			
			particle->IsDragged(false);

		}

	}
}

void Editor::DropCube(const Vector3 & pos)
{
	if (bStart||bSelectedCube==false) return;

	

	if (Mouse::Get()->Up(0))
	{
		DebugCube cube;


		for (uint i = 0; i < 3; i++)
			cube.world.m[3][i] = pos[i];

		debugCubes.emplace_back(cube);
	
		bSelectedCube = false;
	}

}

void Editor::DebugRender()
{
	uint renderedStaticActorCount;
	engine->collider->ActorCount(renderedStaticActorCount);
	uint renderedSkeletalActorCount;
	engine->animator->ActorCount(renderedSkeletalActorCount);


	Color color = Color(0, 1, 0, 1);
	for (uint i = 0; i < renderedStaticActorCount; i++)
	{
		uint drawCount = engine->collider->DrawCount(i);
		for (uint d = 0; d < drawCount; d++)
		{
			vector<Vector3> dest = engine->collider->GetBoxMinMax(i, d);
			//Front
			DebugLine::Get()->RenderLine(dest[0], dest[1], color);
			DebugLine::Get()->RenderLine(dest[1], dest[3], color);
			DebugLine::Get()->RenderLine(dest[3], dest[2], color);
			DebugLine::Get()->RenderLine(dest[2], dest[0], color);

			//Backward
			DebugLine::Get()->RenderLine(dest[4], dest[5], color);
			DebugLine::Get()->RenderLine(dest[5], dest[7], color);
			DebugLine::Get()->RenderLine(dest[7], dest[6], color);
			DebugLine::Get()->RenderLine(dest[6], dest[4], color);

			//Side
			DebugLine::Get()->RenderLine(dest[0], dest[4], color);
			DebugLine::Get()->RenderLine(dest[1], dest[5], color);
			DebugLine::Get()->RenderLine(dest[2], dest[6], color);
			DebugLine::Get()->RenderLine(dest[3], dest[7], color);
		}
	}

	for (uint i = 0; i < renderedSkeletalActorCount; i++)
	{
		uint drawCount = engine->animator->DrawCount(i);
		for (uint d = 0; d < drawCount; d++)
		{
			vector<Vector3> dest = engine->animator->GetBoxMinMax(i, d);
			//Front
			DebugLine::Get()->RenderLine(dest[0], dest[1], color);
			DebugLine::Get()->RenderLine(dest[1], dest[3], color);
			DebugLine::Get()->RenderLine(dest[3], dest[2], color);
			DebugLine::Get()->RenderLine(dest[2], dest[0], color);

			//Backward
			DebugLine::Get()->RenderLine(dest[4], dest[5], color);
			DebugLine::Get()->RenderLine(dest[5], dest[7], color);
			DebugLine::Get()->RenderLine(dest[7], dest[6], color);
			DebugLine::Get()->RenderLine(dest[6], dest[4], color);

			//Side
			DebugLine::Get()->RenderLine(dest[0], dest[4], color);
			DebugLine::Get()->RenderLine(dest[1], dest[5], color);
			DebugLine::Get()->RenderLine(dest[2], dest[6], color);
			DebugLine::Get()->RenderLine(dest[3], dest[7], color);


		}


	}

	//engine->animator->BoxRender();
	lights->GridPointLight();
	//QuadTreeRender(engine->animator->QuadTree()->GetRoot());
	uint totalCount =0;
	for (uint i = 0; i < renderedSkeletalActorCount; i++)
	{
		totalCount += engine->animator->DrawCount(i);
	}
	auto tweenData = engine->animator->tweenData;
	auto root = engine->animator->QuadTree()->GetRoot();
	for (uint i = 0; i < totalCount; i++)
	{
		QuadTreeRender(root, tweenData[i].quadTreeID);
	}

	RenderDebugCube();
	DebugLine::Get()->Render(engine->immediateContext);






	
	{
	
		const uint& stackCount = 30;
		const float& thetaStep = 2.0f * Math::PI / stackCount;
		const Vector3& start = Vector3(0, 0, 0);
		const Vector3& dir = Vector3(0, 1, 0);
		const Vector3& end = dir * 1;
		const float& radius = 0.5f;

		Vector3 axis;
		D3DXVec3Cross(&axis, &Vector3(0, 0, 1), &dir);
		float radian = D3DXVec3Dot(&Vector3(0, 0, 1), &dir) - Math::PI*0.5f;

		Matrix R;
		D3DXMatrixRotationAxis(&R, &axis, radian);
		vector<Vector3> v, v2, v3;
		vector<Vector3> e, e2, e3;
		for (UINT s = 0; s <= stackCount; s++)
		{
			float theta = s * thetaStep;

			Vector3 p = Vector3
			(
				(radius * cosf(theta)),
				0,
				(radius * sinf(theta))
			);
			Vector3 p2 = Vector3
			(
				0,
				(radius * cosf(theta)),
				(radius * sinf(theta))
			);
			Vector3 p3 = Vector3
			(
				(radius * cosf(theta)),
				(radius * sinf(theta)),
				0
			);
			D3DXVec3TransformCoord(&p, &p, &R);
			D3DXVec3TransformCoord(&p2, &p2, &R);
			D3DXVec3TransformCoord(&p3, &p3, &R);
			p += start;
			p2 += start;
			p3 += start;
			v.emplace_back(p);
			v2.emplace_back(p2);
			v3.emplace_back(p3);

			p += end;
			p2 += end;
			p3 += end;
			e.emplace_back(p);
			e2.emplace_back(p2);
			e3.emplace_back(p3);
		}
		uint drawCount= engine->animator->TotalCount();
		
		for (uint i = 0; i < drawCount; i++)
		{
			for (UINT s = 0; s < stackCount; s++)
			{
				if (s <= stackCount / 2)
				{
					DebugLine::Get()->RenderLine(v[s], v[s + 1], i);
					DebugLine::Get()->RenderLine(v2[s], v2[s + 1], i);
				}
				DebugLine::Get()->RenderLine(v3[s], v3[s + 1], i);
			}
			DebugLine::Get()->RenderLine(v3[stackCount / 4], e3[stackCount / 4], i);
			DebugLine::Get()->RenderLine(v3[stackCount / 2], e3[stackCount / 2], i);
			DebugLine::Get()->RenderLine(v3[stackCount * 3 / 4], e3[stackCount * 3 / 4], i);
			DebugLine::Get()->RenderLine(v3[stackCount], e3[stackCount], i);

			for (UINT s = 0; s < stackCount; s++)
			{
				if (s >= stackCount / 2)
				{
					DebugLine::Get()->RenderLine(e[s], e[s + 1], i);
					DebugLine::Get()->RenderLine(e2[s], e2[s + 1], i);
				}
				DebugLine::Get()->RenderLine(e3[s], e3[s + 1], i);
			}

			//DebugLine::Get()->RenderLine(boneBoxCollider[0], boneBoxCollider[1], color, i);
			//DebugLine::Get()->RenderLine(boneBoxCollider[1], boneBoxCollider[3], color, i);
			//DebugLine::Get()->RenderLine(boneBoxCollider[3], boneBoxCollider[2], color, i);
			//DebugLine::Get()->RenderLine(boneBoxCollider[2], boneBoxCollider[0], color, i);

			////Backward																  
			//DebugLine::Get()->RenderLine(boneBoxCollider[4], boneBoxCollider[5], color, i);
			//DebugLine::Get()->RenderLine(boneBoxCollider[5], boneBoxCollider[7], color, i);
			//DebugLine::Get()->RenderLine(boneBoxCollider[7], boneBoxCollider[6], color, i);
			//DebugLine::Get()->RenderLine(boneBoxCollider[6], boneBoxCollider[4], color, i);

			////Side																	  
			//DebugLine::Get()->RenderLine(boneBoxCollider[0], boneBoxCollider[4], color, i);
			//DebugLine::Get()->RenderLine(boneBoxCollider[1], boneBoxCollider[5], color, i);
			//DebugLine::Get()->RenderLine(boneBoxCollider[2], boneBoxCollider[6], color, i);
			//DebugLine::Get()->RenderLine(boneBoxCollider[3], boneBoxCollider[7], color, i);


		}
	}

	
	DebugLine::Get()->BoneBoxRender(engine->immediateContext,engine->physics->modelData_StructuredBufferSRV);


	

	
}

void Editor::QuadTreeRender(shared_ptr<QuadTreeNode> node)
{
	if (node->hierarchyIndex >6)return;

	if (node->childs.empty())return;
	D3DXVECTOR3 dest[8];

	//cout << "boundsMax.y:";
	//cout << node->boundsMax.y << endl;

	dest[0] = Vector3(node->boundsMin.x, node->boundsMin.y, node->boundsMax.z);
	dest[1] = Vector3(node->boundsMax.x, node->boundsMin.y, node->boundsMax.z);
	dest[2] = Vector3(node->boundsMin.x, node->boundsMax.y, node->boundsMax.z);
	dest[3] = Vector3(node->boundsMax.x, node->boundsMax.y, node->boundsMax.z);
	dest[4] = Vector3(node->boundsMin);
	dest[5] = Vector3(node->boundsMax.x, node->boundsMin.y, node->boundsMin.z);
	dest[6] = Vector3(node->boundsMin.x, node->boundsMax.y, node->boundsMin.z);
	dest[7] = Vector3(node->boundsMax.x, node->boundsMax.y, node->boundsMin.z);

	//
	//D3DXMATRIX world = transform->World();
	////D3DXMatrixTranspose(&world, &transform->World());
	//for (UINT i = 0; i < 8; i++)
	//	D3DXVec3TransformCoord(&dest[i], &dest[i], &world);

	Color color = node->hitted ? Color(1, 0, 0, 1) : Color(0, 0, 1, 1);
	//Front
	DebugLine::Get()->RenderLine(dest[0], dest[1], color);
	DebugLine::Get()->RenderLine(dest[1], dest[3], color);
	DebugLine::Get()->RenderLine(dest[3], dest[2], color);
	DebugLine::Get()->RenderLine(dest[2], dest[0], color);

	//Backward
	DebugLine::Get()->RenderLine(dest[4], dest[5], color);
	DebugLine::Get()->RenderLine(dest[5], dest[7], color);
	DebugLine::Get()->RenderLine(dest[7], dest[6], color);
	DebugLine::Get()->RenderLine(dest[6], dest[4], color);

	//Side
	DebugLine::Get()->RenderLine(dest[0], dest[4], color);
	DebugLine::Get()->RenderLine(dest[1], dest[5], color);
	DebugLine::Get()->RenderLine(dest[2], dest[6], color);
	DebugLine::Get()->RenderLine(dest[3], dest[7], color);
	
	for (auto& child : node->childs)
	{
		
		QuadTreeRender(child);
	}
	
}

void Editor::QuadTreeRender(shared_ptr<QuadTreeNode> node, uint NodeID)
{
	
	if (node->hitted == false)return;
	
        D3DXVECTOR3 dest[8];
        
        //cout << "boundsMax.y:";
        //cout << node->boundsMax.y << endl;
        
        dest[0] = Vector3(node->boundsMin.x, node->boundsMin.y, node->boundsMax.z);
        dest[1] = Vector3(node->boundsMax.x, node->boundsMin.y, node->boundsMax.z);
        dest[2] = Vector3(node->boundsMin.x, node->boundsMax.y, node->boundsMax.z);
        dest[3] = Vector3(node->boundsMax.x, node->boundsMax.y, node->boundsMax.z);
        dest[4] = Vector3(node->boundsMin);
        dest[5] = Vector3(node->boundsMax.x, node->boundsMin.y, node->boundsMin.z);
        dest[6] = Vector3(node->boundsMin.x, node->boundsMax.y, node->boundsMin.z);
        dest[7] = Vector3(node->boundsMax.x, node->boundsMax.y, node->boundsMin.z);
        
        //
        //D3DXMATRIX world = transform->World();
        ////D3DXMatrixTranspose(&world, &transform->World());
        //for (UINT i = 0; i < 8; i++)
        //	D3DXVec3TransformCoord(&dest[i], &dest[i], &world);
        
      //  Color color = node->hitted ? Color(1, 0, 0, 1) : Color(0, 0, 1, 1);
		Color color =  Color(0, 0, 1, 1);
		//Front
		DebugLine::Get()->RenderLine(dest[0], dest[1], color);
		DebugLine::Get()->RenderLine(dest[1], dest[3], color);
		DebugLine::Get()->RenderLine(dest[3], dest[2], color);
		DebugLine::Get()->RenderLine(dest[2], dest[0], color);

		//Backward
		DebugLine::Get()->RenderLine(dest[4], dest[5], color);
		DebugLine::Get()->RenderLine(dest[5], dest[7], color);
		DebugLine::Get()->RenderLine(dest[7], dest[6], color);
		DebugLine::Get()->RenderLine(dest[6], dest[4], color);

		//Side
		DebugLine::Get()->RenderLine(dest[0], dest[4], color);
		DebugLine::Get()->RenderLine(dest[1], dest[5], color);
		DebugLine::Get()->RenderLine(dest[2], dest[6], color);
		DebugLine::Get()->RenderLine(dest[3], dest[7], color);
	
		node->hitted = false;
	//}
	
	for (auto& child : node->childs)
	{
		if(child->hitted==true)
		QuadTreeRender(child, NodeID);
	}
}

void Editor::QuadTreeEditor()
{
	if (bStart)return;

	ImGui::Begin("QuadTreeEditor",0,flags);
	{
		if (ImGui::ImageButton(buttonTextures[3]->SRV(), ImVec2(50.0f, 50.0f)))//play
		{
			bSelectedCube = true;
			clickedMeshType = GizmoType::DebugFigures;
			IsClicked = true;
		}
		
		ImGui::Columns(1, "cubeList", false);  // 3-ways, no border
		ImGui::Separator();

		{

			for (uint p = 0; p < debugCubes.size(); p++)
			{
				const string& name = "Cube"+to_string(p);
				const char* label = name.c_str();

				ImGuiTreeNodeFlags  flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

				if (ImGui::TreeNodeEx(label, flags))
				{
					if (ImGui::IsItemClicked())
					{
						controllCubeIndex = p;
						clickedMeshType = GizmoType::DebugFigures;
						IsClicked = true;
					}
					ImGui::TreePop();

				}
			}



		}

		if (ImGui::Button("UpdateHeight", ImVec2(100, 30)))
		{
			Vector3 dest[2];

			Vector3 min = Vector3(-0.5f, -0.5f, -0.5f);
			Vector3 max = Vector3(0.5f, 0.5f, 0.5f);
			for (uint p = 0; p < debugCubes.size(); p++)
			{


				dest[0] = min;
				dest[1] = max;
				
				for (uint b = 0; b < 2; b++)
				{
					D3DXVec3TransformCoord(&dest[b], &dest[b], &debugCubes[p].world);

				}
				engine->animator->QuadTree()->UpdateHeight(dest[0], dest[1]);
			}
			
		}

	}
	ImGui::End();

}

void Editor::RenderDebugCube()
{
	

	
	Color color = Color(1, 0, 0, 0);

	for (uint i = 0; i < debugCubes.size(); i++)
	{
		Vector3  dest[8];


		Vector3 min = Vector3(-0.5f, -0.5f, -0.5f);
		Vector3 max = Vector3(0.5f, 0.5f, 0.5f);

		dest[0] = Vector3(min.x, min.y, max.z);
		dest[1] = Vector3(max.x, min.y, max.z);
		dest[2] = Vector3(min.x, max.y, max.z);
		dest[3] = Vector3(max);
		dest[4] = Vector3(min);
		dest[5] = Vector3(max.x, min.y, min.z);
		dest[6] = Vector3(min.x, max.y, min.z);
		dest[7] = Vector3(max.x, max.y, min.z);
		
		for (uint b = 0; b < 8; b++)
		{
			D3DXVec3TransformCoord(&dest[b], &dest[b], &debugCubes[i].world);
			
		}


		//Front
		DebugLine::Get()->RenderLine(dest[0], dest[1], color);
		DebugLine::Get()->RenderLine(dest[1], dest[3], color);
		DebugLine::Get()->RenderLine(dest[3], dest[2], color);
		DebugLine::Get()->RenderLine(dest[2], dest[0], color);

		//Backward
		DebugLine::Get()->RenderLine(dest[4], dest[5], color);
		DebugLine::Get()->RenderLine(dest[5], dest[7], color);
		DebugLine::Get()->RenderLine(dest[7], dest[6], color);
		DebugLine::Get()->RenderLine(dest[6], dest[4], color);

		//Side
		DebugLine::Get()->RenderLine(dest[0], dest[4], color);
		DebugLine::Get()->RenderLine(dest[1], dest[5], color);
		DebugLine::Get()->RenderLine(dest[2], dest[6], color);
		DebugLine::Get()->RenderLine(dest[3], dest[7], color);
	}
	
}

bool Compare(BoxRayStruct& a, BoxRayStruct& b)
{
	Vector3 size1 = a.max - a.min;
	Vector3 size2 = b.max - b.min;
	return size1.x < size2.x;
}
void Editor::BoxRayTracing()
{
	if (isMove)
		return;

	GlobalData::GetView(&v); GlobalData::GetProj(&p);
	

	Ray ray;
	tree->GetRay(&ray,  v, p);

	uint renderedStaticActorCount;
	engine->collider->ActorCount(renderedStaticActorCount);
	uint renderedSkeletalActorCount;
	engine->animator->ActorCount(renderedSkeletalActorCount);


	Color color = Color(0, 1, 0, 1);
	boxSort.clear();
	boxSort.shrink_to_fit();
	for (uint i = 0; i < renderedStaticActorCount; i++)
	{
		uint drawCount = engine->collider->DrawCount(i);
		for (uint d = 0; d < drawCount; d++)
		{
			vector<Vector3> dest = engine->collider->GetBoxMinMax(i, d);
			BoxRayStruct box;
			box.min = dest[4];
			box.max = dest[3];
			box.actorIndex = i;
			box.instanceIndex = d;
			boxSort.emplace_back(box);
		}
	}
	sort(boxSort.begin(), boxSort.end(), Compare);
	for (uint i = 0; i < boxSort.size(); i++)
	{
		if (IntersectionAABB(ray.org,ray.dir, pos, boxSort[i].min, boxSort[i].max))
		{
			clickedActorIndex = boxSort[i].actorIndex;
			clickedInstanceIndex = boxSort[i].instanceIndex;
			clickedMeshType = GizmoType::StaticMesh;
			controllParticleIndex = -1;
			lights->SetControllLightIndex(-1);
			IsClicked = true;
			return;
		}
	}
	

	for (uint i = 0; i < renderedSkeletalActorCount; i++)
	{
		uint drawCount = engine->animator->DrawCount(i);
		for (uint d = 0; d < drawCount; d++)
		{
			vector<Vector3> dest = engine->animator->GetBoxMinMax(i, d);
			

			if (IntersectionAABB(ray.org, ray.dir, pos, dest[4], dest[3]))
			{
				clickedActorIndex = i;
				clickedInstanceIndex = d;
				clickedMeshType = GizmoType::SkeletalMesh;
				controllParticleIndex = -1;
				lights->SetControllLightIndex(-1);
				IsClicked = true;
				return;
			}
			
		}


	}


	

}

bool Editor::IntersectionAABB(const Vector3 & org, const Vector3 & dir, Vector3 & Pos, const Vector3 & boundsMin, const Vector3 & boundsMax)
{
	float t_min = FLT_MIN;
	float t_max = FLT_MAX;


	for (int i = 0; i < 3; i++)
	{
		if (abs(dir[i]) < Math::EPSILON)
		{
			if (org[i] < boundsMin[i] ||
				org[i] >boundsMax[i])
			{

				return false;
			}

		}
		else
		{
			float denom = 1.0f / dir[i];
			float t1 = (boundsMin[i] - org[i]) * denom;
			float t2 = (boundsMax[i] - org[i]) * denom;

			if (t1 > t2)
			{
				swap(t1, t2);
			}

			t_min = max(t_min, t1);
			t_max = min(t_max, t2);

			if (t_min > t_max)
			{

				return false;
			}


		}
	}

	Vector3 hit = org + t_min * dir;



	//d = t_min;
	return true;
}

void Editor::ApplyStyle()
{
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();

	float fontSize = 15.0f;
	float roundness = 2.0f;
	ImVec4 white = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	ImVec4 text = ImVec4(0.76f, 0.77f, 0.8f, 1.0f);
	ImVec4 black = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	ImVec4 backgroundVeryDark = ImVec4(0.08f, 0.086f, 0.094f, 1.00f);
	ImVec4 backgroundDark = ImVec4(0.117f, 0.121f, 0.145f, 1.00f);
	ImVec4 backgroundMedium = ImVec4(0.26f, 0.26f, 0.27f, 1.0f);
	ImVec4 backgroundLight = ImVec4(0.37f, 0.38f, 0.39f, 1.0f);
	ImVec4 highlightBlue = ImVec4(0.172f, 0.239f, 0.341f, 1.0f);
	ImVec4 highlightBlueHovered = ImVec4(0.202f, 0.269f, 0.391f, 1.0f);
	ImVec4 highlightBlueActive = ImVec4(0.382f, 0.449f, 0.561f, 1.0f);
	ImVec4 barBackground = ImVec4(0.078f, 0.082f, 0.09f, 1.0f);
	ImVec4 bar = ImVec4(0.164f, 0.180f, 0.231f, 1.0f);
	ImVec4 barHovered = ImVec4(0.411f, 0.411f, 0.411f, 1.0f);
	ImVec4 barActive = ImVec4(0.337f, 0.337f, 0.368f, 1.0f);

	// Spatial
	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;
	style.FramePadding = ImVec2(5, 5);
	style.ItemSpacing = ImVec2(6, 5);
	style.Alpha = 1.0f;
	style.WindowRounding = roundness;
	style.FrameRounding = roundness;
	style.PopupRounding = roundness;
	style.GrabRounding = roundness;
	style.ScrollbarSize = 20.0f;
	style.ScrollbarRounding = roundness;

	// Colors
	style.Colors[ImGuiCol_Text] = text;
	style.Colors[ImGuiCol_WindowBg] = backgroundDark;
	style.Colors[ImGuiCol_Border] = black;
	style.Colors[ImGuiCol_FrameBg] = bar;
	style.Colors[ImGuiCol_FrameBgHovered] = highlightBlue;
	style.Colors[ImGuiCol_FrameBgActive] = highlightBlueHovered;
	style.Colors[ImGuiCol_TitleBg] = backgroundVeryDark;
	style.Colors[ImGuiCol_TitleBgActive] = bar;
	style.Colors[ImGuiCol_MenuBarBg] = backgroundVeryDark;
	style.Colors[ImGuiCol_ScrollbarBg] = barBackground;
	style.Colors[ImGuiCol_ScrollbarGrab] = bar;
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = barHovered;
	style.Colors[ImGuiCol_ScrollbarGrabActive] = barActive;
	style.Colors[ImGuiCol_CheckMark] = white;
	style.Colors[ImGuiCol_SliderGrab] = bar;
	style.Colors[ImGuiCol_SliderGrabActive] = barActive;
	style.Colors[ImGuiCol_Button] = barActive;
	style.Colors[ImGuiCol_ButtonHovered] = highlightBlue;
	style.Colors[ImGuiCol_ButtonActive] = highlightBlueActive;
	style.Colors[ImGuiCol_Header] = highlightBlue; // selected items (tree, menu bar etc.)
	style.Colors[ImGuiCol_HeaderHovered] = highlightBlueHovered; // hovered items (tree, menu bar etc.)
	style.Colors[ImGuiCol_HeaderActive] = highlightBlueActive;
	style.Colors[ImGuiCol_Separator] = backgroundLight;
	style.Colors[ImGuiCol_ResizeGrip] = backgroundMedium;
	style.Colors[ImGuiCol_ResizeGripHovered] = highlightBlue;
	style.Colors[ImGuiCol_ResizeGripActive] = highlightBlueHovered;
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.7f, 0.77f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = highlightBlue; // Also used for progress bar
	style.Colors[ImGuiCol_PlotHistogramHovered] = highlightBlueHovered;
	style.Colors[ImGuiCol_TextSelectedBg] = highlightBlue;
	style.Colors[ImGuiCol_PopupBg] = backgroundVeryDark;
	style.Colors[ImGuiCol_DragDropTarget] = backgroundLight;
}