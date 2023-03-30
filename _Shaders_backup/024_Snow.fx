#include "000_Header.fx"
#include "000_Light.fx"


cbuffer CB_Snow
{
    float4 Color ;
    float3 Velocity ;
    float DrawDistance ;
		
    float3 Origin;
    float Turbulence;
		
    float3 Extent ;
   
};
struct VertexInput
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float Scale : Scale0;
    float2 Random : Random0;
};
struct VertexOutput
{
    float4 Position : SV_Position0;

    float2 Uv : Uv0;
    float Alpha : Alpha0;
};
VertexOutput VS(VertexInput input)
{
   
   VertexOutput output;
   
    float3 displace = Time * Velocity; //
    input.Position.y = Origin.y + Extent.y - (input.Position.y - displace.y) % Extent.y;
    
    input.Position.x += cos(Time - input.Position.x) + Turbulence;
    input.Position.z += cos(Time - input.Position.y) + Turbulence;

    input.Position.xyz = Origin + (Extent + (input.Position.xyz + displace) % Extent) % Extent - (Extent * 0.5f);
    //무조건 extent 범위안에 들어오게 된다. 비가 위에서 밑으로내려가서 범위가 넘어가면 다시 위로올려서 내리게한다.
    float4 position = WorldPosition(input.Position);

    float3 up = normalize(-Velocity);
    float3 forward = position.xyz - ViewPosition();
    float3 right = normalize(cross(up, forward));

    position.xyz += (input.Uv.x - 0.5f) * right * input.Scale;
    position.xyz += (1.5f - input.Uv.y * 1.5f) * up * input.Scale;
    position.w = 1.0f;
    output.Position = ViewProjection(position);
    output.Uv = input.Uv;
    float alpha = cos(Time * (input.Position.x + input.Position.z));
    alpha = saturate(1.5f+alpha / DrawDistance * 2.0f);
    output.Alpha = 0.5f * saturate(1 - output.Position.z / DrawDistance) * alpha;
    return output;
    
}

float4 PS(VertexOutput input) : SV_Target0
{
    float4 color = DiffuseMap.Sample(LinearSampler, input.Uv);
    color.rgb = Color.rgb * (1 + input.Alpha) * 2.0f;
    color.a = color.a * (input.Alpha * 1.5f);

    return color;
    //return float4(color.a, 0, 0, 1);
}



BlendState AlphaBlend
{
    BlendEnable[0] = true;
    DestBlend[0] = INV_SRC_ALPHA;
    SrcBlend[0] = SRC_ALPHA;
    BlendOp[0] = Add;

    SrcBlendAlpha[0] = One;
    DestBlendAlpha[0] = One;
    RenderTargetWriteMask[0] = 0x0F;
};

technique11 T0
{
   
  P_BS_VP(P0, AlphaBlend, VS, PS)
}