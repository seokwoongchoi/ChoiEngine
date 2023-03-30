#include "000_Header.fx"
#include "000_Model.fx"
#include "000_Terrain.fx"
#include "pbrCommon.fx"

matrix CascadeViewProj[3];

cbuffer cbPerObjectPS // Model pixel shader constants
{
    float specExp;
    float specIntensity;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////// Mesh //////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
MeshOutput VS_Mesh_PreRender(VertexMesh input)
{
    MeshOutput output = VS_Mesh(input);
   // output.Uv *= 4;
    return output;
}
MeshOutput VS_Model_PreRender(VertexModel input)
{
    MeshOutput output = VS_Model(input);
    return output;
}
MeshOutput VS_Animation_PreRender(VertexModel input)
{
    MeshOutput output = VS_Animation(input);
    return output;
}
struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_Target0;
    float4 Specular : SV_Target1;
    float4 Normal : SV_Target2;
    float4 Bump : SV_Target3;
    
      
};
void GetBumpMapCoord(float2 uv, float3 normal, float3 tangent, SamplerState samp, inout float3 bump)
{
    float4 map = NormalMap.Sample(samp, uv);

    [flatten]
    if (any(map) == false)
        return;


    //탄젠트 공간
    float3 N = normalize(normal); //Z
    float3 T = normalize(tangent - dot(tangent, N) * N); //X
    float3 B = cross(N, T); //Y
    float3x3 TBN = float3x3(T, B, N);

    //이미지로부터 노멀벡터 가져오기
    float3 coord = map.rgb * 2.0f - 1.0f;

    //탄젠트 공간으로 변환
    coord = mul(coord, TBN);

    bump = coord;
}
PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float4 Specular, float3 Normal, float3 bump, float matallic, float roughness, float skyMask = 0, float terrainMask = 1)
{
    PS_GBUFFER_OUT Out;


    Out.ColorSpecInt = float4(BaseColor.rgb, matallic);
    Out.Specular = float4(Specular.rgb, terrainMask);
   
    Out.Normal = float4(Normal.rgb * 0.5 + 0.5, roughness);
    Out.Bump = float4(bump.rgb * 0.5 + 0.5, skyMask);
    
    return Out;
}
PS_GBUFFER_OUT RenderScenePS(MeshOutput In)
{
    // Lookup mesh texture and modulate it with diffuse
    float3 DiffuseColor = DiffuseMap.Sample(LinearSampler, In.Uv);
    float4 Specular = SpecularMap.Sample(LinearSampler, In.Uv);
    float matallic = MatallicMap.Sample(LinearSampler, In.Uv).r;
    float roughness = RoughnessMap.Sample(LinearSampler, In.Uv).r;
	//DiffuseColor *= DiffuseColor;
   // DiffuseColor += float4(0.15f, 0.15f, 0.15f, 0.0f);
    float3 bump = 0;
    GetBumpMapCoord(In.Uv, In.Normal, In.Tangent, LinearSampler, bump);
    //[flatten]
    //if (DiffuseColor.r==0)
    //    discard;
    roughness *= Mat.Roughness;
    matallic *= Mat.Matallic;
    return PackGBuffer(DiffuseColor, float4(Specular.rgb, 2), normalize(In.Normal), normalize(bump), matallic, roughness);
}




/////////////////////////////////////////////////////////////////////////////////////

Texture2D HDRTex;
cbuffer SSReflectionCB
{
   
    float ViewAngleThreshold;
    float EdgeDistThreshold;
    float DepthBias;
    float ReflectionScale;
   // float4 PerspectiveValues;
}


// Pixel size in clip-space
// This is resulotion dependent
// Pick the minimum of the HDR width and height
static const float PixelSize = 2.0 / 1024.0f;

// Number of sampling steps
// This is resulotion dependent
// Pick the maximum of the HDR width and height
static const int nNumSteps = 1280;
float3 CalcViewPos1(float2 csPos, float depth)
{
    float3 position;

    position.xy = csPos.xy * PerspectiveValues.xy * depth;
    position.z = depth;

    return position;
}
	
float4 SSReflectionPS(MeshOutput In) : SV_TARGET
{
	// Pixel position and normal in view space
    float3 vsPos = mul(float4(In.oPosition, 1), View);
    float3 vsNorm = normalize(mul(In.Normal, (float3x3) View));
 //  

	// Calculate the camera to pixel direction
    float3 eyeToPixel = normalize(vsPos);
  //  

	// Calculate the reflected view direction
    float3 vsReflect = reflect(eyeToPixel, vsNorm);

	// The initial reflection color for the pixel
    float4 reflectColor = float4(0.0, 0.0, 0.0, 0.0);

	// Don't bother with reflected vector above the threshold vector
    if (vsReflect.z >= ViewAngleThreshold)
    {
		// Fade the reflection as the view angles gets close to the threshold
        float viewAngleThresholdInv = 1.0 - ViewAngleThreshold;
        float viewAngleFade = saturate(3.0 * (vsReflect.z - ViewAngleThreshold) / viewAngleThresholdInv);

		// Transform the View Space Reflection to clip-space
        float3 vsPosReflect = vsPos + vsReflect;
        float3 csPosReflect = mul(float4(vsPosReflect, 1.0), Projection).xyz / vsPosReflect.z;
        float3 cpPos = In.Position.xyz / In.Position.w;
        float3 csReflect = csPosReflect - cpPos;

		// Resize Screen Space Reflection to an appropriate length.
        float reflectScale = PixelSize / length(csReflect.xy);
        csReflect *= reflectScale;

		// Calculate the first sampling position in screen-space
        float2 ssSampPos = (cpPos + csReflect).xy;
        ssSampPos = ssSampPos * float2(0.5, -0.5) + 0.5;

		// Find each iteration step in screen-space
        float2 ssStep = csReflect.xy * float2(0.5, -0.5);

		// Build a plane laying on the reflection vector
		// Use the eye to pixel direction to build the tangent vector
        float4 rayPlane;
        float3 vRight = cross(eyeToPixel, vsReflect);
        rayPlane.xyz = normalize(cross(vsReflect, vRight));
        rayPlane.w = dot(rayPlane.xyz, vsPos);

		// Iterate over the HDR texture searching for intersection
        for (int nCurStep = 0; nCurStep < nNumSteps; nCurStep++)
        {
			// Sample from depth buffer
            float curDepth = DepthTexture.SampleLevel(LinearSampler, ssSampPos, 0.0).x;

            float curDepthLin = ConvertZToLinearDepth(curDepth);
            float3 curPos = CalcViewPos1(cpPos.xy + csReflect.xy * ((float) nCurStep + 1.0), curDepthLin);

			// Find the intersection between the ray and the scene
			// The intersection happens between two positions on the oposite sides of the plane
            if (rayPlane.w >= dot(rayPlane.xyz, curPos) + DepthBias)
            {
				// Calculate the actual position on the ray for the given depth value
                float3 vsFinalPos = vsPos + (vsReflect / abs(vsReflect.z)) * abs(curDepthLin - vsPos.z + DepthBias);
                float2 csFinalPos = vsFinalPos.xy / PerspectiveValues.xy / vsFinalPos.z;
                ssSampPos = csFinalPos.xy * float2(0.5, -0.5) + 0.5;

				// Get the HDR value at the current screen space location
                reflectColor.xyz = HDRTex.SampleLevel(PointSampler, ssSampPos, Mat.Roughness).xyz;

				// Fade out samples as they get close to the texture edges
                float edgeFade = saturate(distance(ssSampPos, float2(0.5, 0.5)) * 2.0 - EdgeDistThreshold);

				// Calculate the fade value
                reflectColor.w = min(viewAngleFade, 1.0 - edgeFade * edgeFade);

				// Apply the reflection sacle
                reflectColor.w *= ReflectionScale;

				// Advance past the final iteration to break the loop
                nCurStep = nNumSteps;
            }

			// Advance to the next sample
            ssSampPos += ssStep;
        }
    }
    
    return reflectColor;
}










//////////////////////////////// Sky ////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
cbuffer CB_Scatter
{
    float3 WaveLength;
    int SampleCount;

    float3 InvWaveLength;
    float StarIntensity;

    float3 WaveLengthMie;
    float MoonAlpha;
};
struct VertexOutput_Dome
{
    float4 Position : SV_Position0;
    float3 oPosition : Position1;
    float2 Uv : Uv0;
};
float GetRayleighPhase(float c)
{
    return 0.75f * (1.0f + c);
}
float GetMiePhase(float c, float c2)
{
    float g = -0.980f;
    float g2 = -0.980f * -0.980f;
   
    float3 result = 0;
    result.x = 1.5f * ((1.0f - g2) / (2.0f + g2));
    result.y = 1.0f + g2;
    result.z = 2.0f * g;

    return result.x * (1.0f + c2) / pow(result.y - result.z * c, 1.5f);
}
float3 HDR(float3 LDR)
{
    float Exposure = -2.0f; //노출도
    
    return 1.0f - exp(Exposure * LDR);
}
float HitOuterSphere(float3 position, float3 direction)
{
    
    float OuterRadius = 6356.7523142f * 1.0157313f;
    float3 light = -position;

    float b = dot(light, direction);
    float c = dot(light, light);

    float d = c - b * b;
    float q = sqrt(OuterRadius * OuterRadius - d);

    return b + q;
}
float2 GetDensityRatio(float height)
{
    float InnerRadius = 6356.7523142f;
    float2 RayleighMieScaleHeight = { 0.25f, 0.1f };
    float Scale = 1.0 / (6356.7523142 * 1.0157313 - 6356.7523142);
    float altitude = (height - InnerRadius) * Scale;

    return exp(-altitude / RayleighMieScaleHeight);
}
float2 GetDistance(float3 p1, float3 p2)
{
    float2 RayleighMieScaleHeight = { 0.25f, 0.1f };
    float Scale = 1.0 / (6356.7523142 * 1.0157313 - 6356.7523142);
    float2 opticalDepth = 0;

    float3 temp = p2 - p1;
    float far = length(temp);
    float3 direction = temp / far;


    float sampleLength = far / SampleCount;
    float scaledLength = sampleLength * Scale;

    float3 sampleRay = direction * sampleLength;
    p1 += sampleRay * 0.5f;

    for (int i = 0; i < SampleCount; i++)
    {
        float height = length(p1);
        opticalDepth += GetDensityRatio(height);

        p1 += sampleRay;
    }

    return opticalDepth * scaledLength;
}
void CalcMieRay(inout float3 rayleigh, inout float3 mie, float2 uv)
{
    float PI = 3.14159265f;
    float InnerRadius = 6356.7523142f;
    float OuterRadius = 6356.7523142f * 1.0157313f;
    float KrESun = 0.0025f * 20.0f; //0.0025f - 레일리 상수 * 태양의 밝기
    float KmESun = 0.0010f * 20.0f; //0.0025f - 미 상수 * 태양의 밝기
    float Kr4PI = 0.0025f * 4.0f * 3.1415159;
    float Km4PI = 0.0010f * 4.0f * 3.1415159;
    float2 RayleighMieScaleHeight = { 0.25f, 0.1f };
    float Scale = 1.0 / (6356.7523142 * 1.0157313 - 6356.7523142);

    float3 sunDirection = -normalize(GlobalLight.Direction);

    float3 pointPv = float3(0, InnerRadius + 1e-3f, 0.0f);
    float angleXZ = PI * uv.y;
    float angleY = 100.0f * uv.x * PI / 180.0f;

    float3 direction;
    direction.x = sin(angleY) * cos(angleXZ);
    direction.y = cos(angleY);
    direction.z = sin(angleY) * sin(angleXZ);
    direction = normalize(direction);

    float farPvPa = HitOuterSphere(pointPv, direction);
    float3 ray = direction;

    float3 pointP = pointPv;
    float sampleLength = farPvPa / SampleCount;
    float scaledLength = sampleLength * Scale;
    float3 sampleRay = ray * sampleLength;
    pointP += sampleRay * 0.5f;

    float3 rayleighSum = 0;
    float3 mieSum = 0;
    float3 attenuation = 0;
    for (int i = 0; i < SampleCount; i++)
    {
        float pHeight = length(pointP);

        float2 densityRatio = GetDensityRatio(pHeight);
        densityRatio *= scaledLength;


        float2 viewerOpticalDepth = GetDistance(pointP, pointPv);

        float farPPc = HitOuterSphere(pointP, sunDirection);
        float2 sunOpticalDepth = GetDistance(pointP, pointP + sunDirection * farPPc);

        float2 opticalDepthP = sunOpticalDepth.xy + viewerOpticalDepth.xy;
         attenuation = exp(-Kr4PI * InvWaveLength * opticalDepthP.x - Km4PI * opticalDepthP.y);

        rayleighSum += densityRatio.x * attenuation;
        mieSum += densityRatio.y * attenuation;

        pointP += sampleRay;
    }

    float3 rayleigh1 = rayleighSum * KrESun;
    float3 mie1 = mieSum * KmESun;

    
   
  
    rayleigh = rayleigh1 * InvWaveLength;
    mie = mie1 * WaveLengthMie;
   
   

}
VertexOutput_Dome VS_Dome(VertexTexture input)
{
    VertexOutput_Dome output;

    output.Position = WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);

    output.oPosition = -input.Position;
    output.Uv = input.Uv;

    return output;
}
Texture2D StarMap;
PS_GBUFFER_OUT PS_Dome(VertexOutput_Dome input) 
{
    float3 sunDirection = -normalize(GlobalLight.Direction);

    float temp = dot(sunDirection, input.oPosition) / length(input.oPosition);
    float temp2 = temp * temp;

    float3 ray = 0;
    float3 mie = 0;
  
    CalcMieRay(ray, mie, input.Uv);

    float3 rSamples = ray;
    float3 mSamples = mie;

 
    float3 color = 0;
    color = GetRayleighPhase(temp2) * rSamples * 0.5f + GetMiePhase(temp, temp2) * mSamples * 0.5f;
    color = HDR(color);

    color += max(0, (1 - color.rgb)) * float3(0.05f, 0.05f, 0.1f);
    float4 finalColor = float4(color,1) + StarMap.Sample(LinearSampler, input.Uv) * saturate(StarIntensity);
    return PackGBuffer(finalColor.rgb, float4(1, 1, 1, 1), float3(1, 1, 1), float3(1, 1, 1), -1, -1, 1);

}
struct VertexOutput_Moon
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
};
///////////////////////////////////////////////////////////////////////////////
cbuffer CB_Cloud
{
    float CloudTiles;
    float CloudCover ;
    float CloudSharpness;
    float CloudSpeed;
    float2 FirstOffset;
    float2 SecondOffset;
};
struct VertexOutput_Cloud
{
    float4 Position : SV_Position0;
    float2 Uv : Uv0;
    float2 oUv : Uv1;
};
float Fade(float t)
{
  //return t * t * (3.0 - 2.0 * t);
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
VertexOutput_Moon VS_Moon(VertexTexture input)
{
    VertexOutput_Moon output;

    output.Position = WorldPosition(input.Position);
    output.Position = ViewProjection(output.Position);
    output.Uv = input.Uv;

    return output;
}
Texture2D MoonMap;
PS_GBUFFER_OUT PS_Moon(VertexOutput_Moon input)
{
    float4 color = MoonMap.Sample(LinearSampler, input.Uv);
   // color.a *= MoonAlpha;

    return PackGBuffer(color.rgb, float4(1, 1, 1, 1), float3(1, 1, 1), float3(1, 1, 1), -1, -1, 1);
}
///////////////////////////////////////////////////////////////////////////////
texture2D CloudMap;
float Noise(float2 P)
{
    float ONE = 0.00390625;
    float ONEHALF = 0.001953125;

    float2 Pi = ONE * floor(P) + ONEHALF;
    float2 Pf = frac(P);

   
    float2 grad00 = CloudMap.Sample(LinearSampler, Pi).rg * 4.0 - 1.0;
    float n00 = dot(grad00, Pf);

    float2 grad10 = CloudMap.Sample(LinearSampler, Pi + float2(ONE, 0.0)).rg * 4.0 - 1.0;
    float n10 = dot(grad10, Pf - float2(1.0, 0.0));

    float2 grad01 = CloudMap.Sample(LinearSampler, Pi + float2(0.0, ONE)).rg * 4.0 - 1.0;
    float n01 = dot(grad01, Pf - float2(0.0, 1.0));

    float2 grad11 = CloudMap.Sample(LinearSampler, Pi + float2(ONE, ONE)).rg * 4.0 - 1.0;
    float n11 = dot(grad11, Pf - float2(1.0, 1.0));

    float2 n_x = lerp(float2(n00, n01), float2(n10, n11), Fade(Pf.x));

    float n_xy = lerp(n_x.x, n_x.y, Fade(Pf.y));

    return n_xy;
}
texture2D Cloud1;
texture2D Cloud2;
float4 MakeCloudUseTexture(float2 uv)
{
    float2 sampleLocation;
    float4 textureColor1;
    float4 textureColor2;
    float4 finalColor;
    

    // Translate the position where we sample the pixel from using the first texture translation values.
    sampleLocation.x = uv.x + FirstOffset.x;
    sampleLocation.y = uv.y + FirstOffset.y;

    // Sample the pixel color from the first cloud texture using the sampler at this texture coordinate location.
    textureColor1 = Cloud1.Sample(LinearSampler, sampleLocation);
    
    // Translate the position where we sample the pixel from using the second texture translation values.
    sampleLocation.x = uv.x + SecondOffset.x;
    sampleLocation.y = uv.y + SecondOffset.y;

    // Sample the pixel color from the second cloud texture using the sampler at this texture coordinate location.
    textureColor2 = Cloud2.Sample(LinearSampler, sampleLocation);

    // Combine the two cloud textures evenly.
    finalColor = lerp(textureColor1, textureColor2, 0.5f);

    // Reduce brightness of the combined cloud textures by the input brightness value.
    finalColor = finalColor * 0.3f;

    return finalColor;
}
VertexOutput_Cloud VS_Cloud(VertexTexture input)
{
    VertexOutput_Cloud output;

    output.Position = mul(input.Position, World);
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);

    output.Uv = (input.Uv * CloudTiles);
    output.oUv = input.Uv;

    return output;
}
PS_GBUFFER_OUT PS_Cloud(VertexOutput_Cloud input)
{
    //input.Uv = input.Uv * CloudTiles;

    float n = Noise(input.Uv + Time * CloudSpeed);
    float n2 = Noise(input.Uv * 2 + Time * CloudSpeed);
    float n3 = Noise(input.Uv * 4 + Time * CloudSpeed);
    float n4 = Noise(input.Uv * 8 + Time * CloudSpeed);
 
    float nFinal = n + (n2 / 2) + (n3 / 4) + (n4 / 8);
 
    float c = CloudCover - nFinal;
    if (c < 0) 
        c = 0;

    float density = 1.0 - pow(CloudSharpness, c);
  // float4 color = density;
    input.Uv = input.Uv + Time * CloudSpeed;
    float4 color = MakeCloudUseTexture(input.Uv) + density;
  
    return PackGBuffer(color.rgb, float4(1, 1, 1, 1), float3(1, 1, 1), float3(1, 1, 1), -1, -1, 1);
}












////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////    Terrain     ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
ConstantHullOutput_Lod HS_Constant(InputPatch<VertexOutput_Lod, 4> input)
{

    float minY = input[0].BoundsY.x;
    float maxY = input[0].BoundsY.y;

    float3 vMin = float3(input[2].Position.x, minY, input[2].Position.z); //0이하단 2 우상단
    float3 vMax = float3(input[1].Position.x, maxY, input[1].Position.z); //0이하단 2 우상단
    

    float3 boxCenter = (vMin + vMax) * 0.5f;
    float3 boxExtents = (vMax - vMin) * 0.5f;
    //어떻게 짜를지 정함
    ConstantHullOutput_Lod output;
    [flatten]
    if (ContainFrustumCube(boxCenter, boxExtents))
    {
        output.Edge[0] = 0;
        output.Edge[1] = 0;
        output.Edge[2] = 0;
        output.Edge[3] = 0;

        output.Inside[0] = 0;
        output.Inside[1] = 0;

        return output;
    }

    float3 e0 = (input[0].Position + input[2].Position).xyz * 0.5f;
    float3 e1 = (input[0].Position + input[1].Position).xyz * 0.5f;
    float3 e2 = (input[1].Position + input[3].Position).xyz * 0.5f;
    float3 e3 = (input[2].Position + input[3].Position).xyz * 0.5f;

    float3 center = (input[0].Position + input[1].Position + input[2].Position + input[3].Position).xyz * 0.25;
    
    output.Edge[0] = TessFactor(e0); //이 선을 2개로 분할한다.
    output.Edge[1] = TessFactor(e1);
    output.Edge[2] = TessFactor(e2);
    output.Edge[3] = TessFactor(e3);

    output.Inside[0] = TessFactor(center);
    output.Inside[1] = TessFactor(center);

    return output;
}
struct HullOutput
{
    float4 Position : Position0;
};
[domain("quad")] //어떤단계로 영역을 묶어서 출력할것인가
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("HS_Constant")]
[maxtessfactor(64)]
HullOutput_Lod HS(InputPatch<VertexOutput_Lod, 4> input, uint point0 : SV_OutputControlPointID)
{
    //짜르기전에 할 행동
    HullOutput_Lod output;
    output.Position = input[point0].Position;
    output.Uv = input[point0].Uv;
    return output;
}
[domain("quad")]
DomainOutput_Lod DS(ConstantHullOutput_Lod input, float2 uv : SV_DomainLocation, const OutputPatch<HullOutput_Lod, 4> patch)
{
    DomainOutput_Lod output;
    
    float3 v0 = lerp(patch[0].Position.xyz, patch[1].Position.xyz, uv.x);
    float3 v1 = lerp(patch[2].Position.xyz, patch[3].Position.xyz, uv.x);
    float3 position = lerp(v0, v1, uv.y); //베지어

    output.wPosition = position;
    
    float2 uv0 = lerp(patch[0].Uv, patch[1].Uv, uv.x);
    float2 uv1 = lerp(patch[2].Uv, patch[3].Uv, uv.x);
    output.Uv = lerp(uv0, uv1, uv.y);
    float3 displacement = 0;
    output.TiledUv = output.Uv * TexScale;
    displacement = displaymentMap.SampleLevel(LinearSampler, output.TiledUv, 0).xyz*3.6f ;
   
    output.wPosition.y += LodHeightMap.SampleLevel(LinearSampler, output.Uv, 0) * HeightRatio1;
    output.wPosition.y += displacement;
    //z-> lod값 해당 위치의픽셀을 얻어올수있다.
    output.Position = ViewProjection(float4(output.wPosition, 1.0f));
   
  

    return output;

}
PS_GBUFFER_OUT PS(DomainOutput_Lod input)
{
   
    float2 left = input.Uv+ float2(-TexelCellSpaceU, 0.0);
    float2 right = input.Uv + float2(+TexelCellSpaceU, 0.0);
    float2 top = input.Uv + float2(0.0f, -TexelCellSpaceV);
    float2 bottom = input.Uv + float2(0.0f, TexelCellSpaceV);

    float leftY = LodHeightMap.SampleLevel(LinearSampler, left, 0).b * HeightRatio1;
    float rightY = LodHeightMap.SampleLevel(LinearSampler, right, 0).b * HeightRatio1;
    float topY = LodHeightMap.SampleLevel(LinearSampler, top, 0).b * HeightRatio1;
    float bottomY = LodHeightMap.SampleLevel(LinearSampler, bottom, 0).b * HeightRatio1;

    float2 dleft = input.TiledUv + float2(-TexelCellSpaceU, 0.0);
    float2 dright = input.TiledUv + float2(+TexelCellSpaceU, 0.0);
    float2 dtop = input.TiledUv + float2(0.0f, -TexelCellSpaceV);
    float2 dbottom = input.TiledUv + float2(0.0f, TexelCellSpaceV);

    leftY += displaymentMap.SampleLevel(LinearSampler, dleft, 0).r * 3.6f;
    rightY += displaymentMap.SampleLevel(LinearSampler, dright, 0).r * 3.6f;
    topY += displaymentMap.SampleLevel(LinearSampler, dtop, 0).r * 3.6f;
    bottomY += displaymentMap.SampleLevel(LinearSampler, dbottom, 0).r * 3.6f;
    //normal 구하기용

    float3 tangent = normalize(float3(WorldCellSpace * 2, rightY - leftY, 0.0f)); //사각형한칸의크기
    //tangent는 x축에맵핑되고 방향을 왼쪽과 오른쪽의 높이차를 구하고 
    //normal 은 z방향
    float3 biTangent = normalize(float3(0.0f, bottomY - topY, WorldCellSpace * -2));
    float3 normal = cross(tangent, biTangent); //정규직교식;

    //////////////////////////////////////////////////////////////////////////
        
       // Lookup mesh texture and modulate it with diffuse
   
    float2 Uv = input.TiledUv;
    //float2 Uv = input.Uv;

    float4 DiffuseColor = BaseMap.Sample(LinearSampler, Uv);
    float4 Specular = TerrainSpecMap.Sample(LinearSampler, Uv);
    float4 map = NormalMap.Sample(LinearSampler, Uv);
    float roughness = TerrainRoughMap.Sample(LinearSampler, Uv).r;
    float3x3 TBN = float3x3(tangent, biTangent, normal);

    //이미지로부터 노멀벡터 가져오기
    float3 coord = map.rgb * 2.0f - 1.0f;
    float3 bump = 0;
    //탄젠트 공간으로 변환
    bump = mul(coord, TBN);
    
  
    return PackGBuffer(DiffuseColor.rgb, float4(Specular.rgb, 0), normalize(normal), normalize(bump), 0, roughness, 0,0);
   
  
   
}












/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////   CascadedShadow   /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

float4 tempVS(float4 Pos : POSITION) : SV_Position0
{
    return Pos;
}
struct GS_OUTPUT
{
    float4 Pos : SV_POSITION;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(9)]
void CascadedShadowMapsGenGS(triangle MeshOutput InPos[3] : SV_Position, inout TriangleStream<GS_OUTPUT> OutStream)
{
    for (int iFace = 0; iFace < 3; iFace++)
    {
        GS_OUTPUT output;

        output.RTIndex = iFace;

        for (int v = 0; v < 3; v++)
        {
            output.Pos = mul(float4(InPos[v].wPosition, 1.0f), CascadeViewProj[iFace]);
            OutStream.Append(output);
        }
        OutStream.RestartStrip();
    }
}











/////////////////////////////////////////////////////////////////////////////
// directlight shader input/output structure
/////////////////////////////////////////////////////////////////////////////

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
	float2 cpPos	: Uv0;
    float2 Uv : Uv1;
   
};
VS_OUTPUT DirLightVS( uint VertexID : SV_VertexID )
{
	VS_OUTPUT Output;

	Output.Position = float4( arrBasePos[VertexID].xy, 0.0, 1.0);
    Output.cpPos = Output.Position.xy;
    Output.Uv = arrUV[VertexID].xy;
 
    //float2(Output.Position.x, -Output.Position.y) * 0.5f + 0.5f;
    //

	return Output;    
}
float CascadedShadow(float3 position)
{
	// Transform the world position to shadow space
    float4 posShadowSpace = mul(float4(position, 1.0), GlobalLight.ToShadowSpace);

	// Transform the shadow space position into each cascade position
    float4 posCascadeSpaceX = (GlobalLight.ToCascadeOffsetX + posShadowSpace.xxxx) * GlobalLight.ToCascadeScale;
    float4 posCascadeSpaceY = (GlobalLight.ToCascadeOffsetY + posShadowSpace.yyyy) * GlobalLight.ToCascadeScale;
   
	// Check which cascade we are in
    float4 inCascadeX = abs(posCascadeSpaceX) <= 1.0;
    float4 inCascadeY = abs(posCascadeSpaceY) <= 1.0;
    float4 inCascade = inCascadeX * inCascadeY;

	// Prepare a mask for the highest quality cascade the position is in
    float4 bestCascadeMask = inCascade;
    bestCascadeMask.yzw = (1.0 - bestCascadeMask.x) * bestCascadeMask.yzw;
    bestCascadeMask.zw = (1.0 - bestCascadeMask.y) * bestCascadeMask.zw;
    bestCascadeMask.w = (1.0 - bestCascadeMask.z) * bestCascadeMask.w;
    float bestCascade = dot(bestCascadeMask, float4(0.0, 1.0, 2.0, 3.0));

	// Pick the position in the selected cascade
    float3 UVD;
    UVD.x = dot(posCascadeSpaceX, bestCascadeMask);
    UVD.y = dot(posCascadeSpaceY, bestCascadeMask);
    UVD.z = posShadowSpace.z;
  //  UVD.z -= GlobalLight.Bias;
	// Convert to shadow map UV values
    UVD.xy = 0.5 * UVD.xy + 0.5;
    UVD.y = 1.0 - UVD.y;

	// Compute the hardware PCF value
  float shadow = CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(UVD.xy, bestCascade), UVD.z);
	// set the shadow to one (fully lit) for positions with no cascade coverage
 shadow = saturate(shadow + 1.0 - any(bestCascadeMask));

    //blur
    float2 size = 1.0f / float2(1280, 720);
    float2 offsets[] =
    {
        float2(+2 * size.x, -2 * size.y), float2(+size.x, -2 * size.y), float2(0.0f, -2 * size.y), float2(-size.x, -2 * size.y), float2(-2 * size.x, -2 * size.y),
            float2(+2 * size.x, -size.y), float2(+size.x, -size.y), float2(0.0f, -size.y), float2(-size.x, -size.y), float2(-2 * size.x, -size.y),
            float2(+2 * size.x, 0.0f), float2(+size.x, 0.0f), float2(0.0f, 0.0f), float2(-size.x, 0.0f), float2(-2 * size.x, 0.0f),
            float2(+2 * size.x, +size.y), float2(+size.x, +size.y), float2(0.0f, +size.y), float2(-size.x, +size.y), float2(-2 * size.x, +size.y),
            float2(+2 * size.x, +2 * size.y), float2(+size.x, +2 * size.y), float2(0.0f, +2 * size.y), float2(-size.x, +2 * size.y), float2(-2 * size.x, +2 * size.y),

    };
    float weight[] =
    {
        1, 1, 2, 1, 1,
            1, 2, 4, 2, 1,
            2, 4, 8, 4, 2,
            1, 2, 4, 2, 1,
            1, 1, 2, 1, 1,
    };


    float sum = 0.0f;
    float totalweight = 0.0f;

    float3 uv = 0.0f;
    

   [unroll(9)]
    for (int i = 0; i < 25; i++)

    {
        uv = float3(UVD.xy + offsets[i], bestCascade);
        totalweight += weight[i];

      
        sum += CascadeShadowMapTexture.SampleCmpLevelZero(PCFSampler, float3(uv), UVD.z) * weight[i];

    }
    float factor = sum / totalweight;
  
    factor = saturate(factor + UVD.z);
  
   
    return factor;

}
float FresnelSchlick(float f0, float fd90, float view)
{
    return f0 + (fd90 - f0) * pow(max(1.0f - view, 0.1f), 5.0f);
}
float Disney(float NdotL, float LdotH, float NdotV,float roughness)
{
   
    float energyBias = lerp(0.0f, 0.5f, roughness);
    float energyFactor = lerp(1.0f, 1.0f / 1.51f, roughness);
    float fd90 = energyBias + 2.0f * (LdotH * LdotH) * roughness;
    float f0 = 1.0f;

    float lightScatter = FresnelSchlick(f0, fd90, NdotL);
    float viewScatter = FresnelSchlick(f0, fd90, NdotV);

    return lightScatter * viewScatter * energyFactor;
}
float3 GGX(float NdotL, float NdotH, float NdotV, float VdotH, float LdotH,float alpha, float3 realSpecularColor)
{
    float PI = 3.14159265f;
   
    
    float rough4 = alpha * alpha;

    float ndoth = max(NdotH, 0.0f);
    float NdotH2 = ndoth * ndoth;
    float nom = rough4;
    float denom = (NdotH2 * (rough4 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

  
   // float d = (NdotH * rough4 - NdotH) * NdotH + 1.0f;
   // float D = rough4 / (PI * (d * d));
    float D = nom / denom;
    float vdoth = max(VdotH, 0.0f);
   
	// Fresnel

   
    float3 reflectivity = realSpecularColor;
    float fresnel = 1.0;
   
    //float3 F = reflectivity + (fresnel - fresnel * reflectivity) * exp2((-5.55473f * LdotH - 6.98316f) * LdotH);
    float3 F = (reflectivity + (1.0f - reflectivity) * pow(1.0 - VdotH, 5.0f));
	// geometric / visibility
    //float k = alpha * 0.5f;
    //float G_SmithL = NdotL * (1.0f - k) + k;
    //float G_SmithV = NdotV * (1.0f - k) + k;
    //float G = 0.25f / (G_SmithL * G_SmithV);
   
    float ndotv = max(NdotV, 0.0f);
    float ndotl = max(NdotL, 0.0f);

    float r = alpha + 1.0f;
    float k = (r * r) / 8.0f;

    float nom1 = NdotV;
    float denom1 = NdotV * (1.0f - k) + k;
    float ggx1 = nom1 / denom1;

    float nom2 = NdotL;
    float denom2 = NdotL * (1.0f - k) + k;
    float ggx2 = nom2 / denom2;
   
    
    float G= ggx1 * ggx2;
    return G*D*F;
   
}
float3 FinalGamma(float3 color)
{
    return pow(color, 1.0f / 2.2);
}
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)   // cosTheta is n.v and F0 is the base reflectivity
{
    return F0 + (max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0f);
}
float3 RadianceIBLIntegration(float NdotV, float roughness, float3 specular)
{
    float2 preintegratedFG = PreintegratedFG.Sample(LinearSampler, float2(roughness, 1.0 - NdotV)).rg;
    return specular*preintegratedFG.r + preintegratedFG.g;
}

float3 IBL(Material material, float3 eye,float3 KS)
{
    
    float NdotV = saturate(dot(material.normal, eye));

    float3 reflectionVector = normalize(reflect(-eye, material.normal));
    float smoothness = 1.0f - material.roughness;
    float mipLevel = (1.0 - smoothness * smoothness) * 10.0;
    float4 cs = skyIR.SampleLevel(LinearSampler, reflectionVector, mipLevel);
  
    float3 result = pow(cs.xyz, 2.2f) * RadianceIBLIntegration(NdotV, material.roughness, material.Specular);

    float3 diffuseDominantDirection = material.normal;
    float diffuseLowMip = 4.6;
    float3 diffuseImageLighting = skyIR.SampleLevel(LinearSampler, diffuseDominantDirection, diffuseLowMip).rgb;
 //   textureLod(u_EnvironmentMap, diffuseDominantDirection, diffuseLowMip).rgb;
    diffuseImageLighting = pow(diffuseImageLighting, 2.2f);
    float3 g_diffuse = pow(material.diffuseColor, 2.2f);
     return result + diffuseImageLighting * g_diffuse;
}
float4 CalcDirectionalPBR(float3 position, Material material)
{
    float PI = 3.14159265f;
  //calc the half vector
    float3 viewDir = EyePosition() - position;
    float3 lightDir =-GlobalLight.Direction;
    float3 H = normalize(viewDir + lightDir);
    float3 R = normalize(reflect(-viewDir, material.normal));
    //calc all the dot products we need
    float NdotL = saturate(dot(material.Bump, lightDir));
    float NdotH = saturate(dot(material.Bump, H));
   // float NdotH = saturate(dot(R, normalize(viewDir)));
    float NdotV = saturate(dot(material.normal, viewDir));
    float VdotH = saturate(dot(viewDir, H));
    float LdotH = saturate(dot(lightDir, H));
    float alpha = max(0.001f, material.roughness * material.roughness);
   //apply gamma correction for the colors
    float3 g_diffuse =  pow(material.diffuseColor, 2.2);
    float3 g_lightColor = GlobalLight.Specular;
 // Lerp with metallic value to find the good diffuse and specular. 
    float3 realAlbedo = g_diffuse - (g_diffuse * material.matallic);
    realAlbedo = saturate(realAlbedo);
    float3 realSpecularColor = lerp(0.03f, g_diffuse, material.matallic);
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, g_diffuse, material.matallic);
 //calculate the diffuse and specular components
    float3 diffuse = 0;
    
    float3 specular = 0;
    float3 finalColor = 0;
    //Sky
    float3 kS = FresnelSchlickRoughness(1-NdotV, F0, material.roughness);
    //float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    //kD *= 1.0 - material.matallic;

    //float3 irradiance = skyIR.Sample(LinearSampler, material.normal).rgb;
    ////diffuse *= irradiance;
    //const float MAX_REF_LOD = 11.0f;
    //float3 prefilteredColor = skyIR.SampleLevel(LinearSampler, R, material.roughness * MAX_REF_LOD).rgb;
    //float2 brdf = PreintegratedFG.Sample(LinearSampler, float2(1 - NdotV, material.roughness)).rg;
    //float3 specular = prefilteredColor * (kS * brdf.x + brdf.y);
   
 //final result
    diffuse += NdotL * Disney(NdotL, LdotH, NdotV, alpha) ;
    specular += NdotL * GGX(NdotL, NdotH, NdotV, VdotH, LdotH, alpha, F0);
//Add to outgoing radiance Lo
    float shadowAtt = CascadedShadow(position);
  
  
    finalColor = (realAlbedo * diffuse.rgb) + ((IBL(material, viewDir, kS) * material.TerrainMask) + specular.rgb);
   
   // finalColor *= kS;
    //+ IBL(material, viewDir)
    finalColor = FinalGamma(finalColor) * shadowAtt;
   
    return float4(finalColor, 1.0f);
}
float3 GammaToLinear(float3 color)
{
    return float3(color.x * color.x, color.y * color.y, color.z * color.z);

}
float4 DirLightPS(VS_OUTPUT In) : SV_TARGET
{
    // Unpack the GBuffer
	SURFACE_DATA gbd = UnpackGBuffer_Loc(In.Position.xy);
	// Convert the data into the material structure
	Material mat;
	MaterialFromGBuffer(gbd, mat);
     [flatten]
    if (mat.SkyMask > 0)
    {
        return mat.diffuseColor;
    }
  
	// Reconstruct the world position
	float3 position = CalcWorldPos(In.cpPos, gbd.LinearDepth);
    float ao = SsaoTexture.Sample(LinearSampler, In.Uv);
	float3 g_vAmbientLowerColor = GammaToLinear(float3(0.4f, 0.4f, 0.4f));
    float3 g_vAmbientUpperColor = GammaToLinear(float3(0.53f, 0.53f, 0.6f));
    float3 AmbientRange = g_vAmbientUpperColor - g_vAmbientLowerColor;
   
    // Normalize the vertical axis
    float up = mat.Bump.y * 0.5 + 0.5;

	// Calcualte the ambient light
    float3 ambient = g_vAmbientLowerColor + up * AmbientRange;
    ambient *= mat.diffuseColor.rgb;
    ambient *= g_vAmbientUpperColor;
    ambient *= ao;
  
    float3 aox3 = float3(ao, ao, ao);
    float3 finalColor = CalcDirectionalPBR(position,mat).rgb;
    finalColor+= ambient;
    finalColor *= ao;
    float3 eyeToPixel = position - EyePosition();
    finalColor = ApplyFog(finalColor, EyePosition().y, eyeToPixel);
      
    return float4(finalColor, 1.0f);
}


float4 DirCubeMap(MeshOutput input) : SV_TARGET
{
   return DiffuseMap.Sample(LinearSampler, input.Uv);
}











/////////////////////////////////////////////////////////////////////////////
//Point Vertex shader
/////////////////////////////////////////////////////////////////////////////
float4 PointLightVS() :SV_Position0
{
    
    return float4(0, 0, 0, 1.0f);
}
/////////////////////////////////////////////////////////////////////////////
// Point Hull shader
/////////////////////////////////////////////////////////////////////////////
struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
  
};
HS_CONSTANT_DATA_OUTPUT PointLightConstantHS()
{
    HS_CONSTANT_DATA_OUTPUT Output;
	
    float tessFactor = 18.0;
    Output.Edges[0] = Output.Edges[1] = Output.Edges[2] = Output.Edges[3] = tessFactor;
    Output.Inside[0] = Output.Inside[1] = tessFactor;
  
    return Output;
}
struct HS_OUTPUT
{
    float4 HemiDir : POSITION;
   
};
static const float3 HemilDir[2] =
{
    float3(1.0, 1.0, 1.0),
    float3(-1.0, 1.0, -1.0)
};
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("PointLightConstantHS")]
[maxtessfactor(64)]
HS_OUTPUT PointLightHS( uint PatchID : SV_PrimitiveID)
{
    HS_OUTPUT Output;

    Output.HemiDir = float4(HemilDir[PatchID], 0.0f);
   
    return Output;
}
/////////////////////////////////////////////////////////////////////////////
// Domain Shader shader
/////////////////////////////////////////////////////////////////////////////
struct DS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 cpPos : Uv0;
   
   
};
[domain("quad")]
DS_OUTPUT PointLightDS(HS_CONSTANT_DATA_OUTPUT input, float2 UV : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 4> quad)
{
	// Transform the UV's into clip-space
    float2 posClipSpace = UV.xy * 2.0 - 1.0;

	// Find the absulate maximum distance from the center
    float2 posClipSpaceAbs = abs(posClipSpace.xy);
    float maxLen = max(posClipSpaceAbs.x, posClipSpaceAbs.y);

	// Generate the final position in clip-space
    float3 normDir = normalize(float3(posClipSpace.xy, (maxLen - 1.0)) * quad[0].HemiDir.xyz);
    float4 posLS = float4(normDir.xyz, 1.0);
	
	// Transform all the way to projected space
    DS_OUTPUT Output;
   
    
   // Output.Position = mul(posLS, LightProjection[0]);
    Output.Position = mul(posLS, LightProjection);
    
    // Store the clip space position
    Output.cpPos = Output.Position.xy / Output.Position.w;
    
     
    return Output;
}
/////////////////////////////////////////////////////////////////////////////
// Pixel shader
/////////////////////////////////////////////////////////////////////////////
float3 CalcPoint(float3 position, Material material, bool bUseShadow)
{
    float3 ambient = 0;
    float3 diffuse = 0;
    float3 specular = 0;
    float3 finalColor = 0;
    float3 ToLight = PointLight.Position - position;
    float3 ToEye = EyePosition() - position;
    float DistToLight = length(ToLight);
  
    //float bumpMap = saturate(dot(material.bump, -GlobalLight.Direction));
   
  
    // Phong diffuse
    ToLight /= DistToLight; // Normalize
    ambient = material.Ambient * PointLight.Ambient;
    float NDotL = saturate(dot(float3(ToLight.x, ToLight.y, ToLight.z), material.normal));
    diffuse = material.diffuseColor * PointLight.Diffuse * NDotL;

    // Blinn specular
    ToEye = normalize(ToEye);
    float3 HalfWay = normalize(ToEye + ToLight);
    float NDotH = saturate(dot(HalfWay, normalize(material.normal)));
    float3 R = normalize(reflect(-ToLight, material.normal));
    float RdotE = saturate(dot(R, normalize(ToEye)));
    specular = pow(NDotH, 20) * material.Specular.rgb * PointLight.Specular.rgb;


    // Attenuation
    float DistToLightNorm = 1.0 - saturate(DistToLight / PointLight.Range);
    float Attn = DistToLightNorm * DistToLightNorm * (1.0f / PointLight.Intensity);
    //diffuse *= diffuse;
    ambient += ambient;
    diffuse += diffuse * NDotL;
    //specular += specular ;
    
    finalColor = ambient + diffuse + specular;
  // finalColor *= diffuse;
    
   
   finalColor *= NDotL;
   finalColor *= Attn;
  //  finalColor *= finalColor;
    return finalColor;
}
float3 CalcPointPBR(float3 position, Material material)
{


    float3 lightDir = PointLight.Position - position;
    float3 viewDir = EyePosition() - position;
    float DistToLight = length(lightDir);
  
    //float bumpMap = saturate(dot(material.bump, -GlobalLight.Direction));
   
  
    // Phong diffuse
    lightDir /= DistToLight; // Normalize
   
    float3 H = normalize(viewDir + lightDir);
    float3 R = normalize(reflect(-lightDir, material.normal));
   
 //calc all the dot products we need
    float NdotL = saturate(dot(material.Bump, lightDir));
    float NdotH = saturate(dot(material.Bump, H));
   // float NdotH = saturate(dot(R, normalize(viewDir)));
    float NdotV = saturate(dot(material.normal, viewDir));
    float VdotH = saturate(dot(viewDir, H));
    float LdotH = saturate(dot(lightDir, H));
    float alpha = max(0.001f, material.roughness * material.roughness);
    //float alpha = max(material.roughness * material.roughness, 2.0e-3f);
 //apply gamma correction for the colors
    float3 g_diffuse = pow(material.diffuseColor, 2.2);
    float3 g_lightColor = GlobalLight.Specular;

 // Lerp with metallic value to find the good diffuse and specular. 
    float3 realAlbedo = g_diffuse - (g_diffuse * material.matallic);
    realAlbedo = saturate(realAlbedo);
   //float3 F0 = float3(0.04f, 0.04f, 0.04f);
    //F0 = lerp(F0, realAlbedo, material.matallic);
 // 0.03 default specular value for dielectric.
    float3 realSpecularColor = lerp(0.03f, g_diffuse, material.matallic);
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, g_diffuse, material.matallic);

 //calculate the diffuse and specular components
    float3 diffuse = 0;
    float3 specular = 0;

 //final result
    diffuse += NdotL * Disney(NdotL, LdotH, NdotV, alpha) * PointLight.Diffuse.rgb ;
 
    specular += NdotL * GGX(NdotL, NdotH, NdotV, VdotH, LdotH, alpha, F0) * PointLight.Diffuse.rgb ;

	//Add to outgoing radiance Lo


    
    //F0 *= diffuse;
    float shadowAtt = CascadedShadow(position);

  
    float3 finalColor = realAlbedo * diffuse.rgb + (specular.rgb);
   // float3 finalColor = (F0 * (1.0 - specular) + specular);
 
    finalColor = FinalGamma(finalColor) * shadowAtt;
    
    
    // Attenuation
    float DistToLightNorm = 1.0 - saturate(DistToLight / PointLight.Range);
    float Attn = DistToLightNorm * DistToLightNorm * (1.0f / PointLight.Intensity);
    //diffuse *= diffuse;
     
    //specular += specular ;
    
   // finalColor *= diffuse;
    
 
    finalColor *= Attn;
  //  finalColor *= finalColor;
    return finalColor;
}
float4 PointLightCommonPS(DS_OUTPUT In, bool bUseShadow) : SV_TARGET
{
    float3 finalColor = 0;
    
  
    SURFACE_DATA gbd = UnpackGBuffer_Loc(In.Position.xy);
   
	// Convert the data into the material structure
        Material mat;
        MaterialFromGBuffer(gbd, mat);
     
	// Reconstruct the world position
        float3 position = CalcWorldPos(In.cpPos, gbd.LinearDepth);
     

        //float3 dirColor = CalcDirectional(position, mat);
      
	// Calculate the light contribution
      
    //finalColor = CalcPoint(position, mat,true);
    finalColor = CalcPointPBR(position, mat);
    return float4(finalColor * PointLight.Intensity, 1.0f);
}
float4 PointLightPS(DS_OUTPUT In) : SV_TARGET
{
    return PointLightCommonPS(In, false);
}
float4 PointLightShadowPS(DS_OUTPUT In) : SV_TARGET
{
    return PointLightCommonPS(In, true);
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////














/////////////////////////////////////states/////////////////////////////
RasterizerState RS
{
   // FillMode = WireFrame;
    CullMode = Front;
};


DepthStencilState SkyDSS
{
    DepthEnable = false;
    DepthWriteMask = Zero;
    DepthFunc = Less;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;
};

RasterizerState terrainRS
{
    FillMode = WireFrame;
    //CullMode = Front;
};

RasterizerState cascadedRS
{
    CullMode = front;
    DepthBias = 6;
    SlopeScaledDepthBias = -1.0f;
    DepthClipEnable = false;
};

DepthStencilState cascadedDSS
{
    DepthEnable = true;
    DepthWriteMask = All;
    DepthFunc = Less;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;
   // DepthFunc = Less;
   
};




DepthStencilState pointDSS
{
    DepthEnable = true;
    DepthWriteMask = Zero;
    DepthFunc = Greater_Equal;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;
 
   
};

BlendState blendState
{
    BlendEnable[0] = true;
    DestBlend[0] = ONE;
//SRC_COLOR;
    SrcBlend[0] = ONE;
    BlendOp[0] = Add;

    DestBlendAlpha[0] = ONE;
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;
};

DepthStencilState reflectionDSS
{
    DepthEnable = true;
    DepthWriteMask = Zero;
    DepthFunc = Less_Equal;

    StencilEnable = false;
   
 
   
};


////////////////////////////////////////////////////////////////////////////////////////////
technique11 T0
{ 
    
 ////////////////////////////////   /*Shadow*/   /////////////////////////////////
    pass P0
    {
        SetRasterizerState(cascadedRS);
        //SetDepthStencilState(cascadedDSS,0);
        SetVertexShader(CompileShader(vs_5_0, VS_Mesh_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
    pass P1
    {
        SetRasterizerState(cascadedRS);
       // SetDepthStencilState(cascadedDSS,0);
        SetVertexShader(CompileShader(vs_5_0, VS_Model_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
    pass P2
    {
        SetRasterizerState(cascadedRS);
        //SetDepthStencilState(cascadedDSS, 0);
        SetVertexShader(CompileShader(vs_5_0, VS_Animation_PreRender()));
        SetGeometryShader(CompileShader(gs_5_0, CascadedShadowMapsGenGS()));
        SetPixelShader(NULL);
    }
         
  /////// /////////////////////// /*Deferred Packing */// //////////////////////////////
//Sky
    P_DSS_VP(P3, SkyDSS, VS_Dome, PS_Dome)
    P_DSS_BS_VP(P4, SkyDSS, blendState, VS_Cloud, PS_Cloud)
   // P_BS_VP(P5, blendState, VS_Moon, PS_Moon)

//Mesh&Model
    P_VP(P5, VS_Mesh_PreRender, RenderScenePS)
    P_VP(P6, VS_Model_PreRender, RenderScenePS)
    P_VP(P7, VS_Animation_PreRender, RenderScenePS)

    pass P8 //terrain
    {
        //SetRasterizerState(terrainRS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetHullShader(CompileShader(hs_5_0, HS()));
        SetDomainShader(CompileShader(ds_5_0, DS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

 ////////////////////////////////dir light render //////////////////////////////////////   
    pass P9
    {
       // SetBlendState(blendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, DirLightVS()));
        SetPixelShader(CompileShader(ps_5_0, DirLightPS()));
    }
    pass P10
    {
        SetRasterizerState(RS);
        SetDepthStencilState(pointDSS,0);
        SetBlendState(blendState, float4(0, 0, 0, 0), 0xFF);
        SetVertexShader(CompileShader(vs_5_0, PointLightVS()));
        SetHullShader(CompileShader(hs_5_0, PointLightHS()));
        SetDomainShader(CompileShader(ds_5_0, PointLightDS()));
        SetPixelShader(CompileShader(ps_5_0, PointLightShadowPS()));
    }
   
    pass P11
    {
       // SetDepthStencilState( reflectionDSS, 0);
        SetVertexShader(CompileShader(vs_5_0, VS_Mesh_PreRender()));
        SetPixelShader(CompileShader(ps_5_0, SSReflectionPS()));
        

    }

   
}
