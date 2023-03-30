#include "000_Header.fx"

float2 Uv;

struct VertexOutput
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
};

VertexOutput VS(VertexTexture input)
{
    //WVP 변환은 VS에서
    VertexOutput output;
    //mul : 행렬곱이자 벡터곱
    output.Position = WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);
   
    output.Uv = input.Uv;
    return output;
}

float4 PS(VertexOutput input) : SV_Target0
{
   [flatten]
    if (input.Uv.x < Uv.x)
        return float4(0, 0, 0, 1);


    return DiffuseMap.Sample(Sampler, input.Uv);
}

RasterizerState RS
{
    FillMode = Wireframe;

};

////////////////////////////////////////////////////////////////////////////////////
SamplerState Address_Wrap
{
    AddressU = WRAP; //가로로 늘어날때
    AddressV = WRAP; //세로로 늘어날때

  
};
SamplerState Address_MIRROR
{
    AddressU = MIRROR; //가로로 늘어날때
    AddressV = MIRROR; //세로로 늘어날때

  
};

SamplerState Address_CLAMP
{
    AddressU = CLAMP; //가로로 늘어날때
    AddressV = CLAMP; //세로로 늘어날때

  
};

SamplerState Address_BORDER
{
    AddressU = BORDER; //가로로 늘어날때
    AddressV = BORDER; //세로로 늘어날때
    
    BorderColor = float4(0, 0, 1, 1);
  
};


uint Address;
float4 PS_Address(VertexOutput input) : SV_Target0
{
    [branch]
    switch (Address)
    {
        case 0:
            return DiffuseMap.Sample(Address_Wrap, input.Uv);

        case 1:
            return DiffuseMap.Sample(Address_MIRROR, input.Uv);

        case 2:
            return DiffuseMap.Sample(Address_CLAMP, input.Uv);

        case 3:
            return DiffuseMap.Sample(Address_BORDER, input.Uv);
    }
    return float4(0, 0, 0, 1);
};


////////////////////////////////////////////////////////////////////////////////////
SamplerState Point
{
    Filter = MIN_MAG_MIP_POINT;
};

SamplerState Linear
{
    Filter = MIN_MAG_MIP_LINEAR;
};


uint Filter;
float4 PS_Filter(VertexOutput input) : SV_Target0
{
    [branch]
    switch (Filter)
    {
        case 0:
            return DiffuseMap.Sample(Point, input.Uv);
        case 1:
            return DiffuseMap.Sample(Linear, input.Uv);

       
    }
    return float4(0, 0, 0, 1);
};

////////////////////////////////////////////////////////////////////////////////////
technique11 T0
{
    pass P0
    {
        
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

    pass P1
    {
       // SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS_Address()));
    }

    pass P2
    {
       // SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS_Filter()));
    }
}