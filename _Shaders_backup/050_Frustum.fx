#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"

float4 Planes[6];

struct VertexOutput
{
    float4 Position : SV_Position0;
    float4 wvpPosition : Position1;
    float3 oPosition : Position2;
    float3 wPosition : Position3;
    float4 sPosition : Position4;

    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
    float4 Alpha : Alpha0;

    float4 Cull : SV_ClipDistance0;
    float4 Cull2 : SV_ClipDistance1;
};

VertexOutput VS(VertexModel input)
{
    VertexOutput output;

    SetModelWorld(World, input);

    output.oPosition = input.Position.xyz;
    output.Position = WorldPosition(input.Position);
    output.wPosition = output.Position.xyz;

    output.Position = ViewProjection(output.Position);
    output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

    output.sPosition = WorldPosition(input.Position);
    output.sPosition = mul(output.sPosition, ShadowView);
    output.sPosition = mul(output.sPosition, ShadowProjection);
    output.Alpha = 0;
    output.Cull.x = dot(float4(output.wPosition, 1.0f), Planes[0]);
    output.Cull.y = dot(float4(output.wPosition, 1.0f), Planes[1]);
    output.Cull.z = dot(float4(output.wPosition, 1.0f), Planes[2]);
    output.Cull.w = 0;

    output.Cull2.x = dot(float4(output.wPosition, 1.0f), Planes[3]);
    output.Cull2.y = dot(float4(output.wPosition, 1.0f), Planes[4]);
    output.Cull2.z = dot(float4(output.wPosition, 1.0f), Planes[5]);
    output.Cull2.w = 0;

    return output;
}

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
  
   
    return diffuse*NdotL;
  //  return (diffuse * NdotL);
  

}
float4 PS_Cull(VertexOutput input) : SV_Target0
{
    float4 diffuse = DiffuseMap.Sample(LinearSampler, input.Uv);
    float3 normal = normalize(input.Normal);
    float NdotL = saturate(dot(normal, -GlobalLight.Direction));

    //[flatten]
    //if (tankNum == input.ID - 1)
    //{
    //    return (diffuse * NdotL) + float4(1, 0, 0, 1);
    //}
  
   
    return diffuse * NdotL;
  //  return (diffuse * NdotL);
  

}

RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
 //   P_VP(P0, VS_Mesh, PS)
    P_VP(P0, VS_Mesh, PS)
    P_VP(P1, VS, PS_Cull)
   // P_VP(P2, VS_StaticModel, PS)
   // P_RS_VP(P3, RS, VS_Mesh, PS)
    //P_RS_VP(P4, RS, VS_Model, PS)
     //P_RS_VP(P5, RS, VS_Animation, PS)
}