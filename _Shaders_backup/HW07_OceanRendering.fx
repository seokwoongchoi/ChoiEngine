#include "000_Header.fx"
#include "000_Light.fx"

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

#define PATCH_BLEND_BEGIN		10
#define PATCH_BLEND_END			15000

struct ShadingDesc
{
    // Water-reflected sky color
    float4 SkyColor;
	// The color of bottomless water body
    float4 WaterbodyColor;
	// The strength, direction and color of sun streak
    float4 SunColor;

    float3 SunDir;
    float Shineness;

	// The parameter is used for fixing an artifact
    float3 BendParam;
	// Perlin noise for distant wave crest
	// Constants for calculating Texcoord from position
    float PerlinSize;
    
    float3 PerlinAmplitude;
    float TexelLength_x2;

    float3 PerlinOctave;
    float UVScale;
    
    float3 PerlinGradient;
    float UVOffset;

};

// Shading parameters
cbuffer CB_Shading
{
    ShadingDesc Shading;
};


struct OceanLODDesc
{
    float MinDistance;
    float MaxDistance;
    float MinTessellation;
    float MaxTessellation;
    
    float2 HalfOceanSize;
    float2 PerlinMovement;
};

cbuffer CB_Ocean
{
    OceanLODDesc Lod;
};

struct Vertexinput_Lod
{
    float4 Position : Position0;
    float2 Uv : Uv0;
};

struct VertexOutput_Lod
{
    float4 Position : Position0;
    float2 Uv : Uv0;
};

struct ConstantHullOutput_Lod
{
    float Edge[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};


struct HullOutput_Lod
{
    float4 Position : Position0;
    float2 Uv : Uv0;
};

struct DomainOutput_Lod
{
    float4 Position : SV_Position0;
   // float2 Uv : Uv0;
    float2 TiledUv : Uv0; //offset을 통해 몇장 반복하는지를 정하는 uv?     
    float3 wPosition : Uv1;
};

float TessFactor(float3 position)
{
    //테셀 될 정점의 위치?
    //position.y = 0.0f;
    float3 view = ViewPosition();
    //view.y = 0.0f;
    float d = distance(position, view);
    float s = saturate((d - Lod.MinDistance) / (Lod.MaxDistance - Lod.MinDistance));

    s = int(s * 16.0f) / 16.0f;

    return lerp(Lod.MaxTessellation, Lod.MinTessellation, s);
}
//-----------------------------------------------------------------------------------
// Texture & Samplers
//-----------------------------------------------------------------------------------
Texture2D TexPerlin;        // FFT wave gradient map in PS
Texture2D TexDisplacement;  // FFT wave displacement map in VS
Texture2D TexGradient;      // Perlin wave displacement & gradient map in both VS & PS
Texture1D TexFresnel;       // Fresnel factor lookup table
TextureCube TexReflectCube; // A small skybox cube Texture for reflection

//-----------------------------------------------------------------------------
// Name: OceanSurfVS
// Type: VerTex shader                                      
// Desc: Ocean shading verTex shader. Check SDK document for more details
//-----------------------------------------------------------------------------


VertexOutput_Lod OceanSurfVS(Vertexinput_Lod input)
{
    VertexOutput_Lod output;
    output.Position = input.Position;
    output.Uv = input.Uv;
    
    return output;
}

//                                                             ↓ controll_point
ConstantHullOutput_Lod OceanSurfHS_Constant(InputPatch<VertexOutput_Lod, 4> input)
{

    ConstantHullOutput_Lod output;
   
    float3 e0 = (input[0].Position + input[2].Position).xyz * 0.5f;
    float3 e1 = (input[0].Position + input[1].Position).xyz * 0.5f;
    float3 e2 = (input[1].Position + input[3].Position).xyz * 0.5f;
    float3 e3 = (input[2].Position + input[3].Position).xyz * 0.5f;

    output.Edge[0] = TessFactor(e0);
    output.Edge[1] = TessFactor(e1);
    output.Edge[2] = TessFactor(e2);
    output.Edge[3] = TessFactor(e3);

    float3 c = (input[0].Position + input[1].Position + input[2].Position + input[3].Position).xyz;
    c *= 0.25f;
    output.inside[0] = TessFactor(c);
    output.inside[1] = TessFactor(c);

    return output;
}


[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("OceanSurfHS_Constant")]
[maxtessfactor(64)] //최대 정해주는게 빠르다?
HullOutput_Lod OceanSurfHS(InputPatch<VertexOutput_Lod, 4> input, uint pointID : SV_OutputControlPointID)
{
    HullOutput_Lod output;
    output.Position = input[pointID].Position;
    output.Uv = input[pointID].Uv;
    return output;
}

[domain("quad")]
DomainOutput_Lod OceanSurfDS(ConstantHullOutput_Lod input, const OutputPatch<HullOutput_Lod, 4> patch, float2 uv : SV_DomainLocation)
{
    DomainOutput_Lod output;
    float3 p0 = lerp(patch[0].Position, patch[1].Position, uv.x).xyz;
    float3 p1 = lerp(patch[2].Position, patch[3].Position, uv.x).xyz;

    float3 position = lerp(p0, p1, uv.y);

    float2 uv0 = lerp(patch[0].Uv, patch[1].Uv, uv.x);
    float2 uv1 = lerp(patch[2].Uv, patch[3].Uv, uv.x);

    
    

	// UV - 스케일 만큼 반복, offset은 텍스쳐의 시작 위치
    float2 uv_local = abs(position.xz + Lod.HalfOceanSize) * Shading.UVScale + Shading.UVOffset;

	// Blend displacement to avoid tiling artifact
    float3 eye_vec = ViewPosition() - position;
    float dist_2d = length(eye_vec.xz);
    float blend_factor = (PATCH_BLEND_END - dist_2d) / (PATCH_BLEND_END - PATCH_BLEND_BEGIN);
    blend_factor = clamp(blend_factor, 0, 1);

	// Add perlin noise to distant patches
    float perlin = 0;
    if (blend_factor < 1)
    {
        float2 perlin_tc = uv_local * Shading.PerlinSize;
        float perlin_0 = TexPerlin.SampleLevel(PerlinSampler, perlin_tc * Shading.PerlinOctave.x + Lod.PerlinMovement, 0).w;
        float perlin_1 = TexPerlin.SampleLevel(PerlinSampler, perlin_tc * Shading.PerlinOctave.y + Lod.PerlinMovement, 0).w;
        float perlin_2 = TexPerlin.SampleLevel(PerlinSampler, perlin_tc * Shading.PerlinOctave.z + Lod.PerlinMovement, 0).w;
		
        perlin = perlin_0 * Shading.PerlinAmplitude.x + perlin_1 * Shading.PerlinAmplitude.y + perlin_2 * Shading.PerlinAmplitude.z;
    }

	// Displacement map
    float3 displacement = 0;
    if (blend_factor > 0)
        displacement = TexDisplacement.SampleLevel(HeightSampler, uv_local, 0).xyz;
    displacement = lerp(float3(0, 0, perlin), displacement, blend_factor);
    position.xzy += displacement;
    output.Position = WorldPosition(float4(position, 1));
    output.Position = ViewProjection(output.Position);
    //output.Uv = uv_local;
    output.TiledUv = uv_local;
    output.wPosition = position.xzy;
    return output;
}

float4 OceanSurfPS(DomainOutput_Lod input) : SV_Target0
{
     // Calculate eye vector.
    float3 eye_vec = ViewPosition() - input.wPosition;
    float3 eye_dir = normalize(eye_vec);
	

	// --------------- Blend perlin noise for reducing the tiling artifacts

	// Blend displacement to avoid tiling artifact
    float dist_2d = length(eye_vec.xz);
    float blend_factor = (PATCH_BLEND_END - dist_2d) / (PATCH_BLEND_END - PATCH_BLEND_BEGIN);
    blend_factor = clamp(blend_factor * blend_factor, 0, 1);

	// Compose perlin waves from three octaves
    float2 perlin_tc = input.TiledUv * Shading.PerlinSize;
    float2 perlin_tc0 = (blend_factor < 1) ? perlin_tc * Shading.PerlinOctave.x + Lod.PerlinMovement : 0;
    float2 perlin_tc1 = (blend_factor < 1) ? perlin_tc * Shading.PerlinOctave.y + Lod.PerlinMovement : 0;
    float2 perlin_tc2 = (blend_factor < 1) ? perlin_tc * Shading.PerlinOctave.z + Lod.PerlinMovement : 0;

    float2 perlin_0 = TexPerlin.Sample(PerlinSampler, perlin_tc0).xy;
    float2 perlin_1 = TexPerlin.Sample(PerlinSampler, perlin_tc1).xy;
    float2 perlin_2 = TexPerlin.Sample(PerlinSampler, perlin_tc2).xy;
	
    float2 perlin = (perlin_0 * Shading.PerlinGradient.x + perlin_1 * Shading.PerlinGradient.y + perlin_2 * Shading.PerlinGradient.z);


	// --------------- Water body color

	// Texcoord mash optimization: Texcoord of FFT wave is not required when blend_factor > 1
    float2 fft_tc = (blend_factor > 0) ? input.TiledUv : 0;

    float2 grad = TexGradient.Sample(GradientSampler, fft_tc).xy;
    grad = lerp(perlin, grad, blend_factor);

	// Calculate normal here.
    float3 normal = normalize(float3(grad, Shading.TexelLength_x2));
	// Reflected ray
    float3 reflect_vec = reflect(-eye_dir, normal);
	// 해당위치의 노멀과 카메라와의 각도
    float cos_angle = dot(normal, eye_dir);

	// A coarse way to handle transmitted light
    float3 body_color = Shading.WaterbodyColor.rgb;


	// --------------- Reflected color

	// ramp.x for fresnel term. ramp.y for sky blending
    float4 ramp = TexFresnel.Sample(FresnelSampler, cos_angle).xyzw;
	// A workaround to deal with "indirect reflection vectors" (which are rays requiring multiple
	// reflections to reach the sky).
    if (reflect_vec.z < Shading.BendParam.x)
        ramp = lerp(ramp, Shading.BendParam.z, (Shading.BendParam.x - reflect_vec.z) / (Shading.BendParam.x - Shading.BendParam.y));
        //ramp = lerp(ramp, 0.2f, (0.1f - reflect_vec.z) / 0.6f);
    reflect_vec.z = max(0, reflect_vec.z);

    float3 reflection = TexReflectCube.Sample(CubeSampler, reflect_vec).xyz;
	// Hack bit: making higher contrast
    reflection = reflection * reflection * 2.5f;

	// Blend with predefined sky color
    float3 reflected_color = lerp(Shading.SkyColor.rgb, reflection, ramp.y);

	// Combine waterbody color and reflected color
    float3 water_color = lerp(body_color, reflected_color, ramp.x);


	// --------------- Sun spots

    float cos_spec = saturate(dot(reflect_vec, Shading.SunDir));
    float sun_spot = pow(cos_spec, Shading.Shineness);
    float att = 1.0f - saturate((PATCH_BLEND_END - dist_2d) / (PATCH_BLEND_END - PATCH_BLEND_BEGIN));
    
    //태양 위치에 따른 색상 감쇄
    float dayatt = clamp(dot(float3(0, 1, 0), -Shading.SunDir), 0.20f, 1);
    water_color += Shading.SunColor.rgb * sun_spot*att* dayatt;
    dayatt = clamp(dayatt, 0.5f, 1);
    water_color *= dayatt;
    
    return float4(water_color, 1);
    //return float4(Shading.SunColor.rgb, 1);
}


//-----------------------------------------------------------------------------
// Name: OceanSurfPS
// Type: Pixel shader                                      
// Desc: Ocean shading pixel shader. Check SDK document for more details
//-----------------------------------------------------------------------------
//float4 OceanSurfPS(VS_OUTPUT In) : SV_Target
//{
	
//}

//-----------------------------------------------------------------------------
// Name: WireframePS
// Type: Pixel shader                                      
// Desc:
//-----------------------------------------------------------------------------
float4 WireframePS() : SV_Target
{
    return float4(0.9f, 0.9f, 0.9f, 1);
}

technique11 T0
{
    pass P0
    {
        SetRasterizerState(NoneRS);
        SetVertexShader(CompileShader(vs_5_0, OceanSurfVS()));
        SetHullShader(CompileShader(hs_5_0, OceanSurfHS()));
        SetDomainShader(CompileShader(ds_5_0, OceanSurfDS()));
        SetPixelShader(CompileShader(ps_5_0, OceanSurfPS()));
    }

    pass P1
    {
        SetRasterizerState(WireFrameRS);
        SetVertexShader(CompileShader(vs_5_0, OceanSurfVS()));
        SetHullShader(CompileShader(hs_5_0, OceanSurfHS()));
        SetDomainShader(CompileShader(ds_5_0, OceanSurfDS()));
        SetPixelShader(CompileShader(ps_5_0, WireframePS()));
    }
}