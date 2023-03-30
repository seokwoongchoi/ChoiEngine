#include "000_Header.fx"
#include "000_Light.fx"

float clipFactor;

struct VertexOutput
{
    float4 Position : SV_Position0;
    float3 wPosition : Position1;
    float4 Color : COLOR0;
   
};

VertexOutput VS(VertexColor input)
{
    //WVP 변환은 VS에서
    VertexOutput output;
    //mul : 행렬곱이자 벡터곱
    output.Position = WorldPosition(input.Position);
    output.wPosition = output.Position; //swizzling
    output.Position = ViewProjection(output.Position);
   
    output.Color = input.Color;
       
    return output;
}

float4 PS(VertexOutput input) : SV_Target0
{
    float4 diffuse = float4(input.Color.xyz, 0);
   
    
  
   // float3 brushColor = GetBrushColor(input.wPosition); //rasterizer에서 보간이 일어나서 깔금하게나온다.
    clip(diffuse.a + clipFactor);
    return diffuse; //a가 1이면 0.1가 되고 clip함수는 내부가 음수가되면 전부 처내버린다.
  //  return float4(gridColor, 1);
   //return float4(LightDirection*0.5f+0.5f, 1); //체크용 0~1의 범위로 빛이들어오는지 체크

}

RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
    pass P0
    {
        
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

    pass P1
    {
        SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}