#include "Header.hlsl"
Texture2DArray<float4> AnimBoneTransforms : register(t1);
struct VertexSkeletal
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
	float4 BlendIndices : BlendIndices0;
	float4 BlendWeights : BlendWeights0;

 
    uint InstID : SV_InstanceID;
};

///////////////////////////////////////////////////////////////////////////////
struct Keyframe
{
    int Clip;
    uint CurrFrame;
    uint NextFrame;
    float Time;
    float4 pad1;
};

struct TweenFrame
{
    float TakeTime ;
    float TweenTime;
    float2 Padding;

    Keyframe Curr ;
    Keyframe Next ;

   
};
cbuffer CB_AnimationFrame : register(b3)
{
    TweenFrame Tweenframes[MAX_MODEL_INSTANCE];

};

float4 BoneIndeces(float4 BlendIndices)
{
    
    float4 indices = BlendIndices;
    indices.x = lerp(BoneIndex, indices.x, any(BlendIndices));
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
    c0 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 0, frame, clip, 0));
    c1 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 1, frame, clip, 0));
    c2 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 2, frame, clip, 0));
    c3 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 3, frame, clip, 0));
    
    return matrix(c0, c1, c2, c3);
}

matrix GetCurrAnimTransforms(uint boneIndices, uint InstID)
{
   
    matrix curr = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Curr.CurrFrame, Tweenframes[InstID].Curr.Clip);
    matrix next = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Curr.NextFrame, Tweenframes[InstID].Curr.Clip);
    return lerp(curr, next, Tweenframes[InstID].Curr.Time);
}

matrix GetNextAnimTransforms(uint boneIndices, uint InstID)
{
      
    matrix curr = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Next.CurrFrame, Tweenframes[InstID].Next.Clip);
    matrix next = LoadAnimTransforms(boneIndices, Tweenframes[InstID].Next.NextFrame, Tweenframes[InstID].Next.Clip);
    return lerp(curr, next, (matrix) Tweenframes[InstID].Next.Time);
    
}

matrix LerpCurrAnim_NextAnimTransform(uint boneIndices, uint InstID)
{
   
    
    matrix curr = GetCurrAnimTransforms(boneIndices, InstID);
    matrix next = GetNextAnimTransforms(boneIndices, InstID);
    return lerp(curr, next, Tweenframes[InstID].TweenTime);
  
}

void SetAnimationWorld(float4 BlendIndices, float4 BlendWeights, uint InstID)
{
    matrix transform = 0;
    matrix Anim = 0;
  
    float4 boneIndices = BoneIndeces(BlendIndices);
    float4 boneWeights = BoneWeights(BlendWeights);
    uint instId = InstID;
       
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
     
    float4 inst1 = InstTransforms[int2(InstID * 4 + 0, actorIndex)];
    float4 inst2 = InstTransforms[int2(InstID * 4 + 1, actorIndex)];
    float4 inst3 = InstTransforms[int2(InstID * 4 + 2, actorIndex)];
    float4 inst4 = InstTransforms[int2(InstID * 4 + 3, actorIndex)];
    
    World = mul(transform, matrix(inst1, inst2, inst3, inst4));
}

struct VertexPosBlendInst
{
    float4 Position : Position0;
    float4 BlendIndices : BlendIndices0;
    float4 BlendWeights : BlendWeights0;
    uint InstID : SV_InstanceID;

};
float4 CascadedShadowGenVS(VertexPosBlendInst input) : SV_Position
{
    
  
    SetAnimationWorld(input.BlendIndices, input.BlendWeights, input.InstID);
  
    float4 Position = WorldPosition(input.Position);
   
  
    return Position;

}
VertexModelOutput VS(VertexSkeletal input)
{
    VertexModelOutput output;
    SetAnimationWorld(input.BlendIndices,input.BlendWeights,input.InstID);
    output.Position = WorldPosition(input.Position);
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
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////



