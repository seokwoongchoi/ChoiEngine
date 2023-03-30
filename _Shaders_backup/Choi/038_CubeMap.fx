#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"



float4 PS(MeshOutput input) : SV_Target0
{
    float4 color = VS_AllLight(input);

    return VS_Shadow(input, color);
    
    
}

float2 ReflectAmount;
TextureCube CubeMap;
float4 PS_Cube(MeshOutput input) : SV_Target0
{

    
    float4 color = VS_Shadow(input, VS_AllLight(input));

    float3 eye = normalize(input.wPosition - ViewPosition());
    float3 r = reflect(eye, normalize(input.Normal));

    color *= (ReflectAmount.x + CubeMap.Sample(LinearSampler, r) * ReflectAmount.y);
    return color;
    
}
RasterizerState RS
{
   // FillMode = Wireframe;
    CullMode = Front;
};

technique11 T0
{
 
   P_RS_VP(P0,RS, VS_Depth_Mesh, PS_Depth)
   P_RS_VP(P1,RS, VS_Depth_Model, PS_Depth)
   P_RS_VP(P2,RS, VS_Depth_Animation, PS_Depth)

   P_VP(P3, VS_Mesh, PS)
   P_VP(P4, VS_Model, PS)
   P_VP(P5, VS_ModelAnimation, PS)

   P_VP(P6, VS_Mesh, PS_Cube)
   P_VP(P7, VS_Model, PS_Cube)
   P_VP(P8, VS_ModelAnimation, PS_Cube)
}