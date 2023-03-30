#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"

uint FixedY = 0;
struct InstDesc
{
    float4x4 worlds;
    uint Num;
   
};
struct VertexInput
{
    float4 Position : Position0;
    float2 Scale : Scale0;
    InstDesc instDesc : Inst0;
  //  uint TextureNum : Num0;
   uint VertexID : SV_InstanceID0;
  
};
struct VertexOutput
{
    float4 Position : Position1;
    float2 Scale : Scale0;
    InstDesc instDesc : Inst0;
   // uint TextureNum : Num0;
    uint ID : Id0;
};

VertexOutput VS(VertexInput input)
{
    VertexOutput output;

  
    output.Position = input.Position;
    output.instDesc = input.instDesc;
    output.Scale = input.Scale;
    output.ID = input.VertexID;
    
   // output.TextureNum = input.TextureNum;
    return output;
}

struct GeometryOutput
{
    float4 Position : SV_Position0; //픽셀 쉐이더로 들어가는 input중에 반드시 svPosition이 있어야한다.
    float2 Uv : Uv0;
    InstDesc instDesc : Inst0;
    //uint TextureNum : Num0;
    uint ID : Id0;
};

[maxvertexcount(4)]
void GS(point VertexOutput input[1],inout TriangleStream<GeometryOutput> stream)//정점 하나를 받아서 gs에서 정점 4개로만들어서 면을 만든다.
{
    float3 up = float3(0, 1, 0);
   
   // float3 forward = ViewPosition() - input[0].Position.xyz;

    //[flatten]
    //if(FixedY==1)
   // forward.y = 0.0f;

   // forward = normalize(forward);
    //float3 right = cross(forward, up);
    float3 right = float3(1, 0, 0);
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
    
   // [roll(4)]
    for (int i = 0; i < 4;i++)
    {
       
        output.instDesc.worlds = input[0].instDesc.worlds;
        output.Position = WorldPosition(float4(position[i], 1));
        output.Position = mul(float4(position[i], 1), output.instDesc.worlds);
        output.Position = ViewProjection(output.Position);
        output.Uv = uvs[i];
        output.ID = input[0].ID;
       
        output.instDesc.Num = input[0].instDesc.Num;
        stream.Append(output);
        
    }

}

Texture2DArray Maps;

float4 PS_Billboard(GeometryOutput input) : SV_Target0
{
    float3 uvw;
    float4 diffuse = float4(0, 0, 0, 0);
    if(input.ID>0)
    {
        uvw = float3(input.Uv, input.ID *input.instDesc.Num / input.ID);
   
        diffuse = Maps.Sample(LinearSampler, uvw);
    }
   
    
   
    [flatten]
    if (diffuse.a < 0.3f)
      discard;
  
  //  return float4(input.ID, input.ID, input.ID, input.ID);
   
    return diffuse;

}


RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, PS_Billboard()));

    }

    pass P1
    {
        SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, PS_Billboard()));

    }
   // P_VP(P0, VS_Mesh, PS)
   // P_VP(P1, VS_Model, PS)
   //P_VP(P2, VS_Mesh, PS_Billboard)
}