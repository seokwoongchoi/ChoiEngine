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

[maxvertexcount(8)]

void GS(triangle VertexOutput input[3], inout TriangleStream<GeometryOutput> stream)
{
    VertexOutput v[6];
    VertexOutput p[3];
    p[0].Position = (input[0].Position + input[1].Position) * 0.5f;
    p[1].Position = (input[1].Position + input[2].Position) * 0.5f;
    p[2].Position = (input[2].Position + input[0].Position) * 0.5f;

    v[0] = input[0];
    v[1] = p[0];
    v[2] = p[2];
    v[3] = p[1];
    v[4] = input[2];
    v[5] = input[1];

    float4 position = 0;
    GeometryOutput output[6];
    //[unroll(4)]

    for (int i = 0; i < 6;i++)
    {
        position = WorldPosition(v[i].Position);
        position = ViewProjection(position);

        output[i].Position = position;
    }
    
    for (int j = 0; j < 6; j++)
    {
        stream.Append(output[j]);
    }

    stream.RestartStrip();//3개넣고 restart하고 3개넣고 restart하면 트라이앵글리스트랑 똑같아진다
    
    stream.Append(output[1]);
    stream.Append(output[5]);
    stream.Append(output[3]);
};

float4 PS(VertexOutput input) : SV_Target0
{
    
    float4 diffuse = float4(1, 0, 0, 1);
    return diffuse;

}



RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
    //P_VGP(P0, VS, PS)
    P_RS_VGP(P0, RS, VS, GS, PS)


}