//--------------------------------------------------------------------------------------
// File: SoftParticles10.1.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// structs
struct VSSceneIn
{
    float3 Pos            : POSITION;
    float3 Norm           : NORMAL;  
    float2 Tex            : TEXCOORD;
    float3 Tan            : TANGENT;
};

struct PSSceneIn
{
    float4 Pos : SV_Position;
    float3 Norm			  : NORMAL;
    float2 Tex			  : TEXCOORD0;
    float3 Tan			  : TEXCOORD1;
    float3 vPos			  : TEXCOORD2;
};

struct VSParticleIn
{
    float3 Pos			  : POSITION;
    float3 Vel			  : VELOCITY;
    float Life			  : LIFE;
    float Size			  : SIZE;
};

struct GSParticleIn
{
    float4 Pos : SV_Position;
    float Life            : LIFE;	//stage of animation we're in [0..1] 0 is first frame, 1 is last
    float Size            : SIZE;
};

struct PSParticleIn
{
    float4 Pos			  : SV_POSITION;
    float3 Tex			  : TEXCOORD0;
    float2 ScreenTex	  : TEXCOORD1;
    float2 Depth		  : TEXCOORD2;
    float  Size			  : TEXCOORD3;
    float3 worldPos		  : TEXCOORD4;
    float3 particleOrig	  : TEXCOORD5;
    float3 particleColor  : TEXCOORD6;
};

cbuffer cbPerObject : register(b0) //gs
{
    matrix WorldViewProj;
    matrix World;
    matrix InvView;
   
};

cbuffer cbUser : register(b1) //ps
{
    matrix InvProj;
   // float4 lightDirectional;
   // float  SizeZScale;
   // float  FadeDistance;
   // float2 pad;

};

static const float3 positions[4] =
{
    float3(-1, 1, 0),
        float3(1, 1, 0),
        float3(-1, -1, 0),
        float3(1, -1, 0),
};
static const float2 texcoords[4] =
{
    float2(0, 0),
        float2(1, 0),
        float2(0, 1),
        float2(1, 1),
};
    




Texture2D txColorGradient : register(t0); //gs
Texture3D txVolumeDiff : register(t1); //ps
Texture2D txDepth : register(t2); //ps


SamplerState samLinearClamp : register(s0);//GS
SamplerState samVolume : register(s1); //ps
SamplerState samPoint : register(s2); //ps




GSParticleIn VS(VSParticleIn input)
{
    GSParticleIn output;
    
    output.Pos = float4(input.Pos, 1.0f);
    output.Life = input.Life;
    output.Size = input.Size;
    
    return output;
}

[maxvertexcount(4)]
void GS(point GSParticleIn input[1], inout TriangleStream<PSParticleIn> SpriteStream)
{
    PSParticleIn output;
    
    float4 orig = mul( input[0].Pos, World );
    output.particleOrig = orig.xyz;
    
   
    if( input[0].Life > -1 )
    {
        float3 particleColor = txColorGradient.SampleLevel( samLinearClamp, float2(input[0].Life,0), 0 );
      
        output.particleColor = particleColor;
      
        [unroll(4)] 
        for(int i=0; i<4; i++)
        {
            float3 position = positions[i]*input[0].Size;
            position = mul( position, (float3x3)InvView ) + input[0].Pos.xyz;
            output.Pos = mul( float4(position,1.0), WorldViewProj );
          
            output.Tex = float3(texcoords[i],input[0].Life);
            output.ScreenTex = output.Pos.xy/output.Pos.w;
        
            output.Depth = output.Pos.zw;
                    
            output.Size = input[0].Size;
                                    
            float4 posWorld = mul( float4(position,1.0), World );
            output.worldPos = posWorld;							
            
            SpriteStream.Append(output);
        }
        SpriteStream.RestartStrip();
    }
}


float4 PS(PSParticleIn input):SV_Target
{   
    
    
    float2 screenTex = 0.5*( (input.ScreenTex) + float2(1,1));
    screenTex.y = 1 - screenTex.y;
    
    float4 particleSample = txVolumeDiff.Sample( samVolume, input.Tex );
  
    //float size = SizeZScale * input.Size;
    //float particleDepth = input.Depth.x - size * 2.0 * (particleSample.a);
    //particleDepth /= input.Depth.y;
      
    //float depthSample = txDepth.Sample(samPoint, screenTex);
   
    
    //float4 depthViewSample = mul(float4(input.ScreenTex, depthSample, 1), InvProj);
    //float4 depthViewParticle = mul(float4(input.ScreenTex, particleDepth, 1), InvProj);
    
    //float depthDiff = depthViewSample.z / depthViewSample.w - depthViewParticle.z / depthViewParticle.w;
    
    //[flatten]
    //if (depthDiff < 0)
    //    discard;
       
    float depthFade = 1;//    saturate(depthDiff / FadeDistance);
    
   
    
      
   // float4 Light = g_directional1 + ambient;
    particleSample.rgb *=  (input.particleColor );
    particleSample.a *= depthFade;
    return particleSample;
}


