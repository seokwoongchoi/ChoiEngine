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
    matrix Sepia;
    float Strength;
};
float4 PS_Full(VertexOutput input) : SV_TARGET0
{
    
    float4 pixel = DiffuseMap.Sample(LinearSampler, input.Uv);
    return mul(pixel, Sepia) ;
   
    //return float4(Noise, Noise, Noise,1.0f);
   // return float4(pixel.rgb, 1.0f);
}
float4 PS_Partial(VertexOutput input) : SV_TARGET0
{
    
    float4 pixel = DiffuseMap.Sample(LinearSampler, input.Uv);
    return lerp(pixel, mul(pixel, Sepia), Strength);
   
    //return float4(Noise, Noise, Noise,1.0f);
   // return float4(pixel.rgb, 1.0f);
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
 
    P_DSS_VP(P0, Depth, VS, PS_Partial)
   // P_DSS_VP(P1, Depth, VS, PS_Partial)
   // P_DSS_VP(P2, Depth, VS, PS_Distrot)
   
}