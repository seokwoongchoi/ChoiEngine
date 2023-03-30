#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Terrain.fx"



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

    float3 vMin = float3(input[2].Position.x, minY, input[2].Position.z);//0이하단 2 우상단
    float3 vMax = float3(input[1].Position.x, maxY, input[1].Position.z); //0이하단 2 우상단
    

    float3 boxCenter = (vMin + vMax) * 0.5f;
    float3 boxExtents = (vMax - vMin) * 0.5f;
    //어떻게 짜를지 정함
    ConstantHullOutput_Lod output;
    [flatten]
    if(ContainFrustumCube(boxCenter,boxExtents))
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
    output.TiledUv = output.Uv * 10;

    return output;

}


float4 PS(DomainOutput_Lod input) : SV_Target0
{
    float2 left = input.Uv +float2(-TexelCellSpaceU, 0.0);
    float2 right = input.Uv + float2(+TexelCellSpaceU, 0.0);
    float2 top = input.Uv + float2(0.0f, -TexelCellSpaceV);
    float2 bottom = input.Uv + float2(0.0f, TexelCellSpaceV);

    float leftY = LodHeightMap.SampleLevel(LinearSampler, left, 0).b * HeightRatio1;
    float rightY = LodHeightMap.SampleLevel(LinearSampler, right, 0).b * HeightRatio1;
    float topY = LodHeightMap.SampleLevel(LinearSampler, top, 0).b * HeightRatio1;
    float bottomY = LodHeightMap.SampleLevel(LinearSampler, bottom, 0).b * HeightRatio1;
    //normal 구하기용

    float3 tangent = normalize(float3(WorldCellSpace * 2, rightY - leftY, 0.0f)); //사각형한칸의크기
    //tangent는 x축에맵핑되고 방향을 왼쪽과 오른쪽의 높이차를 구하고 
    //normal 은 z방향
    float3 biTangent = normalize(float3(0.0f, bottomY-topY, WorldCellSpace * -2));
    float3 normal = cross(tangent, biTangent); //정규직교식;

    //////////////////////////////////////////////////////////////////////////
    Texture(Material.Diffuse, BaseMap, input.TiledUv);
    NormalMapping(input.Uv, normal, tangent); //정점에서 넘어온 인자들로 
    //TBN축을만든다.
    Texture(Material.Specular, SpecularMap, input.TiledUv);
    MaterialDesc output = MakeMaterial();
    MaterialDesc result = MakeMaterial();
    ComputeLight(output, normal, input.wPosition);
    AddMaterial(result, output);
    ComputePointLights(output, normal, input.wPosition);
    AddMaterial(result, output);
    ComputeSpotLights(output, normal, input.wPosition);
    AddMaterial(result, output);
    ComputeCapsuleLights(output, normal, input.wPosition);
    AddMaterial(result, output);
    
    return BaseMap.Sample(LinearSampler, input.TiledUv);
   // return float4(MaterialToColor(result), 1.0f);
}

RasterizerState RS
{
    FillMode = wireFrame;
};

technique11 T0
{
    pass P0
    {
        //SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}