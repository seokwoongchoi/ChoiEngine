TextureCube EnvMap;
SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};
static const float PI = 3.14159265359;
float NormalDistributionGGXTR(float3 normalVec, float3 halfwayVec, float roughness)
{
    float PI = 3.14159265359;
    float a = roughness * roughness;
    float a2 = a * a; // a2 = a^2
    float NdotH = max(dot(normalVec, halfwayVec), 0.0); // NdotH = normalVec.halfwayVec
    float NdotH2 = NdotH * NdotH; // NdotH2 = NdotH^2
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / denom;
}
float IBLGeometrySchlickGGX(float NdotV, float roughness)  // k is a remapping of roughness based on direct lighting or IBL lighting
{
    float r = roughness;
    float k = (r * r) / 2.0f;

    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}
float IBLGeometrySmith(float3 normalVec, float3 viewDir, float3 lightDir, float k)
{
    float NdotV = max(dot(normalVec, viewDir), 0.0f);
    float NdotL = max(dot(normalVec, lightDir), 0.0f);
    float ggx1 = IBLGeometrySchlickGGX(NdotV, k);
    float ggx2 = IBLGeometrySchlickGGX(NdotL, k);

    return ggx1 * ggx2;
}
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}
float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 normalVec)
{
    float PI = 3.14159265359;
    float a = Roughness * Roughness;
    float Phi = 2.0f * PI * Xi.x;
    float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
    float SinTheta = sqrt(1 - CosTheta * CosTheta);
	// from spherical coordinates to cartesian coordinates - halfway vector
    float3 halfwayVec;
    halfwayVec.x = SinTheta * cos(Phi);
    halfwayVec.y = SinTheta * sin(Phi);
    halfwayVec.z = CosTheta;
	// from tangent-space H vector to world-space sample vector
    float3 UpVector = abs(normalVec.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, normalVec));
    float3 TangentY = cross(normalVec, TangentX);
	// Tangent to world space
    return normalize((TangentX * halfwayVec.x) + (TangentY * halfwayVec.y) + (normalVec * halfwayVec.z));
}

cbuffer ExternalData
{
    float roughness;
}
struct VertexToPixel
{
    float4 position : SV_POSITION;
    float3 uvw : TEXCOORD;
};

float4 main(VertexToPixel input) : SV_TARGET
{
    float3 normalVec = normalize(input.uvw);
    float3 R = normalVec;
    float3 viewDir = R;

    float3 PrefilteredColor = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;
	
    const uint NumSamples = 1024;
    for (uint i = 0; i < NumSamples; i++)
    {
        float2 Xi = Hammersley(i, NumSamples);
        float3 halfwayVec = ImportanceSampleGGX(Xi, roughness, normalVec);
        float3 lightDir = 2 * dot(viewDir, halfwayVec) * halfwayVec - viewDir;
        float NdotL = saturate(dot(normalVec, lightDir));
        if (NdotL > 0)
        {
			// sample from the environment's mip level based on roughness/pdf
            float D = NormalDistributionGGXTR(normalVec, halfwayVec, roughness);
            float NdotH = max(dot(normalVec, halfwayVec), 0.0f);
            float HdotV = max(dot(halfwayVec, viewDir), 0.0f);
            float pdf = D * NdotH / (4.0f * HdotV) + 0.0001f;

            float resolution = 512.0f; // resolution of source cubemap (per face)
            float saTexel = 4.0f * PI / (6.0f * resolution * resolution);
            float saSample = 1.0f / (float(NumSamples) * pdf + 0.0001f);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            PrefilteredColor += EnvMap.SampleLevel(LinearSampler, lightDir, 0).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    PrefilteredColor /= totalWeight;

    return float4(PrefilteredColor, 1.0f);
}
technique11 T0
{
    
 
   
    pass P0
    {
      
        SetVertexShader(CompileShader(vs_5_0, VS_Mesh_PreRender()));
        SetPixelShader(CompileShader(ps_5_0, main()));
        

    }

   
}