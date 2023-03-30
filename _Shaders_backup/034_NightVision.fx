#include "000_Header.fx"
#include "000_Light.fx"

cbuffer CB_Render2D
{
    matrix View2D;
    matrix Projection2D;
};
cbuffer CB_Values
{
    float4 Color;
    float3 Mono;
    float Weight;
    float2 MapSize;
    float Brightness;
};
Texture2D NightVision;
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

float4 PS_NightVision(VertexOutput input) : SV_TARGET0
{
    float2 uvs[9] =
    {
        float2(-1, -1), float2(+0, -1), float2(+1, -1),
        float2(-1, +0), float2(+0, +0), float2(+1, +0),
        float2(-1, +1), float2(+0, +1), float2(+1, +1),
    };
    /* 라플라시안 마스크? */
    float weights[9] =
    {
        -1.0f, -1.0f, -1.0f,
        -1.0f, +8.0f, -1.0f,
        -1.0f, -1.0f, -1.0f
    };

    for (int i = 0; i < 9; i++)
        weights[i] *= Weight;
    
    float2 offset = 1.0f / MapSize;
    float4 color = 0;
    
    for (int k = 0; k < 9; k++)
    {
        float2 uv = uvs[k] * offset;
        float4 pixel = DiffuseMap.Sample(LinearSampler, input.Uv + uv);
        color += (pixel * weights[k]);
    }
    color = abs(saturate(color));
    color = dot(color.rgb, Mono);
    color = saturate(color* Color) * Brightness;
  
    float4 map = NightVision.Sample(LinearSampler, input.Uv);
    color.rgb *= map.rgb;
    return color;
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
    P_DSS_VP(P0, Depth, VS, PS_NightVision)
}