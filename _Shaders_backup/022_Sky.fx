#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"

cbuffer CB_Sky
{
    float4 Center;
    float4 Apex;
    
    float Height;
};

float4 PS(MeshOutput input) : SV_Target0
{
    float4 diffuse = DiffuseMap.Sample(LinearSampler, input.Uv) ;
   
     
    return lerp(Center, Apex, input.oPosition.y * Height);

}
float4 PS_CubeMap(MeshOutput input) : SV_Target0
{
  
     
   return SkyCubeMap.Sample(LinearSampler, input.oPosition);
  //  return float4(1, 1, 1, 1);

}

RasterizerState RS
{
    FrontCounterClockWise = true; //반시계 ON

};
RasterizerState RS2
{
    FillMode = Wireframe;
    FrontCounterClockWise = true; //반시계 ON

};
DepthStencilState DS
{
    DepthEnable = false;
   
};

technique11 T0
{
    P_RS_DSS_VP(P0, RS,DS,VS_Mesh, PS)
    P_RS_DSS_VP(P1, RS2, DS, VS_Mesh, PS)
    P_RS_DSS_VP(P2, RS, DS, VS_Mesh, PS_CubeMap)
  
}