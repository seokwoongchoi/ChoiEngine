#include "000_Header.fx"

////////////////////////////////////////////////////////////////////////////////
// Constant Value
////////////////////////////////////////////////////////////////////////////////
//cos(45)
#define COS_PI_4_16 0.70710678118654752440084436210485f
#define TWIDDLE_1_8 COS_PI_4_16, -COS_PI_4_16
#define TWIDDLE_3_8 -COS_PI_4_16, -COS_PI_4_16

#define COHERENCY_GRANULARITY 128
////////////////////////////////////////////////////////////////////////////////
// I/O Data
////////////////////////////////////////////////////////////////////////////////
struct ChangePerCall
{
    uint ThreadCount;
    uint OStride;
    uint IStride;
    uint PStride;
    float PhaseBase;
};
cbuffer CB_ChangePerCall
{
    ChangePerCall ChangeData;
};

StructuredBuffer<float2> FFT_SrcData;
RWStructuredBuffer<float2> FFT_DstData;

////////////////////////////////////////////////////////////////////////////////
// Convolution FFT
////////////////////////////////////////////////////////////////////////////////
void FT2(inout float2 a, inout float2 b)
{
    float t;

    t = a.x;
    a.x += b.x;
    b.x = t - b.x;

    t = a.y;
    a.y += b.y;
    b.y = t - b.y;
}

void CMUL_forward(inout float2 a, float bx, float by)
{
    float t = a.x;
    a.x = t * bx - a.y * by;
    a.y = t * by + a.y * bx;
}

void UPD_forward(inout float2 a, inout float2 b)
{
    float A = a.x;
    float B = b.y;

    a.x += b.y;
    b.y = a.y + b.x;
    a.y -= b.x;
    b.x = A - B;
}

void FFT_forward_4(inout float2 D[8])
{
    FT2(D[0], D[2]);
    FT2(D[1], D[3]);
    FT2(D[0], D[1]);

    UPD_forward(D[2], D[3]);
}

void FFT_forward_8(inout float2 D[8])
{
    FT2(D[0], D[4]);
    FT2(D[1], D[5]);
    FT2(D[2], D[6]);
    FT2(D[3], D[7]);

    UPD_forward(D[4], D[6]);
    UPD_forward(D[5], D[7]);

    CMUL_forward(D[5], TWIDDLE_1_8);
    CMUL_forward(D[7], TWIDDLE_3_8);

    FFT_forward_4(D);
    FT2(D[4], D[5]);
    FT2(D[6], D[7]);
}

void TWIDDLE(inout float2 d, float phase)
{
    //phase -> 2pi*x?
    //cos (ph), sin(ph)
    float tx, ty;

    sincos(phase, ty, tx);
    float t = d.x;
    d.x = t * tx - d.y * ty;    //실수부  cos-i*sin -> e^(-2pi*x)??
    d.y = t * ty + d.y * tx;    //허수부  sin+i*cos ->-i*e^(-2pi*x)??
}

void TWIDDLE_8(inout float2 D[8], float phase)
{
    TWIDDLE(D[4], 1 * phase);
    TWIDDLE(D[2], 2 * phase);
    TWIDDLE(D[6], 3 * phase);
    TWIDDLE(D[1], 4 * phase);
    TWIDDLE(D[5], 5 * phase);
    TWIDDLE(D[3], 6 * phase);
    TWIDDLE(D[7], 7 * phase);
}

////////////////////////////////////////////////////////////////////////////////
// Compute Shader
////////////////////////////////////////////////////////////////////////////////

[numthreads(COHERENCY_GRANULARITY, 1, 1)]
void Radix008A_CS(uint3 thread_id : SV_DispatchThreadID)
{
    if (thread_id.x >= ChangeData.ThreadCount)
        return;

	// Fetch 복소수 8개
    float2 D[8];

    uint i;
    //해당영역의 크기 만큼 자르기 위해 ChangeData.IStride-1 크기로 자름? ChangeData.IStride가 아니고?
    uint imod = thread_id & (ChangeData.IStride - 1);
    //각 영역의 주소값.
    uint iaddr = ((thread_id - imod) << 3) + imod;
    for (i = 0; i < 8; i++)
        D[i] = FFT_SrcData[iaddr + i * ChangeData.IStride];

	// Math
    FFT_forward_8(D);
    uint p = thread_id & (ChangeData.IStride - ChangeData.PStride);
    float phase = ChangeData.PhaseBase * (float) p;
    TWIDDLE_8(D, phase);

	// Store the result
    uint omod = thread_id & (ChangeData.OStride - 1);
    uint oaddr = ((thread_id - omod) << 3) + omod;
    FFT_DstData[oaddr + 0 * ChangeData.OStride] = D[0];
    FFT_DstData[oaddr + 1 * ChangeData.OStride] = D[4];
    FFT_DstData[oaddr + 2 * ChangeData.OStride] = D[2];
    FFT_DstData[oaddr + 3 * ChangeData.OStride] = D[6];
    FFT_DstData[oaddr + 4 * ChangeData.OStride] = D[1];
    FFT_DstData[oaddr + 5 * ChangeData.OStride] = D[5];
    FFT_DstData[oaddr + 6 * ChangeData.OStride] = D[3];
    FFT_DstData[oaddr + 7 * ChangeData.OStride] = D[7];
}


[numthreads(COHERENCY_GRANULARITY, 1, 1)]
void Radix008A_CS2(uint3 thread_id : SV_DispatchThreadID)
{
    if (thread_id.x >= ChangeData.ThreadCount)
        return;

	// Fetch 8 complex numbers
    uint i;
    float2 D[8];
    uint iaddr = thread_id << 3;
    for (i = 0; i < 8; i++)
        D[i] = FFT_SrcData[iaddr + i];

	// Math
    FFT_forward_8(D);

	// Store the result
    uint omod = thread_id & (ChangeData.OStride - 1);
    uint oaddr = ((thread_id - omod) << 3) + omod;
    FFT_DstData[oaddr + 0 * ChangeData.OStride] = D[0];
    FFT_DstData[oaddr + 1 * ChangeData.OStride] = D[4];
    FFT_DstData[oaddr + 2 * ChangeData.OStride] = D[2];
    FFT_DstData[oaddr + 3 * ChangeData.OStride] = D[6];
    FFT_DstData[oaddr + 4 * ChangeData.OStride] = D[1];
    FFT_DstData[oaddr + 5 * ChangeData.OStride] = D[5];
    FFT_DstData[oaddr + 6 * ChangeData.OStride] = D[3];
    FFT_DstData[oaddr + 7 * ChangeData.OStride] = D[7];
}


technique11 T0
{
    pass P0
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        SetComputeShader(CompileShader(cs_5_0, Radix008A_CS()));
    }
    pass P1
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        SetComputeShader(CompileShader(cs_5_0, Radix008A_CS2()));
    }
}