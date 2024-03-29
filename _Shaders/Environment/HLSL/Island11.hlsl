// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com


SamplerState SamplerPointClamp : register(s0);//ps
SamplerState SamplerLinearClamp : register(s1);//ps
SamplerState SamplerAnisotropicWrap : register(s2);//ps
SamplerState SamplerLinearWrapPS : register(s3);//hs ds ps
SamplerState SamplerLinearWrapHS : register(s4); //hs ds ps
SamplerState SamplerLinearWrapDS : register(s5); //hs ds ps


static const float2 QuadVertices[4] =
{
    { -1.0, -1.0 },
    { 1.0, -1.0 },
    { -1.0, 1.0 },
    { 1.0, 1.0 }
};

static const float2 QuadTexCoordinates[4] =
{
    { 0.0, 1.0 },
    { 1.0, 1.0 },
    { 0.0, 0.0 },
    { 1.0, 0.0 }
};
//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// static textures

Texture2D HeightfieldTextureHS : register(t0); //HS
Texture2D LayerdefTexture : register(t1); //DS
Texture2D RockBumpTexture : register(t2); //ds
Texture2D SandBumpTexture : register(t3); //ds
Texture2D HeightfieldTextureDS : register(t4); //HS DS
Texture2D WaterNormalMapTexture : register(t5); //ds
Texture2D DepthMapTexture : register(t6); //ds
Texture2D WaterBumpTextureDS : register(t7); //ds

Texture2D RockMicroBumpTexture : register(t8); //ps
Texture2D RockDiffuseTexture : register(t9); //ps
Texture2D SandMicroBumpTexture : register(t10); //ps
Texture2D SandDiffuseTexture : register(t11); //ps
Texture2D GrassDiffuseTexture : register(t12); //ps
Texture2D SlopeDiffuseTexture : register(t13); //ps
Texture2D WaterBumpTexture : register(t14); //ps
//water rendertarget textures
Texture2D ReflectionTexture : register(t15); //ps
Texture2D RefractionTexture : register(t16);//ps
Texture2D RefractionDepthTextureResolved : register(t17);//ps
//terrain rendertarget textures

//

Texture2D<float> RefractionDepthTextureMS1 : register(t18); //ps



//--------------------------------------------------------------------------------------
// Shader Inputs/Outputs
//--------------------------------------------------------------------------------------


struct VSIn_Diffuse
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct PSIn_Diffuse
{
    float4 position : SV_Position;
    centroid float2 texcoord : TEXCOORD0;
    centroid float3 normal : NORMAL;
    centroid float4 positionWS : TEXCOORD1;
    centroid float4 layerdef : TEXCOORD2;
   
};

struct PSIn_Quad
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct VSIn_Default
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD;
};


struct DUMMY
{
    float Dummmy : DUMMY;
};

struct HSIn_Heightfield
{
    float2 origin : ORIGIN;
    float2 size : SIZE;
};


struct PatchData
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;

    float2 origin : ORIGIN;
    float2 size : SIZE;
};

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------

cbuffer CB_UPDATE_DS : register(b0)
{
    float3 LightPositionDS : packoffset(c0); //DS PS
    float RenderCausticsDS : packoffset(c0.w); //DS
    float2 WaterBumpTexcoordShiftDS : packoffset(c1); //DS PS
   
  
};
cbuffer CB_UPDATE_PS : register(b1)
{
    float3 LightPositionPS : packoffset(c0); //DS PS
    float pad1 : packoffset(c0.w); //DS
    float2 WaterBumpTexcoordShiftPS : packoffset(c1); //DS PS
  
};
cbuffer CB_HullSahder : register(b2)
{
    float4x4 ModelViewProjectionMatrixHS : packoffset(c0); //h d p
    float3 CameraPositionHS : packoffset(c4); //h d p
    float pad2 : packoffset(c4.w); //p
    float3 CameraDirectionHS : packoffset(c5); //h 
    float TerrainBeingRenderedHS : packoffset(c5.w); //HS
   
};
cbuffer CB_DomainSahder : register(b3)
{
    float4x4 ModelViewProjectionMatrixDS : packoffset(c0); //h d p
    float3 CameraPositionDS : packoffset(c4); //h d p
    float SkipCausticsCalculationDS : packoffset(c4.w); //p
 
};
cbuffer CB_PixelSahder : register(b4)
{
   
    float4x4 ModelViewProjectionMatrixPS : packoffset(c0); //h d p
    
    float3 CameraPositionPS : packoffset(c4); //h d p
    float pad3 : packoffset(c4.w); //p
    
    float3 CameraDirectionPS : packoffset(c5); //h 
    float pad4 : packoffset(c5.w); //p
    
    float4x4 ModelViewMatrixPS : packoffset(c6); //h d p
};




//--------------------------------------------------------------------------------------
// Misc functions
//--------------------------------------------------------------------------------------
// constants defining visual appearance
static const float2 DiffuseTexcoordScale = { 130.0, 130.0 };
static const float2 RockBumpTexcoordScale = { 10.0, 10.0 };
static const float RockBumpHeightScale = 3.0;
static const float2 SandBumpTexcoordScale = { 3.5, 3.5 };
static const float SandBumpHeightScale = 0.5;
static const float TerrainSpecularIntensity = 0.5;
static const float2 WaterMicroBumpTexcoordScale = { 225, 225 };
static const float2 WaterBumpTexcoordScale = { 7, 7 };
static const float WaterHeightBumpScale = 1.0f;
static const float3 WaterDeepColor = { 0.3, 0.6, 0.9 };
static const float3 WaterScatterColor = { 0.3, 0.7, 0.6 };
static const float3 WaterSpecularColor = { 1, 1, 1 };
static const float WaterSpecularIntensity = 350.0;
static const float DynamicTessFactor = 50.0f;
static const float WaterSpecularPower = 800;
static const float2 WaterColorIntensity = { 0.1, 0.2 };
static const float2 HalfSpaceCullPosition = -0.6f;//-30.0f * 2.0f;
static const float2 HeightFieldOrigin = float2(0, 0);
static const float HeightFieldSize = 512;
static const float ZNear = 0.1f;
static const float ZFar = 1000.0f;
static const float2 ScreenSizeInv = float2(1.0f / (1280.0f ), 1.0f / (720.0f ));
// calculating tessellation factor. It is either constant or hyperbolic depending on UseDynamicLOD switch
float CalculateTessellationFactor(float distance) //hs
{
    return  DynamicTessFactor * (1 / (0.06 * distance));
}

// to avoid vertex swimming while tessellation varies, one can use mipmapping for displacement maps
// it's not always the best choice, but it effificiently suppresses high frequencies at zero cost
float CalculateMIPLevelForDisplacementTextures(float distance) //ds
{
    return log2(128 / CalculateTessellationFactor(distance));
}

// primitive simulation of non-uniform atmospheric fog

// constructing the displacement amount and normal for water surface geometry
float4 CombineWaterNormal(float3 world_position)//ds
{
    float4 water_normal = float4(0.0, 4.0, 0.0, 0.0);
    float water_miplevel;
    float distance_to_camera;
    float4 texvalue;
    float texcoord_scale = 1.0;
    float height_disturbance_scale = 1.0;
    float normal_disturbance_scale = 1.0;
    float2 tc;
    float2 variance = { 1.0, 1.0 };

	// calculating MIP level for water texture fetches
    distance_to_camera = length(CameraPositionDS - world_position);
    water_miplevel = CalculateMIPLevelForDisplacementTextures(distance_to_camera) / 2.0 - 2.0;
    tc = (world_position.xz * WaterBumpTexcoordScale / HeightFieldSize);

	// fetching water heightmap
    [unroll(5)]
    for (float i = 0; i < 5; i++)
    {
        texvalue = WaterBumpTextureDS.SampleLevel(SamplerLinearWrapDS, tc * texcoord_scale + WaterBumpTexcoordShiftDS * 0.03 * variance, water_miplevel).rbga;
        variance.x *= -1.0;
        water_normal.xz += (2 * texvalue.xz - float2(1.0, 1.0)) * normal_disturbance_scale;
        water_normal.w += (texvalue.w - 0.5) * height_disturbance_scale;
        texcoord_scale *= 1.4;
        height_disturbance_scale *= 0.65;
        normal_disturbance_scale *= 0.65;
    }
    water_normal.w *= WaterHeightBumpScale;
    return float4(normalize(water_normal.xyz), water_normal.w);
}

// constructing water surface normal for water refraction caustics
float3 CombineSimplifiedWaterNormal(float3 world_position, float mip_level)//ps
{
    float3 water_normal = float3(0.0, 4.0, 0.0);

    float water_miplevel;
    float distance_to_camera;
    float4 texvalue;
    float texcoord_scale = 1.0;
    float normal_disturbance_scale = 1.0;
    float2 tc;
    float2 variance = { 1.0, 1.0 };

    tc = (world_position.xz * WaterBumpTexcoordScale / HeightFieldSize);
	
	// need more high frequensy details for caustics, so summing more "octaves"
    [unroll(8)]
    for (float i = 0; i < 8; i++)
    {
        texvalue = WaterBumpTexture.SampleLevel(SamplerLinearWrapPS, tc * texcoord_scale + WaterBumpTexcoordShiftPS * 0.03 * variance, mip_level /*+i*/).rbga;
        variance.x *= -1.0;
        water_normal.xz += (2 * texvalue.xz - float2(1, 1)) * normal_disturbance_scale;
        texcoord_scale *= 1.4;
        normal_disturbance_scale *= 0.85;
    }
    return normalize(water_normal);
}

// calculating water refraction caustics intensity
float CalculateWaterCausticIntensity(float3 worldpos)//ds
{

    float distance_to_camera = length(CameraPositionDS - worldpos);

    float2 refraction_disturbance;
    float3 n;
    float m = 0.2;
    float cc = 0;
    float k = 0.15;
    float water_depth = 0.5 - worldpos.y;

    float3 pixel_to_light_vector = normalize(LightPositionDS - worldpos);

    worldpos.xz -= worldpos.y * pixel_to_light_vector.xz;
    float3 pixel_to_water_surface_vector = pixel_to_light_vector * water_depth;
    float3 refracted_pixel_to_light_vector;

	// tracing approximately refracted rays back to light
    [unroll(6)]
    for (float i = -3; i <= 3; i += 1)
        [unroll(6)]
        for (float j = -3; j <= 3; j += 1)
        {
            n = 2.0f * WaterNormalMapTexture.SampleLevel(SamplerLinearWrapDS, (worldpos.xz - CameraPositionDS.xz - float2(200.0, 200.0) + float2(i * k, j * k) * m * water_depth) / 400.0, 0).rgb - float3(1.0f, 1.0f, 1.0f);
            refracted_pixel_to_light_vector = m * (pixel_to_water_surface_vector + float3(i * k, 0, j * k)) - 0.5 * float3(n.x, 0, n.z);
            cc += 0.05 * max(0, pow(max(0, dot(normalize(refracted_pixel_to_light_vector), normalize(pixel_to_light_vector))), 500.0f));
        }
    return cc;
}


float GetRefractionDepth(float2 position)//ps
{
    return RefractionDepthTextureResolved.SampleLevel(SamplerLinearClamp, position, 0).r;
}

float GetConservativeRefractionDepth(float2 position)//ps
{
    float result = RefractionDepthTextureResolved.SampleLevel(SamplerPointClamp, position + 2.0 * float2(ScreenSizeInv.x, ScreenSizeInv.y), 0).r;
    result = min(result, RefractionDepthTextureResolved.SampleLevel(SamplerPointClamp, position + 2.0 * float2(ScreenSizeInv.x, -ScreenSizeInv.y), 0).r);
    result = min(result, RefractionDepthTextureResolved.SampleLevel(SamplerPointClamp, position + 2.0 * float2(-ScreenSizeInv.x, ScreenSizeInv.y), 0).r);
    result = min(result, RefractionDepthTextureResolved.SampleLevel(SamplerPointClamp, position + 2.0 * float2(-ScreenSizeInv.x, -ScreenSizeInv.y), 0).r);
    return result;
}



//--------------------------------------------------------------------------------------
// Heightfield shaders
//--------------------------------------------------------------------------------------

HSIn_Heightfield PassThroughVS(float4 PatchParams : PATCH_PARAMETERS)
{
    HSIn_Heightfield output;
    output.origin = PatchParams.xy;
    output.size = PatchParams.zw;
    return output;
}
PatchData PatchConstantHS(InputPatch<HSIn_Heightfield, 1> inputPatch)
{
    PatchData output;

    float distance_to_camera;
    float tesselation_factor;
    float inside_tessellation_factor = 0;
    float in_frustum = 0;

    output.origin = inputPatch[0].origin;
    output.size = inputPatch[0].size;

    float2 texcoord0to1 = (inputPatch[0].origin + inputPatch[0].size / 2.0) / HeightFieldSize;
    texcoord0to1.y = 1 - texcoord0to1.y;
	
	// conservative frustum culling
    float3 patch_center = float3(inputPatch[0].origin.x + inputPatch[0].size.x * 0.5,  HeightfieldTextureHS.SampleLevel(SamplerLinearWrapHS, texcoord0to1, 0).w, inputPatch[0].origin.y + inputPatch[0].size.y * 0.5);
    float3 camera_to_patch_vector = patch_center - CameraPositionHS;
    float3 patch_to_camera_direction_vector = CameraDirectionHS * dot(camera_to_patch_vector, CameraDirectionHS) - camera_to_patch_vector;
    float3 patch_center_realigned = patch_center + normalize(patch_to_camera_direction_vector) * min(2 * inputPatch[0].size.x, length(patch_to_camera_direction_vector));
    float4 patch_screenspace_center = mul(float4(patch_center_realigned, 1.0), ModelViewProjectionMatrixHS);

    [flatten]
    if (((patch_screenspace_center.x / patch_screenspace_center.w > -1.0) && (patch_screenspace_center.x / patch_screenspace_center.w < 1.0)
		&& (patch_screenspace_center.y / patch_screenspace_center.w > -1.0) && (patch_screenspace_center.y / patch_screenspace_center.w < 1.0)
		&& (patch_screenspace_center.w > 0)) || (length(patch_center - CameraPositionHS) < 2 * inputPatch[0].size.x))
    {
        in_frustum = 1;
    }
    
  [flatten]
    if ((in_frustum == false))
    {
        output.Edges[0] = -1;
        output.Edges[1] = -1;
        output.Edges[2] = -1;
        output.Edges[3] = -1;
        output.Inside[0] = -1;
        output.Inside[1] = -1;
        return output;
    }
    

    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(0, inputPatch[0].size.y * 0.5));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[0] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;


    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(inputPatch[0].size.x * 0.5, 0));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[1] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;

    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(inputPatch[0].size.x, inputPatch[0].size.y * 0.5));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[2] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;

    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(inputPatch[0].size.x * 0.5, inputPatch[0].size.y));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[3] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;
    output.Inside[0] = output.Inside[1] = inside_tessellation_factor * 0.25;
    return output;
}
[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(1)]
[patchconstantfunc("PatchConstantHS")]
DUMMY PatchHS(InputPatch<HSIn_Heightfield, 1> inputPatch)
{
    return (DUMMY) 0;
}

[domain("quad")]
PSIn_Diffuse HeightFieldPatchDS(PatchData input,
                                    float2 uv : SV_DomainLocation,
                                    OutputPatch<DUMMY, 1> inputPatch)
{
    PSIn_Diffuse output;
    float3 vertexPosition;
    float4 base_texvalue;
    float2 texcoord0to1 = (input.origin + uv * input.size) / HeightFieldSize;
    float3 base_normal;
    float3 detail_normal;
    float3 detail_normal_rotated;
    float4 detail_texvalue;
    float detail_height;
    float3x3 normal_rotation_matrix;
    float4 layerdef;
    float distance_to_camera;
    float detailmap_miplevel;
    texcoord0to1.y = 1 - texcoord0to1.y;
	
	// fetching base heightmap,normal and moving vertices along y axis
    base_texvalue = HeightfieldTextureDS.SampleLevel(SamplerLinearWrapDS, texcoord0to1, 0);
    base_normal = base_texvalue.xyz;
    base_normal.z = -base_normal.z;
    vertexPosition.xz = input.origin + uv * input.size;
    vertexPosition.y = base_texvalue.w ;

	// calculating MIP level for detail texture fetches
    distance_to_camera = length(CameraPositionDS - vertexPosition);
    detailmap_miplevel = CalculateMIPLevelForDisplacementTextures(distance_to_camera); //log2(1+distance_to_camera*3000/(HeightFieldSize*TessFactor));
	
	// fetching layer definition texture
    layerdef = LayerdefTexture.SampleLevel(SamplerLinearWrapDS, texcoord0to1, 0);
	
	// default detail texture
    detail_texvalue = SandBumpTexture.SampleLevel(SamplerLinearWrapDS, texcoord0to1 * SandBumpTexcoordScale, detailmap_miplevel).rbga;
    detail_normal = normalize(2 * detail_texvalue.xyz - float3(1, 0, 1));
    detail_height = (detail_texvalue.w - 0.5) * SandBumpHeightScale;

	// rock detail texture
    detail_texvalue = RockBumpTexture.SampleLevel(SamplerLinearWrapDS, texcoord0to1 * RockBumpTexcoordScale, detailmap_miplevel).rbga;
    detail_normal = lerp(detail_normal, normalize(2 * detail_texvalue.xyz - float3(1, 1.4, 1)), layerdef.w);
    detail_height = lerp(detail_height, (detail_texvalue.w - 0.5) * RockBumpHeightScale, layerdef.w);

	// moving vertices by detail height along base normal
    vertexPosition += base_normal * detail_height;

	//calculating base normal rotation matrix
    normal_rotation_matrix[1] = base_normal;
    normal_rotation_matrix[2] = normalize(cross(float3(-1.0, 0.0, 0.0), normal_rotation_matrix[1]));
    normal_rotation_matrix[0] = normalize(cross(normal_rotation_matrix[2], normal_rotation_matrix[1]));

	//applying base rotation matrix to detail normal
    detail_normal_rotated = mul(detail_normal, normal_rotation_matrix);

	//adding refraction caustics
    float cc = 0;
	[flatten]
    if ((SkipCausticsCalculationDS == 0)) // doing it only for main
    {
        cc = CalculateWaterCausticIntensity(vertexPosition.xyz);
        // fading caustics out at distance
        cc *= (200.0 / (200.0 + distance_to_camera));
        cc *= min(1, max(0, -WaterHeightBumpScale - vertexPosition.y));
    }
	 


	// writing output params
    output.position = mul(float4(vertexPosition, 1.0), ModelViewProjectionMatrixDS);
    output.texcoord = texcoord0to1 * DiffuseTexcoordScale;
    output.normal = detail_normal_rotated;
    output.positionWS = float4(vertexPosition, cc);
    output.layerdef = float4(detail_height, layerdef.gba);
    return output;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_Target0;
    float4 Specular : SV_Target1;
    float4 Normal : SV_Target2;
};

PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 Normal, float metallic, float roughness, float terrainMask = 1)
{
    PS_GBUFFER_OUT Out;
    Out.ColorSpecInt = float4(BaseColor.rgb, terrainMask);
    Out.Specular = float4(roughness, metallic, 0.0, 0.0);
    Out.Normal = float4(normalize(Normal.rgb + 0.5 + 0.5), 1.0);
    return Out;
}

PS_GBUFFER_OUT HeightFieldPatchPacking(PSIn_Diffuse input)
{
    float3 color;
    float3 pixel_to_light_vector = normalize(LightPositionPS - input.positionWS.xyz);
    float3 pixel_to_eye_vector = normalize(CameraPositionPS - input.positionWS.xyz);
    float3 microbump_normal;
    float3x3 normal_rotation_matrix;
    
    clip((input.positionWS.y - (-30.0f * 2.0f)));

	// fetching default microbump normal
    microbump_normal = normalize(2 * SandMicroBumpTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rbg - float3(1.0, 1.0, 1.0));
    microbump_normal = normalize(lerp(microbump_normal, 2 * RockMicroBumpTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rbg - float3(1.0, 1.0, 1.0), input.layerdef.w));

	//calculating base normal rotation matrix
    normal_rotation_matrix[1] = input.normal;
    normal_rotation_matrix[2] = normalize(cross(float3(-1.0, 0.0, 0.0), normal_rotation_matrix[1]));
    normal_rotation_matrix[0] = normalize(cross(normal_rotation_matrix[2], normal_rotation_matrix[1]));
    microbump_normal = mul(microbump_normal, normal_rotation_matrix);

	// getting diffuse color
    color = SlopeDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rgb;
    color = lerp(color, SandDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rgb, input.layerdef.g * input.layerdef.g);
    color = lerp(color, RockDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rgb, input.layerdef.w * input.layerdef.w);
    color = lerp(color, GrassDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rgb, input.layerdef.b);
	// adding per-vertex lighting defined by displacement of vertex 
   color *= 0.5 + 0.5 * min(1.0, max(0.0, input.layerdef.r / 3.0f + 0.5f));
  //  //ndotl
    color.rgb *= max(0, dot(pixel_to_light_vector, microbump_normal));
  // 	// adding light from the sky
    color.rgb += (0.0 + 0.2 * max(0, (dot(float3(0, 1, 0), microbump_normal)))) * float3(0.1, 0.1, 0.2);
  //  // making all a bit brighter, simultaneously pretending the wet surface is darker than normal;
   color.rgb *= 0.5 + 0.8 * max(0, min(1, input.positionWS.y * 0.5 + 0.5));
 	//// applying refraction caustics
    color.rgb *= (1.0 + max(0, 0.4 + 0.6 * dot(pixel_to_light_vector, microbump_normal)) * input.positionWS.a );
    return PackGBuffer(color.rgb, microbump_normal, 0.0, 0.0, 0);
}
float4 HeightFieldPatchPS(PSIn_Diffuse input) : SV_Target //Reflection
{
    
    float3 color;
    float3 pixel_to_light_vector = normalize(LightPositionPS - input.positionWS.xyz);
    float3 pixel_to_eye_vector = normalize(CameraPositionPS - input.positionWS.xyz);
    float3 microbump_normal;
    float3x3 normal_rotation_matrix;
    clip((input.positionWS.y - (-0.5f)));
	// fetching default microbump normal
    microbump_normal = normalize(2 * SandMicroBumpTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rbg - float3(1.0, 1.0, 1.0));
    microbump_normal = normalize(lerp(microbump_normal, 2 * RockMicroBumpTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rbg - float3(1.0, 1.0, 1.0), input.layerdef.w));

	//calculating base normal rotation matrix
    normal_rotation_matrix[1] = input.normal;
    normal_rotation_matrix[2] = normalize(cross(float3(-1.0, 0.0, 0.0), normal_rotation_matrix[1]));
    normal_rotation_matrix[0] = normalize(cross(normal_rotation_matrix[2], normal_rotation_matrix[1]));
    microbump_normal = mul(microbump_normal, normal_rotation_matrix);

	// getting diffuse color
    color = SlopeDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rgb;
    color = lerp(color.rgb, SandDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rgb, input.layerdef.g * input.layerdef.g);
    color = lerp(color.rgb, RockDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rgb, input.layerdef.w * input.layerdef.w);
    color = lerp(color.rgb, GrassDiffuseTexture.Sample(SamplerAnisotropicWrap, input.texcoord).rgb, input.layerdef.b);
	 //ndotl
    color.rgb *= max(0, dot(pixel_to_light_vector, microbump_normal));
    return float4(color.rgb, 1.0f);
}
//--------------------------------------------------------------------------------------
// Water shaders
//--------------------------------------------------------------------------------------


PatchData WaterPatchConstantHS(InputPatch<HSIn_Heightfield, 1> inputPatch)
{
    PatchData output;
   
    float distance_to_camera;
    float tesselation_factor;
    float inside_tessellation_factor = 0;
    float in_frustum = 0;

    output.origin = inputPatch[0].origin;
    output.size = inputPatch[0].size;

    float2 texcoord0to1 = (inputPatch[0].origin + inputPatch[0].size / 2.0) / HeightFieldSize;
    texcoord0to1.y = 1 - texcoord0to1.y;
	
	// conservative frustum culling
    float3 patch_center = float3(inputPatch[0].origin.x + inputPatch[0].size.x * 0.5, 0, inputPatch[0].origin.y + inputPatch[0].size.y * 0.5);
    float3 camera_to_patch_vector = patch_center - CameraPositionHS.xyz;
    float3 patch_to_camera_direction_vector = CameraDirectionHS.xyz * dot(camera_to_patch_vector, CameraDirectionHS) - camera_to_patch_vector;
    float3 patch_center_realigned = patch_center + normalize(patch_to_camera_direction_vector) * min(2 * inputPatch[0].size.x, length(patch_to_camera_direction_vector));
    float4 patch_screenspace_center = mul(float4(patch_center_realigned, 1.0), ModelViewProjectionMatrixHS);

    [flatten]
    if (((patch_screenspace_center.x / patch_screenspace_center.w > -1.0) && (patch_screenspace_center.x / patch_screenspace_center.w < 1.0)
		&& (patch_screenspace_center.y / patch_screenspace_center.w > -1.0) && (patch_screenspace_center.y / patch_screenspace_center.w < 1.0)
		&& (patch_screenspace_center.w > 0)) || (length(patch_center - CameraPositionHS.xyz) < 2 * inputPatch[0].size.x))
    {
        in_frustum = 1;
    }
    
  [flatten]
    if ((in_frustum == false))
    {
        output.Edges[0] = -1;
        output.Edges[1] = -1;
        output.Edges[2] = -1;
        output.Edges[3] = -1;
        output.Inside[0] = -1;
        output.Inside[1] = -1;
        return output;
    }
    

    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(0, inputPatch[0].size.y * 0.5));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[0] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;


    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(inputPatch[0].size.x * 0.5, 0));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[1] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;

    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(inputPatch[0].size.x, inputPatch[0].size.y * 0.5));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[2] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;

    distance_to_camera = length(CameraPositionHS.xz - inputPatch[0].origin - float2(inputPatch[0].size.x * 0.5, inputPatch[0].size.y));
    tesselation_factor = CalculateTessellationFactor(distance_to_camera);
    output.Edges[3] = tesselation_factor;
    inside_tessellation_factor += tesselation_factor;
    output.Inside[0] = output.Inside[1] = inside_tessellation_factor * 0.25;
    return output;
}

[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(1)]
[patchconstantfunc("PatchConstantHS")]
DUMMY WaterPatchHS(InputPatch<HSIn_Heightfield, 1> inputPatch)
{
    return (DUMMY) 0;
}


[domain("quad")]
PSIn_Diffuse WaterPatchDS(PatchData input,
                                    float2 uv : SV_DomainLocation,
                                    OutputPatch<DUMMY, 1> inputPatch)
{
    PSIn_Diffuse output;
    float3 vertexPosition;
    float2 texcoord0to1 = (input.origin + uv * input.size) / HeightFieldSize;
    float4 water_normal;
    float depthmap_scaler;

	// getting rough estimate of water depth from depth map texture 
    depthmap_scaler = DepthMapTexture.GatherGreen(SamplerLinearWrapDS, float2(texcoord0to1.x, 1 - texcoord0to1.y), 0);
	
	// calculating water surface geometry position and normal
    vertexPosition.xz = input.origin + uv * input.size;
    vertexPosition.y = -WaterHeightBumpScale / 2;
    water_normal = CombineWaterNormal(vertexPosition.xyz);

	// fading out displacement and normal disturbance near shores by 60%
    water_normal.xyz = lerp(float3(0, 1, 0), normalize(water_normal.xyz), 0.4 + 0.6 * depthmap_scaler);
    vertexPosition.y += water_normal.w * WaterHeightBumpScale * (0.4 + 0.6 * depthmap_scaler);
    vertexPosition.xz -= (water_normal.xz) * 0.5 * (0.4 + 0.6 * depthmap_scaler);

	// writing output params
    output.position = mul(float4(vertexPosition, 1.0), ModelViewProjectionMatrixDS);
    output.texcoord = texcoord0to1 * WaterMicroBumpTexcoordScale + WaterBumpTexcoordShiftDS * 0.07;
    output.normal = normalize(water_normal.xyz);
    output.positionWS = float4(vertexPosition.xyz, depthmap_scaler);
    return output;
}

PS_GBUFFER_OUT WaterPatchPS(PSIn_Diffuse input) 
{
   
    float4 color;
    float3 pixel_to_light_vector = normalize(LightPositionPS - input.positionWS.xyz);
    float3 pixel_to_eye_vector = normalize(CameraPositionPS - input.positionWS.xyz);
    float3 reflected_eye_to_pixel_vector;
   
   

    float fresnel_factor;
    float diffuse_factor;
    float specular_factor;
   
    float4 refraction_color;
    float4 reflection_color;
    float4 disturbance_eyespace;

    float water_depth;
    float4 water_color;
    float3 microbump_normal;
    float3x3 normal_rotation_matrix;
    microbump_normal = normalize(2 * WaterBumpTexture.Sample(SamplerAnisotropicWrap, input.texcoord - WaterBumpTexcoordShiftPS * 0.2).gbr - float3(1, -8, 1));
    microbump_normal += normalize(2 * WaterBumpTexture.Sample(SamplerAnisotropicWrap, input.texcoord * 0.5 + WaterBumpTexcoordShiftPS * 0.05).gbr - float3(1, -8, 1));

	// calculating base normal rotation matrix
    normal_rotation_matrix[1] = input.normal.xyz;
    normal_rotation_matrix[2] = normalize(cross(float3(0.0, 0.0, -1.0), normal_rotation_matrix[1]));
    normal_rotation_matrix[0] = normalize(cross(normal_rotation_matrix[2], normal_rotation_matrix[1]));

	// applying base normal rotation matrix to high frequency bump normal
  
    microbump_normal = mul(normalize(microbump_normal), normal_rotation_matrix);

	 
	// need more high frequency bumps for plausible water surface, so creating normal defined by 2 instances of same bump texture
   

    float scatter_factor;
  // simulating scattering/double refraction: light hits the side of wave, travels some distance in water, and leaves wave on the other side
	// it's difficult to do it physically correct without photon mapping/ray tracing, so using simple but plausible emulation below
	
	// only the crests of water waves generate double refracted light
    scatter_factor = 2.5 * max(0, input.positionWS.y * 0.25 + 0.25);

	// the waves that lie between camera and light projection on water plane generate maximal amount of double refracted light 
    scatter_factor *= pow(max(0.0, dot(normalize(float3(pixel_to_light_vector.x, 0.0, pixel_to_light_vector.z)), -pixel_to_eye_vector)), 2.0);
	
	// the slopes of waves that are oriented back to light generate maximal amount of double refracted light 
    scatter_factor *= pow(max(0.0, 1.0 - dot(pixel_to_light_vector, microbump_normal)), 8.0);
	
	// water crests gather more light than lobes, so more light is scattered under the crests
    scatter_factor += 1.5 * WaterColorIntensity.y * max(0, input.positionWS.y + 1) *
		// the scattered light is best seen if observing direction is normal to slope surface
		max(0, dot(pixel_to_eye_vector, microbump_normal)) *
		// fading scattered light out at distance and if viewing direction is vertical to avoid unnatural look
		max(0, 1 - pixel_to_eye_vector.y) * (300.0 / (300 + length(CameraPositionPS - input.positionWS.xyz)));
    scatter_factor *= 0.1 + 0.9 * input.positionWS.a;

	// calculating fresnel factor 
    float r = (1.2 - 1.0) / (1.2 + 1.0);
    fresnel_factor = max(0.0, min(1.0, r + (1.0 - r) * pow(1.0 - dot(microbump_normal, pixel_to_eye_vector), 4)));

	// calculating specular factor
    reflected_eye_to_pixel_vector = -pixel_to_eye_vector + 2 * dot(pixel_to_eye_vector, microbump_normal) * microbump_normal;
    specular_factor = fresnel_factor * pow(max(0, dot(pixel_to_light_vector, reflected_eye_to_pixel_vector)), WaterSpecularPower);
  
	// calculating diffuse intensity of water surface itself
    diffuse_factor = WaterColorIntensity.x + WaterColorIntensity.y * max(0, dot(pixel_to_light_vector, microbump_normal));

	// calculating disturbance which has to be applied to planar reflections/refractions to give plausible results
    disturbance_eyespace = mul(float4(microbump_normal.x, 0, microbump_normal.z, 0), ModelViewMatrixPS);

    float2 reflection_disturbance = float2(disturbance_eyespace.x, disturbance_eyespace.z) * 0.03;
    float2 refraction_disturbance = float2(-disturbance_eyespace.x, disturbance_eyespace.y) * 0.05 *
		// fading out reflection disturbance at distance so reflection doesn't look noisy at distance
		(20.0 / (20 + length(CameraPositionPS - input.positionWS.xyz)));
	
	// calculating correction that shifts reflection up/down according to water wave Y position
    float4 projected_waveheight = mul(float4(input.positionWS.x, input.positionWS.y, input.positionWS.z, 1), ModelViewProjectionMatrixPS);
    float waveheight_correction = -0.5 * projected_waveheight.y / projected_waveheight.w;
    projected_waveheight = mul(float4(input.positionWS.x, -0.8, input.positionWS.z, 1), ModelViewProjectionMatrixPS);
    waveheight_correction += 0.5 * projected_waveheight.y / projected_waveheight.w;
    reflection_disturbance.y = max(-0.15, waveheight_correction + reflection_disturbance.y);

	// picking refraction depth at non-displaced point, need it to scale the refraction texture displacement amount according to water depth
     float refraction_depth = GetRefractionDepth(input.position.xy * ScreenSizeInv);
   
    
    //refraction_depth = 0.0f;
    refraction_depth = ZFar * ZNear / (ZFar - refraction_depth * (ZFar - ZNear));
    float4 vertex_in_viewspace = mul(float4(input.positionWS.xyz, 1), ModelViewMatrixPS);
    water_depth = refraction_depth - vertex_in_viewspace.z;
    float nondisplaced_water_depth = water_depth;
		// scaling refraction texture displacement amount according to water depth, with some limit
    refraction_disturbance *= min(2, water_depth);
	// picking refraction depth again, now at displaced point, need it to calculate correct water depth
    refraction_depth = GetRefractionDepth(input.position.xy * ScreenSizeInv + refraction_disturbance);
    refraction_depth = ZFar * ZNear / (ZFar - refraction_depth * (ZFar - ZNear));
    vertex_in_viewspace = mul(float4(input.positionWS.xyz, 1), ModelViewMatrixPS);
    water_depth = refraction_depth - vertex_in_viewspace.z;
	// zeroing displacement for points where displaced position points at geometry which is actually closer to the camera than the water surface
    float conservative_refraction_depth = GetConservativeRefractionDepth(input.position.xy * ScreenSizeInv + refraction_disturbance);
    conservative_refraction_depth = ZFar * ZNear / (ZFar - conservative_refraction_depth * (ZFar - ZNear));
    vertex_in_viewspace = mul(float4(input.positionWS.xyz, 1), ModelViewMatrixPS);
    float conservative_water_depth = conservative_refraction_depth - vertex_in_viewspace.z;
    [flatten]
    if (conservative_water_depth < 0)
    {
        refraction_disturbance = 0;
        water_depth = nondisplaced_water_depth;
    }
    water_depth = max(0, water_depth);
	// getting reflection and refraction color at disturbed texture coordinates
    reflection_color = ReflectionTexture.SampleLevel(SamplerLinearClamp, float2(input.position.x * ScreenSizeInv.x, 1.0 - input.position.y * ScreenSizeInv.y) + reflection_disturbance, 0);
    refraction_color = RefractionTexture.SampleLevel(SamplerLinearClamp, input.position.xy * ScreenSizeInv + refraction_disturbance, 0);
	// calculating water surface color and applying atmospheric fog to it
    water_color = diffuse_factor * float4(WaterDeepColor, 1);
 	// fading fresnel factor to 0 to soften water surface edges
    fresnel_factor *= min(1, water_depth * 5.0);
	// fading refraction color to water color according to distance that refracted ray travels in water 
    refraction_color = lerp(water_color, refraction_color, min(1, 1.0 * exp(-water_depth / 8.0)));
	// combining final water color
    color.rgb = lerp(refraction_color.rgb, reflection_color.rgb, fresnel_factor);
    color.rgb += WaterSpecularIntensity * specular_factor * WaterSpecularColor * fresnel_factor;
    color.rgb += WaterScatterColor * scatter_factor;
        
    return PackGBuffer(color.rgb, microbump_normal, 0.0, 0.0, 0);
  
}

//--------------------------------------------------------------------------------------
// Water normalmap combine shaders
//--------------------------------------------------------------------------------------

PSIn_Quad WaterNormalmapCombineVS(uint VertexId : SV_VertexID)
{
    PSIn_Quad output;

    output.position = float4(QuadVertices[VertexId], 0, 1);
    output.texcoord = QuadTexCoordinates[VertexId];
    
    return output;
}

float4 WaterNormalmapCombinePS(PSIn_Quad input) : SV_Target
{
    float4 color;
    color.rgb = (CombineSimplifiedWaterNormal(CameraPositionPS + float3(input.texcoord.x * 400.0f - 200.0f, 0, input.texcoord.y * 400.0f - 200.0f), 0).rgb + float3(1.0f, 1.0f, 1.0f)) * 0.5f;
    color.a = 0;
    return color;
}

//--------------------------------------------------------------------------------------
// Fullscreen shaders
//--------------------------------------------------------------------------------------

PSIn_Quad FullScreenQuadVS(uint VertexId : SV_VertexID)
{
    PSIn_Quad output;

    output.position = float4(QuadVertices[VertexId], 0, 1);
    output.texcoord = QuadTexCoordinates[VertexId];
    
    return output;
}

float RefractionDepthManualResolvePS1(PSIn_Quad input) : SV_Target
{
    return RefractionDepthTextureMS1.Load(int3(input.position.xy, 0)).r;
}


  