
#include "000_Header.fx"

Texture2D BaseMap;
struct VertexOutput
{
    float4 Position : SV_Position0;
    float3 Normal : Normal0;
    float2 Uv : Uv0;
   
};

VertexOutput VS(VertexTextureNormal input)
{
    //WVP ��ȯ�� VS����
    VertexOutput output;
    //mul : ��İ����� ���Ͱ�
    output.Position = mul(input.Position, World);
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);

    output.Normal = mul(input.Normal, (float3x3) World);
    output.Uv = input.Uv;
    
    return output;
}

float4 PS(VertexOutput input) : SV_Target0
{
    float4 diffuse = BaseMap.Sample(LinearSampler, input.Uv);
   
    
    float3 normal = normalize(input.Normal);
    float NdotL = saturate(dot(normal, -LightDirection)); 

   return diffuse * NdotL;
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