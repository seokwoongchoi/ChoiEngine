#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"

matrix CubeViews[6];
matrix CubeProjection;

struct GeometryOutput
{
    float4 Position : SV_Position0;
    float4 wvpPosition : Position1;
    float3 oPosition : Position2;
    float3 wPosition : Position3;

    float4 sPosition : Position4;
    float4 sPosition2 : Position5;
    float4 sPosition3 : Position6;
 
    float2 Uv : Uv1;
    float3 Normal : Normal0;
    int ID : Id0;
    float3 Tangent : Tangent0;
    uint TargetIndex : SV_RenderTargetArrayIndex;
};
[maxvertexcont(18)]
void GS_PreRender(triangle MeshOutput_GS input[3], inout TriangleStream<GeometryOutput> stream)
{
  int vertex = 0;
  GeometryOutput output;

  for(uint i = 0;i<6;i++)
  {
        output.TargetIndex = i;

        for (vertex = 0; vertex < 3;vertex++)
        {
            output.Position = mul(input[vertex].Position, CubeViews[i]);
            output.Position = mul(output.Position, CubeProjection);


            output.wPosition = input[vertex].wPosition;
            output.oPosition = input[vertex].oPosition;

            output.sPosition = input[vertex].sPosition;
            output.sPosition2 = input[vertex].sPosition2;
            output.sPosition3 = input[vertex].sPosition3;

            output.Normal = input[vertex].Normal;
            output.Tangent = input[vertex].Tangent;
            stream.Append(output);
        }
        stream.RestartStrip();
    }

}

float4 PS(GeometryOutput input) : SV_Target0
{
    MeshOutput output = 0;
    output.Position = input.Position;
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
 
   //cube preRender
   P_VGP(P0,  VS_Depth_Mesh,GS_PreRender, PS_PreRender)
   P_VGP(P1, VS_Depth_Model, GS_PreRender, PS_PreRender)
   P_VGP(P2, VS_Depth_Animation, GS_PreRender, PS_PreRender)

   P_RS_VP(P3,RS, VS_Depth_Mesh, PS_Depth)
   P_RS_VP(P4,RS, VS_Depth_Model, PS_Depth)
   P_RS_VP(P5,RS, VS_Depth_Animation, PS_Depth)

   P_VP(P6, VS_Mesh, PS)
   P_VP(P7, VS_Model, PS)
   P_VP(P8, VS_ModelAnimation, PS)

   P_VP(P9, VS_Mesh, PS_Cube)
   P_VP(P10, VS_Model, PS_Cube)
   P_VP(P11, VS_ModelAnimation, PS_Cube)
}