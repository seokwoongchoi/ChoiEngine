#include "000_Header.fx"
#include "000_Model.fx"
#include "common.fx"
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

    matrix WorldViewProj;
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

    //output.Position = WorldPosition(input.Position);
    //output.Position = ViewProjection(output.Position);
    output.Position = mul(input.Position, WorldViewProj);
    output.oPosition = -input.Position;
    output.Uv = input.Uv;

    return output;
}


Texture2D StarMap;

float4 PS_Dome(VertexOutput_Dome input):SV_Target0
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

    //color += max(0, (1 - color.rgb)) * float3(0.05f, 0.05f, 0.1f);
    float4 finalColor = float4(color, 1) + StarMap.Sample(LinearSampler, input.Uv) * saturate(StarIntensity);
    return finalColor;

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
    float CloudCover;
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

    output.Position = mul(input.Position, WorldViewProj);
    output.Uv = input.Uv;

    return output;
}


Texture2D MoonMap;

float4 PS_Moon(VertexOutput_Moon input) : SV_Target1
{
    float4 color = MoonMap.Sample(LinearSampler, input.Uv);
    color.a *= MoonAlpha;

    return color;
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

    output.Position = mul(input.Position, WorldViewProj);

    output.Uv = (input.Uv * CloudTiles);
    output.oUv = input.Uv;

    return output;
}

float4 PS_Cloud(VertexOutput_Cloud input):SV_Target2
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
  
    return color;
}


///////////////////////////////////////////////////////////////////////////////


DepthStencilState SkyDSS
{
    DepthEnable = false;

};

BlendState AlphaBlend
{
    BlendEnable[0] = true;
    DestBlend[0] = INV_SRC_ALPHA;
    SrcBlend[0] = SRC_ALPHA;
    BlendOp[0] = Add;

    SrcBlendAlpha[0] = One;
    DestBlendAlpha[0] = One;
    RenderTargetWriteMask[0] = 0x0F;
};
technique11 T0
{
  
    P_DSS_VP(P0, SkyDSS, VS_Dome, PS_Dome)
    P_DSS_BS_VP(P1, SkyDSS, AlphaBlend, VS_Cloud, PS_Cloud)
    P_BS_VP(P2, AlphaBlend, VS_Moon, PS_Moon)
}