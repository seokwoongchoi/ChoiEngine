#define MAX_MODEL_KEYFRAMES 250
#define MAX_MODEL_INSTANCE 100
#define MAX_MODEL_TRANSFORMS 500
Texture2DArray BoneTransforms;
cbuffer CB_UpdateBone
{
    matrix UpdateBone;
};

///////////////////////////////////////////////////////////////////////////////

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
struct PrevVertexModel
{
    float4 Position : Position0;
    float2 Uv : Uv0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
    float4 BlendIndices : BlendIndices0;
    float4 BlendWeights : BlendWeights0;

 

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

   
        
  
    matrix transform = matrix(m1, m2, m3, m4);
   
    world = mul(transform, input.Transform);
}
ModelOutput_Lod VS_Model_Lod(VertexModel input)
{
    ModelOutput_Lod output;

    SetModelWorld(World, input);

  
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
    float2 texCoord : Uv0;
    //float3 positionWS : TEXCOORD1;
    float3 normal : Normal0;
    float3 tangent : Tangent0;
   
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
    
     // Geometry cubic generated control points
    float3 f3B210 : POSITION3;
    float3 f3B120 : POSITION4;
    float3 f3B021 : POSITION5;
    float3 f3B012 : POSITION6;
    float3 f3B102 : POSITION7;
    float3 f3B201 : POSITION8;
    float3 f3B111 : CENTER;
    
    float3 f3N110 : NORMAL3;
    float3 f3N011 : NORMAL4;
    float3 f3N101 : NORMAL5;

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
   
    for (uint ie = 0; ie < 3; ++ie)
    {
        output.Edges[ie] = 1 / (float) 512 * (float) 64;
       //float3 edge = inputPatch[(ie + 1) % 3].Position - inputPatch[ie].Position;
       
       //float3 vec = (inputPatch[(ie + 1) % 3].Position + inputPatch[ie].Position) / 2 - ViewPosition();
       //float d = distance(edge, vec);
       //float len = sqrt(dot(edge, edge) / dot(vec, vec));
       //int tess = lerp(0, 1, len * 5);
       //output.Edges[(ie + 1) % 3] = tess;
     //  output.Edges[(ie + 1) % 3] = 1;
  

    }
  
    
    
      // culling
    int culled[4];
    for (int ip = 0; ip < 4; ++ip)
    {
        culled[ip] = 1;
        culled[ip] &= dot(inputPatch[0].Position.xyz - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
        culled[ip] &= dot(inputPatch[1].Position.xyz - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
        culled[ip] &= dot(inputPatch[2].Position.xyz - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
    }
    if (culled[0] || culled[1] || culled[2] || culled[3])
        output.Edges[0] = 0;
    
    output.f3B210 = ((2.0f * inputPatch[0].Position) + inputPatch[1].Position - (dot((inputPatch[1].Position - inputPatch[0].Position), inputPatch[0].normal) * inputPatch[0].normal)) / 3.0f;
    output.f3B120 = ((2.0f * inputPatch[1].Position) + inputPatch[0].Position - (dot((inputPatch[0].Position - inputPatch[1].Position), inputPatch[1].normal) * inputPatch[1].normal)) / 3.0f;
    output.f3B021 = ((2.0f * inputPatch[1].Position) + inputPatch[2].Position - (dot((inputPatch[2].Position - inputPatch[1].Position), inputPatch[1].normal) * inputPatch[1].normal)) / 3.0f;
    output.f3B012 = ((2.0f * inputPatch[2].Position) + inputPatch[1].Position - (dot((inputPatch[1].Position - inputPatch[2].Position), inputPatch[2].normal) * inputPatch[2].normal)) / 3.0f;
    output.f3B102 = ((2.0f * inputPatch[2].Position) + inputPatch[0].Position - (dot((inputPatch[0].Position - inputPatch[2].Position), inputPatch[2].normal) * inputPatch[2].normal)) / 3.0f;
    output.f3B201 = ((2.0f * inputPatch[0].Position) + inputPatch[2].Position - (dot((inputPatch[2].Position - inputPatch[0].Position), inputPatch[0].normal) * inputPatch[0].normal)) / 3.0f;
    // center control point
    float3 f3E = (output.f3B210 + output.f3B120 + output.f3B021 + output.f3B012 + output.f3B102 + output.f3B201) / 6.0f;
    float3 f3V = (inputPatch[0].Position + inputPatch[1].Position + inputPatch[2].Position) / 3.0f;
    output.f3B111 = f3E + ((f3E - f3V) / 2.0f);
    float3 f3B003 = inputPatch[0].Position;
    float3 f3B030 = inputPatch[1].Position;
    float3 f3B300 = inputPatch[2].Position;
        // And Normals
    float3 f3N002 = inputPatch[0].Normal;
    float3 f3N020 = inputPatch[1].Normal;
    float3 f3N200 = inputPatch[2].Normal;
            
    float fV12 = 2.0f * dot(f3B030 - f3B003, f3N002 + f3N020) / dot(f3B030 - f3B003, f3B030 - f3B003);
    output.f3N110 = normalize(f3N002 + f3N020 - fV12 * (f3B030 - f3B003));
    float fV23 = 2.0f * dot(f3B300 - f3B030, f3N020 + f3N200) / dot(f3B300 - f3B030, f3B300 - f3B030);
    output.f3N011 = normalize(f3N020 + f3N200 - fV23 * (f3B300 - f3B030));
    float fV31 = 2.0f * dot(f3B003 - f3B300, f3N200 + f3N002) / dot(f3B003 - f3B300, f3B003 - f3B300);
    output.f3N101 = normalize(f3N200 + f3N002 - fV31 * (f3B003 - f3B300));
   
  //  output.Inside = 0;
    output.Inside = (output.Edges[0] + output.Edges[1] + output.Edges[2]) / 3;


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
    float3 position = inputPatch[0].Position.xyz * fWW * fW + inputPatch[1].Position.xyz * fUU * fU + inputPatch[2].Position.xyz * fVV * fV

    +input.f3B210 * fWW3 * fU + input.f3B120 * fW * fUU3 + input.f3B201 * fWW3 * fV + input.f3B021 * fUU3 * fV +
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
    output.texCoord = texCoord;
    //output.position = mul(float4(position, 1.0f), g_ModelViewProjectionMatrix);
  //  output.positionWS = position;
    output.normal = normal;
    output.tangent = tangent;
  

   // output.sign = input.sign; //inputPatch[0].sign;

    return output;
}


struct VS_ReflectionOUTPUT
{
    float4 Position : SV_Position;
    float4 ViewPosition : TEXCOORD0;
    float3 ViewNormal : TEXCOORD1;
    float3 csPos : TEXCOORD2;
};

VS_ReflectionOUTPUT SSReflectionVS(VertexModel input)
{
    VS_ReflectionOUTPUT Output;
    SetModelWorld(World, input);

   
    float4 wPosition = WorldPosition(input.Position);
    Output.Position = ViewProjection(wPosition);
  
   
   

    Output.ViewNormal = WorldNormal(input.Normal);
    Output.ViewNormal = mul(Output.ViewNormal, (float3x3) View);
	// Transform to projected space
   //Output.Position = WorldPosition(input.Position);
   
	// Transform the position and normal to view space
    Output.ViewPosition = mul(wPosition, View);
   
   

	// Convert the projected position to clip-space
    Output.csPos = Output.Position.xyz / Output.Position.w;

    return Output;
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
  

    output.Cull.x = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[0]);
    output.Cull.y = dot(float4(output.wPosition - ViewPosition(), 1.0f), -g_FrustumNormals[1]);
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
    float Padding;

    Keyframe Curr;
    Keyframe Next;

   
};

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
};
cbuffer CB_AnimationFrame
{
    TweenFrame Tweenframes[MAX_MODEL_INSTANCE];
    
    
};

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


    [flatten]
    if (clip[0] == 0)
    {
      
	 //[unroll(4)]
        for (int i = 0; i < 4; i++)
        {
            c0 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 0, 0, 0, 0));
            c1 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 1, 0, 0, 0));
            c2 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 2, 0, 0, 0));
            c3 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 3, 0, 0, 0));
            curr = matrix(c0, c1, c2, c3);
            transform += mul(boneWeights[i], curr);
        }
     
     
      
        world = mul(transform, input.Transform);
      
        return;
    }
   
	[unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        c0 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 0, currFrame[0], clip[0], 0));
        c1 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 1, currFrame[0], clip[0], 0));
        c2 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 2, currFrame[0], clip[0], 0));
        c3 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 3, currFrame[0], clip[0], 0));
        curr = matrix(c0, c1, c2, c3);

        n0 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 0, nextFrame[0], clip[0], 0));
        n1 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 1, nextFrame[0], clip[0], 0));
        n2 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 2, nextFrame[0], clip[0], 0));
        n3 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 3, nextFrame[0], clip[0], 0));
        next = matrix(n0, n1, n2, n3);

        anim = lerp(curr, next, (matrix) Tweenframes[input.InstID].Curr.Time);
       
       
		[flatten]
        if (clip[1] >= 0)
        {
            c0 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 0, currFrame[1], clip[1], 0));
            c1 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 1, currFrame[1], clip[1], 0));
            c2 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 2, currFrame[1], clip[1], 0));
            c3 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 3, currFrame[1], clip[1], 0));
            curr = matrix(c0, c1, c2, c3);

            n0 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 0, nextFrame[1], clip[1], 0));
            n1 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 1, nextFrame[1], clip[1], 0));
            n2 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 2, nextFrame[1], clip[1], 0));
            n3 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 3, nextFrame[1], clip[1], 0));
            next = matrix(n0, n1, n2, n3);

            nextAnim = lerp(curr, next, (matrix) Tweenframes[input.InstID].Next.Time);

            anim = lerp(anim, nextAnim, Tweenframes[input.InstID].TweenTime);
        }

        transform += mul(boneWeights[i], anim);
    }

    world = mul(transform, input.Transform);
}
//ModelOutput_Lod VS_Anim_Lod(VertexModel input)
//{
//    ModelOutput_Lod output;

//    SetAnimationWorld(World, input);

  
//   // output.Position = input.Position;
//    output.Position = WorldPosition(input.Position);
 
//   // output.wPosition = output.Position.xyz;

//  // output.Position = ViewProjection(output.Position);

 
//    output.Normal = WorldNormal(input.Normal);
//    output.Tangent = WorldTangent(input.Tangent);
//    output.Uv = input.Uv;


   
//    return output;
//}




//Model_HS_CONSTANT_DATA_OUTPUT Anim_DiffuseConstantHS(InputPatch<ModelOutput_Lod, 3> inputPatch)
//{
//    Model_HS_CONSTANT_DATA_OUTPUT output;
  
//	// tessellation factors are proportional to model space edge length
//    for (uint ie = 0; ie < 3; ++ie)
//    {

//        float3 edge = inputPatch[(ie + 1) % 3].Position - inputPatch[ie].Position;
    
//        float3 vec = (inputPatch[(ie + 1) % 3].Position + inputPatch[ie].Position) / 2 - ViewPosition();
//        float d = distance(edge, vec);
//        float len = sqrt(dot(edge, edge) / dot(vec, vec));
//        int tess = lerp(0, 10, len * 5);
//        output.Edges[(ie + 1) % 3] = max(1, tess);
//        //output.Edges[(ie + 1) % 3] = TessFactorModel(edge);
     

//    }
//     // compute the cubic geometry control points
//    // edge control points
//    output.f3B210 = ((2.0f * inputPatch[0].Position.xyz) + inputPatch[1].Position.xyz - (dot((inputPatch[1].Position.xyz - inputPatch[0].Position.xyz), inputPatch[0].Normal) * inputPatch[0].Normal)) / 3.0f;
//    output.f3B120 = ((2.0f * inputPatch[1].Position.xyz) + inputPatch[0].Position.xyz - (dot((inputPatch[0].Position.xyz - inputPatch[1].Position.xyz), inputPatch[1].Normal) * inputPatch[1].Normal)) / 3.0f;
//    output.f3B021 = ((2.0f * inputPatch[1].Position.xyz) + inputPatch[2].Position.xyz - (dot((inputPatch[2].Position.xyz - inputPatch[1].Position.xyz), inputPatch[1].Normal) * inputPatch[1].Normal)) / 3.0f;
//    output.f3B012 = ((2.0f * inputPatch[2].Position.xyz) + inputPatch[1].Position.xyz - (dot((inputPatch[1].Position.xyz - inputPatch[2].Position.xyz), inputPatch[2].Normal) * inputPatch[2].Normal)) / 3.0f;
//    output.f3B102 = ((2.0f * inputPatch[2].Position.xyz) + inputPatch[0].Position.xyz - (dot((inputPatch[0].Position.xyz - inputPatch[2].Position.xyz), inputPatch[2].Normal) * inputPatch[2].Normal)) / 3.0f;
//    output.f3B201 = ((2.0f * inputPatch[0].Position.xyz) + inputPatch[2].Position.xyz - (dot((inputPatch[2].Position.xyz - inputPatch[0].Position.xyz), inputPatch[0].Normal) * inputPatch[0].Normal)) / 3.0f;
//    // center control point                                                                                                  P                
//    float3 f3E = (output.f3B210 + output.f3B120 + output.f3B021 + output.f3B012 + output.f3B102 + output.f3B201) / 6.0f;
//    float3 f3V = (inputPatch[0].Position + inputPatch[1].Position + inputPatch[2].Position) / 3.0f;
//    output.f3B111 = f3E + ((f3E - f3V) / 2.0f);

//    //output.Inside = (inputPatch[0].Position + inputPatch[1].Position + inputPatch[2].Position) /3;
//    output.Inside = (output.Edges[0] + output.Edges[1] + output.Edges[2]) / 3;

//    float2 t01 = inputPatch[1].Uv - inputPatch[0].Uv;
//    float2 t02 = inputPatch[2].Uv - inputPatch[0].Uv;
//    //output.sign = t01.x * t02.y - t01.y * t02.x > 0.0f ? 1 : -1;

//    return output;
//}
//[domain("tri")]
//[partitioning("integer")]
//[outputtopology("triangle_cw")]
//[outputcontrolpoints(3)]
//[patchconstantfunc("Anim_DiffuseConstantHS")]
//[maxtessfactor(64.0)]
//HSIn_Diffuse Anim_DiffuseHS(InputPatch<ModelOutput_Lod, 3> inputPatch, uint i : SV_OutputControlPointID)
//{
//    HSIn_Diffuse output;
//    output.position = inputPatch[i].Position;
//    output.normal = inputPatch[i].Normal;
//    output.tangent = inputPatch[i].Tangent;
//    output.texCoord = inputPatch[i].Uv;
//    return output;
//}
//[domain("tri")]
//PSIn_TessellatedDiffuse Anim_DiffuseDS(Model_HS_CONSTANT_DATA_OUTPUT input,
//                       float3 barycentricCoords : SV_DomainLocation,
//                       OutputPatch<ModelOutput_Lod, 3> inputPatch)
//{
//    PSIn_TessellatedDiffuse output;

//    float3 coordinates = barycentricCoords;

//    // The barycentric coordinates
//    float fU = barycentricCoords.x;
//    float fV = barycentricCoords.y;
//    float fW = barycentricCoords.z;

//    // Precompute squares and squares * 3 
//    float fUU = fU * fU;
//    float fVV = fV * fV;
//    float fWW = fW * fW;
//    float fUU3 = fUU * 3.0f;
//    float fVV3 = fVV * 3.0f;
//    float fWW3 = fWW * 3.0f;

//    // Compute position from cubic control points and barycentric coords
//    float3 position = inputPatch[0].Position.xyz * fWW * fW + inputPatch[1].Position.xyz * fUU * fU + inputPatch[2].Position.xyz * fVV * fV +
//					  input.f3B210 * fWW3 * fU + input.f3B120 * fW * fUU3 + input.f3B201 * fWW3 * fV + input.f3B021 * fUU3 * fV +
//					  input.f3B102 * fW * fVV3 + input.f3B012 * fU * fVV3 + input.f3B111 * 6.0f * fW * fU * fV;

//    // Compute normal from quadratic control points and barycentric coords
//    float3 normal = inputPatch[0].Normal * coordinates.z + inputPatch[1].Normal * coordinates.x + inputPatch[2].Normal * coordinates.y;
//    normal = normalize(normal);

//    float2 texCoord = inputPatch[0].Uv * coordinates.z + inputPatch[1].Uv * coordinates.x + inputPatch[2].Uv * coordinates.y;

//    float2 displacementTexCoord = texCoord;


//    float3 tangent = inputPatch[0].Tangent * coordinates.z + inputPatch[1].Tangent * coordinates.x + inputPatch[2].Tangent * coordinates.y;
//    tangent = normalize(tangent);

//    output.position = float4(position, 1.0f);
//    output.Cull.x = dot(output.position, ClipPlanes[0]);
//    output.Cull.y = dot(output.position, ClipPlanes[1]);
//    output.Cull.z = dot(output.position, ClipPlanes[2]);
//    output.Cull.w = 0;

//    output.Cull2.x = dot(output.position, ClipPlanes[3]);
//    output.Cull2.y = dot(output.position, ClipPlanes[4]);
//    output.Cull2.z = dot(output.position, ClipPlanes[5]);
//    output.Cull2.w = 0;
//  //  output.position = WorldPosition(float4(position, 1.0f));
//    output.position = ViewProjection(output.position);
//    output.texCoord = texCoord;
//    //output.position = mul(float4(position, 1.0f), g_ModelViewProjectionMatrix);
//  //  output.positionWS = position;
//    output.normal = normal;
//    output.tangent = tangent;
  

//   // output.sign = input.sign; //inputPatch[0].sign;

//    return output;
//}
MeshOutput VS_Animation(VertexModel input)
{
    MeshOutput output;

    SetAnimationWorld(World, input);

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

    matrix transform = 0;
    matrix curr = 0, currAnim = 0;
    matrix next = 0, nextAnim = 0;
	

    float boneIndices[4] = { input.BlendIndices.x, input.BlendIndices.y, input.BlendIndices.z, input.BlendIndices.w };
    float boneWeights[4] = { input.BlendWeights.x, input.BlendWeights.y, input.BlendWeights.z, input.BlendWeights.w };

    float4 c0, c1, c2, c3;
    float4 n0, n1, n2, n3;
    // Attach  Model

    if (any(input.BlendIndices) == false)
    {
        boneIndices[0] = BoneIndex;
        boneWeights[0] = 1.0f;
        
       
    }


    uint clip;
    uint currFrame;
    uint nextFrame;
    float time;
    
    
    
    clip = previewFrame.pClip;
    currFrame = previewFrame.pCurrFrame;
    nextFrame = previewFrame.pNextFrame;
    time = previewFrame.pTime;


    
	
    [unroll(4)]
    for (int i = 0; i < 4; i++)
    {
        c0 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 0, currFrame, clip, 0));
        c1 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 1, currFrame, clip, 0));
        c2 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 2, currFrame, clip, 0));
        c3 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 3, currFrame, clip, 0));
        curr = matrix(c0, c1, c2, c3);

        n0 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 0, nextFrame, clip, 0));
        n1 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 1, nextFrame, clip, 0));
        n2 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 2, nextFrame, clip, 0));
        n3 = BoneTransforms.Load(uint4(boneIndices[i] * 4 + 3, nextFrame, clip, 0));
        next = matrix(n0, n1, n2, n3);

        currAnim = lerp(curr, next, time);

        transform += mul(boneWeights[i], currAnim);
        
    }
    // Attach  Model
   
  
 
  
   
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
