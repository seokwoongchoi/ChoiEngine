
/////////////////////////////////////////////////////////////////////////////
// GBuffer textures and Samplers
/////////////////////////////////////////////////////////////////////////////
Texture2D DepthTexture;
Texture2D ColorSpecIntTexture;
Texture2D NormalTexture;
Texture2D SpecPowTexture;
Texture2D RoughnessTexture;
Texture2D BumpTexture;
Texture2D SsaoTexture;

Texture2D PreintegratedFG;
TextureCube EnvironmentMap;

SamplerState PointSampler;


Texture2DArray CascadeShadowMapTexture;
SamplerComparisonState PCFSampler;
/////////////////////////////////////////////////////////////////////////////
// constants
/////////////////////////////////////////////////////////////////////////////
cbuffer cbGBufferUnpack 
{
  float4 PerspectiveValues;
  matrix ViewInv;
 
}
float3 EyePosition()
{
    return ViewInv._41_42_43;
}


cbuffer cbFog
{
	float3 FogColor;
	float FogStartDepth;
	float3 FogHighlightColor;
	float FogGlobalDensity;
	float3 FogSunDir;
	float FogStartHeight;
}

static const float2 g_SpecPowerRange = { 10.0, 250.0 };


struct LightDesc
{
    float4 Ambient;
    float4 Specular;

    float3 Direction;
    float Padding;

    float3 Position;
    float Padding2;

    matrix ToShadowSpace;
    float4 ToCascadeOffsetX;
    float4 ToCascadeOffsetY;
    float4 ToCascadeScale;

    
};

cbuffer CB_Light
{
    LightDesc GlobalLight;
};


float3 DecodeNormal(float2 encodedNormal)
{
    float4 decodedNormal = encodedNormal.xyyy * float4(2,2,0,0) + float4(-1,-1,1,-1);
    decodedNormal.z = dot(decodedNormal.xyz, -decodedNormal.xyw);
    decodedNormal.xy *= sqrt(decodedNormal.z);
    return decodedNormal.xyz * 2.0 + float3(0.0, 0.0, -1.0);
}

float ConvertZToLinearDepth(float depth)
{
	float linearDepth = PerspectiveValues.z / (depth + PerspectiveValues.w);
	return linearDepth;
}

float3 CalcWorldPos(float2 csPos, float depth)
{
	float4 position;

	position.xy = csPos.xy * PerspectiveValues.xy * depth;
	position.z = depth;
	position.w = 1.0;
	
	return mul(position, ViewInv).xyz;
}

struct SURFACE_DATA
{
	float LinearDepth;
	float3 Color;
	float3 Normal;
	float4 SpecPow;
    float3 bump;
    float3 roughness;
	
};

SURFACE_DATA UnpackGBuffer(float2 UV)
{
	SURFACE_DATA Out;

	float depth = DepthTexture.Sample( PointSampler, UV.xy ).x;
	Out.LinearDepth = ConvertZToLinearDepth(depth);
	float4 baseColorSpecInt = ColorSpecIntTexture.Sample( PointSampler, UV.xy );
	Out.Color = baseColorSpecInt.xyz;

    float4 normal = NormalTexture.Sample(PointSampler, UV.xy);
    Out.Normal = normal.xyz;
	Out.Normal = normalize(Out.Normal * 2.0 - 1.0);
    Out.bump = BumpTexture.Sample(PointSampler, UV.xy).xyz;
	Out.SpecPow = SpecPowTexture.Sample( PointSampler, UV.xy );
    Out.roughness = RoughnessTexture.Sample(PointSampler, UV.xy);
	return Out;
}

SURFACE_DATA UnpackGBuffer_Loc(int2 location)
{
	SURFACE_DATA Out;
	int3 location3 = int3(location, 0);

	float depth = DepthTexture.Load(location3).x;
	Out.LinearDepth = ConvertZToLinearDepth(depth);
	float4 baseColorSpecInt = ColorSpecIntTexture.Load(location3);
	Out.Color = baseColorSpecInt.xyz;
	
    float4 normal = NormalTexture.Load(location3);
    Out.Normal = normal.xyz;
    Out.Normal = normalize(Out.Normal * 2.0 - 1.0);
    Out.bump = BumpTexture.Load(location3).xyz;
    Out.bump = normalize(Out.bump * 2.0 - 1.0);
	Out.SpecPow = SpecPowTexture.Load(location3);
    Out.roughness = RoughnessTexture.Load(location3).xyz;
	return Out;
}

struct Material
{
   float4 Ambient;
   float3 normal;
   float4 diffuseColor;
   float4 specPow;
    float3 bump;
    float3 roughness;
};


void MaterialFromGBuffer(SURFACE_DATA gbd, inout Material mat)
{
    mat.Ambient = float4(1, 1, 1, 1);
	mat.normal = gbd.Normal;
	mat.diffuseColor.xyz = gbd.Color;
	mat.diffuseColor.w = 1.0; // Fully opaque
    mat.specPow = gbd.SpecPow;
    mat.bump = gbd.bump;
    mat.roughness = gbd.roughness;
}

float4 DebugLightPS() : SV_TARGET
{
	return float4(1.0, 1.0, 1.0, 1.0);
}

//////////////////////////////////pointlight Desc///////////////////////////////////////

struct PointLightDesc
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;

    float3 Position;
    float Range;
    
    float Intensity;
    float3 Padding;

   
};

cbuffer CB_PointLights
{
    matrix LightProjection;
    PointLightDesc PointLight;
};

//////////////////////////////////////////////////////////////////////////////