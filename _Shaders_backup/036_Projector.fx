#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"
#include "000_Terrain.fx"

cbuffer CB_Projector
{
    matrix ProjectorView;
    matrix Projection2;

    float4 ProjectorColor;
}
Texture2D ProjectorMap;

void ProjectorPosition(inout float4 wvp,float4 position)
{
    wvp = WorldPosition(position);
    wvp = mul(wvp, ProjectorView); //프로젝터용 뷰로 월드를 변환
    wvp = mul(wvp, Projection2); //프로젝터용 프로젝션으로 월드를 변환
}
MeshOutput VS_Projector_Terrain(VertexTextureAlphaNormalTangent input)
{
    MeshOutput output = VS_Terrain(input);
    ProjectorPosition(output.wvpPosition, input.Position);
    return output;
}
MeshOutput VS_Projector_Mesh(VertexMesh input)
{
    MeshOutput output = VS_Mesh(input);
    ProjectorPosition(output.wvpPosition,input.Position);
    return output;
}
MeshOutput VS_Projector_Model(VertexModel input)
{
    MeshOutput output = VS_Model(input);
    ProjectorPosition(output.wvpPosition, input.Position);
    return output;
}
MeshOutput VS_Projector_Animation(VertexModel input)
{
    MeshOutput output = VS_Animation(input);
    ProjectorPosition(output.wvpPosition, input.Position);
    return output;
}
float4 PS(MeshOutput input) : SV_Target0
{
    float4 color = VS_AllLight(input);

    float2 uv = 0;
    uv.x = input.wvpPosition.x / input.wvpPosition.w * 0.5f + 0.5f;
    uv.y = -input.wvpPosition.y / input.wvpPosition.w * 0.5f + 0.5f;
    [flatten]
    if (saturate(uv.x) == uv.x && saturate(uv.y) == uv.y)
    {
        float4 map = ProjectorMap.Sample(LinearSampler, uv);
       
        map *= ProjectorColor;
        color = lerp(color, map, map.a);
    }
   
   return color;
    

}
float4 PS_Terrain(MeshOutput input) : SV_Target0
{
    ////////////////////////////////////////////////////////////////////////////////
    Texture(Material.Diffuse, DiffuseMap, input.Uv);
    NormalMapping(input.Uv, input.Normal, input.Tangent); //정점에서 넘어온 인자들로 
    //TBN축을만든다.
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

    float3 brushColor = GetBrushColor(input.wPosition); //rasterizer에서 보간이 일어나서 깔금하게나온다.
    float3 gridColor = GetLineColor(input.wPosition);
    float3 targetColor = GetTargetPos(input.wPosition);
    float4 color = 0;
    float2 uv = 0;

    uv.x = input.wvpPosition.x / input.wvpPosition.w * 0.5f + 0.5f;
    uv.y = -input.wvpPosition.y / input.wvpPosition.w * 0.5f + 0.5f;
    [flatten]
    if (saturate(uv.x) == uv.x && saturate(uv.y) == uv.y)
    {
        float4 map = ProjectorMap.Sample(LinearSampler, uv);
       
        map *= ProjectorColor;
        color = lerp(color, map, map.a);
    }
   
    return (diffuse * NdotL) + float4(gridColor, 1) + float4(brushColor, 1) + float4(targetColor, 1)+color;
   
   
  

}

RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
 
   P_VP(P0, VS_Projector_Mesh, PS)
   P_VP(P1, VS_Projector_Model, PS)
   P_VP(P2, VS_Projector_Animation, PS)
    P_VP(P3, VS_Projector_Terrain, PS_Terrain)
  
}