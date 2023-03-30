#include "stdafx.h"
#include "PreviewRenderer.h"
#include "Core/D3D11/D3D11_Helper.h"
#include "Utility/Xml.h"
#include "Resources/Mesh.h"
#include "ProgressBar/ProgressReport.h"
#include "ProgressBar/ProgressBar.h"
#include "ActorEditor.h"



	
PreviewRenderer::PreviewRenderer(ID3D11Device * device)
	:device(device), inputLayout(nullptr), VS(nullptr), PS(nullptr),
	vertexBuffer(nullptr), indexBuffer(nullptr), previewDesc{}, eyeDesc{},
	eyeBuffer(nullptr), rsState(nullptr),
	sampLinear(nullptr), stride(0), bones{}, materials{}, meshCount(0),
	boneCount(0), worldBuffer(nullptr), nullBuffer(nullptr),
	slot(0), offset(0), texture(nullptr), srv(nullptr), nullSRV(nullptr),
	targetPosition(0, 0, 0), rotation(-12.57f, -5.23f), distance(10.0f),
	view{}, proj{}, skyIRSRV(nullptr), bLoaded(false), viewRight(0, 0, 0),
	viewUp(0, 0, 0), blendCount(0), blendMeshIndex(0), tweenBuffer(nullptr), tweenDesc{},
	AdditiveBlendState(nullptr), skinTransforms(nullptr), bPause(true), saveParentMatrix(nullptr), maxkeyframe(0),
	invGlobal{}, parent{}, S{}, R{}, T{}, animation{}, nuArmedBoneCount(0),p(nullptr), meshType(ReadMeshType::Default),
	boxCount(0), behaviorTreeIndex(-1), bHasCam(false)
{
	D3DXMatrixIdentity(&boxWorld);
	preintegratedFG = make_shared< Texture>();
	preintegratedFG->Load(device, L"PBR/PreintegratedFG.bmp", nullptr, true);

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

	D3DXMatrixScaling(&previewDesc.W, 0.01f, 0.01f, 0.01f);
	D3DXMatrixTranspose(&previewDesc.W, &previewDesc.W);
	
	boxMin = Vector3(
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity()
	);
	boxMax = Vector3(
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity()
	);

	for (uint i = 0; i < 3; i++)
		D3DXMatrixIdentity(&colliderBoxData[i].ColliderBoxWorld);
}



PreviewRenderer::~PreviewRenderer()
{
}

void PreviewRenderer::SaveMeshFile(const wstring & name, const ReadMeshType & meshType)
{
	ProgressReport::Get().SetJobCount(ProgressReport::Model, 1000);

		BinaryWriter* w = new BinaryWriter();


		wstring path;
		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{
			path = L"../_Models/StaticMeshes/" + name + L"/" + name + L"_Edit" + L".mesh";
		}
		break;
		case ReadMeshType::SkeletalMesh:
		{
			path = L"../_Models/SkeletalMeshes/" + name + L"/" + name + L"_Edit" + L".mesh";
		}
		break;
		}
		w->Open(path);

		ProgressReport::Get().SetJobCount(ProgressReport::Model, bones.size());
		w->UInt(bones.size());
		for (auto& bone : bones)
		{
			w->Int(bone->index);
			w->String(bone->name);
			w->Int(bone->parentIndex);
			w->Matrix(bone->transform);

			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}

		w->UInt(meshCount - blendCount);
		w->UInt(blendCount);
		if (blendCount > 0)
			w->UInt(blendMeshIndex);

		uint totalVertices = 0;
		uint totalIndices = 0;
		ProgressReport::Get().SetJobCount(ProgressReport::Model, (meshCount * 1000) + 1000);
		for (uint i = 0; i < meshCount; i++)
		{
			auto& meshData = meshes[i];

			w->String(meshData->name);
			w->Int(meshData->boneDesc.boneIndex);

			w->String(meshData->materialName);

			w->UInt(meshData->vertexCount);
			totalVertices += meshData->vertexCount;
			w->UInt(meshData->startVertexIndex);
			w->UInt(meshData->indexCount);
			totalIndices += meshData->indexCount;
			w->UInt(meshData->startIndex);
			
			
			
			for (uint i = 0; i < 500; i++)
				ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}

		w->UInt(totalVertices);
		w->UInt(totalIndices);
		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{
			w->Byte(&staticVertices[0], sizeof(VertexTextureNormalTangent) * totalVertices);
		}
		break;
		case ReadMeshType::SkeletalMesh:
		{
			w->Byte(&skeletalVertices[0], sizeof(VertexTextureNormalTangentBlend) * totalVertices);
		}
		break;
		}
		for (uint i = 0; i < ((meshCount * 1000)+1000)-(500*meshCount)+1001; i++)
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);

		w->Byte(&indices[0], sizeof(uint) *totalIndices);


		Matrix inv;
		Matrix W;
		D3DXMatrixTranspose(&W, &previewDesc.W);
		D3DXMatrixInverse(&inv, nullptr, &W);
	
		Vector3 min, max;
		D3DXVec3TransformCoord(&min, &boxMin, &(boxWorld*inv));
		D3DXVec3TransformCoord(&max, &boxMax, &(boxWorld*inv));
	
		w->Float(min.x);
		w->Float(min.y);
		w->Float(min.z);

		w->Float(max.x);
		w->Float(max.y);
		w->Float(max.z);

		w->UInt(boxCount);
		if (boxCount > 0)
		{			
			for (uint i = 0; i < boxCount; i++)
			{
				w->UInt(colliderBoxData[i].Index);
				w->Matrix(colliderBoxData[i].ColliderBoxWorld);
			    w->Matrix(bones[colliderBoxData[i].Index]->Transform());
			}
		}
		w->Int(behaviorTreeIndex);

		w->Bool(bHasCam);

		w->Close();
		SafeDelete(w);

}

void PreviewRenderer::SaveMaterialFile(const wstring & name)
{
	auto savePath = L"../_Textures/" + name+L"/"+ name + L".material";

	string folder = String::ToString(Path::GetDirectoryName(savePath));
	string file = String::ToString(Path::GetFileName(savePath));

	Path::CreateFolders(folder);

	Xml::XMLDocument* document = new Xml::XMLDocument();

	Xml::XMLDeclaration* decl = document->NewDeclaration();
	document->LinkEndChild(decl);

	Xml::XMLElement* root = document->NewElement("Materials");
	root->SetAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	root->SetAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
	document->LinkEndChild(root);

	uint blendIndex = 0;
	for (uint i=0;i<meshCount;i++)
	{
		auto& material = meshes[i]->material;
		
		if (blendCount>0&&i == blendMeshIndex)
		{
			material = blendMeshes[blendIndex]->material;
			blendIndex++;
		}
		
		Xml::XMLElement*node = document->NewElement("Material");
		root->LinkEndChild(node);

		Xml::XMLElement* element = NULL;

		element = document->NewElement("Name");
		string name = String::ToString(material->Name());
		element->SetText(name.c_str());
		node->LinkEndChild(element);

		element = document->NewElement("DiffuseFile");
		string& diffuseFile = String::ToString(material->DiffuseFile());
		element->SetText(diffuseFile.c_str());
		node->LinkEndChild(element);
		
		element = document->NewElement("NormalFile");
		string& normalFile = String::ToString(material->NormalFile());
		element->SetText(normalFile.c_str());
		node->LinkEndChild(element);
		element = document->NewElement("RoughnessFile");
		string& roughnessFile = String::ToString(material->RoughnessFile());
		element->SetText(roughnessFile.c_str());
		node->LinkEndChild(element);
		element = document->NewElement("MetallicFile");
		string& metallicFile = String::ToString(material->MetallicFile());
		element->SetText(metallicFile.c_str());
		node->LinkEndChild(element);
	

		element = document->NewElement("Diffuse");
		element->SetAttribute("R", material->Diffuse().r);
		element->SetAttribute("G", material->Diffuse().g);
		element->SetAttribute("B", material->Diffuse().b);
		element->SetAttribute("A", material->Diffuse().a);
		node->LinkEndChild(element);

		element = document->NewElement("PBR");
		element->SetAttribute("Roughness", material->Roughness());
		element->SetAttribute("Metallic", material->Metallic());
		node->LinkEndChild(element);
			
		
	}
	
	document->SaveFile((folder + file).c_str());
}

void PreviewRenderer::ReadMesh(const wstring & file, const wstring & modelName, const ReadMeshType & meshType)
{
	this->meshType = meshType;
	staticVertices.clear();
	staticVertices.shrink_to_fit();
	skeletalVertices.clear();
	skeletalVertices.shrink_to_fit();


	ProgressReport::Get().SetJobCount(ProgressReport::Model,1000);
	
	Thread::Get()->AddTask([&, file, modelName, meshType]()
	{
	
		bLoaded = false;
		BinaryReader* r = new BinaryReader();
	

		r->Open(file);
		boneCount = 0;
		bones.clear();
		bones.shrink_to_fit();
		uint tempBoneCount = r->UInt();
		
		ProgressReport::Get().SetJobCount(ProgressReport::Model, tempBoneCount);
		for (UINT i = 0; i < tempBoneCount; i++)
		{
			auto& bone = make_shared< ModelBone>();
			bone->index = r->Int();
			bone->name = r->String();
			bone->parentIndex = r->Int();
			bone->transform = r->Matrix();

			//size_t getAssimpBone = bone->name.find_first_of('$');
			//if (getAssimpBone == string::npos)
			{
				bones.emplace_back(bone);
				boneCount++;
			}
			//else
			//{
			//	int a = 0;
			//}
			

			
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}
		nuArmedBoneCount = boneCount;
		

		meshes.clear();
		meshes.shrink_to_fit();
		
		meshCount = 0;
		uint tempMeshCount = r->UInt();
	
		ProgressReport::Get().SetJobCount(ProgressReport::Model, ( tempMeshCount*1000)+1000);

		for (uint i = 0; i < 1000; i++)
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		uint offset = 0;
		uint indexOffset = 0;
		for (UINT i = 0; i < tempMeshCount; i++)
		{
			auto& mesh = make_shared<Mesh>();

			mesh->name = r->String();
			mesh->boneDesc.boneIndex = r->Int();
			
			
			
			mesh->materialName = r->String();
			cout << mesh->materialName << endl;

			
			//VertexData
			{
				mesh->vertexCount = r->UInt();
			
				vector< VertexTextureNormalTangentBlend>vertices;
				vertices.assign(mesh->vertexCount, VertexTextureNormalTangentBlend());
				void* ptr = reinterpret_cast<void *>(vertices.data());
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
				case ReadMeshType::SkeletalMesh:
				{
					
					skeletalVertices.insert(skeletalVertices.end(),vertices.begin(),vertices.end());
					
				}
				break;

				}
				offset += mesh->vertexCount;

				vertices.clear();
				vertices.shrink_to_fit();
			}
			for (uint i = 0; i < 500; i++)
				ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		
			//IndexData
		
			{
				mesh->indexCount = r->UInt();
				
				vector< uint>tempIndices;
				tempIndices.assign(mesh->indexCount, uint());
				void* ptr = reinterpret_cast<void *>(tempIndices.data());
				r->Byte(&ptr, sizeof(uint) * mesh->indexCount);

				mesh->startIndex = indexOffset;
				this->indices.insert(this->indices.end(), mesh->indexCount, uint());
				for (uint c = 0; c < mesh->indexCount; c++)
				{
					this->indices[indexOffset + c] = tempIndices[c];
				}
				indexOffset += mesh->indexCount;
				tempIndices.clear();
				tempIndices.shrink_to_fit();

			}
			Vector3 min;
			Vector3 max;
			min.x = r->Float();
			min.y = r->Float();
			min.z = r->Float();
			max.x = r->Float();
			max.y = r->Float();
			max.z = r->Float();

			GetBoxSize(min, max);

			
			meshes.emplace_back(mesh);
			meshCount++;

			for (uint i = 0; i < 500; i++)
				ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
			
		}//for(i)
		
		
		r->Close();
		SafeDelete(r);
		BindingMaterialBone();

		ProgressReport::Get().SetJobCount(ProgressReport::Model,  1000);
		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{
			CreateModelTransformSRV();
			BindingStaticMesh();
		}
		break;
		case ReadMeshType::SkeletalMesh:
		{	
			
			BindingSkeletalMesh();
		}
		break;
		}


	
	 
		
		for (uint i = 0; i <2010; i++)
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);


		if (meshType == ReadMeshType::SkeletalMesh)
			ReadClip(modelName);
		bLoaded = true;

	});
	

}

void PreviewRenderer::ReadEditedMesh(const wstring & file, const wstring & modelName, const ReadMeshType & meshType)
{
	staticVertices.clear();
	staticVertices.shrink_to_fit();
	skeletalVertices.clear();
	skeletalVertices.shrink_to_fit();
	indices.clear();
	indices.shrink_to_fit();

	

	Thread::Get()->AddTask([&, file, modelName, meshType]()
	{
		ProgressReport::Get().SetJobCount(ProgressReport::Model, 8000);
		bLoaded = false;
		BinaryReader* r = new BinaryReader();

		
		r->Open(file);
		boneCount = 0;
		bones.clear();
		bones.shrink_to_fit();
		uint tempBoneCount = r->UInt();

		
		for (UINT i = 0; i < tempBoneCount; i++)
		{
			auto& bone = make_shared< ModelBone>();
			bone->index = r->Int();
			bone->name = r->String();
			bone->parentIndex = r->Int();
			bone->transform = r->Matrix();
			bones.emplace_back(bone);
			boneCount++;
	
			
			
			
		}
		for (uint i = 0; i < 2000; i++)
		{
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
	    }

		nuArmedBoneCount = boneCount;


		meshes.clear();
		meshes.shrink_to_fit();

		meshCount = 0;
		uint tempMeshCount = r->UInt();
	
		blendCount = r->UInt();
		tempMeshCount += blendCount;
		if (blendCount > 0)
		{
			blendMeshIndex = r->UInt();
		
		}

		
		uint offset = 0;
		uint indexOffset = 0;

		for (UINT i = 0; i < tempMeshCount; i++)
		{
			if (blendCount > 0 && i == blendMeshIndex)
			{
				for (uint b = 0; b < blendCount; b++)
				{
					shared_ptr<class Mesh>blendMesh = make_shared<class Mesh>();
					blendMesh->name = r->String();
					blendMesh->boneDesc.boneIndex = r->Int();
					blendMesh->materialName = r->String();
					blendMesh->vertexCount = r->UInt();
					blendMesh->startVertexIndex = r->UInt();
					blendMesh->indexCount = r->UInt();
					blendMesh->startIndex = r->UInt();

					blendMeshes.emplace_back(blendMesh);
					
					meshes.emplace_back(blendMesh);
					meshCount++;

					
				}
				continue;
			
			}
			auto& mesh = make_shared<Mesh>();

			mesh->name = r->String();
			mesh->boneDesc.boneIndex = r->Int();
			mesh->materialName = r->String();

		
			mesh->vertexCount = r->UInt();
			mesh->startVertexIndex = r->UInt();
							
			mesh->indexCount = r->UInt();
			mesh->startIndex = r->UInt();
		

			meshes.emplace_back(mesh);
			meshCount++;

		

		}//for(i)
		
		for (uint i = 0; i < 2000; i++)
		{
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}

		uint totalCount = r->UInt();
		uint totalIndexCount = r->UInt();
		indices.assign(totalIndexCount, uint());

		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{
			CreateModelTransformSRV();
			stride = sizeof(VertexTextureNormalTangent);
			staticVertices.assign(totalCount, VertexTextureNormalTangent());
					
			{
				void* ptr = reinterpret_cast<void*>(staticVertices.data());
				r->Byte(&ptr, sizeof(VertexTextureNormalTangent) * totalCount);

			}
		
			{
				void* ptr = reinterpret_cast<void*>(indices.data());
				r->Byte(&ptr, sizeof(uint) *totalIndexCount);

			}
			BindingStaticMesh();


		}
		break;
		case ReadMeshType::SkeletalMesh:
		{	
			
	        stride = sizeof(VertexTextureNormalTangentBlend);
	        skeletalVertices.assign(totalCount, VertexTextureNormalTangentBlend());
		
	        {
	        	void* ptr = reinterpret_cast<void*>(skeletalVertices.data());
	        	r->Byte(&ptr, sizeof(VertexTextureNormalTangentBlend) * totalCount);
	        
	        }
			
			{
				void* ptr = reinterpret_cast<void*>(indices.data());
				r->Byte(&ptr, sizeof(uint) *totalIndexCount);

			}
			
			BindingSkeletalMesh();
		
				
		}
		break;
		}

		for (uint i = 0; i < 2000; i++)
		{
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}
		{
		
			boxMin.x = r->Float();
			boxMin.y = r->Float();
			boxMin.z = r->Float();

			boxMax.x = r->Float();
			boxMax.y = r->Float();
			boxMax.z = r->Float();


			Matrix W;
			D3DXMatrixTranspose(&W, &previewDesc.W);
			D3DXVec3TransformCoord(&boxMin, &boxMin, &W);
			D3DXVec3TransformCoord(&boxMax, &boxMax, &W);

			uint  tempBoxCount= r->UInt();
			if (tempBoxCount > 0)
			{
				for (uint i = 0; i < tempBoxCount; i++)
				{
					colliderBoxData[i].Index=r->UInt();
					colliderBoxData[i].ColliderBoxWorld=r->Matrix();
					Matrix temp= r->Matrix();
					
				}
				boxCount = tempBoxCount;
			}

			
		
		}

		r->Close();
		SafeDelete(r);
	

		BindingMaterialBone();
		
		for (uint i = 0; i < 2000; i++)
		{
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}

		if(meshType==ReadMeshType::SkeletalMesh)
		ReadClip(modelName);
		///////////////////////////////////////////////////////////////////////
	
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
		{

		}
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
			material->MetallicMap(directory + texture);

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

void PreviewRenderer::ReadEditedMaterial(const wstring & name)
{
	materials.clear();
	materials.shrink_to_fit();

	auto path = L"../_Textures/" + name + L"/" + name + L".material";

	shared_ptr<Xml::XMLDocument> document = make_shared< Xml::XMLDocument>();
	Xml::XMLError error = document->LoadFile(String::ToString(path).c_str());
	assert(error == Xml::XML_SUCCESS);

	Xml::XMLElement* root = document->FirstChildElement();
	Xml::XMLElement* materialNode = root->FirstChildElement();

	do
	{
		auto& material = make_shared<Material>(device);


		Xml::XMLElement* node = NULL;

		node = materialNode->FirstChildElement();
		material->Name(String::ToWString(node->GetText()));


		wstring directory = Path::GetDirectoryName(path);
		String::Replace(&directory, L"../_Textures", L"");

		wstring texture = L"";

		node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->DiffuseMap(directory + texture, true);

		node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->NormalMap(directory + texture, true);

		node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->RoughnessMap(directory + texture, true);


		node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->MetallicMap(directory + texture, true);

		/*node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->HeightMap(directory + texture, true);*/

		Color color;

		node = node->NextSiblingElement();
		color.r = node->FloatAttribute("R");
		color.g = node->FloatAttribute("G");
		color.b = node->FloatAttribute("B");
		color.a = node->FloatAttribute("A");
		material->Diffuse(color);

		node = node->NextSiblingElement();
		float roughness = node->FloatAttribute("Roughness");
		material->Roughness(roughness);
		float metallic = node->FloatAttribute("Metallic");
		material->Metallic(metallic);

		materials.emplace_back(material);

		materialNode = materialNode->NextSiblingElement();
	} while (materialNode != NULL);
}

void PreviewRenderer::ReadClip(const wstring & name)
{
	wstring filePath = L"../_Models/SkeletalMeshes/" + name + L"/";
	vector<wstring> files;

	wstring filter = L"*.clip";
	Path::GetFiles(&files, filePath, filter, false);

	for (uint i = 0; i < files.size(); i++)
	{
		auto clipfileName = Path::GetFileName(files[i]);
		const auto& file = L"../_Models/SkeletalMeshes/" + name + L"/" + clipfileName;

		BinaryReader* r = new BinaryReader();
		r->Open(file);
		const auto& clip = make_shared< ModelClip>();

		clip->name = r->String();
		clip->duration = r->Float();
		clip->frameRate = r->Float();
		clip->frameCount = r->UInt();

		UINT keyframesCount = r->UInt();
		for (UINT i = 0; i < keyframesCount; i++)
		{
			const auto& keyframe = make_shared< ModelKeyframe>();
			keyframe->BoneName = r->String();

			UINT size = r->UInt();
			if (size > 0)
			{
				keyframe->Transforms.assign(size, ModelKeyframeData());

				void* ptr = reinterpret_cast<void *>(&keyframe->Transforms[0]);
				r->Byte(&ptr, sizeof(ModelKeyframeData) * size);
			}

			clip->keyframeMap[keyframe->BoneName] = keyframe;

		}

		r->Close();
		SafeDelete(r);

		clips.emplace_back(clip);
	}


	CreateAnimTransformSRV();
	////////////////////////////////////////////////////////////////////////////////////////
	//const auto& file = L"../_Models/SkeletalMeshes/" + name + L".clip";

	//BinaryReader* r = new BinaryReader();
	//r->Open(file);


	//const auto& clip = make_shared< ModelClip>();

	//clip->name = r->String();
	//clip->duration = r->Float();
	//clip->frameRate = r->Float();
	//clip->frameCount = r->UInt();

	//UINT keyframesCount = r->UInt();
	//for (UINT i = 0; i < keyframesCount; i++)
	//{
	//	const auto& keyframe = make_shared< ModelKeyframe>();
	//	keyframe->BoneName = r->String();

	//	UINT size = r->UInt();
	//	if (size > 0)
	//	{
	//		keyframe->Transforms.assign(size, ModelKeyframeData());

	//		void* ptr = (void *)&keyframe->Transforms[0];
	//		r->Byte(&ptr, sizeof(ModelKeyframeData) * size);
	//	}

	//	clip->keyframeMap[keyframe->BoneName] = keyframe;

	//}

	//r->Close();
	//SafeDelete(r);

	//clips.push_back(clip);

	
}

void PreviewRenderer::ReadAttachMesh(const wstring & path, const ReadMeshType & meshType,const uint& parentBoneIndex)
{
	vector<pair<int, int>> changes;
	
	BinaryReader* r = new BinaryReader();

	bLoaded = false;

		
	r->Open(path);


	uint tempBoneCount = r->UInt();

		
	for (UINT i = 0; i < tempBoneCount; i++)
	{
			auto& bone = make_shared< ModelBone>();
			bone->index = r->Int();
			bone->name = r->String();
			bone->parentIndex = r->Int();
			bone->transform = r->Matrix();
			attachBones.emplace_back(bone);
	}


	uint tempMeshCount = r->UInt();
	uint offset = 0;
	switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{
			offset = staticVertices.size();
		}
		break;
		case ReadMeshType::SkeletalMesh:
		{
			offset = skeletalVertices.size();
			
	    }
		break;
		}
	
		uint indexOffset = indices.size();

		for (UINT i = 0; i < tempMeshCount; i++)
		{
			auto& mesh = make_shared<Mesh>();

			mesh->name = r->String();
			mesh->boneDesc.boneIndex = r->Int();
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
					
					staticVertices.insert(staticVertices.end(), mesh->vertexCount, VertexTextureNormalTangent());
					for (uint c = 0; c < mesh->vertexCount; c++)
					{
						staticVertices[offset + c].Position = vertices[c].Position;
						staticVertices[offset + c].Uv = vertices[c].Uv;
						staticVertices[offset + c].Normal = vertices[c].Normal;
						staticVertices[offset + c].Tangent = vertices[c].Tangent;
					}
				}
				break;
				case ReadMeshType::SkeletalMesh:
				{
				
					skeletalVertices.insert(skeletalVertices.end(), vertices.begin(), vertices.end());

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

				vector< uint>tempIndices;
				tempIndices.assign(mesh->indexCount, uint());
				void* ptr = reinterpret_cast<void *>(tempIndices.data());
				r->Byte(&ptr, sizeof(uint) * mesh->indexCount);

				mesh->startIndex = indexOffset;
				this->indices.insert(this->indices.end(), mesh->indexCount, uint());
				for (uint c = 0; c < mesh->indexCount; c++)
				{
					this->indices[indexOffset + c] = tempIndices[c];
				}
				indexOffset += mesh->indexCount;
				tempIndices.clear();
				tempIndices.shrink_to_fit();

			}
			Vector3 min;
			Vector3 max;
			min.x = r->Float();
			min.y = r->Float();
			min.z = r->Float();
			max.x = r->Float();
			max.y = r->Float();
			max.z = r->Float();

			attachMeshes.emplace_back(mesh);
		}//for(i)


		r->Close();
		SafeDelete(r);
		ProgressReport::Get().SetJobCount(ProgressReport::Model, 10000);
		
		shared_ptr<ModelBone> parentBone = bones[parentBoneIndex];
		
		for (auto& bone : attachBones)
		{
			shared_ptr<ModelBone> newBone = make_shared<ModelBone>();
			newBone->name = bone->name;
			newBone->transform = bone->transform;


			if (bone->parent != NULL)
			{
				int parentIndex = bone->parentIndex;

				for (pair<int, int>& temp : changes)
				{
					if (temp.first == parentIndex)
					{
						newBone->parentIndex = temp.second;
						newBone->parent = bones[newBone->parentIndex];
						newBone->parent->childs.push_back(newBone);

						break;
					}
				}//for(temp)
			}
			else
			{
				newBone->parentIndex = parentBoneIndex;
				newBone->parent = parentBone;
				newBone->parent->childs.push_back(newBone);
			}

			newBone->index = bones.size();
			changes.push_back(pair<int, int>(bone->index, newBone->index));



			bones.emplace_back(newBone);
		}//for(bone)
		for (uint i = 0; i < 2000; i++)
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		boneCount = bones.size();
		uint count = attachMeshes.size();
		for (uint i = 0; i < count; i++)
		{
			 auto mesh = attachMeshes.data();
			 auto& newMesh = make_shared<Mesh>();

			for (pair<int, int>& temp : changes)
			{
				if (temp.first == mesh[i]->boneDesc.boneIndex)
				{
					newMesh->boneDesc.boneIndex = temp.second;

					break;
				}
			}//for(temp)


		//	newMesh->bone = bones[newMesh->boneIndex];
			newMesh->name = mesh[i]->name;
			newMesh->materialName = mesh[i]->materialName;

			newMesh->startVertexIndex = mesh[i]->startVertexIndex;
			newMesh->vertexCount = mesh[i]->vertexCount;
			newMesh->startIndex = mesh[i]->startIndex;
			newMesh->indexCount = mesh[i]->indexCount;

			meshes.emplace_back(newMesh);
		}
		meshCount = meshes.size();
		for (uint i = 0; i < 2000; i++)
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		Thread::Get()->AddTask([&]() 
		{

		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{
			CreateModelTransformSRV();
			for (uint i = 0; i < 2000; i++)
				ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
			BindingStaticMesh();
		}
		break;
		case ReadMeshType::SkeletalMesh:
		{
			CreateAnimTransformSRV();
		
			for (uint i = 0; i < 2000; i++)
				ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
			BindingSkeletalMesh();
		}
		break;
		}
		for (uint i = 0; i < 2000; i++)
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);


		BindingMaterialBone();

	
		attachMeshes.clear();
		attachMeshes.shrink_to_fit();
		attachBones.clear();
		attachBones.shrink_to_fit();
		for (uint i = 0; i < 2000; i++)
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);

		bLoaded = true;

		});

		
}

void PreviewRenderer::DeleteMesh(const uint & index)
{

	int meshIndex=-1;
	for (uint i=0;i<meshes.size();i++)
	{
		if (meshes[i]->boneDesc.boneIndex == index)
		{
			meshIndex = i;
		}
	}
	if (meshIndex < 0)
	{
		auto boneRemove =find_if(bones.begin(), bones.end(), [&index](shared_ptr < ModelBone> bone)
		{
			return bone->index == index;
		});
		if (boneRemove != bones.end())
		{
			auto& bone = *boneRemove;
			uint parentIndex = bone->index;
			auto findParent = find_if(bones.begin(), bones.end(), [&parentIndex](shared_ptr < ModelBone> bone)
			{
				return bone->ParentIndex() == parentIndex;
			});
			if (findParent != bones.end())
			{
				auto& noParent = *findParent;
				noParent->parent = nullptr;
				noParent->parentIndex = -1;
				
			}
			boneCount--;
			bones.erase(boneRemove);
		}
			
		return;
	}

	uint startVertexIndex = meshes[meshIndex]->startVertexIndex;
	uint vertexCount = meshes[meshIndex]->vertexCount;
	uint startIndex = meshes[meshIndex]->startIndex;
	uint indexCount= meshes[meshIndex]->indexCount;

	staticVertices.erase(staticVertices.begin() + startVertexIndex, staticVertices.begin() + startVertexIndex + vertexCount);
	
	indices.erase(indices.begin() + startIndex, indices.begin() + startIndex+ indexCount);




	auto remove= find_if(meshes.begin(), meshes.end(), [&index](shared_ptr<Mesh> mesh)
	{
		return mesh->boneDesc.boneIndex == index;
	});
	if (remove != meshes.end())
	meshes.erase(remove);
	meshCount = meshes.size();
	for (uint i = meshIndex; i < meshCount; i++)
	{
		meshes[i]->startVertexIndex -= vertexCount;
		meshes[i]->startIndex -= indexCount;
		meshes[i]->boneDesc.boneIndex -= 1;
	}

	if (blendMeshIndex > static_cast<uint>(meshIndex))
	{
		blendMeshIndex -= 1;
	}

	auto boneRemove = find_if(bones.begin(), bones.end(), [&index](shared_ptr < ModelBone> bone)
	{
		return bone->index == index;
	});
	if (boneRemove != bones.end())
	bones.erase(boneRemove);

	boneCount = static_cast<uint>(bones.size());
	for (uint i = index; i < boneCount; i++)
	{
		bones[i]->index -= 1;

		//if(index>bones[i]->parentIndex)
		bones[i]->parentIndex -= 1;
	
	}

	switch (meshType)
	{
	
	case ReadMeshType::StaticMesh:
		BindingStaticMesh();
		CreateModelTransformSRV();
		break;
	case ReadMeshType::SkeletalMesh:
		BindingSkeletalMesh();
		CreateAnimTransformSRV();
		break;
	
	}
	

}

void PreviewRenderer::BlendMesh(const uint & index)
{
	int meshIndex = -1;
	for (uint i = 0; i < meshes.size(); i++)
	{
		if (meshes[i]->boneDesc.boneIndex == index)
		{
			meshIndex = i;
		}
	}
	if (meshIndex < 0) return;

	auto blendMeshIter = find_if(meshes.begin(), meshes.end(), [&index](shared_ptr<Mesh> mesh)
	{
		return mesh->boneDesc.boneIndex == index;
	});
	if (blendMeshIter != meshes.end())
	{
		blendMeshes.emplace_back(*blendMeshIter);
		
			
		blendMeshIndex = meshIndex;
		blendCount = blendMeshes.size();
		
	}
		
}

void PreviewRenderer::AnimationUpdate(ID3D11DeviceContext* context)
{
	if (!bLoaded|| clips.empty()) return;
	
	const auto& clip = clips[tweenDesc.Curr.Clip];
	static const float speed = 1.0f;
	//현재 애니메이션
	{
		if(bPause==false)
		tweenDesc.Curr.RunningTime += Time::Delta();

		float time = 1.0f / clip->FrameRate() / speed;
		if (tweenDesc.Curr.RunningTime >= time)
		{
			tweenDesc.Curr.RunningTime = 0.0f;

			tweenDesc.Curr.CurrFrame = (tweenDesc.Curr.CurrFrame + 1) % clip->FrameCount();
			tweenDesc.Curr.NextFrame = (tweenDesc.Curr.CurrFrame + 1) % clip->FrameCount();
		}
		tweenDesc.Curr.Time = tweenDesc.Curr.RunningTime/time;
	}

	//바뀔 애니메이션이 존재함
	if (tweenDesc.Next.Clip > -1)
	{
		const auto& nextClip = clips[tweenDesc.Next.Clip];

		tweenDesc.ChangeTime += Time::Delta();
		tweenDesc.TweenTime = tweenDesc.ChangeTime / tweenDesc.TakeTime;

		if (tweenDesc.TweenTime >= 1.0f)
		{
			tweenDesc.Curr = tweenDesc.Next;

			tweenDesc.Next.Clip = -1;
			tweenDesc.Next.CurrFrame = 0;
			tweenDesc.Next.NextFrame = 0;
			tweenDesc.Next.Time = 0;
			tweenDesc.Next.RunningTime = 0.0f;

			tweenDesc.ChangeTime = 0.0f;
			tweenDesc.TweenTime = 0.0f;
		}
		else
		{
			tweenDesc.Next.RunningTime += Time::Delta();

			float time = 1.0f / nextClip->FrameRate() / speed;
			if (tweenDesc.Next.Time >= 1.0f)
			{
				tweenDesc.Next.RunningTime = 0;

				tweenDesc.Next.CurrFrame = (tweenDesc.Next.CurrFrame + 1) % nextClip->FrameCount();
				tweenDesc.Next.NextFrame = (tweenDesc.Next.CurrFrame + 1) % nextClip->FrameCount();
			}
			tweenDesc.Next.Time = tweenDesc.Next.RunningTime / time;
		}
	}

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(tweenBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &tweenDesc, sizeof(tweenDesc));
	context->Unmap(tweenBuffer, 0);

	context->VSSetConstantBuffers(3, 1, &tweenBuffer);
}

void PreviewRenderer::Update(const Vector2& size, const float& camSpeed)
{
	Vector2 val = Vector2(ImGui::GetMouseDragDelta(1).x, ImGui::GetMouseDragDelta(1).y);

	

	if (ImGui::IsAnyWindowHovered())
	{
		if (ImGui::IsMouseDown(1))
		{

			rotation.y += (val.y / 20)*0.008f;
			rotation.x += (val.x / 20)*0.008f;
			
		}

		if (ImGui::IsKeyPressed('E') )
		{
			targetPosition += viewUp * camSpeed;
		
		}
		else if (ImGui::IsKeyPressed('Q'))
		{
			targetPosition -= viewUp * camSpeed;
		}

		if (ImGui::IsKeyPressed('D') )
		{
			targetPosition += viewRight * camSpeed;
		}
		else if (ImGui::IsKeyPressed('A') )
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

	

	eyeDesc.EyePosition.x = targetPosition.x + distance * sinf(rotation.y)*cosf(rotation.x);
	eyeDesc.EyePosition.y = targetPosition.y + distance * cosf(rotation.y);
	eyeDesc.EyePosition.z = targetPosition.z + distance * sinf(rotation.y)*sinf(rotation.x);

	//ImGui::Begin("Debug");
	//{
	//	const string& str1 = string("Dist: ") + to_string(distance) + "/" + string("Y: ") + to_string(R.y) + "/" + string("X: ") + to_string(R.x);
	//	ImGui::Text(str1.c_str());
	//}
	//ImGui::End();
	D3DXMatrixLookAtLH(&view, &eyeDesc.EyePosition, &targetPosition, &Vector3(0, 1, 0));

	float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);
	D3DXMatrixPerspectiveFovLH(&proj, static_cast<float>(D3DX_PI)* 0.25f, aspect, 0.1f, 1000.0f);

	
	
	previewDesc.VP = view * proj;
	Matrix viewInv;
	D3DXMatrixInverse(&viewInv, nullptr, &view);
	viewRight = Vector3(viewInv._11, viewInv._12, viewInv._13);
	viewUp = Vector3(viewInv._21, viewInv._22, viewInv._23);

	
	D3DXMatrixTranspose(&previewDesc.VP, &previewDesc.VP);
}

void PreviewRenderer::Render(ID3D11DeviceContext * context)
{
	progress->Begin();
	progress->Render();

	if (!bLoaded) return;
	
	DebugRender(context);



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
		if (blendCount > 0 && i == blendMeshIndex)continue;
		auto& m = meshes[i];
		m->ApplyPipeline(context);
		context->DrawIndexed(m->indexCount, m->startIndex, m->startVertexIndex);
		m->ClearPipelineMaterial(context);

	}
	ID3D11BlendState* pPrevBlendState;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);

	context->OMSetBlendState(AdditiveBlendState, prevBlendFactor, prevSampleMask);
	for (auto& blendMesh : blendMeshes)
	{
		blendMesh->ApplyPipeline(context);
		context->DrawIndexed(blendMesh->indexCount, blendMesh->startIndex, blendMesh->startVertexIndex);
		blendMesh->ClearPipelineMaterial(context);
	}
	context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
	context->VSSetShaderResources(0, 1, &nullSRV);
	context->VSSetConstantBuffers(0, 1, &nullBuffer);
	context->PSSetConstantBuffers(1, 1, &nullBuffer);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);

	
}

void PreviewRenderer::BoxRender(ID3D11DeviceContext * context,const Matrix & matrix, const Vector3 & min, const Vector3 & max, const Color & color)
{
	
	    dest[0] = Vector3(min.x, min.y, max.z);
	    dest[1] = Vector3(max.x, min.y, max.z);
	    dest[2] = Vector3(min.x, max.y, max.z);
	    dest[3] = Vector3(max);
	    dest[4] = Vector3(min);
	    dest[5] = Vector3(max.x, min.y, min.z);
	    dest[6] = Vector3(min.x, max.y, min.z);
	    dest[7] = Vector3(max.x, max.y, min.z);

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


		Matrix VP;
		D3DXMatrixTranspose(&VP, &previewDesc.VP);
		DebugLine::Get()->PreviewRender(context, matrix*VP);
}

void PreviewRenderer::CapsuleRender(ID3D11DeviceContext * context, const Matrix & matrix, const Color & color)
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
	for (UINT i = 0; i < stackCount; i++)
	{
		if (i <= stackCount / 2)
		{
			DebugLine::Get()->RenderLine(v[i], v[i + 1]);
			DebugLine::Get()->RenderLine(v2[i], v2[i + 1]);
		}
		DebugLine::Get()->RenderLine(v3[i], v3[i + 1]);
	}
	DebugLine::Get()->RenderLine(v3[stackCount / 4], e3[stackCount / 4]);
	DebugLine::Get()->RenderLine(v3[stackCount / 2], e3[stackCount / 2]);
	DebugLine::Get()->RenderLine(v3[stackCount * 3 / 4], e3[stackCount * 3 / 4]);
	DebugLine::Get()->RenderLine(v3[stackCount], e3[stackCount]);

	for (UINT i = 0; i < stackCount; i++)
	{
		if (i >= stackCount / 2)
		{
			DebugLine::Get()->RenderLine(e[i], e[i + 1]);
			DebugLine::Get()->RenderLine(e2[i], e2[i + 1]);
		}
		DebugLine::Get()->RenderLine(e3[i], e3[i + 1]);
	}

	Matrix VP;
	D3DXMatrixTranspose(&VP, &previewDesc.VP);
	DebugLine::Get()->PreviewRender(context,matrix*VP);
}

void PreviewRenderer::DebugRender(ID3D11DeviceContext * context)
{
	BoxRender(context, boxWorld, boxMin, boxMax, Color(0, 1, 0, 1));


	Matrix W;
	D3DXMatrixTranspose(&W, &previewDesc.W);
	if (boxCount > 0)
		for (uint i = 0; i < boxCount; i++)
		{
			
			const Matrix& currFarameMatrix = skinTransforms[tweenDesc.Curr.Clip].Transform[tweenDesc.Curr.CurrFrame][colliderBoxData[i].Index];
			if (unArmedBoneCount > static_cast<uint>(colliderBoxData[i].Index))
			{
				colliderBoxData[i].matrix = bones[colliderBoxData[i].Index]->Transform()*currFarameMatrix* W;
			}
			else
			{
				colliderBoxData[i].matrix = currFarameMatrix * W;
			}
		
			D3DXMatrixDecompose(&colliderBoxData[i].scale, &colliderBoxData[i].q,
				&colliderBoxData[i].position, &colliderBoxData[i].matrix);
			D3DXMatrixRotationQuaternion(&colliderBoxData[i].R, &colliderBoxData[i].q);
			D3DXMatrixTranslation(&colliderBoxData[i].T, colliderBoxData[i].position.x,
				colliderBoxData[i].position.y, colliderBoxData[i].position.z);

			//const Vector3& boneBoxMin = Vector3(-0.5f, -0.5f, -0.5f);
			//const Vector3& boneBoxMax = Vector3(0.5f, 0.5f, 0.5f);

			colliderBoxData[i].result = colliderBoxData[i].ColliderBoxWorld*colliderBoxData[i].R * colliderBoxData[i].T;
			//BoxRender(context,colliderBoxData[i].result, boneBoxMin, boneBoxMax, Color(0, 1, 0, 1));
			CapsuleRender(context, colliderBoxData[i].result,  Color(0, 1, 0, 1));
		}

}

void PreviewRenderer::CreateSahders(const string& file)
{
	SafeRelease(VS);
	SafeRelease(PS);

	ID3DBlob* ShaderBlob = NULL;
	auto path = file;
	auto entryPoint = "VS";
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


	entryPoint = "PS";
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

void PreviewRenderer::GetBoxSize(const Vector3 & min, const Vector3 & max)
{
	Vector3 tempMin, tempMax;

	Matrix w;
	D3DXMatrixTranspose(&w, &previewDesc.W);
	D3DXVec3TransformCoord(&tempMin, &min, &w);
	D3DXVec3TransformCoord(&tempMax, &max, &w);
	boxMin = Math::Min(boxMin, tempMin);
	boxMax = Math::Min(boxMax, tempMax);

	//Matrix S,T;
	//D3DXMatrixScaling(&S, (boxMax.x - boxMin.x), (boxMax.y - boxMin.y), (boxMax.z - boxMin.z));
	//Vector3 center = (boxMax + boxMin)*0.5f;
	//
	//D3DXMatrixTranslation(&T, center.x, center.y, center.z);


	//debugBoxes[0]
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

void PreviewRenderer::BindingStaticMesh()
{
	
	stride = sizeof(VertexTextureNormalTangent);

	
	

	SafeRelease(vertexBuffer);
	SafeRelease(indexBuffer);

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(VertexTextureNormalTangent) * staticVertices.size();
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = staticVertices.data();
	Check(device->CreateBuffer(&desc, &subResource, &vertexBuffer));






	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(uint) * indices.size();
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = indices.data();
	Check(device->CreateBuffer(&desc, &subResource, &indexBuffer));

	
	

}

void PreviewRenderer::BindingSkeletalMesh()
{
	stride = sizeof(VertexTextureNormalTangentBlend);



	SafeRelease(vertexBuffer);
	SafeRelease(indexBuffer);

	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(VertexTextureNormalTangentBlend) * skeletalVertices.size();
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = skeletalVertices.data();
	Check(device->CreateBuffer(&desc, &subResource, &vertexBuffer));




	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(uint) * indices.size();
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = indices.data();
	Check(device->CreateBuffer(&desc, &subResource, &indexBuffer));
	
}

void PreviewRenderer::BindingMaterialBone()
{
	for (auto& bone : bones)
	{
		if (bone->parentIndex > -1)
		{
			if (bone->parent == nullptr)
			{
				bone->parent = bones[bone->parentIndex];
				bone->parent->childs.push_back(bone);
			}
			
		}
		else
			bone->parent = NULL;
	}
	for (uint i = 0; i < meshCount; i++)
	{
		

		meshes[i]->CreateBuffer(device);

		string name = meshes[i]->materialName;
		auto find = find_if(materials.begin(), materials.end(), [&name](shared_ptr<class Material> material) 
		{
			return name == String::ToString(material->Name());
		});
		if (find != materials.end()&& meshes[i]->material == nullptr)
		{
			meshes[i]->material = *find;
			meshes[i]->bHasMaterial = true;
		}
		if (blendCount > 0)
		{
			for (uint i = 0; i < blendCount; i++)
			{

				blendMeshes[i]->CreateBuffer(device);
				string name = blendMeshes[i]->materialName;
				auto find = find_if(materials.begin(), materials.end(), [&name](shared_ptr<class Material> material)
				{
					return name == String::ToString(material->Name());
				});
				if (find != materials.end() && blendMeshes[i]->material == nullptr)
				{
					blendMeshes[i]->material = *find;
					blendMeshes[i]->bHasMaterial = true;
				}
			}
		}

	}

	materials.clear();
	materials.shrink_to_fit();
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

void PreviewRenderer::CreateAnimConstantBuffers()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(TweenDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &tweenBuffer));
}

void PreviewRenderer::UpdateCurrFameAnimTransformSRV()
{
	if (clips.empty())
	{
		CreateModelTransformSRV();
		return;
	}
	for (UINT b = 0; b < boneCount; b++)
	{

		D3DXMatrixInverse(&invGlobal, NULL, &bones[b]->Transform());

		int parentIndex = bones[b]->ParentIndex();

		if (parentIndex < 0)
			D3DXMatrixIdentity(&parent);
		else
			parent = saveParentMatrix[parentIndex];



		const auto& frame = clips[tweenDesc.Curr.Clip]->Keyframe(bones[b]->Name());

		if (frame != NULL)
		{
			const ModelKeyframeData& data = frame->Transforms[tweenDesc.Curr.CurrFrame];

			D3DXMatrixScaling(&S, data.Scale.x, data.Scale.y, data.Scale.z);
			D3DXMatrixRotationQuaternion(&R, &data.Rotation);
			D3DXMatrixTranslation(&T, data.Translation.x, data.Translation.y, data.Translation.z);


			animation = S * R * T;

			saveParentMatrix[b] = animation * parent;

			skinTransforms[tweenDesc.Curr.Clip].Transform[tweenDesc.Curr.CurrFrame][b] = invGlobal * saveParentMatrix[b];
		}
		else
		{
			saveParentMatrix[b] = parent;

			skinTransforms[tweenDesc.Curr.Clip].Transform[tweenDesc.Curr.CurrFrame][b] = bones[b]->Transform()* saveParentMatrix[b];
		}



	}
	INT64 currentTime;
	QueryPerformanceCounter((LARGE_INTEGER *)&currentTime);
	if (chrono::_Is_even<INT64>(currentTime))
	{
		SafeRelease(texture);
		SafeRelease(srv);
		const uint& clipSize = clips.size();
		//Create Texture
		{
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
			desc.Width = boneCount * 4;
			desc.Height = maxkeyframe;
			desc.MipLevels = 1;
			desc.ArraySize = clipSize;
			desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			desc.SampleDesc.Count = 1;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			//	for (UINT c = 0; c < clipSize; c++)
			{
				//for (UINT y = 0; y < clips[c]->FrameCount(); y++)
				{
					UINT start = tweenDesc.Curr.Clip * pageSize;
					void* temp = reinterpret_cast<BYTE *>(p) + boneCount * tweenDesc.Curr.CurrFrame * sizeof(Matrix) + start;

					memcpy(temp, skinTransforms[tweenDesc.Curr.Clip].Transform[tweenDesc.Curr.CurrFrame], sizeof(Matrix) * boneCount);
				}
			}

			D3D11_SUBRESOURCE_DATA* subResource = new D3D11_SUBRESOURCE_DATA[clipSize];
			for (UINT c = 0; c < clipSize; c++)
			{
				void* temp = reinterpret_cast<BYTE *>(p) + c * pageSize;

				subResource[c].pSysMem = temp;
				subResource[c].SysMemPitch = boneCount * sizeof(Matrix);
				subResource[c].SysMemSlicePitch = pageSize;
			}

			Check(device->CreateTexture2D(&desc, subResource, &texture));

			SafeDeleteArray(subResource);



			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Format = desc.Format;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.ArraySize = clipSize;

			HRESULT hr = device->CreateShaderResourceView(texture, &srvDesc, &srv);
			Check(hr);
		}
	}

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
			uint index =  b;
			boneTransforms[index] = matrix * temp[b];

			
			
		}
		

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
	ProgressReport::Get().SetJobCount(ProgressReport::Model, (clips.size()*1000)+1000);
	

	free(p);
	SafeDeleteArray(saveParentMatrix);
	SafeDeleteArray(skinTransforms);
	maxkeyframe = 0;
	const uint& clipSize = clips.size();

	if (!clips.empty())
	{	
		CreateAnimConstantBuffers();

		
	//	if (skinTransforms == nullptr)
		{
			skinTransforms = new BoneTransform[clipSize]();
			
			for (uint i = 0; i < clipSize; i++)
			{
				skinTransforms[i].CreateTransforms(clips[i]->FrameCount(), boneCount);
				maxkeyframe = max(maxkeyframe, clips[i]->FrameCount());
			}
			saveParentMatrix = new Matrix[boneCount];
		}
		
		for (uint i = 0; i < 200; i++)
			ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
	
		for (UINT i = 0; i < clipSize; i++)
		{

			for (UINT f = 0; f < clips[i]->FrameCount(); f++)
			{
				for (UINT b = 0; b < boneCount; b++)
				{
					Matrix invGlobal;
					D3DXMatrixInverse(&invGlobal, NULL, &bones[b]->Transform());

					int parentIndex = bones[b]->ParentIndex();
					Matrix parent;
					if (parentIndex < 0)
						D3DXMatrixIdentity(&parent);
					else
						parent = saveParentMatrix[parentIndex];


				
					const auto& frame = clips[i]->Keyframe(bones[b]->Name());

					if (frame != NULL)
					{
						ModelKeyframeData data = frame->Transforms[f];
						Matrix S, R, T;
						D3DXMatrixScaling(&S, data.Scale.x, data.Scale.y, data.Scale.z);
						D3DXMatrixRotationQuaternion(&R, &data.Rotation);
						D3DXMatrixTranslation(&T, data.Translation.x, data.Translation.y, data.Translation.z);
						
						const Matrix& animation = S * R * T;

						saveParentMatrix[b] = animation * parent;

						skinTransforms[i].Transform[f][b] = invGlobal*saveParentMatrix[b];
					}
					else
					{
						saveParentMatrix[b] = parent;

						skinTransforms[i].Transform[f][b] = bones[b]->Transform()* saveParentMatrix[b];
					}
					
					

				}
			}
			for (uint i = 0; i < 1000; i++)
				ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
		}
	

		
		//Create Texture
		{
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
			desc.Width = boneCount * 4;
			desc.Height = maxkeyframe;
			desc.MipLevels = 1;
			desc.ArraySize = clipSize;
			desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			desc.SampleDesc.Count = 1;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;


			
			pageSize = boneCount * 4 * 16 * maxkeyframe;
			p = malloc(pageSize * clipSize);
			
		

			for (UINT c = 0; c < clipSize; c++)
			{
				for (UINT y = 0; y < clips[c]->FrameCount(); y++)
				{
					UINT start = c * pageSize;
					void* temp = (BYTE *)p + boneCount * y * sizeof(Matrix) + start;

					memcpy(temp, skinTransforms[c].Transform[y], sizeof(Matrix) * boneCount);
				}
			}

			D3D11_SUBRESOURCE_DATA* subResource = new D3D11_SUBRESOURCE_DATA[clipSize];
			for (UINT c = 0; c < clipSize; c++)
			{
				void* temp = (BYTE *)p + c * pageSize;

				subResource[c].pSysMem = temp;
				subResource[c].SysMemPitch = boneCount * sizeof(Matrix);
				subResource[c].SysMemSlicePitch = pageSize;
			}
			for (uint i = 0; i < 400; i++)
				ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
			SafeRelease(texture);
			SafeRelease(srv);
			Check(device->CreateTexture2D(&desc, subResource, &texture));

			SafeDeleteArray(subResource);
			//free(p);
			
		}

		//Create SRV
		{
			D3D11_TEXTURE2D_DESC desc;
			texture->GetDesc(&desc);

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Format = desc.Format;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.ArraySize = clipSize;

			HRESULT hr = device->CreateShaderResourceView(texture, &srvDesc, &srv);
			Check(hr);

			for (uint i = 0; i < 400; i++)
				ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
			//// Create the UAVs
			//D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
			//ZeroMemory(&DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
			//DescUAV.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			//DescUAV.Format = desc.Format;
			//
			//DescUAV.Texture2DArray.ArraySize = model->ClipCount() + 1;
			//
			//
			//Check(D3D::GetDevice()->CreateUnorderedAccessView(texture, &DescUAV, &uav));
		}
	}
	else
	{
	     CreateModelTransformSRV();
		 for (uint i = 0; i < 1000; i++)
			 ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
    }
	//sTransformSRV->SetResource(srv);
	
}

const Matrix & PreviewRenderer::GetSkinnedMatrix(const uint & index)
{
	
	if (nuArmedBoneCount > index)
	{
		skinnedTransform= bones[index]->transform*skinTransforms[tweenDesc.Curr.Clip].Transform[tweenDesc.Curr.CurrFrame][index];
		return skinnedTransform;
	}

	else if (clips.empty())
	{
		return bones[index]->transform;
    }
	else
	{
		return skinTransforms[tweenDesc.Curr.Clip].Transform[tweenDesc.Curr.CurrFrame][index];
	}
}


