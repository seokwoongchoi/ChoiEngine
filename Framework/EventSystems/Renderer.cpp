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
	
	CreateStates();

	//

	//for (uint i = 0; i < MAX_MODEL_INSTANCE; i++)
	//{
	//	D3DXMatrixIdentity(&modelDesc.instTransform[i]);
	//	
	//}
	//

	
}



void Renderer::Depth_PreRender(ID3D11DeviceContext* context)
{
	context->VSSetShader(shadowVS, nullptr, 0);
		
	
	uint depthStride = sizeof(Vector3);
	context->IASetVertexBuffers(slot, 1, &shadowVertexBuffer, &depthStride, &offset);
	context->IASetInputLayout(*sinputLayout);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	for (uint i = 0; i < meshCount; i++)
	{
		auto& m = mesh[i];
		m.ApplyPipelineNoMaterial(context);
		context->DrawIndexedInstanced(m.indexCount, drawCount, m.startIndex, m.startVertexIndex, 0);
		m.ClearPipeline(context);
	}


	
	context->VSSetShader(nullptr, nullptr, 0);
	
}

void Renderer::Reflection_PreRender(ID3D11DeviceContext * context)
{
	context->VSSetShader(vs, nullptr, 0);
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
		m.ApplyPipelineReflectionMaterial(context);
		context->DrawIndexedInstanced(m.indexCount, drawCount, m.startIndex, m.startVertexIndex, 0);
		m.ClearPipelineReflection(context);

	}

	
	
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

void Renderer::Render(ID3D11DeviceContext* context)
{
	context->VSSetShader(vs, nullptr, 0);
	context->PSSetShader(ps, nullptr, 0);
	
	context->PSSetSamplers(0, 1, &sampLinear);
	context->RSSetState(nullptr);
	
	
	
	context->IASetVertexBuffers(slot, 1, &vertexBuffer, &stride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	
	
	for (uint i = 0; i < meshCount; i++)
	{
		auto& m = mesh[i];
		
		
		m.ApplyPipeline(context);
		context->DrawIndexedInstanced(m.indexCount, drawCount, m.startIndex, m.startVertexIndex, 0);
		m.ClearPipelineMaterial(context);
	
	}
	
	

	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
	
}

void Renderer::Forward_Render(ID3D11DeviceContext * context)
{
	if (blendCount < 1) return;
	context->VSSetShader(vs, nullptr, 0);
	context->PSSetShader(forwardPS, nullptr, 0);

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
		blendMesh[i].ApplyPipeline(context);
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
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
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

		mesh[i].CreateBuffer(device,ID);
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

			blendMesh[i].CreateBuffer(device,ID);
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

void Renderer::CreateShader(const string& file)
{
	SafeRelease(vs);
	SafeRelease(shadowVS);
	SafeRelease(ps);
	SafeRelease(reflectionPS);
	
	
	ID3DBlob* ShaderBlob = nullptr;
	auto path = file;
	auto entryPoint =  "VS";
	auto shaderModel = "vs_5_0";

	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));

	

	Check( device->CreateVertexShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&vs
	));
	
	inputLayout =make_shared<InputLayout>();
	inputLayout->Create(device, ShaderBlob);
	SafeRelease(ShaderBlob);

	entryPoint = "CascadedShadowGenVS";
	shaderModel = "vs_5_0";

	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));



	Check(device->CreateVertexShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&shadowVS
	));

	sinputLayout = make_shared<InputLayout>();
	sinputLayout->Create(device, ShaderBlob);

	SafeRelease(ShaderBlob);

	entryPoint = "PS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));

	Check(device->CreatePixelShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&ps
	));

	SafeRelease(ShaderBlob);

	entryPoint = "ReflectionPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader
	(
		path,
		entryPoint,
		shaderModel,
		nullptr,
		&ShaderBlob
	));



	Check(device->CreatePixelShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&reflectionPS
	));
	
	SafeRelease(ShaderBlob);
	

	if (blendCount > 0)
	{
		SafeRelease(forwardPS);

		entryPoint = "ForwardPS";
		shaderModel = "ps_5_0";
		Check(D3D11_Helper::CompileShader
		(
			path,
			entryPoint,
			shaderModel,
			nullptr,
			&ShaderBlob
		));



		Check(device->CreatePixelShader
		(
			ShaderBlob->GetBufferPointer(),
			ShaderBlob->GetBufferSize(),
			nullptr,
			&forwardPS
		));

		SafeRelease(ShaderBlob);
	}
	


	
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
		BindingMesh<VertexTextureNormalTangent>(r);
		break;
	case ReadMeshType::SkeletalMesh:
		BindingMesh<VertexTextureNormalTangentBlend>(r);
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

