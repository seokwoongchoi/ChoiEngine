#include "000_Header.fx"
#include "000_Light.fx"

float4 PS(MeshOutput input) : SV_Target0
{
    float4 diffuse = DiffuseMap.Sample(LinearSampler, input.Uv);
    
    float3 normal = normalize(input.Normal);
    float NdotL = saturate(dot(normal, -GlobalLight.Direction));

    return diffuse * NdotL;
}

technique11 T0
{
    P_VP(P0, VS_Mesh, PS)
}