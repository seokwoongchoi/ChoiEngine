Matrix World;
Matrix View;
Matrix Projection;

float4 Color;

Texture2D Map;

struct VertexInput
{
    float4 Position : Position0;
    float2 Uv : Uv0;
};

struct VertexOutput
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
};

VertexOutput VS(VertexInput input)
{
    //WVP º¯È¯Àº VS¿¡¼­
    VertexOutput output;
    //mul : Çà·Ä°öÀÌÀÚ º¤ÅÍ°ö
    output.Position = mul(input.Position, World);
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);

    output.Uv = input.Uv;
    return output;
}
SamplerState Sampler;
float4 PS(VertexOutput input) : SV_Target0
{
    return Map.Sample(Sampler,input.Uv);
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