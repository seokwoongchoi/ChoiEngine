#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"


float4 PS(MeshOutput input) : SV_Target0
{
    Texture(Material.Diffuse, DiffuseMap, input.Uv);
    Texture(Material.Specular, SpecularMap, input.Uv);
    MaterialDesc output = MakeMaterial();

    MaterialDesc result = MakeMaterial();

    ComputeLight(output, input.Normal, input.wPosition);
    AddMeterial(result, output);
    ComputePointLights(output, input.Normal, input.wPosition);
    AddMeterial(result, output);
    

    return float4(MaterialToColor(result), 1);
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