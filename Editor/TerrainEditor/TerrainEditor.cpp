#include "stdafx.h"
#include "TerrainEditor.h"
#include "Core/D3D11/D3D11_Helper.h"
#include "Environment/Island11_HLSL.h"

TerrainEditor::TerrainEditor(ID3D11Device* device,class Island11* island11)
	:island11(island11),brushType(BrushType::None), updateDescBuffer(nullptr),
	brushDescBuffer(nullptr), UpdateHeightCS(nullptr), SmoothingCS(nullptr)
{
	if (!island11)return;
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(BrushDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &brushDescBuffer));
	bufferDesc.ByteWidth = sizeof(UpdateDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &updateDescBuffer));

	//////////////////////////////////////////////////////////////////////////
	ID3DBlob* ShaderBlob = nullptr;
	auto& path = "../_Shaders/ComputeShaders/UpdateHeightCS.hlsl";
	auto entryPoint = "UpdateHeight";
	auto& shaderModel = "cs_5_0";
	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));


	Check(device->CreateComputeShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&UpdateHeightCS
	));

	SafeRelease(ShaderBlob);

	entryPoint = "Smoothing";
	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));


	Check(device->CreateComputeShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&SmoothingCS
	));
}

TerrainEditor::~TerrainEditor()
{
}

void TerrainEditor::ImGui()
{
	if (!island11)return;
	ImGui::Begin("Terrain", 0, ImGuiWindowFlags_NoMove);
	

	//for brushtype combo box



	const char* brushNames[] = { "None","Raise", "Smoothing","Splatting","Billboard" };
	static const char* brushName = brushNames[static_cast<uint>(brushType)];
	if (ImGui::CollapsingHeader("Terrain Brush", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const char* brushShapes[] = { "None" ,"Square", "Circle" };
		static const char* brushShape = brushShapes[0];
		if (ImGui::BeginCombo("BrushShape", brushShape))
		{
			for (uint i = 0; i < IM_ARRAYSIZE(brushShapes); i++)
			{
				bool bSelected = brushShape == brushShapes[i];
				if (ImGui::Selectable(brushShapes[i], bSelected))
				{
					brushShape = brushShapes[i];
					if (brushShape == "None")
						brushDesc.BrushShape = 0;
					else if (brushShape == "Square")
						brushDesc.BrushShape = 1;
					else if (brushShape == "Circle")
						brushDesc.BrushShape = 2;

				}

				if (bSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		//ImGui::Checkbox("Update", &bUpdate);
		//ImGui::SameLine();
		ImGui::PushItemWidth(100);
		ImGui::ColorEdit4("Color", (float*)&brushDesc.Color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
		ImGui::SameLine();
		ImGui::SliderInt("Range", (int*)&brushDesc.Range, 1, 100);
		ImGui::PopItemWidth();
		
		
		ImGui::Separator();
		if (ImGui::BeginCombo("BrushType", brushName))
		{
			for (uint i = 0; i < IM_ARRAYSIZE(brushNames); i++)
			{
				bool bSelected = brushName == brushNames[i];
				if (ImGui::Selectable(brushNames[i], bSelected))
				{
					brushName = brushNames[i];
					if (brushName == "None")
						brushType = BrushType::None;
					else if (brushName == "Raise")
						brushType = BrushType::Raise;
					else if (brushName == "Smoothing")
						brushType = BrushType::Smooth;
					else if (brushName == "Splatting")
						brushType = BrushType::Splatting;
					else if (brushName == "Billboard")
						brushType = BrushType::Billboard;


				}

				if (bSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();

		}
	}
	static float rfactor = 0.0f;
	if (brushType == BrushType::Raise&&brushDesc.BrushShape == 2)
	{
		ImGui::SliderFloat("BrushRadian", &rfactor, 0.0f, 50.0f);
		ImGui::Separator();
	}
	if (brushType == BrushType::Raise&&brushDesc.BrushShape == 1)
	{
		ImGui::SliderFloat("RaiseSpeed", (float*)(&updateDesc.raiseSpeed), 0.01f, 10.0f);

		ImGui::Separator();
	}
	//if (brushType == BrushType::Billboard)
	//{
	//	if (ImGui::CollapsingHeader("Billboard Detail", ImGuiTreeNodeFlags_DefaultOpen))
	//	{
	//		ImGui::SliderInt("BillBoard Count", reinterpret_cast<int*>(&billboardCount), 1, 1000);

	//		ImGui::InputFloat2("BillBoard Scale", (float*)(&billboardScale));
	//		static bool bTemp = false;
	//		//static const char* billboardName = "N/A";
	//		string billboardName = "N/A";
	//		if (ImGui::BeginCombo("Billboard", bTemp ? String::ToString(billboarfiles[billbaordIndex]).c_str() : billboardName.c_str()))// 
	//		{
	//			for (uint i = 0; i < billboarfiles.size(); i++)
	//			{
	//				ImGui::Image(billboardTexture[i]->SRV(), ImVec2(100, 100));
	//				ImGui::SameLine();
	//				bool bSelected = billboardName == String::ToString(billboarfiles[i]);
	//				if (ImGui::Selectable(String::ToString(billboarfiles[i]).c_str(), bSelected))
	//				{
	//					if (bTemp == false)
	//						bTemp = true;

	//					billboardName = String::ToString(billboarfiles[i]);
	//					billbaordIndex = i;
	//					selectedBillboard = billboardTextures[i];
	//				}


	//				if (bSelected)
	//					ImGui::SetItemDefaultFocus();
	//			}
	//			ImGui::EndCombo();


	//		}
	//		//ImGui::Checkbox("Eraser", &bEraser);

	//	}
	//	ImGui::Separator();
	//}

	

	if (ImGui::Button("UpdateHeight", ImVec2(100, 30)))
	{
		island11->CreateTerrain();
	}

	if (ImGui::Button("SaveHeightMap", ImVec2(100, 30)))
	{
		island11->SaveHeightMap();
	}
	

	ImGui::End();
}

void TerrainEditor::Update(const Vector3 & mousePos,ID3D11DeviceContext* context)
{
	brushDesc.Location = mousePos;
	if (brushType==BrushType::None||Mouse::Get()->Press(0) == false) return;


	updateDesc.Location = mousePos;
	updateDesc.Range = brushDesc.Range;
	updateDesc.BrushShape = brushDesc.BrushShape;

	switch (brushType)
	{
	case BrushType::Raise:
		RaiseHeight(context);
		break;
	case BrushType::Smooth:
		SmoothBrush(context);
		break;
/*	case BrushType::Splatting:
		Splatting(mousePos, brushDesc.Type, brushDesc.Range, rfactor, splatFactor);
		break;*/
	//case BrushType::Billboard:
	/*
		if (Mouse::Get()->Down(0))
			BillboardBrush(mousePos, brushDesc.BrushShape, brushDesc.Range, billboardCount, Vector2(billboardScale[0], billboardScale[1]));*/
//		break;

	}
}

ID3D11Buffer * TerrainEditor::GetBrushBuffer(ID3D11DeviceContext* context)
{

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(brushDescBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &brushDesc, sizeof(brushDesc));
	context->Unmap(brushDescBuffer, 0);
		
	return brushDescBuffer;
}

void TerrainEditor::RaiseHeight(ID3D11DeviceContext* context)
{
	
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(updateDescBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &updateDesc, sizeof(updateDesc));
	context->Unmap(updateDescBuffer, 0);
	ID3D11Buffer* bufferArray[1] = { updateDescBuffer };
	context->CSSetConstantBuffers(0, 1, bufferArray);

	ID3D11UnorderedAccessView* uavArray[1]=
	{
		island11->GetHeightUAV()
	};

	context->CSSetUnorderedAccessViews(0, 1, uavArray, nullptr);
	context->CSSetShader(UpdateHeightCS, nullptr, 0);
	context->Dispatch(1024, 1, 1);



	ZeroMemory(&bufferArray, sizeof(bufferArray));
	context->CSSetConstantBuffers(0, 1, bufferArray);
	context->CSSetShader(nullptr, nullptr, 0);
	ZeroMemory(uavArray, sizeof(uavArray));
	context->CSSetUnorderedAccessViews(0, 1, uavArray, nullptr);
}

void TerrainEditor::SmoothBrush(ID3D11DeviceContext* context)
{

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(updateDescBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &updateDesc, sizeof(updateDesc));
	context->Unmap(updateDescBuffer, 0);
	ID3D11Buffer* bufferArray[1] = { updateDescBuffer };
	context->CSSetConstantBuffers(0, 1, bufferArray);

	ID3D11UnorderedAccessView* uavArray[1] =
	{
		island11->GetHeightUAV()
	};

	context->CSSetUnorderedAccessViews(0, 1, uavArray, nullptr);
	context->CSSetShader(SmoothingCS, nullptr, 0);
	context->Dispatch(1024, 1, 1);



	ZeroMemory(&bufferArray, sizeof(bufferArray));
	context->CSSetConstantBuffers(0, 1, bufferArray);
	context->CSSetShader(nullptr, nullptr, 0);
	ZeroMemory(uavArray, sizeof(uavArray));
	context->CSSetUnorderedAccessViews(0, 1, uavArray, nullptr);
}
