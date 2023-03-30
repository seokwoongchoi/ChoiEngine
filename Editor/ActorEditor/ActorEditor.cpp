#include "stdafx.h"
#include "ActorEditor.h"
#include "Core/Engine.h"
#include "PreviewRenderer.h"
#include "Resources/Mesh.h"
#include "Editor.h"
#include "EventSystems/ColliderSystem.h"
#include "EventSystems/Animator.h"
#include "EventSystems/Renderers/ShadowRenderer.h"
ActorEditor::ActorEditor(ID3D11Device* device, class Engine* engine, class Editor* editor):
	engine(engine),editor(editor),
	device(device),
	backBuffer(nullptr), rtv(nullptr),srv(nullptr), previewRender(nullptr), depthBackBuffer(nullptr),
	bActive(false),
    bEditing(false),
    bFirst(false),
    bDrag(false),
    bLoaded(false),
    bModelLoaded(false),
    bBone(false),
    bBlend(false),
    bCompiled(false),
    bHasEffect(false),
	bMove(false),
	bBindedTree(false),
	dsv(nullptr), meshType(ReadMeshType::Default), currentBone(nullptr), modelName(L""), actorIndex(0), size(800,600),
	currentClipNum(0), mode(EditMode::Render), camSpeed(0.8f),
	gizmoMode(GizmoMode::Default), gizmoColliderIndex(0)
{
	previewRender = new PreviewRenderer(device);
	uint width = D3D::Width();
	uint height = D3D::Height();
	auto format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(D3D11_TEXTURE2D_DESC));
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Format = format;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.SampleDesc.Count = 1;

	Check(device->CreateTexture2D(&textureDesc, nullptr, &backBuffer));

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	ZeroMemory(&rtvDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;

	Check(device->CreateRenderTargetView(backBuffer, &rtvDesc, &rtv));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	Check(device->CreateShaderResourceView(backBuffer, &srvDesc, &srv));


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

		Check(device->CreateTexture2D(&desc, NULL, &depthBackBuffer));
		
	}

	//Create DSV
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;

		Check (device->CreateDepthStencilView(depthBackBuffer, &desc, &dsv));
		


	}

	for (uint i = 0; i < 3; i++)
	{
		buttonTextures[i] = new Texture();

	}
	buttonTextures[0] ->Load(device,L"playButton.png", nullptr, true);
	buttonTextures[1]->Load(device, L"pauseButton.png", nullptr, true);
	buttonTextures[2]->Load(device, L"stopButton.png", nullptr, true);


	windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove| ImGuiWindowFlags_NoScrollbar;

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	{
		D3DXMatrixIdentity(&gizmoWorld);
		D3DXMatrixIdentity(&identity);
		gizmoSnap = Vector3(0.0001f, 0.0001f, 0.0001f);

		D3DXMatrixIdentity(&gizmoDelta);
	}


	

}

ActorEditor::~ActorEditor()
{
	switch (meshType)
	{

	case ReadMeshType::StaticMesh:
		editor->staticActorCount--;
		break;
	case ReadMeshType::SkeletalMesh:
		editor->skeletalActorCount--;
		break;
	}
}

void ActorEditor::Save()
{
	previewRender->SaveMaterialFile(modelName);
	previewRender->SaveMeshFile(modelName, meshType);
}

void ActorEditor::Save(BinaryWriter * w)
{
	Thread::Get()->AddTask([&]() {

		Save();

	});

	
	uint actorCount = 0;
	switch (meshType)
	{

	case ReadMeshType::StaticMesh:
	{
		actorCount = editor->staticActorCount;
	}
	break;
	case ReadMeshType::SkeletalMesh:
	{
		actorCount = editor->skeletalActorCount;
	}
	break;
	}
	w->UInt(actorIndex);
	w->UInt(actorCount);
	w->UInt(static_cast<uint>(meshType));
	w->String(String::ToString(modelName));


	switch (meshType)
	{

	case ReadMeshType::StaticMesh:
	{
		uint drawCount=	engine->collider->DrawCount(actorIndex);
		w->UInt(drawCount);
		for (uint i = 0; i < drawCount; i++)
		{
			Matrix temp;
			engine->collider->GetInstMatrix(&temp,actorIndex, i);
			w->Matrix(temp);
		}
	}
	break;
	case ReadMeshType::SkeletalMesh:
	{
		uint drawCount= engine->animator->DrawCount(actorIndex);
		
		w->UInt(drawCount);
		for (uint i = 0; i < drawCount; i++)
		{
			Matrix temp;
			engine->animator->GetInstMatrix(&temp, actorIndex, i);
			
			w->Matrix(temp);
		}
	}
	break;
	}
	

}

void ActorEditor::Load(BinaryReader * r)
{
	uint temp = r->UInt();
	 temp = r->UInt();
	uint tempmeshType = r->UInt();
	string tempModelName = r->String();
	wstring tempwString = String::ToWString(tempModelName);

	uint actorCount = 0;
	
	switch (tempmeshType)
	{
	case 1:
		LoadStaticMesh(L"../_Models/StaticMeshes/" + tempwString + L"/" + tempwString + L"_Edit" + L".mesh");
		actorCount = editor->staticActorCount;
		break;
	case 2:
		LoadSkeletalMesh(L"../_Models/SkeletalMeshes/" + tempwString + L"/" + tempwString + L"_Edit" + L".mesh");
		actorCount = editor->skeletalActorCount;
		break;
	
	}
	 
	engine->Load(actorIndex, modelName, actorCount, meshType);

	uint drawCount = r->UInt();
	if(drawCount>0)
	switch (tempmeshType)
	{
	case 1:
		for (uint i = 0; i < drawCount; i++)
		{
			Matrix temp = r->Matrix();
			engine->collider->PushDrawCount(actorIndex, temp, true);
			engine->staticShadowRenderer[actorIndex].PushInstance();
		}
		break;
	case 2:
		for (uint i = 0; i < drawCount; i++)
		{
			Matrix temp = r->Matrix();
			engine->animator->PushDrawCount(actorIndex, temp);
			engine->skeletalShadowRenderer[actorIndex].PushInstance();
		}
		break;

	}

	bMove = true;


}

void ActorEditor::Render(ID3D11DeviceContext* context)
{
	
	if (!bModelLoaded|| !bEditing) return;
	
		previewRender->Update(Vector2(size.x,size.y), camSpeed);
		if (meshType == ReadMeshType::SkeletalMesh)
		{
			previewRender->AnimationUpdate(context);
		}

		
		context->ClearRenderTargetView(rtv, Color(0.5f, 0.6f, 0.3f, 1.0f));
		context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
		context->OMSetRenderTargets(1, &rtv, dsv);

		previewRender->Render(context);
	
	

}

void ActorEditor::SetDragDropPayload(const string & data)
{
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
	{
		bDrag = true;
		ImGui::Text(Path::GetFileNameWithoutExtension(data).c_str());
		ImGui::EndDragDropSource();
	}
}

const Matrix & ActorEditor::PreviewWorld()
{
	return previewRender->previewDesc.W;
}

void ActorEditor::BarrierModelUse()
{
	bCompiled = false;
	bMove = false;
	bModelLoaded = false;
}

bool ActorEditor::ImageButton()
{
	
	if (ImGui::ImageButton(srv, ImVec2(80, 80)))
	{
		
		bEditing = true;
	}
	ImGui::SameLine();
	if (ImGui::IsItemHovered())
	{
		return true;
	}

	SetDragDropPayload(String::ToString(modelName));

	Editor();
	ShowHierarchy();
	EditingMode();
	if (bCompiled)
	{
	
		uint actorCount = 0;
		switch (meshType)
		{
		
		case ReadMeshType::StaticMesh:
			actorCount = editor->staticActorCount;
			break;
		case ReadMeshType::SkeletalMesh:
			actorCount = editor->skeletalActorCount;
			break;
		
		}
		engine->Load(actorIndex, modelName, actorCount, meshType);
		bCompiled = false;
		bMove = true;
		
		
		
	}

	return false;
	
}

void ActorEditor::EditingMode()
{
	if (!bEditing ) return ;

	

	ImGui::Begin("EditingMode", &bEditing);
	{
		if (ImGui::Button("Aniamtor", ImVec2(80, 30)) /*&& model&&model->ClipCount() > 0*/)
		{
			mode = EditMode::Animator;
		}
		ImGui::SameLine(100);
		if (ImGui::Button("Render", ImVec2(80, 30)))
		{
			mode = EditMode::Render;


		}
		ImGui::SameLine(190);


		if (ImGui::Button("Compile", ImVec2(80, 30)))
		{
			Compile();
		}
	
	}
	ImGui::End();

	
}

void ActorEditor::Compile()
{
	
	Thread::Get()->AddTask([&]() {

		
		Save();
		bCompiled = true;
	});

}

void ActorEditor::Editor()
{
	if (!bEditing) return;


	ImGui::SetNextWindowSizeConstraints
	(
		ImVec2(800, 600),
		ImVec2(static_cast<float>(D3D::Width()), static_cast<float>(D3D::Height()))
	);


	ImGui::Begin("Editor", &bEditing, windowFlags);
	{
		ImGui::SliderFloat("CamSpeed", reinterpret_cast<float*>(&camSpeed), 0.1f, 5.0f);
		ImVec2 size = ImGui::GetWindowSize();
	
		
		ShowFrame(size);
		
		if (mode == EditMode::Animator)
		{
			ShowAnimList(size);

		}
		else if (mode == EditMode::Render)
		{
			ShowComponents(size);
			ShowMaterial(size);
		}
	}
	ImGui::End();
}

void ActorEditor::ShowFrame(const ImVec2 & size)
{
	ImGui::BeginChild("##Frame", ImVec2(size.x*0.5f, 0), true, windowFlags);
	{
		
		ImGui::Image
		(
			srv, ImVec2(size.x*0.5f, size.y)
		);
		this->size = ImVec2(size.x*0.5f, size.y);
		
		ImGizmo();
	}
	ImGui::EndChild();
}

void ActorEditor::LoadBehaviorTree()
{
	previewRender->behaviorTreeIndex = 0;
	bBindedTree = true;
}

void ActorEditor::ImGizmo()
{
	
	if (!bEditing) return;

	editor->IsClicked = false;

	static ImGuizmo::OPERATION operation(ImGuizmo::TRANSLATE);
	//static ImGuizmo::MODE mode(ImGuizmo::WORLD);
	static ImGuizmo::MODE mode(ImGuizmo::LOCAL);
	if (ImGui::IsKeyPressed('T'))//w
		operation = ImGuizmo::TRANSLATE;
	if (ImGui::IsKeyPressed('Y'))//e
		operation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed('U'))//r
		operation = ImGuizmo::SCALE;
	
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, size.x, size.y);
	
	switch (gizmoMode)
	{
	case GizmoMode::Default:
		break;
	case GizmoMode::SkeletalBone:
		SkeletalGizmo(mode, operation);
		break;
	case GizmoMode::StaticBone:
		StaticGizmo(mode, operation);
		break;
	case GizmoMode::Collider:
	{
	;
	
		
	   ColliderGizmo(mode, operation, gizmoColliderIndex);
		
	}
		
		break;
	case GizmoMode::Effect:
		break;
	}
	
	
	
}

bool ActorEditor::PreviewGizmoSet(ImGuizmo::MODE mode, ImGuizmo::OPERATION operation)
{
	
	ImGuizmo::Manipulate(previewRender->view, previewRender->proj, operation, mode,
		gizmoWorld, gizmoDelta, &gizmoSnap[0]);

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
	return isMove;
}

void ActorEditor::SkeletalGizmo(ImGuizmo::MODE mode, ImGuizmo::OPERATION operation)
{

	if (!currentBone) return;
	{
		

		const auto& index = currentBone->index;
		Matrix skinnedMatrix = previewRender->GetSkinnedMatrix(index);

		
		D3DXMatrixTranspose(&previewWorld, &previewRender->previewDesc.W);
		gizmoWorld = skinnedMatrix * previewWorld;
	
		bool isMove = PreviewGizmoSet(mode, operation);

		if (isMove&&previewRender->bPause&&currentBone->BoneIndex() > 0)
		{
			auto& bones = previewRender->bones;
			Matrix local = bones[index]->Transform();
			Matrix invLocal;
			D3DXMatrixInverse(&invLocal, nullptr, &local);

			//Matrix animMatrix = previewRender->GetCurrentFrameAnimMatrix(index);
			//Matrix invAnim;
			//D3DXMatrixInverse(&invAnim, nullptr, &animMatrix);
			
			Matrix parent = invLocal * skinnedMatrix * previewWorld;
			Matrix parentInv;
			D3DXMatrixInverse(&parentInv, nullptr, &parent);

			Matrix newLocal = gizmoWorld * parentInv;
			previewRender->bones[index]->transform = newLocal;


			//Matrix newAnim = gizmoWorld * parentInv;
			//previewRender->SetCurrentFrameData(index, newAnim);
			previewRender->UpdateCurrFameAnimTransformSRV();


		}

	}

}

void ActorEditor::StaticGizmo(ImGuizmo::MODE mode, ImGuizmo::OPERATION operation)
{
   if (!currentBone) return;
	{


		const auto& index = currentBone->index;
	
		Matrix BoneMatrix = currentBone->Transform();

		D3DXMatrixTranspose(&previewWorld, &previewRender->previewDesc.W);
		gizmoWorld = BoneMatrix * previewWorld;

		bool isMove = PreviewGizmoSet(mode, operation);

		if (isMove&&previewRender->bPause&&currentBone->BoneIndex() > 0)
		{
			Matrix invWorld;
			D3DXMatrixInverse(&invWorld, nullptr, &previewWorld);
		
			Matrix newLocal = gizmoWorld * invWorld;
			previewRender->bones[index]->transform = newLocal;
			previewRender->CreateModelTransformSRV();


		}

	}
}

void ActorEditor::ColliderGizmo(ImGuizmo::MODE mode, ImGuizmo::OPERATION operation, const int& colliderIndex)
{


	if(colliderIndex<0)
	gizmoWorld = previewRender->boxWorld;
	else
	{
		gizmoWorld = previewRender->colliderBoxData[colliderIndex].ColliderBoxWorld;
	}

	bool isMove = PreviewGizmoSet(mode, operation);

	if (isMove)
	{
		if (colliderIndex < 0)
			previewRender->boxWorld = gizmoWorld;
		else
		{
			previewRender->colliderBoxData[colliderIndex].ColliderBoxWorld=	gizmoWorld;
		}
		
	}


}

void ActorEditor::ShowTransfomrs()
{
	if (bModelLoaded)
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Matrix world;
			D3DXMatrixTranspose(&world, &previewRender->previewDesc.W);

			Quaternion rotation;
			Vector3 pos, scale;
			D3DXMatrixDecompose(&scale, &rotation, &pos, &world);

			const auto show_float = [](const char* id, const char* label, float* value)
			{
				ImGui::PushItemWidth(100.0f);
				ImGui::PushID(id);
				ImGui::InputFloat(label, value, 1.0f, 1.0f, "%.3f", ImGuiInputTextFlags_CharsDecimal);
				ImGui::PopID();
				ImGui::PopItemWidth();
			};
			if (currentBone)
			{
				if (currentBone->BoneIndex() > 0)
				{
					auto local = currentBone->transform;
					Vector3 position = Vector3(local._41, local._42, local._43);
				
					ImGui::TextUnformatted("BoneTranslation");
					show_float("##pos_x", "X", &position.x);
					show_float("##pos_y", "Y", &position.y);
					show_float("##pos_z", "Z", &position.z);
					float length=D3DXVec3Length(&(position - Vector3(local._41, local._42, local._43)));
					if (Math::Abs(length)>0.0f)
					{
						Matrix S, R, T;
						Vector3 position1, scale;
						Quaternion q;
						D3DXMatrixDecompose(&scale, &q, &position1, &local);
						D3DXMatrixScaling(&S, scale.x, scale.y, scale.z);
						D3DXMatrixRotationQuaternion(&R, &q);
						D3DXMatrixTranslation(&T, position.x, position.y, position.z);
						previewRender->bones[currentBone->index]->transform = S * R*T;
						switch (meshType)
						{
						case ReadMeshType::StaticMesh:
							previewRender->CreateModelTransformSRV();
							break;
						case ReadMeshType::SkeletalMesh:
							previewRender->UpdateCurrFameAnimTransformSRV();
							//previewRender->CreateAnimTransformSRV();
							break;
					
						}
						
					}
					
				}



			}

			ImGui::TextUnformatted("Rotation");
			show_float("##rot_x", "X", &rotation.x);
			show_float("##rot_y", "Y", &rotation.y);
			show_float("##rot_z", "Z", &rotation.z);
			ImGui::TextUnformatted("Scale");
			show_float("##scl_x", "X", &scale.x);
			show_float("##scl_y", "Y", &scale.y);
			show_float("##scl_z", "Z", &scale.z);

			Matrix S, R, T;
			D3DXMatrixScaling(&S, scale.x, scale.y, scale.z);
			D3DXMatrixTranslation(&T, pos.x, pos.y, pos.z);
			D3DXMatrixRotationQuaternion(&R, &rotation);
			Matrix W = S * R*T;
			D3DXMatrixTranspose(&previewRender->previewDesc.W, &W);
		}
}

void ActorEditor::ShowAnimList(const ImVec2 & size)
{
	if (clipList.empty()) return;

	ImGui::SameLine();
	ImGui::BeginChild("##ClipList", ImVec2(size.x*0.5f - 70.0f, 0), true, windowFlags);
	{


		if (ImGui::CollapsingHeader("ClipList", ImGuiTreeNodeFlags_DefaultOpen))
		{
			wstring clipName = L"N/A";
			ImGui::Columns(1, "my", false);
			ImGui::Separator();
			{
				for (uint i = 0; i < clipList.size(); i++)
				{
					string name =String::ToString( clipList[i]);
					const char* label = name.c_str();
					bool bSelected = clipName == clipList[i];
					if (ImGui::Selectable(label, bSelected, 0, ImGui::CalcTextSize(label)))
					{
						clipName = clipList[i];
						currentClipNum = i;
						previewRender->tweenDesc.Next.Clip = i;


					}
					ImGui::NextColumn();

				}
				ImGui::Columns(1);
				ImGui::Separator();
				//ImGui::ListBoxFooter();
			}
		}
		if (ImGui::CollapsingHeader("Frame", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ShowAnimFrame(size);
		}

	}

	ImGui::EndChild();
}

void ActorEditor::ShowAnimFrame(const ImVec2 & size)
{
	const auto& clip = previewRender->clips[currentClipNum];
	
	uint frame = previewRender->tweenDesc.Curr.CurrFrame;
	ImGui::SliderInt("Frame", (int*)&frame, 0, clip->FrameCount() - 1);
	previewRender->tweenDesc.Curr.CurrFrame = frame;
	
	if (ImGui::ImageButton(buttonTextures[0]->SRV(), ImVec2(20.0f, 20.0f)))//play
	{
		previewRender->bPause = false;
		
	}
	ImGui::SameLine();
	if (ImGui::ImageButton(buttonTextures[1]->SRV(), ImVec2(20.0f, 20.0f)))//pause
	{
		previewRender->bPause = true;
	}
	ImGui::SameLine();
	if (ImGui::ImageButton(buttonTextures[2]->SRV(), ImVec2(20.0f, 20.0f)))//stop
	{

	}
	ImGui::SameLine();
	if (ImGui::Button("Update AllFrame", ImVec2(80.0f, 30.0f)))
	{
		Thread::Get()->AddTask([&]()
		{	
			previewRender->CreateAnimTransformSRV();
		});
	}
	
}

void ActorEditor::ShowComponents(const ImVec2 & size)
{

	if (previewRender->boxCount > 0&& componentList.size()<=2)
	{
		for(uint i=0;i< previewRender->boxCount;i++)
		componentList.emplace_back("BoneCollider" + to_string(i));
	}
	ImGui::SameLine();
	ImGui::BeginChild("##Components", ImVec2(size.x*0.25f - 70.0f, 0), true);
	{

		if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button("Add Component"))
			{
				ImGui::OpenPopup("Component Popup");
			}
			ShowComponentPopUp();

			//ImVec2 listBoxSize = ImVec2(ImGui::GetWindowContentRegionMax().x * 10.0f, 30.0f *componentList.size());


			ImGui::Columns(1, "mycolumns3", false);  // 3-ways, no border
			ImGui::Separator();
			//	ImGui::ListBoxHeader("##ComponentList", listBoxSize);
			{
				for (uint i = 0; i < 10; i++)
				{
					if (componentList.size() > i)
					{
						const string& name = componentList[i];
						const char* label = name.c_str();

						ImGuiTreeNodeFlags  flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

						if (ImGui::TreeNodeEx(label, flags))
						{
							if (ImGui::IsItemClicked())
							{
								componentName = componentList[i];
								if (componentName == "Collider")
								{
									gizmoColliderIndex = -1;
									gizmoMode = GizmoMode::Collider;
								}
								for (uint i = 0; i < MAX_ACTOR_BONECOLLIDER; i++)
								{
									if (componentName == "BoneCollider"+to_string(i))
									{
										gizmoColliderIndex = i;
										gizmoMode = GizmoMode::Collider;
									}
								}
								
								
								/*else if (componentName == "Effect0")
								{
									gizmoType = GizmoType::Effect1;
								}
								else if (componentName == "Effect1")
								{
									gizmoType = GizmoType::Effect2;
								}
								else if (componentName == "ActorCamera")
								{
									gizmoType = GizmoType::ActorCamera;
								}*/
								
							}
							if (bBindedTree&&componentList[i] == "ActorAI")
							{
								string treeName = "BehaviorTree" + to_string(previewRender->behaviorTreeIndex);
								if (ImGui::TreeNodeEx(treeName.c_str(), flags))
								{
									ImGui::TreePop();
								}
							}
							ImGui::TreePop();
						}

					}
					else
					{
						//string name ="";
						const char* label = "";
						//bSelected = componentName == componentList[i];
						//index = i;
						if (ImGui::Selectable(label)) {}
					}

					ImGui::NextColumn();
				}
				if (!componentName.empty() && ImGui::IsWindowHovered() && ImGui::IsAnyItemHovered())
					if (ImGui::GetIO().MouseDown[1])
						ImGui::OpenPopup("ComponentList Popup");
				ShowComponentListPopUp(componentName);
				ImGui::Columns(1);
				ImGui::Separator();
				//ImGui::ListBoxFooter();

			}

			ShowTransfomrs();

		}
	}

	ImGui::EndChild();
}

void ActorEditor::ShowComponentPopUp()
{
	if (ImGui::BeginPopup("Component Popup"))
	{
		if (ImGui::MenuItem("SkeletalMesh"))
		{
			HWND hWnd = NULL;
			function<void(wstring)> f = bind(&ActorEditor::LoadSkeletalMesh, this, placeholders::_1);
			Path::OpenFileDialog(L"", Path::MeshFilter, L"../_Models/SkeletalMeshes/", f, hWnd);
		}
		if (ImGui::MenuItem("StaticMesh"))
		{
			HWND hWnd = NULL;
			function<void(wstring)> f = bind(&ActorEditor::LoadStaticMesh, this, placeholders::_1);
			Path::OpenFileDialog(L"", Path::MeshFilter, L"../_Models/StaticMeshes/", f, hWnd);
		}
		if (ImGui::MenuItem("ActorCamera"))
		{
			auto allTrue = all_of (componentList.begin(), componentList.end(), [](const string& check)
			{
				return check != "ActorCamera";
			});

			if (allTrue)
			{
				componentList.emplace_back("ActorCamera");
			}
		


		}
		if (ImGui::MenuItem("ActorAI"))
		{
			auto allTrue = all_of(componentList.begin(), componentList.end(), [](const string& check)
			{
				return check != "ActorAI";
			});

			if (allTrue)
			{
				componentList.emplace_back("ActorAI");
			}
			  
		}

		ImGui::EndPopup();
	}
}

void ActorEditor::ShowComponentListPopUp(const string & componentName)
{
	if (ImGui::BeginPopup("ComponentList Popup"))
	{
		if (ImGui::MenuItem("Delete"))
		{
			if (componentName == "ActorCamera")
			{

				for (uint i = 0; i < componentList.size(); i++)
				{
					if (componentList[i] == "ActorCamera")
					{
						componentList.erase(componentList.begin() + i);
					}
				}
	


			}

			for (uint b = 0; b < MAX_ACTOR_BONECOLLIDER; b++)
			{
				if (componentName == "BoneCollider" + to_string(b))
				{
					for (uint i = 0; i < componentList.size(); i++)
					{
						if (componentList[i] == "BoneCollider" + to_string(b))
						{
							componentList.erase(componentList.begin() + i);
							if (previewRender->boxCount > 0)
							{
								previewRender->boxCount--;
								ColliderBoxData temp;
								D3DXMatrixIdentity(&temp.ColliderBoxWorld);
								previewRender->colliderBoxData[b] = temp;
							}
						}
					}

				}
			}


		}
		if (componentName == "ActorAI")
		{
			ImGui::Separator();
			//if (ImGui::MenuItem("BehaviorTree"))
			{

				uint count = static_cast<uint>(editor->behaviorTrees.size());
				for (uint i = 0; i < count; i++)
				{

					const string& temp = "BehaviorTree" + to_string(i);
					if (ImGui::MenuItem(temp.c_str()))
					{
						previewRender->behaviorTreeIndex = i;
						bBindedTree = true;
					}

				}


			}
		}

		ImGui::EndPopup();
	}
}

void ActorEditor::ClipFinder(const wstring & file)
{
	wstring filePath = L"../_Models/SkeletalMeshes/" + file + L"/";
	vector<wstring> files;

	wstring filter = L"*.clip";
	Path::GetFiles(&files, filePath, filter, false);


	for (uint i = 0; i < files.size(); i++)
	{
		auto fileName = Path::GetFileName(files[i]);
		wstring noExfileName = Path::GetFileNameWithoutExtension(files[i]);
		clipList.emplace_back(noExfileName);
	}
}

void ActorEditor::LoadSkeletalMesh(const wstring & file)
{
	if (bModelLoaded) return;

	BarrierModelUse();
	
	previewRender->CreateSahders("../_Shaders/PreviewSkeletalModel.hlsl");
	
	modelName = Path::GetFileNameWithoutExtension(file);

	
	
	meshType = ReadMeshType::SkeletalMesh;

	
	size_t index = modelName.find_last_of('_');
	if (index == string::npos)
	{
		previewRender->ReadMaterial(modelName);
		previewRender->ReadMesh(file, modelName,meshType);
	}
	else if (modelName.substr(index + 1, 4) == L"Edit")
	{
		modelName = modelName.substr(0, index);
		previewRender->ReadEditedMaterial(modelName);
		previewRender->ReadEditedMesh(file, modelName, meshType);
	}


	ClipFinder(modelName);
	componentList.emplace_back("SkeletalMesh");
	componentList.emplace_back("Collider");
	actorIndex = editor->skeletalActorCount++;

	bModelLoaded = true;


}

void ActorEditor::LoadStaticMesh(const wstring & file)
{
	if (bModelLoaded) return;

	BarrierModelUse();

	
	modelName = Path::GetFileNameWithoutExtension(file);
	

	meshType = ReadMeshType::StaticMesh;

	
	size_t index = modelName.find_last_of('_');

	if (index==string::npos)
	{
		previewRender->ReadMaterial(modelName);
		previewRender->ReadMesh(file, modelName, meshType);
	}
	else if (modelName.substr(index + 1, 4) == L"Edit")
    {
		modelName = modelName.substr(0, index);
    	previewRender->ReadEditedMaterial(modelName);
    	previewRender->ReadEditedMesh(file, modelName, meshType);
    }
  
    	
	previewRender->CreateSahders("../_Shaders/PreviewStaticModel.hlsl");
	

	componentList.emplace_back("StaticMesh");
	componentList.emplace_back("Collider");

	//if (previewRender->boxCount > 0)
	//{

	//}

	//	
	//

	// currentBone->BoneIndex();
	//previewRender->boxCount = boxIndex + 1;
	//previewRender->colliderBoxData[boxIndex].Index = colliderIndex[boxIndex];



	//componentList.emplace_back("BoneCollider" + to_string(boxIndex));
	actorIndex = editor->staticActorCount++;


	bModelLoaded = true;
	
}

void ActorEditor::LoadMaterial(function<void(wstring, uint, shared_ptr<class Material>)> f, uint num, shared_ptr<class Material> material)
{
	HWND hWnd = NULL;

	f = bind(&ActorEditor::SetMaterial, this, placeholders::_1, placeholders::_2, placeholders::_3);
	auto path = L"../_Textures/" + modelName;
	Path::OpenFileDialog(L"", Path::EveryFilter, path, num, material, f, hWnd);
}

void ActorEditor::SetMaterial(wstring & file, uint textureType, shared_ptr<class Material> material)
{
	switch (textureType)
	{
	case 0:
	{
		material->DiffuseMap(file);
	}
	break;
	case 1:
	{
		material->NormalMap(file);
	}
	break;
	case 2:
	{
		material->RoughnessMap(file);
	}
	break;
	case 3:
	{
		material->MetallicMap(file);
	}
	/*case 4:
	{
		material->HeightMap(file);
	}*/
	break;
	}
}

void ActorEditor::ShowMaterial(const ImVec2 & size)
{
	
	
	
	
	shared_ptr<class Material> material = nullptr;
	const auto ShowTextureSlot = [&](Texture& texture, const char* name,uint MaterialType)
	{
		ImGui::Text(name);
		ImGui::SameLine(70.0f);
		function<void(wstring, uint, shared_ptr<class Material>)> f;

		if(ImGui::ImageButton
		(
			texture ? texture.SRV() : nullptr,
			ImVec2(80, 80)

		))
		{
			LoadMaterial(f, MaterialType, material);
		}
		
		
	};

	ImGui::SameLine();
	ImGui::BeginChild("##Edit", ImVec2(size.x*0.25f + 30.0f, 0), true);
	{
		auto& count = previewRender->meshCount;
		auto& meshes = previewRender->meshes;
		for (uint i=0;i< count;i++)
		{
			if (previewRender->blendCount>0&&i == previewRender->blendMeshIndex) continue;
			auto& mesh = meshes[i];
			if (mesh)
			{
				string& meshName = mesh->name;
				material = mesh->material;
				if (material&&ImGui::CollapsingHeader(meshName.c_str(), ImGuiTreeNodeFlags_OpenOnDoubleClick))
				{
					string& materialName = "MaterialName:" + String::ToString(material->Name());
					ImGui::Text(materialName.c_str());

					string& boneIndex = "BoneIndex:" + to_string(mesh->boneDesc.boneIndex);
					ImGui::Text(boneIndex.c_str());
					string& vertexCount = "VertexCount:" + to_string(mesh->vertexCount);
					ImGui::Text(vertexCount.c_str());
					string& startVertexIndex = "StartVertexIndex:" + to_string(mesh->startVertexIndex);
					ImGui::Text(startVertexIndex.c_str());

					//Albedo
					ShowTextureSlot(material->DiffuseMap(), "Albedo", 0);
					string& fileName = String::ToString(material->DiffuseFile());
					ImGui::Text(fileName.c_str());

					ImGui::Text("Color");
					ImGui::SameLine(70.0f);
					Color diffuse = material->Diffuse();
					string tag = "##ColorAlbedo" + to_string(i);
					ImGui::ColorEdit4(tag.c_str(), diffuse);
					material->Diffuse(diffuse);
					ImGui::Separator();

					//Normal
					ShowTextureSlot(material->NormalMap(), "Normal", 1);
					string& normalName = String::ToString(material->NormalFile());
					ImGui::Text(normalName.c_str());
					ImGui::Separator();

					//Roughness
					ShowTextureSlot(material->RoughnessMap(), "Roughness", 2);
					string& roughnessName = String::ToString(material->RoughnessFile());
					ImGui::Text(roughnessName.c_str());

					ImGui::Text("Factor");
					ImGui::SameLine(70.0f);
					float roughness = material->Roughness();
					tag = "##Roughness" + to_string(i);
					ImGui::SliderFloat(tag.c_str(), &roughness, 0.0f, 2.0f);
					material->Roughness(roughness);
					ImGui::Separator();

					//Metallic
					ShowTextureSlot(material->MetallicMap(), "Metallic", 3);
					string& metaillcName = String::ToString(material->MetallicFile());
					ImGui::Text(metaillcName.c_str());

					ImGui::Text("Factor");
					ImGui::SameLine(70.0f);
					float metallic = material->Metallic();
					tag = "##Metallic" + to_string(i);
					ImGui::SliderFloat(tag.c_str(), &metallic, 0.0f, 2.0f);
					material->Metallic(metallic);

					////Height
					//ShowTextureSlot(material->HeightMap(), "Height", 4);
					//string& heightName = String::ToString(material->HeightFile());
					//ImGui::Text(heightName.c_str());

				
				}
				ImGui::Separator();
			}
		
		}
		for (auto& mesh : previewRender->blendMeshes)
		{
    		string& meshName = mesh->name;
			material = mesh->material;
			if (material&&ImGui::CollapsingHeader(meshName.c_str(), ImGuiTreeNodeFlags_OpenOnDoubleClick))
			{
				string& boneIndex = "BoneIndex:" + to_string(mesh->boneDesc.boneIndex);
				ImGui::Text(boneIndex.c_str());
				string& vertexCount = "VertexCount:" + to_string(mesh->vertexCount);
				ImGui::Text(vertexCount.c_str());
				string& startVertexIndex = "StartVertexIndex:" + to_string(mesh->startVertexIndex);
				ImGui::Text(startVertexIndex.c_str());

				//Albedo
				ShowTextureSlot(material->DiffuseMap(), "Albedo", 0);
				ImGui::Text("Color");
				ImGui::SameLine(70.0f);
				Color diffuse = material->Diffuse();
				ImGui::ColorEdit4("##ColorAlbedo", diffuse);
				material->Diffuse(diffuse);
				ImGui::Separator();

				//Normal
				ShowTextureSlot(material->NormalMap(), "Normal", 1);

				ImGui::Separator();

				//Roughness
				ShowTextureSlot(material->RoughnessMap(), "Roughness", 2);
				ImGui::Text("Factor");
				ImGui::SameLine(70.0f);
				float roughness = material->Roughness();
				ImGui::SliderFloat("##Roughness", &roughness, 0.0f, 1.0f);
				material->Roughness(roughness);
				ImGui::Separator();

				//Metallic
				ShowTextureSlot(material->MetallicMap(), "Metallic", 3);
				ImGui::Text("Factor");
				ImGui::SameLine(70.0f);
				float metallic = material->Metallic();
				ImGui::SliderFloat("##Metallic", &metallic, 0.0f, 1.0f);
				material->Metallic(metallic);
			}
			ImGui::Separator();
		}
		
	}
	ImGui::EndChild();
}

void ActorEditor::ShowHierarchy()
{
	if (!bEditing) return;


	ImGui::Begin("Hierarchy", &bEditing, ImGuiWindowFlags_HorizontalScrollbar| ImGuiWindowFlags_NoMove);
	{
		if (previewRender->bLoaded)
		{
			const auto& bone = previewRender->bones;
			const uint& count = previewRender->boneCount;

			
			switch (meshType)
			{
			case ReadMeshType::StaticMesh:
			{
				for (uint i = 0; i < count; i++)
				{
				   ShowStaticBones(bone[i]);
				}
			}
			break;
			case ReadMeshType::SkeletalMesh:
			{
				shared_ptr<class ModelBone> root = nullptr;
				for (uint i = 0; i < count; i++)
				{
					
					if (bone[i] == nullptr) break;

					if (bone[i]->ChildsData())
					{
						root = bone[i];
						break;
					}
				}
				if(root!=nullptr)
				ShowSkeletalBones(root);
        	}
			break;
			}
		}
		
		
		

		if (currentBone&&ImGui::IsWindowHovered() && ImGui::IsAnyItemHovered())
		{
			
			if (ImGui::GetIO().MouseDown[1])
				ImGui::OpenPopup("Hierarchy Popup");
		}
		ShowHierarcyPopup();

	}
	ImGui::End();
}

void ActorEditor::ShowStaticBones(shared_ptr<class ModelBone> bone)
{

	{

		ImGuiTreeNodeFlags  flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		static string temp = "";
		if (temp == bone->Name())
			flags |= ImGuiTreeNodeFlags_Selected;


		ImGui::PushItemWidth(1.0f);

		if (ImGui::TreeNodeEx(bone->Name().c_str(), flags))
		{
			/*string& boneIndex = "BoneIndex :" + to_string(bone->index);
			ImGui::Text(boneIndex.c_str());
			string& parentBoneIndex = "ParentBoneIndex :" + to_string(bone->parentIndex);
			ImGui::Text(parentBoneIndex.c_str());*/
			if (ImGui::IsItemClicked())
			{
				currentBone = bone;
				if (currentBone)
				{
					gizmoMode = GizmoMode::StaticBone;
				}
				temp = bone->Name();
			
				
			}
			ImGui::TreePop();

		}
		
		
		ImGui::PopItemWidth();
	}


}

void ActorEditor::ShowSkeletalBones(shared_ptr<class ModelBone> bone)
{
	vector<shared_ptr<ModelBone>> childs = bone->GetChilds();
	ImGuiTreeNodeFlags  flags = childs.empty() ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_OpenOnDoubleClick : ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	static string temp = "";
	if (temp == bone->Name())
		flags |= ImGuiTreeNodeFlags_Selected;


	ImGui::PushItemWidth(1.0f);
	if (ImGui::TreeNodeEx(bone->Name().c_str(), flags))
	{
		
		if (ImGui::IsItemClicked())
		{
			currentBone = bone;
			if (currentBone)
			{
				gizmoMode = GizmoMode::SkeletalBone;
			}
			temp = bone->Name();
		
		
		}
		
		for (auto& child : childs)
		{
			if (child)
				ShowSkeletalBones(child);
		}

		
		ImGui::TreePop();
	}
	
	
	ImGui::PopItemWidth();
}

void ActorEditor::ShowHierarcyPopup()
{
	if (ImGui::BeginPopup("Hierarchy Popup"))
	{
		if (ImGui::MenuItem("Copy")) {}
		if (ImGui::MenuItem("Delete"))
		{
			previewRender->DeleteMesh(currentBone->index);
			
			/*auto& bones = previewRender->bones;
			for_each(bones.begin(), bones.end(), [&](shared_ptr<class ModelBone>bone)
			{
				if (bone)
				{
					size_t index = bone->Name().find(".");
					if (index != string::npos)
					{
						if(bone->Name().substr(0,index)=="Circle")
						previewRender->DeleteMesh(bone->index);
					}
				}
				
			});*/

			
		
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("Create Mesh"))
		{

			if (ImGui::MenuItem("Create"))
			{
				CreateAttachMesh();
			}
			//	
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("BlendMesh"))
		{
			if (ImGui::MenuItem("Blend"))	BlendMesh();
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Create Collider"))
		{
			if (ImGui::MenuItem(" Collider"))
			{
				CreateBox();
			}
		
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Create Effect"))
		{/*
			uint count = EffectSystem::Get()->SimulationsCount();
			for (uint i = 0; i < count; i++)
			{
				const string& temp = "Effect" + to_string(i);
				if (ImGui::MenuItem(temp.c_str()))	CreateEffect(i);
			}*/

			ImGui::EndMenu();
		}


		ImGui::EndPopup();
	}
}

void ActorEditor::CreateBox()
{
	
	if (!currentBone ||previewRender->boxCount>= MAX_ACTOR_BONECOLLIDER) return;



	previewRender->colliderBoxData[previewRender->boxCount].Index= currentBone->BoneIndex();
	componentList.emplace_back("BoneCollider" + to_string(previewRender->boxCount));
	previewRender->boxCount++;
}

void ActorEditor::BlendMesh()
{
	if (!currentBone) return;

	previewRender->BlendMesh(currentBone->index);

}

void ActorEditor::CreateAttachMesh()
{
	HWND hWnd = NULL;
	function<void(wstring)> f = bind(&ActorEditor::LoadAttachMesh, this, placeholders::_1);
	Path::OpenFileDialog(L"", Path::EveryFilter, L"../_Models/", f, hWnd);
}

void ActorEditor::LoadAttachMesh(const wstring & file)
{
	BarrierModelUse();

	const auto& NoExtensionName = Path::GetFileNameWithoutExtension(file);

	previewRender->ReadMaterial(NoExtensionName);
	previewRender->ReadAttachMesh(file, meshType,currentBone->index);
	//previewRender->CreateSahders("../_Shaders/PreviewSkeletalModel.hlsl");
	bModelLoaded = true;
}
