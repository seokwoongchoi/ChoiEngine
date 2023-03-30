#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"


float4 PS(MeshOutput input) : SV_Target0
{
    Texture(Material.Diffuse, DiffuseMap, input.Uv);
    NormalMapping(input.Uv, input.Normal, input.Tangent);//정점에서 넘어온 인자들로 
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

 
    //return float4(input.Normal, 1);
   return float4(MaterialToColor(result), 1.0f);

}

RasterizerState RS
{
    FillMode = Wireframe;

};
DepthStencilState Depth
{
    DepthEnable = false;
};

technique11 T0
{
    P_VP(P0, VS_Mesh, PS)
    P_VP(P1, VS_Model, PS)
    P_RS_VP(P2, RS, VS_Mesh, PS)
    P_RS_VP(P3, RS, VS_Model, PS)
   P_DSS_VP(P4, Depth, VS_Mesh, PS)
}