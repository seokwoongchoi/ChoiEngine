#include "000_Header.fx"
//-----------------------------------------------------------------------------------------
// Occlusion
//-----------------------------------------------------------------------------------------

Texture2D DepthTex;
RWTexture2D<float> OcclusionRW;

cbuffer OcclusionConstants
{
	uint2 Res;
}


[numthreads(1024,1, 1)]
void Occlussion(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint3 CurPixel = uint3(dispatchThreadId.x % Res.x, dispatchThreadId.x / Res.y, 0);

	// Skip out of bound pixels
	if(CurPixel.y < Res.y)
	{
		// Get the depth
		float curDepth = DepthTex.Load(CurPixel);

		// Flag anything closer than the sky as occlusion
		OcclusionRW[CurPixel.xy].x = curDepth == 0.0;
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

static const float2 arrBasePos[4] =
{
    float2(1.0, -1.0),
    float2(1.0, 1.0),
	float2(-1.0, 1.0),
	float2(-1.0, -1.0)
};

static const float2 arrUV[4] =
{
    float2(1.0, 1.0),
	float2(1.0, 0.0),
	float2(0.0, 0.0),
	float2(0.0, 1.0),
};

//static const float2 arrBasePos[4] = {
//	float2(1.0, 1.0),
//	float2(1.0, -1.0),
//	float2(-1.0, 1.0),
//	float2(-1.0, -1.0),
//};

//static const float2 arrUV[4] = {
//	float2(1.0, 0.0),
//	float2(1.0, 1.0),
//	float2(0.0, 0.0),
//	float2(0.0, 1.0),
//};

struct VS_OUTPUT
{
	float4 Position	: SV_Position0;
	float2 UV		: Uv0;
};

VS_OUTPUT RayTraceVS( uint VertexID : SV_VertexID )
{
    VS_OUTPUT Output;

	Output.Position = float4(arrBasePos[VertexID].xy, 0.0, 1.0);
	Output.UV = arrUV[VertexID].xy;

	return Output;    
}

//static const int NUM_STEPS = 64;
//static const float NUM_DELTA = 1.0 / 63.0f;
float4 RayTracePS( VS_OUTPUT In ) : SV_Target0
{
    int NUM_STEPS = 64;
    float NUM_DELTA = 1.0 / 63.0f;
	// Find the direction and distance to the sun
	float2 dirToSun = (SunPos - In.UV);
	float lengthToSun = length(dirToSun);
	dirToSun /= lengthToSun;

	// Find the ray delta
	float deltaLen = min(MaxDeltaLen, lengthToSun * NUM_DELTA);
	float2 rayDelta = dirToSun * deltaLen;

	// Each step decay	
	float stepDecay = DistDecay * deltaLen;

	// Initial values
	float2 rayOffset = float2(0.0, 0.0);
	float decay = InitDecay;
	float rayIntensity = 0.0f;

	// Ray march towards the sun
	for(int i = 0; i < NUM_STEPS ; i++)
	{
		// Sample at the current location
		float2 sampPos = In.UV + rayOffset;
		float fCurIntensity = OcclusionTex.Sample( LinearSampler, sampPos );
		
		// Sum the intensity taking decay into account
		rayIntensity += fCurIntensity * decay;

		// Advance to the next position
		rayOffset += rayDelta;

		// Update the decay
		decay = saturate(decay - stepDecay);
	}

	return float4(rayIntensity, 0.0, 0.0, 0.0);
}

//-----------------------------------------------------------------------------------------
// Combine results
//-----------------------------------------------------------------------------------------

Texture2D LightRaysTex;

float4 CombinePS( VS_OUTPUT In ) : SV_Target0
{
	// Ge the ray intensity
	float rayIntensity = LightRaysTex.Sample( LinearSampler, In.UV );

	// Return the color scaled by the intensity
	return float4(RayColor * rayIntensity, 1.0);
}

BlendState blendState
{
    BlendEnable[0] =true;

    DestBlend[0] = ONE;
//SRC_COLOR;
    SrcBlend[0] = ONE;
    BlendOp[0] = Add;

    DestBlendAlpha[0] = ONE;
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;

    
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
