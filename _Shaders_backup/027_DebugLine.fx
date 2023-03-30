
#include "000_Header.fx"
//#include "000_Light.fx"

struct ResultMatrix
{
    float4x4 Bone;
    float4 BoneColor;
    
};

//StructuredBuffer<ResultMatrix> Input;
Texture2D<float4> Input;


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
    output.Position = WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);

    output.Color =input.Color;

   
    
    return output;
}

VertexOutput BoneBoxVS(VertexColor input, uint ID : SV_VertexID)
{
    //WVP º¯È¯Àº VS¿¡¼­
    VertexOutput output;
    //mul : Çà·Ä°öÀÌÀÚ º¤ÅÍ°ö
    uint index = input.Color.a;
    uint vertex = lerp(24, 24 * (3 + ((index - 1) * 2)), saturate(index));
    uint factor = lerp(0, 1, ID > vertex+1);
    
    World._11_12_13_14 = Input.Load(int3(0 + (4 * factor), index, 0));
    World._21_22_23_24 = Input.Load(int3(1 + (4 * factor), index, 0));
    World._31_32_33_34 = Input.Load(int3(2 + (4 * factor), index, 0));
    World._41_42_43_44 = Input.Load(int3(3 + (4 * factor), index, 0));
    //output.Position = mul(input.Position, Input[input.Color.a].Bone);
    output.Position = WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);

    output.Color = input.Color;
   // output.Color = Input[input.Color.a].BoneColor;
   
    
    return output;
}



float4 PS(VertexOutput input) : SV_Target0
{
  

    return float4(input.Color.rgb, 1.0f);
   

}

RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
  P_VP(P0,VS,PS)
  P_VP(P1, BoneBoxVS, PS)
 
}

