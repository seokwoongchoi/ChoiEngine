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

	//uint temp = bMaintainShadow && drawCount == 0 ? 1 : drawCount;



	for (uint i = 0; i < meshCount; i++)
	{
		auto& m = mesh[i];
		m.ApplyPipelineNoMaterial(context, prevCount, ID);
		context->DrawIndexedInstanced(m.indexCount, drawCount, m.startIndex, m.startVertexIndex, 0);
		m.ClearPipeline(context);
	}



	context->VSSetShader(nullptr, nullptr, 0);
}
void ShadowRenderer::Depth_PreRender(ID3D11DeviceContext * context,  const uint & prevCount)
{
	context->VSSetShader(shadowVS, nullptr, 0);


	
	context->IASetVertexBuffers(slot, 1, &shadowVertexBuffer, &shadowStride, &offset);
	context->IASetInputLayout(*inputLayout);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//uint temp = bMaintainShadow && drawCount == 0 ? 1 : drawCount;


	
	for (uint i = 0; i < meshCount; i++)
	{
		auto& m = mesh[i];
		m.ApplyPipelineNoMaterial(context, prevCount, ID);
		context->DrawIndexedInstanced(m.indexCount, drawCount, m.startIndex, m.startVertexIndex, 0);
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



	//D3D11_BLEND_DESC descBlend;
	//descBlend.AlphaToCoverageEnable = true;
	//descBlend.IndependentBlendEnable = FALSE;
	//const D3D11_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	//{
	//	TRUE,
	//	D3D11_BLEND_DEST_COLOR, D3D11_BLEND_SRC_COLOR, D3D11_BLEND_OP_ADD, //srcBlend,descBlend,BlendOp
	//	D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,//srcBlendAlpha,destBlendAlpha,BlendOpAlpha
	//	D3D11_COLOR_WRITE_ENABLE_ALL,//rendertargetWriteMask
	//};



	//for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	//	descBlend.RenderTarget[i] = defaultRenderTargetBlendDesc;


	//Check(device->CreateBlendState(&descBlend, &BlendState));

	
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





