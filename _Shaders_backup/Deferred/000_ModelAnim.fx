#define MAX_MODEL_KEYFRAMES 250
#define MAX_MODEL_INSTANCE 100
#define MAX_MODEL_TRANSFORMS 500
Texture2DArray BoneTransforms;
Texture2DArray AnimBoneTransforms;
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
    float TakeTweenTime;
    float TweenTime;
    float2 Padding;

    Keyframe Curr;
    Keyframe Next;

   
};
StructuredBuffer<TweenFrame> tweens;
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
    float tess;
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

    output.oPosition = input.Position.xyz;
   output.Position = WorldPosition(input.Position);
   
    output.wPosition = output.Position.xyz;
   

   output.Position = ViewProjection(output.Position);
  
   
	output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

    output.Alpha = 0;
  

    output.Cull.x = dot(float4(output.wPosition - ViewPosition() , 1.0f), -g_FrustumNormals[0]);
    output.Cull.y = dot(float4(output.wPosition - ViewPosition() , 1.0f), -g_FrustumNormals[1]);
    output.Cull.z = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
    output.Cull.w = 0;
   
    
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
    output.oPosition = input.Position.xyz;
    output.Position = mul(input.Position, previewWorld);
   
    output.wPosition = output.Position.xyz;
   

    output.Position = OrbitViewProjection(output.Position);
  
   
    output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

 
    output.Alpha = 0;
  
    output.Cull.x = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
    output.Cull.y = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
    output.Cull.z = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
    output.Cull.w = 0;
    
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
        
float4 LoadAnimTransforms(uint boneIndices,uint frame,uint clip)
{
    return AnimBoneTransforms.Load(uint4(boneIndices, frame, clip, 0));
}

matrix GetCurrAnimTransforms(float4 boneIndices,uint index,uint InstID)
{
    
    float4 c0, c1, c2, c3;
    float4 n0, n1, n2, n3;

    matrix curr = 0;
    matrix next = 0;
    matrix anim = 0;
    
    c0 = LoadAnimTransforms(boneIndices[index] * 4 + 0, tweens[InstID].Curr.CurrFrame, tweens[InstID].Curr.Clip);
    c1 = LoadAnimTransforms(boneIndices[index] * 4 + 1, tweens[InstID].Curr.CurrFrame, tweens[InstID].Curr.Clip);
    c2 = LoadAnimTransforms(boneIndices[index] * 4 + 2, tweens[InstID].Curr.CurrFrame, tweens[InstID].Curr.Clip);
    c3 = LoadAnimTransforms(boneIndices[index] * 4 + 3, tweens[InstID].Curr.CurrFrame, tweens[InstID].Curr.Clip);
    curr = matrix(c0, c1, c2, c3);                 
                                                   
    n0 = LoadAnimTransforms(boneIndices[index] * 4 + 0, tweens[InstID].Curr.NextFrame, tweens[InstID].Curr.Clip);
    n1 = LoadAnimTransforms(boneIndices[index] * 4 + 1, tweens[InstID].Curr.NextFrame, tweens[InstID].Curr.Clip);
    n2 = LoadAnimTransforms(boneIndices[index] * 4 + 2, tweens[InstID].Curr.NextFrame, tweens[InstID].Curr.Clip);
    n3 = LoadAnimTransforms(boneIndices[index] * 4 + 3, tweens[InstID].Curr.NextFrame, tweens[InstID].Curr.Clip);
    next = matrix(n0, n1, n2, n3);

    return anim = lerp(curr, next, (matrix) tweens[InstID].Curr.CurrFrame);
}

matrix GetNextAnimTransforms(float4 boneIndices, uint index, uint InstID)
{
    
    float4 c0, c1, c2, c3;
    float4 n0, n1, n2, n3;

    matrix curr = 0;
    matrix next = 0;
    matrix anim = 0;
    
    c0 = LoadAnimTransforms(boneIndices[index] * 4 + 0, tweens[InstID].Next.CurrFrame, tweens[InstID].Next.Clip);
    c1 = LoadAnimTransforms(boneIndices[index] * 4 + 1, tweens[InstID].Next.CurrFrame, tweens[InstID].Next.Clip);
    c2 = LoadAnimTransforms(boneIndices[index] * 4 + 2, tweens[InstID].Next.CurrFrame, tweens[InstID].Next.Clip);
    c3 = LoadAnimTransforms(boneIndices[index] * 4 + 3, tweens[InstID].Next.CurrFrame, tweens[InstID].Next.Clip);
    curr = matrix(c0, c1, c2, c3);
                                                   
    n0 = LoadAnimTransforms(boneIndices[index] * 4 + 0, tweens[InstID].Next.NextFrame, tweens[InstID].Next.Clip);
    n1 = LoadAnimTransforms(boneIndices[index] * 4 + 1, tweens[InstID].Next.NextFrame, tweens[InstID].Next.Clip);
    n2 = LoadAnimTransforms(boneIndices[index] * 4 + 2, tweens[InstID].Next.NextFrame, tweens[InstID].Next.Clip);
    n3 = LoadAnimTransforms(boneIndices[index] * 4 + 3, tweens[InstID].Next.NextFrame, tweens[InstID].Next.Clip);
    next = matrix(n0, n1, n2, n3);

    return anim = lerp(curr, next, (matrix) tweens[InstID].Next.Time);
}


//void SetAnimationWorld(inout matrix world, VertexModel input)
//{
//    matrix transform = 0;
//    matrix currAnim = 0;
//    matrix nextAnim = 0;
//    float4 boneIndices = BoneIndeces(input);
//    float4 boneWeights = BoneWeights(input);
//    uint instId = input.InstID;
   
//	[unroll(4)]
//    for (int i = 0; i < 4; i++)
//    {
//        currAnim = GetCurrAnimTransforms(boneIndices, i, instId);
//        [flatten]
//        if (tweens[input.InstID].Next.Clip >= 0)
//        {
//           nextAnim = GetNextAnimTransforms(boneIndices, i, instId);
//            currAnim = lerp(currAnim, nextAnim, tweens[instId].TweenTime);
//        }

//        transform += mul(boneWeights[i], currAnim);
//    }

//    world = mul(transform, input.Transform);
//}
void SetAnimationWorld(inout matrix world, VertexModel input)
{
    float4 c0, c1, c2, c3;
    float4 n0, n1, n2, n3;

    matrix curr = 0;
    matrix next = 0;
    matrix transform = 0;
    matrix anim = 0;
    matrix nextAnim = 0;
   

    float boneIndices[4] = { input.BlendIndices.x, input.BlendIndices.y, input.BlendIndices.z, input.BlendIndices.w };
    float boneWeights[4] = { input.BlendWeights.x, input.BlendWeights.y, input.BlendWeights.z, input.BlendWeights.w };
   
    //Attach Model
    if (any(input.BlendIndices) == false)
    {
        boneIndices[0] = BoneIndex;
        boneWeights[0] = 1.0f;
        
      //c0 = BoneTransforms.Load(uint4(boneIndices[0] * 4 + 0, 0, 0, 0));
      //c1 = BoneTransforms.Load(uint4(boneIndices[0] * 4 + 1, 0, 0, 0));
      //c2 = BoneTransforms.Load(uint4(boneIndices[0] * 4 + 2, 0, 0, 0));
      //c3 = BoneTransforms.Load(uint4(boneIndices[0] * 4 + 3, 0, 0, 0));
      //transform = matrix(c0, c1, c2, c3);
      //world = mul(transform, input.Transform);
      
      //return;
    }

    uint clip[2];
    uint currFrame[2];
    uint nextFrame[2];

    clip[0] = Tweenframes[input.InstID].Curr.Clip;
    currFrame[0] = Tweenframes[input.InstID].Curr.CurrFrame;
    nextFrame[0] = Tweenframes[input.InstID].Curr.NextFrame;

    clip[1] = Tweenframes[input.InstID].Next.Clip;
    currFrame[1] = Tweenframes[input.InstID].Next.CurrFrame;
    nextFrame[1] = Tweenframes[input.InstID].Next.NextFrame;
    
    //for (int i = 0; i < 4; i++)
    //    transform += mul(boneWeights[i], AnimBoneTransforms.Load(uint4(boneIndices[i], input.InstID, clip[0], 0)));
	[unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        c0 = AnimBoneTransforms.Load(uint4(boneIndices[i] * 4 + 0, input.InstID, clip[0], 0));
        c1 = AnimBoneTransforms.Load(uint4(boneIndices[i] * 4 + 1, input.InstID, clip[0], 0));
        c2 = AnimBoneTransforms.Load(uint4(boneIndices[i] * 4 + 2, input.InstID, clip[0], 0));
        c3 = AnimBoneTransforms.Load(uint4(boneIndices[i] * 4 + 3, input.InstID, clip[0], 0));
        curr = matrix(c0, c1, c2, c3);
    
       
		
        transform += mul(boneWeights[i], curr);
    }

    world = mul(transform, input.Transform);
}
MeshOutput VS_Animation(VertexModel input)
{
	MeshOutput output;

	SetAnimationWorld(World, input);
   // World = input.Transform;
	output.oPosition = input.Position.xyz;
	output.Position = WorldPosition(input.Position);
	output.wPosition = output.Position.xyz;

	output.Position = ViewProjection(output.Position);
	output.wvpPosition = output.Position;

	output.Normal = WorldNormal(input.Normal);
	output.Tangent = WorldTangent(input.Tangent);
	output.Uv = input.Uv;

   
    output.Alpha = 0;
    output.Cull.x = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
    output.Cull.y = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
    output.Cull.z = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
    output.Cull.w = 0;
  
    
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

    output.oPosition = input.Position.xyz;
    output.Position = mul(input.Position, previewWorld);
    output.wPosition = output.Position.xyz;

    output.Position = OrbitViewProjection(output.Position);
    output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

   
    output.Alpha = 0;
    output.Cull.x = 0;
    output.Cull.y = 0;
    output.Cull.z = 0;
    output.Cull.w = 0;
    
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

    output.oPosition = input.Position.xyz;
    output.Position = mul(input.Position, previewWorld);
    output.wPosition = output.Position.xyz;

    output.Position = OrbitViewProjection(output.Position);
    output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

   
    output.Alpha = 0;
    output.Cull.x = 0;
    output.Cull.y = 0;
    output.Cull.z = 0;
    output.Cull.w = 0;
    
    return output;
}

MeshOutput VS_Forward(VertexModel input)
{
    MeshOutput output;
    

    
    matrix previewWorld = mul(forwardTransform, input.Transform);

    output.oPosition = input.Position.xyz;
    output.Position = mul(input.Position, previewWorld);
    output.wPosition = output.Position.xyz;

    output.Position = ViewProjection(output.Position);
    output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

   
    output.Alpha = 0;
    output.Cull.x = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
    output.Cull.y = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
    output.Cull.z = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[2]);
    output.Cull.w = 0;
  
    return output;
}
