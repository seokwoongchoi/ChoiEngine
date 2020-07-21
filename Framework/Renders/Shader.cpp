#include "Framework.h"
#include "Shader.h"

Shader::Shader(ID3D11Device* device,const wstring& file)
	:device(device), file(L"../_Shaders/" + file), initialStateBlock(nullptr), effectDesc{}, effect(nullptr)
{
	ID3D11DeviceContext* context;
	device->GetImmediateContext(&context);
	initialStateBlock = new StateBlock();
	{
		context->RSGetState(&initialStateBlock->RSRasterizerState);
		context->OMGetBlendState(&initialStateBlock->OMBlendState, initialStateBlock->OMBlendFactor, &initialStateBlock->OMSampleMask);
		context->OMGetDepthStencilState(&initialStateBlock->OMDepthStencilState, &initialStateBlock->OMStencilRef);
	}

	CreateEffect();
}

Shader::~Shader()
{
	
	for (Technique& temp : techniques)
	{
		for (Pass& pass : temp.Passes)
		{
			SafeRelease(pass.InputLayout);
		}
	}
	

	SafeDelete(initialStateBlock);
	SafeRelease(effect);
}

void Shader::CreateEffect()
{
	if (Path::ExistFile(file) == false)
	{
		MessageBox(NULL, file.c_str(), L"파일을 찾을 수 없음", MB_OK);
		assert(false);
	}

	ID3DBlob* fxBlob;
	if (Path::GetExtension(file) == L"fx")
	{
		ID3DBlob* error;
		INT flag = D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY | D3D10_SHADER_PACK_MATRIX_ROW_MAJOR;

		HRESULT hr = D3DCompileFromFile(file.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, NULL, "fx_5_0", flag, NULL, &fxBlob, &error);
		if (FAILED(hr))
		{
			if (error != NULL)
			{
				string str = (const char *)error->GetBufferPointer();
				MessageBoxA(NULL, str.c_str(), "Shader Error", MB_OK);
			}
			assert(false);
		}
	}
	else if (Path::GetExtension(file) == L"fxo")
	{
		HANDLE fileHandle = CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		bool bChecked = fileHandle != INVALID_HANDLE_VALUE;
		assert(bChecked);



		DWORD dataSize = GetFileSize(fileHandle, NULL);
		assert(dataSize != 0xFFFFFFFF);

		void* data = malloc(dataSize);
		DWORD readSize;
		Check(ReadFile(fileHandle, data, dataSize, &readSize, NULL));

		CloseHandle(fileHandle);
		fileHandle = NULL;

		D3DCreateBlob(dataSize, &fxBlob);
		memcpy(fxBlob->GetBufferPointer(), data, dataSize);
	}
	else
	{
		wstring errorMsg = wstring(L"이펙트 파일이 아님 : ") + file;
		MessageBox(NULL, errorMsg.c_str(), L"Shader Error", MB_OK);

		assert(false);
	}
	Check(D3DX11CreateEffectFromMemory(fxBlob->GetBufferPointer(), fxBlob->GetBufferSize(), 0, device, &effect));


	effect->GetDesc(&effectDesc);
	for (UINT t = 0; t < effectDesc.Techniques; t++)
	{
		Technique technique;
		technique.ITechnique = effect->GetTechniqueByIndex(t);
		technique.ITechnique->GetDesc(&technique.Desc);
		technique.Name = String::ToWString(technique.Desc.Name);

		for (UINT p = 0; p < technique.Desc.Passes; p++)
		{
			Pass pass;
			pass.IPass = technique.ITechnique->GetPassByIndex(p);
			pass.IPass->GetDesc(&pass.Desc);
			pass.Name = String::ToWString(pass.Desc.Name);
			pass.IPass->GetVertexShaderDesc(&pass.PassVsDesc);
			pass.PassVsDesc.pShaderVariable->GetShaderDesc(pass.PassVsDesc.ShaderIndex, &pass.EffectVsDesc);

			for (UINT s = 0; s < pass.EffectVsDesc.NumInputSignatureEntries; s++)
			{
				D3D11_SIGNATURE_PARAMETER_DESC desc;

				HRESULT hr = pass.PassVsDesc.pShaderVariable->GetInputSignatureElementDesc(pass.PassVsDesc.ShaderIndex, s, &desc);
				Check(hr);

				pass.SignatureDescs.push_back(desc);
			}

			pass.InputLayout = CreateInputLayout(fxBlob, &pass.EffectVsDesc, pass.SignatureDescs);
			pass.StateBlock = initialStateBlock;

			technique.Passes.push_back(pass);
		}

		techniques.push_back(technique);
	}

	for (UINT i = 0; i < effectDesc.ConstantBuffers; i++)
	{
		ID3DX11EffectConstantBuffer* iBuffer;
		iBuffer = effect->GetConstantBufferByIndex(i);

		D3DX11_EFFECT_VARIABLE_DESC vDesc;
		iBuffer->GetDesc(&vDesc);

		int a = 0;
	}

	for (UINT i = 0; i < effectDesc.GlobalVariables; i++)
	{
		ID3DX11EffectVariable* iVariable;
		iVariable = effect->GetVariableByIndex(i);

		D3DX11_EFFECT_VARIABLE_DESC vDesc;
		iVariable->GetDesc(&vDesc);

		int a = 0;
	}

	SafeRelease(fxBlob);
}

ID3D11InputLayout * Shader::CreateInputLayout(const ID3DBlob * fxBlob, const D3DX11_EFFECT_SHADER_DESC* effectVsDesc, const vector<D3D11_SIGNATURE_PARAMETER_DESC>& params)
{
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
	for (auto & paramDesc : params)
	{
		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		if (paramDesc.Mask == 1)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (paramDesc.Mask <= 3)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (paramDesc.Mask <= 7)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (paramDesc.Mask <= 15)
		{
			if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
				elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}

		string name = paramDesc.SemanticName;
		transform(name.begin(), name.end(), name.begin(), toupper);

		if (name == "POSITION")
		{
			elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			//elementDesc.InputSlot = paramDesc.SemanticIndex;
		}


		if (String::StartsWith(name, "INST") == true)
		{
			elementDesc.InputSlot = 1;
			elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			elementDesc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
			elementDesc.InstanceDataStepRate = 1;
		}

		if (String::StartsWith(name, "SV_") == false)
			inputLayoutDesc.push_back(elementDesc);
	}


	const void* pCode = effectVsDesc->pBytecode;
	UINT pCodeSize = effectVsDesc->BytecodeLength;

	if (inputLayoutDesc.size() > 0)
	{
		ID3D11InputLayout* inputLayout = NULL;
		HRESULT hr = device->CreateInputLayout
		(
			&inputLayoutDesc[0]
			, inputLayoutDesc.size()
			, pCode
			, pCodeSize
			, &inputLayout
		);
		Check(hr);

		return inputLayout;
	}

	return NULL;
}



void Shader::Pass::Draw(UINT vertexCount, UINT startVertexLocation)
{
	BeginDraw();
	{
		context->Draw(vertexCount, startVertexLocation);
	}
	EndDraw();
}

void Shader::Pass::DrawIndexed(UINT indexCount, UINT startIndexLocation, INT baseVertexLocation)
{
	BeginDraw();
	{
		context->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
	}
	EndDraw();
}

void Shader::Pass::DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation)
{
	BeginDraw();
	{
		context->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
	}
	EndDraw();
}

void Shader::Pass::DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation)
{
	BeginDraw();
	{
		context->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startIndexLocation);
	}
	EndDraw();
}



void Shader::Pass::DrawInstancedIndirect(ID3D11Buffer * buffer, uint offset)
{
	BeginDraw();
	{
		context->DrawInstancedIndirect(buffer, offset);
	}
	EndDraw();
}

void Shader::Pass::BeginDraw()
{
	IPass->ComputeStateBlockMask(&StateBlockMask);

	context->IASetInputLayout(InputLayout);
	IPass->Apply(0, context);
}

void Shader::Pass::EndDraw()
{
	if (StateBlockMask.RSRasterizerState == 1)
		context->RSSetState(StateBlock->RSRasterizerState);

	if (StateBlockMask.OMDepthStencilState == 1)
		context->OMSetDepthStencilState(StateBlock->OMDepthStencilState, StateBlock->OMStencilRef);

	if (StateBlockMask.OMBlendState == 1)
		context->OMSetBlendState(StateBlock->OMBlendState, StateBlock->OMBlendFactor, StateBlock->OMSampleMask);

	context->HSSetShader(NULL, NULL, 0);
	context->DSSetShader(NULL, NULL, 0);
	context->GSSetShader(NULL, NULL, 0);
}

void Shader::Pass::IndirectBeginDraw()
{
	IPass->ComputeStateBlockMask(&StateBlockMask);
		
	IPass->Apply(0, context);
}

void Shader::Pass::IndirectEndDraw()
{
	context->OMSetBlendState(StateBlock->OMBlendState, StateBlock->OMBlendFactor, StateBlock->OMSampleMask);
	context->GSSetShader(NULL, NULL, 0);
}

void Shader::Pass::Dispatch(UINT x, UINT y, UINT z)
{
	IPass->Apply(0, context);
	context->Dispatch(x, y, z);

	
	ID3D11ShaderResourceView* null[1] = { 0 };
	context->CSSetShaderResources(0, 1, null);

	ID3D11UnorderedAccessView* nullUav[1] = { 0 };
	context->CSSetUnorderedAccessViews(0, 1, nullUav, NULL);

	context->CSSetShader(NULL, NULL, 0);
}



ID3DX11EffectVariable * Shader::Variable(string name)
{
	return effect->GetVariableByName(name.c_str());
}

ID3DX11EffectScalarVariable * Shader::AsScalar(string name)
{
	return effect->GetVariableByName(name.c_str())->AsScalar();
}

ID3DX11EffectVectorVariable * Shader::AsVector(string name)
{
	return effect->GetVariableByName(name.c_str())->AsVector();
}

ID3DX11EffectMatrixVariable * Shader::AsMatrix(string name)
{
	return effect->GetVariableByName(name.c_str())->AsMatrix();
}

ID3DX11EffectStringVariable * Shader::AsString(string name)
{
	return effect->GetVariableByName(name.c_str())->AsString();
}

ID3DX11EffectShaderResourceVariable * Shader::AsSRV(string name)
{
	return effect->GetVariableByName(name.c_str())->AsShaderResource();
}

ID3DX11EffectRenderTargetViewVariable * Shader::AsRTV(string name)
{
	return effect->GetVariableByName(name.c_str())->AsRenderTargetView();
}

ID3DX11EffectDepthStencilViewVariable * Shader::AsDSV(string name)
{
	return effect->GetVariableByName(name.c_str())->AsDepthStencilView();
}

ID3DX11EffectConstantBuffer * Shader::AsConstantBuffer(string name)
{
	return effect->GetConstantBufferByName(name.c_str());
}

ID3DX11EffectShaderVariable * Shader::AsShader(string name)
{
	return effect->GetVariableByName(name.c_str())->AsShader();
}

ID3DX11EffectBlendVariable * Shader::AsBlend(string name)
{
	return effect->GetVariableByName(name.c_str())->AsBlend();
}

ID3DX11EffectDepthStencilVariable * Shader::AsDepthStencil(string name)
{
	return effect->GetVariableByName(name.c_str())->AsDepthStencil();
}

ID3DX11EffectRasterizerVariable * Shader::AsRasterizer(string name)
{
	return effect->GetVariableByName(name.c_str())->AsRasterizer();
}

ID3DX11EffectSamplerVariable * Shader::AsSampler(string name)
{
	return effect->GetVariableByName(name.c_str())->AsSampler();
}

ID3DX11EffectUnorderedAccessViewVariable * Shader::AsUAV(string name)
{
	return effect->GetVariableByName(name.c_str())->AsUnorderedAccessView();
}