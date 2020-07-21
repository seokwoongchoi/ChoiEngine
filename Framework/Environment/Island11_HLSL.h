#pragma once


#define terrain_gridpoints					512
#define terrain_numpatches_1d				64
#define terrain_geometry_scale				1.0f
#define terrain_maxheight					30.0f 
#define terrain_minheight					-30.0f 
#define terrain_fractalfactor				0.68f;
#define terrain_fractalinitialvalue			100.0f
#define terrain_smoothfactor1				0.99f
#define terrain_smoothfactor2				0.10f
#define terrain_rockfactor					0.95f
#define terrain_smoothsteps					40

#define terrain_height_underwater_start		-100.0f
#define terrain_height_underwater_end		-8.0f
#define terrain_height_sand_start			-30.0f
#define terrain_height_sand_end				1.7f
#define terrain_height_grass_start			1.7f
#define terrain_height_grass_end			30.0f
#define terrain_height_rocks_start			-2.0f
#define terrain_height_trees_start			4.0f
#define terrain_height_trees_end			30.0f
#define terrain_slope_grass_start			0.96f
#define terrain_slope_rocks_start			0.85f

#define terrain_far_range terrain_gridpoints*terrain_geometry_scale

#define shadowmap_resource_buffer_size_xy				4096
#define water_normalmap_resource_buffer_size_xy			2048
#define terrain_layerdef_map_texture_size				1024
#define terrain_depth_shadow_map_texture_size			512

#define sky_gridpoints						10
#define sky_texture_angle					0.425f

#define main_buffer_size_multiplier			1.0f
#define reflection_buffer_size_multiplier   1.0f
#define refraction_buffer_size_multiplier   1.0f



struct VertexIsland
{
	Vector2 origin;
	Vector2 size;
};
class Island11
{
public:
	explicit Island11();
	~Island11();
	Island11(const Island11&) = delete;
	Island11& operator=(const Island11&) = delete;
public:
	void Update(ID3D11DeviceContext* context);
	void Reflection(ID3D11DeviceContext* context);
	void Terrain(ID3D11DeviceContext* context);
	void Refraction(ID3D11DeviceContext* context,ID3D11ShaderResourceView* srv, ID3D11Texture2D* texture);
	void Water(ID3D11DeviceContext* context);
private:
	void CreateTerrain();
	void ReCreateBuffers();
	void LoadTextures();
	void CreateConstantBuffers();
	void CreateSamplerStates();
	void CreateStates();
	void CreateShaders();
	
private:
	float TotalTime = 0;
	bool RenderCaustics = true;
private:
	
	void SetupNormalView(ID3D11DeviceContext* context, float TerrainBeingRendered, float skipCausticsCalculation);
	void SetupReflectionView(ID3D11DeviceContext* context);
	void SetupRefractionView();
private:
	float BackBufferWidth;
	float BackBufferHeight;

	uint MultiSampleCount;
	uint MultiSampleQuality;

private:
	Vector3 position;
	Vector3 LookAtPoint;
	Vector3 EyePoint;
	
	Matrix view;
	Matrix proj;
	Vector3 direction;
	Matrix rflectionMatrix;

private:
	D3D11_VIEWPORT reflection_Viewport;
	D3D11_VIEWPORT water_normalmap_resource_viewport;
	D3D11_VIEWPORT main_Viewport;


	float origin[2] = { 0,0 };
	UINT stride = sizeof(float) * 4;
	UINT offset = 0;
	UINT cRT = 1;

	float ClearColor[4] = { 0.8f, 0.8f, 1.0f, 1.0f };
	float RefractionClearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };



private:
	float			height[terrain_gridpoints + 1][terrain_gridpoints + 1];
	Vector3			normal[terrain_gridpoints + 1][terrain_gridpoints + 1];
	Vector3			tangent[terrain_gridpoints + 1][terrain_gridpoints + 1];
	Vector3			binormal[terrain_gridpoints + 1][terrain_gridpoints + 1];

	ID3D11Texture2D		*heightmap_texture;
	ID3D11ShaderResourceView *heightmap_textureSRV;

	ID3D11Texture2D		*layerdef_texture;
	ID3D11ShaderResourceView *layerdef_textureSRV;

	ID3D11Texture2D		*depthmap_texture;
	ID3D11ShaderResourceView *depthmap_textureSRV;

	ID3D11Buffer		*heightfield_vertexbuffer;
private:
	ID3D11Texture2D		*rock_bump_texture;
	ID3D11ShaderResourceView *rock_bump_textureSRV;

	ID3D11Texture2D		*rock_microbump_texture;
	ID3D11ShaderResourceView *rock_microbump_textureSRV;

	ID3D11Texture2D		*rock_diffuse_texture;
	ID3D11ShaderResourceView *rock_diffuse_textureSRV;

	ID3D11Texture2D		*sand_bump_texture;
	ID3D11ShaderResourceView *sand_bump_textureSRV;

	ID3D11Texture2D		*sand_microbump_texture;
	ID3D11ShaderResourceView *sand_microbump_textureSRV;

	ID3D11Texture2D		*sand_diffuse_texture;
	ID3D11ShaderResourceView *sand_diffuse_textureSRV;

	ID3D11Texture2D		*grass_diffuse_texture;
	ID3D11ShaderResourceView *grass_diffuse_textureSRV;

	ID3D11Texture2D		*slope_diffuse_texture;
	ID3D11ShaderResourceView *slope_diffuse_textureSRV;

	ID3D11Texture2D		*water_bump_texture;
	ID3D11ShaderResourceView *water_bump_textureSRV;

	ID3D11Texture2D			 *reflection_color_resource;
	ID3D11ShaderResourceView *reflection_color_resourceSRV;
	ID3D11RenderTargetView   *reflection_color_resourceRTV;

	ID3D11Texture2D			 *refraction_color_resource;
	ID3D11ShaderResourceView *refraction_color_resourceSRV;
	ID3D11RenderTargetView   *refraction_color_resourceRTV;

	ID3D11Texture2D			 *shadowmap_resource;
	ID3D11ShaderResourceView *shadowmap_resourceSRV;
	ID3D11DepthStencilView   *shadowmap_resourceDSV;

	ID3D11Texture2D			 *reflection_depth_resource;
	ID3D11DepthStencilView   *reflection_depth_resourceDSV;


	ID3D11Texture2D			 *refraction_depth_resource;
	ID3D11RenderTargetView   *refraction_depth_resourceRTV;
	ID3D11ShaderResourceView *refraction_depth_resourceSRV;

	ID3D11Texture2D			 *water_normalmap_resource;
	ID3D11ShaderResourceView *water_normalmap_resourceSRV;
	ID3D11RenderTargetView   *water_normalmap_resourceRTV;
private:	
	ID3D11InputLayout* terrainInputLayout;

	

	ID3D11VertexShader*  WaterNormalmapCombineVS;
	ID3D11PixelShader*  WaterNormalmapCombinePS;

	ID3D11VertexShader*  RefractionVS;
	ID3D11PixelShader*  RefractionDepthManualResolvePS1;
	   
	ID3D11VertexShader* TerrainReflectVS;
	ID3D11HullShader*   TerrainReflectPatchHS;
	ID3D11DomainShader* TerrainReflectDS;
	ID3D11PixelShader*  TerrainReflectPS;

	ID3D11VertexShader* TerrainPackingVS;
	ID3D11HullShader*   TerrainPackingPatchHS;
	ID3D11DomainShader* TerrainPackingDS;
	ID3D11PixelShader*  TerrainPackingPS;

	ID3D11VertexShader* WaterVS;
	ID3D11HullShader*   WaterPatchHS;
	ID3D11DomainShader* WaterDS;
	ID3D11PixelShader*  WaterPS;
private:
	ID3D11RasterizerState* oldRS;
	ID3D11RasterizerState* CullBack; //Cull back ClockWise true
	ID3D11RasterizerState* NoCull; //Cull None
	ID3D11DepthStencilState* DepthNormal;//DepthFunc = LESS_EQUAL;
private:
	ID3D11SamplerState* SamplerPointClamp;
	ID3D11SamplerState* SamplerLinearClamp;
	ID3D11SamplerState* SamplerLinearWrap;
	ID3D11SamplerState* SamplerAnisotropicWrap;
private:
	struct CB_UPDATE_DS 
	{
		Vector3 LightPositionDS;
		float RenderCausticsDS; //DS
		Vector2 WaterBumpTexcoordShiftDS; //DS PS
		Vector2 pad;

	}updateDSDesc;
	ID3D11Buffer* updateDSBuffer;
	struct CB_UPDATE_PS 
	{
		Vector3 LightPositionPS ; //DS PS
		float pad;
		Vector2 WaterBumpTexcoordShiftPS ; //DS PS
		Vector2 pad2;


	}updatePSDesc;
	ID3D11Buffer* updatePSBuffer;
	struct CB_HullSahder 
	{
		Matrix ModelViewProjectionMatrixHS; //h d p
		Vector3 CameraPositionHS ; //h d p
		float pad;
		Vector3 CameraDirectionHS; //h 
		float TerrainBeingRenderedHS; //HS

	}hullDesc;
	ID3D11Buffer* hullBuffer;
	struct CB_DomainSahder 
	{
		Matrix ModelViewProjectionMatrixDS ; //h d p
		Vector3 CameraPositionDS ; //h d p
		float SkipCausticsCalculationDS ; //DS

	}domainDesc;
	ID3D11Buffer* domainBuffer;
	struct CB_PixelSahder 
	{
		Matrix ModelViewMatrixPS ; //h d p
		Matrix ModelViewProjectionMatrixPS ; //h d p

		Vector3 CameraPositionPS ; //h d p
		float HalfSpaceCullPositionPS; //p
		
	}pixelDesc;
	ID3D11Buffer* pixelBuffer;
	Matrix VP;
};

float bilinear_interpolation(float fx, float fy, float a, float b, float c, float d);

