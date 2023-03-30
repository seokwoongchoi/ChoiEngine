//--------------------------------------------------------------------------------
// PhongShading.hlsl
//
// This set of shaders implements the most basic phong shading.
//
// Copyright (C) 2010 Jason Zink.  All rights reserved.
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// Resources
//--------------------------------------------------------------------------------


cbuffer cbDraw
{
    float4x4 g_mWorldViewProjection;
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
    float4(1, 0, 0, 0),
        float4(1, 1, 0, 0),
        float4(0, 0, 0, 0),
        float4(0, 1, 0, 0),
};

SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};



struct Particle
{
    float3 position;
	float3 direction;
	float  time;
};

StructuredBuffer<Particle> SimulationState;
Texture2D particleTexture;


// Inter-stage structures
//--------------------------------------------------------------------------------
//struct VS_INPUT
//{
//	uint vertexid			: SV_VertexID;
//};
//--------------------------------------------------------------------------------
struct VS_Input_Indices
{
    uint pId : SV_VertexID;
};

struct DisplayVS_OUTPUT
{
    float4 Position : SV_POSITION; // vertex position 
   
    float2 uv : Uv;
    float PointSize : PSIZE; // point size;
};


//--------------------------------------------------------------------------------
DisplayVS_OUTPUT VSMAIN(VS_Input_Indices In)
{
    DisplayVS_OUTPUT output;
	
   
    float4 pos = float4(SimulationState[In.pId].position.xyz, 1.0f);
    
     // Transform the position from object space to homogeneous projection space
    output.Position = mul(pos, g_mWorldViewProjection);
  
    output.PointSize = g_fPointSize;

    // Compute simple directional lighting equation
  
    output.uv = 0;
	return output;
}
//--------------------------------------------------------------------------------
[maxvertexcount(4)]
void GSMAIN(point DisplayVS_OUTPUT input[1], inout TriangleStream<DisplayVS_OUTPUT> SpriteStream)
{
    DisplayVS_OUTPUT output;

       
	[unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        output.Position = input[0].Position + float4(g_positions[i].xy * input[0].PointSize, 0, 0);
             
        //output.Position.y *= g_fAspectRatio; // Correct for the screen aspect ratio, since the sprite is calculated in eye space
             
        // pass along the texcoords
        output.uv = g_texcoords[i];
      
        output.PointSize = input[0].PointSize;
		
        SpriteStream.Append(output);
    }
    SpriteStream.RestartStrip();
}
//--------------------------------------------------------------------------------
float4 PSMAIN(DisplayVS_OUTPUT input) : SV_Target0
{
    float4 tex = particleTexture.Sample(LinearSampler, input.uv);
	
   [flatten]
    if (tex.x < 0.05) 
        discard;
	
  //  float4 color = tex.xxxx;
 ////* float4(1.0, 0.1, 0.3, 1.0);
   //float4 color = tex;
    return tex;
}
//--------------------------------------------------------------------------------
RasterizerState particleRS
{
     FillMode = WireFrame;
    //CullMode = None;
    //FrontCounterClockwise = false;
    //DepthClipEnable = true;
    //ScissorEnable = false;
    //MultisampleEnable = false;
    //AntialiasedLineEnable = false;
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

DepthStencilState particleDSS
{
 
    DepthEnable = true;
    DepthWriteMask = Zero;
    //DepthFunc = Less_Equal;

    //StencilEnable = true;
    //StencilReadMask = 0xff;
    //StencilWriteMask = 0xff;
  
   
};
technique11 T0
{
    
    pass p0
    {
        SetRasterizerState(particleRS);
        SetBlendState(ParticleBlendState, float4(0, 0, 0, 0), 0xFF);
        SetDepthStencilState(particleDSS, 0);
        SetVertexShader(CompileShader(vs_5_0, VSMAIN()));
        SetGeometryShader(CompileShader(gs_5_0, GSMAIN()));
        SetPixelShader(CompileShader(ps_5_0, PSMAIN()));

       
    }

 
 
}
