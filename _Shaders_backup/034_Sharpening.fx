#include "000_Header.fx"
#include "000_Light.fx"

cbuffer CB_Render2D
{
    matrix View2D;
    matrix Projection2D;
};
cbuffer CB_Values
{
    float2 MapSize;
    float Sharpening;
};

//-----------------------------------------------------------------------------
// Pass0
//-----------------------------------------------------------------------------
struct VertexOutput
{
    float4 Position : SV_POSITION0;
    float2 Uv : Uv0;
    float2 Uv1 : Uv1;
    float2 Uv2 : Uv2;
    float2 Uv3 : Uv3;
    float2 Uv4 : Uv4;
};

VertexOutput VS(VertexTexture input)
{
    
    VertexOutput output;

    output.Position = WorldPosition(input.Position);
    output.Position = mul(output.Position, View2D);
    output.Position = mul(output.Position, Projection2D);
    

    float2 offset = 1 / MapSize;

    output.Uv = input.Uv;
    output.Uv1 = input.Uv + float2(0, -offset.y);
    output.Uv2 = input.Uv + float2(-offset.x, 0);
    output.Uv3 = input.Uv + float2(+offset.x, 0);
    output.Uv4 = input.Uv + float2(0, +offset.y);

   return output;
}


float4 PS_Sharpen(VertexOutput input) : SV_TARGET0
{
    //float4 center = DiffuseMap.Sample(LinearSampler, input.Uv);
    //float4 top = DiffuseMap.Sample(LinearSampler, input.Uv1);
    //float4 left = DiffuseMap.Sample(LinearSampler, input.Uv2);
    //float4 right = DiffuseMap.Sample(LinearSampler, input.Uv3);
    //float4 bottom = DiffuseMap.Sample(LinearSampler, input.Uv4);
    float2 uvs[9] =
    {
        float2(-1, -1), float2(+0, -1), float2(+1, -1),
        float2(-1, +0), float2(+0, +0), float2(+1, +0),
        float2(-1, +1), float2(+0, +1), float2(+1, +1),
    };
    float weights[9] =
    {
        0, -1, 0,
        -1, 4, -1,
         0, -1, 0
    };

  
    float2 offset = 1 / MapSize;
    float3 pixel = 0;
    float4 color = 0;
    for (int i = 0; i < 9; i++)
    {

        float2 uv = uvs[i] * offset;
        color = DiffuseMap.Sample(LinearSampler, input.Uv + uv);
        pixel -= color * weights[i];
     
        
    }
    //float4 color = 4 * center - top - bottom - left - right;
    //return center + Sharpening * color;
    return color + float4(Sharpening * pixel, 1);

}
float4 PS_Embossing(VertexOutput input) : SV_TARGET0
{
    float2 ratio = float2
    (
        1.0f / (float) MapSize.x,
        1.0f / (float) MapSize.y
    );
   
    float3x3 k =
    {
        2, 1, 0,
        1, 0, -1,
        0, -1, -2
    };

    float res = 0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float2 offset = float2(x, y) * ratio;
            float3 tex = DiffuseMap.Sample(LinearSampler, input.Uv + offset).rgb;
            float luminance = dot(tex, float3(0.3f, 0.59f, 0.11f));
         
            res += luminance * k[y + 1][x + 1];
        }
    }
   
    res += 0.5f;

    return float4(res,res,res, 1);
    
}
float count;
float amount;
float4 PS_Blurring(VertexOutput input) : SV_TARGET0
{
    float4 color = 0;

    for (int i = 0; i < count; i++)
    {
        color += DiffuseMap.Sample(LinearSampler, input.Uv);
        input.Uv -= (amount / count) * 0.01f;
    }

    color /= count;

    return float4(color.rgb, 1);

}

int nBit;

float4 PS_BitPlanner(VertexOutput input) : SV_TARGET0
{
    float4 pixel = DiffuseMap.Sample(LinearSampler, input.Uv);

    dword r = dword(pixel.r * 255.0f);
    dword g = dword(pixel.g * 255.0f);
    dword b = dword(pixel.b * 255.0f);

    r >>= nBit;
    g >>= nBit;
    b >>= nBit;
          
    r <<= nBit;
    g <<= nBit;
    b <<= nBit;

    float R, G, B;

    R = r / 255.0f;
    G = g / 255.0f;
    B = b / 255.0f;

    return float4(R, G, B, 1.0f);
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
    P_DSS_VP(P0, Depth, VS, PS_Sharpen)
    P_DSS_VP(P1,Depth, VS, PS_Embossing)
    P_DSS_VP(P2, Depth, VS, PS_Blurring)
    P_DSS_VP(P3, Depth, VS, PS_BitPlanner)
}