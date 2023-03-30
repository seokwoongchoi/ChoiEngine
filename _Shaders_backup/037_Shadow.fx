#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"



float4 PS(MeshOutput input) : SV_Target0
{
    float4 color = VS_AllLight(input);
    
    input.sPosition.xyz /= input.sPosition.w;
   
   
    [flatten]
    if (input.sPosition.x < -1.0f || input.sPosition.x > 1.0f ||
        input.sPosition.y < -1.0f || input.sPosition.y > 1.0f ||
        input.sPosition.z < 0.0f || input.sPosition.z > 1.0f)
     
        return color;
 
   
    input.sPosition.x = input.sPosition.x * 0.5f + 0.5f;
    input.sPosition.y = -input.sPosition.y * 0.5f + 0.5f;
    input.sPosition.z -= ShadowBias;

  

    return ShadowMap.Sample(LinearSampler, float3(input.sPosition.xy, 0));
   // 
   
    //float depth = 0;
    //float factor = 0;
    //float depth = 0;
    
   
   
   //   float2 size = 1.0f / ShadowMapSize;
    //    float2 offsets[] =
    //    {
    //        float2(+size.x, -size.y), float2(0.0f, -size.y), float2(-size.x, -size.y),
    //        float2(+size.x, 0.0f), float2(0.0f, 0.0f), float2(-size.x, 0.0f),
    //        float2(+size.x, +size.y), float2(0.0f, +size.y), float2(-size.x, +size.y),
    //    };

    //    float sum = 0.0f;
    //    float3 uv = 0.0f;
    //depth = input2.sPosition.z;
       

    //    //[unroll(9)]
    //    for (int i = 0; i < 9; i++)
    //    {
    //    uv = float3(input2.sPosition.xy + offsets[i], 0);
    
    //    sum += ShadowMaps.SampleCmpLevelZero(ShadowSampler, uv, depth);
      
    //}

    //    factor = sum / 9.0f;
    //float4 shadowColor = lerp(float4(0, 0, 0,0), float4(1, 1, 1,1), sum);
    //factor = saturate(factor + depth);

  
  
    //return float4(color.rgb * factor, 1);
    
}
RasterizerState RS
{
   // FillMode = Wireframe;
    CullMode = Front;
};

technique11 T0
{
 
  P_RS_VP(P0, RS, VS_Depth_Mesh,  PS_Depth)
  // P_RS_VP(P0,RS, VS_Depth_Mesh, PS_Depth)
  // P_RS_VP(P1,RS, VS_Depth_Model, PS_Depth)
  // P_RS_VP(P2,RS, VS_Depth_Animation, PS_Depth)
   
   P_VGP(P1, VS_Mesh,GS_Depth,PS)
  // P_VP(P4, VS_Model, PS)
  // P_VP(P5, VS_Animation, PS)
}