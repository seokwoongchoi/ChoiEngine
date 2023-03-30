#include "000_Header.fx"
#include "000_Model.fx"
#include "000_Terrain.fx"
#include "PBRHeader.fx"
#include "IBLHeader.fx"
#include "pbrCommon.fx"

TextureCube EnvMap;
TextureCube skyPrefilter;
Texture2D brdfLUT;

matrix CubeViews[6];
matrix CubeProjection;
float uvroughness=0.75f;


/////////////////////////////////////////////////////////////////////////////
// Constant Buffers
/////////////////////////////////////////////////////////////////////////////

matrix CascadeViewProj[3];

cbuffer cbPerObjectPS // Model pixel shader constants
{
    float specExp;
    float specIntensity;
};
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


struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_Target0;
    float4 Normal : SV_Target1;
    float4 Specular : SV_Target2;
      
};
void GetBumpMapCoord(float2 uv, float3 normal, float3 tangent, SamplerState samp, inout float3 bump)
{
    float4 map = NormalMap.Sample(samp, uv);

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
PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 Normal,float4 specular, float matallic, float roughness)
{
    PS_GBUFFER_OUT Out;


    Out.ColorSpecInt = float4(BaseColor.rgb, matallic);
    Out.Normal = float4(Normal.rgb * 0.5 + 0.5, roughness);
    Out.Specular = specular;
  
    return Out;
}

//////////////////////////////// Sky ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


cbuffer CB_Scatter
{
    float3 WaveLength;
    int SampleCount;

    float3 InvWaveLength;
    float StarIntensity;

    float3 WaveLengthMie;
    float MoonAlpha;
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
    float g = -0.980f;
    float g2 = -0.980f * -0.980f;
   
    float3 result = 0;
    result.x = 1.5f * ((1.0f - g2) / (2.0f + g2));
    result.y = 1.0f + g2;
    result.z = 2.0f * g;

    return result.x * (1.0f + c2) / pow(result.y - result.z * c, 1.5f);
}

float3 HDR(float3 LDR)
{
    float Exposure = -2.0f; //노출도
    
    return 1.0f - exp(Exposure * LDR);
}
float HitOuterSphere(float3 position, float3 direction)
{
    
    float OuterRadius = 6356.7523142f * 1.0157313f;
    float3 light = -position;

    float b = dot(light, direction);
    float c = dot(light, light);

    float d = c - b * b;
    float q = sqrt(OuterRadius * OuterRadius - d);

    return b + q;
}

float2 GetDensityRatio(float height)
{
    float InnerRadius = 6356.7523142f;
    float2 RayleighMieScaleHeight = { 0.25f, 0.1f };
    float Scale = 1.0 / (6356.7523142 * 1.0157313 - 6356.7523142);
    float altitude = (height - InnerRadius) * Scale;

    return exp(-altitude / RayleighMieScaleHeight);
}

float2 GetDistance(float3 p1, float3 p2)
{
    float2 RayleighMieScaleHeight = { 0.25f, 0.1f };
    float Scale = 1.0 / (6356.7523142 * 1.0157313 - 6356.7523142);
    float2 opticalDepth = 0;

    float3 temp = p2 - p1;
    float far = length(temp);
    float3 direction = temp / far;


    float sampleLength = far / SampleCount;
    float scaledLength = sampleLength * Scale;

    float3 sampleRay = direction * sampleLength;
    p1 += sampleRay * 0.5f;

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
    float PI = 3.14159265f;
    float InnerRadius = 6356.7523142f;
    float OuterRadius = 6356.7523142f * 1.0157313f;
    float KrESun = 0.0025f * 20.0f; //0.0025f - 레일리 상수 * 태양의 밝기
    float KmESun = 0.0010f * 20.0f; //0.0025f - 미 상수 * 태양의 밝기
    float Kr4PI = 0.0025f * 4.0f * 3.1415159;
    float Km4PI = 0.0010f * 4.0f * 3.1415159;
    float2 RayleighMieScaleHeight = { 0.25f, 0.1f };
    float Scale = 1.0 / (6356.7523142 * 1.0157313 - 6356.7523142);

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
    float4 finalColor = float4(color,1) + StarMap.Sample(LinearSampler, input.Uv) * saturate(StarIntensity);
    return PackGBuffer(finalColor.rgb, float3(0, 0, 0), float4(0, 0, 0, -1), 0, 0);

}


struct VertexOutput_Moon
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
};

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


//////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VertexOutput_Moon VS_Moon(VertexTexture input)
{
    VertexOutput_Moon output;

    output.Position = WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);
    output.Uv = input.Uv;

    return output;
}


Texture2D MoonMap;

PS_GBUFFER_OUT PS_Moon(VertexOutput_Moon input)
{
    float4 color = MoonMap.Sample(LinearSampler, input.Uv);
   // color.a *= MoonAlpha;

    return PackGBuffer(color.rgb, float3(0, 0, 0), float4(0, 0, 0, -1), 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
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
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);

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
    if (c < 0) 
        c = 0;

    float density = 1.0 - pow(CloudSharpness, c);
  // float4 color = density;
    input.Uv = input.Uv + Time * CloudSpeed;
    float4 color = MakeCloudUseTexture(input.Uv) + density;
  
    return PackGBuffer(color.rgb, float3(0, 0, 0), float4(0, 0, 0, -1), 0, 0);
}


///////////////////////////////////////////////////////////////////////////////


DepthStencilState SkyDSS
{
    DepthEnable = false;
    DepthWriteMask = Zero;
    DepthFunc = Less;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;
};
[maxvertexcount(18)]
void GS_PreRender(triangle MeshOutput input[3], inout TriangleStream<GeometryOutput> stream)
{
    int vertex = 0;
    GeometryOutput output;

    for (uint i = 0; i < 6; i++)
    {
        output.TargetIndex = i;

        for (vertex = 0; vertex < 3; vertex++)
        {
            output.Position = mul(input[vertex].Position, CubeViews[i]);
            output.Position = mul(output.Position, CubeProjection);
            
            output.wvpPosition = input[vertex].wvpPosition;
            output.wPosition = input[vertex].wPosition;
            output.oPosition = input[vertex].oPosition;

            output.sPosition = input[vertex].sPosition;
          

            output.Uv = input[vertex].Uv;
            output.Normal = input[vertex].Normal;
            output.Tangent = input[vertex].Tangent;
            stream.Append(output);
        }
        stream.RestartStrip();
    }

}


float4 main(GeometryOutput input) : SV_Target0
{
    float3 normalVec = normalize(input.Normal);
    float3 R = normalVec;
    float3 viewDir = R;

    float3 PrefilteredColor = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
	
    const uint NumSamples = 1024;
    for (uint i = 0; i < NumSamples; i++)
    {
        float2 Xi = Hammersley(i, NumSamples);
        float3 halfwayVec = ImportanceSampleGGX(Xi, input.Uv.x, normalVec);
        float3 lightDir = 2 * dot(viewDir, halfwayVec) * halfwayVec - viewDir;
        float NdotL = saturate(dot(normalVec, lightDir));
        if (NdotL > 0)
        {
			// sample from the environment's mip level based on roughness/pdf
            float D = NormalDistributionGGXTR(normalVec, halfwayVec, input.Uv.x);
            float NdotH = max(dot(normalVec, halfwayVec), 0.0f);
            float HdotV = max(dot(halfwayVec, viewDir), 0.0f);
            float pdf = D * NdotH / (4.0f * HdotV) + 0.0001f;

            float resolution = 512.0f; // resolution of source cubemap (per face)
            float saTexel = 4.0f * 3.14159265359 / (6.0f * resolution * resolution);
            float saSample = 1.0f / (float(NumSamples) * pdf + 0.0001f);

            float mipLevel = input.Uv.x == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            PrefilteredColor += EnvMap.SampleLevel(LinearSampler, lightDir, 0).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    PrefilteredColor /= totalWeight;

    float4 color = DiffuseMap.Sample(LinearSampler, input.Position.xy);
   // return color;
    return float4(PrefilteredColor, 1.0f);
}


///////////////////////////////// Mesh //////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
PS_GBUFFER_OUT RenderScenePS(MeshOutput In)
{
    // Lookup mesh texture and modulate it with diffuse
    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv);
    float4 Specular = SpecularMap.Sample(LinearSampler, In.Uv);
    float matallic = MatallicMap.Sample(LinearSampler, In.Uv);
    float roughness = RoughnessMap.Sample(LinearSampler, In.Uv).r;
	//DiffuseColor *= DiffuseColor;
   // DiffuseColor += float4(0.15f, 0.15f, 0.15f, 0.0f);
    float3 bump = 0;
    GetBumpMapCoord(In.Uv, In.Normal, In.Tangent, LinearSampler, bump);
    return PackGBuffer(DiffuseColor, normalize(bump), Specular, matallic, roughness);
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////    Terrain     ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////



ConstantHullOutput_Lod HS_Constant(InputPatch<VertexOutput_Lod, 4> input)
{

    float minY = input[0].BoundsY.x;
    float maxY = input[0].BoundsY.y;

    float3 vMin = float3(input[2].Position.x, minY, input[2].Position.z); //0이하단 2 우상단
    float3 vMax = float3(input[1].Position.x, maxY, input[1].Position.z); //0이하단 2 우상단
    

    float3 boxCenter = (vMin + vMax) * 0.5f;
    float3 boxExtents = (vMax - vMin) * 0.5f;
    //어떻게 짜를지 정함
    ConstantHullOutput_Lod output;
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
[partitioning("integer")]
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

    output.wPosition.y += LodHeightMap.SampleLevel(LinearSampler, output.Uv, 0) * HeightRatio1;
    //z-> lod값 해당 위치의픽셀을 얻어올수있다.
    output.Position = ViewProjection(float4(output.wPosition, 1.0f));
    output.TiledUv = output.Uv * TexScale;

    return output;

}


PS_GBUFFER_OUT PS(DomainOutput_Lod input)
{
   
    float2 left = input.Uv+ float2(-TexelCellSpaceU, 0.0);
    float2 right = input.Uv + float2(+TexelCellSpaceU, 0.0);
    float2 top = input.Uv + float2(0.0f, -TexelCellSpaceV);
    float2 bottom = input.Uv + float2(0.0f, TexelCellSpaceV);

    float leftY = LodHeightMap.SampleLevel(LinearSampler, left, 0).b * HeightRatio1;
    float rightY = LodHeightMap.SampleLevel(LinearSampler, right, 0).b * HeightRatio1;
    float topY = LodHeightMap.SampleLevel(LinearSampler, top, 0).b * HeightRatio1;
    float bottomY = LodHeightMap.SampleLevel(LinearSampler, bottom, 0).b * HeightRatio1;
    //normal 구하기용

    float3 tangent = normalize(float3(WorldCellSpace * 2, rightY - leftY, 0.0f)); //사각형한칸의크기
    //tangent는 x축에맵핑되고 방향을 왼쪽과 오른쪽의 높이차를 구하고 
    //normal 은 z방향
    float3 biTangent = normalize(float3(0.0f, bottomY - topY, WorldCellSpace * -2));
    float3 normal = cross(tangent, biTangent); //정규직교식;

    //////////////////////////////////////////////////////////////////////////
        
       // Lookup mesh texture and modulate it with diffuse
   
    float2 Uv = input.TiledUv;
    // Uv = input.Uv;

    float4 DiffuseColor = BaseMap.Sample(LinearSampler, Uv);
    float4 Specular = TerrainSpecMap.Sample(LinearSampler, Uv);
    float4 map = NormalMap.Sample(LinearSampler, Uv);
    float matallic = MatallicMap.Sample(LinearSampler, Uv).r;
    float roughness = RoughnessMap.Sample(LinearSampler, Uv).r;
    float3x3 TBN = float3x3(tangent, biTangent, normal);

    //이미지로부터 노멀벡터 가져오기
    float3 coord = map.rgb * 2.0f - 1.0f;
    float3 bump = 0;
    //탄젠트 공간으로 변환
    coord = mul(coord, TBN);
   
  
   
    return PackGBuffer(DiffuseColor.rgb, normalize(coord), Specular, matallic, roughness);
   
}


/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////   CascadedShadow   /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

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
    for (int iFace = 0; iFace < 3; iFace++)
    {
        GS_OUTPUT output;

        output.RTIndex = iFace;

        for (int v = 0; v < 3; v++)
        {
            output.Pos = mul(float4(InPos[v].wPosition, 1.0f), CascadeViewProj[iFace]);
            OutStream.Append(output);
        }
        OutStream.RestartStrip();
    }
}
RasterizerState cascadedRS
{
    CullMode = front;
    DepthBias = 6;
    SlopeScaledDepthBias = -1.0f;
    DepthClipEnable = false;
};

DepthStencilState cascadedDSS
{
    DepthEnable = true;
    DepthWriteMask = All;
    DepthFunc = Less;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;
   // DepthFunc = Less;
   
};

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// shader input/output structure
/////////////////////////////////////////////////////////////////////////////

static const float2 arrBasePos[4] =
{
    float2(1.0, -1.0),
    float2(1.0, 1.0),
	float2(-1.0, 1.0),
	float2(-1.0, -1.0)
};

static const float2 arrUV[4] =
{
    float2(1.0, 1.0),
	float2(1.0, 0.0),
	float2(0.0, 0.0),
	float2(0.0, 1.0),
};


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Vertex shader
/////////////////////////////////////////////////////////////////////////////
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


/////////////////////////////////////////////////////////////////////////////
// Pixel shaders
/////////////////////////////////////////////////////////////////////////////
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
  float shadow = CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(UVD.xy, bestCascade), UVD.z);
	// set the shadow to one (fully lit) for positions with no cascade coverage
 shadow = saturate(shadow + 1.0 - any(bestCascadeMask));

    //blur
    float2 size = 1.0f / float2(1280, 720);
    float2 offsets[] =
    {
        float2(+2 * size.x, -2 * size.y), float2(+size.x, -2 * size.y), float2(0.0f, -2 * size.y), float2(-size.x, -2 * size.y), float2(-2 * size.x, -2 * size.y),
            float2(+2 * size.x, -size.y), float2(+size.x, -size.y), float2(0.0f, -size.y), float2(-size.x, -size.y), float2(-2 * size.x, -size.y),
            float2(+2 * size.x, 0.0f), float2(+size.x, 0.0f), float2(0.0f, 0.0f), float2(-size.x, 0.0f), float2(-2 * size.x, 0.0f),
            float2(+2 * size.x, +size.y), float2(+size.x, +size.y), float2(0.0f, +size.y), float2(-size.x, +size.y), float2(-2 * size.x, +size.y),
            float2(+2 * size.x, +2 * size.y), float2(+size.x, +2 * size.y), float2(0.0f, +2 * size.y), float2(-size.x, +2 * size.y), float2(-2 * size.x, +2 * size.y),

    };
    float weight[] =
    {
        1, 1, 2, 1, 1,
            1, 2, 4, 2, 1,
            2, 4, 8, 4, 2,
            1, 2, 4, 2, 1,
            1, 1, 2, 1, 1,
    };


    float sum = 0.0f;
    float totalweight = 0.0f;

    float3 uv = 0.0f;
    

   [unroll(9)]
    for (int i = 0; i < 25; i++)

    {
        uv = float3(UVD.xy + offsets[i], bestCascade);
        totalweight += weight[i];

      
        sum += CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(uv), UVD.z) * weight[i];

    }
    float factor = sum / totalweight;
  
    factor = saturate(factor + UVD.z);
  
   
    return factor;

}

void CalcRadiance(float3 position, float3 viewDir, float3 normalVec, float3 albedo, float roughness, float metallic, float3 lightPos, float3 lightCol, float3 F0, out float3 rad)
{
    static const float PI = 3.14159265359;

	//calculate light radiance
    float3 lightDir = normalize(lightPos - position);
    float3 halfwayVec = normalize(viewDir + lightDir);
    float distance = length(lightPos - position);
    float attenuation = 1.0f / (distance * distance);
    float3 radiance = lightCol * attenuation;

	//Cook-Torrance BRDF
    float D = NormalDistributionGGXTR(normalVec, halfwayVec, roughness);
    float G = GeometrySmith(normalVec, viewDir, lightDir, roughness);
    float3 F = FresnelSchlick(max(dot(halfwayVec, viewDir), 0.0f), F0);

    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0 - metallic;

    float3 nom = D * G * F;
    float denom = 4 * max(dot(normalVec, viewDir), 0.0f) * max(dot(normalVec, lightDir), 0.0) + 0.001f; // 0.001f just in case product is 0
    float3 specular = nom / denom;

	//Add to outgoing radiance Lo
    float NdotL = max(dot(normalVec, lightDir), 0.0f);
    rad = (((kD * albedo / PI) + specular) * radiance * NdotL);
}




float4 DirLightPS(VS_OUTPUT In) : SV_TARGET
{
    // Unpack the GBuffer
	SURFACE_DATA gbd = UnpackGBuffer_Loc(In.Position.xy);
	
	// Convert the data into the material structure
	Material mat;
	MaterialFromGBuffer(gbd, mat);

	// Reconstruct the world position
	float3 position = CalcWorldPos(In.cpPos, gbd.LinearDepth);
    float ao = SsaoTexture.Sample(LinearSampler, In.Uv);
	
    float3 albedo = mat.diffuseColor.rgb;

	
 
    float3 normalFromMap = mat.normal;

   
    float3 normalVec =mat.normal;
	
	//Metallic
    float metallic = mat.matallic;

	//Rough
    float rough = mat.roughness;
	
    //float3 viewDir = normalize(EyePosition() - position);
    float3 viewDir = -GlobalLight.Direction;
    float3 R = reflect(viewDir, normalVec);

    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metallic);

    float3 rad = float3(0.0f, 0.0f, 0.0f);
	//reflectance equation
    float3 Lo = float3(0.0f, 0.0f, 0.0f);

    CalcRadiance(position, viewDir, normalVec, albedo, rough, metallic, GlobalLight.Direction, float3(1, 1, 1), F0, rad);
    Lo += rad;
   
  

    float3 kS = FresnelSchlickRoughness(max(dot(normalVec, viewDir), 0.0f), F0, rough);
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0 - metallic;

    float3 irradiance = EnvMap.Sample(LinearSampler, normalVec).rgb;
    float3 diffuse = albedo ;

    const float MAX_REF_LOD = 1.0f;
    float3 prefilteredColor = skyPrefilter.SampleLevel(LinearSampler, R, rough * MAX_REF_LOD).rgb;
    //
    //
    float2 brdf = brdfLUT.Sample(LinearSampler, float2(max(dot(normalVec, viewDir), 0.0f), rough)).rg;
    float3 specular = prefilteredColor * (kS * brdf.x + brdf.y);

    float3 ambient = (kD * diffuse + specular) * ao;
    float3 color = ambient + Lo;


    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    color = pow(color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

    return float4(color , 1.0f);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//Point Vertex shader
/////////////////////////////////////////////////////////////////////////////


float4 PointLightVS() :SV_Position0
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
HS_OUTPUT PointLightHS( uint PatchID : SV_PrimitiveID)
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
float3 CalcPoint(float3 position, Material material, bool bUseShadow)
{
    float3 ambient = 0;
    float3 diffuse = 0;
    float3 specular = 0;
    float3 finalColor = 0;
    float3 ToLight = PointLight.Position - position;
    float3 ToEye = EyePosition() - position;
    float DistToLight = length(ToLight);
  
    //float bumpMap = saturate(dot(material.bump, -GlobalLight.Direction));
   
  
    // Phong diffuse
    ToLight /= DistToLight; // Normalize
    ambient = material.Ambient * PointLight.Ambient;
    float NDotL = saturate(dot(float3(ToLight.x, ToLight.y, ToLight.z), material.normal));
    diffuse = material.diffuseColor * PointLight.Diffuse * NDotL;

    // Blinn specular
    ToEye = normalize(ToEye);
    float3 HalfWay = normalize(ToEye + ToLight);
    float NDotH = saturate(dot(HalfWay, normalize(material.normal)));
    float3 R = normalize(reflect(-ToLight, material.normal));
    float RdotE = saturate(dot(R, normalize(ToEye)));
    specular = pow(NDotH, material.specIntensity) * material.specular.rgb * PointLight.Specular.rgb;


    // Attenuation
    float DistToLightNorm = 1.0 - saturate(DistToLight / PointLight.Range);
    float Attn = DistToLightNorm * DistToLightNorm * (1.0f / PointLight.Intensity);
    //diffuse *= diffuse;
    ambient += ambient;
    diffuse += diffuse * NDotL;
    //specular += specular ;
    
    finalColor = ambient + diffuse + specular;
  // finalColor *= diffuse;
    
   
   finalColor *= NDotL;
   finalColor *= Attn;
  //  finalColor *= finalColor;
    return finalColor;
}
float4 PointLightCommonPS(DS_OUTPUT In, bool bUseShadow) : SV_TARGET
{
    float3 finalColor = 0;
    
  
    SURFACE_DATA gbd = UnpackGBuffer_Loc(In.Position.xy);
   
	// Convert the data into the material structure
        Material mat;
        MaterialFromGBuffer(gbd, mat);
     
	// Reconstruct the world position
        float3 position = CalcWorldPos(In.cpPos, gbd.LinearDepth);
     

        //float3 dirColor = CalcDirectional(position, mat);
      
	// Calculate the light contribution
      
    finalColor = CalcPoint(position, mat,true);

	
    return float4(finalColor , 1.0f);
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

/////////////////////////////////////point states/////////////////////////////
RasterizerState RS
{
   // FillMode = WireFrame;
    CullMode = Front;
};


DepthStencilState pointDSS
{
    DepthEnable = true;
    DepthWriteMask = Zero;
    DepthFunc = Greater_Equal;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;
 
   
};

BlendState blendState
{
    BlendEnable[0] = true;

    DestBlend[0] = ONE;
//SRC_COLOR;
    SrcBlend[0] = ONE;
    BlendOp[0] = Add;

    DestBlendAlpha[0] = ONE;
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;

    
};




////////////////////////////////////////////////////////////////////////////////////////////
technique11 T0
{ 
    /*Cube*/
    P_VGP(P0, VS_Mesh_PreRender, GS_PreRender, main)
    P_VGP(P1, VS_Model_PreRender, GS_PreRender, main)
    P_VGP(P2, VS_Animation_PreRender, GS_PreRender, main)
    /*Shadow*/
    pass P3
    {
        SetRasterizerState(cascadedRS);
        //SetDepthStencilState(cascadedDSS,0);
        SetVertexShader(CompileShader(vs_5_0, VS_Mesh_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
    pass P4
    {
        SetRasterizerState(cascadedRS);
       // SetDepthStencilState(cascadedDSS,0);
        SetVertexShader(CompileShader(vs_5_0, VS_Model_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
    pass P5
    {

        SetRasterizerState(cascadedRS);
        //SetDepthStencilState(cascadedDSS, 0);
        SetVertexShader(CompileShader(vs_5_0, VS_Animation_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
         
    /*Deferred Packing */
//Sky
    P_DSS_VP(P6, SkyDSS, VS_Dome, PS_Dome)
    P_DSS_BS_VP(P7, SkyDSS, blendState, VS_Cloud, PS_Cloud)
   // P_BS_VP(P5, blendState, VS_Moon, PS_Moon)
//Mesh&Model
    P_VP(P8, VS_Mesh_PreRender, RenderScenePS)
    P_VP(P9, VS_Model_PreRender, RenderScenePS)
    P_VP(P10, VS_Animation_PreRender, RenderScenePS)
/////////////////////////////////////////////////////////
    pass P11
    {
        //SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

/////dirlight/////
    pass P12
    {
       // SetBlendState(blendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, DirLightVS()));
        SetPixelShader(CompileShader(ps_5_0, DirLightPS()));
    }
    pass P13
    {
        SetRasterizerState(RS);
        SetDepthStencilState(pointDSS,0);
        SetBlendState(blendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, PointLightVS()));
        SetHullShader(CompileShader(hs_5_0, PointLightHS()));
        SetDomainShader(CompileShader(ds_5_0, PointLightDS()));
        SetPixelShader(CompileShader(ps_5_0, PointLightShadowPS()));
    }
   
   
}
