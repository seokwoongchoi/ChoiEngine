#include "Matrix.hlsl"

#define MAX_MODEL_INSTANCE 20
#define MAX_ACTOR_BONECOLLIDER 3


Texture2D<float4> InstTransforms : register(t0);
Texture2DArray<float4> AnimBoneTransforms : register(t1);
Texture2D<float4> boneBoxTransforms : register(t2);

RWTexture2DArray<float4> OutputAnimBoneTransforms : register(u0);
RWTexture2D<float4> Output : register(u1);
//RWTexture2D<float4> EffectOutput;


cbuffer CB_DrawCount:register(b0)
{
    uint staticDrawCount : packoffset(c0.x);
    uint skeletalDrawCount : packoffset(c0.y);
    uint particleIndex : packoffset(c0.z);
    float pad0 : packoffset(c0.w);
};
cbuffer CB_Box : register(b1)
{
    matrix BoxBound[MAX_MODEL_INSTANCE];
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
        matrix final = mul(BoxBound[groupThreadId.x], transform);
          
        
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
};

struct TweenFrame
{
    float TweenTime;
    uint totalCount;
    uint maxBoneCount;
    float Padding;

    AnimationFrame Curr;
    AnimationFrame Next;
};

cbuffer CB_AnimationFrame : register(b2)
{
    TweenFrame Tweenframes[MAX_MODEL_INSTANCE];
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

void GetBoneBoxtMatrix(inout matrix transform, uint actorIndex, uint boneBoxIndex,int boxType)
{
    float4 m1 = boneBoxTransforms[int2(boneBoxIndex * 9 +0+(4*boxType), actorIndex)];
    float4 m2 = boneBoxTransforms[int2(boneBoxIndex * 9 +1+(4*boxType), actorIndex)];
    float4 m3 = boneBoxTransforms[int2(boneBoxIndex * 9 +2+(4*boxType), actorIndex)];
    float4 m4 = boneBoxTransforms[int2(boneBoxIndex * 9 +3+(4*boxType), actorIndex)];
    transform = matrix(m1, m2, m3, m4);
}
[numthreads(256, 1, 1)]
void SkeletalColliderCS(uint3 groupId : SV_GroupID,
                        uint3 groupThreadId : SV_GroupThreadID,
                        uint3 globalThreadId : SV_DispatchThreadID)
{
    
    if (globalThreadId.x < Tweenframes[0].maxBoneCount) 
    for (int i = 0; i < Tweenframes[0].totalCount;i++)
    {
        
              matrix result = lerp(GetCurrAnimTransforms(i, globalThreadId.x),
                        lerp(GetCurrAnimTransforms(i, globalThreadId.x),
                             GetNextAnimTransforms(i, globalThreadId.x),
             Tweenframes[i].TweenTime), saturate(Tweenframes[i].Next.Clip + 1));
          
            GroupMemoryBarrierWithGroupSync();
            OutputAnimBoneTransforms[uint3(globalThreadId.x * 4 + 0, Tweenframes[i].Curr.CurrFrame, Tweenframes[i].Curr.Clip)] = result._11_12_13_14;
            OutputAnimBoneTransforms[uint3(globalThreadId.x * 4 + 1, Tweenframes[i].Curr.CurrFrame, Tweenframes[i].Curr.Clip)] = result._21_22_23_24;
            OutputAnimBoneTransforms[uint3(globalThreadId.x * 4 + 2, Tweenframes[i].Curr.CurrFrame, Tweenframes[i].Curr.Clip)] = result._31_32_33_34;
            OutputAnimBoneTransforms[uint3(globalThreadId.x * 4 + 3, Tweenframes[i].Curr.CurrFrame, Tweenframes[i].Curr.Clip)] = result._41_42_43_44;
            float3 position = 0;
            float4 q = 0;
            float3 scale = 0;
           [unroll(3)]
            for (uint b = 0; b < 3; b++)
            {
                int boneIndex = boneBoxTransforms[int2(b * 9 + 8, groupId.x)].x;
                if (globalThreadId.x == boneIndex)
                {
                    Matrix local;
                    GetBoneBoxtMatrix(local, groupId.x, b, 0);
        
                    matrix computeMatrix = mul(local, result);
                    matrix transform;
                    GetInstMatrix(transform, groupId.x, i);
       
                    matrix final = mul(computeMatrix, transform);
                    decompose(final, position, q, scale);
    
    
                    Matrix BoneScale;
                    GetBoneBoxtMatrix(BoneScale, groupId.x, b, 1);
                    matrix compute = mul(BoneScale, compose(position, q, float3(1, 1, 1)));
                    
                    [flatten]
                    if (b == 2 && Tweenframes[i].Curr.Clip==3)
                    {
                        if ( Tweenframes[i].Curr.CurrFrame < 17 || Tweenframes[i].Curr.CurrFrame > 22)
                        compute = 0;

                    }
        
                    GroupMemoryBarrierWithGroupSync();
                    Output[uint2(b * 4 + 0, i)] = float4(compute._11, compute._12, compute._13, compute._14);
                    Output[uint2(b * 4 + 1, i)] = float4(compute._21, compute._22, compute._23, compute._24);
                    Output[uint2(b * 4 + 2, i)] = float4(compute._31, compute._32, compute._33, compute._34);
                    Output[uint2(b * 4 + 3, i)] = float4(compute._41, compute._42, compute._43, compute._44);
                }
                     
   
               
            }
            
           
                                                                         
     }
      
        
         
   
     
   
}
//[numthreads(10, 1, 1)]
//void SkeletalColliderCS(uint3 groupId        : SV_GroupID,
//                        uint3 groupThreadId  : SV_GroupThreadID, 
//                        uint3 globalThreadId : SV_DispatchThreadID)
//{
    
            
//    uint index = globalThreadId.x;
         
//   // if (globalThreadId.x < skeletalDrawCount)
//    {
//        float3 position = 0;
//        float4 q = 0;
//        float3 scale = 0;
//        [unroll(3)]
//        for(uint i = 0; i < 3; i++)
      
//        {
//            int boneIndex = boneBoxTransforms[int2(i * 9 + 8, groupId.x)].x;
//            matrix result = lerp(GetCurrAnimTransforms(globalThreadId.x, boneIndex),
//                        lerp(GetCurrAnimTransforms(globalThreadId.x, boneIndex),
//                             GetNextAnimTransforms(globalThreadId.x, boneIndex),
//             Tweenframes[globalThreadId.x].TweenTime), saturate(Tweenframes[globalThreadId.x].Next.Clip + 1));
        
         
   
//            Matrix local;
//            GetBoneBoxtMatrix(local, groupId.x, i, 0);
        
//            matrix computeMatrix = mul(local, result);
//            matrix transform;
//            GetInstMatrix(transform, groupId.x, groupThreadId.x);
       
//            matrix final = mul(computeMatrix, transform);
//            decompose(final, position, q, scale);
    
    
//            Matrix BoneScale;
//            GetBoneBoxtMatrix(BoneScale, groupId.x, i, 1);
//            matrix compute = mul(BoneScale, compose(position, q, float3(1, 1, 1)));
        
//            GroupMemoryBarrierWithGroupSync();
//            Output[uint2(i * 4 + 0, index)] = float4(compute._11, compute._12, compute._13, compute._14);
//            Output[uint2(i * 4 + 1, index)] = float4(compute._21, compute._22, compute._23, compute._24);
//            Output[uint2(i * 4 + 2, index)] = float4(compute._31, compute._32, compute._33, compute._34);
//            Output[uint2(i * 4 + 3, index)] = float4(compute._41, compute._42, compute._43, compute._44);
//        }
     
        
       
    
     
//    }
   
     
   
//}


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




