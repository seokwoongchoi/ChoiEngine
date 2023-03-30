#include <IBLHeader.fx>
#include "000_Header.fx"
#include "000_Model.fx"
#include "000_Terrain.fx"



MeshOutput VS_Mesh_PreRender(VertexMesh input)
{
    MeshOutput output = VS_Mesh(input);
   // output.Uv *= 4;
    return output;
}

MeshOutput VS_Model_PreRender(VertexModel input)
{
    MeshOutput output = VS_Model(input);
    return output;
}

MeshOutput VS_Animation_PreRender(VertexModel input)
{
    MeshOutput output = VS_Animation(input);
    return output;
}

MeshOutput VS_Terrain_PreRender(VertexInput_Lod input)
{
    MeshOutput output = VS(input);
    return output;
}




technique11 T0
{
 
   //cube preRender
  
   P_VGP(P3, VS_Terrain_PreRender, GS_PreRender, main)

   
}