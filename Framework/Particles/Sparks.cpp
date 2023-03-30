#include "Framework.h"
#include "Sparks.h"
#include "Core/D3D11/D3D11_Helper.h"
#include "GBuffer/LightManager.h"
extern class LightManager Lighting;
Sparks::Sparks(ID3D11Device* device,uint ID)
	:ParticleSimulation(device), wvpBuffer(nullptr),
	 NoDepth(nullptr), AdditiveBlendState(nullptr), simulateBuffer(nullptr),  readBuffer(0),
	particleArray(nullptr), bodyData{0,nullptr,nullptr}, drawCount(0)//, vertexBuffer(nullptr)
{
	
	
	id = ID;
	// Create the additive blend state

	D3D11_BLEND_DESC descBlend;
	descBlend.AlphaToCoverageEnable = FALSE;
	descBlend.IndependentBlendEnable = FALSE;
	D3D11_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		TRUE,
		D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, //srcBlend,descBlend,BlendOp
		D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,//srcBlendAlpha,destBlendAlpha,BlendOpAlpha
		D3D11_COLOR_WRITE_ENABLE_ALL,//rendertargetWriteMask
	};

	
	

	for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		descBlend.RenderTarget[i] = defaultRenderTargetBlendDesc;
	Check(device->CreateBlendState(&descBlend, &AdditiveBlendState));

	//Create Rasterizer
	D3D11_RASTERIZER_DESC reasterizerDesc;
	ZeroMemory(&reasterizerDesc, sizeof(reasterizerDesc));
	reasterizerDesc.CullMode = D3D11_CULL_NONE;
	reasterizerDesc.FillMode = D3D11_FILL_SOLID;
	reasterizerDesc.FrontCounterClockwise = false;
	reasterizerDesc.DepthClipEnable = true;
	Check(device->CreateRasterizerState(&reasterizerDesc, &rsState));

	D3D11_DEPTH_STENCIL_DESC tmpDsDesc;
	tmpDsDesc.DepthEnable = false;
	tmpDsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	tmpDsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	tmpDsDesc.StencilEnable = FALSE;
	Check(device->CreateDepthStencilState(&tmpDsDesc, &NoDepth));


	CreateShaders(device);
	CreateBuffers(device);
}

Sparks::~Sparks()
{
}

void Sparks::InitBasicSpark()
{

	if (bodyData.Bodies != simulateDesc.numParticles)
	{
		bodyData.Bodies = simulateDesc.numParticles;
		SafeDeleteArray(bodyData.position);
		SafeDeleteArray(bodyData.velocity);
	
		bodyData.position = new float[bodyData.Bodies * 3];
		bodyData.velocity = new float[bodyData.Bodies * 3];
		
		SafeDeleteArray(particleArray);
		particleArray = new Vector4[bodyData.Bodies * 3];
		
	}



	float scale = clusterScale;
	float vscale = scale * velocityScale;
	float inner = 2.5f * scale;
	float outer = 4.0f * scale;
	int p = 0, v = 0, t = 0;

	unsigned int i = 0;
	srand(12);
	while (i < bodyData.Bodies)
	{
		float x, y, z;
		x = rand() / static_cast<float>(RAND_MAX) * 2.0f - 1.0f;
		y = rand() / static_cast<float>(RAND_MAX) * 2.0f - 1.0f;
		z = rand() / static_cast<float>(RAND_MAX) * 2.0f - 1.0f;



		Vector3 point(x, y, z);
		float len = D3DXVec3Length(&point);
		D3DXVec3Normalize(&point, &point);
		if (len > 1)
			continue;

		bodyData.position[p++] = point.x * (inner + (outer - inner) * rand() / static_cast<float>(RAND_MAX));
		bodyData.position[p++] = point.y * (inner + (outer - inner) * rand() / static_cast<float>(RAND_MAX));
		bodyData.position[p++] = point.z * (inner + (outer - inner) * rand() / static_cast<float>(RAND_MAX));

		x = (rand() / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
		y = (rand() / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
		z = (rand() / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
		Vector3 axis(x, y, z);
		D3DXVec3Normalize(&axis, &axis);

		if (1 - D3DXVec3Dot(&point, &axis) < 1e-6)
		{
			axis.x = point.y;
			axis.y = point.x;
			D3DXVec3Normalize(&axis, &axis);
		}

		Vector3 vv(bodyData.position[3 * i], bodyData.position[3 * i + 1], bodyData.position[3 * i + 2]);
		D3DXVec3Cross(&vv, &vv, &axis);

		bodyData.velocity[v++] = vv.x * vscale;
		bodyData.velocity[v++] = vv.y * vscale;
		bodyData.velocity[v++] = vv.z * vscale;

		i++;
	}


}

void Sparks::CreateShaders(ID3D11Device * device)
{
	SafeRelease(VS);
	SafeRelease(preivewVS);
	SafeRelease(GS);
	SafeRelease(PS);
	SafeRelease(updateCS);
	ID3DBlob* ShaderBlob = nullptr;
	 //path = "../_Shaders/Particles/nBodyCS.hlsl";
	 //entryPoint = "CSMAIN";
	 //shaderModel = "cs_5_0";
	
	ShaderBlob = D3D11_Helper::LoadBinary(L"../_Shaders/Particles/nBody_cs.cso");

	Check(device->CreateComputeShader
	(
		ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(),
		nullptr,
		&updateCS
	));



	SafeRelease(ShaderBlob);

	auto path = "../_Shaders/Particles/ParticleSystemRender.hlsl";
	auto entryPoint = "VSMAIN";
	auto shaderModel = "vs_5_0";

	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));

	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &VS));

	SafeRelease(ShaderBlob);

	entryPoint = "PreviewVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));

	Check(device->CreateVertexShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &preivewVS));

	SafeRelease(ShaderBlob);

	entryPoint = "GSMAIN";
	shaderModel = "gs_5_0";

	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));

	Check(device->CreateGeometryShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &GS));
	SafeRelease(ShaderBlob);

	entryPoint = "PSMAIN";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &ShaderBlob));
	Check(device->CreatePixelShader(ShaderBlob->GetBufferPointer(),
		ShaderBlob->GetBufferSize(), NULL, &PS));
	SafeRelease(ShaderBlob);
}

void Sparks::CreateBuffers(ID3D11Device * device)
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(WVPDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &wvpBuffer));

	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(SimulationParametersDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &simulateBuffer));

}

int Sparks::InitBodies(const wstring& path)
{
	BinaryReader* r = new BinaryReader();
	r->Open(path);
	   	
	simulateDesc.numParticles = r->UInt();
	simulateDesc.softeningSquared= r->Float();
	simulateDesc.distance= r->Float();
	clusterScale = r->Float();
    velocityScale =  r->Float();
	wvpDesc.intensity = r->Float();
	wvpDesc.pontSize= r->Float();
	wvpDesc.pontSize *= 0.1f;
	simulateDesc.timer= r->Float();
	string temp = r->String();
	if (temp.length()>0)
	{
		SafeDelete(particleTexture);
		particleTexture = new Texture();
		particleTexture->Load(device, String::ToWString(temp));
	}

	wvpDesc.positionIndex = r->Int();
	
	r->Close();
	SafeDelete(r);


	InitBasicSpark();
	if (wvpDesc.positionIndex == 0)
	{
		SafeRelease(AdditiveBlendState);
		D3D11_BLEND_DESC descBlend;
		descBlend.AlphaToCoverageEnable = FALSE;
		descBlend.IndependentBlendEnable = FALSE;
		D3D11_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			TRUE,
				D3D11_BLEND_DEST_COLOR, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, //srcBlend,descBlend,BlendOp
				D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,//srcBlendAlpha,destBlendAlpha,BlendOpAlpha
				D3D11_COLOR_WRITE_ENABLE_ALL,//rendertargetWriteMask
		};
		for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		descBlend.RenderTarget[i] = defaultRenderTargetBlendDesc;
		
		Check(device->CreateBlendState(&descBlend, &AdditiveBlendState));
	}

	//vector< Vector3> vertices;
	//
	//vertices.assign(simulateDesc.numParticles, Vector3());

	//SafeRelease(vertexBuffer);


	//D3D11_BUFFER_DESC desc;
	//ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));
	//desc.ByteWidth = sizeof(Vector3) *simulateDesc.numParticles;
	//desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	//desc.Usage = D3D11_USAGE_DEFAULT;
	//D3D11_SUBRESOURCE_DATA subResource;
	//ZeroMemory(&subResource, sizeof(subResource));
	//subResource.pSysMem = vertices.data();
	//Check(device->CreateBuffer(&desc, &subResource, &vertexBuffer));

	return wvpDesc.positionIndex;

}

void Sparks::InitBodies()
{
	
	
	simulateDesc.runningTime = 0.0f;
	InitBasicSpark();
	Reset(0);
	
}

void Sparks::Update(ID3D11DeviceContext * context)
{
	if (simulateDesc.timer <= simulateDesc.runningTime) return;

	{

		simulateDesc.timestep =Time::Delta();
		simulateDesc.readOffset = readBuffer * simulateDesc.numParticles;
		simulateDesc.writeOffset = (1 - readBuffer) * simulateDesc.numParticles;

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(simulateBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &simulateDesc, sizeof(simulateDesc));
		context->Unmap(simulateBuffer, 0);
	}
	context->CSSetConstantBuffers(0, 1, &simulateBuffer);

	uint initCounts = 0;
	context->CSSetUnorderedAccessViews(0, 1, &StructuredBufferUAV, &initCounts);
	context->CSSetShader(updateCS, nullptr, 0);
	context->Dispatch(simulateDesc.numParticles / 256, 1, 1);
	
	readBuffer = 1 - readBuffer;


	context->CSSetShader(nullptr, nullptr, 0);
	ID3D11UnorderedAccessView* nullUav = nullptr;
	context->CSSetUnorderedAccessViews(0, 1, &nullUav, &initCounts);
	ID3D11Buffer* nullBuffer = nullptr;
	context->CSSetConstantBuffers(0, 1, &nullBuffer);
}

void Sparks::Render(ID3D11DeviceContext * context, ID3D11ShaderResourceView* positionSRV)
{
	
	if (simulateDesc.timer <= simulateDesc.runningTime|| positionSRV==nullptr)
	{
		
		Lighting.ClearEffectPointLights();
		
		if (drawCount > 0)
			drawCount--;
		return;
		//return drawCount;
	}
	context->SetPredication(nullptr, false);
//	while (simulateDesc.timer > simulateDesc.runningTime)
	{
		{

			simulateDesc.timestep = Time::Delta();
			simulateDesc.readOffset = readBuffer * simulateDesc.numParticles;
			simulateDesc.writeOffset = (1 - readBuffer) * simulateDesc.numParticles;

			D3D11_MAPPED_SUBRESOURCE MappedResource;
			context->Map(simulateBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
			memcpy(MappedResource.pData, &simulateDesc, sizeof(simulateDesc));
			context->Unmap(simulateBuffer, 0);
		}
		context->CSSetConstantBuffers(0, 1, &simulateBuffer);

		uint initCounts = 0;
		context->CSSetUnorderedAccessViews(0, 1, &StructuredBufferUAV, &initCounts);
		context->CSSetShader(updateCS, nullptr, 0);
		context->Dispatch(simulateDesc.numParticles / 256, 1, 1);

		readBuffer = 1 - readBuffer;


		context->CSSetShader(nullptr, nullptr, 0);
		ID3D11UnorderedAccessView* nullUav = nullptr;
		context->CSSetUnorderedAccessViews(0, 1, &nullUav, &initCounts);
		ID3D11Buffer* nullBuffer = nullptr;
		context->CSSetConstantBuffers(0, 1, &nullBuffer);
		{
			Matrix vp;
			GlobalData::GetVP(&vp);
			D3DXMatrixTranspose(&wvpDesc.WVP, &vp);
			wvpDesc.readOffset = readBuffer * simulateDesc.numParticles;

			D3D11_MAPPED_SUBRESOURCE MappedResource;
			context->Map(wvpBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
			memcpy(MappedResource.pData, &wvpDesc, sizeof(wvpDesc));
			context->Unmap(wvpBuffer, 0);
		}

		context->VSSetConstantBuffers(0, 1, &wvpBuffer);

	
		ID3D11ShaderResourceView* srrArray[2] = { StructuredBufferSRV,positionSRV };

		context->VSSetShaderResources(0, 2, srrArray);
		ID3D11ShaderResourceView* srv = *particleTexture;
		context->PSSetShaderResources(2, 1, &srv);
		context->PSSetSamplers(0, 1, &sampler);



		context->VSSetShader(VS, nullptr, 0);
		context->GSSetShader(GS, nullptr, 0);
		context->PSSetShader(PS, nullptr, 0);
		context->RSSetState(rsState);
		context->OMSetDepthStencilState(NoDepth, 0);
		context->IASetInputLayout(nullptr);
		
		context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);



		ID3D11BlendState* pPrevBlendState;
		FLOAT prevBlendFactor[4];
		UINT prevSampleMask;
		context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);

		context->OMSetBlendState(AdditiveBlendState, prevBlendFactor, prevSampleMask);

		
		context->DrawInstanced(simulateDesc.numParticles, drawCount,0,0);





		context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
		context->RSSetState(nullptr);
		context->VSSetShader(nullptr, nullptr, 0);
		context->GSSetShader(nullptr, nullptr, 0);
		context->PSSetShader(nullptr, nullptr, 0);
		ZeroMemory(srrArray, sizeof(srrArray));
		context->VSSetShaderResources(0, 2, srrArray);
		ID3D11ShaderResourceView* nullsrv = nullptr;
		context->PSSetShaderResources(2, 1, &nullsrv);
		//ID3D11Buffer* nullBuffer = nullptr;
		context->VSSetConstantBuffers(0, 1, &nullBuffer);


		simulateDesc.runningTime += simulateDesc.timestep;
	}
	
}

void Sparks::Render(ID3D11DeviceContext * context, ID3D11ShaderResourceView * positionSRV, ID3D11Buffer * indirectBuffer)
{
	if (simulateDesc.timer <= simulateDesc.runningTime || positionSRV == nullptr)
	{

		Lighting.ClearEffectPointLights();

		
		return;
		//return drawCount;
	}
	context->SetPredication(nullptr, false);
	//	while (simulateDesc.timer > simulateDesc.runningTime)
	{
		{

			simulateDesc.timestep = Time::Delta();
			simulateDesc.readOffset = readBuffer * simulateDesc.numParticles;
			simulateDesc.writeOffset = (1 - readBuffer) * simulateDesc.numParticles;

			D3D11_MAPPED_SUBRESOURCE MappedResource;
			context->Map(simulateBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
			memcpy(MappedResource.pData, &simulateDesc, sizeof(simulateDesc));
			context->Unmap(simulateBuffer, 0);
		}
		context->CSSetConstantBuffers(0, 1, &simulateBuffer);

		uint initCounts = 0;
		context->CSSetUnorderedAccessViews(0, 1, &StructuredBufferUAV, &initCounts);
		context->CSSetShader(updateCS, nullptr, 0);
		context->Dispatch(simulateDesc.numParticles / 256, 1, 1);

		readBuffer = 1 - readBuffer;


		context->CSSetShader(nullptr, nullptr, 0);
		ID3D11UnorderedAccessView* nullUav = nullptr;
		context->CSSetUnorderedAccessViews(0, 1, &nullUav, &initCounts);
		ID3D11Buffer* nullBuffer = nullptr;
		context->CSSetConstantBuffers(0, 1, &nullBuffer);
		{
			Matrix vp;
			GlobalData::GetVP(&vp);
			D3DXMatrixTranspose(&wvpDesc.WVP, &vp);
			wvpDesc.readOffset = readBuffer * simulateDesc.numParticles;

			D3D11_MAPPED_SUBRESOURCE MappedResource;
			context->Map(wvpBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
			memcpy(MappedResource.pData, &wvpDesc, sizeof(wvpDesc));
			context->Unmap(wvpBuffer, 0);
		}

		context->VSSetConstantBuffers(0, 1, &wvpBuffer);


		ID3D11ShaderResourceView* srrArray[2] = { StructuredBufferSRV,positionSRV };

		context->VSSetShaderResources(0, 2, srrArray);
		ID3D11ShaderResourceView* srv = *particleTexture;
		context->PSSetShaderResources(2, 1, &srv);
		context->PSSetSamplers(0, 1, &sampler);



		context->VSSetShader(VS, nullptr, 0);
		context->GSSetShader(GS, nullptr, 0);
		context->PSSetShader(PS, nullptr, 0);
		context->RSSetState(rsState);
		context->OMSetDepthStencilState(NoDepth, 0);
		context->IASetInputLayout(nullptr);
		
		context->IASetVertexBuffers(0, 0, nullptr, nullptr,nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);



		ID3D11BlendState* pPrevBlendState;
		FLOAT prevBlendFactor[4];
		UINT prevSampleMask;
		context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);

		context->OMSetBlendState(AdditiveBlendState, prevBlendFactor, prevSampleMask);
		
	//context->DrawIndexedInstancedIndirect(indirectBuffer, 0);
		context->DrawInstancedIndirect(indirectBuffer,0);
		//context->DrawInstanced(simulateDesc.numParticles, drawCount,0,0);
	




		context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
		context->RSSetState(nullptr);
		context->VSSetShader(nullptr, nullptr, 0);
		context->GSSetShader(nullptr, nullptr, 0);
		context->PSSetShader(nullptr, nullptr, 0);
		ZeroMemory(srrArray, sizeof(srrArray));
		context->VSSetShaderResources(0, 2, srrArray);
		ID3D11ShaderResourceView* nullsrv = nullptr;
		context->PSSetShaderResources(2, 1, &nullsrv);
		//ID3D11Buffer* nullBuffer = nullptr;
		context->VSSetConstantBuffers(0, 1, &nullBuffer);


		simulateDesc.runningTime += simulateDesc.timestep;
	}

}

void Sparks::PreviewRender(ID3D11DeviceContext * context, const Matrix & view, const Matrix & proj)
{
	if (simulateDesc.timer <= simulateDesc.runningTime)
	{
		return;
	}
	
	Update(context);
	{
		
		D3DXMatrixTranspose(&wvpDesc.WVP, &(view*proj));
		wvpDesc.readOffset = readBuffer * simulateDesc.numParticles;

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(wvpBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &wvpDesc, sizeof(wvpDesc));
		context->Unmap(wvpBuffer, 0);
	}

	context->VSSetConstantBuffers(0, 1, &wvpBuffer);
	context->VSSetShaderResources(0, 1, &StructuredBufferSRV);

	context->PSSetSamplers(0, 1, &sampler);

	context->RSSetState(rsState);
	context->IASetInputLayout(nullptr);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	context->VSSetShader(preivewVS, nullptr, 0);
	context->GSSetShader(GS, nullptr, 0);
	context->PSSetShader(PS, nullptr, 0);

	ID3D11BlendState* pPrevBlendState;
	FLOAT prevBlendFactor[4];
	UINT prevSampleMask;
	context->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);

	context->OMSetBlendState(AdditiveBlendState, prevBlendFactor, prevSampleMask);

	context->Draw(simulateDesc.numParticles, 0);
	context->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);


	context->RSSetState(nullptr);
	context->VSSetShader(nullptr, nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);

	ID3D11ShaderResourceView* nullsrv = nullptr;
	context->VSSetShaderResources(0, 1, &nullsrv);
	context->PSSetShaderResources(2, 1, &nullsrv);
	ID3D11Buffer* nullBuffer = nullptr;
	context->VSSetConstantBuffers(0, 1, &nullBuffer);


	simulateDesc.runningTime += simulateDesc.timestep;
}

void Sparks::Reset(const uint& drawCount)
{
	if ( bodyData.Bodies < 1)
		return;

	
	simulateDesc.runningTime = 0.0f;
	
	this->drawCount = drawCount;
	readBuffer = 0;
	
	

	for (uint i = 0; i < bodyData.Bodies; i++)
	{
		particleArray[i]=	
			Vector4(bodyData.position[i * 3 + 0],
				bodyData.position[i * 3 + 1],
				bodyData.position[i * 3 + 2], 1.0f);


		particleArray[i + bodyData.Bodies]= particleArray[i];

		particleArray[i + 2 * bodyData.Bodies]=
			Vector4(bodyData.velocity[i * 3 + 0],
				bodyData.velocity[i * 3 + 1],
				bodyData.velocity[i * 3 + 2],
				1.0f);


	}
	

	D3D11_SUBRESOURCE_DATA initData = { particleArray, 0, 0 };

	SafeRelease(StructuredBuffer);
	SafeRelease(StructuredBufferSRV);
	SafeRelease(StructuredBufferUAV);

	

	// Create Structured Buffer
	D3D11_BUFFER_DESC sbDesc;
	sbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	sbDesc.CPUAccessFlags = 0;
	sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbDesc.StructureByteStride = sizeof(Vector4);
	sbDesc.ByteWidth = sizeof(Vector4) * bodyData.Bodies * 3;
	sbDesc.Usage = D3D11_USAGE_DEFAULT;
	Check(device->CreateBuffer(&sbDesc, &initData, &StructuredBuffer));
	

	// create the Shader Resource View (SRV) for the structured buffer
	D3D11_SHADER_RESOURCE_VIEW_DESC sbSRVDesc;
	sbSRVDesc.Buffer.ElementOffset = 0;
	sbSRVDesc.Buffer.ElementWidth = sizeof(Vector4);
	sbSRVDesc.Buffer.FirstElement = 0;
	sbSRVDesc.Buffer.NumElements = bodyData.Bodies * 3;
	sbSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	sbSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	Check(device->CreateShaderResourceView(StructuredBuffer, &sbSRVDesc, &StructuredBufferSRV));
	

	// create the UAV for the structured buffer
	D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
	sbUAVDesc.Buffer.FirstElement = 0;
	sbUAVDesc.Buffer.Flags = 0;
	sbUAVDesc.Buffer.NumElements = bodyData.Bodies * 3;
	sbUAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	Check(device->CreateUnorderedAccessView(StructuredBuffer, &sbUAVDesc, &StructuredBufferUAV));
	
	//SafeDeleteArray(particleArray);

	
	
}


