#define FLT_EPSILON      1.192092896e-07F  
const static float4 Start = float4(0.0, 0.0, 0.0, 1);
const static float4 End = float4(0.0, 1.0, 0.0, 1);


cbuffer CB_DrawCount : register(b0)
{
   
    uint drawCount : packoffset(c0.x);
    uint skeletalCount : packoffset(c0.y);
    uint staticCount : packoffset(c0.z);
    uint pad : packoffset(c0.w);
    
};
Texture2D<float4> Input : register(t0);
RWStructuredBuffer<float4> CopyOutput : register(u0);
RWTexture2D<float4> EffectPostionTexture : register(u1);


//RWTexture2D<float4> EffectOutput : register(u1);

void CreateObj(inout matrix obj,int index, int BoxType)
{
   
    obj._11_12_13_14 = Input[int2(0 + (4 * BoxType), index)];
    obj._21_22_23_24 = Input[int2(1 + (4 * BoxType), index)];
    obj._31_32_33_34 = Input[int2(2 + (4 * BoxType), index)];
    obj._41_42_43_44 = Input[int2(3 + (4 * BoxType), index)];
     

}
float Clamp(float n, float min, float max)
{
   
    if (n < min)
        return min;
    else if (n > max)
        return max;
    
    return n;
}
float LengthSq(float3 value)
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

float3 ClosestPtSegmentSegmentNorm(float3 p1, float3 q1, float3 p2, float3 q2, float s, float  t, inout float3 c1, float3 c2)
{
    
    float3 d1 = q1 - p1;
    float3 d2 = q2 - p2;
    float3 r = p1 - p2;
    float a = LengthSq(d1);
    float e = LengthSq(d2);
    float f = dot(d2, r);
   
    if (a <= FLT_EPSILON && e <= FLT_EPSILON)
    {
        s = t = 0.0f;
        c1 = p1;
        c2 = p2;
        return (c1 - c2);
    }
    
  
    if (a <= FLT_EPSILON)
    {
        s = 0.0f;
        t = f / e;
        t = Clamp(t, 0.0f, 1.0f);
    }
    else
    {
        float c = dot(d1, r);
       
        if (e <= FLT_EPSILON)
        {
            t = 0.0f;
            s = Clamp(-c / a, 0.0f, 1.0f);
        }
        else
        {
            float b = dot(d1, d2);
            float denom = a * e - b * b;
            
           
            if (denom != 0.0f)
            {
                s = Clamp((b * f - c * e) / denom, 0.0f, 1.0f) ;
            }
            else
            {
                s = 0.0f;
            }
         
            float tnom = (b * s + f);
           
            
            if (tnom < 0.0f)
            {
                t = 0.0f;
                s = Clamp(-c / a, 0.0f, 1.0f);
            }
            else if (tnom > e)
            {
                t = 1.0f;
                s = Clamp((b - c) / a, 0.0f, 1.0f);
            }
            else
            {
                t = tnom / e;
            }
        }
    }
    c1 = p1 + d1 * s;
    c2 = p2 + d2 * t;
    return (c1 - c2);
}

int IsHit(matrix obj1, matrix obj2,int index,inout float3 hitPos)
//int IsHit(matrix obj1, matrix obj2, inout float3 hitPos)
{
   
    float3 ob1Start = mul(Start, obj1).xyz;
    float3 ob2Start = mul(Start, obj2).xyz;
    
    float3 ob1End = mul(End, obj1).xyz;
    float3 ob2End = mul(End, obj2).xyz;
    
    float s, t;
    float3 c1 = float3(0, 0, 0);
    float3 c2 = float3(0, 0, 0);
    float dist2 = LengthSq(ClosestPtSegmentSegmentNorm(ob1Start, ob1End, ob2Start, ob2End,s,t,c1,c2));
    hitPos = c1;
    float radius1 = LengthSq(obj1._11_22_33)*0.5f;
    float radius2 = LengthSq(obj2._11_22_33)*0.5f;
    float radius = radius1 + radius2;
   
    
    if (dist2 <= radius * radius)
    {
      
        return index;
    }
   
    return -1;
    

}

groupshared int shared_data[3];
groupshared matrix shared_matrix[4];
groupshared matrix sword_matrix;
groupshared matrix body_matrix;
[numthreads(1, 20, 4)]
void CS(uint3 groupID : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)
{
   
   
    shared_data[0] = 0;
    shared_data[1] = 0;
    shared_data[2] = 0;
    // matrix enemybody;
    
   
  
    CreateObj(shared_matrix[groupThreadId.z], groupID.x, groupThreadId.z % 3);
    CopyOutput[groupThreadId.y].x = -1;
    CopyOutput[groupThreadId.y].y = -1;
    CopyOutput[groupThreadId.y].z = -1;
    CopyOutput[groupThreadId.y].w = -1;
  
    GroupMemoryBarrierWithGroupSync();
    int index = groupThreadId.y % skeletalCount;
    float3 hitPos = float3(0, 0, 0);
    if (groupID.x != index && index < skeletalCount)
    {
        int Result = -1;
          
        if (groupThreadId.z > 2)
        {
            CreateObj(body_matrix, index, 0);
            Result = IsHit(body_matrix, shared_matrix[groupThreadId.z], groupThreadId.z, hitPos);
        }
        else
        {
            CreateObj(sword_matrix, index, 2);
            Result = IsHit(sword_matrix, shared_matrix[groupThreadId.z], groupThreadId.z, hitPos);
        
        }
        
      
        [unroll(4)]
        for (uint i = 0; i < 4; i++)
        {
            if (Result == i)
            {
            
                CopyOutput[index][i] = groupID.x;
                     
                if (groupThreadId.z < 3)
                {
                    EffectPostionTexture[int2(i, shared_data[i])] = float4(hitPos, 1.0f);
                    shared_data[i]++;
                }
              
              

            }
        }
    
   
      
    }
 
}
  
 


