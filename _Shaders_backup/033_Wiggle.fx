#include "000_Header.fx"
#include "000_Light.fx"

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
    float2 Offset;
    float2 Amount;
}
float4 PS_Wiggle(VertexOutput input) : SV_TARGET0
{
    float2 uv = input.Uv;
    uv.x += sin(Time + uv.x * Offset.x) * Amount.x;
    uv.y += cos(Time + uv.x * Offset.y) * Amount.y + 0.1f;
    float4 pixel = DiffuseMap.Sample(LinearSampler, uv);
   
  
    return float4(pixel.rgb, 1.0f);
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
 
    P_DSS_VP(P0, Depth, VS, PS_Wiggle)
   
}