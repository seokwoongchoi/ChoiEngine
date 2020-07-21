// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

#include "000_Header.fx"
Texture2D effectTexture;
cbuffer CB_Effect
{
    matrix effectWorld;
    matrix effectVP;
    
};

struct EffectVSInput
{
    float4 Position : Position0;
    float2 Scale : Scale0;
    uint TextureNum : Num0;
    float Random : Random0;
    uint VertexID : SV_VertexID;
};
struct EffectVSOutput
{
    float4 Position : Position1;
    float2 Scale : Scale0;
    uint TextureNum : Num0;
    float Random : Random0;
    uint ID : Id0;
};

EffectVSOutput EffectVS(EffectVSInput input)
{
    EffectVSOutput output;

  
    output.Position = input.Position;
    output.Scale = input.Scale;
    output.TextureNum = input.TextureNum;
    output.Random = input.Random;
    output.ID = input.VertexID;
    return output;
}

struct EffectGeometryOutput
{
    float4 Position : SV_Position0; //픽셀 쉐이더로 들어가는 input중에 반드시 svPosition이 있어야한다.
    float2 Uv : Uv0;
    uint TextureNum : Num0;
    uint ID : Id0;
};


[maxvertexcount(4)]
void EffectDisplayGS(point EffectVSOutput input[1], inout TriangleStream<EffectGeometryOutput> SpriteStream)
{
 
    float3 up = float3(0, lerp(1, 0, input[0].ID), 0);
    float3 right = float3(1, 0, 0);
    float3 fowawrd = float3(0, 0, input[0].ID);
  
 
 
    float2 size =5 * 0.5f;
  
  
    float3 position[4];
    position[0] = float3(input[0].Position.xyz - size.x * right - size.y * up - size.y * fowawrd);
    position[1] = float3(input[0].Position.xyz - size.x * right + size.y * up + size.y * fowawrd);
    position[2] = float3(input[0].Position.xyz + size.x * right - size.y * up - size.y * fowawrd);
    position[3] = float3(input[0].Position.xyz + size.x * right + size.y * up + size.y * fowawrd);
   
    

 
    float2 uvs[4] =
    {
        float2(0, 1), float2(0, 0), float2(1, 1), float2(1, 0)
   
      
    };
 
    EffectGeometryOutput output;
  
    [unroll(4)]
    for (int i = 0; i < 4; i++)
    {
       
        output.Position = mul(float4(position[i], 1.0f), effectWorld);
        output.Position = mul(output.Position, effectVP);
        output.Uv = uvs[i];
    
        output.ID = input[0].ID;
        output.TextureNum = input[0].TextureNum;
	
    
        SpriteStream.Append(output);
      
    }
    SpriteStream.RestartStrip();

}

[maxvertexcount(4)]
void EffectGS(point EffectVSOutput input[1], inout TriangleStream<EffectGeometryOutput> SpriteStream)
{
 
    float3 up = float3(0, lerp(1, 0, input[0].ID), 0);
    float3 right = float3(1, 0, 0);
    float3 fowawrd = float3(0, 0, input[0].ID);
  
 
 
    float2 size = 5 * 0.5f;
  
  
    float3 position[4];
    position[0] = float3(input[0].Position.xyz - size.x * right - size.y * up - size.y * fowawrd);
    position[1] = float3(input[0].Position.xyz - size.x * right + size.y * up + size.y * fowawrd);
    position[2] = float3(input[0].Position.xyz + size.x * right - size.y * up - size.y * fowawrd);
    position[3] = float3(input[0].Position.xyz + size.x * right + size.y * up + size.y * fowawrd);
   
    

 
    float2 uvs[4] =
    {
        float2(0, 1), float2(0, 0), float2(1, 1), float2(1, 0)
   
      
    };
 
    EffectGeometryOutput output;
  
    [unroll(4)]
    for (int i = 0; i < 4; i++)
    {
       
        output.Position = mul(float4(position[i], 1.0f), effectWorld);
        output.Position = ViewProjection(output.Position);
        output.Uv = uvs[i];
    
        output.ID = input[0].ID;
        output.TextureNum = input[0].TextureNum;
	
    
        SpriteStream.Append(output);
      
    }
    SpriteStream.RestartStrip();

}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
//DisplayPS_OUTPUT DisplayPS(DisplayVS_OUTPUT In)
//{
//    DisplayPS_OUTPUT Output;
	
//    Output.RGBColor = In.Diffuse;
		
//    return Output;
//}

float4 EffectDisplayPSTex(EffectGeometryOutput In) : SV_Target0
{
   
    float4 tex = effectTexture.Sample(LinearSampler, In.Uv);
	
   [flatten]
    if (tex.x < 0.5f) 
       discard;
	
   //float4 color = tex.xxxx;
 ////* float4(1.0, 0.1, 0.3, 1.0);
   //float4 color = tex;
    return tex;

}
RasterizerState particleRS
{
   
    CullMode = None;
    FrontCounterClockwise = false;
    DepthClipEnable = true;
    ScissorEnable = false;
    MultisampleEnable = false;
    AntialiasedLineEnable = false;
};
BlendState ParticleBlendState
{

    BlendEnable[0] = true;
    DestBlend[0] = ONE;
 // SRC_COLOR;D3D11_BLEND_ZERO
    SrcBlend[0] = ONE;
    BlendOp[0] = Add;
  
    DestBlendAlpha[0] = ONE;
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
  
    RenderTargetWriteMask[0] = 15;

   

    
};
BlendState AlphaToCoverage
{
    AlphaToCoverageEnable = true;
   // IndependentBlendEnable = false;

 //   BlendEnable[0] = true;
 //   DestBlend[0] = ONE;
 //// SRC_COLOR;D3D11_BLEND_ZERO
 //   SrcBlend[0] = ONE;
 //   BlendOp[0] = Add;
  
 //   DestBlendAlpha[0] = ONE;
 //   SrcBlendAlpha[0] = ONE;
 //   BlendOpAlpha[0] = Add;
  
    RenderTargetWriteMask[0] = 15;

   

    
};


DepthStencilState particleDSS
{
 
    DepthEnable = true;
    DepthWriteMask = All;
    DepthFunc =Less_Equal;

   StencilEnable = true;
    StencilReadMask = 0xff;
   StencilWriteMask = 0xff;
  
   
};

technique11 T0
{
    pass P0
    {
        SetRasterizerState(particleRS);
        SetBlendState(ParticleBlendState, float4(0, 0, 0, 0), 0xFF);
        SetDepthStencilState(particleDSS, 0);
        SetVertexShader(CompileShader(vs_5_0, EffectVS()));
        SetGeometryShader(CompileShader(gs_5_0, EffectDisplayGS()));
        SetPixelShader(CompileShader(ps_5_0, EffectDisplayPSTex()));
    }
    pass P1
    {
        SetRasterizerState(particleRS);
        SetBlendState(ParticleBlendState, float4(0, 0, 0, 0), 0xFF);
        SetDepthStencilState(particleDSS, 0);
        SetVertexShader(CompileShader(vs_5_0, EffectVS()));
        SetGeometryShader(CompileShader(gs_5_0, EffectGS()));
        SetPixelShader(CompileShader(ps_5_0, EffectDisplayPSTex()));
    }
   
}


    