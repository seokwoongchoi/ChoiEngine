#include "000_Matrix.fx"

struct ClipFrameBoneMatrix
{
    matrix Bone;
};

struct ResultMatrix
{
    matrix Result;
    
};

StructuredBuffer<ClipFrameBoneMatrix> Input;
RWStructuredBuffer<ResultMatrix> Output;

#define MAX_MODEL_TRANSFORMS 500
#define MAX_MODEL_KEYFRAMES 300
#define MAX_MODEL_INSTANCE 100

struct AnimationFrame
{
    int Clip;

    uint CurrFrame;
    uint NextFrame;

    float Time;
    float Running;

    float3 Padding;
};

struct TweenFrame
{
    float TakeTime;
    float TweenTime;
    float RunningTime;
    float Padding;

    AnimationFrame Curr;
    AnimationFrame Next;
};

cbuffer CB_AnimationFrame
{
    TweenFrame Tweenframes[MAX_MODEL_INSTANCE];
};

cbuffer CB_Attach
{
    matrix local;
    matrix transforms[MAX_MODEL_INSTANCE];
    uint AttachBoneIndex;
    
    float3 pad;
};

void GetCurrAnimTransforms(inout matrix currAnim,uint index)
{
    
    
    uint boneIndex[2];
    boneIndex[0] = (Tweenframes[index].Curr.Clip * MAX_MODEL_KEYFRAMES * MAX_MODEL_TRANSFORMS);
    boneIndex[0] += (Tweenframes[index].Curr.CurrFrame * MAX_MODEL_TRANSFORMS);
    boneIndex[0] += AttachBoneIndex;

    boneIndex[1] = (Tweenframes[index].Curr.Clip * MAX_MODEL_KEYFRAMES * MAX_MODEL_TRANSFORMS);
    boneIndex[1] += (Tweenframes[index].Curr.NextFrame * MAX_MODEL_TRANSFORMS);
    boneIndex[1] += AttachBoneIndex;

    matrix currFrame = Input[boneIndex[0]].Bone;
    matrix nextFrame = Input[boneIndex[1]].Bone;

    currAnim = lerp(currFrame, nextFrame, Tweenframes[index].Curr.Time);
}

void GetNextAnimTransforms(inout matrix nextAnim,uint index)
{
    matrix result = 0;
    uint boneIndex[2];
    boneIndex[0] = (Tweenframes[index].Next.Clip * MAX_MODEL_KEYFRAMES * MAX_MODEL_TRANSFORMS);
    boneIndex[0] += (Tweenframes[index].Next.CurrFrame * MAX_MODEL_TRANSFORMS);
    boneIndex[0] += AttachBoneIndex;

    boneIndex[1] = (Tweenframes[index].Next.Clip * MAX_MODEL_KEYFRAMES * MAX_MODEL_TRANSFORMS);
    boneIndex[1] += (Tweenframes[index].Next.NextFrame * MAX_MODEL_TRANSFORMS);
    boneIndex[1] += AttachBoneIndex;

    matrix currFrame = Input[boneIndex[0]].Bone;
    matrix nextFrame = Input[boneIndex[1]].Bone;

     nextAnim = lerp(currFrame, nextFrame, Tweenframes[index].Next.Time);

     
}
[numthreads(MAX_MODEL_INSTANCE, 1, 1)]
void CS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    
    if (DispatchThreadID.x < MAX_MODEL_INSTANCE)
    {
        uint index = DispatchThreadID.x;
        matrix result = 0;
      
        GetCurrAnimTransforms(result, index);
    
       [flatten]
        if (Tweenframes[index].Next.Clip > -1)
        {
            matrix nextAnim = 0;
            GetNextAnimTransforms(nextAnim,index);
            result = lerp(result, nextAnim, Tweenframes[index].TweenTime);
        }
        matrix final = local * result * transforms[i];
        Output[index].Result = result;
    }
   
   // Output[index].index = index;
}

technique11 T0
{
    pass P0
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, CS()));
    }
}