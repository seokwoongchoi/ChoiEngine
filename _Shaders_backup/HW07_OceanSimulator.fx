#include "000_Header.fx"


// Constants

#define PI 3.1415926536f
#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16

// Textures and sampling states
Texture2D samplerDisplacementMap;


// The following three should contains only real numbers. But we have only C2C FFT now.
StructuredBuffer<float2> InputDxyz;

StructuredBuffer<float2> InputH0;
StructuredBuffer<float> InputOmega;
RWStructuredBuffer<float2> OutputHt;

struct ImmutableDesc
{
    uint ActualDim;
    uint InWidth;
    uint OutWidth;
    uint OutHeight;
    uint DxAddressOffset;
    uint DyAddressOffset;
};

cbuffer CB_Immutable
{
    ImmutableDesc ImmuteData;
};
struct ChangePerFrameDesc
{
    float Time;
    float ChoppyScale;
    float GridLen;
};
cbuffer CB_ChangePerFrame
{
    ChangePerFrameDesc ChangePF;
};
//---------------------------------------- Vertex Shaders ------------------------------------------
struct VS_QUAD_OUTPUT
{
    float4 Position : SV_POSITION; // vertex position
    float2 TexCoord : TEXCOORD0; // vertex texture coords 
};

VS_QUAD_OUTPUT QuadVS(float4 vPos : POSITION)
{
    VS_QUAD_OUTPUT Output;

    Output.Position = vPos;
    Output.TexCoord.x = 0.5f + vPos.x * 0.5f;
    Output.TexCoord.y = 0.5f - vPos.y * 0.5f;

    return Output;
}

//----------------------------------------- Pixel Shaders ------------------------------------------


// Post-FFT data wrap up: Dx, Dy, Dz -> Displacement
float4 UpdateDisplacementPS(VS_QUAD_OUTPUT In) : SV_Target
{
    uint index_x = (uint) (In.TexCoord.x * (float) ImmuteData.OutWidth);
    uint index_y = (uint) (In.TexCoord.y * (float) ImmuteData.OutHeight);
    uint addr = ImmuteData.OutWidth * index_y + index_x;

	// cos(pi * (m1 + m2))
    int sign_correction = ((index_x + index_y) & 1) ? -1 : 1;

    float dx = InputDxyz[addr + ImmuteData.DxAddressOffset].x * sign_correction * ChangePF.ChoppyScale;
    float dy = InputDxyz[addr + ImmuteData.DyAddressOffset].x * sign_correction * ChangePF.ChoppyScale;
    float dz = InputDxyz[addr].x * sign_correction;

    return float4(dx, dy, dz, 1);
}

// Displacement -> Normal, Folding
float4 GenGradientFoldingPS(VS_QUAD_OUTPUT In) : SV_Target
{
	// Sample neighbour texels
    float2 one_texel = float2(1.0f / (float) ImmuteData.OutWidth, 1.0f / (float) ImmuteData.OutHeight);

    float2 tc_left = float2(In.TexCoord.x - one_texel.x, In.TexCoord.y);
    float2 tc_right = float2(In.TexCoord.x + one_texel.x, In.TexCoord.y);
    float2 tc_back = float2(In.TexCoord.x, In.TexCoord.y - one_texel.y);
    float2 tc_front = float2(In.TexCoord.x, In.TexCoord.y + one_texel.y);

    float3 displace_left = samplerDisplacementMap.Sample(LinearSampler, tc_left).xyz;
    float3 displace_right = samplerDisplacementMap.Sample(LinearSampler, tc_right).xyz;
    float3 displace_back = samplerDisplacementMap.Sample(LinearSampler, tc_back).xyz;
    float3 displace_front = samplerDisplacementMap.Sample(LinearSampler, tc_front).xyz;
	
	// Do not store the actual normal value. Using gradient instead, which preserves two differential values.
    float2 gradient = { -(displace_right.z - displace_left.z), -(displace_front.z - displace_back.z) };
	

	// Calculate Jacobian corelation from the partial differential of height field
    float2 Dx = (displace_right.xy - displace_left.xy) * ChangePF.ChoppyScale * ChangePF.GridLen;
    float2 Dy = (displace_front.xy - displace_back.xy) * ChangePF.ChoppyScale * ChangePF.GridLen;
    float J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;

	// Practical subsurface scale calculation: max[0, (1 - J) + Amplitude * (2 * Coverage - 1)].
    float fold = max(1.0f - J, 0);

	// Output
    return float4(gradient, 0, fold);
}

//---------------------------------------- Compute Shaders -----------------------------------------

// Pre-FFT data preparation:

// Notice: In CS5.0, we can output up to 8 RWBuffers but in CS4.x only one output buffer is allowed,
// that way we have to allocate one big buffer and manage the offsets manually. The restriction is
// not caused by NVIDIA GPUs and does not present on NVIDIA GPUs when using other computing APIs like
// CUDA and OpenCL.

// H(0) -> H(t)
[numthreads(BLOCK_SIZE_X, BLOCK_SIZE_Y, 1)]
void UpdateSpectrumCS(uint3 DTid : SV_DispatchThreadID)
{
    int in_index = DTid.y * ImmuteData.InWidth + DTid.x;
    int in_mindex = (ImmuteData.ActualDim - DTid.y) * ImmuteData.InWidth + (ImmuteData.ActualDim - DTid.x);
    int out_index = DTid.y * ImmuteData.OutWidth + DTid.x;

	// H(0) -> H(t)
    float2 h0_k = InputH0[in_index];
    float2 h0_mk = InputH0[in_mindex];
    float sin_v, cos_v;
    sincos(InputOmega[in_index] * ChangePF.Time, sin_v, cos_v);

    float2 ht;
    ht.x = (h0_k.x + h0_mk.x) * cos_v - (h0_k.y + h0_mk.y) * sin_v;
    ht.y = (h0_k.x - h0_mk.x) * sin_v + (h0_k.y - h0_mk.y) * cos_v;

	// H(t) -> Dx(t), Dy(t)
    float kx = DTid.x - ImmuteData.ActualDim * 0.5f;
    float ky = DTid.y - ImmuteData.ActualDim * 0.5f;
    float sqr_k = kx * kx + ky * ky;
    float rsqr_k = 0;
    if (sqr_k > 1e-12f)
        rsqr_k = 1 / sqrt(sqr_k);
	//float rsqr_k = 1 / sqrtf(kx * kx + ky * ky);
    kx *= rsqr_k;
    ky *= rsqr_k;
    float2 dt_x = float2(ht.y * kx, -ht.x * kx);
    float2 dt_y = float2(ht.y * ky, -ht.x * ky);

    if ((DTid.x < ImmuteData.OutWidth) && (DTid.y < ImmuteData.OutHeight))
    {
        OutputHt[out_index] = ht;
        OutputHt[out_index + ImmuteData.DxAddressOffset] = dt_x;
        OutputHt[out_index + ImmuteData.DyAddressOffset] = dt_y;
    }
}

technique11 T0
{

    P_VP(P0, QuadVS, UpdateDisplacementPS)
    P_VP(P1,QuadVS, GenGradientFoldingPS)
    
    pass P2_CS
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        SetComputeShader(CompileShader(cs_5_0, UpdateSpectrumCS()));
    }
}