#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"

int tankNum;

float4 PS(MeshOutput input) : SV_Target0
{
    float4 diffuse = DiffuseMap.Sample(LinearSampler, input.Uv) ;
    float3 normal = normalize(input.Normal);
    float NdotL = saturate(dot(normal, -GlobalLight.Direction));

    //[flatten]
    //if (tankNum == input.ID - 1)
    //{
    //    return (diffuse * NdotL) + float4(1, 0, 0, 1);
    //}

   

    return (diffuse * NdotL);
  

}

RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
    P_VP(P0, VS_Model, PS)
  //  P_VP(P1, VS_Mesh, PS)
   // P_RS_VP(P2, RS, VS_Mesh, PS)
    //P_RS_VP(P1, RS, VS_Model, PS)
   
}