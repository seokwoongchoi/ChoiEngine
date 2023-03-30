#include "stdafx.h"
#include "LightEditor.h"
#include "Editor.h"
#include "GBuffer/LightManager.h"
extern LightManager Lighting;
LightEditor::LightEditor(ID3D11Device * device,  Editor* editor)
	:device(device),editor(editor),bDrag(false), controllLightIndex(-1),
	pointCount(0),
    spotCount(0),
    capsuleCount(0)
{
	pointTexture = new Texture();//
	pointTexture->Load(device, L"LightIcons/PointLight.png",nullptr);
	spotTexture = new Texture();//
	spotTexture->Load(device, L"LightIcons/SpotLight.png", nullptr);
	capsuleTexture = new Texture();//
	capsuleTexture->Load(device, L"LightIcons/CapsuleLight.png", nullptr);

	D3DXMatrixIdentity(&world);
}

LightEditor::~LightEditor()
{
}

void LightEditor::Update(const Vector3& mousePos)
{
	if (Mouse::Get()->Up(0) && bDrag)
	{
		switch (lightType)
		{
		
		case LightType::pointLight:
		{
			float pointRange = 50;
			const Vector3& pointPosition = Vector3(mousePos.x, mousePos.y + 10.0f, mousePos.z);

			Lighting.AddPointLight(pointPosition, pointRange, Vector3(1, 1, 1), 1.0f, false);
			
			pointCount++;

		}
		break;
		case LightType::spotLight:
		{
			spotCount++;
		}

		break;
	

		}

		bDrag = false;
	}
	

	
}

void LightEditor::ImageButton()
{
	ImGui::Begin("Lights", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	//	if (!ImGui::IsAnyItemHovered())
	{
		ImGui::ImageButton(pointTexture->SRV(), ImVec2(50.0f, 50.0f));
		SetDragDropPayload(LightType::pointLight, pointTexture->GetFile());
		ImGui::SameLine();
		ImGui::ImageButton(spotTexture->SRV(), ImVec2(50.0f, 50.0f));
		SetDragDropPayload(LightType::spotLight, spotTexture->GetFile());
		ImGui::SameLine();
		ImGui::ImageButton(capsuleTexture->SRV(), ImVec2(50.0f, 50.0f));
		SetDragDropPayload(LightType::capsuleLight, capsuleTexture->GetFile());
	}
	ImGui::End();

	PointLightRender();
	SpotLightRender();
	CapsuleLightRender();


	

}

void LightEditor::SetDragDropPayload(const LightType & type, const wstring & data)
{
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
	{
		ImGui::SetDragDropPayload
		(
			reinterpret_cast<const char*>(&type),
			data.c_str(),
			data.length() + 1

		);
		lightType = type;
		bDrag = true;
		//dragLightName = Path::GetFileNameWithoutExtension(data);
		const string& temp = String::ToString(data);
		ImGui::Text(Path::GetFileNameWithoutExtension(temp).c_str());
		ImGui::EndDragDropSource();
	}
}

void LightEditor::Save(BinaryWriter * w)
{
	w->UInt(pointCount);
	if (pointCount > 0)
	{
		auto pointLight = Lighting.GetPointLights();

		for (uint i = 0; i < pointCount; i++)
		{
			w->Vector3(pointLight[i].Color);
			w->Vector3(pointLight[i].Position);
			w->Float(pointLight[i].Intencity);
			w->Float(pointLight[i].Range);
		
			
		}
	}
	
}

int LightEditor::LightList()
{
	ImGui::Begin("LightList", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	{
		ImGui::Columns(1, "mycolumns1", false);  // 3-ways, no border
		ImGui::Separator();
		
		{
		
			for (uint p = 0; p < pointCount; p++)
			{
				const string& name = "PointLight"+to_string(p);
				const char* label = name.c_str();

				ImGuiTreeNodeFlags  flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

				if (ImGui::TreeNodeEx(label, flags))
				{
					if (ImGui::IsItemClicked())
					{
						controllLightIndex = p;
					}
					ImGui::TreePop();
					
				}
			}

		
		
		}

		PointLightControll(controllLightIndex);
	}
	ImGui::End();

	return controllLightIndex;
}

void LightEditor::PointLightRender()
{
	if (pointCount > 0)
	{
		Matrix V, P;
			 GlobalData::GetView(&V); GlobalData::GetProj(&P);
		auto pointLight = Lighting.GetPointLights();
		for (uint i = 0; i < pointCount; i++)
		{

			
			Vector3 position = Vector3(pointLight[i].Position.x,
				pointLight[i].Position.y + (pointLight[i].Range*0.5f),
				pointLight[i].Position.z);

			Vector3 projection;
			Projection(&projection, position, world, V, P);

			if (Mouse::Get()->Down(0) && bDrag == false)
			{
				auto min = Vector2(projection.x - 30.0f, projection.y - 30.0f);
				auto max = Vector2(projection.x + 30.0f, projection.y + 30.0f);

				Vector3 mousePos = Mouse::Get()->GetPosition();


				if (mousePos.x > min.x && mousePos.x<max.x &&
					mousePos.y> min.y && mousePos.y < max.y)
				{
					cout << "clicked" << endl;
				}
			}
		/*	Gui::Get()->RenderText(projection.x + 30, projection.y, 0, 0, 1,
				"X:" + to_string(projection.x) + "Y:" + to_string(projection.y));*/
			Color color = Color(pointLight[i].Color.x, pointLight[i].Color.y, pointLight[i].Color.z, 1.0f);
			editor->RenderTexture(D3DXVECTOR2(projection.x, projection.y), D3DXVECTOR2(100.0f, 100.0f), color, pointTexture ? pointTexture : nullptr);

			
		}


	}
}

void LightEditor::SpotLightRender()
{
}

void LightEditor::CapsuleLightRender()
{
}



void LightEditor::PointLightControll(int index)
{
	if (index < 0)return;
	auto pointLight = Lighting.GetPointLights();

	if (ImGui::CollapsingHeader("PointLight"))
	{
		ImGui::ColorEdit3("Point Diffuse", &pointLight[index].Color[0]);
		
		ImGui::SliderFloat("Point Intensity", &pointLight[index].Intencity,0.1f,2.0f);
		ImGui::SliderFloat("Point Range", &pointLight[index].Range,0.0,100.0f);

	}
}

void LightEditor::SpotLightControll(int index)
{
}

void LightEditor::GridPointLight()
{
	auto pointLight = Lighting.GetPointLights();
	UINT stackCount = 30;
	float thetaStep = 2.0f * Math::PI / stackCount;

	for (uint i = 0; i < pointCount; i++)
	{

		Vector3 pos = Vector3(pointLight[i].Position.x,
			pointLight[i].Position.y- (pointLight[i].Range*0.2f),
			pointLight[i].Position.z);
		//pos.y -= 10.0f;
		float radius = pointLight[i].Range*0.7f;

	
		for (UINT i = 0; i <= stackCount; i++)
		{
			float theta = i * thetaStep;

			Vector3 p = Vector3
			(
				(radius * cosf(theta)),
				0,
				(radius * sinf(theta))
			);
			Vector3 p2 = Vector3
			(
				(radius * cosf(theta)),
				(radius * sinf(theta)),
				0
			);
			Vector3 p3 = Vector3
			(
				0,
				(radius * cosf(theta)),
				(radius * sinf(theta))
			);
			p += pos;
			p2 += pos;
			p3 += pos;
			v.emplace_back(p);
			v2.emplace_back(p2);
			v3.emplace_back(p3);
		}
		for (UINT i = 0; i < stackCount; i++)
		{
			DebugLine::Get()->RenderLine(v[i], v[i + 1]);
			DebugLine::Get()->RenderLine(v2[i], v2[i + 1]);
			DebugLine::Get()->RenderLine(v3[i], v3[i + 1]);
		}
		v.clear();
		v.shrink_to_fit();

		v2.clear();
		v2.shrink_to_fit();

		v3.clear();
		v3.shrink_to_fit();
	}
	
}

void LightEditor::GridSpotLight()
{
}

void LightEditor::GridCapsuleLight()
{
}

void LightEditor::GetLightPosition(Vector3 & pos)
{
	if (controllLightIndex < 0)return ;
	auto pointLight = Lighting.GetPointLights();
	pos= pointLight[controllLightIndex].Position;
}

void LightEditor::SetLightPosition(const Vector3 & pos)
{
	if (controllLightIndex < 0)return;
	auto pointLight = Lighting.GetPointLights();
	 pointLight[controllLightIndex].Position=pos;
}

void LightEditor::Projection(OUT Vector3 * position, Vector3 & source, const Matrix & W, const Matrix & V, const Matrix & P)
{
	Matrix wvp = W * V*P;

	Vector3 temp = source;
	D3DXVec3TransformCoord(position, &temp, &wvp);


	position->x = ((position->x + 1.0f)*0.5f)*static_cast<float>(D3D::Width()) ;
	position->y = ((-position->y + 1.0f)*0.5f)*static_cast<float>(D3D::Height());
	position->z = (position->z*(1.0f - 0.0f)) + 0.0f;
}
