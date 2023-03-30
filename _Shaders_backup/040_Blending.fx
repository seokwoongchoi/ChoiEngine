#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"


uint Selected1;

cbuffer CB_Render2D
{
    matrix View2D;
    matrix Projection2D;
};

//-----------------------------------------------------------------------------
// Pass0
//-----------------------------------------------------------------------------
struct VertexOutput
{
    float4 Position : SV_POSITION0;
    float2 Uv : Uv0;
};

VertexOutput VS(VertexTextureNormalTangent input)
{
    VertexOutput output;

    output.Position = WorldPosition(input.Position);
    output.Position = mul(output.Position, View2D);
    output.Position = mul(output.Position, Projection2D);
    output.Uv = input.Uv;

    return output;
}

float4 PS(VertexOutput input) : SV_TARGET0
{
    return DiffuseMap.Sample(LinearSampler, input.Uv);
}

//-----------------------------------------------------------------------------
// Techniques
//-----------------------------------------------------------------------------
DepthStencilState Depth
{
    DepthEnable = false;
};

BlendState Blend;

technique11 T0
{
    pass P0
    {
        SetDepthStencilState(Depth, 0);

        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

    pass P1
    {
        SetDepthStencilState(Depth, 0);
        SetBlendState(Blend, float4(0, 0, 0, 0), 0xFF);

        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}
///////////////////////////////////////////////////////////////////////////////////
//#include "000_Header.fx"
//#include "000_Light.fx"

//uint Selected;
//Texture2D Maps[2];

//cbuffer CB_Render2D
//{
//    matrix View2D;
//    matrix Projection2D;
//};

////-----------------------------------------------------------------------------
//// Pass0
////-----------------------------------------------------------------------------
//struct VertexOutput
//{
//    float4 Position : SV_POSITION0;
//    float2 Uv : Uv0;
//};

//VertexOutput VS(VertexTextureNormalTangent input)
//{
//    VertexOutput output;

//    output.Position = WorldPosition(input.Position);
//    output.Position = mul(output.Position, View2D);
//    output.Position = mul(output.Position, Projection2D);
//    output.Uv = input.Uv;

//    return output;
//}


//float4 Modulate(float4 arg1, float4 arg2)
//{
//    return arg1 * arg2;
//}

//float4 Modulate2X(float4 arg1, float4 arg2)
//{
//    return arg1 * arg2 * 2.0f;
//}

//float4 Modulate4X(float4 arg1, float4 arg2)
//{
//    return arg1 * arg2 * 4.0f;
//}

//float4 Add(float4 arg1, float4 arg2)
//{
//    return arg1 + arg2;
//}

//float4 AddSmooth(float4 arg1, float4 arg2)
//{
//    return (arg1 + arg2) - (arg1 * arg2);
//}

//float4 AddSigned(float4 arg1, float4 arg2)
//{
//    return arg1 + arg2 - 0.5f;
//}

//float4 AddSigned2X(float4 arg1, float4 arg2)
//{
//    return (arg1 + arg2 - 0.5f) * 2.0f;
//}

//float4 Subtract(float4 arg1, float4 arg2)
//{
//    return arg1 - arg2;
//}

//float4 InvSubtract(float4 arg1, float4 arg2)
//{
//    return arg2 - arg1;
//}

//float4 InvSelect1(float4 arg1, float4 arg2)
//{
//    return 1.0f - arg1;
//}

//float4 InvSelect2(float4 arg1, float4 arg2)
//{
//    return 1.0f - arg2;
//}

//float4 InvAdd(float4 arg1, float4 arg2)
//{
//    return 1.0f - (arg1 + arg2);
//}

//float4 PS(VertexOutput input) : SV_TARGET0
//{
//    float4 color = 0;

//    float4 arg1 = Maps[0].Sample(LinearSampler, input.Uv);
//    float4 arg2 = Maps[1].Sample(LinearSampler, input.Uv);

//    color = arg1;

//    [branch]
//    switch (Selected)
//    {
//        case 0:
//            color = Modulate(arg1, arg2);
//            break;

//        case 1:
//            color = Modulate2X(arg1, arg2);
//            break;

//        case 2:
//            color = Modulate4X(arg1, arg2);
//            break;

//        case 3:
//            color = Add(arg1, arg2);
//            break;

//        case 4:
//            color = AddSmooth(arg1, arg2);
//            break;

//        case 5:
//            color = AddSigned(arg1, arg2);
//            break;

//        case 6:
//            color = AddSigned2X(arg1, arg2);
//            break;

//        case 7:
//            color = Subtract(arg1, arg2);
//            break;

//        case 8:
//            color = InvSubtract(arg1, arg2);
//            break;

//        case 9:
//            color = InvSelect1(arg1, arg2);
//            break;

//        case 10:
//            color = InvSelect2(arg1, arg2);
//            break;

//        case 11:
//            color = InvAdd(arg1, arg2);
//            break;
//    }

//    return float4(color.rgb, 1);
//}

////-----------------------------------------------------------------------------
//// Techniques
////-----------------------------------------------------------------------------
//DepthStencilState Depth
//{
//    DepthEnable = false;
//};

//technique11 T0
//{
//    pass P0
//    {
//        SetDepthStencilState(Depth, 0);

//        SetVertexShader(CompileShader(vs_5_0, VS()));
//        SetPixelShader(CompileShader(ps_5_0, PS()));
//    }
//}