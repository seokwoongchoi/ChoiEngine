#include "Framework.h"
#include "Renderer.h"
#include "Resources/Mesh.h"
#include "Core/D3D11/D3D11_Helper.h"
#include "Utility/Xml.h"

void Renderer::Initiallize(ID3D11Device* device,const uint & ID)
{
	this->device = device;
		
	this->ID = ID;

	offset = 0;
	slot = 0;
	

	
}

Mesh * Renderer::GetMesh()
{
	return  &mesh[0];
}



void Renderer::Reflection_PreRender(ID3D11DeviceContext * context,const uint&drawCount, const uint& prevDrawCount)
{
	context->VSSetShader(reflectionVS, nullptr, 0);
	context->PSSetShader(reflectionPS, nullptr, 0);


	context->PSSetSamplers(0, 1, &sampLinear);
	context->RSSetState(nullptr);
	

	
	context->IASetVertexBuffers(slot, 1, &vertexBuffer, &stride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	for (uint i = 0; i < meshCount; i++)
	{
		auto& m = mesh[i];
		m.ApplyPipelineReflectionMaterial(context, prevDrawCount, ID);
		context->DrawIndexedInstanced(m.indexCount, drawCount, m.startIndex, m.startVertexIndex, 0);
		m.ClearPipelineReflection(context);

	}
	
	
	
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

void Renderer::Render(ID3D11DeviceContext* context, const uint& drawCount, const uint& prevDrawCount)
{
	

	context->VSSetShader(vs, nullptr, 0);
	context->PSSetShader(ps, nullptr, 0);
	
	context->PSSetSamplers(0, 1, &sampLinear);
	context->RSSetState(nullptr);
	
	
	
	context->IASetVertexBuffers(slot, 1, &vertexBuffer, &stride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	

	if (bAlphaToCoverage)
	{
		ID3D11BlendState* pPrevBlendState;
		FLOAT prevBlendFactor[4];
		UINT prevSampleMask;

		for (uint i = 0; i < meshCount; i++)
		{
			auto& m = mesh[i];

			if (i == 0)
			{

				context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);
				context->OMSetBlendState(AlphaToCoverageEnable, prevBlendFactor, prevSampleMask);
			}

			m.ApplyPipeline(context, prevDrawCount, ID);
			context->DrawIndexedInstanced(m.indexCount, drawCount, m.startIndex, m.startVertexIndex, 0);
			m.ClearPipelineMaterial(context);

			if (i == 0)
			{
				context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
			}
		}
	}
	else
	{
		

		for (uint i = 0; i < meshCount; i++)
		{
			auto& m = mesh[i];
	

			m.ApplyPipeline(context, prevDrawCount, ID);
			context->DrawIndexedInstanced(m.indexCount, drawCount, m.startIndex, m.startVertexIndex, 0);
			m.ClearPipelineMaterial(context);
		
		}
	}
	
	


	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
	
}

void Renderer::Forward_Render(ID3D11DeviceContext * context, const uint& drawCount, const uint& prevDrawCount)
{
	if (blendCount < 1) return;
	context->VSSetShader(vs, nullptr, 0);
	context->PSSetShader(forwardPS, nullptr, 0);

	context->PSSetSamplers(0, 1, &sampLinear);
	context->RSSetState(nullptr);


	context->IASetVertexBuffers(slot, 1, &vertexBuffer, &stride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11BlendState* pPrevBlendState;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);

	context->OMSetBlendState(AdditiveBlendState, prevBlendFactor, prevSampleMask);

	for (uint i = 0; i < blendCount; i++)
	{
		blendMesh[i].ApplyPipeline(context, prevDrawCount, ID);
		context->DrawIndexedInstanced(blendMesh[i].indexCount, drawCount, blendMesh[i].startIndex, blendMesh[i].startVertexIndex, 0);

		blendMesh[i].ClearPipelineMaterial(context);
	}
	
	
	context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
	
	
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

void Renderer::CreateStates()
{
	SafeRelease(sampLinear);
	SafeRelease(rsState);
	SafeRelease(AdditiveBlendState);
	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(samDesc));
	samDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &sampLinear));

	/////////////////////////////////////////////////////////////////////////
	//Create Rasterizer
	D3D11_RASTERIZER_DESC reasterizerDesc;
	ZeroMemory(&reasterizerDesc,  sizeof(reasterizerDesc));
	reasterizerDesc.CullMode = D3D11_CULL_BACK;
	reasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	reasterizerDesc.FrontCounterClockwise = false;
	Check(device->CreateRasterizerState(&reasterizerDesc, &rsState));
	//////////////////////////////////////////////////////////////////////////


	if (blendCount > 0)
	{
		// Create the additive blend state
		D3D11_BLEND_DESC descBlend;
		ZeroMemory(&descBlend, sizeof(descBlend));
		descBlend.AlphaToCoverageEnable = false;
		descBlend.IndependentBlendEnable = false;
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
	

	if (bAlphaToCoverage)
	{
		D3D11_BLEND_DESC descBlend;
		ZeroMemory(&descBlend, sizeof(descBlend));
		descBlend.AlphaToCoverageEnable = true;
		descBlend.IndependentBlendEnable = false;
		const D3D11_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			false,
			D3D11_BLEND_DEST_COLOR, D3D11_BLEND_SRC_COLOR, D3D11_BLEND_OP_ADD, //srcBlend,descBlend,BlendOp
			D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,//srcBlendAlpha,destBlendAlpha,BlendOpAlpha
			D3D11_COLOR_WRITE_ENABLE_ALL,//rendertargetWriteMask
		};


		for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			descBlend.RenderTarget[i] = defaultRenderTargetBlendDesc;
		Check(device->CreateBlendState(&descBlend, &AlphaToCoverageEnable));
	}
	
}



void Renderer::ReadMaterial(const wstring& name)
{

	materials.clear();
	materials.shrink_to_fit();

	auto path = L"../_Textures/"+ name+L"/" + name + L".material";

	shared_ptr<Xml::XMLDocument> document = make_shared< Xml::XMLDocument>();
	Xml::XMLError error = document->LoadFile(String::ToString(path).c_str());
	assert(error == Xml::XML_SUCCESS);

	Xml::XMLElement* root = document->FirstChildElement();
	Xml::XMLElement* materialNode = root->FirstChildElement();

	do
	{
		auto material = make_shared<Material>(device);


		Xml::XMLElement* node = NULL;

		node = materialNode->FirstChildElement();
		material->Name(String::ToWString(node->GetText()));


		wstring directory = Path::GetDirectoryName(path);
		String::Replace(&directory, L"../_Textures", L"");

		wstring texture = L"";
		
		node = node->NextSiblingElement();
		texture = String::ToWString(node->GetText());
		if (texture.length() > 0)
			material->DiffuseMap(directory + texture,true);
			
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

void Renderer::BindMaterial()
{
	for (uint i = 0; i < meshCount; i++)
	{

		mesh[i].CreateBuffer(device);
		string name = mesh[i].materialName;
		auto find = find_if(materials.begin(), materials.end(), [&name](shared_ptr<class Material> material)
		{
			return name == String::ToString(material->Name());
		});
		if (find != materials.end() && mesh[i].material == nullptr)
		{
			mesh[i].material = *find;
			mesh[i].bHasMaterial = true;
		}
	}
	if (blendCount > 0)
	{
		for (uint i = 0; i < blendCount; i++)
		{

			blendMesh[i].CreateBuffer(device);
			string name = blendMesh[i].materialName;
			auto find = find_if(materials.begin(), materials.end(), [&name](shared_ptr<class Material> material)
			{
				return name == String::ToString(material->Name());
			});
			if (find != materials.end() && blendMesh[i].material == nullptr)
			{
				blendMesh[i].material = *find;
				blendMesh[i].bHasMaterial = true;
			}
		}
	}
	materials.clear();
	materials.shrink_to_fit();

}

void Renderer::CreateShader(const string& file,  bool IsTree)
{
	SafeRelease(vs);
	
	SafeRelease(ps);
	SafeRelease(reflectionPS);
	SafeRelease(reflectionVS);

	ID3DBlob* ShaderBlob = nullptr;
	auto path = file ;
	auto entryPoint = "VS";
	auto shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &vs));

	inputLayout = make_shared<InputLayout>();
	inputLayout->Create(device, ShaderBlob);
	SafeRelease(ShaderBlob);


	entryPoint = "ReflectionVS";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &reflectionVS));
	SafeRelease(ShaderBlob);


	if (IsTree == true)
	{
		bAlphaToCoverage = true;
		entryPoint = "TreePS";
		shaderModel = "ps_5_0";
		Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
		Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
			ShaderBlob->GetBufferSize(), nullptr, &ps));
		SafeRelease(ShaderBlob);
	}
	else
	{
		entryPoint = "PS";
		shaderModel = "ps_5_0";
		Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
		Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
			ShaderBlob->GetBufferSize(), nullptr, &ps));
		SafeRelease(ShaderBlob);
	}
	
	


	
	entryPoint = "ReflectionPS";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), nullptr, &reflectionPS));
	SafeRelease(ShaderBlob);


	if (blendCount > 0)
	{
		SafeRelease(forwardPS);

		path = "../_Shaders/StaticMeshCSO/ForwardPS.cso";
		ShaderBlob = D3D11_Helper::LoadBinary(String::ToWString(path));


		Check(device->CreatePixelShader
		(
			ShaderBlob->GetBufferPointer(),
			ShaderBlob->GetBufferSize(),
			nullptr,
			&forwardPS
		));

		SafeRelease(ShaderBlob);
	}


	CreateStates();

	
	//ID3DBlob* ShaderBlob = nullptr;
	//auto path = file+"/VS.cso";
	//
	//ShaderBlob = D3D11_Helper::LoadBinary(String::ToWString(path));
	//Check( device->CreateVertexShader
	//(
	//	ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(),
	//	nullptr,
	//	&vs
	//));
	//
	//inputLayout =make_shared<InputLayout>();
	//inputLayout->Create(device, ShaderBlob);
	//SafeRelease(ShaderBlob);

	//path = file + "/ReflectionVS.cso";
	//ShaderBlob = D3D11_Helper::LoadBinary(String::ToWString(path));
	//
	//Check(device->CreateVertexShader
	//(
	//	ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(),
	//	nullptr,
	//	&reflectionVS
	//));

	//SafeRelease(ShaderBlob);

	//
	//path = file + "/PS.cso";
	//ShaderBlob = D3D11_Helper::LoadBinary(String::ToWString(path));

	//Check(device->CreatePixelShader
	//(
	//	ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(),
	//	nullptr,
	//	&ps
	//));


	///*path = "../_Shaders/SkeletalMesh.hlsl";
	//auto entryPoint = "PS";
	//auto shaderModel = "ps_5_0";
	//Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	//Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(), nullptr, &ps));
	//SafeRelease(ShaderBlob);*/

	//SafeRelease(ShaderBlob);

	//path = file + "/ReflectionPS.cso";
	//ShaderBlob = D3D11_Helper::LoadBinary(String::ToWString(path));
	//

	//Check(device->CreatePixelShader
	//(
	//	ShaderBlob->GetBufferPointer(),
	//	ShaderBlob->GetBufferSize(),
	//	nullptr,
	//	&reflectionPS
	//));
	//
	//SafeRelease(ShaderBlob);
	//

	//if (blendCount > 0)
	//{
	//	SafeRelease(forwardPS);

	//	path = file + "/ForwardPS.cso";
	//	ShaderBlob = D3D11_Helper::LoadBinary(String::ToWString(path));


	//	Check(device->CreatePixelShader
	//	(
	//		ShaderBlob->GetBufferPointer(),
	//		ShaderBlob->GetBufferSize(),
	//		nullptr,
	//		&forwardPS
	//	));

	//	SafeRelease(ShaderBlob);
	//}
	//


	//
}

bool Renderer::ReadMesh(BinaryReader* r,const ReadMeshType& meshType)
{
	

	

	meshCount = r->UInt();
	if (meshCount > 1)
	{
		SafeDeleteArray(mesh);
	}
	else
	{
		SafeDelete(mesh);
	}

	mesh = new Mesh[meshCount];
	blendCount = r->UInt();
	if (blendCount > 0)
	{
		blendMeshIndex = r->UInt();
		blendMesh = new Mesh[blendCount];
	}


	uint blendOffset = 0;
	for (UINT i = 0; i < meshCount + blendCount; i++)
	{

		if (blendCount > 0 && i == blendMeshIndex)
		{
			for (uint b = 0; b < blendCount; b++)
			{
				blendMesh[b].name = r->String();
				blendMesh[b].boneDesc.boneIndex = r->Int();
				blendMesh[b].materialName = r->String();
				blendMesh[b].vertexCount = r->UInt();
				blendMesh[b].startVertexIndex = r->UInt();
				blendMesh[b].indexCount = r->UInt();
				blendMesh[b].startIndex = r->UInt();

				
				blendOffset++;
			}

			continue;
		}


		mesh[i - blendOffset].name = r->String();
		mesh[i - blendOffset].boneDesc.boneIndex = r->Int();
		mesh[i - blendOffset].materialName = r->String();
		mesh[i - blendOffset].vertexCount = r->UInt();
		mesh[i - blendOffset].startVertexIndex = r->UInt();
		mesh[i - blendOffset].indexCount = r->UInt();
		mesh[i - blendOffset].startIndex = r->UInt();

		
	}

	switch (meshType)
	{

	case ReadMeshType::StaticMesh:
		BindingStaticMesh(r);
		break;
	case ReadMeshType::SkeletalMesh:
		BindingSkeletalMesh(r);
		break;
	
	}

	{

		boxMin.x = r->Float();
		boxMin.y = r->Float();
		boxMin.z = r->Float();

		boxMax.x = r->Float();
		boxMax.y = r->Float();
		boxMax.z = r->Float();

		
	}



	BindMaterial();


	

	// });

	return true;
}

void Renderer::BindingStaticMesh(BinaryReader * r)
{
	vector< VertexTextureNormalTangent> vertices;
	stride = sizeof(VertexTextureNormalTangent);
	vector< uint> indices;
	uint totalCount = r->UInt();
	uint totalIndexCount = r->UInt();
	vertices.assign(totalCount, VertexTextureNormalTangent());
	indices.assign(totalIndexCount, uint());


	{
		void* ptr = reinterpret_cast<void*>(vertices.data());
		r->Byte(&ptr, sizeof(VertexTextureNormalTangent) * totalCount);

	}

	{
		void* ptr = reinterpret_cast<void*>(indices.data());
		r->Byte(&ptr, sizeof(uint) *totalIndexCount);

	}


	SafeRelease(vertexBuffer);
	SafeRelease(shadowVertexBuffer);
	SafeRelease(indexBuffer);


	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(VertexTextureNormalTangent) * totalCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = vertices.data();
	Check(device->CreateBuffer(&desc, &subResource, &vertexBuffer));



	////Thread::Get()->AddTask([&]()
	{

		
		vector< Vector3> shadowVertices;
		shadowVertices.assign(totalCount, Vector3());

		for (uint i = 0; i < totalCount; i++)
		{
			
			memcpy(shadowVertices[i], vertices[i].Position, sizeof(Vector3));
		}
		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
		desc.ByteWidth = sizeof(Vector3) * totalCount;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.Usage = D3D11_USAGE_DEFAULT;


		D3D11_SUBRESOURCE_DATA subResource;
		ZeroMemory(&subResource, sizeof(subResource));
		subResource.pSysMem = shadowVertices.data();
	



		Check(device->CreateBuffer(&desc, &subResource, &shadowVertexBuffer));
		shadowVertices.clear();
		shadowVertices.shrink_to_fit();

	}
	////);

	

	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(uint) * totalIndexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = indices.data();
	Check(device->CreateBuffer(&desc, &subResource, &indexBuffer));




	vertices.clear();
	vertices.shrink_to_fit();

	indices.clear();
	indices.shrink_to_fit();
}

void Renderer::BindingSkeletalMesh(BinaryReader * r)
{
	vector< VertexTextureNormalTangentBlend> vertices;
	stride = sizeof(VertexTextureNormalTangentBlend);
	vector< uint> indices;
	uint totalCount = r->UInt();
	uint totalIndexCount = r->UInt();
	vertices.assign(totalCount, VertexTextureNormalTangentBlend());
	indices.assign(totalIndexCount, uint());


	{
		void* ptr = reinterpret_cast<void*>(vertices.data());
		r->Byte(&ptr, sizeof(VertexTextureNormalTangentBlend) * totalCount);

	}

	{
		void* ptr = reinterpret_cast<void*>(indices.data());
		r->Byte(&ptr, sizeof(uint) *totalIndexCount);

	}


	SafeRelease(vertexBuffer);
	SafeRelease(shadowVertexBuffer);
	SafeRelease(indexBuffer);


	D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(VertexTextureNormalTangentBlend) * totalCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = vertices.data();
	Check(device->CreateBuffer(&desc, &subResource, &vertexBuffer));



	//Thread::Get()->AddTask([&]()
	{

		
		vector< shadowDesc> shadowVertices;
		shadowVertices.assign(totalCount, shadowDesc());

		for (uint i = 0; i < totalCount; i++)
		{
			memcpy(shadowVertices[i].Position, vertices[i].Position, sizeof(Vector3));
			memcpy(shadowVertices[i].Indices, vertices[i].Indices, sizeof(Vector4));
			memcpy(shadowVertices[i].Weights, vertices[i].Weights, sizeof(Vector4));
		}
		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
		desc.ByteWidth = sizeof(shadowDesc) * totalCount;
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.Usage = D3D11_USAGE_DEFAULT;


		D3D11_SUBRESOURCE_DATA subResource;
		ZeroMemory(&subResource, sizeof(subResource));
		subResource.pSysMem = shadowVertices.data();
	
		Check(device->CreateBuffer(&desc, &subResource, &shadowVertexBuffer));
		shadowVertices.clear();
		shadowVertices.shrink_to_fit();
	}
	//);



	ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	desc.ByteWidth = sizeof(uint) * totalIndexCount;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = indices.data();
	Check(device->CreateBuffer(&desc, &subResource, &indexBuffer));




	vertices.clear();
	vertices.shrink_to_fit();

	indices.clear();
	indices.shrink_to_fit();
	bComplete = true;
}


