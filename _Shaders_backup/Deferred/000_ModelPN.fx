#define MAX_MODEL_TRANSFORMS 450
//#define MAX_MODEL_KEYFRAMES 350
#define MAX_MODEL_INSTANCE 50
//#include "000_Matrix.fx"
Texture2DArray BoneTransforms;
//struct CopyFactor
//{
  
//    float4 Indices;
//    float4 Factor;
//    float4 Direction;
//};
//StructuredBuffer<CopyFactor> CollisonData;

struct VertexModel
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
	float4 BlendIndices : BlendIndices0;
	float4 BlendWeights : BlendWeights0;

 

    matrix Transform : Inst0;
    uint InstID : SV_InstanceID;
};

struct ModelOutput_Lod
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;

};

cbuffer CB_Bone
{
    uint BoneIndex;
    float g_TessellationFactor;
};

///////////////////////////////////////////////////////////////////////////////

void SetModelWorld(inout matrix world, VertexModel input)
{
    float4 m1 = BoneTransforms.Load(int4(BoneIndex * 4 + 0, input.InstID, 0, 0));
    float4 m2 = BoneTransforms.Load(int4(BoneIndex * 4 + 1, input.InstID, 0, 0));
    float4 m3 = BoneTransforms.Load(int4(BoneIndex * 4 + 2, input.InstID, 0, 0));
    float4 m4 = BoneTransforms.Load(int4(BoneIndex * 4 + 3, input.InstID, 0, 0));

     
    matrix transform = matrix(m1 , m2 , m3 , m4);
    //world = input.Transform;
    
  
    world = mul(transform, input.Transform);
}



MeshOutput VS_Model(VertexModel input)
{
    MeshOutput output;

    SetModelWorld(World, input);

  //  output.oPosition = input.Position.xyz;
   output.Position = WorldPosition(input.Position);
   
   output.wPosition = output.Position.xyz;
   

   output.Position = ViewProjection(output.Position);
  
   
	//output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

   
   //output.Cull.x = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
   //output.Cull.y = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
   //output.Cull.z = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
   //output.Cull.w = 0;

    
    return output;
}
MeshOutput VS_PreviewModel(VertexModel input)
{
    MeshOutput output;
    
    float4 m1 = BoneTransforms.Load(int4(BoneIndex * 4 + 0, 0, 0, 0));
    float4 m2 = BoneTransforms.Load(int4(BoneIndex * 4 + 1, 0, 0, 0));
    float4 m3 = BoneTransforms.Load(int4(BoneIndex * 4 + 2, 0, 0, 0));
    float4 m4 = BoneTransforms.Load(int4(BoneIndex * 4 + 3, 0, 0, 0));
  
    
    matrix transform = matrix(m1, m2, m3, m4);
    matrix previewWorld = mul(transform, World);
   
    output.Position = mul(input.Position, previewWorld);
   
    output.wPosition = output.Position.xyz;

    output.Position = OrbitViewProjection(output.Position);
  
   
   
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

 
   
    //output.Cull.x = 0;
    //output.Cull.y = 0;
    //output.Cull.z = 0;
    //output.Cull.w = 0;
    
    return output;
}

///////////////////////////////////////////////////////////////////////////////



struct PreviewFrame
{
    
    int pClip;

    uint pCurrFrame;
    uint pNextFrame;

    float pTime;
    float pRunningTime;

    float3 Padding;
   
};
cbuffer CB_PreviewFrame
{
    PreviewFrame previewFrame;
    matrix previewTransforms[MAX_MODEL_TRANSFORMS];
    
};
struct Keyframe
{
    int Clip;

    uint CurrFrame;
    uint NextFrame;

    float Time;
    float RunningTime;

    float3 Padding;
};

struct TweenFrame
{
    float TakeTime;
    float TweenTime;
    float2 Padding;

    Keyframe Curr;
    Keyframe Next;

   
};
//StructuredBuffer<TweenFrame> Tweenframes;
cbuffer CB_AnimationFrame
{
	TweenFrame Tweenframes[MAX_MODEL_INSTANCE];


};

float4 BoneIndeces(VertexModel input)
{
    
    float4 indices = float4(input.BlendIndices.x, input.BlendIndices.y, input.BlendIndices.z, input.BlendIndices.w);
       
   
    indices.x = lerp(BoneIndex,indices.x, any(input.BlendIndices));
      
    return indices;
}

float4 BoneWeights(VertexModel input)
{
    
    float4 weights = float4(input.BlendWeights.x, input.BlendWeights.y, input.BlendWeights.z, input.BlendWeights.w);
    weights.x = lerp(1.0f, weights.x, any(input.BlendIndices));
      
    return weights;
}
        
matrix LoadAnimTransforms(uint boneIndices, uint frame, uint clip)
{
    float4 c0, c1, c2, c3;
    c0  =BoneTransforms.Load(uint4( boneIndices * 4 + 0, frame, clip, 0));
    c1 = BoneTransforms.Load(uint4( boneIndices * 4 + 1, frame, clip, 0));
    c2 = BoneTransforms.Load(uint4( boneIndices * 4 + 2, frame, clip, 0));
    c3 = BoneTransforms.Load(uint4( boneIndices * 4 + 3, frame, clip, 0));
    
    return matrix(c0, c1, c2, c3);
}

matrix GetCurrAnimTransforms(uint boneIndices, uint InstID)
{
   
    matrix curr = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Curr.CurrFrame, Tweenframes[InstID].Curr.Clip);
    matrix next = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Curr.NextFrame, Tweenframes[InstID].Curr.Clip);
    //float3 currPos, nextPos = 0;
    //float3 currScale, nextScale = 0;
    //float4 currQ, nextQ = 0;
    //decompose(curr, currPos, currQ, currScale);
    //decompose(next, nextPos, nextQ, nextScale);
    
    //float3 resultPos = lerp(currPos, nextPos, (float3) Tweenframes[InstID].Curr.Clip);
    //float3 resultScale = lerp(currScale, nextScale, (float3) Tweenframes[InstID].Curr.Clip);
    //float4 resultQ = q_slerp(currQ, nextQ, Tweenframes[InstID].Curr.Clip);
   
    //return compose(resultPos, resultQ, resultScale);
   return lerp(curr, next,  Tweenframes[InstID].Curr.Time);
}

matrix GetNextAnimTransforms(uint boneIndices, uint InstID)
{
      
    matrix curr = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Next.CurrFrame, Tweenframes[InstID].Next.Clip);
    matrix next = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Next.NextFrame, Tweenframes[InstID].Next.Clip);
   
    //float3 currPos, nextPos = 0;
    //float3 currScale, nextScale = 0;
    //float4 currQ, nextQ = 0;
    //decompose(curr, currPos, currQ, currScale);
    //decompose(next, nextPos, nextQ, nextScale);
    
    //float3 resultPos = lerp(currPos, nextPos, (float3) Tweenframes[InstID].Next.Clip);
    //float3 resultScale = lerp(currScale, nextScale, (float3) Tweenframes[InstID].Next.Clip);
    //float4 resultQ = q_slerp(currQ, nextQ, Tweenframes[InstID].Next.Clip);
   
    //return compose(resultPos, resultQ, resultScale);
    return lerp(curr, next, (matrix) Tweenframes[InstID].Next.Time);
    
}

matrix LerpCurrAnim_NextAnimTransform(uint boneIndices,  uint InstID)
{
   
    
    matrix curr = GetCurrAnimTransforms(boneIndices,  InstID);
    matrix next = GetNextAnimTransforms(boneIndices,  InstID);
    return lerp(curr, next, Tweenframes[InstID].TweenTime);
    
    //float3 currPos, nextPos = 0;
    //float3 currScale, nextScale = 0;
    //float4 currQ, nextQ = 0;
    
    //decompose(curr, currPos, currQ, currScale);
    //decompose(next, nextPos, nextQ, nextScale);
    
    //float3 resultPos = lerp(currPos, nextPos, (float3)Tweenframes[InstID].TweenTime);
    //float3 resultScale = lerp(currScale, nextScale, (float3)Tweenframes[InstID].TweenTime);
    //float4 resultQ = q_slerp(currQ, nextQ, Tweenframes[InstID].TweenTime);
    ////float4 resultQ = UE4_Slerp(currQ, nextQ, Tweenframes[InstID].TweenTime);
    
    //return compose(resultPos, resultQ, resultScale);
   
}

void SetAnimationWorld(inout matrix world, VertexModel input)
{
   matrix transform = 0;
   matrix Anim = 0;
    matrix nextAnim = 0;
    float4 boneIndices = BoneIndeces(input);
   float4 boneWeights = BoneWeights(input);
   uint instId = input.InstID;
    
      
    [unroll(4)]
    for (int i = 0; i < 4; i++)
    {
       
        Anim =
        lerp(
        GetCurrAnimTransforms(boneIndices[i], instId), LerpCurrAnim_NextAnimTransform(boneIndices[i], instId),
        saturate(Tweenframes[instId].Next.Clip + 1)
        );
        
      
        transform += mul(boneWeights[i], Anim);
    }
    //uint drawCount = CollisonData[0].Indices.z;
    
    
    //uint collisonIndex = lerp(ActorIndex,0, saturate(ActorIndex));
    
    //[flatten]
    //if (CollisonData[instId].Factor.z == 1)
    //{
    //    input.Transform._41 -= CollisonData[instId].Direction.x * 0.5f;
    //    input.Transform._42 -= CollisonData[instId].Direction.y * 0.5f;
    //    input.Transform._43 -= CollisonData[instId].Direction.z * 0.5f;
    //}
   
  
   //input.Transform._42 = GetHeight(input.Transform._41, input.Transform._43);
   
   world = mul(transform, input.Transform);
}

ModelOutput_Lod VS_Model_Lod(VertexModel input)
{
    ModelOutput_Lod output;

    SetAnimationWorld(World, input);
    

  
   // output.Position = input.Position;
  
    output.Position = WorldPosition(input.Position);

   // output.wPosition = output.Position.xyz;

  // output.Position = ViewProjection(output.Position);

    //SetModelWorld(World, input);
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    
   
  
    output.Uv = input.Uv;


   
    return output;
}
struct PSIn_TessellatedDiffuse
{
    float4 position : SV_Position0;
    float2 Uv : Uv0;
    //float3 positionWS : TEXCOORD1;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
    float sign : TEXCOROD4;
   
};
struct HSIn_Diffuse
{
    float4 position : Position0;
    float2 texCoord : Uv0;
    float3 normal : Normal0;
    float3 tangent : Tangent0;

};

struct Model_HS_CONSTANT_DATA_OUTPUT
{
    float Edges[3] : SV_TessFactor;
    float Inside : SV_InsideTessFactor;
    float3 f3B210 : POSITION3;
    float3 f3B120 : POSITION4;
    float3 f3B021 : POSITION5;
    float3 f3B012 : POSITION6;
    float3 f3B102 : POSITION7;
    float3 f3B201 : POSITION8;
    float3 f3B111 : CENTER;

    float sign : SIGN;

};
float TessFactorModel(float3 position)
{
    position = float3(position.x, 0.0f, position.z);
    float3 view = float3(ViewPosition().x, 0.0f, ViewPosition().z);

    float d = distance(position, view);

    float factor = saturate((d - 1) / (500 - 1));

    return lerp(3, 1, factor);
};
Model_HS_CONSTANT_DATA_OUTPUT DiffuseConstantHS(InputPatch<ModelOutput_Lod, 3> inputPatch)
{
    Model_HS_CONSTANT_DATA_OUTPUT output;
  
	//// tessellation factors are proportional to model space edge length
   [unroll(3)]
    for (uint ie = 0; ie < 3; ++ie)
    {
        
       // output.Edges[ie] = 1 / (float) 512 * (float) 64;
        float3 edge = inputPatch[(ie + 1) % 3].Position - inputPatch[ie].Position;
       
        float3 vec = (inputPatch[(ie + 1) % 3].Position + inputPatch[ie].Position) / 2 - ViewPosition();
       //float d = distance(edge, vec);
        float len = sqrt(dot(edge, edge) / dot(vec, vec));
       int tess = lerp(0, 1, len *400);
        output.Edges[(ie + 1) % 3] = tess;
        
       // output.Edges[(ie + 1) % 3] = max(1, len * g_TessellationFactor);
    
  

    }
  
    
    
      // culling
    int culled[4];
      [unroll(4)]
    for (int ip = 0; ip < 4; ++ip)
    {
        culled[ip] = 1;
        culled[ip] &= dot(inputPatch[0].Position.xyz - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
        culled[ip] &= dot(inputPatch[1].Position.xyz - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
        culled[ip] &= dot(inputPatch[2].Position.xyz - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
    }
    if (culled[0] || culled[1] || culled[2] || culled[3])
        output.Edges[0] = 0;
    output.f3B210 = ((2.0f * inputPatch[0].Position.xyz) + inputPatch[1].Position.xyz - (dot((inputPatch[1].Position.xyz - inputPatch[0].Position.xyz), inputPatch[0].Normal) * inputPatch[0].Normal)) / 3.0f;
    output.f3B120 = ((2.0f * inputPatch[1].Position.xyz) + inputPatch[0].Position.xyz - (dot((inputPatch[0].Position.xyz - inputPatch[1].Position.xyz), inputPatch[1].Normal) * inputPatch[1].Normal)) / 3.0f;
    output.f3B021 = ((2.0f * inputPatch[1].Position.xyz) + inputPatch[2].Position.xyz - (dot((inputPatch[2].Position.xyz - inputPatch[1].Position.xyz), inputPatch[1].Normal) * inputPatch[1].Normal)) / 3.0f;
    output.f3B012 = ((2.0f * inputPatch[2].Position.xyz) + inputPatch[1].Position.xyz - (dot((inputPatch[1].Position.xyz - inputPatch[2].Position.xyz), inputPatch[2].Normal) * inputPatch[2].Normal)) / 3.0f;
    output.f3B102 = ((2.0f * inputPatch[2].Position.xyz) + inputPatch[0].Position.xyz - (dot((inputPatch[0].Position.xyz - inputPatch[2].Position.xyz), inputPatch[2].Normal) * inputPatch[2].Normal)) / 3.0f;
    output.f3B201 = ((2.0f * inputPatch[0].Position.xyz) + inputPatch[2].Position.xyz - (dot((inputPatch[2].Position.xyz - inputPatch[0].Position.xyz), inputPatch[0].Normal) * inputPatch[0].Normal)) / 3.0f;
    // center control point
    float3 f3E = (output.f3B210 + output.f3B120 + output.f3B021 + output.f3B012 + output.f3B102 + output.f3B201) / 6.0f;
    float3 f3V = (inputPatch[0].Position + inputPatch[1].Position + inputPatch[2].Position) / 3.0f;
    output.f3B111 = f3E + ((f3E - f3V) / 2.0f);
  //  output.Inside = 0;
    output.Inside = (output.Edges[0] + output.Edges[1] + output.Edges[2]) / 3;

    
    float2 t01 = inputPatch[1].Uv - inputPatch[0].Uv;
    float2 t02 = inputPatch[2].Uv - inputPatch[0].Uv;
    output.sign = t01.x * t02.y - t01.y * t02.x > 0.0f ? 1 : -1;
    return output;
}
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("DiffuseConstantHS")]
[maxtessfactor(64.0)]
HSIn_Diffuse DiffuseHS(InputPatch<ModelOutput_Lod, 3> inputPatch, uint i : SV_OutputControlPointID)
{
    //HSIn_Diffuse output;
    //output.position = inputPatch[i].Position;
    //output.normal = inputPatch[i].Normal;
    //output.tangent = inputPatch[i].Tangent;
    //output.texCoord = inputPatch[i].Uv;
    return inputPatch[i];
}
[domain("tri")]
PSIn_TessellatedDiffuse DiffuseDS(Model_HS_CONSTANT_DATA_OUTPUT input,
                       float3 barycentricCoords : SV_DomainLocation,
                       OutputPatch<ModelOutput_Lod, 3> inputPatch)
{
    PSIn_TessellatedDiffuse output;

    float3 coordinates = barycentricCoords;

    // The barycentric coordinates
    float fU = barycentricCoords.x;
    float fV = barycentricCoords.y;
    float fW = barycentricCoords.z;

    // Precompute squares and squares * 3 
    float fUU = fU * fU;
    float fVV = fV * fV;
    float fWW = fW * fW;
    float fUU3 = fUU * 3.0f;
    float fVV3 = fVV * 3.0f;
    float fWW3 = fWW * 3.0f;

    // Compute position from cubic control points and barycentric coords
    float3 position =
    inputPatch[0].Position.xyz * fWW * fW +
    inputPatch[1].Position.xyz * fUU * fU +
    inputPatch[2].Position.xyz * fVV * fV
    + input.f3B210 * fWW3 * fU + input.f3B120 * fW * fUU3 + input.f3B201 * fWW3 * fV + input.f3B021 * fUU3 * fV +
					  input.f3B102 * fW * fVV3 + input.f3B012 * fU * fVV3 + input.f3B111 * 6.0f * fW * fU * fV;
					 
    // Compute normal from quadratic control points and barycentric coords
    float3 normal = inputPatch[0].Normal * coordinates.z + inputPatch[1].Normal * coordinates.x + inputPatch[2].Normal * coordinates.y;
    normal = normalize(normal);

    float2 texCoord = inputPatch[0].Uv * coordinates.z + inputPatch[1].Uv * coordinates.x + inputPatch[2].Uv * coordinates.y;

    //float2 displacementTexCoord = texCoord;


    float3 tangent = inputPatch[0].Tangent * coordinates.z + inputPatch[1].Tangent * coordinates.x + inputPatch[2].Tangent * coordinates.y;
    tangent = normalize(tangent);

    output.position = float4(position, 1.0f);
    //SetModelWorld(World, input);
    //output.position = WorldPosition(float4(position, 1.0f));
    output.position = ViewProjection(output.position);
    output.Uv = texCoord;
    //output.position = mul(float4(position, 1.0f), g_ModelViewProjectionMatrix);
  //  output.positionWS = position;
    output.Normal = normal;
    output.Tangent = tangent;
  
    output.sign = input.sign; //inputPatch[0].sign;
   // output.sign = input.sign; //inputPatch[0].sign;

    return output;
}
MeshOutput VS_Animation(VertexModel input)
{
	MeshOutput output; 

	SetAnimationWorld(World, input);
  
 
   
	output.Position = WorldPosition(input.Position);
    output.wPosition = output.Position.xyz;
   

	output.Position = ViewProjection(output.Position);

	output.Normal = WorldNormal(input.Normal);
	output.Tangent = WorldTangent(input.Tangent);
	output.Uv = input.Uv;

   
   //output.Cull.x = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
   //output.Cull.y = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
   //output.Cull.z = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
   //output.Cull.w = 0;
  
    
  
  
    
	return output;
}





MeshOutput VS_PreviewAnimation(VertexModel input)
{
    MeshOutput output;

   // matrix transform = 0;
   // SetPreviewAnimationWorld(transform, input);
   // matrix previewWorld = mul(transform, World);
    
    float boneIndices[4] = { input.BlendIndices.x, input.BlendIndices.y, input.BlendIndices.z, input.BlendIndices.w };
    float boneWeights[4] = { input.BlendWeights.x, input.BlendWeights.y, input.BlendWeights.z, input.BlendWeights.w };

    [flatten]
    if (any(input.BlendIndices) == false)
    {
        boneIndices[0] = BoneIndex;
        boneWeights[0] = 1.0f;
    }
  
    matrix Transform = 0;
     [unroll(4)]
    for (int i = 0; i < 4; i++)
       Transform += mul(boneWeights[i], previewTransforms[(uint) boneIndices[i]]);
    
  
       
    
   
   
    matrix previewWorld = mul(Transform, World);

   
    output.Position = mul(input.Position, previewWorld);
    output.wPosition = output.Position.xyz;

    output.Position = OrbitViewProjection(output.Position);
  
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

 
    //output.Cull.x = 0;
    //output.Cull.y = 0;
    //output.Cull.z = 0;
    //output.Cull.w = 0;
    
    return output;
}

cbuffer CB_PreviewForward
{
    matrix forwardTransform;
    
};
MeshOutput VS_PreviewForward(VertexModel input)
{
    MeshOutput output;
    

    
    matrix previewWorld = mul(forwardTransform, World);

   
    output.Position = mul(input.Position, previewWorld);
    output.wPosition = output.Position.xyz;

    output.Position = OrbitViewProjection(output.Position);
    
    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

   
    
   //output.Cull.x = 0;
   //output.Cull.y = 0;
   //output.Cull.z = 0;
   //output.Cull.w = 0;
    
    return output;
}

MeshOutput VS_Forward(VertexModel input)
{
    MeshOutput output;
    

    
    matrix previewWorld = mul(forwardTransform, input.Transform);

   
    output.Position = mul(input.Position, previewWorld);
    output.wPosition = output.Position.xyz;
    

    output.Position = ViewProjection(output.Position);
   

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

    //output.Cull.x = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
    //output.Cull.y = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
    //output.Cull.z = dot(float4(output.wPosition.xyz - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
    //output.Cull.w = 0;
  
    return output;
}
