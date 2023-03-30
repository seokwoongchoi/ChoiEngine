
Texture2D noiseTexture;
SamplerState LinearSample
{
    Filter = MIN_MAG_MIP_LINEAR;
    //AddressU = WRAP;
    //AddressV = WRAP;
    //Filter = ANISOTROPIC;
    AddressU = WRAP;
    AddressV = WRAP;
};

cbuffer CB_PS
{
    float4 CameraPos;
    float3 CameraDir;
    float Time;
    matrix VP;
};

#define D_DEMO_FREE
//#define D_DEMO_SHOW_IMPROVEMENT_FLAT
//#define D_DEMO_SHOW_IMPROVEMENT_NOISE
//#define D_DEMO_SHOW_IMPROVEMENT_FLAT_NOVOLUMETRICSHADOW
//#define D_DEMO_SHOW_IMPROVEMENT_NOISE_NOVOLUMETRICSHADOW





#ifdef D_DEMO_FREE
	// Apply noise on top of the height fog?
#define D_FOG_NOISE 1.0

	// Height fog multiplier to show off improvement with new integration formula
#define D_STRONG_FOG 0.0

    // Enable/disable volumetric shadow (single scattering shadow)
#define D_VOLUME_SHADOW_ENABLE 1

	// Use imporved scattering?
	// In this mode it is full screen and can be toggle on/off.
#define D_USE_IMPROVE_INTEGRATION 1

//
// Pre defined setup to show benefit of the new integration. Use D_DEMO_FREE to play with parameters
//
#elif defined(D_DEMO_SHOW_IMPROVEMENT_FLAT)
#define D_STRONG_FOG 10.0
#define D_FOG_NOISE 0.0
#define D_VOLUME_SHADOW_ENABLE 1
#elif defined(D_DEMO_SHOW_IMPROVEMENT_NOISE)
#define D_STRONG_FOG 5.0
#define D_FOG_NOISE 1.0
#define D_VOLUME_SHADOW_ENABLE 1
#elif defined(D_DEMO_SHOW_IMPROVEMENT_FLAT_NOVOLUMETRICSHADOW)
#define D_STRONG_FOG 10.0
#define D_FOG_NOISE 0.0
#define D_VOLUME_SHADOW_ENABLE 0
#elif defined(D_DEMO_SHOW_IMPROVEMENT_NOISE_NOVOLUMETRICSHADOW)
#define D_STRONG_FOG 3.0
#define D_FOG_NOISE 1.0
#define D_VOLUME_SHADOW_ENABLE 0
#endif



/*
 * Other options you can tweak
 */

// Used to control wether transmittance is updated before or after scattering (when not using improved integration)
// If 0 strongly scattering participating media will not be energy conservative
// If 1 participating media will look too dark especially for strong extinction (as compared to what it should be)
// Toggle only visible zhen not using the improved scattering integration.
#define D_UPDATE_TRANS_FIRST 1

// Apply bump mapping on walls
#define D_DETAILED_WALLS 1

// Use to restrict ray marching length. Needed for volumetric evaluation.
#define D_MAX_STEP_LENGTH_ENABLE 1

// Light position and color
#define LPOS float3( 20.0+15.0*sin(Time), 15.0+12.0*cos(Time),-20.0)
#define LCOL (600.0*float3( 1.0, 0.9, 0.5))


float displacementSimple(float2 p)
{
    float f;
    f = 0.5000 * noiseTexture.Sample(LinearSample, p, 0.0).x;
    p = p * 2.0;
    f += 0.2500 * noiseTexture.Sample(LinearSample, p, 0.0).x;
    p = p * 2.0;
    f += 0.1250 * noiseTexture.Sample(LinearSample, p, 0.0).x;
    p = p * 2.0;
    f += 0.0625 * noiseTexture.Sample(LinearSample, p, 0.0).x;
    p = p * 2.0;
    
    return f;
}


float3 getSceneColor(float3 p, float material)
{
    [branch]
    if (material == 1.0)
    {
        return float3(1.0, 0.5, 0.5);
    }
    else if (material == 2.0)
    {
        return float3(0.5, 1.0, 0.5);
    }
    else if (material == 3.0)
    {
        return float3(0.5, 0.5, 1.0);
    }
	
    return float3(0.0, 0.0, 0.0);
}


float getClosestDistance(float3 p, out float material)
{
    float d = 0.0;
#if D_MAX_STEP_LENGTH_ENABLE
    float minD = 1.0; // restrict max step for better scattering evaluation
#else
	float minD = 10000000.0;
#endif
    material = 0.0;
    
    float yNoise = 0.0;
    float xNoise = 0.0;
    float zNoise = 0.0;
#if D_DETAILED_WALLS
    yNoise = 1.0*clamp(displacementSimple(p.xz*0.005),0.0,1.0);
    xNoise = 2.0*clamp(displacementSimple(p.zy*0.005),0.0,1.0);
    zNoise = 0.5*clamp(displacementSimple(p.xy*0.01),0.0,1.0);
#endif
    
    d = max(0.0, p.y - yNoise);
    if (d < minD)
    {
        minD = d;
        material = 2.0;
    }
	
    d = max(0.0, p.x - xNoise);
    if (d < minD)
    {
        minD = d;
        material = 1.0;
    }
	
    d = max(0.0, 40.0 - p.x - xNoise);
    if (d < minD)
    {
        minD = d;
        material = 1.0;
    }
	
    d = max(0.0, -p.z - zNoise);
    if (d < minD)
    {
        minD = d;
        material = 3.0;
    }
    
    return minD;
}


float3 calcNormal(in float3 pos)
{
    float material = 0.0;
    float3 eps = float3(0.3, 0.0, 0.0);
    return normalize(float3(
           getClosestDistance(pos + eps.xyy, material) - getClosestDistance(pos - eps.xyy, material),
           getClosestDistance(pos + eps.yxy, material) - getClosestDistance(pos - eps.yxy, material),
           getClosestDistance(pos + eps.yyx, material) - getClosestDistance(pos - eps.yyx, material)));

}

float3 evaluateLight(in float3 pos)
{
    float3 lightPos = LPOS;
    float3 lightCol = LCOL;
    float3 L = lightPos - pos;
    return lightCol * 1.0 / dot(L, L);
}

float3 evaluateLight(in float3 pos, in float3 normal)
{
    float3 lightPos = LPOS;
    float3 L = lightPos - pos;
    float distanceToL = length(L);
    float3 Lnorm = L / distanceToL;
    return max(0.0, dot(normal, Lnorm)) * evaluateLight(pos);
}

// To simplify: wavelength independent scattering and extinction
void getParticipatingMedia(out float sigmaS, out float sigmaE, in float3 pos)
{
    float heightFog = 7.0 + D_FOG_NOISE * 3.0 * clamp(displacementSimple(pos.xz * 0.005 + Time * 0.01), 0.0, 1.0);
    heightFog = 0.3 * clamp((heightFog - pos.y) * 1.0, 0.0, 1.0);
    
    const float fogFactor = 1.0 + D_STRONG_FOG * 5.0;
    
    const float sphereRadius = 5.0;
    float sphereFog = clamp((sphereRadius - length(pos - float3(20.0, 19.0, -17.0))) / sphereRadius, 0.0, 1.0);
    
    const float constantFog = 0.02;

    sigmaS = constantFog + heightFog * fogFactor + sphereFog;
   
    const float sigmaA = 0.0;
    sigmaE = max(0.000000001, sigmaA + sigmaS); // to avoid division by zero extinction
}

float phaseFunction()
{
    return 1.0 / (4.0 * 3.14);
}

float volumetricShadow(in float3 from, in float3 to)
{
#if D_VOLUME_SHADOW_ENABLE
    const float numStep = 16.0; // quality control. Bump to avoid shadow alisaing
    float shadow = 1.0;
    float sigmaS = 0.0;
    float sigmaE = 0.0;
    float dd = length(to - from) / numStep;
    for (float s = 0.5; s < (numStep - 0.1); s += 1.0)// start at 0.5 to sample at center of integral part
    {
        float3 pos = from + (to - from) * (s / (numStep));
        getParticipatingMedia(sigmaS, sigmaE, pos);
        shadow *= exp(-sigmaE * dd);
    }
    return shadow;
#else
    return 1.0;
#endif
}

void traceScene(bool improvedScattering, float3 rO, float3 rD, inout float3 finalPos, inout float3 normal, inout float3 albedo, inout float4 scatTrans)
{
    const int numIter = 32;
	
    float sigmaS = 0.0;
    float sigmaE = 0.0;
    
    float3 lightPos = LPOS;
    
    // Initialise volumetric scattering integration (to view)
    float transmittance = 1.0;
    float3 scatteredLight = float3(0.0, 0.0, 0.0);
    
    float d = 1.0; // hack: always have a first step of 1 unit to go further
    float material = 0.0;
    float3 p = float3(0.0, 0.0, 0.0);
    float dd = 0.0;
    [unroll(numIter)]
    for (int i = 0; i < numIter; ++i)
    {
        float3 p = rO + d * rD;
        
        
        getParticipatingMedia(sigmaS, sigmaE, p);
        
#ifdef D_DEMO_FREE
        if (D_USE_IMPROVE_INTEGRATION > 0) // freedom/tweakable version
#else
        if(improvedScattering)
#endif
        {
            // See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/
            float3 S = evaluateLight(p) * sigmaS * phaseFunction() * volumetricShadow(p, lightPos); // incoming light
            float3 Sint = (S - S * exp(-sigmaE * dd)) / sigmaE; // integrate along the current step segment
            scatteredLight += transmittance * Sint; // accumulate and also take into account the transmittance from previous steps

            // Evaluate transmittance to view independentely
            transmittance *= exp(-sigmaE * dd);
        }
        else
        {
            // Basic scatering/transmittance integration
#if D_UPDATE_TRANS_FIRST
            transmittance *= exp(-sigmaE * dd);
#endif
            scatteredLight += sigmaS * evaluateLight(p) * phaseFunction() * volumetricShadow(p, lightPos) * transmittance * dd;
#if !D_UPDATE_TRANS_FIRST
            transmittance *= exp(-sigmaE * dd);
#endif
        }
        
		
        dd = getClosestDistance(p, material);
        if (dd < 0.2)
            break; // give back a lot of performance without too much visual loss
        d += dd;
    }
	
    albedo = getSceneColor(p, material);
	
    finalPos = rO + d * rD;
    
    normal = calcNormal(finalPos);
    
    scatTrans = float4(scatteredLight, transmittance);
}




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
    float2 UV : TEXCOORD0;
    float3 Direction : DIRECTION0;
};

VS_OUTPUT FullScreenQuadVS(uint VertexID : SV_VertexID)
{
    VS_OUTPUT Output;

    Output.Position = float4(arrBasePos[VertexID].xy, 0.0, 1.0);
    Output.UV = arrUV[VertexID].xy;
    Output.Direction = mul(normalize(float3(Output.Position.xy, 1.0)), (float3x3) VP);
    return Output;
}

float4 FinalPassPS(VS_OUTPUT In) : SV_TARGET
{
   
    //Time
    //iMouse
    //iResolution
    
    
    float2 uv = In.UV;
    
    float hfactor = 720.0f/ 1280.0f; // make it screen ratio independent
    float2 uv2 = float2(2.0, 2.0 * hfactor) * uv - float2(1.0, hfactor);
	
    float3 camPos = CameraPos.xyz;
  
    float3 camX = float3(1.0, 0.0, 0.0);
    float3 camY = float3(0.0, -1.0, 0.0);
    float3 camZ = float3(0.0, 0.0, 1.0);
	
    float3 rO = camPos;
    float3 rD = normalize(uv2.x * camX + uv2.y * camY + camZ);
    float3 finalPos = rO;
    float3 albedo = float3(0.0, 0.0, 0.0);
    float3 normal = float3(0.0, 0.0, 0.0);
    float4 scatTrans = float4(0.0, 0.0, 0.0, 0.0);
    traceScene(true,
        rO, rD, finalPos, normal, albedo, scatTrans);
	
    
    //lighting
    float3 color = (albedo / 3.14) * evaluateLight(finalPos, normal) * volumetricShadow(finalPos, LPOS);
    // Apply scattering/transmittance
    color = color * scatTrans.w + scatTrans.xyz;
    
    // Gamma correction
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2)); // simple linear to gamma, exposure of 1.0
   
#ifndef D_DEMO_FREE
    // Separation line
    if(abs(fragCoord.x-(iResolution.x*0.5))<0.6)
        color.r = 0.5;
#endif
    float4 fragColor = float4(color, 1.0);
 
    return fragColor;
}
technique11 T0
{
    
 ////////////////////////////////   /*Shadow*/   /////////////////////////////////
    pass P0
    {
       
        SetVertexShader(CompileShader(vs_5_0, FullScreenQuadVS()));
        SetPixelShader(CompileShader(ps_5_0, FinalPassPS()));
    }
}
//void mainImage(out float4 fragColor, in float2 fragCoord)
//{
//    float2 p = (2.0 * fragCoord - iResolution.xy) / iResolution.y;

//    float2 m = iMouse.xy / iResolution.xy;
    
//    // camera
//    float3 ro = 4.0 * normalize(float3(sin(3.0 * m.x), 0.4 * m.y, cos(3.0 * m.x)));
//    float3 ta = float3(0.0, -1.0, 0.0);
//    float3x3 ca = setCamera(ro, ta, 0.0);
//    // ray
//    float3 rd = mul(ca, normalize(float3(p.xy, 1.5)));
    
//    fragColor = render(ro, rd, float2(fragCoord - 0.5));
//}

