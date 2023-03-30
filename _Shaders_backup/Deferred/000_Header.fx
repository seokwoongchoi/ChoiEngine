cbuffer CB_PerFrame
{
 
   // matrix View;
   
   // matrix Projection;
    matrix VP;
    //float4 g_FrustumNormals[4];

    
};



cbuffer CB_Preview
{
   matrix OrbitVP;
  
};

cbuffer CB_Freedom
{
    matrix FreedomVP;
  
};


cbuffer CB_World
{
    matrix World;
};


Texture2D DiffuseMap;
Texture2D SpecularMap;
Texture2D NormalMap;
Texture2D RoughnessMap;
Texture2D MatallicMap;

TextureCube SkyCubeMap;

Texture2D ShadowMap;
SamplerComparisonState ShadowSampler;


SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = WRAP;
  
};

//SamplerState LinearSampler
//{
//    Filter = COMPARISON_ANISOTROPIC;
//   AddressU = WRAP;
//    AddressV = WRAP;
//   AddressW = WRAP;
//    ComparisonFunc = LESS;
  
//   MaxAnisotropy = 16;
    
//};

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////

float4 WorldPosition(float4 position)
{
    return mul(position, World);
}

float4 ViewProjection(float4 position)
{
    
    return mul(position, VP);
}



float4 FreedomViewProjection(float4 position)
{
    return mul(position, FreedomVP);
}
float4 OrbitViewProjection(float4 position)
{
    return mul(position, OrbitVP);
}

float3 WorldNormal(float3 normal)
{
    return mul(normal, (float3x3) World);
}

float3 WorldTangent(float3 tangent)
{
    return mul(tangent, (float3x3) World);
}


///////////////////////////////////////////////////////////////////////////////


struct VertexMesh
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
  
};

struct MeshOutput
{
    float4 Position : SV_Position0;
    float3 wPosition : Position1;
	float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
  
   
    //float4 Cull : SV_ClipDistance0;
  
   
};



MeshOutput VS_Mesh(VertexMesh input)
{
    MeshOutput output;

   // output.Position = input.Position;
   // output.oPosition = input.Position.xyz;
    output.wPosition = WorldPosition(input.Position).xyz;
    
 //   output.wPosition = output.Position.xyz;
    matrix temp = mul(World, VP);
    output.Position = mul(input.Position,temp);
	//output.wvpPosition = output.Position;
  
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

  
   // output.Alpha = 0;
   
   //output.Cull.x = dot(float4( output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
   // output.Cull.y = dot(float4( output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
   // output.Cull.z = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
   // output.Cull.w = 0;

    return output;
}

MeshOutput VS_PreviewMesh(VertexMesh input)
{
    MeshOutput output;

   // output.Position = input.Position;
    //output.oPosition = input.Position.xyz;
    output.Position = WorldPosition(input.Position);
   // output.wPosition = output.Position.xyz;

    output.Position = OrbitViewProjection(output.Position);
  //  output.wvpPosition = output.Position;
  
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

  

  //  output.Alpha = 0;
   
    
   
    //output.Cull.x = 0;
    //output.Cull.y = 0;
    //output.Cull.z = 0;
    //output.Cull.w = 0;
  
   

    return output;
}

static const float2 arrBasePos[4] =
{
    float2(-1.0, 1.0),
	float2(1.0, 1.0),
	float2(-1.0, -1.0),
	float2(1.0, -1.0),
};
static const float2 arrUV[4] =
{
    float2(0.0, 0.0),
    float2(1.0, 0.0),
	float2(0.0, 1.0),
	float2(1.0, 1.0),
};
struct VS_OUTPUT
{
    float4 Position : SV_Position; // vertex position 
    float2 cpPos : Uv0;
    float2 Uv : Uv1;
   
};

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
    SetDepthStencilState(dss, 1); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}
#define P_DSS_VGP(name, dss, vs,gs, ps) \
pass name \
{ \
    SetDepthStencilState(dss, 0); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
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

#define P_DSS_Ref_BS_VP(name, dss,Ref,bs, vs, ps) \
pass name \
{ \
    SetDepthStencilState(dss, Ref); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

#define P_RS_DSS_Ref_VP(name,rs, dss,Ref, vs, ps) \
pass name \
{ \
    SetRasterizerState(rs);\
    SetDepthStencilState(dss, Ref); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}

//moon,cloud
#define P_DSS_BS_VP(name, dss, bs, vs, ps) \
pass name \
{ \
    SetDepthStencilState(dss, 1); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}
#define P_RS_DSS_BS_VP(name, rs,dss, bs, vs, ps) \
pass name \
{ \
SetRasterizerState(rs);\
    SetDepthStencilState(dss, 0); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}
#define P_DSS_BS_VGP(name, dss, bs, vs,gs, ps) \
pass name \
{ \
    SetDepthStencilState(dss, 0); \
    SetBlendState(bs, float4(0, 0, 0, 0), 0xFF); \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}
#define P_VGP(name, vs, gs,ps) \
pass name \
{ \
    SetVertexShader(CompileShader(vs_5_0, vs())); \
    SetGeometryShader(CompileShader(gs_5_0, gs())); \
    SetPixelShader(CompileShader(ps_5_0, ps())); \
}