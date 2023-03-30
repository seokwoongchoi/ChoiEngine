
#include "000_Header.fx"
#include "000_Light.fx"

struct VertexOutput
{
    float4 Position : SV_Position0;
    float4 Color : Color0;
   
};

VertexOutput VS(VertexColor input)
{
    //WVP º¯È¯Àº VS¿¡¼­
    VertexOutput output;
    //mul : Çà·Ä°öÀÌÀÚ º¤ÅÍ°ö
    output.Position =WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);

    output.Color =input.Color;

   
    
    return output;
}

float4 PS(VertexOutput input) : SV_Target0
{
  

   return input.Color;
   

}

RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
  P_VP(P0,VS,PS)
}