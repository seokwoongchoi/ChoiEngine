#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"


float4 PS(MeshOutput input) : SV_Target0
{
    float4 diffuse = DiffuseMap.Sample(LinearSampler, input.Uv) ;
   
    
  
    
    float3 normal = normalize(input.Normal);
    float NdotL = saturate(dot(normal, -GlobalLight.Direction));

  //  float3 brushColor = GetBrushColor(input.wPosition); //rasterizer에서 보간이 일어나서 깔금하게나온다.
   // float3 gridColor = GetLineColor(input.wPosition);
    return (diffuse * NdotL) ;
  //  return float4(gridColor, 1);
   //return float4(LightDirection*0.5f+0.5f, 1); //체크용 0~1의 범위로 빛이들어오는지 체크

}

RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
    P_VP(P0, VS_Mesh, PS)
    P_VP(P1, VS_Model, PS)
    P_RS_VP(P2, RS, VS_Mesh, PS)
    P_RS_VP(P3, RS, VS_Model, PS)
   
}