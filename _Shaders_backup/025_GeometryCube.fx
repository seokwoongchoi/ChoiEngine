#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"



struct VertexOutput
{
    float4 Position : Position0;
    
  
};
VertexOutput VS(Vertex input)
{
    VertexOutput output;
   
    output.Position = input.Position;
    
   
    
   
    return output;
  
    
};
struct GeometryOutput
{
   float4 Position : SV_Position0;
};

[maxvertexcount(14)]

void GS(triangle VertexOutput input[3], inout TriangleStream<GeometryOutput> stream)
{
    VertexOutput v[8];
   
   //¾Õ
    v[0].Position = input[2].Position + float4(0, 0, -1, 0);//¿ìÇÏ
   
    v[1].Position = input[0].Position + float4(0, 0, -1, 0);//ÁÂÇÏ
    
    v[2].Position = input[2].Position + float4(0, 0, -1, 0) + float4(0, input[1].Position.y, 0, 0); //¿ì»ó
    v[3].Position = input[0].Position + float4(0, 0, -1, 0) + float4(0, input[1].Position.y, 0, 0); //ÁÂ»ó
   
      
    //µÚ
   
    v[4].Position = input[2].Position + float4(0, 0, 1, 0); //¿ìÇÏ
    v[5].Position = input[0].Position + float4(0, 0, 1, 0);//ÁÂÇÏ
   
    v[6].Position = v[5].Position + float4(0, input[1].Position.y, 0, 0); //ÁÂ»ó
    v[7].Position = v[4].Position + float4(0, input[1].Position.y, 0, 0); //¿ì»ó

    float4 position = 0;
    GeometryOutput output[8];
    //[unroll(4)]

    for (int i = 0; i < 8;i++)
    {
        position = WorldPosition(v[i].Position);
        position = ViewProjection(position);

        output[i].Position = position;
    }
    
   
    stream.Append(output[3]);
    stream.Append(output[2]);
    stream.Append(output[6]);
    stream.Append(output[7]);
    
    stream.Append(output[4]);
    stream.Append(output[2]);

   
    stream.Append(output[0]);

    stream.Append(output[3]);
    stream.Append(output[1]);
    stream.Append(output[6]);
   
    stream.Append(output[5]);
    stream.Append(output[4]);
    stream.Append(output[1]);
    stream.Append(output[0]);
    
    
  
    
};

float4 PS(VertexOutput input) : SV_Target0
{
    
    float4 diffuse = float4(1, 0, 0, 1);
    return diffuse;

}



RasterizerState RS
{
    FillMode = Wireframe;
    Cullmode = None;
  // FrontCounterClockwise = true;
};

technique11 T0
{
    //P_VGP(P0, VS, PS)
    P_RS_VGP(P0, RS, VS, GS, PS)


}