Matrix World;
Matrix View;
Matrix Projection;

float4 Color;


struct VertexInput
{
    float4 Position : Position0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
};

struct VertexOutput
{
    float4 Position : SV_Position0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
};

VertexOutput VS(VertexInput input)
{
    //WVP 변환은 VS에서
    VertexOutput output;
    //mul : 행렬곱이자 벡터곱
    output.Position = mul(input.Position, World);
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);
    
    output.Tangent = input.Tangent;
    output.Normal =input.Normal; //노말을 월드변환하는이유 회전한만큼 방향을 다시 갱신하기위해
    //위치값은 필요없으므로 3x3행렬로 월드변환
    return output;
}

float4 PS(VertexOutput input) : SV_Target0
{
    return float4(input.Normal, 1);
}

RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
    pass P0
    {
        SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

    pass P1
    {
       
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}