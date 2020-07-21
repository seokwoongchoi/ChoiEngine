#include "Header.hlsl"


Texture2D<float4> BoneTransforms : register(t0);
void SetModelWorld(int InstID)
{
    
    float4 m1 = BoneTransforms[int2(BoneIndex * 4 + 0, 0)];
    float4 m2 = BoneTransforms[int2(BoneIndex * 4 + 1, 0)];
    float4 m3 = BoneTransforms[int2(BoneIndex * 4 + 2, 0)];
    float4 m4 = BoneTransforms[int2(BoneIndex * 4 + 3, 0)];
    
    World = mul(matrix(m1, m2, m3, m4), InstTransforms[InstID]);
}
struct VertexPosInst
{
    float4 Position : Position0;
    uint InstID : SV_InstanceID;

};
float4 CascadedShadowGenVS(VertexPosInst input) : SV_Position
{
    
  
    SetModelWorld(input.InstID);
  
    float4 Position = WorldPosition(input.Position);
   
  
    return Position;

}

struct VertexStatic
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
      uint InstID : SV_InstanceID;
};


///////////////////////////////////////////////////////////////////////////////



VertexModelOutput VS(VertexStatic input)
{
    VertexModelOutput output;

   
    SetModelWorld(input.InstID);
  
    output.Position = WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;
    
    //output.Cull.x = dot(float4(worldPosition - ViewPosition(), 1.0f), -FrustumNormals[0]);
    //output.Cull.y = dot(float4(worldPosition - ViewPosition(), 1.0f), -FrustumNormals[1]);
    //output.Cull.z = dot(float4(worldPosition - ViewPosition(), 1.0f), -FrustumNormals[2]);
    //output.Cull.w = dot(float4(worldPosition - ViewPosition(), 1.0f), -FrustumNormals[3]);
    return output;
}

