#include "000_Header.fx"
#include "000_Light.fx"

cbuffer CB_randomColor
{
    float4 randomColor;
};
struct InstDesc
{
    float4x4 worlds;
    float4 Color;
   
};
struct VertexIntput
{
    float4 Position :Position0;
    
    float2 Uv : Uv0;
    InstDesc instDesc : Inst0;
    uint InstID : SV_InstanceID0;
  
};


struct VertexOutput
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
    float4 Color : Color0;
    InstDesc instDesc : Inst0;
    uint ID : Id0;
   
//    matrix Transform : Inst0;
  
};
VertexOutput VS(VertexIntput input)
{
    VertexOutput output;
   // input.Position.x += input.InstID * 3;
    output.Position = WorldPosition(input.Position);
    output.Position = mul(input.Position,input.instDesc.worlds);
   

    output.Position = ViewProjection(output.Position);
    output.Uv = input.Uv;
    output.ID = input.InstID;
    output.instDesc = input.instDesc;
    output.Color = input.instDesc.Color;
   
    return output;
  
    
}
Texture2DArray Maps;

float4 PS(VertexOutput input) : SV_Target0
{
    
    //float4 colors[8] =
    //{
    //    float4(1, 0, 0, 1),
    //   float4(1, 1, 0, 1),
    //   float4(1, 0, 1, 1),
    //   float4(1, 0.5f, 1, 1),
    //     float4(1, 0, 0, 1),
    //   float4(1, 0.5f, 0, 1),
    //   float4(0.5f, 0, 0.3f, 1),
    //   float4(1, 1, 1, 1),
    //};
    
    //return colors[input.ID % 8];
    //return float4((float) input.ID * randomColor.r, (float) input.ID * randomColor.g, (float) input.ID * randomColor.b, 1);
    float3 uvw = float3(input.Uv, input.ID%6);
    return Maps.Sample(LinearSampler, uvw); //[]안에는 무조건 상수가 들어가야한다 아니면 에러남

}




technique11 T0
{
   P_VP(P0,VS,PS)
 
}