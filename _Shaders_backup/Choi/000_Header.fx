cbuffer CB_PerFrame
{
    matrix View;
    matrix ViewInverse;
    matrix Projection;
    matrix VP;

    float Time;
};

cbuffer CB_World
{
    matrix World;
};



Texture2D DiffuseMap;
Texture2D SpecularMap;
Texture2D NormalMap;

TextureCube SkyCubeMap;

Texture2D ShadowMap; //DSV�� ���� SRV�� ���� ��������

SamplerComparisonState ShadowSampler;



SamplerState Sampler;
SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

///////////////////////////////////////////////////////////////////

struct Vertex
{
    float4 Position : POSITION0;
};

struct VertexNormal
{
    float4 Position : POSITION0;
    float3 Normal : NORMAL0;
};

struct VertexColor
{
    float4 Position : POSITION0;
    float4 Color : COLOR0;
};

struct VertexColorNormal
{
    float4 Position : POSITION0;
    float4 Color : COLOR0;
    float3 Normal : NORMAL0;
};

struct VertexTexture
{
    float4 Position : POSITION0;
    float2 Uv : TEXCOORD0;
};

struct VertexTextureColor
{
    float4 Position : POSITION0;
    float2 Uv : TEXCOORD0;
    float4 Color : COLOR0;
};

struct VertexTextureColorNormal
{
    float4 Position : POSITION0;
    float2 Uv : TEXCOORD0;
    float4 Color : COLOR0;
    float3 Normal : NORMAL0;
  
};

struct VertexTextureNormal
{
    float4 Position : POSITION0;
    float2 Uv : TEXCOORD0;
    float3 Normal : NORMAL0;
};

struct VertexColorTextureNormal
{
    float4 Position : POSITION0;
    float4 Color : COLOR0;
    float2 Uv : TEXCOORD0;
    float3 Normal : NORMAL0;
};

struct VertexTextureNormalBlend
{
    float4 Position : POSITION0;
    float2 Uv : TEXCOORD0;
    float3 Normal : NORMAL0;
    float4 BlendIndices : BLENDINDICES0;
    float4 BlendWeights : BLENDWEIGHTS0;
};

struct VertexTextureNormalTangent
{
    float4 Position : POSITION0;
    float2 Uv : TEXCOORD0;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
};

struct VertexTextureAlphaNormalTangent
{
    float4 Position : POSITION0;
    float2 Uv : TEXCOORD0;
    float4 Alpha : ALPHA0;
    float3 Normal : NORMAL0;
    float3 Tangent : TANGENT0;
};

struct VertexTextureNormalTangentBlend
{
    float4 Position : POSITION0;
    float2 Uv : TEXCOORD0;
    float3 Normal : NORMAL0;
    float4 BlendIndices : BLENDINDICES0;
    float4 BlendWeights : BLENDWEIGHTS0;
};

////////////////////////////////////////////////////////////////////////

float4 WorldPosition(float4 position)
{
    return mul(position, World);
}
float4 ViewProjection(float4 position)
{
  
    return mul(position, VP);

}
float3 WorldNormal(float3 normal)
{
    return mul(normal, (float3x3) World);
}
float3 WorldTangent(float3 tangent)
{
    return mul(tangent, (float3x3) World);
}
float3 ViewPosition()
{
    return ViewInverse._41_42_43;
}

void Texture(inout float4 color,Texture2D t,float2 uv,SamplerState samp)
{
    color = color * t.Sample(samp, uv);
}

void Texture(inout float4 color, Texture2D t, float2 uv)
{
    Texture(color, t, uv, LinearSampler);
}
///////////////////////////////////////////////////////////////////////////////
cbuffer CB_Shadow
{
    matrix ShadowView;
    matrix ShadowProjection;
    
    float2 ShadowMapSize;
    float ShadowBias;

    uint ShadowQuality;
};
struct VertexMesh
{
    float4 Position :Position0;
    float2 Uv : Uv0;
    float3 Normal :Normal0;
    float3 Tangent : Tangent0;
  
};

struct MeshOutput
{
    float4 Position : SV_Position0;
    float4 wvpPosition : Position1;
    float3 oPosition : Position2;
    float3 wPosition : Position3;

    float4 sPosition : Position4;
   
 
    float2 Uv : Uv1;
    float3 Normal : Normal0;
   // int ID : Id0;
    float3 Tangent : Tangent0;

    float4 Clip : SV_ClipDistance;
  
};


MeshOutput VS_Mesh(VertexMesh input)
{
    MeshOutput output;

    output.oPosition = input.Position.xyz;
    output.Position = WorldPosition(input.Position);
    output.wPosition = output.Position.xyz;

    output.Position = ViewProjection(output.Position); //sv_position�� �����Ͷ���¡�� ������(ȭ�鿡 ����������)
    output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

    output.sPosition = WorldPosition(input.Position);
    output.sPosition = mul(output.sPosition, ShadowView);
    output.sPosition = mul(output.sPosition, ShadowProjection);

    output.Clip = 0;
    
   
    return output;
}

//struct GeometryOutput1
//{
  
    
//    float4 Position : SV_Position0;
//    float4 wvpPosition : Position1;
//    float3 oPosition : Position2;
//    float3 wPosition : Position3;

//    float4 sPosition : Position4;
//    float4 sPosition2 : Position5;
//    float4 sPosition3 : Position6;
 
//    float2 Uv : Uv1;
//    float3 Normal : Normal0;
//    int ID : Id0;
//    float3 Tangent : Tangent0;
   
//};

//[maxvertexcount(9)]
//void GS_Depth(triangle MeshOutput input[3], inout TriangleStream<GeometryOutput1> stream)//���� �ϳ��� �޾Ƽ� gs���� ���� 4���θ��� ���� �����.
//{
    
//    GeometryOutput1 output[3];
//    for (uint k = 0; k < 3;k++)
//    {
//        output[k].oPosition = input[k].Position.xyz;
//        output[k].Position = WorldPosition(input[k].Position);
//        output[k].wPosition = output[k].Position.xyz;
              
//        output[k].Position = ViewProjection(output[k].Position); //sv_position�� �����Ͷ���¡�� ������(ȭ�鿡 ����������)
//        output[k].wvpPosition = output[k].Position;
             
//        output[k].Normal = WorldNormal(input[k].Normal);
//        output[k].Tangent = WorldTangent(input[k].Tangent);
//        output[k].Uv = input[k].Uv;

       
//    }
       
       
//        for (uint j = 0; j < 3; j++)
//        {
//            output[j].sPosition = mul(input[j].sPosition, ShadowView[0]);
//        output[j].sPosition = mul(output[j].sPosition, ShadowProjection[0]);

//          output [j].sPosition2 = mul(input[j].sPosition2, ShadowView[1]);
//        output[j].sPosition2 = mul(output[j].sPosition2, ShadowProjection[1]);

//        output[j].sPosition3 = mul(input[j].sPosition3, ShadowView[2]);
//        output[j].sPosition3 = mul(output[j].sPosition3, ShadowProjection[2]);
              
//        stream.Append(output[j]);

//        }
//        stream.RestartStrip();

  

//}
/////////////////////////////////////////////////////////////////////////////


MeshOutput VS_Mesh_GS(VertexMesh input)
{
    MeshOutput output;

    output.oPosition = input.Position.xyz;
    output.Position = WorldPosition(input.Position);
    output.wPosition = output.Position.xyz;

    output.Position = ViewProjection(output.Position); //sv_position�� �����Ͷ���¡�� ������(ȭ�鿡 ����������)
    output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

    output.sPosition = WorldPosition(input.Position);
    output.sPosition = mul(output.sPosition, ShadowView);
    output.sPosition = mul(output.sPosition, ShadowProjection);

  
    output.Clip = 0;
    
   
    return output;
}

struct GeometryOutput
{
    float4 Position : SV_Position0;
    float4 wvpPosition : Position1;
    float3 oPosition : Position2;
    float3 wPosition : Position3;

    float4 sPosition : Position4;
   
 
    float2 Uv : Uv1;
    float3 Normal : Normal0;
   // int ID : Id0;
    float3 Tangent : Tangent0;
    uint TargetIndex : SV_RenderTargetArrayIndex;
};

/////////////////////////  Depth //////////////////////////////////////////////

struct DepthOutput
{
 
    float4 Position : SV_Position0;
    float4 sPosition : Position1;
    
  
};
DepthOutput VS_Depth_Mesh(VertexMesh input)
{
    DepthOutput output;
   //  output.Position = input.Position;
    output.Position = WorldPosition(input.Position);
    output.Position = mul(output.Position, ShadowView);
    output.Position = mul(output.Position, ShadowProjection);

    output.sPosition = output.Position;
   
    return output;
}



float4 PS_Depth(DepthOutput input) : SV_Target0
{
   
    float depth1 = input.sPosition.z / input.sPosition.w;
   
    return float4(depth1, depth1, depth1, 1);

}
///////////////////////////////////////////////////////////////////////////////


#define P_VP(name, vs, ps) \
pass name \
{ \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_VP(name, rs, vs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_BS_VP(name, bs, vs, ps) \
pass name \
{ \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}
#define P_DSS_VP(name, dss, vs, ps) \
pass name \
{ \
    SetDepthStencilState(dss, 0); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}
#define P_RS_DSS_VP(name, rs, dss, vs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetDepthStencilState(dss, 0); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_BS_VP(name, rs, bs, vs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}
#define P_VGP(name, vs, gs, ps) \
pass name \
{ \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_VGP(name, rs, vs, gs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_DSS_VGP(name, rs, dss, vs, gs, ps) \
pass name \
{ \
    SetRasterizerState(rs); \
    SetDepthStencilState(dss, 0); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}
