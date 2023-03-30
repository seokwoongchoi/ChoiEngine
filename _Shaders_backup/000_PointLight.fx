#include "000_Header.fx"
#include "000_DeferredLight.fx"
/////////////////////////////////S////////////////////////////////////////////
// constants
/////////////////////////////////////////////////////////////////////////////

cbuffer cbPointLightDomain 
{
    float4x4 LightProjection ;
}



/////////////////////////////////////////////////////////////////////////////
// Vertex shader
/////////////////////////////////////////////////////////////////////////////


   float4 PointLightVS() : SV_Position0
{
   
    return float4(0, 0, 0, 1.0f);
}

/////////////////////////////////////////////////////////////////////////////
// Hull shader
/////////////////////////////////////////////////////////////////////////////
struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};

HS_CONSTANT_DATA_OUTPUT PointLightConstantHS()
{
    HS_CONSTANT_DATA_OUTPUT Output;
	
    float tessFactor = 18.0;
    Output.Edges[0] = Output.Edges[1] = Output.Edges[2] = Output.Edges[3] = tessFactor;
    Output.Inside[0] = Output.Inside[1] = tessFactor;

    return Output;
}

struct HS_OUTPUT
{
    float4 HemiDir : Position0;
};

static const float3 HemilDir[2] =
{
    float3(1.0, 1.0, 1.0),
	float3(-1.0, 1.0, -1.0)
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("PointLightConstantHS")]
HS_OUTPUT PointLightHS(uint PatchID : SV_PrimitiveID)
{
    HS_OUTPUT Output;

    Output.HemiDir = float4(HemilDir[PatchID], 1.0f);

    return Output;
}

/////////////////////////////////////////////////////////////////////////////
// Domain Shader shader
/////////////////////////////////////////////////////////////////////////////
struct DS_OUTPUT
{
    float4 Position : SV_POSITION0;
    float2 cpPos : Uv0;
};

[domain("quad")]
DS_OUTPUT PointLightDS(HS_CONSTANT_DATA_OUTPUT input, float2 UV : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 4> quad)
{
	// Transform the UV's into clip-space
    float2 posClipSpace = UV.xy * 2.0 - 1.0;

	// Find the absulate maximum distance from the center
    float2 posClipSpaceAbs = abs(posClipSpace.xy);
    float maxLen = max(posClipSpaceAbs.x, posClipSpaceAbs.y);

	// Generate the final position in clip-space
    float3 normDir = normalize(float3(posClipSpace.xy, (maxLen - 1.0)) * quad[0].HemiDir.xyz);
    float4 posLS = float4(normDir.xyz, 1.0);
	
	// Transform all the way to projected space
    DS_OUTPUT Output;
    Output.Position = mul(posLS, LightProjection);

	// Store the clip space position
    Output.cpPos = Output.Position.xy / Output.Position.w;

    return Output;
}

/////////////////////////////////////////////////////////////////////////////
// Pixel shader
/////////////////////////////////////////////////////////////////////////////


float4 PointLightCommonPS(DS_OUTPUT In) : SV_Target0
{
	

    float4 unpackNormal = NormalTexture.Sample(LinearSampler, In.cpPos);
    float3 normal = normalize(unpackNormal.xyz * 2.0f - 1.0f);
   
    float bump = unpackNormal.w;
  
    float depth = DepthTexture.Sample(LinearSampler, In.cpPos).r;
    float LinearDepth = ConvertZToLinearDepth(depth);
    float3 wPosition = CalcWorldPos(In.cpPos, LinearDepth);
    
    Texture(Material.Diffuse, ColorSpecIntTexture, In.cpPos);
    Texture(Material.Specular, SpecPowTexture, In.cpPos);
    MulBump(bump);
    MaterialDesc output = MakeMaterial();
  
     
    ComputePointLights(output, normal, wPosition);
    
   
    return float4(MaterialToColor(output), 1.0f);
}



DepthStencilState pointDSS
{
    DepthEnable = true;
    DepthWriteMask = All;
    DepthFunc = Greater_Equal;
   
};

technique11 T0
{
  
    pass P0
    {
       SetDepthStencilState(pointDSS, 0);
        SetVertexShader(CompileShader(vs_5_0, PointLightVS()));
       SetHullShader(CompileShader(hs_5_0, PointLightHS()));
       SetDomainShader(CompileShader(ds_5_0, PointLightDS()));
        
        SetPixelShader(CompileShader(ps_5_0, PointLightCommonPS()));
    }
  
  
   
}