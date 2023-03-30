#include "000_Header.fx"
#include "000_DeferredLight.fx"
#include "000_Model.fx"




struct PS_GBuffer_Out
{
    float4 Diffuse : SV_Target0;
    float4 Normal : SV_Target1;
    float4 Specular : SV_Target2;
    float4 Position : SV_Target3;
};



MeshOutput VS_Mesh_PreRender(VertexMesh input)
{
    MeshOutput output = VS_Mesh(input);
    return output;
}

MeshOutput VS_Model_PreRender(VertexModel input)
{
    MeshOutput output = VS_Model(input);
    return output;
}

MeshOutput VS_Animation_PreRender(VertexModel input)
{
    MeshOutput output = VS_Animation(input);
    return output;
}
static const float2 SpecPowerRange = { 0.1f, 250.0f };

PS_GBuffer_Out DrawPS(MeshOutput input)
{
    PS_GBuffer_Out output;
    float4 DiffuseColor = float4(1, 1, 1, 1);
    float4 Specular = float4(1, 1, 1, 1);
    float4 NormalColor = float4(1, 1, 1, 1);
    Texture(DiffuseColor, DiffuseMap, input.Uv);
    Texture(Specular, SpecularMap, input.Uv);
    float bump = 0;
    GetBump(input.Uv, input.Normal, input.Tangent, LinearSampler,bump);
  
     
    output.Diffuse = DiffuseColor;
    output.Specular = Specular;
    output.Normal = float4(normalize(input.Normal) * 0.5f + 0.5f, bump);
    output.Position = float4(input.oPosition, 1.0f);
   
  
    return output;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct VS_OUTPUT
{
    float4 Position : SV_Position0;
    float2 Uv : Uv1;

};

static const float2 arrBasePos[4] =
{
	float2(1.0, -1.0),
    float2(1.0, 1.0),
	float2(-1.0, 1.0),
	float2(-1.0, -1.0),
};
VS_OUTPUT DirLightVS(uint VertexID:SV_VertexID)
{
    VS_OUTPUT output;
    output.Position = float4(arrBasePos[VertexID].xy, 0.0f, 1.0f);
    output.Uv = float2(output.Position.x, -output.Position.y) * 0.5f + 0.5f;

    return output;

}


float4 DirLightPS(VS_OUTPUT In) : SV_Target0
{
	// Unpack the GBuffer
  
    float4 unpackNormal = NormalTexture.Sample(LinearSampler, In.Uv);
    float3 normal = normalize(unpackNormal.xyz * 2.0f - 1.0f);
   
    float bump = unpackNormal.w;
  
    float depth = DepthTexture.Sample(LinearSampler, In.Uv).r;
    float LinearDepth = ConvertZToLinearDepth(depth);
    float3 wPosition = CalcWorldPos(In.Uv, LinearDepth);
    
    Texture(Material.Diffuse, ColorSpecIntTexture, In.Uv);
    Texture(Material.Specular, SpecPowTexture, In.Uv);
    MulBump(bump);
    MaterialDesc output = MakeMaterial();
    MaterialDesc result = MakeMaterial();
    ComputeLight(output, normal, wPosition);
   
    
    return float4(MaterialToColor(output), 1.0f);

}
//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// constants
/////////////////////////////////////////////////////////////////////////////

cbuffer cbPointLightDomain
{
    float4x4 LightProjection;
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
	

    float4 unpackNormal = NormalTexture.Sample(LinearSampler, In.Position.xy);
    float3 normal = normalize(unpackNormal.xyz * 2.0f - 1.0f);
   
    float bump = unpackNormal.w;
  
    float depth = DepthTexture.Sample(LinearSampler, In.Position.xy).r;
    float LinearDepth = ConvertZToLinearDepth(depth);
    float3 wPosition = CalcWorldPos(In.cpPos, LinearDepth);
    
    Texture(Material.Diffuse, ColorSpecIntTexture, In.Position.xy);
    Texture(Material.Specular, SpecPowTexture, In.Position.xy);
    MulBump(bump);
    MaterialDesc output = MakeMaterial();
  
     
    ComputePointLights(output, normal, wPosition);
    
   // return Material.Diffuse;
    return float4(MaterialToColor(output), 1.0f);
}

///////////////////////////////////////////////////////////////////////////////

DepthStencilState pointDSS
{
    DepthEnable = true;
    DepthWriteMask = All;
    DepthFunc = Greater_Equal;
   
};



technique11 T0
{
  
    /*Deferred Packing */
    P_VP(P0, VS_Mesh_PreRender, DrawPS)
    P_VP(P1, VS_Model_PreRender, DrawPS)
    P_VP(P2, VS_Animation_PreRender, DrawPS)
    
    /* Render */
    P_VP(P3, DirLightVS, DirLightPS)
  
  
    pass P4
    {
        SetDepthStencilState(pointDSS, 0);
        SetVertexShader(CompileShader(vs_5_0, PointLightVS()));
        SetHullShader(CompileShader(hs_5_0, PointLightHS()));
        SetDomainShader(CompileShader(ds_5_0, PointLightDS()));
        
        SetPixelShader(CompileShader(ps_5_0, PointLightCommonPS()));
    }
   
}