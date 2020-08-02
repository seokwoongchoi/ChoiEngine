#include "stdafx.h"
#include "Editor.h"
#include "Core/Engine.h"
#include "./ImGui/imgui_impl_win32.h"
#include "./ImGui/imgui_impl_dx11.h"
#include "ImGui/ImGuizmo.h"

#include "ActorEditor/ActorEditor.h"
#include "PostEffects/HDR.h"
#include "PostEffects/SSLR.h"
#include "PostEffects/SSAO.h"
#include "EventSystems/Renderer.h"
#include "EventSystems/ColliderSystem.h"
#include "EventSystems/Animator.h"
#include "Utility/QuadTree.h"


Editor::Editor()
	:bStart(false), flags(0), IsPushed(false), pushedActorIndex(-1), pushedMeshType(ReadMeshType::Default),
	staticActorCount(0),skeletalActorCount(0), bShowCollider(false)
{
    engine= new Engine();


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigWindowsResizeFromEdges = true;
    io.ConfigViewportsNoTaskBarIcon = true;

	ImGui_ImplWin32_Init(D3D::GetDesc().Handle);

	
	ImGui_ImplDX11_Init(engine->device, engine->immediateContext);
	ApplyStyle();

  
	flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;


	{
		buttonTextures[0] = new Texture();
		buttonTextures[0]->Load(engine->device, L"playButton.png");
		buttonTextures[1] = new Texture();
		buttonTextures[1]->Load(engine->device, L"pauseButton.png");
		buttonTextures[2] = new Texture();
		buttonTextures[2]->Load(engine->device, L"stopButton.png");
	}
	tree = new QuadTree();

	DebugLine::Get()->Initiallize(engine->device);
	
	D3DXMatrixIdentity(&identity);
}

Editor::~Editor()
{


	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

  
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
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(D3D::Width()), static_cast<float>(D3D::Height())));

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
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoNavFocus
	);

	if (IsPushed)
	{
		static ImGuizmo::OPERATION operation(ImGuizmo::TRANSLATE);
		//static ImGuizmo::MODE mode(ImGuizmo::WORLD);
		static ImGuizmo::MODE mode(ImGuizmo::LOCAL);
		if (ImGui::IsKeyPressed('T'))//w
			operation = ImGuizmo::TRANSLATE;
		if (ImGui::IsKeyPressed('Y'))//e
			operation = ImGuizmo::ROTATE;
		if (ImGui::IsKeyPressed('U'))//r
			operation = ImGuizmo::SCALE;
		
		switch (pushedMeshType)
		{
		case ReadMeshType::StaticMesh:
		{
		
			Matrix gizmo = engine->collider->GetInstMatrix(pushedActorIndex);
			Matrix gizmoDelta;
			D3DXMatrixIdentity(&gizmoDelta);
		
			const	ImGuiIO& io = ImGui::GetIO();
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
			ImGuizmo::Manipulate(GlobalData::GetView(), GlobalData::GetProj(), operation, mode,
				gizmo, gizmoDelta, nullptr);

			bool isMove = false;
			if (gizmoDelta != identity)
			{
				isMove = true;
			}
			Vector3 ScaleLenth = Vector3(gizmoDelta._11 - 1.0f, gizmoDelta._22 - 1.0f, gizmoDelta._33 - 1.0f);
			float scaleL = D3DXVec3Length(&ScaleLenth);
			if (Math::Abs(scaleL) > 0.5f)
			{
				isMove = false;
			}
			if (isMove)
			{
				engine->collider->SetInstMatrix(pushedActorIndex,gizmo);
			}
			
		}
		break;
		case ReadMeshType::SkeletalMesh:
		{
			/*auto& renderer = engine->skeletalRenderer[pushedActorIndex];
			Matrix temp;
			D3DXMatrixTranspose(&temp, &renderer.RenderWorld());
			const	ImGuiIO& io = ImGui::GetIO();
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
			ImGuizmo::Manipulate(GlobalData::GetView(), GlobalData::GetProj(), operation, mode,
				temp, nullptr, nullptr);
			renderer.RenderWorld(temp);*/
		}
		break;
	
		}
		
		
	}
	
	ImGui::End();
	if (!bStart&&tree->Intersection(pos))
	{
		AddTrasform(pos);
	}

	engine->Update();

	
	
}

void Editor::Render()
{
	engine->Begin();
	engine->Render();
	
	MenuBar();
	ToolBar();
	for (auto& actor : actors)
	{
		actor->Render(engine->deferredContext[4]);
	}
	engine->deferredContext[4]->FinishCommandList(false, &engine->commadList[4]);
	engine->immediateContext->ExecuteCommandList(engine->commadList[4], true);
	SafeRelease(engine->commadList[4]);
	
	PostEffects();
	ActorAsset();

	if(bShowCollider)
	DebugRender();
	
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
		ImGui::SliderFloat("intensity", (float*)&sslrparms->intensity, 0.01f, 5.0f);
		ImGui::SliderFloat("decay", (float*)&sslrparms->decay, 0.01f, 10.0f);
		ImGui::SliderFloat("ddecay", (float*)&sslrparms->ddecay, 0.01f, 10.0f);
		//ImGui::SliderFloat("dist", (float*)&sslrparms->dist, 50.0f, 1000.0f);
		//ImGui::SliderFloat("MaxDeltaLen", (float*)&sslrparms->MaxDeltaLen, -1.0f, 5.0f);
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
}

void Editor::ActorAsset()
{
	if (bStart)return;

	ImGui::Begin("Assets", 0, flags);
	{
		static bool isEditActor = false;
		static vector< shared_ptr<ActorEditor>>::iterator saveIter;
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
				//HWND hWnd = NULL;
				//function<void(wstring)> f = bind(&Editor::Load, this, placeholders::_1);
			    //Path::OpenFileDialog(L"", Path::LevelFilter, L"../../_Levels/", f, hWnd);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save")) {}
			if (ImGui::MenuItem("Save as..."))
			{
				//HWND hWnd = NULL;
				//function<void(wstring)> f = bind(&Editor::Save, this, placeholders::_1);
				//Path::SaveFileDialog(L"", Path::LevelFilter, L"../../_Levels/", f, hWnd);
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
			bStart = true;
			if (Time::Get()->Stopped())
			{
				Time::Get()->Start();
			}
		}
	    ImGui::SameLine();
		if (ImGui::ImageButton(buttonTextures[1]->SRV(), ImVec2(20.0f, 20.0f)))//pause
		{
			bStart = false;
			
		}
		ImGui::SameLine();
		if (ImGui::ImageButton(buttonTextures[2]->SRV(), ImVec2(20.0f, 20.0f)))//stop
		{
			bStart = false;
			Time::Get()->Stop();
		}
		
		ImGui::Checkbox("Show ColliderBox", &bShowCollider);
		const string& str = string("Frame Rate : ") + to_string(ImGui::GetIO().Framerate);
		ImGui::Text(str.c_str());

		
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

			pushedActorIndex = actor->ActorIndex();
			pushedMeshType = actor->MeshType();
			engine->PusInstance(pushedActorIndex, previewWorld, pushedMeshType);

		//	uint boxCallIndex = actor->BoxCallIndex();
		//	DebugLine::Get()->SetInstPointer(engine->collider->GetInstMatrixPointer(pushedActorIndex), boxCallIndex,0);
		//	DebugLine::Get()->DrawCount(boxCallIndex);

			actor->IsDragged(false);
			IsPushed = true;
		}

	}

}

void Editor::DebugRender()
{
	uint renderedStaticActorCount = engine->collider->ActorCount();
	uint renderedSkeletalActorCount = engine->animator->ActorCount();

	Color color = Color(1, 0, 0, 1);
	for (uint i = 0; i < renderedStaticActorCount; i++)
	{
		uint darCount = engine->collider->DrawCount(i);
		for (uint d = 0; d < darCount; d++)
		{
			Vector3* dest = engine->collider->GetBoxMinMax(i, d);
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
		uint darCount = engine->animator->DrawCount(i);
		for (uint d = 0; d < darCount; d++)
		{
			Vector3* dest = engine->animator->GetBoxMinMax(i, d);
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
	DebugLine::Get()->Render(engine->immediateContext);

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