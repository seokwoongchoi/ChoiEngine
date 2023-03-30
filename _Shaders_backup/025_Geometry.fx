#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"

int FixedY = 0;
struct VertexIntput
{
    float4 Position :Position0;
    
    float2 Scale : Uv0;
   
    uint VertexID : SV_VertexID;
  
};


struct VertexOutput
{
    float4 Position : Position0;
    float2 Scale : Scale0;
   
   
    uint ID : Id0;
   
//    matrix Transform : Inst0;
  
};
VertexOutput VS(VertexIntput input)
{
    VertexOutput output;
   
    output.Position = input.Position;
    output.Scale = input.Scale;
    output.ID = input.VertexID;

    
   
    return output;
  
    
};
struct GeometryOutput
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
       
    uint ID : Id0;
};

[maxvertexcount(4)]
void GS(point VertexOutput input[1],inout TriangleStream<GeometryOutput>stream)
{
    float3 up = float3(0, 1, 0);
    float3 forward = ViewPosition() - input[0].Position.xyz;
    forward = normalize(forward);
    [flatten]
    if(FixedY==1)
        forward.y = 0.0f;
    float3 right = cross(forward, up);
    //float3 right = float3(1, 0, 0);
    float2 size = input[0].Scale * 0.5f;

    float3 position[4];
    position[0] = float3(input[0].Position.xyz - size.x * right - size.y * up);
    position[1] = float3(input[0].Position.xyz - size.x * right + size.y * up);
    position[2] = float3(input[0].Position.xyz + size.x * right - size.y * up);
    position[3] = float3(input[0].Position.xyz + size.x * right + size.y * up);

    float2 uvs[4] =
    {
        float2(0, 1), float2(0, 0), float2(1, 1), float2(1, 0)
    };
    GeometryOutput output;

   [roll(4)]
    for (int i = 0; i < 4;i++)
    {
        output.Position = WorldPosition(float4(position[i], 1));
        output.Position = ViewProjection(output.Position);
        output.Uv = uvs[i];
        output.ID = input[0].ID;
        stream.Append(output);
    }
   

}
Texture2DArray Maps;

float4 PS_Billboard(GeometryOutput input) : SV_Target0
{
    
    float3 uvw = float3(input.Uv, input.ID%5);
    float4 diffuse = Maps.Sample(LinearSampler, uvw);

    [flatten]
    if(diffuse.a<0.3f)
        discard;

    return diffuse;

}
float4 PS(MeshOutput input) : SV_Target0
{
    float4 diffuse = DiffuseMap.Sample(LinearSampler, input.Uv);
   
    
  
    
    float3 normal = normalize(input.Normal);
    float NdotL = saturate(dot(normal, -GlobalLight.Direction));

  //  float3 brushColor = GetBrushColor(input.wPosition); //rasterizer에서 보간이 일어나서 깔금하게나온다.
   // float3 gridColor = GetLineColor(input.wPosition);
    return (diffuse * NdotL);
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
    P_VGP(P2, VS, GS, PS_Billboard)

}