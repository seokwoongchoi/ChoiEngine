#include "Framework.h"
#include "Island11.h"
#include "Utility/QuadTree.h"

//#include "Core/D3D11/D3D11_Helper.h"
#define PI 3.14159265358979323846f
int gp_wrap(int a)
{
	if (a < 0) return (a + terrain_gridpoints);
	if (a >= terrain_gridpoints) return (a - terrain_gridpoints);
	return a;
}


Island11::Island11(ID3D11Device* device,QuadTree* tree)
	:viewDesc{}, device(nullptr), updateDesc{},MultiSampleCount(4), MultiSampleQuality(0), BackBufferWidth(1280.0f), BackBufferHeight(720.0f), RenderCaustics(true), TotalTime(0.0f),
	shader(nullptr),rock_bump_texture(nullptr),rock_bump_textureSRV(nullptr),rock_microbump_texture(nullptr),rock_microbump_textureSRV(nullptr),rock_diffuse_texture(nullptr),rock_diffuse_textureSRV(nullptr),
    sand_bump_texture(nullptr),sand_bump_textureSRV(nullptr),sand_microbump_texture(nullptr),sand_microbump_textureSRV(nullptr),sand_diffuse_texture(nullptr),sand_diffuse_textureSRV(nullptr),
    grass_diffuse_texture(nullptr),grass_diffuse_textureSRV(nullptr),slope_diffuse_texture(nullptr),slope_diffuse_textureSRV(nullptr),water_bump_texture(nullptr),water_bump_textureSRV(nullptr),
    reflection_color_resource(nullptr),reflection_color_resourceSRV(nullptr),reflection_color_resourceRTV(nullptr),refraction_color_resource(nullptr),refraction_color_resourceSRV(nullptr),refraction_color_resourceRTV(nullptr),
    shadowmap_resource(nullptr),shadowmap_resourceSRV(nullptr),shadowmap_resourceDSV(nullptr),reflection_depth_resource(nullptr),reflection_depth_resourceDSV(nullptr),refraction_depth_resource(nullptr),
    refraction_depth_resourceRTV(nullptr),refraction_depth_resourceSRV(nullptr),water_normalmap_resource(nullptr),water_normalmap_resourceSRV(nullptr),water_normalmap_resourceRTV(nullptr)
{
	BackBufferWidth = static_cast<float>(D3D::Width());
	BackBufferHeight = static_cast<float>(D3D::Height());
	this->device = device;
	shader = new Shader(device,L"Environment/Island11.fx");

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(ViewDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &viewBuffer));
	
	sViewBuffer = shader->AsConstantBuffer("CB_ViewMatrix");


	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(UpdateDesc);
	Check(device->CreateBuffer(&bufferDesc, NULL, &updateBuffer));

	sUpdateBuffer = shader->AsConstantBuffer("CB_UPDATE");


	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(Matrix);
	Check(device->CreateBuffer(&bufferDesc, NULL, &mainViewBuffer));

	
	CreateTerrain(tree);
	LoadTextures();
	ReCreateBuffers();
	
	sWaterNormalMapTexture = shader->AsSRV("WaterNormalMapTexture");
	sRefractionDepthTextureMS1 = shader->AsSRV("RefractionDepthTextureMS1");

	sReflectionTexture=shader->AsSRV("ReflectionTexture");
	sRefractionTexture = shader->AsSRV("RefractionTexture");
	sRefractionDepthTextureResolved = shader->AsSRV("RefractionDepthTextureResolved");

	sTerrainBeingRendered= shader->AsScalar("TerrainBeingRendered");
	sSkipCausticsCalculation=shader->AsScalar("SkipCausticsCalculation");
	
	shader->AsSRV("HeightfieldTexture")->SetResource(heightmap_textureSRV);
	shader->AsSRV("LayerdefTexture")->SetResource(layerdef_textureSRV);
	shader->AsSRV("RockBumpTexture")->SetResource(rock_bump_textureSRV);
	shader->AsSRV("RockMicroBumpTexture")->SetResource(rock_microbump_textureSRV);
	shader->AsSRV("RockDiffuseTexture")->SetResource(rock_diffuse_textureSRV);
	shader->AsSRV("SandBumpTexture")->SetResource(sand_bump_textureSRV);
	shader->AsSRV("SandMicroBumpTexture")->SetResource(sand_microbump_textureSRV);
	shader->AsSRV("SandDiffuseTexture")->SetResource(sand_diffuse_textureSRV);
	shader->AsSRV("GrassDiffuseTexture")->SetResource(grass_diffuse_textureSRV);
	shader->AsSRV("SlopeDiffuseTexture")->SetResource(slope_diffuse_textureSRV);
	shader->AsSRV("WaterBumpTexture")->SetResource(water_bump_textureSRV);
	shader->AsSRV("DepthMapTexture")->SetResource(depthmap_textureSRV);

	shader->AsScalar("HeightFieldSize")->SetFloat(terrain_gridpoints*terrain_geometry_scale);



	reflection_Viewport.Width = (float)BackBufferWidth*reflection_buffer_size_multiplier;
	reflection_Viewport.Height = (float)BackBufferHeight*reflection_buffer_size_multiplier;
	reflection_Viewport.MaxDepth = 1;
	reflection_Viewport.MinDepth = 0;
	reflection_Viewport.TopLeftX = 0;
	reflection_Viewport.TopLeftY = 0;

	

	main_Viewport.Width = (float)BackBufferWidth*main_buffer_size_multiplier;
	main_Viewport.Height = (float)BackBufferHeight*main_buffer_size_multiplier;
	main_Viewport.MaxDepth = 1;
	main_Viewport.MinDepth = 0;
	main_Viewport.TopLeftX = 0;
	main_Viewport.TopLeftY = 0;

	

	water_normalmap_resource_viewport.Width = water_normalmap_resource_buffer_size_xy;
	water_normalmap_resource_viewport.Height = water_normalmap_resource_buffer_size_xy;
	water_normalmap_resource_viewport.MaxDepth = 1;
	water_normalmap_resource_viewport.MinDepth = 0;
	water_normalmap_resource_viewport.TopLeftX = 0;
	water_normalmap_resource_viewport.TopLeftY = 0;


//
//	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	
//	// Compile the shaders
//	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
//#if defined( DEBUG ) || defined( _DEBUG )
//	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
//	// Setting this flag improves the shader debugging experience, but still allows 
//	// the shaders to be optimized and to run exactly the way they will run in 
//	// the release configuration of this program.
//	dwShaderFlags |= D3DCOMPILE_DEBUG;
//#endif
//
//	auto path = "../_Shaders/Environment/TerrainPacking.hlsl";
//	auto entryPoint = "PassThroughVS";
//	auto shaderModel = "vs_5_0";
//	// Load the directional light shaders
//	ID3DBlob* pShaderBlob = NULL;
//		
//	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
//	Check(device->CreateVertexShader(pShaderBlob->GetBufferPointer(),
//		pShaderBlob->GetBufferSize(), NULL, &TerrainPackingVS));
//
//	const D3D11_INPUT_ELEMENT_DESC TerrainLayout =
//	{ "PATCH_PARAMETERS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,   0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 };
//
//
//	auto hr = device->CreateInputLayout
//	(
//		&TerrainLayout,
//		1,
//		pShaderBlob->GetBufferPointer(),
//		pShaderBlob->GetBufferSize(),
//		&terrainInputLayout
//	);
//	assert(SUCCEEDED(hr));
//
//	SafeRelease(pShaderBlob);
//
//	entryPoint = "PatchHS";
//	shaderModel = "hs_5_0";
//	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
//	Check(device->CreateHullShader(pShaderBlob->GetBufferPointer(),
//		pShaderBlob->GetBufferSize(), NULL, &TerrainPackingPatchHS));
//	SafeRelease(pShaderBlob);
//
//	entryPoint = "HeightFieldPatchDS";
//	shaderModel = "ds_5_0";
//	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
//	Check(device->CreateDomainShader(pShaderBlob->GetBufferPointer(),
//		pShaderBlob->GetBufferSize(), NULL, &TerrainPackingDS));
//	SafeRelease(pShaderBlob);
//
//
//
//	entryPoint = "HeightFieldPatchPacking";
//	shaderModel = "ps_5_0";
//	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
//	Check(device->CreatePixelShader(pShaderBlob->GetBufferPointer(),
//		pShaderBlob->GetBufferSize(), NULL, &TerrainPackingPS));
//	SafeRelease(pShaderBlob);
//
//	////////////////////////////////
//
//	D3D11_SAMPLER_DESC samDesc;
//	ZeroMemory(&samDesc, sizeof(D3D11_SAMPLER_DESC));
//	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
//	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
//	samDesc.MaxAnisotropy = 1;
//	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
//	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
//	Check(device->CreateSamplerState(&samDesc, &SamplerLinearWrap));
//
//
//	ZeroMemory(&samDesc, sizeof(D3D11_SAMPLER_DESC));
//	samDesc.Filter = D3D11_FILTER_MAXIMUM_ANISOTROPIC;
//	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
//	samDesc.MaxAnisotropy = 16;
//	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
//	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
//	Check(device->CreateSamplerState(&samDesc, &SamplerAnisotropicWrap));
//
//	//////////////////////////////////////////////////////////////////
//
//
//	
//	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
//	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
//	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//	bufferDesc.ByteWidth = sizeof(CB_HS);
//	Check(device->CreateBuffer(&bufferDesc, NULL, &hsBuffer));
//
//
//	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
//	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
//	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//	bufferDesc.ByteWidth = sizeof(CB_DS);
//	Check(device->CreateBuffer(&bufferDesc, NULL, &dsBuffer));
//
//	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
//	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
//	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
//	bufferDesc.ByteWidth = sizeof(CB_PS);
//	Check(device->CreateBuffer(&bufferDesc, NULL, &psBuffer));
//
//	/////////////////////////////////////////////////////////////////////////
//
//	D3D11_RASTERIZER_DESC descRast = {
//	D3D11_FILL_SOLID,
//	D3D11_CULL_FRONT,
//	FALSE,
//	D3D11_DEFAULT_DEPTH_BIAS,
//	D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
//	D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
//	TRUE,
//	FALSE,
//	FALSE,
//	FALSE
//	};
//
//
//	descRast.CullMode = D3D11_CULL_BACK;
//	descRast.FrontCounterClockwise = true;
//	Check(device->CreateRasterizerState(&descRast, &CullBack));


}

Island11::~Island11()
{
	SafeRelease(heightmap_texture);
	SafeRelease(heightmap_textureSRV);

	SafeRelease(rock_bump_texture);
	SafeRelease(rock_bump_textureSRV);

	SafeRelease(rock_microbump_texture);
	SafeRelease(rock_microbump_textureSRV);

	SafeRelease(rock_diffuse_texture);
	SafeRelease(rock_diffuse_textureSRV);


	SafeRelease(sand_bump_texture);
	SafeRelease(sand_bump_textureSRV);

	SafeRelease(sand_microbump_texture);
	SafeRelease(sand_microbump_textureSRV);

	SafeRelease(sand_diffuse_texture);
	SafeRelease(sand_diffuse_textureSRV);

	SafeRelease(slope_diffuse_texture);
	SafeRelease(slope_diffuse_textureSRV);

	SafeRelease(grass_diffuse_texture);
	SafeRelease(grass_diffuse_textureSRV);

	SafeRelease(layerdef_texture);
	SafeRelease(layerdef_textureSRV);

	SafeRelease(water_bump_texture);
	SafeRelease(water_bump_textureSRV);

	SafeRelease(depthmap_texture);
	SafeRelease(depthmap_textureSRV);
/*
	SafeRelease(sky_texture);
	SafeRelease(sky_textureSRV);

*/

	
	SafeRelease(reflection_color_resource);
	SafeRelease(reflection_color_resourceSRV);
	SafeRelease(reflection_color_resourceRTV);
	SafeRelease(refraction_color_resource);
	SafeRelease(refraction_color_resourceSRV);
	SafeRelease(refraction_color_resourceRTV);

	SafeRelease(reflection_depth_resource);
	SafeRelease(reflection_depth_resourceDSV);
	SafeRelease(refraction_depth_resource);
	SafeRelease(refraction_depth_resourceRTV);
	SafeRelease(refraction_depth_resourceSRV);

	SafeRelease(shadowmap_resource);
	SafeRelease(shadowmap_resourceDSV);
	SafeRelease(shadowmap_resourceSRV);

	//SafeRelease(sky_vertexbuffer);
	//SafeRelease(trianglestrip_inputlayout);

	SafeRelease(heightfield_vertexbuffer);
	//SafeRelease(heightfield_inputlayout);

	SafeRelease(water_normalmap_resource);
	SafeRelease(water_normalmap_resourceSRV);
	SafeRelease(water_normalmap_resourceRTV);
}



void Island11::Update(ID3D11DeviceContext* context)
{
	
	TotalTime += Time::Delta();
	Vector3 SunPos;
	SunPos = -1 * 10000 * GlobalData::LightDirection();
	SunPos.y += 5500.f;
	if (SunPos.y < 4000.0f)
	{
		SunPos.y = 4000.0f;
	}
	
	updateDesc.LightPosition = SunPos;
	updateDesc.WaterBumpTexcoordShift = Vector2(TotalTime*1.5f, TotalTime*0.75f);
	updateDesc.ScreenSizeInv = Vector2(1.0f / (BackBufferWidth*main_buffer_size_multiplier), 1.0f / (BackBufferHeight*main_buffer_size_multiplier));
	updateDesc.RenderCaustics = RenderCaustics ? 1.0f : 0.0f;

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(updateBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &updateDesc, sizeof(updateDesc));
	context->Unmap(updateBuffer, 0);

	//terrainLod->Update();
	//sUpdateBuffer->SetConstantBuffer(updateBuffer);
	
}
void Island11::Reflection(ID3D11DeviceContext* context)
{
	context->RSSetViewports(1, &reflection_Viewport);
	context->OMSetRenderTargets(1, &reflection_color_resourceRTV, reflection_depth_resourceDSV);
	context->ClearRenderTargetView(reflection_color_resourceRTV, RefractionClearColor);
	context->ClearDepthStencilView(reflection_depth_resourceDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
	SetupReflectionView(context);
	
	sSkipCausticsCalculation->SetFloat(1.0f);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	stride = sizeof(float) * 4;
	context->IASetVertexBuffers(0, 1, &heightfield_vertexbuffer, &stride, &offset);
	shader->Draw(context,0, 2, terrain_numpatches_1d*terrain_numpatches_1d, 0);

}

void Island11::Caustics(ID3D11DeviceContext * context)
{
	if (RenderCaustics)
	{
		SetupNormalView(context);
		context->RSSetViewports(1, &water_normalmap_resource_viewport);
		context->OMSetRenderTargets(1, &water_normalmap_resourceRTV, NULL);
		context->ClearRenderTargetView(water_normalmap_resourceRTV, ClearColor);


		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		context->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
		shader->Draw(context, 0, 1, 4, 0);
	}
}


void Island11::Terrain(ID3D11DeviceContext* context)
{
	
	//terrainLod->Render(context);
	SetupNormalView(context);
	sUpdateBuffer->SetConstantBuffer(updateBuffer);
	
	
	
	sWaterNormalMapTexture->SetResource(water_normalmap_resourceSRV);
	sTerrainBeingRendered->SetFloat(1.0f);
	sSkipCausticsCalculation->SetFloat(0.0f);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	stride = sizeof(float) * 4;
	context->IASetVertexBuffers(0, 1, &heightfield_vertexbuffer, &stride, &offset);
	shader->Draw(context,0,4, terrain_numpatches_1d*terrain_numpatches_1d, 0);
}

void Island11::Refraction(ID3D11DeviceContext* context,ID3D11ShaderResourceView* srv, ID3D11Texture2D* texture)
{
	
	
	context->ResolveSubresource(refraction_color_resource, 0, texture, 0, DXGI_FORMAT_R10G10B10A2_UNORM);

    context->RSSetViewports(1, &main_Viewport);
	context->OMSetRenderTargets(1, &refraction_depth_resourceRTV, nullptr);
	context->ClearRenderTargetView(refraction_depth_resourceRTV, Color(0, 0, 0, 0));


	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	sRefractionDepthTextureMS1->SetResource(srv);
	context->IASetVertexBuffers(0, 0, NULL, NULL, NULL);

	shader->Draw(context,0, 0, 4, 0);
}

void Island11::Water(ID3D11DeviceContext* context)
{
	//SetupNormalView(context);
	
	SetupNormalView(context);
	sUpdateBuffer->SetConstantBuffer(updateBuffer);
	// drawing water surface to main buffer
	//shader->AsSRV("DepthTexture")->SetResource(shadowmap_resourceSRV
	
	sReflectionTexture->SetResource(reflection_color_resourceSRV);
	sRefractionTexture->SetResource(refraction_color_resourceSRV);
	sRefractionDepthTextureResolved->SetResource(refraction_depth_resourceSRV);
	sWaterNormalMapTexture->SetResource(water_normalmap_resourceSRV);
	sTerrainBeingRendered->SetFloat(0.0f);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	stride = sizeof(float) * 4;
	context->IASetVertexBuffers(0, 1, &heightfield_vertexbuffer, &stride, &offset);
	shader->Draw(context,0, 3, terrain_numpatches_1d*terrain_numpatches_1d, 0);

}



void Island11::SetupNormalView(ID3D11DeviceContext* context)
{


	view = GlobalData::GetView();
	proj = GlobalData::GetProj();
	viewDesc.View = view;
	viewDesc.VP = view * proj;
//	D3DXMatrixTranspose(&viewDesc.View, &view);
	//D3DXMatrixTranspose(&viewDesc.VP, &(view * proj));
	viewDesc.camPos = GlobalData::Position();

	direction = GlobalData::Forward();
	D3DXVec3Normalize(&viewDesc.camDirection, &direction);

	viewDesc.halfSpaceCullSign = cullSign;

	viewDesc.halfSpaceCullPosition = terrain_minheight * halfSpaceCullPosition;

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(viewBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &viewDesc, sizeof(viewDesc));
	context->Unmap(viewBuffer, 0);
	
	sViewBuffer->SetConstantBuffer(viewBuffer);




}

void Island11::SetupReflectionView(ID3D11DeviceContext* context)
{


	position = GlobalData::Position();
	EyePoint = GlobalData::Position();

	LookAtPoint = GlobalData::LookAt();
	EyePoint.y = -1.0f*EyePoint.y + 1.0f;
	LookAtPoint.y = -1.0f*LookAtPoint.y + 1.0f;

	view = GlobalData::GetView();
	view._41 = 0; view._42 = 0; view._43 = 0;
	D3DXMatrixTranspose(&view, &view);
	view._41 = position.x;
	view._42 = position.y;

	view._42 = -view._42 - 1.0f;

	view._43 = position.z;

	view._21 *= -1.0f;
	view._23 *= -1.0f;
	view._32 *= -1.0f;
	D3DXMatrixInverse(&rflectionMatrix, nullptr, &view);

	viewDesc.VP = rflectionMatrix * proj;
	
	viewDesc.camPos = EyePoint;
	direction = LookAtPoint - EyePoint;
	D3DXVec3Normalize(&viewDesc.camDirection, &direction);
	viewDesc.halfSpaceCullSign = 1.0f;
	viewDesc.halfSpaceCullPosition = -0.6f;
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(viewBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &viewDesc, sizeof(viewDesc));
	context->Unmap(viewBuffer, 0);
	sViewBuffer->SetConstantBuffer(viewBuffer);

	Matrix VP;
	D3DXMatrixTranspose(&VP, &viewDesc.VP);
	ZeroMemory(&MappedResource, sizeof(MappedResource));
	context->Map(mainViewBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, VP, sizeof(Matrix));
	context->Unmap(mainViewBuffer, 0);
	context->VSSetConstantBuffers(1, 1, &mainViewBuffer);
}

void Island11::SetupRefractionView()
{
	shader->AsScalar("HalfSpaceCullSign")->SetFloat(-1.0f);
	shader->AsScalar("HalfSpaceCullPosition")->SetFloat(terrain_minheight);

}



void Island11::CreateTerrain(QuadTree* tree)
{
	auto& height = tree->heightMap;
	int i, j, k, l;
	float x, z;
	int ix, iz;
	float * backterrain;
	D3DXVECTOR3 vec1, vec2, vec3;
	int currentstep = terrain_gridpoints;
	float mv, rm;
	float offset = 0, yscale = 0, maxheight = 0, minheight = 0;

	float *height_linear_array;
	float *patches_rawdata;
	
	D3D11_SUBRESOURCE_DATA subresource_data;
	D3D11_TEXTURE2D_DESC tex_desc;
	D3D11_SHADER_RESOURCE_VIEW_DESC textureSRV_desc;

	backterrain = (float *)malloc((terrain_gridpoints + 1)*(terrain_gridpoints + 1) * sizeof(float));
	rm = terrain_fractalinitialvalue;
	backterrain[0] = 0;
	backterrain[0 + terrain_gridpoints * terrain_gridpoints] = 0;
	backterrain[terrain_gridpoints] = 0;
	backterrain[terrain_gridpoints + terrain_gridpoints * terrain_gridpoints] = 0;
	currentstep = terrain_gridpoints;
	srand(12);
	// generating fractal terrain using square-diamond method
	while (currentstep > 1)
	{
		//square step;
		i = 0;
		j = 0;


		while (i < terrain_gridpoints)
		{
			j = 0;
			while (j < terrain_gridpoints)
			{

				mv = backterrain[i + terrain_gridpoints * j];
				mv += backterrain[(i + currentstep) + terrain_gridpoints * j];
				mv += backterrain[(i + currentstep) + terrain_gridpoints * (j + currentstep)];
				mv += backterrain[i + terrain_gridpoints * (j + currentstep)];
				mv /= 4.0;
				backterrain[i + currentstep / 2 + terrain_gridpoints * (j + currentstep / 2)] = (float)(mv + rm * ((rand() % 1000) / 1000.0f - 0.5f));
				j += currentstep;
			}
			i += currentstep;
		}

		//diamond step;
		i = 0;
		j = 0;

		while (i < terrain_gridpoints)
		{
			j = 0;
			while (j < terrain_gridpoints)
			{

				mv = 0;
				mv = backterrain[i + terrain_gridpoints * j];
				mv += backterrain[(i + currentstep) + terrain_gridpoints * j];
				mv += backterrain[(i + currentstep / 2) + terrain_gridpoints * (j + currentstep / 2)];
				mv += backterrain[i + currentstep / 2 + terrain_gridpoints * gp_wrap(j - currentstep / 2)];
				mv /= 4;
				backterrain[i + currentstep / 2 + terrain_gridpoints * j] = (float)(mv + rm * ((rand() % 1000) / 1000.0f - 0.5f));

				mv = 0;
				mv = backterrain[i + terrain_gridpoints * j];
				mv += backterrain[i + terrain_gridpoints * (j + currentstep)];
				mv += backterrain[(i + currentstep / 2) + terrain_gridpoints * (j + currentstep / 2)];
				mv += backterrain[gp_wrap(i - currentstep / 2) + terrain_gridpoints * (j + currentstep / 2)];
				mv /= 4;
				backterrain[i + terrain_gridpoints * (j + currentstep / 2)] = (float)(mv + rm * ((rand() % 1000) / 1000.0f - 0.5f));

				mv = 0;
				mv = backterrain[i + currentstep + terrain_gridpoints * j];
				mv += backterrain[i + currentstep + terrain_gridpoints * (j + currentstep)];
				mv += backterrain[(i + currentstep / 2) + terrain_gridpoints * (j + currentstep / 2)];
				mv += backterrain[gp_wrap(i + currentstep / 2 + currentstep) + terrain_gridpoints * (j + currentstep / 2)];
				mv /= 4;
				backterrain[i + currentstep + terrain_gridpoints * (j + currentstep / 2)] = (float)(mv + rm * ((rand() % 1000) / 1000.0f - 0.5f));

				mv = 0;
				mv = backterrain[i + currentstep + terrain_gridpoints * (j + currentstep)];
				mv += backterrain[i + terrain_gridpoints * (j + currentstep)];
				mv += backterrain[(i + currentstep / 2) + terrain_gridpoints * (j + currentstep / 2)];
				mv += backterrain[i + currentstep / 2 + terrain_gridpoints * gp_wrap(j + currentstep / 2 + currentstep)];
				mv /= 4;
				backterrain[i + currentstep / 2 + terrain_gridpoints * (j + currentstep)] = (float)(mv + rm * ((rand() % 1000) / 1000.0f - 0.5f));
				j += currentstep;
			}
			i += currentstep;
		}
		//changing current step;
		currentstep /= 2;
		rm *= terrain_fractalfactor;
	}

	// scaling to minheight..maxheight range
	for (i = 0; i < terrain_gridpoints + 1; i++)
		for (j = 0; j < terrain_gridpoints + 1; j++)
		{
			height[i][j] = backterrain[i + terrain_gridpoints * j];
		}
	maxheight = height[0][0];
	minheight = height[0][0];
	for (i = 0; i < terrain_gridpoints + 1; i++)
		for (j = 0; j < terrain_gridpoints + 1; j++)
		{
			if (height[i][j] > maxheight) maxheight = height[i][j];
			if (height[i][j] < minheight) minheight = height[i][j];
		}
	offset = minheight - terrain_minheight;
	yscale = (terrain_maxheight - terrain_minheight) / (maxheight - minheight);

	for (i = 0; i < terrain_gridpoints + 1; i++)
		for (j = 0; j < terrain_gridpoints + 1; j++)
		{
			height[i][j] -= minheight;
			height[i][j] *= yscale;
			height[i][j] += terrain_minheight;
		}

	// moving down edges of heightmap	
	for (i = 0; i < terrain_gridpoints + 1; i++)
		for (j = 0; j < terrain_gridpoints + 1; j++)
		{
			mv = (float)((i - terrain_gridpoints / 2.0f)*(i - terrain_gridpoints / 2.0f) + (j - terrain_gridpoints / 2.0f)*(j - terrain_gridpoints / 2.0f));
			rm = (float)((terrain_gridpoints*0.8f)*(terrain_gridpoints*0.8f) / 4.0f);
			if (mv > rm)
			{
				height[i][j] -= ((mv - rm) / 1000.0f)*terrain_geometry_scale;
			}
			if (height[i][j] < terrain_minheight)
			{
				height[i][j] = terrain_minheight;
			}
		}


	// terrain banks
	for (k = 0; k < 10; k++)
	{
		for (i = 0; i < terrain_gridpoints + 1; i++)
			for (j = 0; j < terrain_gridpoints + 1; j++)
			{
				mv = height[i][j];
				if ((mv) > 0.02f)
				{
					mv -= 0.02f;
				}
				if (mv < -0.02f)
				{
					mv += 0.02f;
				}
				height[i][j] = mv;
			}
	}

	// smoothing 
	for (k = 0; k < terrain_smoothsteps; k++)
	{
		for (i = 0; i < terrain_gridpoints + 1; i++)
			for (j = 0; j < terrain_gridpoints + 1; j++)
			{

				vec1.x = 2 * terrain_geometry_scale;
				vec1.y = terrain_geometry_scale * (height[gp_wrap(i + 1)][j] - height[gp_wrap(i - 1)][j]);
				vec1.z = 0;
				vec2.x = 0;
				vec2.y = -terrain_geometry_scale * (height[i][gp_wrap(j + 1)] - height[i][gp_wrap(j - 1)]);
				vec2.z = -2 * terrain_geometry_scale;

				D3DXVec3Cross(&vec3, &vec1, &vec2);
				D3DXVec3Normalize(&vec3, &vec3);


				if (((vec3.y > terrain_rockfactor) || (height[i][j] < 1.2f)))
				{
					rm = terrain_smoothfactor1;
					mv = height[i][j] * (1.0f - rm) + rm * 0.25f*(height[gp_wrap(i - 1)][j] + height[i][gp_wrap(j - 1)] + height[gp_wrap(i + 1)][j] + height[i][gp_wrap(j + 1)]);
					backterrain[i + terrain_gridpoints * j] = mv;
				}
				else
				{
					rm = terrain_smoothfactor2;
					mv = height[i][j] * (1.0f - rm) + rm * 0.25f*(height[gp_wrap(i - 1)][j] + height[i][gp_wrap(j - 1)] + height[gp_wrap(i + 1)][j] + height[i][gp_wrap(j + 1)]);
					backterrain[i + terrain_gridpoints * j] = mv;
				}

			}
		for (i = 0; i < terrain_gridpoints + 1; i++)
			for (j = 0; j < terrain_gridpoints + 1; j++)
			{
				height[i][j] = (backterrain[i + terrain_gridpoints * j]);
			}
	}
	for (i = 0; i < terrain_gridpoints + 1; i++)
		for (j = 0; j < terrain_gridpoints + 1; j++)
		{
			rm = 0.5f;
			mv = height[i][j] * (1.0f - rm) + rm * 0.25f*(height[gp_wrap(i - 1)][j] + height[i][gp_wrap(j - 1)] + height[gp_wrap(i + 1)][j] + height[i][gp_wrap(j + 1)]);
			backterrain[i + terrain_gridpoints * j] = mv;
		}
	for (i = 0; i < terrain_gridpoints + 1; i++)
		for (j = 0; j < terrain_gridpoints + 1; j++)
		{
			height[i][j] = (backterrain[i + terrain_gridpoints * j]);
		}


	free(backterrain);

	//calculating normals
	for (i = 0; i < terrain_gridpoints + 1; i++)
		for (j = 0; j < terrain_gridpoints + 1; j++)
		{
			vec1.x = 2 * terrain_geometry_scale;
			vec1.y = terrain_geometry_scale * (height[gp_wrap(i + 1)][j] - height[gp_wrap(i - 1)][j]);
			vec1.z = 0;
			vec2.x = 0;
			vec2.y = -terrain_geometry_scale * (height[i][gp_wrap(j + 1)] - height[i][gp_wrap(j - 1)]);
			vec2.z = -2 * terrain_geometry_scale;
			D3DXVec3Cross(&normal[i][j], &vec1, &vec2);
			D3DXVec3Normalize(&normal[i][j], &normal[i][j]);
		}


	// buiding layerdef 
	byte* temp_layerdef_map_texture_pixels = (byte *)malloc(terrain_layerdef_map_texture_size*terrain_layerdef_map_texture_size * 4);
	byte* layerdef_map_texture_pixels = (byte *)malloc(terrain_layerdef_map_texture_size*terrain_layerdef_map_texture_size * 4);
	for (i = 0; i < terrain_layerdef_map_texture_size; i++)
		for (j = 0; j < terrain_layerdef_map_texture_size; j++)
		{
			x = (float)(terrain_gridpoints)*((float)i / (float)terrain_layerdef_map_texture_size);
			z = (float)(terrain_gridpoints)*((float)j / (float)terrain_layerdef_map_texture_size);
			ix = (int)floor(x);
			iz = (int)floor(z);
			rm = bilinear_interpolation(x - ix, z - iz, height[ix][iz], height[ix + 1][iz], height[ix + 1][iz + 1], height[ix][iz + 1])*terrain_geometry_scale;

			temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 0] = 0;
			temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 1] = 0;
			temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 2] = 0;
			temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 3] = 0;

			if ((rm > terrain_height_underwater_start) && (rm <= terrain_height_underwater_end))
			{
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 0] = 255;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 1] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 2] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 3] = 0;
			}

			if ((rm > terrain_height_sand_start) && (rm <= terrain_height_sand_end))
			{
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 0] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 1] = 255;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 2] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 3] = 0;
			}

			if ((rm > terrain_height_grass_start) && (rm <= terrain_height_grass_end))
			{
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 0] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 1] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 2] = 255;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 3] = 0;
			}

			mv = bilinear_interpolation(x - ix, z - iz, normal[ix][iz].y, normal[ix + 1][iz].y, normal[ix + 1][iz + 1].y, normal[ix][iz + 1].y);

			if ((mv < terrain_slope_grass_start) && (rm > terrain_height_sand_end))
			{
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 0] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 1] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 2] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 3] = 0;
			}

			if ((mv < terrain_slope_rocks_start) && (rm > terrain_height_rocks_start))
			{
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 0] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 1] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 2] = 0;
				temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 3] = 255;
			}

		}
	for (i = 0; i < terrain_layerdef_map_texture_size; i++)
		for (j = 0; j < terrain_layerdef_map_texture_size; j++)
		{
			layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 0] = temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 0];
			layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 1] = temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 1];
			layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 2] = temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 2];
			layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 3] = temp_layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 3];
		}


	for (i = 2; i < terrain_layerdef_map_texture_size - 2; i++)
		for (j = 2; j < terrain_layerdef_map_texture_size - 2; j++)
		{
			int n1 = 0;
			int n2 = 0;
			int n3 = 0;
			int n4 = 0;
			for (k = -2; k < 3; k++)
				for (l = -2; l < 3; l++)
				{
					n1 += temp_layerdef_map_texture_pixels[((j + k)*terrain_layerdef_map_texture_size + i + l) * 4 + 0];
					n2 += temp_layerdef_map_texture_pixels[((j + k)*terrain_layerdef_map_texture_size + i + l) * 4 + 1];
					n3 += temp_layerdef_map_texture_pixels[((j + k)*terrain_layerdef_map_texture_size + i + l) * 4 + 2];
					n4 += temp_layerdef_map_texture_pixels[((j + k)*terrain_layerdef_map_texture_size + i + l) * 4 + 3];
				}
			layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 0] = (byte)(n1 / 25);
			layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 1] = (byte)(n2 / 25);
			layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 2] = (byte)(n3 / 25);
			layerdef_map_texture_pixels[(j*terrain_layerdef_map_texture_size + i) * 4 + 3] = (byte)(n4 / 25);
		}

	// putting the generated data to textures

	subresource_data.pSysMem = layerdef_map_texture_pixels;
	subresource_data.SysMemPitch = terrain_layerdef_map_texture_size * 4;
	subresource_data.SysMemSlicePitch = 0;

	tex_desc.Width = terrain_layerdef_map_texture_size;
	tex_desc.Height = terrain_layerdef_map_texture_size;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = 0;
	Check(device->CreateTexture2D(&tex_desc, &subresource_data, &layerdef_texture));

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = tex_desc.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	textureSRV_desc.Texture2D.MostDetailedMip = 0;
	Check(device->CreateShaderResourceView(layerdef_texture, &textureSRV_desc, &layerdef_textureSRV));

	free(temp_layerdef_map_texture_pixels);
	free(layerdef_map_texture_pixels);

	height_linear_array = new float[terrain_gridpoints*terrain_gridpoints * 4];
	patches_rawdata = new float[terrain_numpatches_1d*terrain_numpatches_1d * 4];

	for (int i = 0; i < terrain_gridpoints; i++)
		for (int j = 0; j < terrain_gridpoints; j++)
		{
			height_linear_array[(i + j * terrain_gridpoints) * 4 + 0] = normal[i][j].x;
			height_linear_array[(i + j * terrain_gridpoints) * 4 + 1] = normal[i][j].y;
			height_linear_array[(i + j * terrain_gridpoints) * 4 + 2] = normal[i][j].z;
			height_linear_array[(i + j * terrain_gridpoints) * 4 + 3] = height[i][j];
		}
	subresource_data.pSysMem = height_linear_array;
	subresource_data.SysMemPitch = terrain_gridpoints * 4 * sizeof(float);
	subresource_data.SysMemSlicePitch = 0;

	tex_desc.Width = terrain_gridpoints;
	tex_desc.Height = terrain_gridpoints;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = 0;
	Check(device->CreateTexture2D(&tex_desc, &subresource_data, &heightmap_texture));

	SafeDeleteArray(height_linear_array);

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = tex_desc.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	Check(device->CreateShaderResourceView(heightmap_texture, &textureSRV_desc, &heightmap_textureSRV));


	//terrainLod = new TerrainLod(device, heightmap_textureSRV);
	//building depthmap
	byte * depth_shadow_map_texture_pixels = (byte *)malloc(terrain_depth_shadow_map_texture_size*terrain_depth_shadow_map_texture_size * 4);
	for (i = 0; i < terrain_depth_shadow_map_texture_size; i++)
		for (j = 0; j < terrain_depth_shadow_map_texture_size; j++)
		{
			x = (float)(terrain_gridpoints)*((float)i / (float)terrain_depth_shadow_map_texture_size);
			z = (float)(terrain_gridpoints)*((float)j / (float)terrain_depth_shadow_map_texture_size);
			ix = (int)floor(x);
			iz = (int)floor(z);
			rm = bilinear_interpolation(x - ix, z - iz, height[ix][iz], height[ix + 1][iz], height[ix + 1][iz + 1], height[ix][iz + 1])*terrain_geometry_scale;

			if (rm > 0)
			{
				depth_shadow_map_texture_pixels[(j*terrain_depth_shadow_map_texture_size + i) * 4 + 0] = 0;
				depth_shadow_map_texture_pixels[(j*terrain_depth_shadow_map_texture_size + i) * 4 + 1] = 0;
				depth_shadow_map_texture_pixels[(j*terrain_depth_shadow_map_texture_size + i) * 4 + 2] = 0;
			}
			else
			{
				float no = (1.0f*255.0f*(rm / (terrain_minheight*terrain_geometry_scale))) - 1.0f;
				if (no > 255) no = 255;
				if (no < 0) no = 0;
				depth_shadow_map_texture_pixels[(j*terrain_depth_shadow_map_texture_size + i) * 4 + 0] = (byte)no;

				no = (10.0f*255.0f*(rm / (terrain_minheight*terrain_geometry_scale))) - 40.0f;
				if (no > 255) no = 255;
				if (no < 0) no = 0;
				depth_shadow_map_texture_pixels[(j*terrain_depth_shadow_map_texture_size + i) * 4 + 1] = (byte)no;

				no = (100.0f*255.0f*(rm / (terrain_minheight*terrain_geometry_scale))) - 300.0f;
				if (no > 255) no = 255;
				if (no < 0) no = 0;
				depth_shadow_map_texture_pixels[(j*terrain_depth_shadow_map_texture_size + i) * 4 + 2] = (byte)no;
			}
			depth_shadow_map_texture_pixels[(j*terrain_depth_shadow_map_texture_size + i) * 4 + 3] = 0;
		}

	subresource_data.pSysMem = depth_shadow_map_texture_pixels;
	subresource_data.SysMemPitch = terrain_depth_shadow_map_texture_size * 4;
	subresource_data.SysMemSlicePitch = 0;

	tex_desc.Width = terrain_depth_shadow_map_texture_size;
	tex_desc.Height = terrain_depth_shadow_map_texture_size;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = 0;
	Check(device->CreateTexture2D(&tex_desc, &subresource_data, &depthmap_texture));

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = tex_desc.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	Check(device->CreateShaderResourceView(depthmap_texture, &textureSRV_desc, &depthmap_textureSRV));

	free(depth_shadow_map_texture_pixels);

	// creating terrain vertex buffer
	for (int i = 0; i < terrain_numpatches_1d; i++)
		for (int j = 0; j < terrain_numpatches_1d; j++)
		{
			patches_rawdata[(i + j * terrain_numpatches_1d) * 4 + 0] = i * terrain_geometry_scale*terrain_gridpoints / terrain_numpatches_1d;
			patches_rawdata[(i + j * terrain_numpatches_1d) * 4 + 1] = j * terrain_geometry_scale*terrain_gridpoints / terrain_numpatches_1d;
			patches_rawdata[(i + j * terrain_numpatches_1d) * 4 + 2] = terrain_geometry_scale * terrain_gridpoints / terrain_numpatches_1d;
			patches_rawdata[(i + j * terrain_numpatches_1d) * 4 + 3] = terrain_geometry_scale * terrain_gridpoints / terrain_numpatches_1d;
		}

	D3D11_BUFFER_DESC buf_desc;
	memset(&buf_desc, 0, sizeof(buf_desc));

	buf_desc.ByteWidth = terrain_numpatches_1d * terrain_numpatches_1d * 4 * sizeof(float);
	buf_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buf_desc.Usage = D3D11_USAGE_DEFAULT;

	subresource_data.pSysMem = patches_rawdata;
	subresource_data.SysMemPitch = 0;
	subresource_data.SysMemSlicePitch = 0;

	Check(device->CreateBuffer(&buf_desc, &subresource_data, &heightfield_vertexbuffer));
	SafeDeleteArray(patches_rawdata);
}

void Island11::ReCreateBuffers()
{
	D3D11_TEXTURE2D_DESC tex_desc;
	D3D11_SHADER_RESOURCE_VIEW_DESC textureSRV_desc;
	D3D11_DEPTH_STENCIL_VIEW_DESC DSV_desc;




	SafeRelease(reflection_color_resource);
	SafeRelease(reflection_color_resourceSRV);
	SafeRelease(reflection_color_resourceRTV);
	SafeRelease(refraction_color_resource);
	SafeRelease(refraction_color_resourceSRV);
	SafeRelease(refraction_color_resourceRTV);

	SafeRelease(reflection_depth_resource);
	SafeRelease(reflection_depth_resourceDSV);
	SafeRelease(refraction_depth_resource);
	SafeRelease(refraction_depth_resourceSRV);
	SafeRelease(refraction_depth_resourceRTV);

	SafeRelease(shadowmap_resource);
	SafeRelease(shadowmap_resourceDSV);
	SafeRelease(shadowmap_resourceSRV);

	SafeRelease(water_normalmap_resource);
	SafeRelease(water_normalmap_resourceSRV);
	SafeRelease(water_normalmap_resourceRTV);



	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	ZeroMemory(&tex_desc, sizeof(tex_desc));

	tex_desc.Width = (UINT)(BackBufferWidth*reflection_buffer_size_multiplier);
	tex_desc.Height = (UINT)(BackBufferHeight*reflection_buffer_size_multiplier);
	tex_desc.MipLevels = (UINT)max(1, log(max((float)tex_desc.Width, (float)tex_desc.Height)) / (float)log(2.0f));
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	textureSRV_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	textureSRV_desc.Texture2D.MostDetailedMip = 0;

	Check(device->CreateTexture2D(&tex_desc, NULL, &reflection_color_resource));
	Check(device->CreateShaderResourceView(reflection_color_resource, &textureSRV_desc, &reflection_color_resourceSRV));
	Check(device->CreateRenderTargetView(reflection_color_resource, NULL, &reflection_color_resourceRTV));


	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	ZeroMemory(&tex_desc, sizeof(tex_desc));

	tex_desc.Width = (UINT)(BackBufferWidth*refraction_buffer_size_multiplier);
	tex_desc.Height = (UINT)(BackBufferHeight*refraction_buffer_size_multiplier);
	tex_desc.MipLevels = (UINT)max(1, log(max((float)tex_desc.Width, (float)tex_desc.Height)) / (float)log(2.0f));
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	textureSRV_desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	textureSRV_desc.Texture2D.MostDetailedMip = 0;

	Check(device->CreateTexture2D(&tex_desc, NULL, &refraction_color_resource));
	Check(device->CreateShaderResourceView(refraction_color_resource, &textureSRV_desc, &refraction_color_resourceSRV));
	Check(device->CreateRenderTargetView(refraction_color_resource, NULL, &refraction_color_resourceRTV));

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	ZeroMemory(&tex_desc, sizeof(tex_desc));

	// recreating reflection and refraction depth buffers

	tex_desc.Width = (UINT)(BackBufferWidth*reflection_buffer_size_multiplier);
	tex_desc.Height = (UINT)(BackBufferHeight*reflection_buffer_size_multiplier);
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = 0;

	DSV_desc.Format = DXGI_FORMAT_D32_FLOAT;
	DSV_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DSV_desc.Flags = 0;
	DSV_desc.Texture2D.MipSlice = 0;

	Check(device->CreateTexture2D(&tex_desc, NULL, &reflection_depth_resource));
	Check(device->CreateDepthStencilView(reflection_depth_resource, &DSV_desc, &reflection_depth_resourceDSV));


	tex_desc.Width = (UINT)(BackBufferWidth*refraction_buffer_size_multiplier);
	tex_desc.Height = (UINT)(BackBufferHeight*refraction_buffer_size_multiplier);
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R32_FLOAT;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = 0;

	textureSRV_desc.Format = DXGI_FORMAT_R32_FLOAT;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = 1;
	textureSRV_desc.Texture2D.MostDetailedMip = 0;

	Check(device->CreateTexture2D(&tex_desc, NULL, &refraction_depth_resource));
	Check(device->CreateRenderTargetView(refraction_depth_resource, NULL, &refraction_depth_resourceRTV));
	Check(device->CreateShaderResourceView(refraction_depth_resource, &textureSRV_desc, &refraction_depth_resourceSRV));

	//// recreating shadowmap resource
	//tex_desc.Width = shadowmap_resource_buffer_size_xy;
	//tex_desc.Height = shadowmap_resource_buffer_size_xy;
	//tex_desc.MipLevels = 1;
	//tex_desc.ArraySize = 1;
	//tex_desc.Format = DXGI_FORMAT_R32_TYPELESS;
	//tex_desc.SampleDesc.Count = 1;
	//tex_desc.SampleDesc.Quality = 0;
	//tex_desc.Usage = D3D11_USAGE_DEFAULT;
	//tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	//tex_desc.CPUAccessFlags = 0;
	//tex_desc.MiscFlags = 0;

	//DSV_desc.Format = DXGI_FORMAT_D32_FLOAT;
	//DSV_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	//DSV_desc.Flags = 0;
	//DSV_desc.Texture2D.MipSlice = 0;

	//textureSRV_desc.Format = DXGI_FORMAT_R32_FLOAT;
	//textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//textureSRV_desc.Texture2D.MipLevels = 1;
	//textureSRV_desc.Texture2D.MostDetailedMip = 0;

	//Check(device->CreateTexture2D(&tex_desc, NULL, &shadowmap_resource));
	//Check(device->CreateShaderResourceView(shadowmap_resource, &textureSRV_desc, &shadowmap_resourceSRV));
	//Check(device->CreateDepthStencilView(shadowmap_resource, &DSV_desc, &shadowmap_resourceDSV));

	// recreating water normalmap buffer

	tex_desc.Width = water_normalmap_resource_buffer_size_xy;
	tex_desc.Height = water_normalmap_resource_buffer_size_xy;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = 0;

	textureSRV_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	textureSRV_desc.Texture2D.MostDetailedMip = 0;

	Check(device->CreateTexture2D(&tex_desc, NULL, &water_normalmap_resource));
	Check(device->CreateShaderResourceView(water_normalmap_resource, &textureSRV_desc, &water_normalmap_resourceSRV));
	Check(device->CreateRenderTargetView(water_normalmap_resource, NULL, &water_normalmap_resourceRTV));



}

void Island11::LoadTextures()
{

	D3D11_SHADER_RESOURCE_VIEW_DESC textureSRV_desc;


	// Load images
	D3DX11_IMAGE_LOAD_INFO imageLoadInfo;
	D3DX11_IMAGE_INFO imageInfo;
	wstring path = L"../../_Textures/TerrainTextures/rock_bump6.dds";

	//DXUTFindDXSDKMediaFileCch(path.c_str(), MAX_PATH, L"TerrainTextures/rock_bump6.dds");
	D3DX11GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL);
	memset(&imageLoadInfo, 0, sizeof(imageLoadInfo));
	imageLoadInfo.Width = imageInfo.Width;
	imageLoadInfo.Height = imageInfo.Height;
	imageLoadInfo.MipLevels = imageInfo.MipLevels;
	imageLoadInfo.Format = imageInfo.Format;
	imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3DX11CreateTextureFromFile(device, path.c_str(), &imageLoadInfo, NULL, (ID3D11Resource**)&rock_bump_texture, NULL);

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = imageLoadInfo.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = imageLoadInfo.MipLevels;
	device->CreateShaderResourceView(rock_bump_texture, &textureSRV_desc, &rock_bump_textureSRV);

	path = L"../../_Textures/TerrainTextures/terrain_rock4.dds";
	//DXUTFindDXSDKMediaFileCch(path.c_str(), MAX_PATH, L"TerrainTextures/terrain_rock4.dds");
	D3DX11GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL);
	memset(&imageLoadInfo, 0, sizeof(imageLoadInfo));
	imageLoadInfo.Width = imageInfo.Width;
	imageLoadInfo.Height = imageInfo.Height;
	imageLoadInfo.MipLevels = imageInfo.MipLevels;
	imageLoadInfo.Format = imageInfo.Format;
	imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3DX11CreateTextureFromFile(device, path.c_str(), &imageLoadInfo, NULL, (ID3D11Resource**)&rock_diffuse_texture, NULL);

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = imageLoadInfo.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = imageLoadInfo.MipLevels;
	device->CreateShaderResourceView(rock_diffuse_texture, &textureSRV_desc, &rock_diffuse_textureSRV);

	path = L"../../_Textures/TerrainTextures/sand_diffuse.dds";
	//DXUTFindDXSDKMediaFileCch(path.c_str(), MAX_PATH, L"TerrainTextures/sand_diffuse.dds");
	D3DX11GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL);
	memset(&imageLoadInfo, 0, sizeof(imageLoadInfo));
	imageLoadInfo.Width = imageInfo.Width;
	imageLoadInfo.Height = imageInfo.Height;
	imageLoadInfo.MipLevels = imageInfo.MipLevels;
	imageLoadInfo.Format = imageInfo.Format;
	imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3DX11CreateTextureFromFile(device, path.c_str(), &imageLoadInfo, NULL, (ID3D11Resource**)&sand_diffuse_texture, NULL);

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = imageLoadInfo.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = imageLoadInfo.MipLevels;
	device->CreateShaderResourceView(sand_diffuse_texture, &textureSRV_desc, &sand_diffuse_textureSRV);

	path = L"../../_Textures/TerrainTextures/rock_bump4.dds";
	//DXUTFindDXSDKMediaFileCch(path.c_str(), MAX_PATH, L"TerrainTextures/rock_bump4.dds");
	D3DX11GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL);
	memset(&imageLoadInfo, 0, sizeof(imageLoadInfo));
	imageLoadInfo.Width = imageInfo.Width;
	imageLoadInfo.Height = imageInfo.Height;
	imageLoadInfo.MipLevels = imageInfo.MipLevels;
	imageLoadInfo.Format = imageInfo.Format;
	imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3DX11CreateTextureFromFile(device, path.c_str(), &imageLoadInfo, NULL, (ID3D11Resource**)&sand_bump_texture, NULL);

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = imageLoadInfo.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = imageLoadInfo.MipLevels;
	device->CreateShaderResourceView(sand_bump_texture, &textureSRV_desc, &sand_bump_textureSRV);

	path = L"../../_Textures/TerrainTextures/terrain_grass.dds";
	//DXUTFindDXSDKMediaFileCch(path.c_str(), MAX_PATH, L"TerrainTextures/terrain_grass.dds");
	D3DX11GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL);
	memset(&imageLoadInfo, 0, sizeof(imageLoadInfo));
	imageLoadInfo.Width = imageInfo.Width;
	imageLoadInfo.Height = imageInfo.Height;
	imageLoadInfo.MipLevels = imageInfo.MipLevels;
	imageLoadInfo.Format = imageInfo.Format;
	imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3DX11CreateTextureFromFile(device, path.c_str(), &imageLoadInfo, NULL, (ID3D11Resource**)&grass_diffuse_texture, NULL);

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = imageLoadInfo.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = imageLoadInfo.MipLevels;
	device->CreateShaderResourceView(grass_diffuse_texture, &textureSRV_desc, &grass_diffuse_textureSRV);


	path = L"../../_Textures/TerrainTextures/terrain_slope.dds";
	//DXUTFindDXSDKMediaFileCch(path.c_str(), MAX_PATH, L"TerrainTextures/terrain_slope.dds");
	D3DX11GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL);
	memset(&imageLoadInfo, 0, sizeof(imageLoadInfo));
	imageLoadInfo.Width = imageInfo.Width;
	imageLoadInfo.Height = imageInfo.Height;
	imageLoadInfo.MipLevels = imageInfo.MipLevels;
	imageLoadInfo.Format = imageInfo.Format;
	imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3DX11CreateTextureFromFile(device, path.c_str(), &imageLoadInfo, NULL, (ID3D11Resource**)&slope_diffuse_texture, NULL);

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = imageLoadInfo.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = imageLoadInfo.MipLevels;
	device->CreateShaderResourceView(slope_diffuse_texture, &textureSRV_desc, &slope_diffuse_textureSRV);

	path = L"../../_Textures/TerrainTextures/lichen1_normal.dds";
	//DXUTFindDXSDKMediaFileCch(path.c_str(), MAX_PATH, L"TerrainTextures/lichen1_normal.dds");
	D3DX11GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL);
	memset(&imageLoadInfo, 0, sizeof(imageLoadInfo));
	imageLoadInfo.Width = imageInfo.Width;
	imageLoadInfo.Height = imageInfo.Height;
	imageLoadInfo.MipLevels = imageInfo.MipLevels;
	imageLoadInfo.Format = imageInfo.Format;
	imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3DX11CreateTextureFromFile(device, path.c_str(), &imageLoadInfo, NULL, (ID3D11Resource**)&sand_microbump_texture, NULL);

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = imageLoadInfo.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = imageLoadInfo.MipLevels;
	device->CreateShaderResourceView(sand_microbump_texture, &textureSRV_desc, &sand_microbump_textureSRV);


	path = L"../../_Textures/TerrainTextures/rock_bump4.dds";
	//DXUTFindDXSDKMediaFileCch(path.c_str(), MAX_PATH, L"TerrainTextures/rock_bump4.dds");
	D3DX11GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL);
	memset(&imageLoadInfo, 0, sizeof(imageLoadInfo));
	imageLoadInfo.Width = imageInfo.Width;
	imageLoadInfo.Height = imageInfo.Height;
	imageLoadInfo.MipLevels = imageInfo.MipLevels;
	imageLoadInfo.Format = imageInfo.Format;
	imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3DX11CreateTextureFromFile(device, path.c_str(), &imageLoadInfo, NULL, (ID3D11Resource**)&rock_microbump_texture, NULL);

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = imageLoadInfo.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = imageLoadInfo.MipLevels;
	device->CreateShaderResourceView(rock_microbump_texture, &textureSRV_desc, &rock_microbump_textureSRV);

	path = L"../../_Textures/TerrainTextures/water_bump.dds";
	//DXUTFindDXSDKMediaFileCch(path.c_str(), MAX_PATH, L"TerrainTextures/water_bump.dds");
	D3DX11GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL);
	memset(&imageLoadInfo, 0, sizeof(imageLoadInfo));
	imageLoadInfo.Width = imageInfo.Width;
	imageLoadInfo.Height = imageInfo.Height;
	imageLoadInfo.MipLevels = imageInfo.MipLevels;
	imageLoadInfo.Format = imageInfo.Format;
	imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3DX11CreateTextureFromFile(device, path.c_str(), &imageLoadInfo, NULL, (ID3D11Resource**)&water_bump_texture, NULL);

	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = imageLoadInfo.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = imageLoadInfo.MipLevels;
	device->CreateShaderResourceView(water_bump_texture, &textureSRV_desc, &water_bump_textureSRV);

	//	path = L"../../_Textures/TerrainTextures/sky.dds";
	//	//DXUTFindDXSDKMediaFileCch(path.c_str(), MAX_PATH, L"TerrainTextures/sky.dds");
	//	D3DX11GetImageInfoFromFile(path.c_str(), NULL, &imageInfo, NULL);
	//	memset(&imageLoadInfo, 0, sizeof(imageLoadInfo));
	//	imageLoadInfo.Width = imageInfo.Width;
	//	imageLoadInfo.Height = imageInfo.Height;
	//	imageLoadInfo.MipLevels = imageInfo.MipLevels;
	//	imageLoadInfo.Format = imageInfo.Format;
	//	imageLoadInfo.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	//	D3DX11CreateTextureFromFile(device, path.c_str(), &imageLoadInfo, NULL, (ID3D11Resource**)&sky_texture, NULL);
	//
	//	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	//	textureSRV_desc.Format = imageLoadInfo.Format;
	//	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//	textureSRV_desc.Texture2D.MipLevels = imageLoadInfo.MipLevels;
	//	device->CreateShaderResourceView(sky_texture, &textureSRV_desc, &sky_textureSRV);
}



float bilinear_interpolation(float fx, float fy, float a, float b, float c, float d)
{
	float s1, s2, s3, s4;
	s1 = fx * fy;
	s2 = (1 - fx)*fy;
	s3 = (1 - fx)*(1 - fy);
	s4 = fx * (1 - fy);
	return((a*s3 + b * s4 + c * s1 + d * s2));
}
