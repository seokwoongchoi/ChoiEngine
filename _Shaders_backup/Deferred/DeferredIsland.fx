#define Width 512
#define Factor Width*0.5f
#include "000_Header.fx"
#include "pbrCommon.fx"
#include "000_Terrain.fx"
#include "000_ModelEdit.fx"
//#include "000_ModelAnim.fx" 
//#include "000_ModelTS.fx"

#include "000_States.fx"
#include "DebugLine.fx"
//#include "Island11.fx"
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Mesh //////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

MeshOutput VS_Mesh_PreRender(VertexMesh input)
{
    MeshOutput output = VS_Mesh(input);

   // output.Uv *= 4;
    return output;
}

MeshOutput VS_Model_PreRender(VertexModel input)
{
    MeshOutput output = VS_Model(input);
    return output;
}


MeshOutput VS_Animation_PreRender(VertexModel input)
{
    MeshOutput output = VS_Animation(input);
    return output;
}
/////////////////////PreviewRender////////////////////////////
MeshOutput VS_Mesh_PreviewRender(VertexMesh input)
{
    MeshOutput output = VS_PreviewMesh(input);
    return output;
}

MeshOutput VS_Model_PreviewRender(VertexModel input)
{
    MeshOutput output = VS_PreviewModel(input);
    return output;
}
MeshOutput VS_Animation_PreviewRender(VertexModel input)
{
    MeshOutput output = VS_PreviewAnimation(input);
    return output;
}

MeshOutput VS_Forward_PreviewRender(VertexModel input)
{
    MeshOutput output = VS_PreviewForward(input);
    return output;
}
///////////////////////////////////////////////////////////////////////

void GetBumpMapCoord(float2 uv, float3 normal, float3 tangent, float detailmap_miplevel,inout float3 bump)
{
  //  float4 map = NormalMap.SampleLevel(LinearSampler, uv, detailmap_miplevel);

    float4 map = NormalMap.Sample(LinearSampler, uv);
    [flatten]
    if (any(map) == false)
        return;


    //탄젠트 공간
    float3 N = normalize(normal); //Z
    float3 T = normalize(tangent - dot(tangent, N) * N); //X
    float3 B = cross(N, T); //Y
    float3x3 TBN = float3x3(T, B, N);

    //이미지로부터 노멀벡터 가져오기
    float3 coord = map.rgb * 2.0f - 1.0f;

    //탄젠트 공간으로 변환
    coord = mul(coord, TBN);

    bump = coord;
}

PS_GBUFFER_OUT RenderScenePS(MeshOutput In)
{
   
    // Lookup mesh texture and modulate it with diffuse
    
    //float distance_to_camera = length(ViewPosition() - In.wPosition);
    //float detailmap_miplevel = CalculateMIPLevelForDisplacementTextures(distance_to_camera);
    ////float detailmap_miplevel = 10;
    //float3 DiffuseColor = DiffuseMap.SampleLevel(LinearSampler, In.Uv, detailmap_miplevel);
    //float4 Specular = SpecularMap.SampleLevel(LinearSampler, In.Uv, detailmap_miplevel);
    //float matallic = MatallicMap.SampleLevel(LinearSampler, In.Uv, detailmap_miplevel).r;
    //float roughness = RoughnessMap.SampleLevel(LinearSampler, In.Uv, detailmap_miplevel).r;
    
    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv);
    float4 Specular = SpecularMap.Sample(LinearSampler, In.Uv);
    float matallic = MatallicMap.Sample(LinearSampler, In.Uv).r;
    float roughness = RoughnessMap.Sample(LinearSampler, In.Uv).r;
	//DiffuseColor *= DiffuseColor;
   // DiffuseColor += float4(0.15f, 0.15f, 0.15f, 0.0f);
    float3 bump = 0;
    GetBumpMapCoord(In.Uv, In.Normal, In.Tangent, 0, bump);
   
    DiffuseColor *= Mat.Diffuse;
     Specular *= Mat.Specular;
    
    //[flatten]
    //if (any(Specular))
    //{
    //    finalSpec = Specular;
    //}
    
    
    roughness *= Mat.Roughness;
    matallic *= Mat.Matallic;
    
    return PackGBuffer(DiffuseColor, Specular, normalize(In.Normal), normalize(bump), matallic, roughness);
}



//////////////////////////////// Sky ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

static float PI = 3.14159265f;
static float InnerRadius = 6356.7523142f;
static float OuterRadius = 6356.7523142f * 1.0157313f;
static float KrESun = 0.0025f * 20.0f; //0.0025f - 레일리 상수 * 태양의 밝기
static float KmESun = 0.0010f * 20.0f; //0.0025f - 미 상수 * 태양의 밝기
static float Kr4PI = 0.0025f * 4.0f * 3.1415159;
static float Km4PI = 0.0010f * 4.0f * 3.1415159;
static float2 RayleighMieScaleHeight = { 0.25f, 0.1f };
static float Scale = 1.0 / (6356.7523142 * 1.0157313 - 6356.7523142);
static float g = -0.980f;
static float g2 = -0.980f * -0.980f;
static float Exposure = -2.0f; //노출도

cbuffer CB_Scatter
{
    float3 WaveLength;
    int SampleCount;

    float3 InvWaveLength;
    float StarIntensity;

    float3 WaveLengthMie;
    float MoonAlpha;
    
    float Time;
    float3 pad;
};


struct VertexOutput_Dome
{
    float4 Position : SV_Position0;
    float3 oPosition : Position1;
    float2 Uv : Uv0;
};
float GetRayleighPhase(float c)
{
    return 0.75f * (1.0f + c);
}
float GetMiePhase(float c, float c2)
{
    
   
    float3 result = 0;
    result.x = 1.5f * ((1.0f - g2) / (2.0f + g2));
    result.y = 1.0f + g2;
    result.z = 2.0f * g;

    return result.x * (1.0f + c2) / pow(result.y - result.z * c, 1.5f);
}
float3 HDR(float3 LDR)
{
    
    
    return 1.0f - exp(Exposure * LDR);
}
float HitOuterSphere(float3 position, float3 direction)
{
    
  
    float3 light = -position;

    float b = dot(light, direction);
    float c = dot(light, light);

    float d = c - b * b;
    float q = sqrt(OuterRadius * OuterRadius - d);

    return b + q;
}
float2 GetDensityRatio(float height)
{
   
    
    float altitude = (height - InnerRadius) * Scale;

    return exp(-altitude / RayleighMieScaleHeight);
}
float2 GetDistance(float3 p1, float3 p2)
{
  
    float2 opticalDepth = 0;

    float3 temp = p2 - p1;
    float far = length(temp);
    float3 direction = temp / far;


    float sampleLength = far / SampleCount;
    float scaledLength = sampleLength * Scale;

    float3 sampleRay = direction * sampleLength;
    p1 += sampleRay * 0.5f;
    
    [unroll(8)]
    for (int i = 0; i < SampleCount; i++)
    {
        float height = length(p1);
        opticalDepth += GetDensityRatio(height);

        p1 += sampleRay;
    }

    return opticalDepth * scaledLength;
}
void CalcMieRay(inout float3 rayleigh, inout float3 mie, float2 uv)
{
    

    float3 sunDirection = -normalize(GlobalLight.Direction);

    float3 pointPv = float3(0, InnerRadius + 1e-3f, 0.0f);
    float angleXZ = PI * uv.y;
    float angleY = 100.0f * uv.x * PI / 180.0f;

    float3 direction;
    direction.x = sin(angleY) * cos(angleXZ);
    direction.y = cos(angleY);
    direction.z = sin(angleY) * sin(angleXZ);
    direction = normalize(direction);

    float farPvPa = HitOuterSphere(pointPv, direction);
    float3 ray = direction;

    float3 pointP = pointPv;
    float sampleLength = farPvPa / SampleCount;
    float scaledLength = sampleLength * Scale;
    float3 sampleRay = ray * sampleLength;
    pointP += sampleRay * 0.5f;

    float3 rayleighSum = 0;
    float3 mieSum = 0;
    float3 attenuation = 0;
    [unroll(8)]
    for (int i = 0; i < SampleCount; i++)
    {
        float pHeight = length(pointP);

        float2 densityRatio = GetDensityRatio(pHeight);
        densityRatio *= scaledLength;


        float2 viewerOpticalDepth = GetDistance(pointP, pointPv);

        float farPPc = HitOuterSphere(pointP, sunDirection);
        float2 sunOpticalDepth = GetDistance(pointP, pointP + sunDirection * farPPc);

        float2 opticalDepthP = sunOpticalDepth.xy + viewerOpticalDepth.xy;
         attenuation = exp(-Kr4PI * InvWaveLength * opticalDepthP.x - Km4PI * opticalDepthP.y);

        rayleighSum += densityRatio.x * attenuation;
        mieSum += densityRatio.y * attenuation;

        pointP += sampleRay;
    }

    float3 rayleigh1 = rayleighSum * KrESun;
    float3 mie1 = mieSum * KmESun;

    
   
  
    rayleigh = rayleigh1 * InvWaveLength;
    mie = mie1 * WaveLengthMie;
   
   

}
VertexOutput_Dome VS_Dome(VertexTexture input)
{
    VertexOutput_Dome output;

    output.Position = WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);

    output.oPosition = -input.Position;
    output.Uv = input.Uv;

    return output;
}
Texture2D StarMap;
PS_GBUFFER_OUT PS_Dome(VertexOutput_Dome input)
{
    float3 sunDirection = -normalize(GlobalLight.Direction);

    float temp = dot(sunDirection, input.oPosition) / length(input.oPosition);
    float temp2 = temp * temp;

    float3 ray = 0;
    float3 mie = 0;
  
    CalcMieRay(ray, mie, input.Uv);

    float3 rSamples = ray;
    float3 mSamples = mie;

 
    float3 color = 0;
    color = GetRayleighPhase(temp2) * rSamples * 0.5f + GetMiePhase(temp, temp2) * mSamples * 0.5f;
    color = HDR(color);

    color += max(0, (1 - color.rgb)) * float3(0.05f, 0.05f, 0.1f);
    float4 finalColor = float4(color, 1) + StarMap.Sample(LinearSampler, input.Uv) * saturate(StarIntensity);
    return PackGBuffer(finalColor.rgb, float4(1, 1, 1, 1), float3(0, 0, 0), float3(0, 0, 0), -1, -1, 1);

}










//////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

cbuffer CB_Cloud
{
    float CloudTiles;
    float CloudCover ;
    float CloudSharpness;
    float CloudSpeed;
    float2 FirstOffset;
    float2 SecondOffset;
};
struct VertexOutput_Cloud
{
    float4 Position : SV_Position0;
  
    float2 Uv : Uv0;
    float2 oUv : Uv1;
};
float Fade(float t)
{
  //return t * t * (3.0 - 2.0 * t);
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}
texture2D CloudMap;
float Noise(float2 P)
{
    float ONE = 0.00390625;
    float ONEHALF = 0.001953125;

    float2 Pi = ONE * floor(P) + ONEHALF;
    float2 Pf = frac(P);

   
    float2 grad00 = CloudMap.Sample(LinearSampler, Pi).rg * 4.0 - 1.0;
    float n00 = dot(grad00, Pf);

    float2 grad10 = CloudMap.Sample(LinearSampler, Pi + float2(ONE, 0.0)).rg * 4.0 - 1.0;
    float n10 = dot(grad10, Pf - float2(1.0, 0.0));

    float2 grad01 = CloudMap.Sample(LinearSampler, Pi + float2(0.0, ONE)).rg * 4.0 - 1.0;
    float n01 = dot(grad01, Pf - float2(0.0, 1.0));

    float2 grad11 = CloudMap.Sample(LinearSampler, Pi + float2(ONE, ONE)).rg * 4.0 - 1.0;
    float n11 = dot(grad11, Pf - float2(1.0, 1.0));

    float2 n_x = lerp(float2(n00, n01), float2(n10, n11), Fade(Pf.x));

    float n_xy = lerp(n_x.x, n_x.y, Fade(Pf.y));

    return n_xy;
}
texture2D Cloud1;
texture2D Cloud2;
float4 MakeCloudUseTexture(float2 uv)
{
    float2 sampleLocation;
    float4 textureColor1;
    float4 textureColor2;
    float4 finalColor;
    

    // Translate the position where we sample the pixel from using the first texture translation values.
    sampleLocation.x = uv.x + FirstOffset.x;
    sampleLocation.y = uv.y + FirstOffset.y;

    // Sample the pixel color from the first cloud texture using the sampler at this texture coordinate location.
    textureColor1 = Cloud1.Sample(LinearSampler, sampleLocation);
    
    // Translate the position where we sample the pixel from using the second texture translation values.
    sampleLocation.x = uv.x + SecondOffset.x;
    sampleLocation.y = uv.y + SecondOffset.y;

    // Sample the pixel color from the second cloud texture using the sampler at this texture coordinate location.
    textureColor2 = Cloud2.Sample(LinearSampler, sampleLocation);

    // Combine the two cloud textures evenly.
    finalColor = lerp(textureColor1, textureColor2, 0.5f);

    // Reduce brightness of the combined cloud textures by the input brightness value.
    finalColor = finalColor * 0.3f;

    return finalColor;
}
VertexOutput_Cloud VS_Cloud(VertexTexture input)
{
    VertexOutput_Cloud output;

    output.Position = mul(input.Position, World);
    output.Position = ViewProjection(output.Position);
  
  
    output.Uv = (input.Uv * CloudTiles);
    output.oUv = input.Uv;

    return output;
}
PS_GBUFFER_OUT PS_Cloud(VertexOutput_Cloud input)
{
    //input.Uv = input.Uv * CloudTiles;

    float n = Noise(input.Uv + Time * CloudSpeed);
    float n2 = Noise(input.Uv * 2 + Time * CloudSpeed);
    float n3 = Noise(input.Uv * 4 + Time * CloudSpeed);
    float n4 = Noise(input.Uv * 8 + Time * CloudSpeed);
 
    float nFinal = n + (n2 / 2) + (n3 / 4) + (n4 / 8);
 
    float c = CloudCover - nFinal;
    
    [flatten]
    if (c < 0) 
        c = 0;

    float density = 1.0 - pow(CloudSharpness, c);
  // float4 color = density;
    input.Uv = input.Uv + Time * CloudSpeed;
    float4 color = MakeCloudUseTexture(input.Uv) + density;
  
    return PackGBuffer(color.rgb, float4(1, 1, 1, 1), float3(0, 0, 0), float3(0, 0, 0), -1, -1, 1);
}

///////////////////////////////SkyCubeMap//////////////////////////////////////////////////////////////
cbuffer CB_CubeMap
{
    matrix CubeViews[6];
    matrix CubeProjection;
};
VertexOutput_Dome VS_DomeCubeMap(VertexTexture input)
{
    VertexOutput_Dome output;
    output.Position = WorldPosition(input.Position);
    output.oPosition = -input.Position;
    output.Uv = input.Uv;


    return output;
}
struct GeometryOutput
{
    float4 Position : SV_Position0;
    float3 oPosition : Position2;
    float2 Uv : Uv0;
  
    uint TargetIndex : SV_RenderTargetArrayIndex;
};
[maxvertexcount(18)]
void GS_SkyCubeMap(triangle VertexOutput_Dome input[3], inout TriangleStream<GeometryOutput> stream)
{
    int vertex = 0;
    GeometryOutput output;
     [unroll(6)]
    for (uint i = 0; i < 6; i++)
    {
        output.TargetIndex = i;
        [unroll(3)]
        for (vertex = 0; vertex < 3; vertex++)
        {
            output.Position = mul(input[vertex].Position, CubeViews[i]);
            output.Position = mul(output.Position, CubeProjection);
                     
            output.oPosition = input[vertex].oPosition;

            output.Uv = input[vertex].Uv;
          
            stream.Append(output);
        }
        stream.RestartStrip();
    }

}

float4 PS_DomeCubeMap(GeometryOutput input) : SV_Target
{
    float3 sunDirection = -normalize(GlobalLight.Direction);

    float temp = dot(sunDirection, input.oPosition) / length(input.oPosition);
    float temp2 = temp * temp;

    float3 ray = 0;
    float3 mie = 0;
  
    CalcMieRay(ray, mie, input.Uv);

    float3 rSamples = ray;
    float3 mSamples = mie;

 
    float3 color = 0;
    color = GetRayleighPhase(temp2) * rSamples * 0.5f + GetMiePhase(temp, temp2) * mSamples * 0.5f;
    color = HDR(color);

    color += max(0, (1 - color.rgb)) * float3(0.05f, 0.05f, 0.1f);
    float4 finalColor = float4(color, 1) + StarMap.Sample(LinearSampler, input.Uv) * saturate(StarIntensity);
    
    return finalColor;
    

}

VertexOutput_Cloud VS_CloudCubeMap(VertexTexture input)
{
    VertexOutput_Cloud output;
    output.Position = WorldPosition(input.Position);
    output.Uv = (input.Uv * CloudTiles);
    output.oUv = input.Uv;

    return output;
}

struct GeometryOutputCloud
{
  
    float4 Position : SV_Position0;
  
    float2 Uv : Uv0;
    float2 oUv : Uv1;

    uint TargetIndex : SV_RenderTargetArrayIndex;
};
[maxvertexcount(18)]
void GS_CloudCubeMap(triangle VertexOutput_Cloud input[3], inout TriangleStream<GeometryOutputCloud> stream)
{
    int vertex = 0;
    GeometryOutputCloud output;
    [unroll(6)]
    for (uint i = 0; i < 6; i++)
    {
        output.TargetIndex = i;
         [unroll(3)]
        for (vertex = 0; vertex < 3; vertex++)
        {
            output.Position = mul(input[vertex].Position, CubeViews[i]);
            output.Position = mul(output.Position, CubeProjection);
                   
         
            output.Uv = input[vertex].Uv;
            output.oUv = input[vertex].oUv;
            stream.Append(output);
        }
        stream.RestartStrip();
    }

}

float4 PS_CloudCubeMap(GeometryOutputCloud input) : SV_Target
{
    //input.Uv = input.Uv * CloudTiles;

    float n = Noise(input.Uv + Time * CloudSpeed);
    float n2 = Noise(input.Uv * 2 + Time * CloudSpeed);
    float n3 = Noise(input.Uv * 4 + Time * CloudSpeed);
    float n4 = Noise(input.Uv * 8 + Time * CloudSpeed);

    float nFinal = n + (n2 / 2) + (n3 / 4) + (n4 / 8);

    float c = CloudCover - nFinal;
  
    [flatten]
    if (c < 0) 
        c = 0;

    float density = 1.0 - pow(CloudSharpness, c);
  // float4 color = density;
    input.Uv = input.Uv + Time * CloudSpeed;
    float4 color = MakeCloudUseTexture(input.Uv) + density;

    return color;
   
}


////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////    Terrain     ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

ConstantHullOutput_Lod HS_Constant(InputPatch<VertexOutput_Lod, 4> input)
{


  
    //어떻게 짜를지 정함
    ConstantHullOutput_Lod output;
  float minY = input[0].BoundsY.x;
float maxY = input[0].BoundsY.y;
   //float minY =0;
   //float maxY = 3.402823466e+38f;
    
   //[unroll(4)]
   //for (int ip = 0; ip < 4; ++ip)
   // {
   //     minY = min(minY, GetHeight(input[ip].Position.x, input[ip].Position.z));
   //     maxY = max(maxY, GetHeight(input[ip].Position.x, input[ip].Position.z));
    
   // }


float3 vMin = float3(input[2].Position.x, minY, input[2].Position.z); //0이하단 2 우상단
float3 vMax = float3(input[1].Position.x, maxY, input[1].Position.z); //0이하단 2 우상단


float3 boxCenter = (vMin + vMax) * 0.5f;
float3 boxExtents = (vMax - vMin) * 0.5f;
  [flatten]
if (ContainFrustumCube(boxCenter, boxExtents))
{
    output.Edge[0] = 0;
    output.Edge[1] = 0;
    output.Edge[2] = 0;
    output.Edge[3] = 0;

    output.Inside[0] = 0;
    output.Inside[1] = 0;

    return output;
}
   
   //culling
    
    
   //   output.Inside[0] = 0;
   //   output.Inside[1] = 0;
    
   //[unroll(4)]
   //for (int ip = 0; ip < 4; ++ip)
   //{
          
   //    int culled = 1;
     
   //    culled &= dot(float3(input[0].Position.x, GetHeight(input[0].Position.x, input[0].Position.z), input[0].Position.z) - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
   //    culled &= dot(float3(input[1].Position.x, GetHeight(input[1].Position.x, input[1].Position.z), input[1].Position.z) - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
   //    culled &= dot(float3(input[2].Position.x, GetHeight(input[2].Position.x, input[2].Position.z), input[2].Position.z) - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
   //    culled &= dot(float3(input[3].Position.x, GetHeight(input[3].Position.x, input[3].Position.z), input[3].Position.z) - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
   //    if (culled)
   //    {
   //        output.Edge[ip] = 0;
   //        return output;
   //    }
     
   // }
   
    float3 e0 = (input[0].Position + input[2].Position).xyz * 0.5f;
    float3 e1 = (input[0].Position + input[1].Position).xyz * 0.5f;
    float3 e2 = (input[1].Position + input[3].Position).xyz * 0.5f;
    float3 e3 = (input[2].Position + input[3].Position).xyz * 0.5f;

    float3 center = (input[0].Position + input[1].Position + input[2].Position + input[3].Position).xyz * 0.25;
    
    output.Edge[0] = TessFactor(e0); //이 선을 2개로 분할한다.
    output.Edge[1] = TessFactor(e1);
    output.Edge[2] = TessFactor(e2);
    output.Edge[3] = TessFactor(e3);

    output.Inside[0] = TessFactor(center);
    output.Inside[1] = TessFactor(center);
   
    return output;

}
struct HullOutput
{
    float4 Position : Position0;
};
[domain("quad")] //어떤단계로 영역을 묶어서 출력할것인가
//[partitioning("integer")]
//[partitioning("fractional_odd")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("HS_Constant")]
[maxtessfactor(64)]
HullOutput_Lod HS(InputPatch<VertexOutput_Lod, 4> input, uint point0 : SV_OutputControlPointID)
{
    //짜르기전에 할 행동
    HullOutput_Lod output;
    output.Position = input[point0].Position;
    output.Uv = input[point0].Uv;
    
    return output;
}
[domain("quad")]
DomainOutput_Lod DS(ConstantHullOutput_Lod input, float2 uv : SV_DomainLocation, const OutputPatch<HullOutput_Lod, 4> patch)
{
    DomainOutput_Lod output;
    
    float3 v0 = lerp(patch[0].Position.xyz, patch[1].Position.xyz, uv.x);
    float3 v1 = lerp(patch[2].Position.xyz, patch[3].Position.xyz, uv.x);
    float3 position = lerp(v0, v1, uv.y); //베지어

    output.wPosition = position;
    
 
    
    float2 uv0 = lerp(patch[0].Uv, patch[1].Uv, uv.x);
    float2 uv1 = lerp(patch[2].Uv, patch[3].Uv, uv.x);
    output.Uv = lerp(uv0, uv1, uv.y);
   // output.TiledUv = output.Uv ;
    output.TiledUv = output.Uv * TexScale;
   // float3 displacement = 0;
   //  displacement = displaymentMap.SampleLevel(LinearSampler, output.TiledUv, 0).xyz * 2;
   //output.wPosition.y += displacement;

    output.wPosition.y += LodHeightMap.SampleLevel(LinearSampler, output.Uv, 0) * HeightRatio;
    
    
   
   
    
    //z-> lod값 해당 위치의픽셀을 얻어올수있다.
  
    output.Position = ViewProjection(float4(output.wPosition, 1.0f));
   

    return output;

}

PS_GBUFFER_OUT PS(DomainOutput_Lod input)
{
   
    float2 left = input.Uv + float2(-TexelCellSpaceU, 0.0);
    float2 right = input.Uv + float2(+TexelCellSpaceU, 0.0);
    float2 top = input.Uv + float2(0.0f, -TexelCellSpaceV);
    float2 bottom = input.Uv + float2(0.0f, TexelCellSpaceV);

    float leftY = LodHeightMap.SampleLevel(LinearSampler, left, 0) * HeightRatio;
    float rightY = LodHeightMap.SampleLevel(LinearSampler, right, 0) * HeightRatio;
    float topY = LodHeightMap.SampleLevel(LinearSampler, top, 0) * HeightRatio;
    float bottomY = LodHeightMap.SampleLevel(LinearSampler, bottom, 0) * HeightRatio;
  

   
   // float2 dleft = input.TiledUv + float2(-TexelCellSpaceU, 0.0);
   // float2 dright = input.TiledUv + float2(+TexelCellSpaceU, 0.0);
   // float2 dtop = input.TiledUv + float2(0.0f, -TexelCellSpaceV);
   // float2 dbottom = input.TiledUv + float2(0.0f, TexelCellSpaceV);

   //leftY += displaymentMap.SampleLevel(LinearSampler, dleft, 0).r * 2;
   //rightY += displaymentMap.SampleLevel(LinearSampler, dright, 0).r * 2;
   //topY += displaymentMap.SampleLevel(LinearSampler, dtop, 0).r * 2;
   //bottomY += displaymentMap.SampleLevel(LinearSampler, dbottom, 0).r * 2;
    //normal 구하기용

    float3 tangent = normalize(float3(WorldCellSpace * 2, rightY - leftY, 0.0f)); //사각형한칸의크기
    //tangent는 x축에맵핑되고 방향을 왼쪽과 오른쪽의 높이차를 구하고 
    //normal 은 z방향
    float3 biTangent = normalize(float3(0.0f, bottomY - topY, WorldCellSpace * -2));
    float3 normal = cross(tangent, biTangent); //정규직교식;

    //////////////////////////////////////////////////////////////////////////
  
    
    float2 Uv = input.TiledUv;
    //float distance_to_camera = length(ViewPosition() - input.wPosition);
    //float detailmap_miplevel = CalculateMIPLevelForDisplacementTextures(distance_to_camera);
    //log2(1+distance_to_camera*3000/(g_HeightFieldSize*g_TessFactor));
    
    float4 diffuseColor = BaseMap.Sample(LinearSampler, Uv);
    float4 map = NormalMap.Sample(LinearSampler, Uv);
    float roughness = TerrainRoughMap.Sample(LinearSampler, Uv).r;
 
    float3x3 TBN = float3x3(tangent, biTangent, normal);

    //이미지로부터 노멀벡터 가져오기
    float3 coord = map.rgb * 2.0f - 1.0f;
    float3 bump = 0;
    //탄젠트 공간으로 변환
    bump = mul(coord, TBN);
    
    //return PackGBuffer(float3(0, 0, 0).rgb, float4(1, 1, 1, 1), normalize(normal), normalize(bump), 0, roughness, 0, 0);
    return PackGBuffer(diffuseColor.rgb, float4(1, 1, 1, 1), normalize(normal), normalize(bump), 0, roughness, 0, 0);
   
  
   
}



////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////    Billboard     ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
struct BillboardVSInput
{
    float4 Position : Position0;
    float2 Scale : Scale0;
    uint TextureNum : Num0;
    float Random : Random0;
    uint VertexID : SV_VertexID;
};
struct BillboardVSOutput
{
    float4 Position : Position1;
    float2 Scale : Scale0;
    uint TextureNum : Num0;
    float Random : Random0;
    uint ID : Id0;
};


BillboardVSOutput BillboardVS(BillboardVSInput input)
{
    BillboardVSOutput output;

  
    output.Position = input.Position;
   
    output.Scale = input.Scale;
    output.TextureNum = input.TextureNum;
    output.Random = input.Random;
    output.ID = input.VertexID;
    return output;
}

struct BilboardGeometryOutput
{
    float4 Position : SV_Position0; //픽셀 쉐이더로 들어가는 input중에 반드시 svPosition이 있어야한다.
    float2 Uv : Uv0;
    uint TextureNum : Num0;
    uint ID : Id0;
};

static const float2 uvs[4] =
{
    float2(0, 1), float2(0, 0), float2(1, 1), float2(1, 0)
     
        
};
[maxvertexcount(4)]
void BillboardGS(triangle BillboardVSOutput input[3], inout TriangleStream<BilboardGeometryOutput> stream)//정점 하나를 받아서 gs에서 정점 4개로만들어서 면을 만든다.
{
    float3 up = float3(0, 1, 0);
   
    float3 forward = ViewPosition() - input[0].Position.xyz;
    forward = normalize(forward);
    float3 right = cross(forward, up);
    float2 size = input[0].Scale * 0.5f;
   

    float3 breeze = 0;
    float factor = 0.05f;
    breeze.x += cos(Time - input[0].Random) * (factor * input[0].Scale.x);
  
  //  float3 temp = float3(forward.x + 0.3f, forward.y, forward.z);
    float3 position[4];
    position[0] = float3(input[0].Position.xyz - size.x * right- size.y * up);
    position[1] = float3(input[0].Position.xyz - size.x * right + size.y * up) + breeze;
    position[2] = float3(input[0].Position.xyz + size.x * right - size.y * up);
    position[3] = float3(input[0].Position.xyz + size.x * right + size.y * up) + breeze;


    
  
   
    
  
    BilboardGeometryOutput output;
    
    [unroll(4)]
    for (int i = 0; i <4; i++)
    {
        output.Position = WorldPosition(float4(position[i], 1));
        output.Position = ViewProjection(output.Position);
         output.Uv = uvs[i];
        output.ID = input[0].ID;
        output.TextureNum = input[0].TextureNum;
      
        stream.Append(output);
        
    }

}

Texture2DArray BillboardMaps;



PS_GBUFFER_OUT PS_Billboard(BilboardGeometryOutput input)
{
    
   
    float4 diffuse = float4(0, 0, 0, 0);
  
  
    //int id = lerp(0, input.ID, saturate(input.ID));
    float3 uvw = float3(input.Uv,  input.TextureNum );
    
   
    diffuse = BillboardMaps.Sample(LinearSampler, uvw);
  
    
  
    [flatten]
    if (diffuse.a < 0.3f)
        discard;
  

    //clip(diffuse.a - 0.9f);
    return PackGBuffer(diffuse.rgb, float4(0, 0, 0, 0), float3(0, 0, 0), float3(0, 0, 0), 0,0 , 1);
  
}







/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////   CascadedShadow   /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
cbuffer CB_CasecadedShadow
{
    matrix CascadeViewProj[3];

};
float4 tempVS(float4 Pos : POSITION) : SV_Position0
{
    return Pos;
}
struct GS_OUTPUT
{
    float4 Pos : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};
[maxvertexcount(9)]
void CascadedShadowMapsGenGS(triangle MeshOutput InPos[3] : SV_Position, inout TriangleStream<GS_OUTPUT> OutStream)
{
    //float3 xProd = cross(normalize(InPos[2].wPosition - InPos[0].wPosition),
   // normalize(InPos[2].wPosition - InPos[0].wPosition));
    //then, if the x component of the input triangle is positive,   
    //discard the current triangle altogether by simply not     
    //outputting it to the rasterizer:     
    //[flatten]
    //if (xProd.x == 0.0f)
    //{
    //    return;
    //}
         
    [unroll(3)]
    for (int iFace = 0; iFace < 3; iFace++)
    {
        GS_OUTPUT output;

        output.RTIndex = iFace;
        [unroll(3)]
        for (int v = 0; v < 3; v++)
        {
           
           // int culled[4];
         
            output.Pos = mul(float4(InPos[v].wPosition, 1.0f), CascadeViewProj[iFace]);
            OutStream.Append(output);
        }
        OutStream.RestartStrip();
    }
}


/////////////////////////////////////////////////////////////////////////////
// directlight shader input/output structure
/////////////////////////////////////////////////////////////////////////////

static const float2 arrBasePos[4] =
{
    float2(-1.0, 1.0),
	float2(1.0, 1.0),
	float2(-1.0, -1.0),
	float2(1.0, -1.0),
};
static const float2 arrUV[4] =
{
    float2(0.0, 0.0),
    float2(1.0, 0.0),
	float2(0.0, 1.0),
	float2(1.0, 1.0),
};
struct VS_OUTPUT
{
    float4 Position : SV_Position; // vertex position 
	float2 cpPos	: Uv0;
    float2 Uv : Uv1;
   
};
VS_OUTPUT DirLightVS( uint VertexID : SV_VertexID )
{
	VS_OUTPUT Output;

	Output.Position = float4( arrBasePos[VertexID].xy, 0.0, 1.0);
    Output.cpPos = Output.Position.xy;
    Output.Uv = arrUV[VertexID].xy;
 
    //float2(Output.Position.x, -Output.Position.y) * 0.5f + 0.5f;
    //

	return Output;    
}
static float2 size = 1.0f / float2(1280, 720);
static float2 offsets[] =
{
    float2(+2 * size.x, -2 * size.y), float2(+size.x, -2 * size.y), float2(0.0f, -2 * size.y), float2(-size.x, -2 * size.y), float2(-2 * size.x, -2 * size.y),
            float2(+2 * size.x, -size.y), float2(+size.x, -size.y), float2(0.0f, -size.y), float2(-size.x, -size.y), float2(-2 * size.x, -size.y),
            float2(+2 * size.x, 0.0f), float2(+size.x, 0.0f), float2(0.0f, 0.0f), float2(-size.x, 0.0f), float2(-2 * size.x, 0.0f),
            float2(+2 * size.x, +size.y), float2(+size.x, +size.y), float2(0.0f, +size.y), float2(-size.x, +size.y), float2(-2 * size.x, +size.y),
            float2(+2 * size.x, +2 * size.y), float2(+size.x, +2 * size.y), float2(0.0f, +2 * size.y), float2(-size.x, +2 * size.y), float2(-2 * size.x, +2 * size.y),

};
static float weight[] =
{
    1, 1, 2, 1, 1,
            1, 2, 4, 2, 1,
            2, 4, 8, 4, 2,
            1, 2, 4, 2, 1,
            1, 1, 2, 1, 1,
};

void Gaussianblur(inout float totalweight, inout float sum, float3 UVD, float bestCascade)
{
    float3 uv = 0;
     [unroll(9)]
    for (int i = 0; i < 9; i++)
    {
        uv = float3(UVD.xy + offsets[i], bestCascade);
        totalweight += weight[i];

      
        sum += CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(uv), UVD.z) * weight[i];

    }
}

float CascadedShadow(float3 position)
{
	// Transform the world position to shadow space
    float4 posShadowSpace = mul(float4(position, 1.0), GlobalLight.ToShadowSpace);

	// Transform the shadow space position into each cascade position
    float4 posCascadeSpaceX = (GlobalLight.ToCascadeOffsetX + posShadowSpace.xxxx) * GlobalLight.ToCascadeScale;
    float4 posCascadeSpaceY = (GlobalLight.ToCascadeOffsetY + posShadowSpace.yyyy) * GlobalLight.ToCascadeScale;
   
	// Check which cascade we are in
    float4 inCascadeX = abs(posCascadeSpaceX) <= 1.0;
    float4 inCascadeY = abs(posCascadeSpaceY) <= 1.0;
    float4 inCascade = inCascadeX * inCascadeY;

	// Prepare a mask for the highest quality cascade the position is in
    float4 bestCascadeMask = inCascade;
    bestCascadeMask.yzw = (1.0 - bestCascadeMask.x) * bestCascadeMask.yzw;
    bestCascadeMask.zw = (1.0 - bestCascadeMask.y) * bestCascadeMask.zw;
    bestCascadeMask.w = (1.0 - bestCascadeMask.z) * bestCascadeMask.w;
    float bestCascade = dot(bestCascadeMask, float4(0.0, 1.0, 2.0, 3.0));

	// Pick the position in the selected cascade
    float3 UVD;
    UVD.x = dot(posCascadeSpaceX, bestCascadeMask);
    UVD.y = dot(posCascadeSpaceY, bestCascadeMask);
    UVD.z = posShadowSpace.z;
  //  UVD.z -= GlobalLight.Bias;
	// Convert to shadow map UV values
    UVD.xy = 0.5 * UVD.xy + 0.5;
    UVD.y = 1.0 - UVD.y;

	// Compute the hardware PCF value
 //float shadow = CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(UVD.xy, bestCascade), UVD.z);
	// set the shadow to one (fully lit) for positions with no cascade coverage
 //shadow = saturate(shadow + 1.0 - any(bestCascadeMask));

    //blur
    
    //return shadow;

    float sum = 0.0f;
    float totalweight = 0.0f;

    float3 uv = 0.0f;
    
    Gaussianblur(totalweight, sum, UVD, bestCascade);
  
    float factor = sum / totalweight;
  
    factor = saturate(factor + UVD.z);
  
   
    return factor;

}
float FresnelSchlick(float f0, float fd90, float view)
{
    return f0 + (fd90 - f0) * pow(max(1.0f - view, 0.1f), 5.0f);
}
float Disney(float NdotL, float LdotH, float NdotV,float roughness)
{
   
    float energyBias = lerp(0.0f, 0.5f, roughness);
    float energyFactor = lerp(1.0f, 1.0f / 1.51f, roughness);
    float fd90 = energyBias + 2.0f * (LdotH * LdotH) * roughness;
    float f0 = 1.0f;

    float lightScatter = FresnelSchlick(f0, fd90, NdotL);
    float viewScatter = FresnelSchlick(f0, fd90, NdotV);

    return lightScatter * viewScatter * energyFactor;
}
float3 GGX(float NdotL, float NdotH, float NdotV, float VdotH, float LdotH,float alpha, float3 realSpecularColor)
{
    float PI = 3.14159265f;
   
    
    float rough4 = alpha * alpha;

    float ndoth = max(NdotH, 0.0f);
    float NdotH2 = ndoth * ndoth;
    float nom = rough4;
    float denom = (NdotH2 * (rough4 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

  
   // float d = (NdotH * rough4 - NdotH) * NdotH + 1.0f;
   // float D = rough4 / (PI * (d * d));
    float D = nom / denom;
    float vdoth = max(VdotH, 0.0f);
   
	// Fresnel

   
    float3 reflectivity = realSpecularColor;
    float fresnel = 1.0;
   
    //float3 F = reflectivity + (fresnel - fresnel * reflectivity) * exp2((-5.55473f * LdotH - 6.98316f) * LdotH);
    float3 F = (reflectivity + (1.0f - reflectivity) * pow(1.0 - VdotH, 5.0f));
	// geometric / visibility
    //float k = alpha * 0.5f;
    //float G_SmithL = NdotL * (1.0f - k) + k;
    //float G_SmithV = NdotV * (1.0f - k) + k;
    //float G = 0.25f / (G_SmithL * G_SmithV);
   
    float ndotv = max(NdotV, 0.0f);
    float ndotl = max(NdotL, 0.0f);

    float r = alpha + 1.0f;
    float k = (r * r) / 8.0f;

    float nom1 = NdotV;
    float denom1 = NdotV * (1.0f - k) + k;
    float ggx1 = nom1 / denom1;

    float nom2 = NdotL;
    float denom2 = NdotL * (1.0f - k) + k;
    float ggx2 = nom2 / denom2;
   
    
    float G= ggx1 * ggx2;
    return G*D*F;
   
}
float3 FinalGamma(float3 color)
{
    return pow(color, 1.0f / 2.2);
}
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)   // cosTheta is n.v and F0 is the base reflectivity
{
    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0f);
}
float3 RadianceIBLIntegration(float NdotV, float roughness, float3 specular)
{
    float2 preintegratedFG = PreintegratedFG.Sample(LinearSampler, float2(roughness,  NdotV)).rg;
    return specular*preintegratedFG.r + preintegratedFG.g;
}

float3 IBL(Material material, float3 eye,float3 KS)
{
    float3 g_diffuse = pow(material.diffuseColor, 2.2f);
   
    float NdotV = saturate(dot(material.normal, eye));

    float3 reflectionVector = normalize(reflect(-eye, material.normal));
    float smoothness = 1.0f - material.roughness;
   float mipLevel = (1.0 - smoothness * smoothness) * 10.0f;
    float4 cs = skyIR.SampleLevel(LinearSampler, reflectionVector, mipLevel);
   
    float3 result = pow(cs.xyz, 2.2f) * RadianceIBLIntegration(NdotV, material.roughness, material.Specular);

    float3 diffuseDominantDirection = material.normal;
    float diffuseLowMip = 9.6f;
    float3 diffuseImageLighting = skyIR.SampleLevel(LinearSampler, diffuseDominantDirection, diffuseLowMip).rgb;
  
 //   textureLod(u_EnvironmentMap, diffuseDominantDirection, diffuseLowMip).rgb;
    diffuseImageLighting = pow(diffuseImageLighting, 2.2f);
    float3 final = (result) + (diffuseImageLighting) * g_diffuse;
  
    return (final * material.TerrainMask);
   // return (result * float3(0.74f, 0.59f, 0.247f)) + (diffuseImageLighting * float3(0.74f, 0.59f, 0.247f)) * g_diffuse;
}

float3 PreviewIBL(Material material, float3 eye, float3 KS)
{
    float3 g_diffuse = pow(material.diffuseColor, 2.2f);
   
    float NdotV = saturate(dot(material.normal, eye));

    float3 reflectionVector = normalize(reflect(-eye, material.normal));
    float smoothness = 1.0f - material.roughness;
    float mipLevel = (1.0 - smoothness * smoothness) * 10.0f;
    float4 cs = PreviewskyIR.SampleLevel(LinearSampler, reflectionVector, mipLevel);
   
    float3 result = pow(cs.xyz, 2.2f) * RadianceIBLIntegration(NdotV, material.roughness, material.Specular);

    float3 diffuseDominantDirection = material.normal;
    float diffuseLowMip = 9.6f;
    float3 diffuseImageLighting = PreviewskyIR.SampleLevel(LinearSampler, diffuseDominantDirection, diffuseLowMip).rgb;
  
 //   textureLod(u_EnvironmentMap, diffuseDominantDirection, diffuseLowMip).rgb;
    diffuseImageLighting = pow(diffuseImageLighting, 2.2f);
    float3 final = (result) + (diffuseImageLighting) * g_diffuse;
  
    return (final * material.TerrainMask);
   // return (result * float3(0.74f, 0.59f, 0.247f)) + (diffuseImageLighting * float3(0.74f, 0.59f, 0.247f)) * g_diffuse;
}






float LightFactor(float3 lightDir)
{
    
    float lightFactor = saturate(lightDir.y * 10 + 1);
    
    return lightFactor;

}

float4 CalcDirectionalPBR(float3 position, Material mat,int bShadow)
{
    
  //calc the half vector
    float3 viewDir = ViewPosition() - position;
   
    float3 lightDir = -GlobalLight.Direction;
    
    
    float3 H = normalize(viewDir - GlobalLight.Direction);
    //float3 R = normalize(reflect(-viewDir, mat.normal));
    //calc all the dot products we need
    float3 realAlbedo = RealAlbedo(mat);
    float3 realSpecularColor = RealSpecularColor(mat);
    float ndotl = NdotL(mat.Bump, lightDir);
    float ndoth = NdotH(mat.Bump, H);
  
    float ndotv = NdotV(mat.Bump, viewDir);
    float vdoth = VdotH(H, viewDir);
    float ldoth = LdotH(H, lightDir);
    
    float3 f0 = F0(mat);
    float3 kS = FresnelSchlickRoughness(1 - ndotv, f0, mat.roughness);
    float alpha = Alpha(mat);
    
    float3 diffuse = ndotl * Disney(ndotl, ldoth, ndotv, alpha);
    float3 specular = ndotl * GGX(ndotl, ndoth, ndotv, vdoth, ldoth, alpha, f0);
    
    float3 ibl = IBL(mat, viewDir, kS);
    float3 finalColor = (realAlbedo * diffuse.rgb) + (ibl + specular.rgb);
   
    finalColor = FinalGamma(finalColor);
    
    
    float shadow = CascadedShadow(position);
     finalColor = lerp(finalColor, finalColor * shadow, bShadow);
    
    float lightFactor = LightFactor(lightDir);
    finalColor = lerp(finalColor, finalColor * lightFactor, bShadow);
    
   
    
    return float4(finalColor, 1.0f);
}
float3 GammaToLinear(float3 color)
{
    return float3(color.x * color.x, color.y * color.y, color.z * color.z);

}
float3 Ambient(Material mat)
{
    
    float3 g_vAmbientLowerColor = GammaToLinear(float3(0.4f, 0.4f, 0.4f));
    float3 g_vAmbientUpperColor = GammaToLinear(float3(0.53f, 0.53f, 0.6f));
    float3 AmbientRange = g_vAmbientUpperColor - g_vAmbientLowerColor;
   
    // Normalize the vertical axis
    float up = mat.Bump.y * 0.5 + 0.5;

	// Calcualte the ambient light
    float3 ambient = g_vAmbientLowerColor + up * AmbientRange;
    ambient *= mat.diffuseColor.rgb;
    ambient *= g_vAmbientUpperColor;
    
    return ambient;
}

float3 Island11PS(float3 position, Material mat)
{
    
    float3 finalColor = FinalGamma(mat.diffuseColor);
    float3 lightDir = -GlobalLight.Direction;
    
    float shadow = CascadedShadow(position);
    finalColor = lerp(finalColor, finalColor * shadow, 1);
    
    float lightFactor = LightFactor(lightDir);
    finalColor = lerp(finalColor, finalColor * lightFactor, 1);
    return finalColor;
}
float4 DirLightPS(VS_OUTPUT In) : SV_TARGET
{
   
    Material mat = UnpackGBuffer(In.Position.xy);
	
	 [flatten]
    if (mat.SkyMask > 0)
    {
              
        return mat.diffuseColor;
      

    }
    
  
    float3 position = CalcWorldPos(In.cpPos, mat.LinearDepth);
    
   
  
	
    
    float ao = SsaoTexture.Sample(LinearSampler, In.Uv);
    float3 ambient = Ambient(mat);
    ambient *= ao;
    
    float3 finalColor =
    CalcDirectionalPBR(position, mat, 1).rgb;
    
    finalColor+= ambient;
    finalColor *= ao;
   
    finalColor+=GetBrushColor(position);

    
    float3 eyeToPixel = normalize(position - EyePosition());
    finalColor = ApplyFog(finalColor, EyePosition().y, eyeToPixel);
   
    return float4(finalColor, 1.0f);
}

float4 PrevRenderPS(MeshOutput In) : SV_TARGET
{
   
    // Lookup mesh texture and modulate it with diffuse
    float3 DiffuseColor = DiffuseMap.SampleLevel(LinearSampler, In.Uv,0);
    float4 Specular = SpecularMap.SampleLevel(LinearSampler, In.Uv, 0);
    float matallic = MatallicMap.SampleLevel(LinearSampler, In.Uv, 0).r;
    float roughness = RoughnessMap.SampleLevel(LinearSampler, In.Uv, 0).r;
	//DiffuseColor *= DiffuseColor;
   // DiffuseColor += float4(0.15f, 0.15f, 0.15f, 0.0f);
    float3 bump = 0;
    GetBumpMapCoord(In.Uv, In.Normal, In.Tangent, 0, bump);
  
    DiffuseColor *= Mat.Diffuse;
    Specular *= Mat.Specular;
    roughness *= Mat.Roughness;
    matallic *= Mat.Matallic;
	// Convert the data into the material structure
    Material mat;
    mat.normal = normalize(In.Normal);
    mat.diffuseColor.xyz = DiffuseColor.xyz;
    mat.diffuseColor.w = 1.0; // Fully opaque
    mat.Specular = Specular.xyz;
    mat.TerrainMask = 0.5f;
    mat.Bump = normalize(bump);
    mat.SkyMask = 0;
    mat.roughness = roughness;
    mat.matallic = matallic;
	    
    float3 finalColor = CalcDirectionalPBR(In.Position.xyz, mat,0);
     
    return float4(finalColor, 1.0f);
}





/////////////////////////////////////////////////////////////////////////////
//Point Vertex shader
/////////////////////////////////////////////////////////////////////////////
float4 PointLightVS() : SV_Position0
{
    
    return float4(0, 0, 0, 1.0f);
}
/////////////////////////////////////////////////////////////////////////////
// Point Hull shader
/////////////////////////////////////////////////////////////////////////////
struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
  
};
HS_CONSTANT_DATA_OUTPUT PointLightConstantHS()
{
    HS_CONSTANT_DATA_OUTPUT Output;
	
    float tessFactor = 18.0;
    Output.Edges[0] = Output.Edges[1] = Output.Edges[2] = Output.Edges[3] = tessFactor;
    Output.Inside[0] = Output.Inside[1] = tessFactor;
  
    return Output;
}
struct HS_OUTPUT
{
    float4 HemiDir : POSITION;
   
};
static const float3 HemilDir[2] =
{
    float3(1.0, 1.0, 1.0),
    float3(-1.0, 1.0, -1.0)
};
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("PointLightConstantHS")]
[maxtessfactor(64)]
HS_OUTPUT PointLightHS(uint PatchID : SV_PrimitiveID)
{
    HS_OUTPUT Output;

    Output.HemiDir = float4(HemilDir[PatchID], 0.0f);
   
    return Output;
}
/////////////////////////////////////////////////////////////////////////////
// Domain Shader shader
/////////////////////////////////////////////////////////////////////////////
struct DS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 cpPos : Uv0;
   
   
};
[domain("quad")]
DS_OUTPUT PointLightDS(HS_CONSTANT_DATA_OUTPUT input, float2 UV : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 4> quad)
{
	// Transform the UV's into clip-space
    float2 posClipSpace = UV.xy * 2.0 - 1.0;

	// Find the absulate maximum distance from the center
    float2 posClipSpaceAbs = abs(posClipSpace.xy);
    float maxLen = max(posClipSpaceAbs.x, posClipSpaceAbs.y);

	// Generate the final position in clip-space
    float3 normDir = normalize(float3(posClipSpace.xy, (maxLen - 1.0)) * quad[0].HemiDir.xyz);
    float4 posLS = float4(normDir.xyz, 1.0);
	
	// Transform all the way to projected space
    DS_OUTPUT Output;
   
    
   // Output.Position = mul(posLS, LightProjection[0]);
    Output.Position = mul(posLS, LightProjection);
    
    // Store the clip space position
    Output.cpPos = Output.Position.xy / Output.Position.w;
    
     
    return Output;
}
/////////////////////////////////////////////////////////////////////////////
// Pixel shader
/////////////////////////////////////////////////////////////////////////////

float3 CalcPointPBR(float3 position, Material mat)
{
    float3 lightDir = PointLight.Position - position;
    float3 viewDir = ViewPosition() - position;
   
    float DistToLight = length(lightDir);
      
    lightDir /= DistToLight; // Normalize
    
    float3 H = normalize(viewDir + lightDir);
    //float3 R = normalize(reflect(-viewDir, mat.normal));
   
    float3 realAlbedo = RealAlbedo(mat);
    float3 realSpecularColor = RealSpecularColor(mat);
    float ndotl = NdotL(mat.Bump, lightDir);
    float ndoth = NdotH(mat.Bump, H);
  
    float ndotv = NdotV(mat.Bump, viewDir);
    float vdoth = VdotH(H, viewDir);
    float ldoth = LdotH(H, lightDir);
    
    float3 f0 = F0(mat);
    float3 kS = FresnelSchlickRoughness(1 - ndotv, f0, mat.roughness);
    float alpha = Alpha(mat);
      
 
    float3 diffuse = ndotl * Disney(ndotl, ldoth, ndotv, alpha) * PointLight.Diffuse.rgb;
    float3 specular = ndotl * GGX(ndotl, ndoth, ndotv, vdoth, ldoth, alpha, f0) * PointLight.Diffuse.rgb;
      
    float3 finalColor = realAlbedo * diffuse.rgb + (specular.rgb);
    finalColor = FinalGamma(finalColor);
       
    // Attenuation
    float DistToLightNorm = 1.0 - saturate(DistToLight / PointLight.Range);
    float Attn = DistToLightNorm * DistToLightNorm;
    
    finalColor *= Attn;
    finalColor *= PointLight.Intensity;
 
    return finalColor;
}
float4 PointLightCommonPS(DS_OUTPUT In, bool bUseShadow) : SV_TARGET
{
        
    Material mat = UnpackGBuffer(In.Position.xy);
   	
  
    float3 position = CalcWorldPos(In.cpPos, mat.LinearDepth);
    
   
    float3 finalColor = CalcPointPBR(position, mat);
   
   
   
    return float4(finalColor * PointLight.Intensity, 1.0f);
}
float4 PointLightPS(DS_OUTPUT In) : SV_TARGET
{
    return PointLightCommonPS(In, false);
}
float4 PointLightShadowPS(DS_OUTPUT In) : SV_TARGET
{
    return PointLightCommonPS(In, true);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//// Vertex shader
///////////////////////////////////////////////////////////////////////////////
float4 SpotLightVS() : SV_Position
{
    return float4(0.0, 0.0, 0.0, 1.0);
}

/////////////////////////////////////////////////////////////////////////////
// Hull shader
/////////////////////////////////////////////////////////////////////////////
struct HS_Const_SpotOutput
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};


HS_Const_SpotOutput SpotLightConstantHS()
{
    HS_Const_SpotOutput Output;
	
    float tessFactor = 18.0;
    Output.Edges[0] = Output.Edges[1] = Output.Edges[2] = Output.Edges[3] = tessFactor;
    Output.Inside[0] = Output.Inside[1] = tessFactor;

    return Output;
}
struct HS_SPOT_OUTPUT
{
    float4 Position : POSITION;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("SpotLightConstantHS")]
HS_SPOT_OUTPUT SpotLightHS()
{
    HS_SPOT_OUTPUT Output;

    Output.Position = float4(0.0, 0.0, 0.0, 1.0);

    return Output;
}

/////////////////////////////////////////////////////////////////////////////
// Domain Shader shader
/////////////////////////////////////////////////////////////////////////////
struct DS_SPOT_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 cpPos : Uv0;
};



[domain("quad")]
DS_SPOT_OUTPUT SpotLightDS(HS_Const_SpotOutput input, float2 UV : SV_DomainLocation, const OutputPatch<HS_SPOT_OUTPUT, 4> quad)
{
    float CylinderPortion = 0.2;
    float ExpendAmount = (1.0 + CylinderPortion);
	// Transform the UV's into clip-space
    float2 posClipSpace = UV.xy * float2(2.0, -2.0) + float2(-1.0, 1.0);

	// Find the vertex offsets based on the UV
    float2 posClipSpaceAbs = abs(posClipSpace.xy);
    float maxLen = max(posClipSpaceAbs.x, posClipSpaceAbs.y);

	// Force the cone vertices to the mesh edge
    float2 posClipSpaceNoCylAbs = saturate(posClipSpaceAbs * ExpendAmount);
    float maxLenNoCapsule = max(posClipSpaceNoCylAbs.x, posClipSpaceNoCylAbs.y);
    float2 posClipSpaceNoCyl = sign(posClipSpace.xy) * posClipSpaceNoCylAbs;

	// Convert the positions to half sphere with the cone vertices on the edge
    float3 halfSpherePos = normalize(float3(posClipSpaceNoCyl.xy, 1.0 - maxLenNoCapsule));

	// Scale the sphere to the size of the cones rounded base
    
    halfSpherePos = normalize(float3(halfSpherePos.xy * fSinAngle, fCosAngle));

	// Find the offsets for the cone vertices (0 for cone base)
    float cylinderOffsetZ = saturate((maxLen * ExpendAmount - 1.0) / CylinderPortion);

	// Offset the cone vertices to thier final position
    float4 posLS = float4(halfSpherePos.xy * (1.0 - cylinderOffsetZ), halfSpherePos.z - cylinderOffsetZ * fCosAngle, 1.0);

	// Transform all the way to projected space and generate the UV coordinates
    DS_SPOT_OUTPUT Output;
    Output.Position = mul(posLS, SpotLightProjection);
    Output.cpPos = Output.Position.xy / Output.Position.w;

    return Output;
}

/////////////////////////////////////////////////////////////////////////////
// Pixel shader
/////////////////////////////////////////////////////////////////////////////
float3 CalcSpotPBR(float3 position, Material material)
{
    float PI = 3.141592653589793;
    float3 lightDir = SpotPosition - position;
    float3 viewDir = EyePosition() - position;
    float DistToLight = length(lightDir);
      
    lightDir /= DistToLight; // Normalize
   
    float3 H = normalize(viewDir + lightDir);
    float3 R = normalize(reflect(-lightDir, material.normal));
  
 //calc all the dot products we need
    float NdotL = saturate(dot(material.Bump, lightDir));
    float NdotH = saturate(dot(material.Bump, H));
   // float NdotH = saturate(dot(R, normalize(viewDir)));
    float NdotV = saturate(dot(material.normal, viewDir));
    float VdotH = saturate(dot(viewDir, H));
    float LdotH = saturate(dot(lightDir, H));
    float alpha = max(0.001f, material.roughness * material.roughness);
  
    float3 g_diffuse = pow(material.diffuseColor, 2.2);
    float3 g_lightColor = GlobalLight.Specular;

 // Lerp with metallic value to find the good diffuse and specular. 
    float3 realAlbedo = g_diffuse - (g_diffuse * material.matallic);
    realAlbedo = saturate(realAlbedo);
    float3 realSpecularColor = lerp(0.03f, g_diffuse, material.matallic);
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, g_diffuse, material.matallic);

 //calculate the diffuse and specular components
    float3 diffuse = 0;
    float3 specular = 0;

 //final result
    diffuse += NdotL * Disney(NdotL, LdotH, NdotV, alpha) * SpotDiffuse.rgb;
    specular += NdotL * GGX(NdotL, NdotH, NdotV, VdotH, LdotH, alpha, F0) * SpotSpecular.rgb;
   
    float3 finalColor = realAlbedo * diffuse.rgb + (specular.rgb);
    finalColor *= SpotIntensity;
    finalColor = FinalGamma(finalColor);
      
  	// Cone attenuation
    float cosAng = dot(SpotvDirToLight, lightDir);
    float conAtt = saturate((cosAng - SpotCosOuterCone) / SpotCosConeAttRange);
    conAtt *= conAtt;
 
    float DistToLightNorm = 1.0 - saturate(DistToLight * SpotRange);
    float Attn = DistToLightNorm * DistToLightNorm;
  
    return finalColor * Attn * conAtt;

}

float4 SpotLightPS(DS_SPOT_OUTPUT In) : SV_TARGET
{
	// Unpack the GBuffer
    Material mat = UnpackGBuffer(In.Position.xy);
	
	// Convert the data into the material structure
   

	// Reconstruct the world position
    float3 position = CalcWorldPos(In.cpPos, mat.LinearDepth);

	// Calculate the light contribution
    float3 finalColor = CalcSpotPBR(position, mat);
    
  
    return float4(finalColor * SpotIntensity, 1.0f);
}

technique11 T0
{
    
 ////////////////////////////////   /*Shadow*/   /////////////////////////////////
    pass P0
    {
        SetRasterizerState(cascadedRS);
        SetDepthStencilState(cascadedDSSLess, 0);
        SetVertexShader(CompileShader(vs_5_0, VS_Mesh_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
    pass P1
    {
        SetRasterizerState(cascadedRS);
         SetDepthStencilState(cascadedDSSLess, 0);
          SetVertexShader(CompileShader(vs_5_0, VS_Model_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
     pass P2

    {
        SetRasterizerState(cascadedRS);
        SetDepthStencilState(cascadedDSSLess, 0);
        SetVertexShader(CompileShader(vs_5_0, VS_Animation_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
         
  /////// /////////////////////// /*Deferred Packing */// //////////////////////////////
//Sky
    P_DSS_VP(P3, SkyDSS, VS_Dome, PS_Dome)
    P_DSS_BS_VP(P4, SkyDSS, blendState, VS_Cloud, PS_Cloud)
 
//Mesh&Model

     P_VP(P5, VS_Mesh_PreRender, RenderScenePS)
     P_VP(P6, VS_Model_PreRender, RenderScenePS)
 
  
     P_VP(P7, VS_Animation_PreRender, RenderScenePS)
   
    pass P8 //terrain
    {
        //SetRasterizerState(terrainRS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

 ////////////////////////////////dir light render //////////////////////////////////////   
    pass P9
    {
       
        SetVertexShader(CompileShader(vs_5_0, DirLightVS()));
        SetPixelShader(CompileShader(ps_5_0, DirLightPS()));
    }

    pass P10 //Point
    {
        SetRasterizerState(RS);
        SetDepthStencilState(pointDSS, 1);
        SetBlendState(blendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, PointLightVS()));
        SetHullShader(CompileShader(hs_5_0, PointLightHS()));
        SetDomainShader(CompileShader(ds_5_0, PointLightDS()));
        SetPixelShader(CompileShader(ps_5_0, PointLightShadowPS()));
    }
    
    pass P11//Spot
    {
       
       
        SetRasterizerState(RS);
        SetDepthStencilState(pointDSS, 1);
        SetBlendState(blendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, SpotLightVS()));
        SetHullShader(CompileShader(hs_5_0, SpotLightHS()));
        SetDomainShader(CompileShader(ds_5_0, SpotLightDS()));
        SetPixelShader(CompileShader(ps_5_0, SpotLightPS()));

    }
/////////////////////////////////////////////////////////////////////////////////////////
    pass P12
    {
        SetVertexShader(CompileShader(vs_5_0, BillboardVS()));
        SetGeometryShader(CompileShader(gs_5_0, BillboardGS()));
        SetPixelShader(CompileShader(ps_5_0, PS_Billboard()));

    }
  
  
   P_VP(P13, VS_Animation_PreviewRender, PrevRenderPS)
  
    pass P14
    {
       // SetRasterizerState(OceanRS);
        SetBlendState(ForwardblendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, VS_Forward_PreviewRender()));
        SetPixelShader(CompileShader(ps_5_0, PrevRenderPS()));
    }

    pass P15
    {
       // SetRasterizerState(OceanRS);
       SetBlendState(TransparencyblendState, float4(0, 0, 0, 0), 0xFF);
      //  SetBlendState(ForwardblendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, VS_Forward()));
        SetPixelShader(CompileShader(ps_5_0, RenderScenePS()));
    }

    pass P16
    {
       
        SetVertexShader(CompileShader(vs_5_0, DebugVS()));
        SetPixelShader(CompileShader(ps_5_0, DebugPS()));
    }
   ////Sky
   // P_DSS_VGP(P17, SkyDSS, VS_DomeCubeMap, GS_SkyCubeMap, PS_DomeCubeMap)
   // P_DSS_BS_VGP(P18, SkyDSS, blendState, VS_CloudCubeMap, GS_CloudCubeMap, PS_CloudCubeMap)

    //pass P17//terrain depth
    //{
    //    SetRasterizerState(CullBackMS);
    //    SetDepthStencilState(DepthNormal, 0);
    //    SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

    //    SetVertexShader(CompileShader(vs_5_0, PassThroughVS()));
    //    SetHullShader(CompileShader(hs_5_0, PatchHS()));
    //    SetDomainShader(CompileShader(ds_5_0, HeightFieldPatchDS()));
    //    SetGeometryShader(NULL);
    //    SetPixelShader(CompileShader(ps_5_0, ColorPS(float4(1.0f, 1.0f, 1.0f, 1.0f))));
    //}

    //pass P18 //WaterNormalMapComnine
    //{
    //    SetRasterizerState(NoCullMS);
    //    SetDepthStencilState(NoDepthStencil, 0);
    //    SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

    //    SetVertexShader(CompileShader(vs_5_0, WaterNormalmapCombineVS()));
    //    SetHullShader(NULL);
    //    SetDomainShader(NULL);
    //    SetGeometryShader(NULL);
    //    SetPixelShader(CompileShader(ps_5_0, WaterNormalmapCombinePS()));
    //}
    //pass P19 //Terrain reflection
    //{
    //    SetRasterizerState(CullBackMS);
    //    //SetDepthStencilState(DepthNormal, 0);
    //    SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

    //    SetVertexShader(CompileShader(vs_5_0, PassThroughVS()));
    //    SetHullShader(CompileShader(hs_5_0, PatchHS()));
    //    SetDomainShader(CompileShader(ds_5_0, HeightFieldPatchDS()));
    //    SetGeometryShader(NULL);
    //    SetPixelShader(CompileShader(ps_5_0, HeightFieldPatchPS()));
    //}

    //pass P20 //TerrainPacking
    //{
    //    SetRasterizerState(CullBackMS);
    //    //SetDepthStencilState(DepthNormal, 0);
    //    SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

    //    SetVertexShader(CompileShader(vs_5_0, PassThroughVS()));
    //    SetHullShader(CompileShader(hs_5_0, PatchHS()));
    //    SetDomainShader(CompileShader(ds_5_0, HeightFieldPatchDS()));
    //    SetGeometryShader(NULL);
    //    SetPixelShader(CompileShader(ps_5_0, HeightFieldPatchPacking()));
    //}


 

    //pass P21//water
    //{
    //    SetRasterizerState(CullBackMS);
    //    SetDepthStencilState(DepthNormal, 0);
    // //   SetBlendState(blendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

    //    SetVertexShader(CompileShader(vs_5_0, PassThroughVS()));
    //    SetHullShader(CompileShader(hs_5_0, PatchHS()));
    //    SetDomainShader(CompileShader(ds_5_0, WaterPatchDS()));
    //    SetGeometryShader(NULL);
    //    SetPixelShader(CompileShader(ps_5_0, WaterPatchPS()));
    //}

    //pass P22 //Maintobackbuffer
    //{
    //    SetRasterizerState(NoCullMS);
    //    SetDepthStencilState(NoDepthStencil, 0);
    // //   SetBlendState(blendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

    //    SetVertexShader(CompileShader(vs_5_0, FullScreenQuadVS()));
    //    SetHullShader(NULL);
    //    SetDomainShader(NULL);
    //    SetGeometryShader(NULL);
    //    SetPixelShader(CompileShader(ps_5_0, MainToBackBufferPS()));
    //}

    //pass P23
    //{
    //    SetRasterizerState(NoCullMS);
    //    SetDepthStencilState(NoDepthStencil, 0);
    //    SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    //    SetVertexShader(CompileShader(vs_5_0, FullScreenQuadVS()));
    //    SetHullShader(NULL);
    //    SetDomainShader(NULL);
    //    SetGeometryShader(NULL);
    //    SetPixelShader(CompileShader(ps_5_0, RefractionDepthManualResolvePS1()));
    //}

    

}
  
