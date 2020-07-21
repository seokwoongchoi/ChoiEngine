#include "stdafx.h"
#include "PreviewRenderer.h"
#include "Core/D3D11/D3D11_Helper.h"
#include "Utility/Xml.h"
#include "Resources/Mesh.h"
#include "ProgressBar/ProgressBar.h"
#include "ProgressBar/ProgressReport.h"
PreviewRenderer::PreviewRenderer(ID3D11Device * device)
	:device(device), inputLayout(nullptr), VS(nullptr), PS(nullptr), vertexBuffer(nullptr),
indexBuffer(nullptr), previewDesc{}, eyeDesc{}, eyeBuffer(nullptr), rsState(nullptr),
sampLinear(nullptr), stride(0), bones{}, materials{}, meshCount(0), boneCount(0), worldBuffer(nullptr), nullBuffer(nullptr),
slot(0), offset(0), texture(nullptr), srv(nullptr), nullSRV(nullptr), targetPosition(0, 0, 0), R(-12.57f, -5.23f), distance(127.0f), view{}, proj{},
skyIRSRV(nullptr), bLoaded(false), viewRight(0, 0, 0), viewUp(0, 0, 0), blendCount(0), blendMeshIndex(0),
AdditiveBlendState(nullptr), bLoadingStart(false)

{
	preintegratedFG = make_shared< Texture>();
	preintegratedFG->Load(device, L"PBR/PreintegratedFG.bmp");

	const wstring& temp = L"../../_Textures/Environment/SunsetCube1024.dds";
	//wstring temp = L"../../_Textures/Environment/sky_ocean.dds";

	D3DX11CreateShaderResourceViewFromFile
	(
		device, temp.c_str(), NULL, NULL, &skyIRSRV, NULL
	);

	//SafeRelease(skyIRSRV);
	CreateConstantBuffers();
	CreateStates();
	progress = make_shared< ProgressBar>();
}
PreviewRenderer::~PreviewRenderer()
{
}

void PreviewRenderer::SaveMeshFile(const wstring & name, const ReadMeshType & meshType)
{
	//Thread::Get()->AddTask([&]()
	//{
		BinaryWriter* w = new BinaryWriter();

		wstring path;
		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{
			path = L"../_Models/StaticMeshes/" + name + L"/" + name + L".meshedit";
		}
		break;
		case ReadMeshType::SkeletaMesh:
		{
			path = L"../_Models/SkeletalMeshes/" + name + L"/" + name + L".meshedit";
		}
		break;

		}
		w->Open(path);

		w->UInt(bones.size());
		for (auto& bone : bones)
		{
			w->Int(bone->index);
			w->String(bone->name);
			w->Int(bone->parentIndex);
			w->Matrix(bone->transform);


		}

		w->UInt(meshCount - blendCount);
		w->UInt(blendCount);
		if (blendCount > 0)
			w->UInt(blendMeshIndex);

		uint totalVertices = 0;
		uint totalIndices = 0;
		for (uint i = 0; i < meshCount; i++)
		{
			auto& meshData = meshes[i];

			w->String(meshData->name);
			w->Int(meshData->boneIndex);

			w->String(meshData->materialName);

			w->UInt(meshData->vertexCount);
			totalVertices += meshData->vertexCount;
			w->UInt(meshData->startVertexIndex);
			w->UInt(meshData->indexCount);
			totalIndices += meshData->indexCount;
			w->UInt(meshData->startIndex);

			w->Float(meshData->minPos.x);
			w->Float(meshData->minPos.y);
			w->Float(meshData->minPos.z);


			w->Float(meshData->maxPos.x);
			w->Float(meshData->maxPos.y);
			w->Float(meshData->maxPos.z);

		}
		w->UInt(totalVertices);
		w->UInt(totalIndices);
		w->Byte(&staticVertices[0], sizeof(VertexTextureNormalTangent) * totalVertices);

		w->Byte(&indices[0], sizeof(uint) *totalIndices);
		w->Close();
		SafeDelete(w);
	//});
}

void PreviewRenderer::ReadMesh(const wstring & name, const ReadMeshType & meshType)
{
	
	Thread::Get()->AddTask([&]() 
	{
		bLoadingStart = true;
		bLoaded = false;
		BinaryReader* r = new BinaryReader();
		// VerdeResidence/VerdeResidence.mesh";
		wstring path;
		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{
			path = L"../_Models/StaticMeshes/" + name + L"/" + name + L".mesh";
		}
		break;
		case ReadMeshType::SkeletaMesh:
		{
			path = L"../_Models/SkeletalMeshes/" + name + L"/" + name + L".mesh";
		}
		break;

		}

		r->Open(path);
		boneCount = 0;
		bones.clear();
		bones.shrink_to_fit();
		uint tempBoneCount = r->UInt();
		ProgressReport::Get().SetJobCount(ProgressReport::Model,  tempBoneCount );
		for (UINT i = 0; i < tempBoneCount; i++)
		{
			auto& bone = make_shared< ModelBone>();
			bone->index = r->Int();
			bone->name = r->String();
			bone->parentIndex = r->Int();
			bone->transform = r->Matrix();
			bones.emplace_back(bone);
			boneCount++;

		
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}

	
		meshes.clear();
		meshes.shrink_to_fit();


		uint tempMeshCount = r->UInt();
		ProgressReport::Get().SetJobCount(ProgressReport::Model, (tempMeshCount*100)+1);
		

		uint offset = 0;
		uint indexOffset = 0;
		for (UINT i = 0; i < tempMeshCount; i++)
		{
			auto& mesh = make_shared<Mesh>();

			mesh->name = r->String();
			mesh->boneIndex = r->Int();
			mesh->materialName = r->String();


			
			//VertexData
			{
				mesh->vertexCount = r->UInt();
			
				vector< VertexTextureNormalTangentBlend>vertices;
				vertices.assign(mesh->vertexCount, VertexTextureNormalTangentBlend());
				void* ptr = (void *)(vertices.data());
				r->Byte(&ptr, sizeof(VertexTextureNormalTangentBlend) * mesh->vertexCount);

				mesh->startVertexIndex = offset;
				switch (meshType)
				{
				case ReadMeshType::StaticMesh:
				{
					staticVertices.insert(staticVertices.end(),mesh->vertexCount, VertexTextureNormalTangent());
					for (uint c = 0; c < mesh->vertexCount; c++)
					{
						staticVertices[offset + c].Position = vertices[c].Position;
						staticVertices[offset + c].Uv = vertices[c].Uv;
						staticVertices[offset + c].Normal = vertices[c].Normal;
						staticVertices[offset + c].Tangent = vertices[c].Tangent;
					}
					
				}
				break;
				case ReadMeshType::SkeletaMesh:
				{
					skeletalVertices.insert(skeletalVertices.begin() + offset,vertices.begin(),vertices.end());
					
				}
				break;

				}
				offset += mesh->vertexCount;

				vertices.clear();
				vertices.shrink_to_fit();
			}

		
			//IndexData
		
			{
				mesh->indexCount = r->UInt();
				
				vector< uint>indices;
				indices.assign(mesh->indexCount, uint());
				void* ptr = (void *)&(indices[0]);
				r->Byte(&ptr, sizeof(uint) * mesh->indexCount);

				mesh->startIndex = indexOffset;
				this->indices.insert(this->indices.end(), mesh->indexCount, uint());
				for (uint c = 0; c < mesh->indexCount; c++)
				{
					this->indices[indexOffset + c] = indices[c];
				}
				indexOffset += mesh->indexCount;
				indices.clear();
				indices.shrink_to_fit();

			}
			mesh->minPos.x = r->Float();
			mesh->minPos.y = r->Float();
			mesh->minPos.z = r->Float();
			mesh->maxPos.x = r->Float();
			mesh->maxPos.y = r->Float();
			mesh->maxPos.z = r->Float();

			
			meshes.emplace_back(mesh);
			meshCount++;
			for (uint i = 0; i < 100; i++)
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}//for(i)

		
		r->Close();
		SafeDelete(r);
		BindingBone(meshType);

		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{
			BindingStaticMesh();
		}
		break;
		case ReadMeshType::SkeletaMesh:
		{
			BindingSkeletaMesh();
		}
		break;
		}
		ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		bLoadingStart = false;
		bLoaded = true;

	});
	

}

void PreviewRenderer::ReadMaterial(const wstring & name)
{
	materials.clear();
	materials.shrink_to_fit();
	auto path = L"../../_Textures/" + name + L"/" + name + L".material";

	shared_ptr<Xml::XMLDocument> document = make_shared< Xml::XMLDocument>();
	Xml::XMLError error = document->LoadFile(String::ToString(path).c_str());
	assert(error == Xml::XML_SUCCESS);

	Xml::XMLElement* root = document->FirstChildElement();
	Xml::XMLElement* materialNode = root->FirstChildElement();

	do
	{
		auto& material = make_shared< Material>(device);


		Xml::XMLElement* node = NULL;

		node = materialNode->FirstChildElement();
		material->Name(String::ToWString(node->GetText()));


		wstring directory = Path::GetDirectoryName(path);
		String::Replace(&directory, L"../../_Textures", L"");

		wstring texture = L"";

		node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->DiffuseMap(directory + texture);

		node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->SpecularMap(directory + texture);

		node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->NormalMap(directory + texture);

		node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->RoughnessMap(directory + texture);


		node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->MatallicMap(directory + texture);

		Color color;

		node = node->NextSiblingElement();
		color.r = node->FloatAttribute("R");
		color.g = node->FloatAttribute("G");
		color.b = node->FloatAttribute("B");
		color.a = node->FloatAttribute("A");
		material->Ambient(color);

		node = node->NextSiblingElement();
		color.r = node->FloatAttribute("R");
		color.g = node->FloatAttribute("G");
		color.b = node->FloatAttribute("B");
		color.a = node->FloatAttribute("A");
		material->Diffuse(color);

		node = node->NextSiblingElement();
		color.r = node->FloatAttribute("R");
		color.g = node->FloatAttribute("G");
		color.b = node->FloatAttribute("B");
		color.a = node->FloatAttribute("A");
		material->Specular(color);

		/*	node = node->NextSiblingElement();
			color.r = node->FloatAttribute("R");
			color.g = node->FloatAttribute("G");
			color.b = node->FloatAttribute("B");
			color.a = node->FloatAttribute("A");
			material->Roughness(color);

			node = node->NextSiblingElement();
			color.r = node->FloatAttribute("R");
			color.g = node->FloatAttribute("G");
			color.b = node->FloatAttribute("B");
			color.a = node->FloatAttribute("A");
			material->Matallic(color);*/
			//material->Shininess(node->FloatText());

		materials.emplace_back(material);


		materialNode = materialNode->NextSiblingElement();
	} while (materialNode != NULL);
	
}

void PreviewRenderer::ReadClip(const wstring & name)
{
	/*auto& file = L"../../_Models/SkeletalMeshes/" + name + L".clip";

	BinaryReader* r = new BinaryReader();
	r->Open(file);


	ModelClip* clip = new ModelClip();

	clip->name = String::ToWString(r->String());
	clip->duration = r->Float();
	clip->frameRate = r->Float();
	clip->frameCount = r->UInt();

	UINT keyframesCount = r->UInt();
	for (UINT i = 0; i < keyframesCount; i++)
	{
		ModelKeyframe* keyframe = new ModelKeyframe();
		keyframe->BoneName = String::ToWString(r->String());

		UINT size = r->UInt();
		if (size > 0)
		{
			keyframe->Transforms.assign(size, ModelKeyframeData());

			void* ptr = (void *)&keyframe->Transforms[0];
			r->Byte(&ptr, sizeof(ModelKeyframeData) * size);
		}

		clip->keyframeMap[keyframe->BoneName] = keyframe;

	}

	r->Close();
	SafeDelete(r);
*/
	//clips.push_back(clip);
}

void PreviewRenderer::DeleteMesh(const uint & index)
{

	int meshIndex=-1;
	for (uint i=0;i<meshes.size();i++)
	{
		if (meshes[i]->boneIndex == index)
		{
			meshIndex = i;
		}
	}
	if (meshIndex < 0)return;
	uint startVertexIndex = meshes[meshIndex]->startVertexIndex;
	uint vertexCount = meshes[meshIndex]->vertexCount;
	uint startIndex = meshes[meshIndex]->startIndex;
	uint indexCount= meshes[meshIndex]->indexCount;

	staticVertices.erase(staticVertices.begin() + startVertexIndex, staticVertices.begin() + startVertexIndex + vertexCount);
	
	indices.erase(indices.begin() + startIndex, indices.begin() + startIndex+ indexCount);




	auto remove=remove_if(meshes.begin(), meshes.end(), [&index](shared_ptr<Mesh> mesh) 
	{
		return mesh->boneIndex == index;
	});
	if (remove != meshes.end())
	meshes.erase(remove);
	meshCount -= 1;
	for (uint i = meshIndex; i < meshCount; i++)
	{
		meshes[i]->startVertexIndex -= vertexCount;
		meshes[i]->startIndex -= indexCount;
		meshes[i]->boneIndex -= 1;
	}

	auto boneRemove = remove_if(bones.begin(), bones.end(), [&index](shared_ptr < ModelBone> bone)
	{
		return bone->index == index;
	});
	if (boneRemove != bones.end())
	bones.erase(boneRemove);

	boneCount -= 1;
	for (uint i = index; i < boneCount; i++)
	{
		bones[i]->index -= 1;

		//if(index>bones[i]->parentIndex)
		bones[i]->parentIndex -= 1;
	
	}

	BindingStaticMesh();
	CreateModelTransformSRV();

}

void PreviewRenderer::BlendMesh(const uint & index)
{

}

void PreviewRenderer::Update(const Vector2& size)
{
	Vector2 val = Vector2(ImGui::GetMouseDragDelta(1).x, ImGui::GetMouseDragDelta(1).y);

	

	if (ImGui::IsAnyWindowHovered())
	{
		if (ImGui::IsMouseDown(1))
		{

			R.y += (val.y / 20)*0.008f;
			R.x += (val.x / 20)*0.008f;
			
		}

		if (ImGui::IsKeyPressed('E') )
		{
			targetPosition += viewUp * 0.8f;
		
		}
		else if (ImGui::IsKeyPressed('Q'))
		{
			targetPosition -= viewUp * 0.8f;
		}

		if (ImGui::IsKeyPressed('D') )
		{
			targetPosition += viewRight * 0.8f;
		}
		else if (ImGui::IsKeyPressed('A') )
		{
			targetPosition -= viewRight * 0.8f;
		
		}
		if (ImGui::IsKeyPressed('W'))//&& distance > 0
		{
			distance -= 1.0f;
		}
		else if (ImGui::IsKeyPressed('S') && distance < 300)
		{
			distance += 1.0f;
		}

	}

	

	eyeDesc.EyePosition.x = targetPosition.x + distance * sinf(R.y)*cosf(R.x);
	eyeDesc.EyePosition.y = targetPosition.y + distance * cosf(R.y);
	eyeDesc.EyePosition.z = targetPosition.z + distance * sinf(R.y)*sinf(R.x);

	//ImGui::Begin("Debug");
	//{
	//	const string& str1 = string("Dist: ") + to_string(distance) + "/" + string("Y: ") + to_string(R.y) + "/" + string("X: ") + to_string(R.x);
	//	ImGui::Text(str1.c_str());
	//}
	//ImGui::End();
	D3DXMatrixLookAtLH(&view, &eyeDesc.EyePosition, &targetPosition, &Vector3(0, 1, 0));

	float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);
	D3DXMatrixPerspectiveFovLH(&proj, static_cast<float>(D3DX_PI)* 0.25f, aspect, 0.1f, 1000.0f);

	
	D3DXMatrixScaling(&previewDesc.W, 0.01f, 0.01f, 0.01f);
	previewDesc.VP = view * proj;
	Matrix viewInv;
	D3DXMatrixInverse(&viewInv, nullptr, &view);
	viewRight = Vector3(viewInv._11, viewInv._12, viewInv._13);
	viewUp = Vector3(viewInv._21, viewInv._22, viewInv._23);

	D3DXMatrixTranspose(&previewDesc.W, &previewDesc.W);
	D3DXMatrixTranspose(&previewDesc.VP, &previewDesc.VP);
}

void PreviewRenderer::Render(ID3D11DeviceContext * context)
{
	if (bLoadingStart)
	{
		progress->Begin();
		progress->Render();

	}

	if (!bLoaded) return;
	context->VSSetShader(VS, nullptr, 0);
	context->PSSetShader(PS, nullptr, 0);


	ID3D11ShaderResourceView* arrSRV[2] = { *preintegratedFG ,skyIRSRV };
	context->PSSetShaderResources(4, 2, arrSRV);
	context->PSSetSamplers(0, 1, &sampLinear);
	//context->RSSetState(nullptr);
	{
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(worldBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &previewDesc, sizeof(previewDesc));
		context->Unmap(worldBuffer, 0);

		context->VSSetConstantBuffers(0, 1, &worldBuffer);
	}
	{
		D3D11_MAPPED_SUBRESOURCE MappedResource;
	    context->Map(eyeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	    memcpy(MappedResource.pData, &eyeDesc, sizeof(eyeDesc));
	    context->Unmap(eyeBuffer, 0);

		context->PSSetConstantBuffers(1, 1, &eyeBuffer);
	}
	
	context->VSSetShaderResources(0, 1, &srv);




	context->IASetVertexBuffers(slot, 1, &vertexBuffer, &stride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	for (uint i = 0; i < meshCount; i++)
	{
		auto& m = meshes[i];
		m->ApplyPipeline(context);
		context->DrawIndexed(m->indexCount,  m->startIndex, m->startVertexIndex);
		m->ClearPipelineMaterial(context);

	}


	context->VSSetShaderResources(0, 1, &nullSRV);
	context->VSSetConstantBuffers(0, 1, &nullBuffer);
	context->PSSetConstantBuffers(1, 1, &nullBuffer);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

void PreviewRenderer::CreateSahders(const string& file)
{
	SafeRelease(VS);
	SafeRelease(PS);

	ID3DBlob* ShaderBlob = NULL;
	auto path = "../_Shaders/PreviewModel.hlsl";
	auto entryPoint = "StaticVS";
	auto shaderModel = "vs_5_0";

	auto result = D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	);

	if (!result)
		assert(false);

	auto hr = device->CreateVertexShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&VS
	);
	assert(SUCCEEDED(hr));
	inputLayout = new InputLayout();
	inputLayout->Create(device, ShaderBlob);
	SafeRelease(ShaderBlob);


	entryPoint = "StaticPS";
	shaderModel = "ps_5_0";
	result = D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	);

	if (!result)
		assert(false);

	hr = device->CreatePixelShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&PS
	);
	assert(SUCCEEDED(hr));
	SafeRelease(ShaderBlob);
}

void PreviewRenderer::CreateStates()
{
	SafeRelease(sampLinear);

	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &sampLinear));

	SafeRelease(AdditiveBlendState);





	// Create the additive blend state
	D3D11_BLEND_DESC descBlend;
	descBlend.AlphaToCoverageEnable = FALSE;
	descBlend.IndependentBlendEnable = FALSE;
	const D3D11_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		TRUE,
		D3D11_BLEND_DEST_COLOR, D3D11_BLEND_SRC_COLOR, D3D11_BLEND_OP_ADD, //srcBlend,descBlend,BlendOp
		D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,//srcBlendAlpha,destBlendAlpha,BlendOpAlpha
		D3D11_COLOR_WRITE_ENABLE_ALL,//rendertargetWriteMask
	};



	for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		descBlend.RenderTarget[i] = defaultRenderTargetBlendDesc;


	Check(device->CreateBlendState(&descBlend, &AdditiveBlendState));
}

void PreviewRenderer::BindingBone(const ReadMeshType& meshType)
{
	for (auto& bone : bones)
	{
		if (bone->parentIndex > -1)
		{
			bone->parent = bones[bone->parentIndex];
			bone->parent->childs.push_back(bone);
		}
		else
			bone->parent = NULL;
	}
	switch (meshType)
	{
	case ReadMeshType::StaticMesh:
	{
		//vsTextureSlot = 0;
		CreateModelTransformSRV();
	}
	break;
	case ReadMeshType::SkeletaMesh:
	{
		//vsTextureSlot = 1;
	}
	break;
	}
	
}

void PreviewRenderer::BindingStaticMesh()
{
	
	stride = sizeof(VertexTextureNormalTangent);
	
	uint totalCount = 0;
	uint totalIndexCount = 0;
		

	uint offset = 0;
	uint indexOffset = 0;
	for (uint i = 0; i < meshCount; i++)
	{
		totalCount += meshes[i]->vertexCount;
		totalIndexCount += meshes[i]->indexCount;

		

		const uint& tempIndex = meshes[i]->boneIndex;
		auto findBone = find_if(bones.begin(), bones.end(), [=, &tempIndex](shared_ptr<ModelBone> bone)
		{
			return bone->index == tempIndex;

		});
		if (findBone != bones.end())
		{
			meshes[i]->bone = *findBone;
		}

		meshes[i]->CreateBuffer(device);
		for (auto& material : materials)
		{
			if (meshes[i]->materialName == String::ToString(material->Name()))
			{
				meshes[i]->material = material;
			}
		}
	}

	materials.clear();
	materials.shrink_to_fit();

	SafeRelease(vertexBuffer);
	SafeRelease(indexBuffer);

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(VertexTextureNormalTangent) * totalCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = staticVertices.data();
	Check(device->CreateBuffer(&desc, &subResource, &vertexBuffer));

	
	

	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(uint) * totalIndexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = indices.data();
	Check(device->CreateBuffer(&desc, &subResource, &indexBuffer));




}

void PreviewRenderer::BindingSkeletaMesh()
{
}

void PreviewRenderer::CreateConstantBuffers()
{
	// Allocate constant buffers
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(PreviewDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &worldBuffer));

	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(EyeDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &eyeBuffer));
}

void PreviewRenderer::CreateModelTransformSRV()
{
	SafeRelease(texture);
	SafeRelease(srv);
	//CreateTexture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.Width = boneCount * 4;
		desc.Height = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		Matrix* temp = new Matrix[boneCount];
		Matrix* boneTransforms = new Matrix[boneCount];

		for (UINT i = 0; i < 1; i++)
		{
			for (UINT b = 0; b < boneCount; b++)
			{
				auto& bone = bones[b];

				Matrix parent;
				int parentIndex = bone->ParentIndex();

				if (parentIndex < 0)
					D3DXMatrixIdentity(&parent);

				else
					parent = temp[parentIndex];

				Matrix matrix = bone->Transform();
				temp[b] = parent;
				uint index = i * boneCount + b;
				boneTransforms[index] = matrix * temp[b];

			}//for(b)
		}//for(i)

		SafeDeleteArray(temp);
	


		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = boneTransforms;
		subResource.SysMemPitch = sizeof(Matrix);
		subResource.SysMemSlicePitch = 0;


		Check(device->CreateTexture2D(&desc, &subResource, &texture));


		SafeDeleteArray(boneTransforms);

		//free(p);
	}


	//Create SRV
	{
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = desc.Format;

		Check(device->CreateShaderResourceView(texture, &srvDesc, &srv));
	}


}

void PreviewRenderer::CreateAnimTransformSRV()
{
	SafeRelease(texture);
	SafeRelease(srv);
	//CreateTexture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.Width = boneCount * 4;
		desc.Height = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		Matrix* temp = new Matrix[boneCount];
		Matrix* boneTransforms = new Matrix[boneCount];

		for (UINT i = 0; i < 1; i++)
		{
			for (UINT b = 0; b < boneCount; b++)
			{
				auto& bone = bones[b];

				Matrix parent;
				int parentIndex = bone->ParentIndex();

				if (parentIndex < 0)
					D3DXMatrixIdentity(&parent);

				else
					parent = temp[parentIndex];

				Matrix matrix = bone->Transform();
				temp[b] = parent;
				uint index = i * boneCount + b;
				boneTransforms[index] = matrix * temp[b];

			}//for(b)
		}//for(i)

		SafeDeleteArray(temp);



		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = boneTransforms;
		subResource.SysMemPitch = sizeof(Matrix);
		subResource.SysMemSlicePitch = 0;


		Check(device->CreateTexture2D(&desc, &subResource, &texture));


		SafeDeleteArray(boneTransforms);

		//free(p);
	}


	//Create SRV
	{
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Format = desc.Format;

		Check(device->CreateShaderResourceView(texture, &srvDesc, &srv));
	}

}
