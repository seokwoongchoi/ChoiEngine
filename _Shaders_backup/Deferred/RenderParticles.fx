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
#include "000_Matrix.fx"
Texture2D particleTexture;
StructuredBuffer<float4> g_particles;

Texture2D EffectTransforms;

//--------------------------------------------------------------------------------------
// Global constants
//--------------------------------------------------------------------------------------

cbuffer cbDraw
{
    float4x4 ParticleViewProjection;
    float g_fPointSize;
    uint g_readOffset;
};


const static float4 g_positions[4] =
{
    float4(0.5, -0.5, 0, 0),
        float4(0.5, 0.5, 0, 0),
        float4(-0.5, -0.5, 0, 0),
        float4(-0.5, 0.5, 0, 0),
};

const static float4 g_texcoords[4] =
{
    float4(0, 0, 0, 0),
    float4(0, 1, 0, 0),
    float4(1, 0, 0, 0),
    float4(1, 1, 0, 0),
};

//SamplerState LinearSampler
//{
//    Filter = MIN_MAG_MIP_LINEAR;
//    AddressU = WRAP;
//    AddressV = WRAP;
//};



//--------------------------------------------------------------------------------------
// Vertex shader and pixel shader input/output structures
//--------------------------------------------------------------------------------------

struct InstDesc
{
    float4x4 worlds;
    float4 factor;
   
};
struct VS_Input_Indices
{
    uint pId : SV_VertexID;
    
    
    InstDesc instDesc :  Inst0;
    uint InstID : SV_InstanceID;
};

struct DisplayVS_OUTPUT
{
    float4 Position : SV_POSITION; // vertex position 
   
    float2 uv : Uv;
    float PointSize : PSIZE; // point size;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
DisplayVS_OUTPUT DisplayVS_StructBuffer(VS_Input_Indices In)
{
    DisplayVS_OUTPUT Output = (DisplayVS_OUTPUT) 0;
    float4 m1, m2, m3, m4;
    m1 = EffectTransforms.Load(int3(0 , In.instDesc.factor.a, 0));
    m2 = EffectTransforms.Load(int3(1, In.instDesc.factor.a, 0));
    m3 = EffectTransforms.Load(int3(2, In.instDesc.factor.a, 0));
    m4 = EffectTransforms.Load(int3(3, In.instDesc.factor.a, 0));
  
   
    //matrix tempWorld = mul(temp, In.instDesc.worlds);
    //
    //matrix effectWorld = mul(tempWorld, world);
    matrix world = mul(matrix(m1, m2, m3, m4), In.instDesc.worlds);
    float3 position = 0;
    float4 q = 0;
    float3 scale = 0;
    decompose(world, position, q, scale);
    matrix temp =  compose(position, q, float3(1, 1, 1));
    matrix wvp = mul(temp, ParticleViewProjection);
     
    float4 pos = float4(g_particles[g_readOffset + In.pId].xyz, 1.0f);
    Output.Position = mul(pos, wvp);
  
    Output.PointSize = g_fPointSize;

   
    Output.uv = 0;
         
    return Output;
}

DisplayVS_OUTPUT DisplayVS_Preview(VS_Input_Indices In)
{
    DisplayVS_OUTPUT Output = (DisplayVS_OUTPUT) 0;
    
   
    float4 pos = float4(g_particles[g_readOffset + In.pId].xyz, 1.0f);
    Output.Position = WorldPosition(pos);
     // Transform the position from object space to homogeneous projection space
    Output.Position = mul(Output.Position, ParticleViewProjection);
   // Output.Position = pos;
    Output.PointSize = g_fPointSize;

    // Compute simple directional lighting equation
  
    Output.uv = 0;
         
    return Output;
}



//--------------------------------------------------------------------------------------
//
// Geometry shader for creating point sprites
//
//--------------------------------------------------------------------------------------

[maxvertexcount(4)]
void DisplayGS(point DisplayVS_OUTPUT input[1], inout TriangleStream<DisplayVS_OUTPUT> SpriteStream)
{
    DisplayVS_OUTPUT output;
  
	[unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        output.Position = input[0].Position + float4(g_positions[i].xy * input[0].PointSize,0, 0);
           
        //output.Position.y *= 1280/720; // Correct for the screen aspect ratio, since the sprite is calculated in eye space
           
        // pass along the texcoords
        output.uv = g_texcoords[i];
    
        output.PointSize = input[0].PointSize;

        SpriteStream.Append(output);
    }
    SpriteStream.RestartStrip();
}

float4 DisplayPSTex(DisplayVS_OUTPUT In):SV_Target0
{
   
    float4 tex = particleTexture.Sample(LinearSampler, In.uv);
	
   [flatten]
    if (tex.x < 0.05) 
       discard;
	
   //float4 color = tex.xxxx;
 ////* float4(1.0, 0.1, 0.3, 1.0);
   //float4 color = tex;
    return tex ;
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
    SrcBlend[0] = SRC_COLOR;
    BlendOp[0] = Add;
  
    DestBlendAlpha[0] = ONE;
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;

   

    
};

BlendState PreviewParticleBlendState
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
        SetVertexShader(CompileShader(vs_5_0, DisplayVS_StructBuffer()));
        SetGeometryShader(CompileShader(gs_5_0, DisplayGS()));
        SetPixelShader(CompileShader(ps_5_0, DisplayPSTex()));
    }

    pass P1
    {
        SetRasterizerState(particleRS);
        SetBlendState(PreviewParticleBlendState, float4(0, 0, 0, 0), 0xFF);
        SetDepthStencilState(particleDSS, 0);
        SetVertexShader(CompileShader(vs_5_0, DisplayVS_Preview()));
        SetGeometryShader(CompileShader(gs_5_0, DisplayGS()));
        SetPixelShader(CompileShader(ps_5_0, DisplayPSTex()));
    }
   
}


    