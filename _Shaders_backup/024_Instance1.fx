#include "000_Header.fx"
#include "000_Light.fx"
cbuffer CB_randomColor
{
    float4 randomColor;
};
struct VertexIntput
{
    float4 Position :Position0;
    float2 Uv : Uv0;

    matrix Transform : Inst0;
    uint InstID : SV_InstanceID0;
};


struct VertexOutput
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
    uint ID : Id0;
//    matrix Transform : Inst0;
  
};
VertexOutput VS(VertexIntput input)
{
    VertexOutput output;
   // input.Position.x += input.InstID * 3;
    output.Position = WorldPosition(input.Position);
    output.Position = mul(input.Position,input.Transform);
   

    output.Position = ViewProjection(output.Position);
    output.Uv = input.Uv;
    output.ID = input.InstID;
    return output;
  
    
}
Texture2D Maps[100];

float4 PS(VertexOutput input) : SV_Target0
{
    

    float4 colors[8] =
    {
        float4(1, 0, 0, 1),
       float4(1, 1, 0, 1),
       float4(1, 0, 1, 1),
       float4(1, 0.5f, 1, 1),
         float4(1, 0, 0, 1),
       float4(1, 0.5f, 0, 1),
       float4(0.5f, 0, 0.3f, 1),
       float4(1, 1, 1, 1),
    };
    
    return colors[input.ID % 8];
   // return float4((float) input.ID / 100.0f, 0, 0, 1);
    //return randomColor;
}




technique11 T0
{
   P_VP(P0,VS,PS)
 
}