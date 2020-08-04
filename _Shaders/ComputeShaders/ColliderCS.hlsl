#include "Matrix.hlsl"

#define MAX_MODEL_INSTANCE 10
#define MAX_ACTOR_BONECOLLIDER 2


Texture2D<float4> InstTransforms : register(t0);
Texture2DArray<float4> AnimBoneTransforms : register(t1);

RWTexture2D<float4> Output;
//RWTexture2D<float4> EffectOutput;

cbuffer CB_Box
{
   
    matrix BoxBound;
   
};
cbuffer CB_DrawCount:register(b0)
{
    uint staticDrawCount : packoffset(c0.x);
    uint skeletalDrawCount : packoffset(c0.y);
    uint particleIndex : packoffset(c0.z);
    float pad0 : packoffset(c0.w);
};
void GetInstMatrix(inout matrix transform, uint actorIndex, uint InstID)
{
    float4 inst1 = InstTransforms[int2(InstID * 4 + 0, actorIndex)];
    float4 inst2 = InstTransforms[int2(InstID * 4 + 1, actorIndex)];
    float4 inst3 = InstTransforms[int2(InstID * 4 + 2, actorIndex)];
    float4 inst4 = InstTransforms[int2(InstID * 4 + 3, actorIndex)];
    transform = matrix(inst1, inst2, inst3, inst4);
}

[numthreads(MAX_MODEL_INSTANCE, 1, 1)]
void StaticColliderCS(uint3 groupID : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID, uint3 globalThreadId : SV_DispatchThreadID)
{
    if (globalThreadId.x < staticDrawCount)
    {
        matrix transform = 0;
        GetInstMatrix(transform, groupThreadId.x, groupID.x);
        matrix final = mul(BoxBound, transform);
          
        
        GroupMemoryBarrierWithGroupSync();
        uint index = skeletalDrawCount + globalThreadId.x;
      
        Output[uint2(0, index)] = float4(final._11, final._12, final._13, final._14);
        Output[uint2(1, index)] = float4(final._21, final._22, final._23, final._24);
        Output[uint2(2, index)] = float4(final._31, final._32, final._33, final._34);
        Output[uint2(3, index)] = float4(final._41, final._42, final._43, final._44);
    
        Output[uint2(4, index)] = 0;
        Output[uint2(5, index)] = 0;
        Output[uint2(6, index)] = 0;
        Output[uint2(7, index)] = 0;
    }
   
     
   
}
struct AnimationFrame
{
    int Clip;

    uint CurrFrame;
    uint NextFrame;

    float Time;
    
    float4 Padding;
};

struct TweenFrame
{
    float TakeTime;
    float TweenTime;
    

    float2 Padding;

    AnimationFrame Curr;
    AnimationFrame Next;
};



cbuffer CB_AnimationFrame
{
    TweenFrame Tweenframes[MAX_MODEL_INSTANCE];
};


struct AttachData
{
    matrix local;
    matrix BoneScale;
    
    int index;
    
    float2 padding;
};


cbuffer CB_Attach
{
    AttachData BoneCollider[MAX_ACTOR_BONECOLLIDER];
  
};

struct EffectData
{
    matrix effectlocal;
  
    int EffectIndex;
    float3 pad0;
};

cbuffer CB_EffectAttach
{
    EffectData effectData[MAX_ACTOR_BONECOLLIDER];
   
};

matrix LoadAnimTransforms(uint boneIndices, uint frame, uint clip)
{
    float4 c0, c1, c2, c3;
    c0 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 0, frame, clip, 0));
    c1 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 1, frame, clip, 0));
    c2 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 2, frame, clip, 0));
    c3 = AnimBoneTransforms.Load(uint4(boneIndices * 4 + 3, frame, clip, 0));
    
    return matrix(c0, c1, c2, c3);
}

matrix GetCurrAnimTransforms(uint index, uint AttachBoneIndex)
{
    matrix curr = LoadAnimTransforms(AttachBoneIndex, Tweenframes[index].Curr.CurrFrame, Tweenframes[index].Curr.Clip);
    matrix next = LoadAnimTransforms(AttachBoneIndex, Tweenframes[index].Curr.NextFrame, Tweenframes[index].Curr.Clip);
    return lerp(curr, next, (matrix) Tweenframes[index].Curr.Time);
 }

matrix GetNextAnimTransforms(uint index, uint AttachBoneIndex)
{
    
    
    matrix curr = LoadAnimTransforms(AttachBoneIndex, Tweenframes[index].Next.CurrFrame, Tweenframes[index].Next.Clip);
    matrix next = LoadAnimTransforms(AttachBoneIndex, Tweenframes[index].Next.NextFrame, Tweenframes[index].Next.Clip);
    return lerp(curr, next, (matrix) Tweenframes[index].Next.Time);
       
}
[numthreads(MAX_MODEL_INSTANCE, 1, 1)]
void SkeletalColliderCS(uint3 groupID : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID, uint3 globalThreadId : SV_DispatchThreadID)
{
    if (globalThreadId.x < skeletalDrawCount)
    {
      
        matrix result = 0;
        matrix computeMatrix = 0;
        matrix final = 0;
       
        matrix transform = 0;
        float3 position = 0;
        float4 q = 0;
        float3 scale = 0;
        matrix compute2 = 0;
        GroupMemoryBarrierWithGroupSync();
        result = lerp(
        GetCurrAnimTransforms(groupThreadId.x, BoneCollider[0].index),
        lerp(GetCurrAnimTransforms(groupThreadId.x, BoneCollider[0].index), GetNextAnimTransforms(groupThreadId.x, BoneCollider[0].index),
        Tweenframes[groupThreadId.x].TweenTime),
        saturate(Tweenframes[groupThreadId.x].Next.Clip + 1)
        );
              
        
        computeMatrix = mul(BoneCollider[0].local, result);
        GetInstMatrix(transform, groupThreadId.x, groupID.x);
        final = mul(computeMatrix, transform);
        decompose(final, position, q, scale);
        
       
        matrix compute = mul(BoneCollider[0].BoneScale, compose(position, q, float3(1, 1, 1)));
        
        [flatten]
        if (BoneCollider[1].index > 0)
        {
            matrix result2 = lerp(
            GetCurrAnimTransforms(groupThreadId.x, BoneCollider[1].index),
            lerp(GetCurrAnimTransforms(groupThreadId.x, BoneCollider[1].index), GetNextAnimTransforms(groupThreadId.x, BoneCollider[1].index),
            Tweenframes[groupThreadId.x].TweenTime),
            saturate(Tweenframes[groupThreadId.x].Next.Clip + 1)
            );
       
            matrix computeMatrix2 = mul(BoneCollider[1].local, result2);
            matrix final2 = mul(computeMatrix2, transform);
              
            float3 position2 = 0;
            float4  q2 = 0;
            float3  scale2 = 0;
        
            decompose(final2, position2, q2, scale2);
            compute2 = mul(BoneCollider[1].BoneScale, compose(position2, q2, float3(1, 1, 1)));
        }
        
      
        GroupMemoryBarrierWithGroupSync();

            
      
        
        uint index = globalThreadId.x;
        
        Output[uint2(0, index)] = float4(compute._11, compute._12, compute._13, compute._14);
        Output[uint2(1, index)] = float4(compute._21, compute._22, compute._23, compute._24);
        Output[uint2(2, index)] = float4(compute._31, compute._32, compute._33, compute._34);
        Output[uint2(3, index)] = float4(compute._41, compute._42, compute._43, compute._44);
        
        Output[uint2(4, index)] = float4(compute2._11, compute2._12, compute2._13, compute2._14);
        Output[uint2(5, index)] = float4(compute2._21, compute2._22, compute2._23, compute2._24);
        Output[uint2(6, index)] = float4(compute2._31, compute2._32, compute2._33, compute2._34);
        Output[uint2(7, index)] = float4(compute2._41, compute2._42, compute2._43, compute2._44);
          
   
    }
   
}


//[numthreads(1, 1, 1)]
//void EffectCS(uint3 globalThreadId : SV_DispatchThreadID)
//{
    
   
   
//   matrix result3 = lerp
//         (
//         GetCurrAnimTransforms(globalThreadId.x, effectData[particleIndex].EffectIndex),
//         lerp
//         (
//         GetCurrAnimTransforms(globalThreadId.x, effectData[particleIndex].EffectIndex),
//         GetNextAnimTransforms(globalThreadId.x, effectData[particleIndex].EffectIndex),
//         Tweenframes[globalThreadId.x].TweenTime
//         ),
//         saturate(Tweenframes[globalThreadId.x].Next.Clip + 1)
//         );
//    //matrix local = mul(effectData[particleIndex].effectTranslation,effectData[particleIndex].effectlocal);
//    matrix computeMatrix3 = mul(effectData[particleIndex].effectlocal, result3);
//   // GetInstMatrix(transform, globalThreadId.x);
   
            
     
   
//   float3 position = 0;
//    float4 q = float4(0.0f, 0.0f, 0.0f,0.0f);
//   float3 scale = 0;
        
//    decompose(computeMatrix3, position, q, scale);
//    matrix result = compose( position, q, float3(1, 1, 1));
//   //
        

     
//    EffectOutput[uint2(0, globalThreadId.x)] = float4(result._11, result._12, result._13, result._14);
//    EffectOutput[uint2(1, globalThreadId.x)] = float4(result._21, result._22, result._23, result._24);
//    EffectOutput[uint2(2, globalThreadId.x)] = float4(result._31, result._32, result._33, result._34);
//    EffectOutput[uint2(3, globalThreadId.x)] = float4(result._41, result._42, result._43, result._44);
        
//}




