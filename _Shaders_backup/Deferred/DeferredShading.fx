#include "000_Header.fx"
#include "000_Model.fx"
#include "000_Terrain.fx"
#include "common.fx"

/////////////////////////////////////////////////////////////////////////////
// Constant Buffers
/////////////////////////////////////////////////////////////////////////////

cbuffer cbPerObjectPS // Model pixel shader constants
{
	float specExp;
	float specIntensity;
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


struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_Target0;
   float4 Normal : SV_Target1;
	float4 SpecPow : SV_Target2;
    float4 BumpMap : SV_Target3;
};
void GetBumpMapCoord(float2 uv, float3 normal, float3 tangent, SamplerState samp, inout float3 bump)
{
    float4 map = NormalMap.Sample(samp, uv);

    [flatten]
    if (any(map) == false)
        return;


    //탄젠트 공간
    float3 N = normalize(normal); //Z
    float3 T = normalize(tangent - dot(tangent, N) * N); //X
    float3 B = cross(N, T); //Y
    float3x3 TBN = float3x3(T, B, N);

    //이미지로부터 노멀벡터 가져오기
    float3 coord = map.rgb * 2.0f - 1.0f;

    //탄젠트 공간으로 변환
    coord = mul(coord, TBN);

    bump = coord;
}
PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 Normal, float4 Specular,float3 bump)
{
	PS_GBUFFER_OUT Out;


	Out.ColorSpecInt = float4(BaseColor.rgb,1.0f);
    
    Out.Normal = float4(Normal * 0.5 + 0.5, 0.0f);
    Out.SpecPow = Specular;
    Out.BumpMap = float4(bump * 0.5 + 0.5, 0.0f);

	return Out;
}

PS_GBUFFER_OUT RenderScenePS(MeshOutput In)
{ 
    // Lookup mesh texture and modulate it with diffuse
    float3 DiffuseColor = DiffuseMap.Sample( LinearSampler, In.Uv );
    float4 Specular = SpecularMap.Sample(LinearSampler, In.Uv);
	//DiffuseColor *= DiffuseColor;
   // DiffuseColor += float4(0.15f, 0.15f, 0.15f, 0.0f);
    float3 bump=0;
    GetBumpMapCoord(In.Uv, In.Normal, In.Tangent, LinearSampler, bump);
    return PackGBuffer(DiffuseColor, normalize(In.Normal), Specular, bump);
}

////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////    Terrain     ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

VertexOutput_Lod VS(VertexInput_Lod input)
{
    VertexOutput_Lod output;
    output.Position = WorldPosition(input.Position);
    output.Uv = input.Uv;
    output.BoundsY = input.BoundsY;

    return output;
}



ConstantHullOutput_Lod HS_Constant(InputPatch<VertexOutput_Lod, 4> input)
{

    float minY = input[0].BoundsY.x;
    float maxY = input[0].BoundsY.y;

    float3 vMin = float3(input[2].Position.x, minY, input[2].Position.z); //0이하단 2 우상단
    float3 vMax = float3(input[1].Position.x, maxY, input[1].Position.z); //0이하단 2 우상단
    

    float3 boxCenter = (vMin + vMax) * 0.5f;
    float3 boxExtents = (vMax - vMin) * 0.5f;
    //어떻게 짜를지 정함
    ConstantHullOutput_Lod output;
    [flatten]
    if (ContainFrustumCube(boxCenter, boxExtents))
    {
        output.Edge[0] = 0;
        output.Edge[1] = 0;
        output.Edge[2] = 0;
        output.Edge[3] = 0;

        output.Inside[0] = 0;
        output.Inside[1] = 0;

        return output;
    }

    float3 e0 = (input[0].Position + input[2].Position).xyz * 0.5f;
    float3 e1 = (input[0].Position + input[1].Position).xyz * 0.5f;
    float3 e2 = (input[1].Position + input[3].Position).xyz * 0.5f;
    float3 e3 = (input[2].Position + input[3].Position).xyz * 0.5f;

    float3 center = (input[0].Position + input[1].Position + input[2].Position + input[3].Position).xyz * 0.25;
    
    output.Edge[0] = TessFactor(e0); //이 선을 2개로 분할한다.
    output.Edge[1] = TessFactor(e1);
    output.Edge[2] = TessFactor(e2);
    output.Edge[3] = TessFactor(e3);

    output.Inside[0] = TessFactor(center);
    output.Inside[1] = TessFactor(center);

    return output;
}

struct HullOutput
{
    float4 Position : Position0;
};

[domain("quad")] //어떤단계로 영역을 묶어서 출력할것인가
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("HS_Constant")]
[maxtessfactor(64)]
HullOutput_Lod HS(InputPatch<VertexOutput_Lod, 4> input, uint point0 : SV_OutputControlPointID)
{
    //짜르기전에 할 행동
    HullOutput_Lod output;
    output.Position = input[point0].Position;
    output.Uv = input[point0].Uv;
    return output;
}



[domain("quad")]
DomainOutput_Lod DS(ConstantHullOutput_Lod input, float2 uv : SV_DomainLocation, const OutputPatch<HullOutput_Lod, 4> patch)
{
    DomainOutput_Lod output;
    
    float3 v0 = lerp(patch[0].Position.xyz, patch[1].Position.xyz, uv.x);
    float3 v1 = lerp(patch[2].Position.xyz, patch[3].Position.xyz, uv.x);
    float3 position = lerp(v0, v1, uv.y); //베지어

    output.wPosition = position;
    
    float2 uv0 = lerp(patch[0].Uv, patch[1].Uv, uv.x);
    float2 uv1 = lerp(patch[2].Uv, patch[3].Uv, uv.x);
    output.Uv = lerp(uv0, uv1, uv.y);

    output.wPosition.y += LodHeightMap.SampleLevel(LinearSampler, output.Uv, 0) * HeightRatio1;
    //z-> lod값 해당 위치의픽셀을 얻어올수있다.
    output.Position = ViewProjection(float4(output.wPosition, 1.0f));
    output.TiledUv = output.Uv * TexScale;

    return output;

}


PS_GBUFFER_OUT PS(DomainOutput_Lod input)
{
    float2 left = input.Uv + float2(-TexelCellSpaceU, 0.0);
    float2 right = input.Uv + float2(+TexelCellSpaceU, 0.0);
    float2 top = input.Uv + float2(0.0f, -TexelCellSpaceV);
    float2 bottom = input.Uv + float2(0.0f, TexelCellSpaceV);

    float leftY = LodHeightMap.SampleLevel(LinearSampler, left, 0).b * HeightRatio1;
    float rightY = LodHeightMap.SampleLevel(LinearSampler, right, 0).b * HeightRatio1;
    float topY = LodHeightMap.SampleLevel(LinearSampler, top, 0).b * HeightRatio1 ;
    float bottomY = LodHeightMap.SampleLevel(LinearSampler, bottom, 0).b * HeightRatio1 ;
    //normal 구하기용

    float3 tangent = normalize(float3(WorldCellSpace * 2, rightY - leftY, 0.0f)); //사각형한칸의크기
    //tangent는 x축에맵핑되고 방향을 왼쪽과 오른쪽의 높이차를 구하고 
    //normal 은 z방향
    float3 biTangent = normalize(float3(0.0f, bottomY - topY, WorldCellSpace * -2));
    float3 normal = cross(tangent, biTangent); //정규직교식;

    //////////////////////////////////////////////////////////////////////////
        
       // Lookup mesh texture and modulate it with diffuse
   
    float2 Uv = input.Uv;
    Uv *= 4;
    float3 DiffuseColor = BaseMap.Sample(LinearSampler, Uv);
    float4 Specular = TerrainSpecMap.Sample(LinearSampler, Uv);
	//DiffuseColor *= DiffuseColor;
   // DiffuseColor += float4(0.15f, 0.15f, 0.15f, 0.0f);
    float3 bump = 0;
    GetBumpMapCoord(Uv, normal, tangent, LinearSampler, bump);
    return PackGBuffer(DiffuseColor, normalize(normal), Specular, bump);
   
}


/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////   CascadedShadow   /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

float4 tempVS(float4 Pos : POSITION) : SV_Position0
{
    return Pos;
}
struct GS_OUTPUT
{
    float4 Pos : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

cbuffer cbuffercbShadowMapCubeGS
{
    float4x4 CascadeViewProj[3];
};

[maxvertexcount(9)]
void CascadedShadowMapsGenGS(triangle MeshOutput InPos[3] : SV_Position, inout TriangleStream<GS_OUTPUT> OutStream)
{
    for (int iFace = 0; iFace < 3; iFace++)
    {
        GS_OUTPUT output;

        output.RTIndex = iFace;

        for (int v = 0; v < 3; v++)
        {
            output.Pos = mul(float4(InPos[v].wPosition, 1.f), CascadeViewProj[iFace]);
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
    CullMode = Front;
    DepthBias =6;
   SlopeScaledDepthBias = -1.0f;
    DepthClipEnable = false;
};
////////////////////////////////////////////////////////////////////////////////////
technique11 T0
{
    /*Shadow*/
    pass P0
    {
        SetRasterizerState(cascadedRS);
        SetVertexShader(CompileShader(vs_5_0, VS_Mesh_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
    pass P1
    {
        SetRasterizerState(cascadedRS);
        SetVertexShader(CompileShader(vs_5_0, VS_Model_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
    pass P2
    {
        SetRasterizerState(cascadedRS);
        SetVertexShader(CompileShader(vs_5_0, VS_Animation_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
    
 
    /*Deferred Packing */
    P_VP(P3, VS_Mesh_PreRender, RenderScenePS)
    P_VP(P4, VS_Model_PreRender, RenderScenePS)
    P_VP(P5, VS_Animation_PreRender, RenderScenePS)
    pass P6
    {
        //SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
         SetPixelShader(CompileShader(ps_5_0, PS()));
    }
   
}