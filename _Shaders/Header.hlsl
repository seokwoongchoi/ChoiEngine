#define MAX_MODEL_INSTANCE 50

cbuffer CB_World : register(b0)
{
   static matrix World : packoffset(c0);
};

cbuffer CB_View : register(b1)
{
    matrix VP : packoffset(c0);
};

cbuffer CB_Bone : register(b2)
{
    uint BoneIndex : packoffset(c0.x);
   
};

cbuffer CB_ModelInstance : register(b3)
{
    matrix InstTransforms[MAX_MODEL_INSTANCE] : packoffset(c0);
   
};

cbuffer CB_Material : register(b0)
{
    float4 DiffuseFactor : packoffset(c0);
    float4 LightDir : packoffset(c1);
 };
SamplerState LinearSampler : register(s0);


Texture2D DiffuseMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MatallicMap : register(t3);



struct VertexModelOutput
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
    
   // float4 Cull : SV_CullDistance0;
};

///////////////////////////////////////////////////////////////////////////////


float4 WorldPosition(float4 position)
{
    return mul(position, World);
}


float4 ViewProjection(float4 position)
{
    
    return mul(position, VP);
}


float3 WorldNormal(float3 normal)
{
    return mul(normal, (float3x3) World);
}

float3 WorldTangent(float3 tangent)
{
    return mul(tangent, (float3x3) World);
}



///////////////////////////////////////////////////////////////////////////////

struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_Target0;
    float4 Specular : SV_Target1;
    float4 Normal : SV_Target2;
 };

PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 Normal, float metallic, float roughness, float terrainMask = 1)
{
    PS_GBUFFER_OUT Out;
    Out.ColorSpecInt = float4(BaseColor.rgb, metallic);
    Out.Specular = float4(0.0,0.0,0.0, terrainMask);
    Out.Normal = float4(Normal.rgb*0.5+0.5 ,roughness);
    return Out;
}

void GetBumpMapCoord(float2 uv, float3 normal, float3 tangent, inout float3 bump)
{
  

    float3 normalMap = NormalMap.Sample(LinearSampler, uv).rgb;
    [branch]
    if (any(normalMap) == false)
        return;
   
    // Expand the range of the normal value from (0, +1) to (-1, +1).
    normalMap = (normalMap * 2.0f) - 1.0f;
    float3 N = normalize(normal); //Z
    float3 T = normalize(tangent - dot(tangent, N) * N); //X
    float3 BiNor = cross(N, T); //Y
    // Calculate the normal from the data in the bump map.
    bump = (normalMap.x * T) + (normalMap.y * BiNor) + (normalMap.z * N);
	
    // Normalize the resulting bump normal.
   
 
}

PS_GBUFFER_OUT PS(VertexModelOutput In)
{
  
    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv).rgb;
   
    float metallic = MatallicMap.GatherRed(LinearSampler, In.Uv).r;
    float roughness = RoughnessMap.GatherRed(LinearSampler, In.Uv).r;
   
 
    float3 bump = 0;
    GetBumpMapCoord(In.Uv, In.Normal, In.Tangent, bump);
       
    DiffuseColor *= DiffuseFactor.xyz;
 
    roughness *= DiffuseFactor.w;
    metallic *= LightDir.w;
    return PackGBuffer(DiffuseColor, normalize(bump), metallic, roughness);
}


float4 ReflectionPS(VertexModelOutput In) : SV_Target
{
  
    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv).rgb;
    float3 bump = 0;
    GetBumpMapCoord(In.Uv, In.Normal, In.Tangent, bump);
    DiffuseColor *= DiffuseFactor.xyz;
    
     
    float3 lightDir = -LightDir.xyz;
    float ndotl = dot(bump, lightDir);
    
    float3 finalColor = DiffuseColor * ndotl;
    return float4(finalColor, 1.0f);
}


float4 ForwardPS(VertexModelOutput In) : SV_Target
{
    //clip(In.Position.y +0.6f);
    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv).rgb;
  
    return float4(DiffuseColor, 1.0f);
}