
/////////////////////////////////////////////////////////////////////////////
// GBuffer textures and Samplers
/////////////////////////////////////////////////////////////////////////////
Texture2D DepthTexture : register(t0);
Texture2D ColorSpecIntTexture : register(t1);
Texture2D SpecPowTexture : register(t2);
Texture2D NormalTexture : register(t3);
Texture2D<float4> SsaoTexture : register(t4);
Texture2D skyTexture;
Texture2D PreintegratedFG;

TextureCube skyIR;
TextureCube PreviewskyIR;

SamplerState PointSampler;
SamplerState AnisoClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    //AddressU = WRAP;
    //AddressV = WRAP;
    //Filter = ANISOTROPIC;
    AddressU = CLAMP;
    AddressV = CLAMP;
};


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
float3 ViewPosition()
{
    return ViewInv._41_42_43;
}

struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_Target0;
    float4 Specular : SV_Target1;
    float4 Normal : SV_Target2;
  
    
      
};
struct MaterialDesc
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;

    float Roughness;
    float Matallic;
};

cbuffer CB_Material
{
    MaterialDesc Mat;
};

float3 EyePosition()
{
    return ViewInv._41_42_43;
}


cbuffer cbFog
{
	float3 FogColor;
    float FogStartDist;
	float3 FogHighlightColor;
	float FogGlobalDensity;
	
    float FogHeightFalloff;
}

//static const float2 g_SpecPowerRange = { 10.0, 250.0 };


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

   // float2 mousePos;
    
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
float4 CalcWorldPosCUBE(float2 csPos, float depth)
{
    float4 position;

    position.xy = csPos.xy * PerspectiveValues.xy * depth;
    position.z = depth;
    position.w = 1.0;
	
    return mul(position, ViewInv);
}






struct Material
{  
    
   float4 diffuseColor;

   float3 Specular;
   float TerrainMask;

   float3 normal;
   float LinearDepth;

     
   float roughness;
   float metallic;
};


Material UnpackGBuffer(int2 location)
{
    Material Out;
    int3 location3 = int3(location, 0);
  
     float4 baseColorSpecInt = ColorSpecIntTexture.Load(location3);
    Out.diffuseColor = float4(baseColorSpecInt.xyz, 1);
    
    float depth = DepthTexture.Load(location3).x;
    Out.LinearDepth = ConvertZToLinearDepth(depth);

    float4 normal = NormalTexture.Load(location3);
    Out.normal = normal.xyz;
    Out.normal = normalize(Out.normal * 2.0 - 1.0);
 
    float4 specular = SpecPowTexture.Load(location3);
    Out.Specular = specular.xyz;
    Out.TerrainMask = specular.w;
 
    
    Out.metallic = baseColorSpecInt.w;
    Out.roughness = normal.w;
    return Out;
}




float4 DebugLightPS() : SV_TARGET
{
	return float4(1.0, 1.0, 1.0, 1.0);
}

//////////////////////////////////pointlight Desc///////////////////////////////////////

struct PointLightDesc
{
  
    float4 Diffuse;
    float3 Specular;
 

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

//////////////////////////////////spotlight Desc///////////////////////////////////////



cbuffer CB_SpotDomain
{
   
    matrix SpotLightProjection;
    float fSinAngle;
    float fCosAngle;

    //float Padding1;
   // float Padding2;
};

cbuffer CB_SpotPixel
{
    float4 SpotDiffuse;
    float4 SpotSpecular;
    float3 SpotPosition;
    float SpotRange;

    float3 SpotvDirToLight; //-dir

    float SpotIntensity;

    float SpotCosConeAttRange;
    float SpotCosOuterCone;
   // float pixelPadding1;
   // float pixelPadding2;
};



//////////////////////////////////////////////////////////////////////////////

float3 ApplyFog(float3 originalColor, float eyePosY, float3 eyeToPixel)
{
    float pixelDist = length(eyeToPixel);
    float3 eyeToPixelNorm = eyeToPixel / pixelDist;

	// Find the fog staring distance to pixel distance
    float fogDist = max(pixelDist - FogStartDist, 0.0);

	// Distance based fog intensity
    float fogHeightDensityAtViewer = exp(-FogHeightFalloff * eyePosY);
    float fogDistInt = fogDist * fogHeightDensityAtViewer;
   // float fogDistInt = FogStartDist * fogHeightDensityAtViewer;

	// Height based fog intensity
    float eyeToPixelY = eyeToPixel.y * (fogDist / pixelDist);
    float t = FogHeightFalloff * eyeToPixelY;
    const float thresholdT = 0.01;
    float fogHeightInt = abs(t) > thresholdT ?
		(1.0 - exp(-t)) / t : 1.0;

	// Combine both factors to get the final factor
    float fogFinalFactor = exp(-FogGlobalDensity * fogDistInt * fogHeightInt);

	// Find the sun highlight and use it to blend the fog color
    float sunHighlightFactor = saturate(dot(eyeToPixelNorm, -GlobalLight.Direction));
    sunHighlightFactor = pow(sunHighlightFactor, 8.0);
    float3 fogFinalColor = lerp(FogColor, FogHighlightColor, sunHighlightFactor);

    return lerp(fogFinalColor, originalColor, fogFinalFactor);
}

float3 RealAlbedo(float3 diffuse, float metallic)
{
    float3 g_diffuse = pow(diffuse, 2.2f);
    float3 g_lightColor = GlobalLight.Specular;
 
    float3 realAlbedo = g_diffuse - (g_diffuse * metallic);
    realAlbedo = saturate(realAlbedo);
    
    return realAlbedo;

}

float3 RealSpecularColor(float3 diffuse,float metallic)
{
    float3 g_diffuse = pow(diffuse, 2.2f);
    float3 realSpecularColor = lerp(0.03f, g_diffuse,metallic);
    
    return realSpecularColor;

}
float NdotL(float3 bump,float3 lightDir)
{
    return saturate(dot(bump, lightDir));
}

float NdotH(float3 bump,float3 H)
{
    return saturate(dot(bump, H));
}

float NdotV(float3 bump, float3 viewDir)
{
    return saturate(dot(bump, viewDir));
}

float VdotH(float3 H, float3 viewDir)
{
    return saturate(dot(viewDir, H));
}

float LdotH(float3 H, float3 lightDir)
{
    return saturate(dot(lightDir, H));
}

float3 F0(float3 diffuse,float metallic)
{
    float3 g_diffuse = pow(diffuse, 2.2f).rgb;
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, g_diffuse, metallic);
    
    return F0;
}

float Alpha(float roughness)
{
    float alpha = max(0.001f, roughness * roughness);
    return alpha;
}


PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float4 Specular, float3 Normal, float metallic, float roughness,float terrainMask = 1)
{
    PS_GBUFFER_OUT Out;


    Out.ColorSpecInt = float4(BaseColor.rgb, metallic);
    Out.Specular = float4(Specular.rgb, terrainMask);
    
    Out.Normal = float4(Normal.rgb * 0.5 + 0.5, roughness);
 
    
    return Out;
}