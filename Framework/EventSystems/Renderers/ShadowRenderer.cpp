#include "Framework.h"
#include "ShadowRenderer.h"
#include "Renderer.h"
#include "Resources/Mesh.h"
#include "Core/D3D11/D3D11_Helper.h"
void ShadowRenderer::Depth_PreRender(ID3D11DeviceContext * context, const uint & drawCount, const uint & prevCount)
{
	context->VSSetShader(shadowVS, nullptr, 0);


	
	context->IASetVertexBuffers(slot, 1, &shadowVertexBuffer, &shadowStride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	uint temp = bMaintainShadow && drawCount == 0 ? 1 : drawCount;

	for (uint i = 0; i < meshCount; i++)
	{
		auto& m = mesh[i];
		m.ApplyPipelineNoMaterial(context, prevCount, ID);
		context->DrawIndexedInstanced(m.indexCount, temp, m.startIndex, m.startVertexIndex, 0);
		m.ClearPipeline(context);
	}



	context->VSSetShader(nullptr, nullptr, 0);
}

void ShadowRenderer::Initiallize(ID3D11Device * device, const uint & ID)
{
	this->device = device;

	this->ID = ID;

	offset = 0;
	slot = 0;
	
}

void ShadowRenderer::CreateShader(const string & file)
{
	SafeRelease(shadowVS);

	ID3DBlob* ShaderBlob = nullptr;
	auto path = file + "/CascadedShadowGenVS.cso";
	ShaderBlob = D3D11_Helper::LoadBinary(String::ToWString(path));

	Check(device->CreateVertexShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&shadowVS
	));

	inputLayout = make_shared<InputLayout>();
	inputLayout->Create(device, ShaderBlob);

	SafeRelease(ShaderBlob);

}





