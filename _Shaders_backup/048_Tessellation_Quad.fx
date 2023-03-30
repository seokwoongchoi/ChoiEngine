#include "000_Header.fx"
#include "000_Light.fx"

uint Factor[6];
struct VertexOutput
{
    float4 Position : Posiiton0;
    
};

VertexOutput VS(Vertex input)
{
    VertexOutput output;
    output.Position = input.Position;

    return output;
}

struct ConstantHullOutput
{
    float Edge[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;

};


ConstantHullOutput HS_Constant(InputPatch<VertexOutput,4>input)
{
    //어떻게 짜를지 정함
    ConstantHullOutput output;
    output.Edge[0] = Factor[0];//이 선을 2개로 분할한다.
    output.Edge[1] = Factor[1];
    output.Edge[2] = Factor[2];
    output.Edge[3] = Factor[3];

    output.Inside[0] = Factor[4];
    output.Inside[1] = Factor[5];

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
HullOutput HS(InputPatch<VertexOutput, 4> input, uint point0 : SV_OutputControlPointID)
{
    //짜르기전에 할 행동
    HullOutput output;
    output.Position = input[point0].Position;

    return output;
}

struct DomainOutput
{
    float4 Position : SV_Position0;
};

[domain("quad")]
DomainOutput DS(ConstantHullOutput input, float2 uv : SV_DomainLocation, const OutputPatch<HullOutput,4>patch)
{
    DomainOutput output;
    
    float3 v0 = lerp(patch[0].Position.xyz, patch[1].Position.xyz, 1 - uv.y);
    float3 v1 = lerp(patch[2].Position.xyz, patch[3].Position.xyz, 1 - uv.y);
    float3 position = lerp(v0, v1, uv.x);
    output.Position = float4(position, 1);

    return output;

}


float4 PS(DomainOutput input):SV_Target0
{
    return float4(1, 0, 0, 1);
}

RasterizerState RS
{
    FillMode = wireFrame;
};

technique11 T0
{
    pass P0
    {
        SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}