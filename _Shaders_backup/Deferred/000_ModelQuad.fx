#define MAX_MODEL_INSTANCE 100
#define MAX_MODEL_TRANSFORMS 128
Texture2DArray BoneTransforms;
Texture2D TempBoneTransforms;
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

    float4 tm1 = TempBoneTransforms.Load(int3( input.InstID, 0, 0));
    float4 tm2 = TempBoneTransforms.Load(int3( input.InstID, 0, 0));
    float4 tm3 = TempBoneTransforms.Load(int3( input.InstID, 0, 0));
    float4 tm4 = TempBoneTransforms.Load(int3( input.InstID, 0, 0));

    matrix transform = matrix(m1 + tm1, m2 + tm2, m3 + tm3, m4 + tm4);
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

struct HS_CONSTANT_DATA_OUTPUT_TESS_QUADS
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
    float3 vEdgePos[8] : EDGEPOS;
    float3 vInteriorPos[4] : INTERIORPOS;
   // float sign : SIGN;


};
float TessFactorModel(float3 position)
{
    position = float3(position.x, 0.0f, position.z);
    float3 view = float3(ViewPosition().x, 0.0f, ViewPosition().z);

    float d = distance(position, view);

    float factor = saturate((d - 1) / (500 - 1));

    return lerp(3, 1, factor);
};
HS_CONSTANT_DATA_OUTPUT_TESS_QUADS DiffuseConstantTessQuadsHS(InputPatch<ModelOutput_Lod, 4> inputPatch)
{
    HS_CONSTANT_DATA_OUTPUT_TESS_QUADS output;
    
	// tessellation factors are proportional to model space edge length
    for (int ie = 0; ie < 4; ++ie)
    {

        //float3 edge = inputPatch[(ie + 1) & 3].Position.xyz - inputPatch[ie].Position.xyz;
       // float3 vec = (inputPatch[(ie + 1) & 3].Position + inputPatch[ie].Position) / 2 - g_FrustumOrigin;
       // float len = sqrt(dot(edge, edge) / dot(vec, vec));
        output.Edges[(ie + 1) & 3] =1;

    }
    output.Inside[1] = (output.Edges[0] + output.Edges[2]) / 2;
    output.Inside[0] = (output.Edges[1] + output.Edges[3]) / 2;

     //culling
    for (int ip = 0; ip < 4; ++ip)
    {
        int culled = 1;
        culled &= dot(inputPatch[0].Position.xyz - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
        culled &= dot(inputPatch[1].Position.xyz - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
        culled &= dot(inputPatch[2].Position.xyz - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
        culled &= dot(inputPatch[3].Position.xyz - ViewPosition(), g_FrustumNormals[ip].xyz) > 0;
        if (culled)
            output.Edges[ip] = 0;
    }

	// edges
    [unroll]
    for (int iedge = 0; iedge < 4; ++iedge)
    {
        int i = iedge;
        int j = (iedge + 1) & 3;
        float3 vPosTmp = 0;

        vPosTmp = inputPatch[j].Position.xyz - inputPatch[i].Position.xyz;
        output.vEdgePos[iedge * 2] = (2 * inputPatch[i].Position.xyz + inputPatch[j].Position.xyz - dot(vPosTmp, inputPatch[i].Normal) * inputPatch[i].Normal) / 3;

        i = j;
        j = iedge;
        vPosTmp = inputPatch[j].Position.xyz - inputPatch[i].Position.xyz;
        output.vEdgePos[iedge * 2 + 1] = (2 * inputPatch[i].Position.xyz + inputPatch[j].Position.xyz - dot(vPosTmp, inputPatch[i].Normal) * inputPatch[i].Normal) / 3;
    }

    // interior
    float3 q = output.vEdgePos[0];
    for (int i = 1; i < 8; ++i)
    {
        q += output.vEdgePos[i];
    }
    float3 center = inputPatch[0].Position.xyz + inputPatch[1].Position.xyz + inputPatch[2].Position.xyz + inputPatch[3].Position.xyz;

    for (i = 0; i < 4; ++i)
    {
        float3 Ei = (2 * (output.vEdgePos[i * 2] + output.vEdgePos[((i + 3) & 3) * 2 + 1] + q) - (output.vEdgePos[((i + 1) & 3) * 2 + 1] + output.vEdgePos[((i + 2) & 3) * 2])) / 18;
        float3 Vi = (center + 2 * (inputPatch[(i + 3) & 3].Position.xyz + inputPatch[(i + 1) & 3].Position.xyz) + inputPatch[(i + 2) & 3].Position.xyz) / 9;
        output.vInteriorPos[i] = 3. / 2 * Ei - 1. / 2 * Vi;
    }

    float2 t01 = inputPatch[1].Uv - inputPatch[0].Uv;
    float2 t02 = inputPatch[2].Uv - inputPatch[0].Uv;
   // output.sign = t01.x * t02.y - t01.y * t02.x > 0.0f ? 1 : -1;

    return output;
}
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("DiffuseConstantTessQuadsHS")]
[maxtessfactor(64.0)]
HSIn_Diffuse DiffuseTessQuadsHS(InputPatch<ModelOutput_Lod, 4> inputPatch,
                        uint i : SV_OutputControlPointID)
{
    HSIn_Diffuse output;
    output.position = inputPatch[i].Position;
    output.normal = inputPatch[i].Normal;
    output.tangent = inputPatch[i].Tangent;
    output.texCoord = inputPatch[i].Uv;
    return output;
  //  return inputPatch[i];
}
float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4(invT * invT * invT,
                   3.0f * t * invT * invT,
                   3.0f * t * t * invT,
                   t * t * t);
}
float4 EvaluateBezier(float3 p0, float3 p1, float3 p2, float3 p3,
                       float3 p4, float3 p5, float3 p6, float3 p7,
                       float3 p8, float3 p9, float3 p10, float3 p11,
                       float3 p12, float3 p13, float3 p14, float3 p15,
                       float4 BasisU,
                       float4 BasisV)
{
    float3 Value;
    Value = BasisV.x * (p0 * BasisU.x + p1 * BasisU.y + p2 * BasisU.z + p3 * BasisU.w);
    Value += BasisV.y * (p4 * BasisU.x + p5 * BasisU.y + p6 * BasisU.z + p7 * BasisU.w);
    Value += BasisV.z * (p8 * BasisU.x + p9 * BasisU.y + p10 * BasisU.z + p11 * BasisU.w);
    Value += BasisV.w * (p12 * BasisU.x + p13 * BasisU.y + p14 * BasisU.z + p15 * BasisU.w);
    return float4(Value, 1);
}

[domain("quad")]
PSIn_TessellatedDiffuse DiffuseTessQuadsDS(HS_CONSTANT_DATA_OUTPUT_TESS_QUADS input,
                            float2 uv : SV_DomainLocation,
                            OutputPatch<HSIn_Diffuse, 4> inputPatch)
{
    PSIn_TessellatedDiffuse output;

    float4 BasisU = BernsteinBasis(uv.x);
    float4 BasisV = BernsteinBasis(uv.y);
    output.position = EvaluateBezier(inputPatch[0].position.xyz, input.vEdgePos[0], input.vEdgePos[1], inputPatch[1].position.xyz,
                                       input.vEdgePos[7], input.vInteriorPos[0], input.vInteriorPos[1], input.vEdgePos[2],
                                       input.vEdgePos[6], input.vInteriorPos[3], input.vInteriorPos[2], input.vEdgePos[3],
                                       inputPatch[3].position.xyz, input.vEdgePos[5], input.vEdgePos[4], inputPatch[2].position.xyz,
                                       BasisU, BasisV);
    output.normal = (1 - uv.y) * (inputPatch[0].normal * (1 - uv.x) + inputPatch[1].normal * uv.x) +
                    uv.y * (inputPatch[3].normal * (1 - uv.x) + inputPatch[2].normal * uv.x);
    output.normal = normalize(output.normal);

    output.texCoord = (1 - uv.y) * (inputPatch[0].texCoord * (1 - uv.x) + inputPatch[1].texCoord * uv.x) +
                      uv.y * (inputPatch[3].texCoord * (1 - uv.x) + inputPatch[2].texCoord * uv.x);
    
    float2 displacementTexCoord = output.texCoord;

    float3 tangent = (1 - uv.y) * (inputPatch[0].tangent * (1 - uv.x) + inputPatch[1].tangent * uv.x) +
                        uv.y * (inputPatch[3].tangent * (1 - uv.x) + inputPatch[2].tangent * uv.x);



    output.tangent = tangent;
    output.position = ViewProjection(output.position);
   // output.position = mul(float4(output.position, 1.0f), g_ModelViewProjectionMatrix);

    return output;
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

    output.sPosition = WorldPosition(input.Position);
    output.sPosition = mul(output.sPosition, ShadowView);
    output.sPosition = mul(output.sPosition, ShadowProjection);
    output.Alpha = 0;
  

    output.Cull.x = dot(float4(output.wPosition, 1.0f), ClipPlanes[0]);
    output.Cull.y = dot(float4(output.wPosition, 1.0f), ClipPlanes[1]);
    output.Cull.z = dot(float4(output.wPosition, 1.0f), ClipPlanes[2]);
    output.Cull.w = 0;
    
    output.Cull2.x = dot(float4(output.wPosition, 1.0f), ClipPlanes[3]);
    output.Cull2.y = dot(float4(output.wPosition, 1.0f), ClipPlanes[4]);
    output.Cull2.z = dot(float4(output.wPosition, 1.0f), ClipPlanes[5]);
    output.Cull2.w = 0;
    
    return output;
}
MeshOutput VS_PreviewModel(VertexModel input)
{
    MeshOutput output;

    SetModelWorld(World, input);

    output.oPosition = input.Position.xyz;
    output.Position = WorldPosition(input.Position);
   
    output.wPosition = output.Position.xyz;
   

    output.Position = OrbitViewProjection(output.Position);
  
   
    output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

    output.sPosition = WorldPosition(input.Position);
    output.sPosition = mul(output.sPosition, ShadowView);
    output.sPosition = mul(output.sPosition, ShadowProjection);
    output.Alpha = 0;
  
  
    output.Cull.x = 0;
    output.Cull.y = 0;
    output.Cull.z = 0;
    output.Cull.w = 0;

    output.Cull2.x = 0;
    output.Cull2.y = 0;
    output.Cull2.z = 0;
    output.Cull2.w = 0;
    
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
   // uint g_TessellationFactor;
	float2 Padding;

	Keyframe Curr;
	Keyframe Next;

   
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


	uint clip[2];
	uint currFrame[2];
	uint nextFrame[2];

	clip[0] = Tweenframes[input.InstID].Curr.Clip;
	currFrame[0] = Tweenframes[input.InstID].Curr.CurrFrame;
	nextFrame[0] = Tweenframes[input.InstID].Curr.NextFrame;

	clip[1] = Tweenframes[input.InstID].Next.Clip;
	currFrame[1] = Tweenframes[input.InstID].Next.CurrFrame;
	nextFrame[1] = Tweenframes[input.InstID].Next.NextFrame;


	//[unroll(4)]
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

    output.sPosition = WorldPosition(input.Position);
    output.sPosition = mul(output.sPosition, ShadowView);
    output.sPosition = mul(output.sPosition, ShadowProjection);
    output.Alpha = 0;
  
   output.Cull.x = dot(float4(output.wPosition, 1.0f), ClipPlanes[0]);
   output.Cull.y = dot(float4(output.wPosition, 1.0f), ClipPlanes[1]);
   output.Cull.z = dot(float4(output.wPosition, 1.0f), ClipPlanes[2]);
   output.Cull.w = 0;

   output.Cull2.x = dot(float4(output.wPosition, 1.0f), ClipPlanes[3]);
   output.Cull2.y = dot(float4(output.wPosition, 1.0f), ClipPlanes[4]);
   output.Cull2.z = dot(float4(output.wPosition, 1.0f), ClipPlanes[5]);
   output.Cull2.w = 0;
    
	return output;
}

MeshOutput VS_PreviewAnimation(VertexModel input)
{
    MeshOutput output;

    SetAnimationWorld(World, input);

    output.oPosition = input.Position.xyz;
    output.Position = WorldPosition(input.Position);
    output.wPosition = output.Position.xyz;

    output.Position = OrbitViewProjection(output.Position);
    output.wvpPosition = output.Position;

    output.Normal = WorldNormal(input.Normal);
    output.Tangent = WorldTangent(input.Tangent);
    output.Uv = input.Uv;

    output.sPosition = WorldPosition(input.Position);
    output.sPosition = mul(output.sPosition, ShadowView);
    output.sPosition = mul(output.sPosition, ShadowProjection);
    output.Alpha = 0;
  
    output.Cull.x =0;
    output.Cull.y =0;
    output.Cull.z =0;
    output.Cull.w = 0;

    output.Cull2.x = 0;
    output.Cull2.y = 0;
    output.Cull2.z = 0;
    output.Cull2.w = 0;
    
    return output;
}

