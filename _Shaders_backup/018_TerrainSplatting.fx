#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Terrain.fx"




float4 PS(MeshOutput input) : SV_Target0
{
    ////////////////////////////////////////////////////////////////////////////////
    Texture(Material.Diffuse, DiffuseMap, input.Uv);
    NormalMapping(input.Uv, input.Normal, input.Tangent); //�������� �Ѿ�� ���ڵ�� 
    //TBN���������.
    Texture(Material.Specular, SpecularMap, input.Uv);
    MaterialDesc output = MakeMaterial();
    MaterialDesc result = MakeMaterial();
    ComputeLight(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
    ComputePointLights(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
    ComputeSpotLights(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
    ComputeCapsuleLights(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
 //////////////////////////////////////////////////////////////////////////////////////////////////////
    float4 diffuse = 0;
    //float4 base = GetBaseColor(input.Uv);
    float4 layer = LayerMap.Sample(LinearSampler, input.Uv);
    float4 layer2 = LayerMap2.Sample(LinearSampler, input.Uv);
    float4 layer3 = LayerMap3.Sample(LinearSampler, input.Uv);
    float4 layer4 = float4(MaterialToColor(result), 1.0f);
  
   
    layer = layer * input.Alpha.r;
    layer2 = lerp(layer, layer2, input.Alpha.g);
    layer3 = lerp(layer2, layer3, input.Alpha.b);
    layer4 = lerp(layer3, layer4, input.Alpha.a);
    
   // return float4(input.Color, 1);
   //return float4(rate, rate, rate, 1);
    diffuse = layer4;
  
    
    float3 normal = normalize(input.Normal);
    float NdotL = saturate(dot(normal, -GlobalLight.Direction));

    float3 brushColor = GetBrushColor(input.wPosition); //rasterizer���� ������ �Ͼ�� ����ϰԳ��´�.
    float3 gridColor = GetLineColor(input.wPosition);
    float3 targetColor = GetTargetPos(input.wPosition);
   
   
    return (diffuse * NdotL)+ float4(gridColor, 1) + float4(brushColor, 1) + float4(targetColor, 1);
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
        
        SetVertexShader(CompileShader(vs_5_0, VS_Terrain()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

    pass P1
    {
        SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS_Terrain()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
    
}