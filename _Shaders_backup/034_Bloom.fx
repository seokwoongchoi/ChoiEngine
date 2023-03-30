#include "000_Header.fx"
#include "000_Light.fx"
#define MAX_BLOOM_COUNT 63
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
struct BlurDesc
{
    float2 Offset;
    float Weight;
    float Padding;
};
cbuffer CB_Values
{
    float2 MapSize;
    float ThresHold;
    float Intensity;
    uint BlurCount;

    float3 CB_Values_Padding;
    BlurDesc BlurX[MAX_BLOOM_COUNT];
    BlurDesc BlurY[MAX_BLOOM_COUNT];
}
float4 PS_Luminosity(VertexOutput input) : SV_TARGET0
{
  
    float4 color = DiffuseMap.Sample(LinearSampler, input.Uv);
   
  
    return saturate(saturate((color - ThresHold) / (1 - ThresHold)) * 2.0f);
}

struct PS_Output
{
    float4 Color : SV_Target0;
    float4 Color2 : SV_Target1;
};
PS_Output PS_Blur(VertexOutput input) 
{
    
    float2 uv = 0;

    float4 colorX = 0;
    float4 colorY = 0;
    uint count = BlurCount * 2 - 1;
    for (int x = 0; x < count;x++)
    {
        uv = input.Uv + BlurX[x].Offset;
        
        colorX += DiffuseMap.Sample(LinearSampler, uv) * BlurX[x].Weight;
    }
    for (int y = 0; y < count; y++)
    {
        uv = input.Uv + BlurY[y].Offset;
        
        colorY += DiffuseMap.Sample(LinearSampler, uv) * BlurY[y].Weight;
        

    }

    PS_Output output;
    output.Color = colorX;
    output.Color2 = colorY;
    return output;
    
}
Texture2D LuminosityMap;
Texture2D BlurXMap;
Texture2D BlurYMap;
float4 PS_Composite(VertexOutput input) : SV_TARGET0
{
    float4 l = LuminosityMap.Sample(LinearSampler, input.Uv) * Intensity;
    float4 x=  BlurXMap.Sample(LinearSampler, input.Uv);
    float4 y = BlurYMap.Sample(LinearSampler, input.Uv);
    
    float4 b = (x + y) * 0.5f;
    b *= (1 - saturate(l));
   
    return b + l;
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
 
    P_DSS_VP(P0, Depth, VS, PS_Luminosity)
    P_DSS_VP(P1, Depth, VS, PS_Blur)
    P_DSS_VP(P2, Depth, VS, PS_Composite)
}