#include "000_Header.fx"
#include "000_Light.fx"
#define MAX_RADIAL_BLUR_COUNT 32
cbuffer CB_Render2D
{
    matrix View2D;
    matrix Projection2D;
};

//cbuffer CB_Default
//{
//    float4 Tone;
//};


//-----------------------------------------------------------------------------
// Pass0
//-----------------------------------------------------------------------------
struct VertexOutput
{
    float4 Position : SV_POSITION0;
    float2 Uv : Uv0;
};

VertexOutput VS(VertexTexture input)
{
    VertexOutput output;

    output.Position = WorldPosition(input.Position);
    output.Position = mul(output.Position, View2D);
    output.Position = mul(output.Position, Projection2D);
    output.Uv = input.Uv;

    return output;
}
cbuffer CB_Values
{
    float2 MapSize;
    uint BlurCount;
    float Radius;
    float Amount;
    float Offset;
}
Texture2D BlurMap0;
Texture2D BlurMap1;
Texture2D BlurMap2;
Texture2D BlurMap3;
Texture2D BlurMap4;
Texture2D BlurMap5;
Texture2D BlurMap6;
Texture2D BlurMap7;
float4 PS(VertexOutput input) : SV_TARGET0
{
    float2 radius = input.Uv - float2(0.5f, 0.5f);//중심
    float r = length(radius) + Offset;
    radius /= r;
    r = 2 * r / Radius; //퍼질범위
    r = saturate(r);
    
    float delta = radius * r * r * Amount / BlurCount;
    delta = -delta;

    float4 color = 0;
    [roll(MAX_RADIAL_BLUR_COUNT)]
  //  for (int i = 0; i < BlurCount;i++)
    {
        color += DiffuseMap.Sample(LinearSampler, input.Uv + delta);
        color += BlurMap0.Sample(LinearSampler, input.Uv + delta);
        color += BlurMap1.Sample(LinearSampler, input.Uv + delta);
        color += BlurMap2.Sample(LinearSampler, input.Uv + delta);
        color += BlurMap3.Sample(LinearSampler, input.Uv + delta);
        color += BlurMap4.Sample(LinearSampler, input.Uv + delta);
        color += BlurMap5.Sample(LinearSampler, input.Uv + delta);
        color += BlurMap6.Sample(LinearSampler, input.Uv + delta);
        color += BlurMap7.Sample(LinearSampler, input.Uv + delta);
        //input.Uv += delta;

    }
    color /= BlurCount; //평균을 낸다.
  
    return float4(color.rgb, 1.0f);
}

//-----------------------------------------------------------------------------
// Techniques
//-----------------------------------------------------------------------------
DepthStencilState Depth
{
    DepthEnable = false;
};

technique11 T0
{
 
    P_DSS_VP(P0, Depth, VS, PS)
   
}