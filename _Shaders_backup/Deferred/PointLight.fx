#include "common.fx"



/////////////////////////////////////////////////////////////////////////////
//Point Vertex shader
/////////////////////////////////////////////////////////////////////////////
float4 PointLightVS() : SV_Position0
{
    return float4(0.0, 0.0, 0.0, 1.0); 
}

/////////////////////////////////////////////////////////////////////////////
// Point Hull shader
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
	float4 HemiDir : POSITION;
};

static const float3 HemilDir[2] = {
	float3(1.0, 1.0,1.0),
    float3(-1.0, 1.0, -1.0)
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("PointLightConstantHS")]
[maxtessfactor(64)]
HS_OUTPUT PointLightHS(uint PatchID : SV_PrimitiveID)
{
	HS_OUTPUT Output;

    Output.HemiDir = float4(HemilDir[PatchID], 0.0f);

	return Output;
}

/////////////////////////////////////////////////////////////////////////////
// Domain Shader shader
/////////////////////////////////////////////////////////////////////////////
struct DS_OUTPUT
{
	float4 Position : SV_POSITION;
    float2 cpPos : Uv0;
};

[domain("quad")]
DS_OUTPUT PointLightDS( HS_CONSTANT_DATA_OUTPUT input, float2 UV : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 4> quad)
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
	Output.Position = mul( posLS, LightProjection );

	// Store the clip space position
	Output.cpPos = Output.Position.xy / Output.Position.w;

	return Output;
}

/////////////////////////////////////////////////////////////////////////////
// Pixel shader
/////////////////////////////////////////////////////////////////////////////

float3 CalcPoint(float3 position, Material material, bool bUseShadow)
{
    float3 ambient = 0;
    float3 diffuse = 0;
    float3 specular = 0;
    float3 finalColor = 0;
    float3 ToLight = PointLight.Position - position;
    float3 ToEye = EyePosition() - position;
    float DistToLight = length(ToLight);
  
    //float bumpMap = saturate(dot(material.bump, -GlobalLight.Direction));
   
  
    // Phong diffuse
    ToLight /= DistToLight; // Normalize
    ambient = material.Ambient * PointLight.Ambient;
    float NDotL = saturate(dot(float3(ToLight.x, ToLight.y, ToLight.z), material.bump));
    diffuse = material.diffuseColor * PointLight.Diffuse * NDotL;

    // Blinn specular
    ToEye = normalize(ToEye);
    float3 HalfWay = normalize(ToEye + ToLight);
    float NDotH = saturate(dot(HalfWay, normalize(material.normal)));
    float3 R = normalize(reflect(-ToLight, material.normal));
    float RdotE = saturate(dot(R, normalize(ToEye)));
    specular = pow(NDotH, material.specPow.a) * material.specPow.rgb * PointLight.Specular.rgb;


    // Attenuation
    float DistToLightNorm = 1.0 - saturate(DistToLight / PointLight.Range);
    float Attn = DistToLightNorm * DistToLightNorm * (1.0f / PointLight.Intensity);
    //diffuse *= diffuse;
    ambient += ambient;
    diffuse += diffuse * NDotL;
    //specular += specular ;
    
    finalColor = ambient + diffuse + specular;
  // finalColor *= diffuse;
    
   
    finalColor *= NDotL;
    finalColor *= Attn;
  //  finalColor *= finalColor;
    return finalColor;
}

//float3 CalcPoint(float3 position, Material material, bool bUseShadow)
//{

    
//    float3 ToLight = PointLights[0].Position - position;
//    float3 ToEye = EyePosition() - position;
//    float DistToLight = length(ToLight);
    
//    float bumpMap = saturate(dot(material.bump, -GlobalLight.Direction));
//    [flatten]
//    if (DistToLight > PointLights[0].Range)
//        return material.diffuseColor.rgb ;

//    // Phong diffuse
//    ToLight /= DistToLight; // Normalize
//    float NDotL = dot(ToLight, normalize(material.normal));
//    float3 finalColor = material.diffuseColor * bumpMap;

//    // Blinn specular
//    ToEye = normalize(ToEye);
//    float3 HalfWay = normalize(ToEye + ToLight);
//    float NDotH = saturate(dot(HalfWay, normalize(material.normal)));
//    finalColor += pow(NDotH, material.specPow.a) * material.specPow.rgb;

  
  
//    // Attenuation
//    float DistToLightNorm = 1.0 - saturate(DistToLight / PointLights[0].Range);
//    float Attn = DistToLightNorm * DistToLightNorm * (1.0f / PointLights[0].Intensity);;
   
//    finalColor *= PointLights[0].Diffuse * Attn;
   
//    return finalColor;
//}



float4 PointLightCommonPS(DS_OUTPUT In, bool bUseShadow) : SV_TARGET
{
	// Unpack the GBuffer
    SURFACE_DATA gbd = UnpackGBuffer_Loc(In.Position.xy);
	
	// Convert the data into the material structure
	Material mat;
	MaterialFromGBuffer(gbd, mat);
    
	// Reconstruct the world position
	float3 position = CalcWorldPos(In.cpPos, gbd.LinearDepth);

	// Calculate the light contribution
	float3 finalColor = CalcPoint(position, mat, bUseShadow);

	// return the final color
   // return float4(1, 0, 0, 1);
	return float4(finalColor, 1.0f);
}

float4 PointLightPS(DS_OUTPUT In) : SV_TARGET
{
	return PointLightCommonPS(In, false);
}

float4 PointLightShadowPS(DS_OUTPUT In) : SV_TARGET
{
	return PointLightCommonPS(In, true);
}

RasterizerState RS
{
   // FillMode = WireFrame;
    CullMode = Front;
};

DepthStencilState pointDSS
{
    DepthEnable = true;
    DepthWriteMask =Zero;
    DepthFunc = Greater;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;
   // DepthFunc = Less;
   
};

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
    RenderTargetWriteMask[0] = 15;

    
};

technique11 T0
{
  
    pass P0
    {
        SetRasterizerState(RS);
        SetDepthStencilState(pointDSS, 0);
        SetBlendState(blendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, PointLightVS()));
        SetHullShader(CompileShader(hs_5_0, PointLightHS()));
        SetDomainShader(CompileShader(ds_5_0, PointLightDS()));
        
        SetPixelShader(CompileShader(ps_5_0, PointLightShadowPS()));
    }
    
   
}
