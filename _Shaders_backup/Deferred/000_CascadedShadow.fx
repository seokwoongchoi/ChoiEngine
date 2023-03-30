
/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////   CascadedShadow   /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
cbuffer CB_CasecadedShadow
{
    matrix CascadeViewProj[3];

};
float4 tempVS(float4 Pos : POSITION) : SV_Position0
{
    return Pos;
}
struct GS_OUTPUT
{
    float4 Pos : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};


static const  float2 size = 1.0f / float2(1280, 720);

static const float2 offsets[] =
{
    float2(+2 * size.x, -2 * size.y), float2(+size.x, -2 * size.y), //float2(0.0f, -2 * size.y), float2(-size.x, -2 * size.y), float2(-2 * size.x, -2 * size.y),
    float2(+2 * size.x, -size.y), float2(+size.x, -size.y),         //float2(0.0f, -size.y), float2(-size.x, -size.y), float2(-2 * size.x, -size.y),
    float2(+2 * size.x, 0.0f), float2(+size.x, 0.0f),               //float2(0.0f, 0.0f), float2(-size.x, 0.0f), float2(-2 * size.x, 0.0f),
    float2(+2 * size.x, +size.y), float2(+size.x, +size.y),         //float2(0.0f, +size.y), float2(-size.x, +size.y), float2(-2 * size.x, +size.y),
    float2(+2 * size.x, +2 * size.y), float2(+size.x, +2 * size.y), //float2(0.0f, +2 * size.y), float2(-size.x, +2 * size.y), float2(-2 * size.x, +2 * size.y),

};
static const float weight[] =
{
    1, 1, 2, 1, 1,
            1, 2, 4, 2, 1,
            2, 4, 8, 4, 2,
            1, 2, 4, 2, 1,
            1, 1, 2, 1, 1,
};


void Gaussianblur(inout float totalweight, inout float sum, float3 UVD, float bestCascade)
{
    float3 uv = 0;
     [unroll(10)]
    for (int i = 0; i < 10; i++)
    {
        uv = float3(UVD.xy + offsets[i], bestCascade);
        totalweight += weight[i];

      
        sum += CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(uv), UVD.z) * weight[i];

    }
}

float CascadedShadow(float3 position)
{
	// Transform the world position to shadow space
    float4 posShadowSpace = mul(float4(position, 1.0), GlobalLight.ToShadowSpace);

	// Transform the shadow space position into each cascade position
    float4 posCascadeSpaceX = (GlobalLight.ToCascadeOffsetX + posShadowSpace.xxxx) * GlobalLight.ToCascadeScale;
    float4 posCascadeSpaceY = (GlobalLight.ToCascadeOffsetY + posShadowSpace.yyyy) * GlobalLight.ToCascadeScale;
   
	// Check which cascade we are in
    float4 inCascadeX = abs(posCascadeSpaceX) <= 1.0;
    float4 inCascadeY = abs(posCascadeSpaceY) <= 1.0;
    float4 inCascade = inCascadeX * inCascadeY;

	// Prepare a mask for the highest quality cascade the position is in
    float4 bestCascadeMask = inCascade;
    bestCascadeMask.yzw = (1.0 - bestCascadeMask.x) * bestCascadeMask.yzw;
    bestCascadeMask.zw = (1.0 - bestCascadeMask.y) * bestCascadeMask.zw;
    bestCascadeMask.w = (1.0 - bestCascadeMask.z) * bestCascadeMask.w;
    float bestCascade = dot(bestCascadeMask, float4(0.0, 1.0, 2.0, 3.0));

	// Pick the position in the selected cascade
    float3 UVD;
    UVD.x = dot(posCascadeSpaceX, bestCascadeMask);
    UVD.y = dot(posCascadeSpaceY, bestCascadeMask);
    UVD.z = posShadowSpace.z;
  //  UVD.z -= GlobalLight.Bias;
	// Convert to shadow map UV values
    UVD.xy = 0.5 * UVD.xy + 0.5;
    UVD.y = 1.0 - UVD.y;

	// Compute the hardware PCF value
 //float shadow = CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(UVD.xy, bestCascade), UVD.z);
	// set the shadow to one (fully lit) for positions with no cascade coverage
 //shadow = saturate(shadow + 1.0 - any(bestCascadeMask));

    //blur
    
    //return shadow;

    float sum = 0.0f;
    float totalweight = 0.0f;

    float3 uv = 0.0f;
    
    Gaussianblur(totalweight, sum, UVD, bestCascade);
  
    float factor = sum / totalweight;
  
    factor = saturate(factor + UVD.z);
  
   
    return factor;

}


[maxvertexcount(9)]
void CascadedShadowMapsGenGS(triangle MeshOutput InPos[3] : SV_Position, inout TriangleStream<GS_OUTPUT> OutStream)
{
    //float3 xProd = cross(normalize(InPos[2].wPosition - InPos[0].wPosition),
   // normalize(InPos[2].wPosition - InPos[0].wPosition));
    //then, if the x component of the input triangle is positive,   
    //discard the current triangle altogether by simply not     
    //outputting it to the rasterizer:     
    //[flatten]
    //if (xProd.x == 0.0f)
    //{
    //    return;
    //}
         
    [unroll(3)]
    for (int iFace = 0; iFace < 3; iFace++)
    {
        GS_OUTPUT output;

        output.RTIndex = iFace;
        [unroll(3)]
        for (int v = 0; v < 3; v++)
        {
           
           // int culled[4];
         
            output.Pos = mul(float4(InPos[v].wPosition, 1.0f), CascadeViewProj[iFace]);
            OutStream.Append(output);
        }
        OutStream.RestartStrip();
    }
}


