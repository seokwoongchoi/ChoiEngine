#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"



struct VertexInput
{
    float4 Position : Position0;
    float2 Scale : Scale0;
    uint TextureNum : Num0;
    float Random : Random0;
    uint VertexID : SV_VertexID;
};
struct VertexOutput
{
    float4 Position : Position1;
    float2 Scale : Scale0;
    uint TextureNum : Num0;
    float Random : Random0;
    uint ID : Id0;
};

VertexOutput VS(VertexInput input)
{
    VertexOutput output;

  
    output.Position = input.Position;
    output.Scale = input.Scale;
    output.TextureNum = input.TextureNum;
    output.Random = input.Random;
    output.ID = input.VertexID;
    return output;
}

struct BilboardGeometryOutput
{
    float4 Position : SV_Position0; //픽셀 쉐이더로 들어가는 input중에 반드시 svPosition이 있어야한다.
    float2 Uv : Uv0;
    uint TextureNum : Num0;
    uint ID : Id0;
};

[maxvertexcount(4)]
void GS(point VertexOutput input[1], inout TriangleStream<BilboardGeometryOutput> stream)//정점 하나를 받아서 gs에서 정점 4개로만들어서 면을 만든다.
{
    float3 up = float3(0, 1, 0);
   
    float3 forward = ViewPosition() - input[0].Position.xyz;
    forward = normalize(forward);
    float3 right = cross(forward, up);
    float2 size = input[0].Scale * 0.5f;

    float3 breeze = 0;
    breeze.x += cos(Time - input[0].Random);

    float3 position[4];
    position[0] = float3(input[0].Position.xyz - size.x * right - size.y * up);
    position[1] = float3(input[0].Position.xyz - size.x * right + size.y * up) + breeze;
    position[2] = float3(input[0].Position.xyz + size.x * right - size.y * up);
    position[3] = float3(input[0].Position.xyz + size.x * right + size.y * up) + breeze;

    float2 uvs[4] =
    {
        float2(0, 1), float2(0, 0), float2(1, 1), float2(1, 0)
    };
    BilboardGeometryOutput output;
    
    [roll(4)]
    for (int i = 0; i < 4;i++)
    {
        output.Position = WorldPosition(float4(position[i], 1));
        output.Position = ViewProjection(output.Position);
        output.Uv = uvs[i];
        output.ID = input[0].ID;
        output.TextureNum = input[0].TextureNum;
        stream.Append(output);
        
    }

}

Texture2DArray Maps;

float4 PS_Billboard(BilboardGeometryOutput input) : SV_Target0
{
    float3 uvw;
   
    float4 diffuse = float4(0, 0, 0, 0); 
    if (input.ID > 0)
    {
        uvw = float3(input.Uv, input.ID * input.TextureNum / input.ID);
   
        //diffuse = Maps.Sample(LinearSampler, uvw);
        diffuse = Maps.Load(uint4(uvw, 0));
        
    }
   
    [flatten]
    if (diffuse.a < 0.3f)
      discard;
  

    clip(diffuse.a - 0.9f);

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