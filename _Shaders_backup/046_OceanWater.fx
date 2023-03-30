#include "000_Header.fx"
#include "000_Light.fx"
#include "000_Model.fx"
#include "000_Terrain.fx"


Texture2D HeightMap;

cbuffer CB_Ocean
{
    float WaveFrequency;
    float WaveAmplitude;

    float2 TextureScale;

    float2 BumpSpeed;
    float BumpHeight;
    float HeightRatio; //높이 보정값

    float4 ShallowColor; //얕은색
    float4 DeepColor; //깊은색

    float FresnelBias; //편향
    float FresnelPower; //강도
    float FresnelAmount; //보정값
    float ShoreBlend; //알파값 변화정도

    float2 OceanSize; //버텍스의 전체크기
};
struct VertexOutput
{
    float4 Position : SV_Position0;
    float3 oPosition : Position1;
    float3 wPosition : Position2;
    float2 Uv : Uv0;

    float3x3 Tangent : Tangent0;
    float2 Bump[3] : Bump0;
};

struct Wave
{
    float Frequency;
    float Amplitude;
    float Phase;
    float2 Direction;

};
float EvaluateWave(Wave wave,float2 position)
{
    float s = sin(dot(wave.Direction, position) + wave.Frequency + Time) + wave.Phase;
    return wave.Amplitude * s;
}

float EvaluateWaveDifferent(Wave wave, float2 position)
{
    float s = cos(dot(wave.Direction, position) + wave.Frequency + Time) + wave.Phase;
    return wave.Amplitude *wave.Frequency* s;
}

VertexOutput VS_Ocean(VertexTexture input)
{
    VertexOutput output;
    Wave wave[3] =
    {
        0.0f, 0.0f, 0.50f, float2(-1.0f, 0.0f),
        0.0f, 0.0f, 1.50f, float2(-0.70f, 0.70f),
        0.0f, 0.0f, 0.25f, float2(0.20f, 1.0f)
    };

    wave[0].Frequency = WaveFrequency;
    wave[0].Amplitude = WaveAmplitude;
    wave[1].Frequency = WaveFrequency * 2.0f;
    wave[1].Amplitude = WaveAmplitude * 0.5f;
    wave[2].Frequency = WaveFrequency * 3.0f;
    wave[2].Amplitude = WaveAmplitude * 1.0f;
    
    float ddx = 0, ddy = 0;
    //[unroll(3)]
    for (int i = 0; i < 3; i++)
    {
        input.Position.y = EvaluateWave(wave[i], input.Position.xz);

        float diff = EvaluateWaveDifferent(wave[i], input.Position.xz);
        ddx += diff * wave[i].Direction.x;
        ddy += diff * wave[i].Direction.y;

    }

    float3 T = float3(1, ddx, 0);
    float3 B = float3(-ddx, 1, -ddy);
    float3 N = float3(0, ddy, 1);

    float3x3 matTangent = float3x3
    (
    normalize(T) * BumpHeight, normalize(B) * BumpHeight, normalize(N)
    );
    output.Tangent = mul((float3x3) World, matTangent);
    output.Uv = input.Uv * TextureScale;

    float time = fmod(Time, 100);
    output.Bump[0] = output.Uv + time * BumpSpeed;
    output.Bump[1] = output.Uv + 2.0f*time * BumpSpeed*4.0f;
    output.Bump[2] = output.Uv + 4.0f * time * BumpSpeed*8.0f;
       
    //input.Position.y = EvaluateWave(wave[1], input.Position.xz);
    //input.Position.z = EvaluateWave(wave[2], input.Position.xz);

    output.oPosition = input.Position.xyz;
    output.Position = WorldPosition(input.Position);
    output.wPosition = output.Position.xyz;
    output.Position = ViewProjection(output.Position);
    //output.Uv = input.Uv;


    return output;

 }

bool WithInBound(float3 position)
{
    return (position.x > 0.0f && position.z > 0.0f && position.x < OceanSize.x && position.z < OceanSize.y);
}

float EvaluateAlpha(float3 position)
{
    float2 temp = float2(position.x / OceanSize.x, position.z / OceanSize.y);
    temp.y = 1.0f - temp.y;

    float color = HeightMap.Sample(Sampler, temp).r / HeightRatio;

    return 1.0f - color * ShoreBlend;
}
float4 PS_Ocean(VertexOutput input):SV_Target0
{
    float4 t0 = NormalMap.Sample(LinearSampler, input.Bump[0]) * 2.0f - 1.0f;
    float4 t1 = NormalMap.Sample(LinearSampler, input.Bump[1]) * 2.0f - 1.0f;
    float4 t2 = NormalMap.Sample(LinearSampler, input.Bump[2]) * 2.0f - 1.0f;

    //float3 normal = t0.xyz + t1.xyz + t2.xyz;
    float3 normal = t0.xyz;
     normal = mul(normal, input.Tangent);
    normal = normalize(normal);

    float4 color = 0; // float4(0, 0, 0.6f, 1.0f);
    float3 view = ViewPosition() - input.wPosition;

    float facing = 1.0f - saturate(dot(normalize(view), normal));
    float fresnel = FresnelBias + (1.0f - FresnelBias) * pow(facing, FresnelPower);

    float alpha = 0;
    color = lerp(DeepColor, ShallowColor, facing);
    if(ShoreBlend>0.0f&&WithInBound(input.oPosition))
    {
        alpha = EvaluateAlpha(input.oPosition);
        color.rgb = lerp(2, color, color.a);
    }

    color.rgb *= Material.Ambient.rgb * fresnel;
    
    float3 reflection = normalize(reflect(GlobalLight.Direction, normal));
    float intensity = (dot(reflection, normalize(view)));

    float3 specular = pow(intensity,5);
    Material.Specular.rgb = float3(1, 1, 1);//물의 반사색 조절
    color.rgb = color.rgb + specular * Material.Specular.rgb;
    return float4(color.rgb, alpha);
}
////////////////////////////////////////////////////////////////////////////
float4 PS_Terrain(MeshOutput input) : SV_Target0
{
    
    //Material.Diffuse = GetTerrainColor(input.Uv);
 //////////////////////////////////////////////////////////////////////////////////////////////////////
   
    Texture(Material.Diffuse, DiffuseMap, input.Uv);
    NormalMapping(input.Uv, input.Normal, input.Tangent); //정점에서 넘어온 인자들로 
    //TBN축을만든다.
    Texture(Material.Specular, SpecularMap, input.Uv);
    MaterialDesc output = MakeMaterial();
    MaterialDesc result = MakeMaterial();
    ComputeLight(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
    ComputePointLights(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
    ComputeSpotLights(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
    ComputeCapsuleLights(output, input.Normal, input.wPosition);
    AddMaterial(result, output);
    
    float4 diffuse = 0;
    float4 layer = LayerMap.Sample(LinearSampler, input.Uv);
    float4 layer2 = LayerMap2.Sample(LinearSampler, input.Uv);
    float4 layer3 = LayerMap3.Sample(LinearSampler, input.Uv);
    float4 layer4 = float4(MaterialToColor(result), 1.0f);
  
 
    layer = layer * input.Alpha.r;
    layer2 = lerp(layer, layer2, input.Alpha.g);
    layer3 = lerp(layer2, layer3, input.Alpha.b);
    layer4 = lerp(layer3, layer4, input.Alpha.a);
    
    diffuse = layer4;
  
    
    float3 normal = normalize(input.Normal);
    float NdotL = saturate(dot(normal, -GlobalLight.Direction));
       
   
    return VS_Shadow(input, diffuse * NdotL);
  //  return float4(gridColor, 1);
   //return float4(LightDirection*0.5f+0.5f, 1); //체크용 0~1의 범위로 빛이들어오는지 체크

}
float4 PS(MeshOutput input) : SV_Target0
{
    return VS_Shadow(input,VS_AllLight(input)); //
}
///////////////////////////////////////////////////////////////////////////////


BlendState AlphaBlend
{
    BlendEnable[0] = true;
    DestBlend[0] = INV_SRC_ALPHA;
    SrcBlend[0] = SRC_ALPHA;
    BlendOp[0] = Add;

    DestBlendAlpha[0] = ONE;
    SrcBlendAlpha[0] = ONE;
    RenderTargetWriteMask[0] = 0x0F;
};
RasterizerState RS
{
   // FillMode = Wireframe;
    CullMode = Front;
};

RasterizerState RS_Wire
{
    FillMode = Wireframe;
   
};


technique11 T0
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS_Terrain()));
        SetPixelShader(CompileShader(ps_5_0, PS_Terrain()));
    }
     /* Depth */
    P_RS_VP(P1, RS, VS_Depth_Mesh, PS_Depth)
    P_RS_VP(P2, RS, VS_Depth_Model, PS_Depth)
    P_RS_VP(P3, RS, VS_Depth_Animation, PS_Depth)
  
     
    
    /* Render */
    P_VP(P4, VS_Mesh, PS)
    P_VP(P5, VS_Model, PS)
    P_VP(P6, VS_Animation, PS)

    P_VP(P7, VS_Ocean, PS_Ocean)
    // P_BS_VP(P7, AlphaBlend, VS_Ocean, PS_Ocean)
}