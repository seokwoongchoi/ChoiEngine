Texture2D HDRTex;
StructuredBuffer<float> AvgLum;
Texture2D<float4> BloomTex;
Texture2D<float4> DOFBlurTex ;
Texture2D<float> DepthTex ;

SamplerState PointSampler
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = WRAP;
    AddressV = WRAP;
};

SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};



static const float2 arrBasePos[4] =
{
    float2(-1.0, 1.0),
	float2(1.0, 1.0),
	float2(-1.0, -1.0),
	float2(1.0, -1.0),
};

static const float2 arrUV[4] =
{
    float2(0.0, 0.0),
	float2(1.0, 0.0),
	float2(0.0, 1.0),
	float2(1.0, 1.0),
};

//-----------------------------------------------------------------------------------------
// Vertex shader
//-----------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Position : SV_Position; // vertex position 
	float2 UV		: Uv0;
};

VS_OUTPUT FullScreenQuadVS( uint VertexID : SV_VertexID )
{
    VS_OUTPUT Output;

    Output.Position = float4( arrBasePos[VertexID].xy, 0.0, 1.0);
    Output.UV = arrUV[VertexID].xy;
    
    return Output;    
}

//-----------------------------------------------------------------------------------------
// Pixel shader
//-----------------------------------------------------------------------------------------

cbuffer FinalPassConstants
{
	// Tone mapping
	float MiddleGrey;
	float LumWhiteSqr;
   
    float2 ProjValues;
    float2 DOFFarValues;
    
    float BloomScale;

}

static const float3 LUM_FACTOR = float3(0.299, 0.587, 0.114);

float ConvertZToLinearDepth(float depth)
{
    float linearDepth = ProjValues.x / (depth + ProjValues.y);
    return linearDepth;
}

float3 DistanceDOF(float3 colorFocus, float3 colorBlurred, float depth)
{
	// Find the depth based blur factor
    float blurFactor = saturate((depth - DOFFarValues.x) * DOFFarValues.y);

	// Lerp with the blurred color based on the CoC factor
    return lerp(colorFocus, colorBlurred, blurFactor);
}

float3 ToneMapping(float3 HDRColor)
{
    float LScale = dot(HDRColor, LUM_FACTOR);
    LScale *= MiddleGrey / AvgLum[0];
    LScale = (LScale + LScale * LScale / LumWhiteSqr) / (1.0 + LScale);
    //LScale = saturate(LScale);

	// Apply the luminance scale to the pixels color
    return HDRColor * LScale;
	
}

float4 FinalPassPS( VS_OUTPUT In ) : SV_TARGET
{
	// Get the color sample
    float3 color = HDRTex.Sample(PointSampler, In.UV.xy).xyz;
   
    float depth = DepthTex.Sample(PointSampler, In.UV.xy);
   [flatten]
   if (depth < 1.0)
   {
        float3 colorBlurred = DOFBlurTex.Sample(LinearSampler, In.UV.xy).xyz;
        depth = ConvertZToLinearDepth(depth);
        color = DistanceDOF(color, colorBlurred, depth);
   }
   
    color += BloomScale * BloomTex.Sample(LinearSampler, In.UV.xy).xyz;
    
    color = ToneMapping(color);
    //color = BloomScale * BloomTex.Sample(LinearSampler, In.UV.xy).xyz;
    return float4(color, 1.0);
}



////////////////////////////////////////////////////////////////////////////////////////////



technique11 T0
{
  
     pass p0
    {
        SetVertexShader(CompileShader(vs_5_0, FullScreenQuadVS()));
        SetPixelShader(CompileShader(ps_5_0, FinalPassPS()));
           
    }

 
    
}
  