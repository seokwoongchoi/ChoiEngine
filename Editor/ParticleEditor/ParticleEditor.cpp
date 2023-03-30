#include "stdafx.h"
#include "ParticleEditor.h"
#include "Particles/ParticleSimulation.h"
#include "Particles/Sparks.h"
#include "Particles/SoftParticle.h"
#include "Core/Engine.h"
ParticleEditor::ParticleEditor(ID3D11Device * device, class Engine* engine, class Editor* editor, uint ID)
	:engine(engine), editor(editor),bEditing(false), particle(nullptr), device(device),
	backBuffer(nullptr), rtv(nullptr), srv(nullptr), depthBackBuffer(nullptr), dsv(nullptr), viewport{},
	bStart(false), particleTexture(nullptr), textureName(L""), targetPosition(0, 0, 0), rotation(-12.57f, -5.23f), distance(10.0f),
	view{}, proj{}, viewRight(1, 0, 0),	viewUp(0, 1, 0), EyePosition(0,0,0), particleType(ReadParticleType::Default),ID(ID)
, bDrag(false), bSmokeSetted(false)
{
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

		Check(device->CreateDepthStencilView(depthBackBuffer, &desc, &dsv));



	}

	for (uint i = 0; i < 3; i++)
	{
		buttonTextures[i] = new Texture();

	}
	buttonTextures[0]->Load(device, L"playButton.png", nullptr);
	buttonTextures[1]->Load(device, L"pauseButton.png", nullptr);
	buttonTextures[2]->Load(device, L"stopButton.png", nullptr);




	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;

	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;


}

ParticleEditor::~ParticleEditor()
{
}

void ParticleEditor::SetWorld(const Matrix & world)
{
	{
		auto smoke = dynamic_cast<SoftParticle*>(particle);
		if (smoke)
		{
			smoke->simulateDesc.World = world;
			
			
			bSmokeSetted = true;
			
			
		}

	}
}

const Matrix & ParticleEditor::GetWorld()
{
	auto smoke = dynamic_cast<SoftParticle*>(particle);
	if (smoke)
	{
		return smoke->simulateDesc.World;
	}

	throw std::exception();
}



void ParticleEditor::PreviewRender(ID3D11DeviceContext * context)
{
	if (!bEditing) return;

	context->ClearRenderTargetView(rtv, Color(0.5f, 0.6f, 0.3f, 1.0f));
	context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
	context->OMSetRenderTargets(1, &rtv, dsv);

	if (particle)
	{
		CamUpdate();

		
		ID3D11ShaderResourceView* srv = particleTexture ? particleTexture->SRV():nullptr;
		context->PSSetShaderResources(2, 1, &srv);
		
	
	

		if (bStart)
		{
	
			particle->PreviewRender(context, view, proj);
		}
	
	}

}

void ParticleEditor::Save(BinaryWriter * w)
{
	switch (particleType)
	{
	
	case ReadParticleType::Spark:
	{
		w->UInt(1);
		auto& path = "../_ParticleDatas/Sparks" + to_string(ID) + ".particle";
		w->String(path);
	}
		break;
	case ReadParticleType::Blood:
		break;
	case ReadParticleType::Smoke:
		w->UInt(3);
		auto& path = "../_ParticleDatas/Smoke" + to_string(ID) + ".particle";
		w->String(path);
		break;
	
	}

}

void ParticleEditor::Editor()
{
	if (!bEditing) return;

	ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints
	(
		ImVec2(800, 600),
		ImVec2(static_cast<float>(D3D::Width()), static_cast<float>(D3D::Height()))
	);
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove;

	//ImGuiWindowFlags_MenuBar|

	ImGui::Begin("ParticleEditor", &bEditing);
	{

		ImVec2 size = ImGui::GetWindowSize();


		ShowFrame(size);

		SelectParticleType(size);


	}
	ImGui::End();
}

void ParticleEditor::ShowFrame(const ImVec2 & size)
{
	ImGui::BeginChild("##Frame", ImVec2(size.x*0.5f, 0), true, ImGuiWindowFlags_NoScrollbar);
	{

		//pers->Set(size.x * 0.5f, size.y);
		ImGui::Image
		(
			 srv, ImVec2(size.x*0.5f, size.y)
		);

		this->size = ImVec2(size.x*0.5f, size.y);
	}
	ImGui::EndChild();
}

void ParticleEditor::SelectParticleType(const ImVec2 & size)
{
	ImGui::SameLine();
	ImGui::BeginChild("##ParticleType", ImVec2(size.x*0.5f - 70.0f, 0), true);
	{


		if (ImGui::CollapsingHeader("ParticleType", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button("Sparks", ImVec2(80, 30)))
			{
				
				SafeDelete(particle);
				particle = new Sparks(device, ID);

				particleType = ReadParticleType::Spark;
			}
			ImGui::SameLine(100);
			if (ImGui::Button("Blood", ImVec2(80, 30)))
			{
				//particle.reset();
				//particle = make_shared<Sparks>(device, ID);
				//particleType = ReadParticleType::Blood;
			}
			ImGui::SameLine(190);

			if (ImGui::Button("Smoke", ImVec2(80, 30)))
			{
				//particle.reset();
				//particle = make_shared<SoftParticle>(device, ID);
				SafeDelete(particle);
				particle = new SoftParticle(device,ID);
				particleType = ReadParticleType::Smoke;
				

			}
			ImGui::SameLine(300);

			auto spark = dynamic_cast<Sparks*>(particle);
		    if(spark)
			if (ImGui::ImageButton
			(
				
				particleTexture ?particleTexture->SRV(): nullptr,
				ImVec2(80, 80)

			))
			{
				function<void(wstring)> f;
				HWND hWnd = NULL;
				f = bind(&ParticleEditor::SetTexture, this, placeholders::_1);
				Path::OpenFileDialog(L"", Path::EveryFilter, L"../../_Textures/", f, hWnd);
			}

			
		
			auto smoke = dynamic_cast<SoftParticle*>(particle);
		
			if (spark)
			{
				ImGui::SliderInt("NumBodies", (int*)&spark->simulateDesc.numParticles,0,1024);
				ImGui::SliderFloat("Squared", &spark->simulateDesc.softeningSquared, 0.01f,0.2f);
							
			    ImGui::SliderFloat("velocityScale", &spark->velocityScale, 0.1f, 10.0f);
				ImGui::SliderFloat("clusterScale", &spark->clusterScale, 0.1f, 10.0f);
			
				ImGui::SliderFloat("Intensity", &spark->wvpDesc.intensity, 0.1f, 2.0f);
				ImGui::SliderFloat("pontSize", &spark->wvpDesc.pontSize,0.1f,5.0f);
				ImGui::SliderFloat("Time", &spark->simulateDesc.timer,0.0,2.0f);

				ImGui::Text("Sword Hit:");
				ImGui::SameLine();

				const char* BoneColliers[] = { "BodyCollider" ,"HeadCollider", "SwordCollider" };

				if (ImGui::BeginCombo("##BoneCollider", BoneCollider.c_str()))
				{
					for (uint i = 0; i < IM_ARRAYSIZE(BoneColliers); i++)
					{
						bool bSelected = BoneCollider == BoneColliers[i];
						if (ImGui::Selectable(BoneColliers[i], bSelected))
						{
							BoneCollider = BoneColliers[i];
							if (BoneCollider == "BodyCollider")
								BindEffectBone = 0;
							else if (BoneCollider == "HeadCollider")
								BindEffectBone = 1;
							else if (BoneCollider == "SwordCollider")
								BindEffectBone = 2;

						}

						if (bSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}
			else if (smoke)
			{
				ImGui::SliderInt("NumBodies", (int*)&smoke->simulateDesc.numParticles, 500, 500);
			
				ImGui::SliderFloat("ParticleLifeSpan", &smoke->simulateDesc.ParticleLifeSpan, 0.01f, 5.0f);
				ImGui::SliderFloat("EmitRate", &smoke->simulateDesc.EmitRate, 0.01f, 0.05f);
				ImGui::SliderFloat("ParticleMaxSize", &smoke->simulateDesc.ParticleMaxSize, 0.1f, 5.0f);
				ImGui::SliderFloat("ParticleMinSize", &smoke->simulateDesc.ParticleMinSize, 0.1f, 5.0f);
				ImGui::SliderFloat("FadeDistance", &smoke->simulateDesc.FadeDistance, 0.01f, 10.0f);
				ImGui::SliderFloat("articleVel", &smoke->simulateDesc.articleVel, 0.1f, 5.0f);
				ImGui::DragFloat3("factor", &smoke->simulateDesc.factor[0], 0.1f, 5.0f);
				ImGui::DragFloat3("dir", &smoke->simulateDesc.dir[0], 0.0f, 1.0f);
				
			}
		

			if (ImGui::ImageButton(buttonTextures[0]->SRV(), ImVec2(20.0f, 20.0f)))//play
			{
				if (spark)
				spark->InitBodies();


				bStart = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(buttonTextures[1]->SRV(), ImVec2(20.0f, 20.0f)))//pause
			{
				bStart = false;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(buttonTextures[2]->SRV(), ImVec2(20.0f, 20.0f)))//stop
			{

			}
			ImGui::SameLine();
			if(spark||smoke&& bSmokeSetted)
			if (ImGui::Button("Compile", ImVec2(80, 30)))
			{

				Compile();

			}

			if (ImGui::Button("Load", ImVec2(80, 30)))
			{

				HWND hWnd = NULL;
				function<void(wstring,bool)> f = bind(&ParticleEditor::Load, this, placeholders::_1, placeholders::_2);
				Path::OpenFileDialog(L"", Path::EveryFilter, L"../_ParticleDatas/",false, f, hWnd);

			}
		
		}


	}

	ImGui::EndChild();
}

void ParticleEditor::CamUpdate()
{
	Vector2 val = Vector2(ImGui::GetMouseDragDelta(1).x, ImGui::GetMouseDragDelta(1).y);


	float camSpeed = 0.8f;
	if (ImGui::IsAnyWindowHovered())
	{
		if (ImGui::IsMouseDown(1))
		{

			rotation.y += (val.y / 20)*0.008f;
			rotation.x += (val.x / 20)*0.008f;

		}

		if (ImGui::IsKeyPressed('E'))
		{
			targetPosition += viewUp * camSpeed;

		}
		else if (ImGui::IsKeyPressed('Q'))
		{
			targetPosition -= viewUp * camSpeed;
		}

		if (ImGui::IsKeyPressed('D'))
		{
			targetPosition += viewRight * camSpeed;
		}
		else if (ImGui::IsKeyPressed('A'))
		{
			targetPosition -= viewRight * camSpeed;

		}
		if (ImGui::IsKeyPressed('W'))//&& distance > 0
		{
			distance -= camSpeed;
		}
		else if (ImGui::IsKeyPressed('S') && distance < 300)
		{
			distance += camSpeed;
		}

	}


	
	EyePosition.x = targetPosition.x + distance * sinf(rotation.y)*cosf(rotation.x);
	EyePosition.y = targetPosition.y + distance * cosf(rotation.y);
	EyePosition.z = targetPosition.z + distance * sinf(rotation.y)*sinf(rotation.x);

	D3DXMatrixLookAtLH(&view, &EyePosition, &targetPosition, &Vector3(0, 1, 0));

	float aspect = static_cast<float>(D3D::Width()) / static_cast<float>(D3D::Height());
	D3DXMatrixPerspectiveFovLH(&proj, static_cast<float>(D3DX_PI)* 0.25f, aspect, 0.1f, 1000.0f);

}

void ParticleEditor::ImageButton()
{
	if (ImGui::ImageButton(srv, ImVec2(80, 80)))
	{

		bEditing = true;
	}
	ImGui::SameLine();

	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
	{
		bDrag = true;
		string data = "";
		switch (particleType)
		{
		
		case ReadParticleType::Spark:
			data = "Spark" + to_string(ID);
			break;
		case ReadParticleType::Blood:
			break;
		case ReadParticleType::Smoke:
			data = "Smoke" + to_string(ID);
			break;
	
		}
		ImGui::Text(data.c_str());
		ImGui::EndDragDropSource();
	}
	Editor();
}

void ParticleEditor::Compile()
{
	auto spark = dynamic_cast<Sparks*>(particle);
	wstring path;
	if (spark)
	{
		BinaryWriter* w = new BinaryWriter();
		 path = L"../_ParticleDatas/Sparks"+to_wstring(BindEffectBone)+L".particle";
		w->Open(path);
		w->UInt(spark->simulateDesc.numParticles);
		w->Float(spark->simulateDesc.softeningSquared);
		w->Float(spark->simulateDesc.distance);
		w->Float(spark->clusterScale);
		w->Float(spark->velocityScale);
		w->Float(spark->wvpDesc.intensity);
		w->Float(spark->wvpDesc.pontSize);
		w->Float(spark->simulateDesc.timer);
		string temp = String::ToString(textureName);
		w->String(temp);
		w->Int(BindEffectBone);
		w->Close();
		SafeDelete(w);
	
		engine->LoadParticle(ID, path, particleType);
	}
	auto smoke = dynamic_cast<SoftParticle*>(particle);
	if (smoke)
	{
		BinaryWriter* w = new BinaryWriter();
		path = L"../_ParticleDatas/Smoke" + to_wstring(ID) + L".particle";
		w->Open(path);
		w->UInt(smoke->simulateDesc.numParticles);
		w->Float(smoke->simulateDesc.ParticleLifeSpan);
		w->Float(smoke->simulateDesc.EmitRate);
		w->Float(smoke->simulateDesc.ParticleMaxSize);
		w->Float(smoke->simulateDesc.ParticleMinSize);
		w->Float(smoke->simulateDesc.articleVel);
		w->Vector3(smoke->simulateDesc.dir);
		w->Vector3(smoke->simulateDesc.factor);
		w->Matrix(smoke->simulateDesc.World);
		w->Close();
		SafeDelete(w);


		engine->LoadParticle(ID, path, particleType);
		cout << "Loaded" << endl;
	}
	
}

void ParticleEditor::Load(const wstring & file, bool IsLevel)
{
	auto fileName = Path::GetFileNameWithoutExtension(file);
	auto temp = fileName.substr(0, fileName.length()-1);
	if (temp == L"Sparks")
	{
		auto spark = dynamic_cast<Sparks*>(particle);
		if (spark == nullptr)
		{
			particle = new Sparks(device, ID);
			spark = dynamic_cast<Sparks*>(particle);
		}
		particleType = ReadParticleType::Spark;
		BinaryReader* r = new BinaryReader();

		r->Open(file);
		spark->simulateDesc.numParticles = r->UInt();
		spark->simulateDesc.softeningSquared = r->Float();
		spark->simulateDesc.distance = r->Float();
		spark->clusterScale = r->Float();

		spark->velocityScale = r->Float();

		spark->wvpDesc.intensity = r->Float();
		spark->wvpDesc.pontSize = r->Float();

		spark->simulateDesc.timer = r->Float();
		string temp = r->String();
		if (temp.length() > 0)
		{
			SafeDelete(particleTexture);
			particleTexture = new Texture();
			particleTexture->Load(device, String::ToWString(temp));
			textureName = String::ToWString(temp);
		}
		BindEffectBone=r->Int();
		switch (BindEffectBone)
		{
		case 0:
			BoneCollider = "BodyCollider";
			break;
		case 1:
			BoneCollider = "HeadCollider";
			break;
		case 2:
			BoneCollider = "SwordCollider";
			break;
		}
		r->Close();

		SafeDelete(r);
	}
	else if (temp == L"Smoke")
	{
		auto smoke = dynamic_cast<SoftParticle*>(particle);
		if (smoke == nullptr)
		{
			particle = new SoftParticle(device, ID);
			smoke = dynamic_cast<SoftParticle*>(particle);
		}
		particleType = ReadParticleType::Smoke;
		BinaryReader* r = new BinaryReader();

		r->Open(file);
		smoke->simulateDesc.numParticles = r->UInt();
		smoke->simulateDesc.ParticleLifeSpan = r->Float();
		smoke->simulateDesc.EmitRate = r->Float();
		smoke->simulateDesc.ParticleMaxSize = r->Float();
		smoke->simulateDesc.ParticleMinSize = r->Float();
		smoke->simulateDesc.articleVel = r->Float();

		smoke->simulateDesc.dir = r->Vector3();
		smoke->simulateDesc.factor = r->Vector3();
		smoke->simulateDesc.World = r->Matrix();
		
		r->Close();

		SafeDelete(r);
	}
	
	
	if (IsLevel)
	{
		Compile();
	}

	
}

void ParticleEditor::SetTexture(const wstring & file)
{
	SafeDelete(particleTexture);
	particleTexture = new Texture();
	particleTexture->Load(device,file);
	textureName = file;
}
