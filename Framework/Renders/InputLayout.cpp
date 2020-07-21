#include "Framework.h"
#include "InputLayout.h"

InputLayout::InputLayout()
	: inputLayout(nullptr)
{
	
}

InputLayout::~InputLayout()
{
	Clear();
}

void InputLayout::Create(ID3D11Device* device, D3D11_INPUT_ELEMENT_DESC * descs, const uint & count, ID3DBlob * blob)
{
	if (descs == nullptr || count == 0)
		assert(false);

	auto hr = device->CreateInputLayout
	(
		descs,
		count,
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		&inputLayout
	);
	assert(SUCCEEDED(hr));
}


void InputLayout::Create(ID3D11Device* device, ID3DBlob * blob)
{
	if (blob == nullptr)
		assert(false);

	//CreateShaderResourceView Shader Reflection
	ID3D11ShaderReflection* reflector = nullptr;
	auto hr = D3DReflect
	(
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		__uuidof(ID3D11ShaderReflection), // IID_ID3D11ShaderReflection
		reinterpret_cast<void**>(&reflector)
	);
	assert(SUCCEEDED(hr));

	//Get Shader Information
	D3D11_SHADER_DESC shaderDesc;
	reflector->GetDesc(&shaderDesc);

	//Get Element Information
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDescs;
	for (uint i = 0; i < shaderDesc.InputParameters; i++)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		reflector->GetInputParameterDesc(i, &paramDesc);

		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName			= paramDesc.SemanticName;
		elementDesc.SemanticIndex			= paramDesc.SemanticIndex;
		elementDesc.InputSlot				= 0;
		elementDesc.AlignedByteOffset		= D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass			= D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate	= 0;

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
			inputLayoutDescs.push_back(elementDesc);

		//inputLayoutDescs.emplace_back(elementDesc);
	}

	hr = device->CreateInputLayout
	(
		inputLayoutDescs.data(),
        static_cast<uint>(inputLayoutDescs.size()),
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		&inputLayout
	);
	assert(SUCCEEDED(hr));
	SafeRelease(reflector);
}

void InputLayout::Clear()
{
	SafeRelease(inputLayout);
}


