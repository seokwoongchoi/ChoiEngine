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
	CreateConstantBuffers();
	CreateStates();

	//

	//for (uint i = 0; i < MAX_MODEL_INSTANCE; i++)
	//{
	//	D3DXMatrixIdentity(&modelDesc.instTransform[i]);
	//	
	//}
	//

	
}

void Renderer::Update(ID3D11DeviceContext * context)
{
	
	
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(worldBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &modelDesc, sizeof(modelDesc));
	context->Unmap(worldBuffer, 0);

	
}

void Renderer::Depth_PreRender(ID3D11DeviceContext* context)
{
	context->VSSetShader(shadowVS, nullptr, 0);
	context->VSSetConstantBuffers(3, 1, &worldBuffer);
	context->VSSetShaderResources(0, 1, &srv);
	
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
	context->VSSetShaderResources(0, 1, &nullSRV);
	context->VSSetConstantBuffers(3, 1, &nullBuffer);
}

void Renderer::Reflection_PreRender(ID3D11DeviceContext * context)
{
	context->VSSetShader(vs, nullptr, 0);
	context->PSSetShader(reflectionPS, nullptr, 0);

	context->PSSetSamplers(0, 1, &sampLinear);
	context->RSSetState(nullptr);
	
	context->VSSetConstantBuffers(3, 1, &worldBuffer);
	context->VSSetShaderResources(0, 1, &srv);


	
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

	context->VSSetShaderResources(0, 1, &nullSRV);
	context->VSSetConstantBuffers(3, 1, &nullBuffer);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);

}

void Renderer::Render(ID3D11DeviceContext* context)
{
	context->VSSetShader(vs, nullptr, 0);


	context->PSSetShader(ps, nullptr, 0);
	
	context->PSSetSamplers(0, 1, &sampLinear);
	context->RSSetState(nullptr);
	
	context->VSSetConstantBuffers(3, 1, &worldBuffer);
	context->VSSetShaderResources(0, 1, &srv);

	
	
	
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
	
	

	context->VSSetShaderResources(0, 1, &nullSRV);
	context->VSSetConstantBuffers(3, 1, &nullBuffer);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
	
}

void Renderer::Forward_Render(ID3D11DeviceContext * context)
{
	if (blendCount < 1) return;

	context->VSSetShader(vs, nullptr, 0);
	context->PSSetShader(forwardPS, nullptr, 0);

	context->PSSetSamplers(0, 1, &sampLinear);
	context->RSSetState(nullptr);

	context->VSSetConstantBuffers(3, 1, &worldBuffer);
	context->VSSetShaderResources(0, 1, &srv);




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
	context->VSSetShaderResources(0, 1, &nullSRV);
	context->VSSetConstantBuffers(3, 1, &nullBuffer);
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

void Renderer::CreateConstantBuffers()
{
	SafeRelease(worldBuffer);
	// Allocate constant buffers
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(ModelData);
	Check(device->CreateBuffer(&bufferDesc, NULL, &worldBuffer));

	
}

bool Renderer::ReadMesh(const wstring & name, const ReadMeshType & meshType)
{
	//Thread::Get()->AddTask([&,name,meshType]()
	//{
	
	  

       BinaryReader* r = new BinaryReader();
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

        r->Open(path);
     
        boneCount = r->UInt();
		bones.clear();
		bones.shrink_to_fit();

        for (UINT i = 0; i < boneCount; i++)
        {
        	auto& bone = make_shared<ModelBone>();
        	bone->index = r->Int();
        	bone->name = r->String();
        	bone->parentIndex = r->Int();
        	bone->transform = r->Matrix();
        	bones.emplace_back(bone);
        }
		BindingBone(meshType);


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
	 
	
		
        for (UINT i = 0; i < meshCount+ blendCount; i++)
        {
			
			if (blendCount > 0 &&i== blendMeshIndex)
			{
				for (uint b = 0; b < blendCount; b++)
				{
					blendMesh[b].name = r->String();
					blendMesh[b].boneIndex = r->Int();
					blendMesh[b].materialName = r->String();
					blendMesh[b].vertexCount = r->UInt();
					blendMesh[b].startVertexIndex = r->UInt();
					blendMesh[b].indexCount = r->UInt();
					blendMesh[b].startIndex = r->UInt();

					blendMesh[b].minPos.x = r->Float();
					blendMesh[b].minPos.y = r->Float();
					blendMesh[b].minPos.z = r->Float();
					blendMesh[b].maxPos.x = r->Float();
					blendMesh[b].maxPos.y = r->Float();
					blendMesh[b].maxPos.z = r->Float();
				}
				
				continue;
			}
		
			
                mesh[i].name = r->String();
                mesh[i].boneIndex = r->Int();
                mesh[i].materialName = r->String();
				mesh[i].vertexCount = r->UInt();
				mesh[i].startVertexIndex = r->UInt();
				mesh[i].indexCount = r->UInt();
				mesh[i].startIndex = r->UInt();            

				mesh[i].minPos.x = r->Float();
                mesh[i].minPos.y = r->Float();
                mesh[i].minPos.z = r->Float();
                mesh[i].maxPos.x = r->Float();
                mesh[i].maxPos.y = r->Float();
                mesh[i].maxPos.z = r->Float();
        }

		switch (meshType)
		{
		case ReadMeshType::StaticMesh:
		{
			BindingStaticMesh(r);
		}
		break;
		case ReadMeshType::SkeletaMesh:
		{
			BindingSkeletalMesh(r);
		}
		break;

		}
	

        r->Close();
        SafeDelete(r);



		for (uint i = 0; i < meshCount; i++)
		{

			const uint& tempIndex = mesh[i].boneIndex;
			auto findBone = find_if(bones.begin(), bones.end(), [=, &tempIndex](shared_ptr<ModelBone> bone)
			{
				return bone->index == tempIndex;

			});
			if (findBone != bones.end())
			{
				mesh[i].bone = *findBone;
			}

			mesh[i].CreateBuffer(device);
			for (auto& material : materials)
			{
				if (mesh[i].materialName == String::ToString(material->Name()))
				{
					mesh[i].material = material;
				}
			}
		}
		materials.clear();
		materials.shrink_to_fit();
	       
   // });

	return true;
}

void Renderer::ReadMaterial(const wstring& name)
{

	materials.clear();
	materials.shrink_to_fit();

	auto path = L"../../_Textures/"+ name+L"/" + name + L".material";

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

void Renderer::CreateModelTransformSRV()
{
	SafeRelease(texture);
	SafeRelease(srv);
	
	//CreateTexture
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
		desc.Width = boneCount * 4;
		desc.Height = MAX_MODEL_INSTANCE;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		Matrix* temp = new Matrix[boneCount];
		Matrix* boneTransforms = new Matrix[boneCount];
		
		for (UINT i = 0; i < MAX_MODEL_INSTANCE; i++)
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
				uint index =  b;
				boneTransforms[index] = matrix * temp[b];
				
			}//for(b)
		}//for(i)
		
		SafeDeleteArray(temp);
		bones.clear();
		bones.shrink_to_fit();
	

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

void Renderer::CreateModelAnimationSRV()
{
}

void Renderer::BindingBone(const ReadMeshType& meshType)
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
		
		CreateModelTransformSRV();
	}
	break;
	case ReadMeshType::SkeletaMesh:
	{
		CreateModelAnimationSRV();
	}
	break;
	}
	
}

void Renderer::BindingStaticMesh(BinaryReader* r)
{
	vector< VertexTextureNormalTangent> vertices;
	stride = sizeof(VertexTextureNormalTangent);
	vector< Vector3> shadowVertices;
	vector< uint> indices;

	uint totalCount = r->UInt();
	uint totalIndexCount = r->UInt();

	vertices.assign(totalCount, VertexTextureNormalTangent());
	shadowVertices.assign(totalCount, Vector3());
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



	//Thread::Get()->AddTask([&]()
	{
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
	shadowVertices.clear();
	shadowVertices.shrink_to_fit();
	indices.clear();
	indices.shrink_to_fit();


}

void Renderer::BindingSkeletalMesh(BinaryReader* r)
{
}
