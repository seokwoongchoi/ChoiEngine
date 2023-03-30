#define MAX_POINT_LIGHT 32
#define MAX_SPOT_LIGHT 32
#define MAX_CAPSULE_LIGHT 32
struct MaterialDesc
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
};

cbuffer CB_Material
{
    MaterialDesc Material;
};

struct LightDesc
{
    float4 Ambient;
    float4 Specular;
    float3 Direction;
    float padding;
    float3 Positon;

};

cbuffer CB_Light
{
    LightDesc GlobalLight;
};
MaterialDesc MakeMaterial()
{
  MaterialDesc output;
  output.Ambient=float4(0,0,0,1);
    output.Diffuse = float4(0, 0, 0, 1);
    output.Specular = float4(0, 0, 0, 1);
  return output;
}


void AddMeterial(inout MaterialDesc result, MaterialDesc val)
{
    result.Ambient += val.Ambient;
    result.Diffuse += val.Diffuse;
    result.Specular += val.Specular;
}

float3 MaterialToColor(MaterialDesc result)
{
    return (result.Ambient + result.Diffuse + result.Specular).rgb;
}
void ComputeLight(inout MaterialDesc output,float3 normal, float3 wPosition)
{
    //float NdotL = normalize(dot(-GlobalLight.Direction, normal));
    //color *= NdotL;

    output.Ambient = 0;
    output.Diffuse = 0;
    output.Specular = 0;

    float3 direction = -GlobalLight.Direction;
    float  NdotL = dot(direction, normalize(normal));
    
    output.Ambient = GlobalLight.Ambient * Material.Ambient;

    [flatten]
    if(NdotL>0.0f)
    {
        output.Diffuse = NdotL * Material.Diffuse;
         [flatten]
        if(any(Material.Specular.rgba))
        {
            wPosition = ViewPosition() - wPosition;
            float3 R = normalize(reflect(-direction, normal));//다시뒤집어줘야 들어오는 방향
            float RdotE = saturate(dot(R, normalize(wPosition)));

            float shininess = pow(RdotE, Material.Specular.a);
           
            output.Specular = shininess * Material.Specular * GlobalLight.Specular;
            
            
        }

    }
   


}

int Selected;

void NormalMapping(float2 uv,float3 normal, float3 tangent, SamplerState samp)
{
    float4 map = NormalMap.Sample(samp, uv);
    [flatten]
    if(any(map) == false)return;
    //탄젠트 공간
    float3 N = normalize(normal);  //z
    float3 T = normalize(tangent - dot(tangent,N)*N);//정규직교한다. 내각에다가 노멀만큼곱해서 연장한다음에
    //탄젠트에서 뺀다.
    float3 B = cross(N, T);
    float3x3 TBN =float3x3(T, B, N);
    
    float3 coord = map.rgb * 2.0f - 1.0f;//이미지로부터 노멀벡터 가져오기
    coord = mul(coord, TBN);//이미지를 tbn공간으로 변환
    float NdotL = saturate(dot(coord, -GlobalLight.Direction));
   
    //if (Selected == 1)
   //  Material.Diffuse = NdotL;
    //if (Selected == 2)
    // Material.Diffuse *= NdotL;
    
   //float4(coord * 0.5f + 0.5f, 1);
    

}
void NormalMapping(float2 uv, float3 normal, float3 tangent)
{
    NormalMapping(uv, normal, tangent, LinearSampler);
}

///////////////////////////////////////////////////////////////////////////////

struct PointLightDesc
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;

    float3 Position;
    float Range;

    float intensity;
    float3 Padding1;

 
    
};

cbuffer CB_PointLights
{
    uint PointLightCount;
    float3 CB_PointLights_Padding;
    
    PointLightDesc PointLights[MAX_POINT_LIGHT];

}
void ComputePointLights(inout MaterialDesc output,float3 normal, float3 wPosition)
{
    output = MakeMaterial();
    MaterialDesc result = MakeMaterial();
   // [roll(MAX_POINT_LIGHT)]
    for (int i = 0; i < PointLightCount; i++)
    {
        float3 light = PointLights[i].Position - wPosition;
        float dist = length(light);
        
        [flatten]
        if (dist > PointLights[i].Range)
            continue;

        light /= dist; //정규화
        
        result.Ambient = Material.Ambient * PointLights[i].Ambient;

        float NDotL = dot(light, normalize(normal));

        [flatten]
        if (NDotL > 0.0f)
        {
            wPosition = ViewPosition() - wPosition;

            float3 R = normalize(reflect(-light, normal));
            float RdotE = saturate(dot(R, normalize(wPosition)));
            float shininess = pow(RdotE, Material.Specular.a);
            result.Diffuse = NDotL * Material.Diffuse * PointLights[i].Diffuse;
            result.Specular = shininess * Material.Specular * PointLights[i].Specular;

        }
        //Light가 얼마나섞일지 감쇄
       // float att = 1.0f / PointLights[i].intensity;
        float temp = 1.0f - saturate(dist / PointLights[i].Range);
        float att = temp * temp * (1.0f / PointLights[i].intensity);
        output.Ambient += result.Ambient;
        output.Diffuse += result.Diffuse * att;
        output.Specular += result.Specular * att;
       
        
    }
    
}
/////////////////////////////////////////////////////////////////////////////

struct SpotLightDesc
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;

    float3 Position;
    float Range;

    float3 Direction;
    float Angle;

    float intensity;
    float3 Padding;

   
    
};

cbuffer CB_SpotLights
{
    uint SpotLightCount;
    float3 CB_SpotLights_Padding;
    
    SpotLightDesc SpotLights[MAX_SPOT_LIGHT];

}
void ComputeSpotLights(inout MaterialDesc output, float3 normal, float3 wPosition)
{
    output = MakeMaterial();
     MaterialDesc result = MakeMaterial();
  //  [roll(MAX_SPOT_LIGHT)]
    for (int i = 0; i < SpotLightCount; i++)
    {
        float3 light = SpotLights[i].Position - wPosition;
        float dist = length(light);
        
        [flatten]
        if (dist > SpotLights[i].Range)
            continue;

        light /= dist; //정규화
        
        result.Ambient = Material.Ambient * SpotLights[i].Ambient;

        float NDotL = dot(light, normalize(normal));

        [flatten]
        if (NDotL > 0.0f)
        {
            wPosition = ViewPosition() - wPosition;

            float3 R = normalize(reflect(-light, normal));
            float RdotE = saturate(dot(R, normalize(wPosition)));
            float shininess = pow(RdotE, Material.Specular.a);
            result.Diffuse = NDotL * Material.Diffuse * SpotLights[i].Diffuse;
            result.Specular = shininess * Material.Specular * SpotLights[i].Specular;

        }
        //Light가 얼마나섞일지 감쇄
        float spot = pow(saturate(dot(-light, SpotLights[i].Direction)), SpotLights[i].Angle);
        float att = spot * (1.0f / SpotLights[i].intensity);

        output.Ambient += result.Ambient*spot; //환경광이 더강해지게 곱해준다.
        output.Diffuse += result.Diffuse * att;
        output.Specular += result.Specular * att;
       
        
    }
    
}
/////////////////////////////////////////////////////////////////////////////

struct CapsuleLightDesc
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;

    float3 Position;
    float Range;

    float3 Direction;
    float Length;

    float Intensity;
    float3 Padding;

   
    
};

cbuffer CB_CapsuleLights
{
    uint CapsuleLightCount;
    float3 CB_CapsuleLights_Padding;
    
    CapsuleLightDesc CapsuleLights[MAX_CAPSULE_LIGHT];

}
void ComputeCapsuleLights(inout MaterialDesc output, float3 normal, float3 wPosition)
{
    output = MakeMaterial();
    MaterialDesc result = MakeMaterial();
    //[roll(MAX_CAPSULE_LIGHT)]
    for (int i = 0; i < CapsuleLightCount; i++)
    {
        float3 start = wPosition-CapsuleLights[i].Position ;
        float3 distOnLine = dot(start, CapsuleLights[i].Direction)/CapsuleLights[i].Length;
        distOnLine = saturate(distOnLine) * CapsuleLights[i].Length;

      
        float3 pointOnLine = CapsuleLights[i].Position + CapsuleLights[i].Direction* distOnLine;
        float3 light = pointOnLine - wPosition;
        
        float dist = length(light);
        light /= dist; //정규화

        [flatten]
        if (dist > CapsuleLights[i].Range)
            continue;

      
        
        result.Ambient = Material.Ambient * CapsuleLights[i].Ambient;

        float NDotL = dot(light, normalize(normal));

        [flatten]
        if (NDotL > 0.0f)
        {
            wPosition = ViewPosition() - wPosition;

            float3 R = normalize(reflect(-light, normal));
            float RdotE = saturate(dot(R, normalize(wPosition)));
            float shininess = pow(RdotE, Material.Specular.a);
            result.Diffuse = NDotL * Material.Diffuse * CapsuleLights[i].Diffuse;
            result.Specular = shininess * Material.Specular * CapsuleLights[i].Specular;

        }
        //Light가 얼마나섞일지 감쇄
        float temp = 1.0f - saturate(dist / CapsuleLights[i].Range);//길이 계산
        //float att = temp * temp/CapsuleLights[i].intensity;
        float att = temp * temp * (1.0f / CapsuleLights[i].Intensity);
        output.Ambient += result.Ambient ; //환경광이 더강해지게 곱해준다.
        output.Diffuse += result.Diffuse * att;
        output.Specular += result.Specular * att;
       
       

    }
    
}
///////////////////////////////////////////////////////////////////////////////////

float4 VS_AllLight(MeshOutput input)
{
    Texture(Material.Diffuse, DiffuseMap, input.Uv);
    NormalMapping(input.Uv, input.Normal, input.Tangent); //정점에서 넘어온 인자들로 
    //TBN축을만든다.
    
    Texture(Material.Specular, SpecularMap, input.Uv);
    MaterialDesc output = MakeMaterial();

    MaterialDesc result = MakeMaterial();

    ComputeLight(output, input.Normal, input.wPosition);
    AddMeterial(result, output);

    ComputePointLights(output, input.Normal, input.wPosition);
    AddMeterial(result, output);

    ComputeSpotLights(output, input.Normal, input.wPosition);
    AddMeterial(result, output);

    ComputeCapsuleLights(output, input.Normal, input.wPosition);
    AddMeterial(result, output);

 
   return float4(MaterialToColor(result), 1.0f);
}

float4 VS_AllLight(GeometryOutput input)
{
    MeshOutput output;
    output.Position = input.Position;
    output.wvpPosition = input.wvpPosition;
    output.oPosition = input.oPosition;
    output.wPosition = input.wPosition;

    output.sPosition = input.sPosition;
   

    output.Uv = input.Uv;
    output.Normal = input.Normal;
    output.Tangent = input.Tangent;
    
    return VS_AllLight(output);

}

float4 VS_Shadow(MeshOutput input, float4 color)
{
    input.sPosition.xyz /= input.sPosition.w;

    [flatten]
    if (input.sPosition.x < -1.0f || input.sPosition.x > 1.0f ||
        input.sPosition.y < -1.0f || input.sPosition.y > 1.0f ||
        input.sPosition.z < 0.0f || input.sPosition.z > 1.0f)
        return color;

    input.sPosition.x = input.sPosition.x * 0.5f + 0.5f;
    input.sPosition.y = -input.sPosition.y * 0.5f + 0.5f;
    input.sPosition.z -= ShadowBias;

    float depth = 0;
    float factor = 0;

    float2 size = 1.0f / ShadowMapSize;
    float2 offsets[] =
    {
        float2(+size.x, -size.y), float2(0.0f, -size.y), float2(-size.x, -size.y),
        float2(+size.x, 0.0f), float2(0.0f, 0.0f), float2(-size.x, 0.0f),
        float2(+size.x, +size.y), float2(0.0f, +size.y), float2(-size.x, +size.y),
    };

    float sum = 0.0f;
    float2 uv = 0.0f;
        
    depth = input.sPosition.z;

    
    //[unroll(9)]
    for (int i = 0; i < 9; i++)
    {
        uv = input.sPosition.xy + offsets[i];
        
        sum += ShadowMap.SampleCmpLevelZero(ShadowSampler, uv, depth).r;
    }

    factor = sum / 9.0f;
    factor = saturate(factor + depth);

    return float4(color.rgb * factor, 1);
}

float4 VS_Shadow(GeometryOutput input,float4 color)
{
    MeshOutput output;
    output.Position = input.Position;
    output.wvpPosition = input.wvpPosition;
    output.oPosition = input.oPosition;
    output.wPosition = input.wPosition;

    output.sPosition = input.sPosition;
  
    output.Uv = input.Uv;
    output.Normal = input.Normal;
    output.Tangent = input.Tangent;
    
   return VS_Shadow(output,color);

}
