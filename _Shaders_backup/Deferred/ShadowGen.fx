

cbuffer cbuffercbShadowMapCubeGS
{
    matrix CascadeViewProj[3];
};
float4 GenVS(float4 Pos : POSITION) : SV_Position
{
    return Pos;
}

struct GS_OUTPUT
{
	float4 Pos		: SV_POSITION;
	uint RTIndex	: SV_RenderTargetArrayIndex;
};


[maxvertexcount(9)]
void CascadedShadowMapsGenGS(triangle float4 InPos[3] : SV_Position, inout TriangleStream<GS_OUTPUT> OutStream)
{
	for(int iFace = 0; iFace < 3; iFace++ )
	{
		GS_OUTPUT output;

		output.RTIndex = iFace;

		for(int v = 0; v < 3; v++ )
		{
			output.Pos = mul(InPos[v], CascadeViewProj[iFace]);
			OutStream.Append(output);
		}
		OutStream.RestartStrip();
	}
}
RasterizerState RS
{
   // FillMode = WireFrame;
    CullMode = Front;
};
RasterizerState cascadedRS
{

    DepthBias = 6;
    SlopeScaledDepthBias = 1.0f;
    DepthClipEnable = false;
};
technique11 T0
{
    /*Shadow*/
    pass P0
    {
        SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, GenVS()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
   
   
}