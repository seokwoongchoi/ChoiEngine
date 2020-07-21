#include "stdafx.h"
#include "ActorEditor.h"
#include "Core/Engine.h"
#include "PreviewRenderer.h"
#include "Resources/Mesh.h"

ActorEditor::ActorEditor(ID3D11Device* device):
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
	dsv(nullptr), meshType(ReadMeshType::StaticMesh), currentBone(nullptr), modelName(L""), actorIndex(0), size(800,600)
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
	
}


ActorEditor::~ActorEditor()
{
	
}



void ActorEditor::Render(ID3D11DeviceContext* context)
{

	if (!bModelLoaded) return;
	
		previewRender->Update(Vector2(size.x,size.y));

		context->ClearRenderTargetView(rtv, Color(0.0f, 0.0f, 0.0f, 1.0f));
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

void ActorEditor::ImageButton(class Engine* engine)
{
	if (ImGui::ImageButton(srv, ImVec2(80, 80)))
	{
		bEditing = true;
	}

	SetDragDropPayload(String::ToString(modelName));

	Editor();
	ShowHierarchy();
	
	if (EditingMode())
	{
		engine->Load(actorIndex, modelName,meshType);
	}
	
}

bool ActorEditor::EditingMode()
{
	if (!bEditing ) return false;

	bool isCompiles = false;

	ImGui::Begin("EditingMode", &bEditing);
	{
		if (ImGui::Button("Aniamtor", ImVec2(80, 30)) /*&& model&&model->ClipCount() > 0*/)
		{
			//mode = EditMode::Animator;
		}
		ImGui::SameLine(100);
		if (ImGui::Button("Render", ImVec2(80, 30)))
		{
			//mode = EditMode::Render;


		}
		ImGui::SameLine(190);


		if (ImGui::Button("Compile", ImVec2(80, 30)))
		{
			Compile();

			isCompiles= true;

		}

	}
	ImGui::End();

	return isCompiles;
}

void ActorEditor::Compile()
{
	previewRender->SaveMeshFile(modelName,meshType);
	bCompiled = true;
}

void ActorEditor::Editor()
{
	if (!bEditing) return;

	ImGui::SetWindowSize(ImVec2(900, 600), ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints
	(
		ImVec2(800, 600),
		ImVec2(1280, 720)
	);
	ImGuiWindowFlags windowFlags = 0;//ImGuiWindowFlags_NoResize |ImGuiWindowFlags_NoMove;
	//ImGuiWindowFlags_MenuBar|

	



	ImGui::Begin("Editor", &bEditing, windowFlags);
	{
	
		size = ImGui::GetWindowSize();
		const string& str = string("SizeX : ") + to_string(size.x) + "/" + string("SizeY : ") + to_string(size.y);
		ImGui::Text(str.c_str());
	
		ShowFrame(size);
		
		ShowComponents(size);
		ShowMaterial(size);
	}
	ImGui::End();
}

void ActorEditor::ShowFrame(const ImVec2 & size)
{
	ImGui::BeginChild("##Frame", ImVec2(size.x*0.5f, 0), true, ImGuiWindowFlags_NoScrollbar);
	{
		
	//	ImGuiViewport* viewPort= ImGui::GetWindowViewport();
		//viewPort->Size = ImVec2(1280, 720);
		//ImGui::SetNextWindowViewport(viewPort->ID);
		ImGui::Image
		(
			srv, ImVec2(size.x*0.5f, size.y)
		);
	
	}
	ImGui::EndChild();
}

void ActorEditor::ShowComponents(const ImVec2 & size)
{
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
								/*componentName = componentList[i];
								if (componentName == "Collider")
								{
									gizmoType = GizmoType::Box;
								}
								else if (componentName == "BoneCollider0")
								{
									gizmoType = GizmoType::Box1;
								}
								else if (componentName == "BoneCollider1")
								{
									gizmoType = GizmoType::Box2;
								}
								else if (componentName == "Effect0")
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
								}
								*/
							}
						/*	if (IsBindedTree&&componentList[i] == "ActorAi")
							{
								string treeName = "BehaviorTree" + to_string(sharedData->behviorTreeNum);
								if (ImGui::TreeNodeEx(treeName.c_str(), flags))
								{
									ImGui::TreePop();
								}
							}*/
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


			//if (bModelLoaded)
			//	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
			//	{

			//		auto previewTransform = previewRender->PreviewTransform();
			//		Quaternion rotation;
			//		previewTransform->Rotation(&rotation);

			//		Vector3 scale;
			//		previewTransform->Scale(&scale);


			//		const auto show_float = [](const char* id, const char* label, float* value)
			//		{
			//			ImGui::PushItemWidth(100.0f);
			//			ImGui::PushID(id);
			//			ImGui::InputFloat(label, value, 1.0f, 1.0f, "%.3f", ImGuiInputTextFlags_CharsDecimal);
			//			ImGui::PopID();
			//			ImGui::PopItemWidth();
			//		};
			//		if (currentBone)
			//		{
			//			if (currentBone->BoneIndex() > 0)
			//			{
			//				auto local = model->BoneByIndex(currentBone->BoneIndex())->Transform();
			//				Vector3 position = Vector3(local._41, local._42, local._43);
			//				//previewTransform->Position(&position);

			//				ImGui::TextUnformatted("BoneTranslation");
			//				//ImGui::SameLine(70.0f); 
			//				show_float("##pos_x", "X", &position.x);
			//				//ImGui::SameLine();    
			//				show_float("##pos_y", "Y", &position.y);
			//				//ImGui::SameLine();     
			//				show_float("##pos_z", "Z", &position.z);
			//				/*Matrix S, R, T;
			//				Vector3 position1, scale;
			//				Quaternion q;
			//				D3DXMatrixDecompose(&scale, &q, &position1, &local);
			//				D3DXMatrixScaling(&S, scale.x, scale.y, scale.z);
			//				D3DXMatrixRotationQuaternion(&R, &q);
			//				D3DXMatrixTranslation(&T, position.x, position.y, position.z);
			//				model->ChangeBone(S*T*R, currentBone->BoneIndex());*/
			//			}



			//		}

			//		ImGui::TextUnformatted("Rotation");
			//		//ImGui::SameLine(70.0f); 
			//		show_float("##rot_x", "X", &rotation.x);
			//		//ImGui::SameLine();     
			//		show_float("##rot_y", "Y", &rotation.y);
			//		//ImGui::SameLine();     
			//		show_float("##rot_z", "Z", &rotation.z);

			//		ImGui::TextUnformatted("Scale");
			//		//ImGui::SameLine(70.0f); 
			//		show_float("##scl_x", "X", &scale.x);
			//		//ImGui::SameLine();     
			//		show_float("##scl_y", "Y", &scale.y);
			//		//ImGui::SameLine();      
			//		show_float("##scl_z", "Z", &scale.z);


			//		//previewTransform->Position(position);
			//		previewTransform->Rotation(rotation);
			//		previewTransform->Scale(scale);

			//	}

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
			//if (!components[ComponentType::ActorCamera])
			//{

			//	//components.emplace_back(animator);
			//	CreateActorCamera();
			//}


		}
		if (ImGui::MenuItem("ActorAI"))
		{
			/*if (!components[ComponentType::ActorAi])
			{

				CreateActorAI();
			}*/


		}

		ImGui::EndPopup();
	}
}

void ActorEditor::ShowComponentListPopUp(const string & componentName)
{
}

void ActorEditor::LoadSkeletalMesh(const wstring & file)
{
	bCompiled = false;
	modelName = Path::GetFileNameWithoutExtension(file);

	previewRender->ReadMaterial(modelName);
	meshType = ReadMeshType::SkeletaMesh;
	previewRender->ReadMesh(modelName, meshType);
	previewRender->CreateSahders("../_Shaders/SkeletalMesh.hlsl");

	
	bModelLoaded = true;
	
	
	auto remove = remove_if(componentList.begin(), componentList.end(), [](const string& mesh)
	{
		return mesh == "StaticMesh";
	});
	if(remove!=componentList.end())
	componentList.erase(remove);

	componentList.emplace_back("SkeletalMesh");
}

void ActorEditor::LoadStaticMesh(const wstring & file)
{
	
	bCompiled = false;
	modelName = Path::GetFileNameWithoutExtension(file);
	
	previewRender->ReadMaterial(modelName);
	meshType = ReadMeshType::StaticMesh;
	previewRender->ReadMesh(modelName, meshType);
	previewRender->CreateSahders("../_Shaders/StaticMesh.hlsl");

	bModelLoaded = true;
	
	auto remove = remove_if(componentList.begin(), componentList.end(), [](const string& mesh)
	{
		return mesh == "SkeletalMesh";
	});
	if (remove != componentList.end())
		componentList.erase(remove);
	componentList.emplace_back("StaticMesh");
}

void ActorEditor::LoadMaterial(function<void(wstring, uint, Material*)> f, uint num, shared_ptr<class Material> material)
{
}

void ActorEditor::SetMaterial(wstring & file, uint textureType, shared_ptr<class Material> material)
{
}

void ActorEditor::ShowMaterial(const ImVec2 & size)
{
	const auto ShowTextureSlot = [&](shared_ptr<class Material> material, uint num)//, TextureType type
	{
		

		auto& texture1 = material->DiffuseMap();
		ImGui::Text(String::ToString(material->Name()).c_str());
		ImGui::SameLine(70.0f);

		function<void(wstring, uint, Material*)> f;

		if (ImGui::ImageButton
		(
			texture1.SRV() ? texture1.SRV() : nullptr,
			ImVec2(80, 80)

		))
		{
			LoadMaterial(f, num, material);
		}
		ImGui::Text("Color");
		ImGui::SameLine(70.0f);
		Color Diffuse = material->Diffuse();
		ImGui::ColorEdit4(String::ToString(material->Name()).c_str(), Diffuse);
		material->Diffuse(Diffuse);


	};

	const auto ShowTextureSlotRoughness = [&](shared_ptr<class Material> material, uint num)//, TextureType type
	{
		auto& texture1 = material->RoughnessMap();
		ImGui::Text(String::ToString(material->Name()).c_str());
		ImGui::SameLine(70.0f);

		function<void(wstring, uint, Material*)> f;

		if (ImGui::ImageButton
		(
			texture1.SRV()? texture1.SRV():nullptr,
			ImVec2(80, 80)

		))
		{
			LoadMaterial(f, num, material);
		}
		ImGui::Text("Color");
		ImGui::SameLine(70.0f);
		float roughness = material->Roughness();
		ImGui::SliderFloat(String::ToString(material->Name()).c_str(), &roughness, 0.0f, 20.0f);

		material->Roughness(roughness);
		//DragDrop(material, type);

	};

	ImGui::SameLine();
	ImGui::BeginChild("##Edit", ImVec2(size.x*0.25f + 30.0f, 0), true);
	{

		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{


			
			   auto& count = previewRender->meshCount;

			   auto& mesh = previewRender->meshes;
				if (ImGui::CollapsingHeader("Diffuse", ImGuiTreeNodeFlags_OpenOnDoubleClick))
				{

					for (uint i = 0; i < count; i++)
					{
						//Albedo
						ShowTextureSlot(mesh[i]->material, 0);
						ImGui::Separator();

					}
					if (bBlend)
					{
						//count = model->forwardMeshesCount();
						//mesh = model->forwardMeshsData();
						//for (uint i = 0; i < count; i++)
						//{
						//	//Albedo
						//	ShowTextureSlot(mesh[i], 0);
						//	ImGui::Separator();

						//}

					}
				}
				if (ImGui::CollapsingHeader("Roughness", ImGuiTreeNodeFlags_OpenOnDoubleClick))
				{

					for (uint i = 0; i < count; i++)
					{


						ShowTextureSlotRoughness(mesh[i]->material, 1);
						ImGui::Separator();



					}
					if (bBlend)
					{
						/*count = model->forwardMeshesCount();
						mesh = model->forwardMeshsData();
						for (uint i = 0; i < count; i++)
						{

							ShowTextureSlotRoughness(mesh[i], i);
							ImGui::Separator();

						}*/

					}
				}


			

			ImGui::Separator();
		}
	}
	ImGui::EndChild();
}

void ActorEditor::ShowHierarchy()
{
	if (!bEditing) return;


	ImGui::Begin("Hierarchy", &bEditing, ImGuiWindowFlags_HorizontalScrollbar);
	{
		auto& bone = previewRender->bones;
		uint& count = previewRender->boneCount;
		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{


			for (uint i = 0; i < count; i++)
			{
				ShowBone(bone[i]);
			}
		}
		break;
		case ReadMeshType::SkeletaMesh:
		{
			shared_ptr<class ModelBone> root = nullptr;


			for (uint i = 0; i < count; i++)
			{

				if (bone[i]->ChildsData())
				{
					root = bone[i];
					break;
				}

			}
			ShowChild(root);


		}
		break;

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

void ActorEditor::ShowBone(shared_ptr<class ModelBone> bone)
{

	{

		ImGuiTreeNodeFlags  flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		static string temp = "";
		if (temp == bone->Name())
			flags |= ImGuiTreeNodeFlags_Selected;


		ImGui::PushItemWidth(1.0f);

		if (ImGui::TreeNodeEx(bone->Name().c_str(), flags))
		{
			if (ImGui::IsItemClicked())
			{
				currentBone = bone;
				/*if (currentBone)
				{
					gizmoType = GizmoType::Bone;
				}*/
				temp = bone->Name();

			}
			ImGui::TreePop();

		}
		ImGui::PopItemWidth();
	}


}

void ActorEditor::ShowChild(shared_ptr<class ModelBone> bone)
{
	auto child = bone->ChildsData();
	ImGuiTreeNodeFlags  flags = !child ? ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_OpenOnDoubleClick : ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick;
	static string temp = "";
	if (temp == bone->Name())
		flags |= ImGuiTreeNodeFlags_Selected;


	ImGui::PushItemWidth(1.0f);
	if (ImGui::TreeNodeEx(bone->Name().c_str(), flags))
	{
		if (ImGui::IsItemClicked())
		{
			currentBone = bone;
			/*if (currentBone)
			{
				gizmoType = GizmoType::Bone;
			}*/
			temp = bone->Name();

		}
		uint count = bone->GetChilds().size();
		for (uint i = 0; i < count; i++)
		{
			ShowChild(child[i]);

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
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("Create Mesh"))
		{

			if (ImGui::MenuItem("Create"))
			{

			}
			//	CreateWeaponMesh();
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

			}
			//CreateBox();

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

void ActorEditor::BlendMesh()
{
}
