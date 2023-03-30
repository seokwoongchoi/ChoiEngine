#include "000_Header.fx"
//-----------------------------------------------------------------------------------------
// Occlusion
//-----------------------------------------------------------------------------------------

Texture2D DepthTex;
RWTexture2D<float> OcclusionRW;

cbuffer OcclusionConstants
{
    uint nWidth;
    uint nHeight;
}



[numthreads(720,1, 1)]
void Occlussion(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint2 Res = uint2(nWidth, nHeight);
    uint3 CurPixel = uint3(dispatchThreadId.x % Res.x, dispatchThreadId.x / Res.y, 0);

	// Skip out of bound pixels
	if(CurPixel.y < Res.y)
	{
		// Get the depth
		float curDepth = DepthTex.Load(CurPixel);

		// Flag anything closer than the sky as occlusion
       OcclusionRW[CurPixel.xy].x = curDepth>0.9;
     
    }
}

//-----------------------------------------------------------------------------------------
// Ray tracing
//-----------------------------------------------------------------------------------------

cbuffer RayTraceConstants
{
	float2 SunPos ;
	float InitDecay;
	float DistDecay;
	float3 RayColor;
	float MaxDeltaLen;
}

Texture2D OcclusionTex;


VS_OUTPUT RayTraceVS( uint VertexID : SV_VertexID )
{
    VS_OUTPUT Output;

	Output.Position = float4(arrBasePos[VertexID].xy, 1.0, 1.0);
	Output.Uv = arrUV[VertexID].xy;

	return Output;    
}

static const int NUM_STEPS =64;
static const float NUM_DELTA = 1.0 / 63.0f;


float4 RayTracePS( VS_OUTPUT In ) : SV_Target0
{
 
   
//// Find the direction and distance to the sun
   float2 dirToSun = (SunPos-In.Uv );
   float lengthToSun = length(dirToSun);
 
    dirToSun /= lengthToSun;

   float deltaLen = min(MaxDeltaLen, lengthToSun * NUM_DELTA);
   float2 rayDelta = dirToSun * deltaLen;



   float stepDecay = DistDecay * deltaLen;

   float decay = InitDecay;
   float rayIntensity = 0.0f;

   float2 rayOffset = float2(0.0, 0.0);

  // float rayIntensity = 0.0f;


   [unroll(64)]
   for (int i = 0; i < NUM_STEPS; i++)
   {
//	// Sample at the current location
       float2 sampPos = In.Uv + rayOffset;
       float fCurIntensity = OcclusionTex.Sample(LinearSampler, sampPos);
	
//	// Sum the intensity taking decay into account
       rayIntensity += fCurIntensity * decay;

//	// Advance to the next position
       rayOffset += rayDelta;

//	// Update the decay
       decay = saturate(decay - stepDecay);
   }
    


   //rayOffset += rayDelta;

   return float4(rayIntensity, 0.0, 0.0, 0.0);
 
 ///////////////////////////////////////////////////////////////////////////////////
 //   float rayIntensity = OcclusionTex.Sample(LinearSampler, In.UV);
    
 //   float2 dirToSun = (SunPos-In.UV );

 //  // float deltaLen = min(MaxDeltaLen, lengthToSun * NUM_DELTA);
 //   float2 rayDelta = dirToSun /7.0f;

 //   float2 rayOffset = dirToSun;
 
 //   float decay = InitDecay;
   
  
 
 
 //   [unroll(8)]
 //   for (int i = 0; i < 8; i++)
 //   {
	//// Sample at the current location
 //     float2 sampPos = In.UV + rayOffset;
 //     float fCurIntensity = OcclusionTex.Sample(LinearSampler, sampPos);

	//// Sum the intensity taking decay into account
 //     rayIntensity += fCurIntensity * decay;
 //       decay += DistDecay;
	//// Advance to the next position
 //     rayOffset += rayDelta;

	//// Update the decay
       
 //   }
 
   

 //   return float4(rayIntensity, rayIntensity, rayIntensity, rayIntensity);
}



//-----------------------------------------------------------------------------------------
// Combine results
//-----------------------------------------------------------------------------------------

Texture2D LightRaysTex;

static const float2 size = 1.0f / float2(1280, 720);

static const float2 offsets[] =
{
    float2(+2 * size.x, -2 * size.y), float2(+size.x, -2 * size.y), float2(0.0f, -2 * size.y), float2(-size.x, -2 * size.y),float2(-2 * size.x, -2 * size.y),
    float2(+2 * size.x, -size.y),     float2(+size.x, -size.y),     float2(0.0f, -size.y),     float2(-size.x, -size.y),    float2(-2 * size.x, -size.y),
    float2(+2 * size.x, 0.0f),        float2(+size.x, 0.0f),        float2(0.0f, 0.0f),        float2(-size.x, 0.0f),       float2(-2 * size.x, 0.0f),
    float2(+2 * size.x, +size.y),     float2(+size.x, +size.y),     float2(0.0f, +size.y),     float2(-size.x, +size.y),    float2(-2 * size.x, +size.y),
    float2(+2 * size.x, +2 * size.y), float2(+size.x, +2 * size.y), float2(0.0f, +2 * size.y), float2(-size.x, +2 * size.y),float2(-2 * size.x, +2 * size.y),

};
static const float weight[] =
{
    1, 1, 2, 1, 1,
            1, 2, 4, 2, 1,
            2, 4, 8, 4, 2,
            1, 2, 4, 2, 1,
            1, 1, 2, 1, 1,
};

void Gaussianblur(inout float totalweight, inout float sum,float2 inputUV)
{
    float2 uv = 0;
     [unroll(25)]
    for (int i = 0; i < 25; i++)
    {
        uv = float2(inputUV.xy + offsets[i]);
        totalweight += weight[i];

      
        sum += LightRaysTex.Sample(LinearSampler, uv) * weight[i];

    }
}

float4 CombinePS( VS_OUTPUT In ) : SV_Target0
{
	// Ge the ray intensity
    float sum = 0.0f;
    float totalweight = 0.0f;
     
    Gaussianblur(totalweight, sum, In.Uv);
    float factor = sum / totalweight;

	// Return the color scaled by the intensity
  
    //return float4(rayIntensity, 0, 0, 1);
    return float4(RayColor * saturate(factor), 1.0);
}


BlendState blendState
{
    BlendEnable[0] = true;

    DestBlend[0] = ONE;
//SRC_COLOR;
    SrcBlend[0] = ONE;
    BlendOp[0] = Add;

    DestBlendAlpha[0] = ONE;
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
    //RenderTargetWriteMask[0] = 15;

  
};

////////////////////////////////////////////////////////////////////////////////////////////
technique11 T0
{
    pass p0
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        SetComputeShader(CompileShader(cs_5_0, Occlussion()));
    }
    pass P1
    {
       
        SetVertexShader(CompileShader(vs_5_0, RayTraceVS()));
        SetPixelShader(CompileShader(ps_5_0, RayTracePS()));
    }
    pass P2
    {
        SetBlendState(blendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, RayTraceVS()));
        SetPixelShader(CompileShader(ps_5_0, CombinePS()));
    }
   
}
