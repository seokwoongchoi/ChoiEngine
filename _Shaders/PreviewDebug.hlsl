
cbuffer CB_VP : register(b0)
{
    matrix WVP : packoffset(c0);
  
};

struct VertexColor
{
    float4 Position : POSITION0;
    float4 Color : Color0;
 
};
struct VertexOutput
{
    float4 Position : SV_Position0;
    float4 Color : Color0;
 
};

VertexOutput DebugVS(VertexColor input)
{
   
    VertexOutput output;
   output.Position = mul(input.Position, WVP);
    
   
    output.Color = input.Color;
   
    return output;
}

float4 DebugPS(VertexOutput In) : SV_Target
{
   // return float4(1, 0, 0, 1);
    return float4(In.Color.rgb, 1.0);

}

