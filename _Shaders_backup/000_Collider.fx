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
    //WVP ��ȯ�� VS����
    VertexOutput output;
    //mul : ��İ����� ���Ͱ�
    output.Position = WorldPosition(input.Position);
    output.wPosition = output.Position; //swizzling
    output.Position = ViewProjection(output.Position);
   
    output.Color = input.Color;
       
    return output;
}

float4 PS(VertexOutput input) : SV_Target0
{
    float4 diffuse = float4(input.Color.xyz, 0);
   
    
  
   // float3 brushColor = GetBrushColor(input.wPosition); //rasterizer���� ������ �Ͼ�� ����ϰԳ��´�.
    clip(diffuse.a + clipFactor);
    return diffuse; //a�� 1�̸� 0.1�� �ǰ� clip�Լ��� ���ΰ� �������Ǹ� ���� ó��������.
  //  return float4(gridColor, 1);
   //return float4(LightDirection*0.5f+0.5f, 1); //üũ�� 0~1�� ������ ���̵������� üũ

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