#include "Framework.h"
#include "Island11_HLSL.h"
#include "Core/D3D11/D3D11_Helper.h"
#include "Utility/QuadTree.h"
#define PI 3.14159265358979323846f
int gp_wrap(int a)
{
	if (a < 0) return (a + terrain_gridpoints);
	if (a >= terrain_gridpoints) return (a - terrain_gridpoints);
	return a;
}


Island11::Island11(ID3D11Device* device, class QuadTree* tree)
	:device(device), MultiSampleCount(1), MultiSampleQuality(0),BackBufferWidth(1280.0f), BackBufferHeight(720.0f),
	rock_bump_texture(nullptr),rock_bump_textureSRV(nullptr),rock_microbump_texture(nullptr),rock_microbump_textureSRV(nullptr),rock_diffuse_texture(nullptr),rock_diffuse_textureSRV(nullptr),
    sand_bump_texture(nullptr),sand_bump_textureSRV(nullptr),sand_microbump_texture(nullptr),sand_microbump_textureSRV(nullptr),sand_diffuse_texture(nullptr),sand_diffuse_textureSRV(nullptr),
    grass_diffuse_texture(nullptr),grass_diffuse_textureSRV(nullptr),slope_diffuse_texture(nullptr),slope_diffuse_textureSRV(nullptr),water_bump_texture(nullptr),water_bump_textureSRV(nullptr),
    reflection_color_resource(nullptr),reflection_color_resourceSRV(nullptr),reflection_color_resourceRTV(nullptr),refraction_color_resource(nullptr),refraction_color_resourceSRV(nullptr),refraction_color_resourceRTV(nullptr),
    reflection_depth_resource(nullptr),reflection_depth_resourceDSV(nullptr),refraction_depth_resource(nullptr),
    refraction_depth_resourceRTV(nullptr),refraction_depth_resourceSRV(nullptr),water_normalmap_resource(nullptr),water_normalmap_resourceSRV(nullptr),water_normalmap_resourceRTV(nullptr)
	, heightMapTexture(nullptr),
	heightmap_texture(nullptr),
    heightmap_textureSRV(nullptr),
    heightmap_textureUAV(nullptr),
	layerdef_texture(nullptr),
    layerdef_textureSRV(nullptr),
	depthmap_texture(nullptr),
    depthmap_textureSRV(nullptr),
	tree(tree)
{
	BackBufferWidth = static_cast<float>(D3D::Width());
	BackBufferHeight = static_cast<float>(D3D::Height());
	CreateVertexBuffer();
	CreateConstantBuffers();
	CreateSamplerStates();
	CreateStates();
	CreateShaders();
	CreateTerrain();
	LoadTextures();
	ReCreateBuffers();

	

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
}

Island11::~Island11()
{
	SafeRelease(heightmap_texture);
	SafeRelease(heightmap_textureSRV);
	SafeRelease(heightmap_textureUAV);

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
	

	//ID3D11ShaderResourceView* arrDSResourece[5] =
	//{ layerdef_textureSRV ,rock_bump_textureSRV ,sand_bump_textureSRV, depthmap_textureSRV,
	//heightmap_textureSRV };

	//context->DSSetShaderResources(0, 5, arrDSResourece);
	//context->DSSetShaderResources(16, 1, &water_normalmap_resourceSRV);

	//ID3D11ShaderResourceView* arrPSResourece[7] =
	//{ rock_microbump_textureSRV ,rock_diffuse_textureSRV ,sand_microbump_textureSRV, sand_diffuse_textureSRV,
	//water_bump_textureSRV,grass_diffuse_textureSRV,slope_diffuse_textureSRV };
	//context->PSSetShaderResources(6, 7, arrPSResourece);

	//ID3D11SamplerState* samArray[4] = { SamplerPointClamp ,SamplerLinearClamp ,SamplerAnisotropicWrap,SamplerLinearWrap };
	//context->PSSetSamplers(0, 4, samArray);
	//context->HSSetSamplers(4, 1, &SamplerLinearWrap);
	//context->DSSetSamplers(5, 1, &SamplerLinearWrap);
	//
	TotalTime += Time::Get()->Delta();

	context->RSGetState(&oldRS);
	Vector3 SunPos;
	SunPos = -1 * 10000 * GlobalData::LightDirection();
	SunPos.y += 5500.f;
	if (SunPos.y < 4000.0f)
	{
		SunPos.y = 4000.0f;
	}

	updateDesc.LightPositionDS = SunPos;
	updateDesc.WaterBumpTexcoordShiftDS = Vector2(TotalTime*1.5f, TotalTime*0.75f);
	updateDesc.RenderCausticsDS = RenderCaustics ? 1.0f : 0.0f;

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(updateBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, &updateDesc, sizeof(CB_UPDATE));
	context->Unmap(updateBuffer, 0);
	context->DSSetConstantBuffers(0, 1, &updateBuffer);
	context->PSSetConstantBuffers(1, 1, &updateBuffer);
}
void Island11::Caustics(ID3D11DeviceContext * context)
{
	if (RenderCaustics)
	{
		SetupNormalView(context, 0.0f, 1.0f);
		context->RSSetState(NoCull);		
		context->RSSetViewports(1, &water_normalmap_resource_viewport);
		context->OMSetRenderTargets(1, &water_normalmap_resourceRTV, NULL);
		context->ClearRenderTargetView(water_normalmap_resourceRTV, ClearColor);
		
		context->VSSetShader(WaterNormalmapCombineVS, nullptr, 0);
		context->PSSetShader(WaterNormalmapCombinePS, nullptr, 0); 


		ID3D11ShaderResourceView* arrPSResourece[1] =
		{ water_bump_textureSRV };

		context->PSSetShaderResources(14, 1, arrPSResourece);
		context->PSSetSamplers(3, 1, &SamplerLinearWrap);



		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);


		context->Draw(4, 0);

		context->RSSetState(oldRS);

		ZeroMemory(&arrPSResourece, sizeof(arrPSResourece));
		context->PSSetShaderResources(14, 1, arrPSResourece);

		ID3D11SamplerState* nullSamp = nullptr;
		context->PSSetSamplers(3, 1, &nullSamp);
		
		context->VSSetShader(nullptr, nullptr, 0);
		context->PSSetShader(nullptr, nullptr, 0);
	}
}
void Island11::Reflection(ID3D11DeviceContext* context)
{
	


	context->RSSetViewports(1, &reflection_Viewport);
	context->OMSetRenderTargets(1, &reflection_color_resourceRTV, reflection_depth_resourceDSV);
	context->ClearRenderTargetView(reflection_color_resourceRTV, RefractionClearColor);
	context->ClearDepthStencilView(reflection_depth_resourceDSV, D3D11_CLEAR_DEPTH, 1.0, 0);
	SetupReflectionView(context);

	context->RSSetState(CullBack);
	ID3D11DepthStencilState* PrevDepthState;
	UINT PrevStencil;
	context->OMGetDepthStencilState(&PrevDepthState, &PrevStencil);
	//context->OMSetDepthStencilState(DepthNormal, 2);

	context->VSSetShader(TerrainPackingVS, nullptr, 0);
	context->HSSetShader(TerrainPackingPatchHS, nullptr, 0);
	context->DSSetShader(TerrainPackingDS, nullptr, 0);
	context->PSSetShader(TerrainReflectPS, nullptr, 0);


	context->HSSetShaderResources(0, 1, &heightmap_textureSRV);

	ID3D11ShaderResourceView* arrDSResourece[5] =
	{ layerdef_textureSRV ,rock_bump_textureSRV ,sand_bump_textureSRV, heightmap_textureSRV,
	water_normalmap_resourceSRV };
	context->DSSetShaderResources(1, 5, arrDSResourece);


	ID3D11ShaderResourceView* arrPSResourece[6] =
	{ rock_microbump_textureSRV ,rock_diffuse_textureSRV ,sand_microbump_textureSRV, sand_diffuse_textureSRV,
	grass_diffuse_textureSRV,slope_diffuse_textureSRV };
	context->PSSetShaderResources(8, 6, arrPSResourece);


	context->PSSetSamplers(2, 1, &SamplerAnisotropicWrap);
	context->HSSetSamplers(4, 1, &SamplerLinearWrap);
	context->DSSetSamplers(5, 1, &SamplerLinearWrap);


	context->IASetInputLayout(*inputLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	stride = sizeof(float) * 4;
	context->IASetVertexBuffers(0, 1, &heightfield_vertexbuffer, &stride, &offset);
	context->Draw(terrain_numpatches_1d*terrain_numpatches_1d, 0);



	ID3D11ShaderResourceView* nullView = nullptr;
	context->HSSetShaderResources(0, 1, &nullView);
	ZeroMemory(&arrDSResourece, sizeof(arrDSResourece));
	context->DSSetShaderResources(1, 5, arrDSResourece);
	ZeroMemory(&arrPSResourece, sizeof(arrPSResourece));
	context->PSSetShaderResources(8, 6, arrPSResourece);

	ID3D11SamplerState* nullSamp = nullptr;
	context->PSSetSamplers(2, 1, &nullSamp);
	context->HSSetSamplers(4, 1, &nullSamp);
	context->DSSetSamplers(5, 1, &nullSamp);
	context->RSSetState(oldRS);
	context->OMSetDepthStencilState(PrevDepthState, PrevStencil);
	context->VSSetShader(nullptr, nullptr, 0);
	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}


void Island11::Terrain(ID3D11DeviceContext* context)
{
	SetupNormalView(context, 1.0f, 0.0f);

	context->RSSetState(CullBack);
	ID3D11DepthStencilState* PrevDepthState;
	UINT PrevStencil;
	context->OMGetDepthStencilState(&PrevDepthState, &PrevStencil);
	//context->OMSetDepthStencilState(DepthNormal, 2);

	context->VSSetShader(TerrainPackingVS, nullptr, 0);
	context->HSSetShader(TerrainPackingPatchHS, nullptr, 0);
	context->DSSetShader(TerrainPackingDS, nullptr, 0);
	context->PSSetShader(TerrainPackingPS, nullptr, 0);
		
	//auto srv = heightMapTexture->SRV();
	context->HSSetShaderResources(0, 1, &heightmap_textureSRV);

	ID3D11ShaderResourceView* arrDSResourece[5] =
	{ layerdef_textureSRV ,rock_bump_textureSRV ,sand_bump_textureSRV, heightmap_textureSRV,
	water_normalmap_resourceSRV };
	context->DSSetShaderResources(1, 5, arrDSResourece);
	

	ID3D11ShaderResourceView* arrPSResourece[6] =
	{ rock_microbump_textureSRV ,rock_diffuse_textureSRV ,sand_microbump_textureSRV, sand_diffuse_textureSRV,
	grass_diffuse_textureSRV,slope_diffuse_textureSRV };
	context->PSSetShaderResources(8, 6, arrPSResourece);
	
	
	context->PSSetSamplers(2, 1, &SamplerAnisotropicWrap);
	context->HSSetSamplers(4, 1, &SamplerLinearWrap);
	context->DSSetSamplers(5, 1, &SamplerLinearWrap);

	
	context->IASetInputLayout(*inputLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	stride = sizeof(float) * 4;
	context->IASetVertexBuffers(0, 1, &heightfield_vertexbuffer, &stride, &offset);
	context->Draw( terrain_numpatches_1d*terrain_numpatches_1d, 0);



	ID3D11ShaderResourceView* nullView = nullptr;
	context->HSSetShaderResources(0, 1, &nullView);
	ZeroMemory(&arrDSResourece, sizeof(arrDSResourece));
	context->DSSetShaderResources(1, 5, arrDSResourece);
	ZeroMemory(&arrPSResourece, sizeof(arrPSResourece));
	context->PSSetShaderResources(8, 6, arrPSResourece);

	ID3D11SamplerState* nullSamp = nullptr;
	context->PSSetSamplers(2, 1, &nullSamp);
	context->HSSetSamplers(4, 1, &nullSamp);
	context->DSSetSamplers(5, 1, &nullSamp);
	context->RSSetState(oldRS);
	context->OMSetDepthStencilState(PrevDepthState, PrevStencil);
	context->VSSetShader(nullptr, nullptr, 0);
	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

void Island11::Refraction(ID3D11DeviceContext* context,ID3D11ShaderResourceView* srv, ID3D11Texture2D* texture)
{
	context->RSSetState(NoCull);

	context->VSSetShader(RefractionVS, nullptr, 0);
	context->PSSetShader(RefractionDepthManualResolvePS1, nullptr, 0);

	context->ResolveSubresource(refraction_color_resource, 0, texture, 0, DXGI_FORMAT_R10G10B10A2_UNORM);
	context->RSSetViewports(1, &main_Viewport);
	
	context->OMSetRenderTargets(1, &refraction_depth_resourceRTV, nullptr);
	context->ClearRenderTargetView(refraction_depth_resourceRTV, Color(1, 1, 1, 1));

	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->PSSetShaderResources(18, 1, &srv);
	

	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);

	context->Draw(  4, 0);
	context->RSSetState(oldRS);

	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}

void Island11::Water(ID3D11DeviceContext* context)
{
	SetupNormalView(context, 0.0f, 1.0f);
	context->RSSetState(CullBack);

	context->VSSetShader(TerrainPackingVS, nullptr, 0);
	context->HSSetShader(WaterPatchHS, nullptr, 0);
	context->DSSetShader(WaterDS, nullptr, 0);
	context->PSSetShader(WaterPS, nullptr, 0);

	
	ID3D11ShaderResourceView* arrDSResourece[2] =
	{  depthmap_textureSRV ,water_bump_textureSRV };

	context->DSSetShaderResources(6, 2, arrDSResourece);
	
	ID3D11ShaderResourceView* arrPSResourece[4] =
	{   water_bump_textureSRV,reflection_color_resourceSRV ,
		refraction_color_resourceSRV,refraction_depth_resourceSRV };

	context->PSSetShaderResources(14, 4, arrPSResourece);

	ID3D11SamplerState* samArray[3] = { SamplerPointClamp ,SamplerLinearClamp ,SamplerAnisotropicWrap };
	context->PSSetSamplers(0, 3, samArray);
	context->DSSetSamplers(5, 1, &SamplerLinearWrap);
	

	context->IASetInputLayout(*inputLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	stride = sizeof(float) * 4;
	context->IASetVertexBuffers(0, 1, &heightfield_vertexbuffer, &stride, &offset);
	
	context->Draw(terrain_numpatches_1d*terrain_numpatches_1d, 0);
	context->RSSetState(oldRS);

	
	ZeroMemory(&arrDSResourece, sizeof(arrDSResourece));
	context->DSSetShaderResources(6, 2, arrDSResourece);
	ZeroMemory(&arrPSResourece, sizeof(arrPSResourece));
	context->PSSetShaderResources(14, 4, arrPSResourece);

	ZeroMemory(&samArray, sizeof(samArray));
	context->PSSetSamplers(0, 3, samArray);
	ID3D11SamplerState* nullSamp = nullptr;
	context->DSSetSamplers(5, 1, &nullSamp);

	context->VSSetShader(nullptr, nullptr, 0);
	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);
}



void Island11::SetupNormalView(ID3D11DeviceContext* context,float TerrainBeingRendered,float skipCausticsCalculation)
{

	
	{
		Matrix  vp;
		GlobalData::GetView(&view);
		GlobalData::GetVP(&vp);
		D3DXMatrixTranspose(&frameDesc.ModelViewMatrix, &view);
		D3DXMatrixTranspose(&frameDesc.ModelViewProjectionMatrix, &vp);
	
		frameDesc.CameraPosition = GlobalData::Position();
		frameDesc.SkipCausticsCalculation = skipCausticsCalculation;
		D3DXVec3Normalize(&frameDesc.CameraDirection, &GlobalData::Forward());
	
		frameDesc.TerrainBeingRendered= TerrainBeingRendered;
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(frameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &frameDesc, sizeof(CB_Frame));
		context->Unmap(frameBuffer, 0);
		context->HSSetConstantBuffers(2, 1, &frameBuffer);
		context->DSSetConstantBuffers(3, 1, &frameBuffer);
		context->PSSetConstantBuffers(4, 1, &frameBuffer);
	}

	





}

void Island11::SetupReflectionView(ID3D11DeviceContext* context)
{
	

	auto position = GlobalData::Position();
	EyePoint = GlobalData::Position();

	LookAtPoint = GlobalData::LookAt();
	EyePoint.y = -1.0f*EyePoint.y + 1.0f;
	LookAtPoint.y = -1.0f*LookAtPoint.y + 1.0f;

	 GlobalData::GetView(&view);

	for(uint i=0;i<3;i++)
	  view.m[3][i] = 0;

	D3DXMatrixTranspose(&view, &view);
	view.m[3][0] = position.x;
	view.m[3][1] = position.y;
	view.m[3][1] = -view._42 - 1.0f;

	view.m[3][2] = position.z;

	view.m[1][0] *= -1.0f;
	view.m[1][2] *= -1.0f;
	view.m[2][1] *= -1.0f;


	D3DXMatrixInverse(&rflectionMatrix, nullptr, &view);

	Matrix proj,VP;
	GlobalData::GetProj(&proj);
	D3DXMatrixMultiply(&VP, &rflectionMatrix, &proj);
	


	{
		
		D3DXMatrixTranspose(&frameDesc.ModelViewProjectionMatrix, &VP);
		frameDesc.CameraPosition = EyePoint;
		frameDesc.SkipCausticsCalculation = 1.0f;
		D3DXVec3Normalize(&frameDesc.CameraDirection, &(LookAtPoint - EyePoint));

		frameDesc.TerrainBeingRendered = 1.0f;
		D3D11_MAPPED_SUBRESOURCE MappedResource;
		context->Map(frameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
		memcpy(MappedResource.pData, &frameDesc, sizeof(CB_Frame));
		context->Unmap(frameBuffer, 0);
		context->HSSetConstantBuffers(2, 1, &frameBuffer);
		context->DSSetConstantBuffers(3, 1, &frameBuffer);
		context->PSSetConstantBuffers(4, 1, &frameBuffer);
	}

	
	D3DXMatrixTranspose(&VP, &VP);
	D3D11_MAPPED_SUBRESOURCE MappedResource;
	context->Map(mainViewBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
	memcpy(MappedResource.pData, VP, sizeof(Matrix));
	context->Unmap(mainViewBuffer, 0);
	context->VSSetConstantBuffers(1, 1, &mainViewBuffer);
}

void Island11::SetupRefractionView()
{
	
}




void Island11::CreateVertexBuffer()
{
	float* patches_rawdata = new float[terrain_numpatches_1d*terrain_numpatches_1d * 4];
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

	D3D11_SUBRESOURCE_DATA subresource_data;
	ZeroMemory(&subresource_data, sizeof(D3D11_SUBRESOURCE_DATA));
	subresource_data.pSysMem = patches_rawdata;
	subresource_data.SysMemPitch = 0;
	subresource_data.SysMemSlicePitch = 0;

	Check(device->CreateBuffer(&buf_desc, &subresource_data, &heightfield_vertexbuffer));
	SafeDeleteArray(patches_rawdata);
}

void Island11::CreateTerrain()
{
	SafeRelease(heightmap_texture);
	SafeRelease(heightmap_textureSRV);
	SafeRelease(heightmap_textureUAV);
	SafeRelease(layerdef_texture);
	SafeRelease(layerdef_textureSRV);

	SafeRelease(depthmap_texture);
    SafeRelease(depthmap_textureSRV);



	float**	heightMap=new float*[terrain_gridpoints + 1];
	Vector3** normal = new Vector3*[terrain_gridpoints + 1];
	for (uint i = 0; i < terrain_gridpoints + 1; i++)
	{
		heightMap[i] = new float[terrain_gridpoints+1];
		normal[i] = new Vector3[terrain_gridpoints + 1];
	}

	//LoadHeightMap(L"HeightMap1.height",normal,heightMap);
	
	SafeDelete(heightMapTexture);
	heightMapTexture = new Texture();
	
	heightMapTexture->Load(device, L"Terrain/Map.dds", nullptr, true);


	ID3D11Texture2D* srcTexture;

	heightMapTexture->SRV()->GetResource((ID3D11Resource **)&srcTexture);


	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	srcTexture->GetDesc(&desc);
	//desc.Width = srcDesc.Width;
	//desc.Height = srcDesc.Height;
	//desc.MipLevels = 1;
	//desc.ArraySize = 1;
	//desc.Format = readFormat;
	//desc.SampleDesc = srcDesc.SampleDesc;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;


	HRESULT hr;

	ID3D11Texture2D* texture;
	hr = device->CreateTexture2D(&desc, NULL, &texture);
	Check(hr);

	ID3D11DeviceContext* context;
	device->GetImmediateContext(&context);
	hr = D3DX11LoadTextureFromTexture(context, srcTexture, NULL, texture);
	Check(hr);


	D3D11_MAPPED_SUBRESOURCE map;
	D3DXCOLOR* colors = new D3DXCOLOR[desc.Width * desc.Height];
	context->Map(texture, 0, D3D11_MAP_READ, NULL, &map);
	{
		memcpy(colors, map.pData, sizeof(D3DXCOLOR) * desc.Width * desc.Height);
	}
	context->Unmap(texture, 0);


	float* data = new float[terrain_gridpoints*terrain_gridpoints * 4];
	for (int i = 0; i < terrain_gridpoints; i++)
		for (int j = 0; j < terrain_gridpoints; j++)
		{

			UINT index = i + j * terrain_gridpoints;
			data[(i + j * terrain_gridpoints) * 4 + 0] = colors[index].r;
			data[(i + j * terrain_gridpoints) * 4 + 1] = colors[index].g;
			data[(i + j * terrain_gridpoints) * 4 + 2] = colors[index].b;
			normal[i][j] = Vector3(colors[index].r, colors[index].g, colors[index].b);
			data[(i + j * terrain_gridpoints) * 4 + 3] = colors[index].a;
			heightMap[i][j] = colors[index].a;

		}

	tree->CreateQuadTree(Vector2(0.0f, 0.0f), Vector2(512.0f, 512.0f), heightMap);
	SafeDeleteArray(colors);
	SafeRelease(srcTexture);
	SafeRelease(texture);


	
	D3D11_TEXTURE2D_DESC tex_desc;
	ZeroMemory(&tex_desc, sizeof(tex_desc));
	tex_desc.Width = terrain_gridpoints;
	tex_desc.Height = terrain_gridpoints;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE| D3D11_BIND_UNORDERED_ACCESS;
	tex_desc.CPUAccessFlags =0;
	tex_desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA subresource_data;
	ZeroMemory(&subresource_data, sizeof(subresource_data));

	subresource_data.pSysMem = data;
	subresource_data.SysMemPitch = terrain_gridpoints * 4 * sizeof(float);
	subresource_data.SysMemSlicePitch = 0;

	Check(device->CreateTexture2D(&tex_desc, &subresource_data, &heightmap_texture));


	D3D11_SHADER_RESOURCE_VIEW_DESC textureSRV_desc;
	ZeroMemory(&textureSRV_desc, sizeof(textureSRV_desc));
	textureSRV_desc.Format = tex_desc.Format;
	textureSRV_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureSRV_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	Check(device->CreateShaderResourceView(heightmap_texture, &textureSRV_desc, &heightmap_textureSRV));


	// create the UAV for the structured buffer
	D3D11_UNORDERED_ACCESS_VIEW_DESC sbUAVDesc;
	sbUAVDesc.Buffer.FirstElement = 0;
	sbUAVDesc.Buffer.Flags = 0;
	sbUAVDesc.Buffer.NumElements = terrain_gridpoints * terrain_gridpoints ;
	sbUAVDesc.Format = tex_desc.Format;
	sbUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	Check(device->CreateUnorderedAccessView(heightmap_texture, &sbUAVDesc, &heightmap_textureUAV));


	int i, j, k, l;
	float x, z;
	int ix, iz;
	float * backterrain;
	D3DXVECTOR3 vec1, vec2, vec3;
	int currentstep = terrain_gridpoints;
	float mv, rm;
	float offset = 0, yscale = 0, maxheight = 0, minheight = 0;

	

	

	backterrain = (float *)malloc((terrain_gridpoints + 1)*(terrain_gridpoints + 1) * sizeof(float));
	rm = terrain_fractalinitialvalue;
	backterrain[0] = 0;
	backterrain[0 + terrain_gridpoints * terrain_gridpoints] = 0;
	backterrain[terrain_gridpoints] = 0;
	backterrain[terrain_gridpoints + terrain_gridpoints * terrain_gridpoints] = 0;
	currentstep = terrain_gridpoints;
	//srand(12);
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


	// smoothing 
	for (k = 0; k < terrain_smoothsteps; k++)
	{
		for (i = 0; i < terrain_gridpoints + 1; i++)
			for (j = 0; j < terrain_gridpoints + 1; j++)
			{

				vec1.x = 2 * terrain_geometry_scale;
				vec1.y = terrain_geometry_scale * (heightMap[gp_wrap(i + 1)][j] - heightMap[gp_wrap(i - 1)][j]);
				vec1.z = 0;
				vec2.x = 0;
				vec2.y = -terrain_geometry_scale * (heightMap[i][gp_wrap(j + 1)] - heightMap[i][gp_wrap(j - 1)]);
				vec2.z = -2 * terrain_geometry_scale;

				D3DXVec3Cross(&vec3, &vec1, &vec2);
				D3DXVec3Normalize(&vec3, &vec3);


				if (((vec3.y > terrain_rockfactor) || (heightMap[i][j] < 1.2f)))
				{
					rm = terrain_smoothfactor1;
					mv = heightMap[i][j] * (1.0f - rm) + rm * 0.25f*(heightMap[gp_wrap(i - 1)][j] + heightMap[i][gp_wrap(j - 1)] + heightMap[gp_wrap(i + 1)][j] + heightMap[i][gp_wrap(j + 1)]);
					backterrain[i + terrain_gridpoints * j] = mv;
				}
				else
				{
					rm = terrain_smoothfactor2;
					mv = heightMap[i][j] * (1.0f - rm) + rm * 0.25f*(heightMap[gp_wrap(i - 1)][j] + heightMap[i][gp_wrap(j - 1)] + heightMap[gp_wrap(i + 1)][j] + heightMap[i][gp_wrap(j + 1)]);
					backterrain[i + terrain_gridpoints * j] = mv;
				}

			}
		for (i = 0; i < terrain_gridpoints + 1; i++)
			for (j = 0; j < terrain_gridpoints + 1; j++)
			{
				heightMap[i][j] = (backterrain[i + terrain_gridpoints * j]);
			}
	}
	for (i = 0; i < terrain_gridpoints + 1; i++)
		for (j = 0; j < terrain_gridpoints + 1; j++)
		{
			rm = 0.5f;
			mv = heightMap[i][j] * (1.0f - rm) + rm * 0.25f*(heightMap[gp_wrap(i - 1)][j] + heightMap[i][gp_wrap(j - 1)] + heightMap[gp_wrap(i + 1)][j] + heightMap[i][gp_wrap(j + 1)]);
			backterrain[i + terrain_gridpoints * j] = mv;
		}
	for (i = 0; i < terrain_gridpoints + 1; i++)
		for (j = 0; j < terrain_gridpoints + 1; j++)
		{
			heightMap[i][j] = (backterrain[i + terrain_gridpoints * j]);
		}


	free(backterrain);


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
			rm = bilinear_interpolation(x - ix, z - iz, heightMap[ix][iz], heightMap[ix + 1][iz], heightMap[ix + 1][iz + 1], heightMap[ix][iz + 1])*terrain_geometry_scale;

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
			rm = bilinear_interpolation(x - ix, z - iz, heightMap[ix][iz], heightMap[ix + 1][iz], heightMap[ix + 1][iz + 1], heightMap[ix][iz + 1])*terrain_geometry_scale;

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


	

	for (uint i = 0; i < terrain_gridpoints + 1; i++)
	{
		SafeDeleteArray(heightMap[i]);
		SafeDeleteArray(normal[i]);
	}
		
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

void Island11::CreateConstantBuffers()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.ByteWidth = sizeof(CB_UPDATE);
	Check(device->CreateBuffer(&bufferDesc, NULL, &updateBuffer));
	bufferDesc.ByteWidth = sizeof(CB_Frame);
	Check(device->CreateBuffer(&bufferDesc, NULL, &frameBuffer));
	bufferDesc.ByteWidth = sizeof(Matrix);
	Check(device->CreateBuffer(&bufferDesc, NULL, &mainViewBuffer));

	


}

void Island11::CreateSamplerStates()
{
	D3D11_SAMPLER_DESC samDesc;
	ZeroMemory(&samDesc, sizeof(D3D11_SAMPLER_DESC));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &SamplerPointClamp));

	ZeroMemory(&samDesc, sizeof(D3D11_SAMPLER_DESC));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &SamplerLinearClamp));


	ZeroMemory(&samDesc, sizeof(D3D11_SAMPLER_DESC));
	samDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 1;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &SamplerLinearWrap));


	ZeroMemory(&samDesc, sizeof(D3D11_SAMPLER_DESC));
	samDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samDesc.AddressU = samDesc.AddressV = samDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samDesc.MaxAnisotropy = 16;
	samDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Check(device->CreateSamplerState(&samDesc, &SamplerAnisotropicWrap));
}

void Island11::CreateStates()
{

	D3D11_RASTERIZER_DESC reasterizerDesc;
	ZeroMemory(&reasterizerDesc, sizeof(reasterizerDesc));
	
	reasterizerDesc.CullMode = D3D11_CULL_NONE;
	reasterizerDesc.FillMode = D3D11_FILL_SOLID;
	Check(device->CreateRasterizerState(&reasterizerDesc, &NoCull));

	reasterizerDesc.FillMode = D3D11_FILL_SOLID;
	//reasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	reasterizerDesc.CullMode = D3D11_CULL_BACK;
	reasterizerDesc.FrontCounterClockwise = true;
	Check(device->CreateRasterizerState(&reasterizerDesc, &CullBack));
	



	D3D11_DEPTH_STENCIL_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.DepthEnable = FALSE;
	descDepth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	descDepth.DepthFunc = D3D11_COMPARISON_LESS;
	descDepth.StencilEnable = TRUE;
	descDepth.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	descDepth.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	const D3D11_DEPTH_STENCILOP_DESC noSkyStencilOp = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS };
	descDepth.FrontFace = noSkyStencilOp;
	descDepth.BackFace = noSkyStencilOp;
	
	Check(device->CreateDepthStencilState(&descDepth, &DepthNormal));
}

void Island11::CreateShaders()
{
	// Compile the shaders
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	auto path = "../_Shaders/Environment/HLSL/Island11.hlsl";
	auto entryPoint = "WaterNormalmapCombineVS";
	auto shaderModel = "vs_5_0";
	// Load the directional light shaders
	ID3DBlob* pShaderBlob = NULL;
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreateVertexShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &WaterNormalmapCombineVS));

	//FullScreenInputLayout = new InputLayout();
	//FullScreenInputLayout->Create(device, pShaderBlob);
	
	SafeRelease(pShaderBlob);

	entryPoint = "FullScreenQuadVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreateVertexShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &RefractionVS));
	SafeRelease(pShaderBlob);

	entryPoint = "WaterNormalmapCombinePS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &WaterNormalmapCombinePS));
	SafeRelease(pShaderBlob);
	entryPoint = "RefractionDepthManualResolvePS1";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &RefractionDepthManualResolvePS1));

	SafeRelease(pShaderBlob);

	entryPoint = "PassThroughVS";
	shaderModel = "vs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreateVertexShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &TerrainPackingVS));
	inputLayout = make_shared<InputLayout>();
	inputLayout->Create(device, pShaderBlob);
	SafeRelease(pShaderBlob);

	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	

	entryPoint = "PatchHS";
	shaderModel = "hs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreateHullShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &TerrainPackingPatchHS));
	SafeRelease(pShaderBlob);

	entryPoint = "WaterPatchHS";
	shaderModel = "hs_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreateHullShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &WaterPatchHS));
	SafeRelease(pShaderBlob);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	

	entryPoint = "HeightFieldPatchDS";
	shaderModel = "ds_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreateDomainShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &TerrainPackingDS));
	SafeRelease(pShaderBlob);

	entryPoint = "WaterPatchDS";
	shaderModel = "ds_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreateDomainShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &WaterDS));
	SafeRelease(pShaderBlob);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	

	entryPoint = "HeightFieldPatchPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &TerrainReflectPS));
	SafeRelease(pShaderBlob);


	entryPoint = "HeightFieldPatchPacking";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &TerrainPackingPS));
	SafeRelease(pShaderBlob);


	entryPoint = "WaterPatchPS";
	shaderModel = "ps_5_0";
	Check(D3D11_Helper::CompileShader(path, entryPoint, shaderModel, nullptr, &pShaderBlob));
	Check(device->CreatePixelShader(pShaderBlob->GetBufferPointer(),
		pShaderBlob->GetBufferSize(), NULL, &WaterPS));

	

	SafeRelease(pShaderBlob);

	
}

void Island11::SaveHeightMap()
{
	

	/*D3D11_TEXTURE2D_DESC dstDesc;
	ZeroMemory(&dstDesc, sizeof(D3D11_TEXTURE2D_DESC));
	dstDesc.Width = terrain_gridpoints;
	dstDesc.Height = terrain_gridpoints;
	dstDesc.MipLevels = 1;
	dstDesc.ArraySize = 1;
	dstDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	dstDesc.SampleDesc.Count = 1;
	dstDesc.SampleDesc.Quality = 0;
	dstDesc.Usage = D3D11_USAGE_STAGING;
	dstDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

	ID3D11Texture2D* dstTexture = nullptr;

	auto hr = device ->CreateTexture2D
	(
		&dstDesc,
		nullptr,
		&dstTexture
	);
	assert(SUCCEEDED(hr));
*/
	ID3D11DeviceContext* context;
	device->GetImmediateContext(&context);


	//context->CopyResource((ID3D11Resource*)dstTexture, (ID3D11Resource*)heightmap_texture);
	
	auto hr = D3DX11SaveTextureToFileA
	(
		context,
		heightmap_texture,
		D3DX11_IFF_DDS,
		"../_Textures/Terrain/Map.dds"
	);
	assert(SUCCEEDED(hr));
	//SafeRelease(dstTexture);

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	heightmap_texture->GetDesc(&desc);
	desc.BindFlags = 0;
	desc.MiscFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;



	ID3D11Texture2D* texture;
	hr = device->CreateTexture2D(&desc, NULL, &texture);
	Check(hr);


	hr = D3DX11LoadTextureFromTexture(context, heightmap_texture, NULL, texture);
	Check(hr);


	D3D11_MAPPED_SUBRESOURCE map;
	D3DXCOLOR* colors = new D3DXCOLOR[desc.Width * desc.Height];
	context->Map(texture, 0, D3D11_MAP_READ, NULL, &map);
	{
		memcpy(colors, map.pData, sizeof(D3DXCOLOR) * desc.Width * desc.Height);
	}
	context->Unmap(texture, 0);


	float**	heightMap = new float*[terrain_gridpoints + 1];
	for (uint i = 0; i < terrain_gridpoints + 1; i++)
	{
		heightMap[i] = new float[terrain_gridpoints + 1];

	}
	for (int i = 0; i < terrain_gridpoints; i++)
		for (int j = 0; j < terrain_gridpoints; j++)
		{

		   UINT index = i + j * terrain_gridpoints;
		   heightMap[i][j] = colors[index].a;

		}

	tree->UpdateHeight(Vector2(0.0f, 0.0f), Vector2(512.0f, 512.0f), heightMap);
	SafeDeleteArray(colors);
	
	SafeRelease(texture);
	for (uint i = 0; i < terrain_gridpoints + 1; i++)
	{
		SafeDeleteArray(heightMap[i]);
    }
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
