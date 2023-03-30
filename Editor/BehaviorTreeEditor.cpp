#include "stdafx.h"
#include "BehaviorTreeEditor.h"
#include "ImGui/Blueprints/Blueprints.h"
#include "ImGui/NodeEditor/Include/imgui_node_editor.h"
BehaviorTreeEditor::BehaviorTreeEditor(ID3D11Device * device)
	:bShowBehaviorTree(false)
{
	behaviorTreeTexture = new Texture();
	behaviorTreeTexture->Load(device, L"BehaviorTree.jpg", nullptr);
	Application_Initialize(device);
}

BehaviorTreeEditor::~BehaviorTreeEditor()
{
}

void BehaviorTreeEditor::Save(BinaryWriter * w)
{
	w->String("../_BehaviorTreeDatas/BehaviorTree0.behaviortree");
}

void BehaviorTreeEditor::Initiallize(ID3D11Device * device)
{
}

void BehaviorTreeEditor::Render()
{

}

void BehaviorTreeEditor::ImageButton()
{
	if (ImGui::ImageButton(behaviorTreeTexture->SRV(), ImVec2(80, 80)))
	{
		bShowBehaviorTree = true;
	}
	ImGui::SameLine();
	if (!bShowBehaviorTree)return;

	Application_Frame(&bShowBehaviorTree);
}

void BehaviorTreeEditor::LoadAndCompile(const string& path)
{
	LoadAllNodes(L"../_BehaviorTreeDatas/BehaviorTree0.behaviortree");
	Compile();
}
