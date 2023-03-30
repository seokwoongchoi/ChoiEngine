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
    uint ActorIndex;
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

float4 BoneIndeces(float4 BlendIndices)
{
    
    float4 indices = BlendIndices;
    indices.x = lerp(BoneIndex,indices.x, any(BlendIndices));
     return indices;
}

float4 BoneWeights(float4 BlendWeights)
{
    
    float4 weights = BlendWeights;
     weights.x = lerp(1.0f, weights.x, any(BlendWeights));
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
    return lerp(curr, next,  Tweenframes[InstID].Curr.Time);
}

matrix GetNextAnimTransforms(uint boneIndices, uint InstID)
{
      
    matrix curr = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Next.CurrFrame, Tweenframes[InstID].Next.Clip);
    matrix next = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Next.NextFrame, Tweenframes[InstID].Next.Clip);
    return lerp(curr, next, (matrix) Tweenframes[InstID].Next.Time);
    
}

matrix LerpCurrAnim_NextAnimTransform(uint boneIndices,  uint InstID)
{
    matrix curr = GetCurrAnimTransforms(boneIndices, InstID);
    matrix next = GetNextAnimTransforms(boneIndices, InstID);
    return lerp(curr, next, Tweenframes[InstID].TweenTime);
}

void SetAnimationWorld(inout matrix world, VertexModel input)
{
   matrix transform = 0;
   matrix Anim = 0;
    matrix nextAnim = 0;
    float4 boneIndices = BoneIndeces(input.BlendIndices);
    float4 boneWeights = BoneWeights(input.BlendWeights);
   
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
