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

float4 PS_Inverse(VertexOutput input) : SV_TARGET0
{
    float4 pixel = DiffuseMap.Sample(LinearSampler, input.Uv);
   
  
    return float4(1.0f - pixel.rgb, 1.0f);
}
float4 PS_Grayscale(VertexOutput input) : SV_TARGET0
{
    float4 pixel = DiffuseMap.Sample(LinearSampler, input.Uv);
    float color = (pixel.r + pixel.g + pixel.b) / 3.0f;
    return float4(color, color, color, 1.0f); //단색화
    
}

float4 PS_Grayscale2(VertexOutput input) : SV_TARGET0
{
    float4 pixel = DiffuseMap.Sample(LinearSampler, input.Uv);
    float color = dot(pixel.rgb, float3(0.299f, 0.576f, 0.114f));
    return float4(color, color, color, 1.0f); //단색화
    
}

float4 Tone;
float4 PS_Tone(VertexOutput input) : SV_TARGET0
{
    float4 pixel = DiffuseMap.Sample(LinearSampler, input.Uv);
  
    pixel.r *= Tone.r;
    pixel.g *= Tone.g;
    pixel.b *= Tone.b;
    return float4(pixel.rgb, 1.0f); 
    
}
float4 Gamma;
float4 PS_Gamma(VertexOutput input) : SV_TARGET0
{
    float4 pixel = DiffuseMap.Sample(LinearSampler, input.Uv);
    pixel.r = pow(pixel.r, 1.0f / Gamma.r);
    pixel.g = pow(pixel.g, 1.0f / Gamma.g);
    pixel.b = pow(pixel.b, 1.0f / Gamma.b);

    return float4(pixel.r, pixel.g, pixel.b, 1.0f);
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
 
    P_DSS_VP(P0, Depth, VS, PS_Inverse)
    P_DSS_VP(P1, Depth, VS, PS_Grayscale)
    P_DSS_VP(P2, Depth, VS, PS_Grayscale2)
    P_DSS_VP(P3, Depth, VS, PS_Tone)
    P_DSS_VP(P4, Depth, VS, PS_Gamma)
  
}