//#include "000_Matrix.fx"
//#include "000_Header.fx"
//#define MAX_MODEL_TRANSFORMS 500
//#define MAX_MODEL_KEYFRAMES 300
//#define MAX_MODEL_INSTANCE 100
#define EPSILON  0.000001f
const static float4 BoundsMin = float4(-0.5, -0.5, -0.5, 1);
const static float4 BoundsMax = float4(0.5, 0.5, 0.5, 1);

struct Sphere
{
    float3 position;
    float3 velocity;
    int radius;
};
struct InputMatrix
{
    matrix Bone;
    float4 Factor;
};

//struct ResultMatrix
//{
//    float4x4 Result;
//    float4 Color;
    
//};

struct CopyFactor
{
  
    float4 Indices;
    float4 Factor;
    float4 Direction;
};

cbuffer CB_Ray
{
    float3 Position;
    float drawCount;
    float3 Direction;
    float pad2;
    
};
Texture2DArray InstInput;
//StructuredBuffer<InputMatrix> Input;
Texture2D<float4> Input;


//RWStructuredBuffer<ResultMatrix> Output;


RWStructuredBuffer<CopyFactor> CopyOutput;

void swap(inout float src, inout float dest)
{
    float temp = 0;
    temp = src;
    src = dest;
    dest = temp;
}

int IntersectionAABB(float3 org, float3 dir, float3 lb, float3 rt)
{
    float3 dirfrac = 0;
     dirfrac.x = 1.0f / dir.x;
     dirfrac.y = 1.0f / dir.y;
     dirfrac.z = 1.0f / dir.z;
// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
// r.org is origin of ray
    
    float t1 = (lb.x - org.x) * dirfrac.x;
    float t2 = (rt.x - org.x) * dirfrac.x;
    float t3 = (lb.y - org.y) * dirfrac.y;
    float t4 = (rt.y - org.y) * dirfrac.y;
    float t5 = (lb.z - org.z) * dirfrac.z;
    float t6 = (rt.z - org.z) * dirfrac.z;

    float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
    [branch]
    if (tmax < 0)
    {
        //t = tmax;
        return 0;
    }

// if tmin > tmax, ray doesn't intersect AABB
    [branch]
    if (tmin > tmax)
    {
        //t = tmax;
        return 0;
    }

   // t = tmin;
    return 1;

   
    
}

int SperatingPlane(float3 position, float3 direction, 
float3 Box1AxisX, float3 Box1AxisY,float3 Box1AxisZ, 
float3 Box2AxisX, float3 Box2AxisY, float3 Box2AxisZ, float3 HalfSize1, float3 HalfSize2)
{
    float val = abs(dot(position, direction));

    float val2 = 0.0f;
    val2 += abs(dot((Box1AxisX * HalfSize1.x),  direction));
    val2 += abs(dot((Box1AxisY * HalfSize1.y),  direction));
    val2 += abs(dot((Box1AxisZ * HalfSize1.z),  direction));
    val2 += abs(dot((Box2AxisX * HalfSize2.x),  direction));
    val2 += abs(dot((Box2AxisY * HalfSize2.y),  direction));
    val2 += abs(dot((Box2AxisZ * HalfSize2.z),  direction));

  
    return val > val2;
}
float3 BoxPosition(matrix world)
{
    return float3(world._41, world._42, world._43);
}
float3 BoxScale(matrix world)
{
    return float3(world._11, world._22, world._33);
}

float3 MatrixForward(matrix world)
{
    return float3(world._31, world._32, world._33);
}

float3 MatrixUp(matrix world)
{
    return float3(world._21, world._22, world._23);
}
float3 MatrixRight(matrix world)
{
    return float3(world._11, world._12, world._13);
}

float3 minus(float3 v1,  float3 v2)
{
    float3 r;
    r.x = v1.x - v2.x;
    r.y = v1.y - v2.y;
    r.z = v1.z - v2.z;
    return r;
}

double dotProduct(float3 v1, float3 v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

float3 scale(float3 v, double a)
{
    float3 r;
    r.x = v.x * a;
    r.y = v.y * a;
    r.z = v.z * a;
    return r;
}

float3 projectUonV(float3 u,  float3 v)
{
    float3 r;
    r = scale(v, dotProduct(u, v) / dotProduct(v, v));
    return r;
}

int distanceSquared(float3 v1, float3 v2)
{
    float3 delta = minus(v2, v1);
    return dotProduct(delta, delta);
}


int doesItCollide(float s1radius, float s2radius, float3 s1position, float3 s2position)
{
    int rSquared = s1radius + s2radius;
    rSquared *=rSquared;
    return distanceSquared(s1position, s2position) <rSquared;
}

int IntersectionSphere(matrix Box1, matrix Box2)
{
    
    GroupMemoryBarrierWithGroupSync();
   
  
    
        
    float4 Min = mul(BoundsMin, Box1);
    float4 Max = mul(BoundsMax, Box1);
    float3 sPos1 = (Min.xyz + Max.xyz) * 0.5f;
    
    float4 Min2 = mul(BoundsMin, Box2);
    float4 Max2 = mul(BoundsMax, Box2);
    float3 sPos2 = (Min2.xyz + Max2.xyz) * 0.5f;
   
    
    float3 dist = sPos2 - sPos1;
    
    float l2 = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
    int rSquared = length(Max.xyz - Min.xyz) * 0.5f + length(Max2.xyz - Min2.xyz) * 0.5f;
    rSquared *= rSquared;
    
    
    //if (l2 > rSquared)
    //    return 0;
    
    //float l = (length(Max.xyz - Min.xyz) * 0.5f + length(Max2.xyz - Min2.xyz) * 0.5f) - sqrt(l2);
    
    
   // float3 hitVec = mul(normalize(dist), l);
   int result= doesItCollide(length(Max.xyz - Min.xyz) * 0.5f, length(Max2.xyz - Min2.xyz) * 0.5f, (Min.xyz + Max.xyz) * 0.5f, (Min2.xyz + Max2.xyz) * 0.5f);
    GroupMemoryBarrierWithGroupSync();
    
    return result;
}
int Collison(matrix Box1,matrix Box2,uint threadY)
{
    
  
   // matrix Box1 = Input[x].Bone;
   // matrix Box2 = Input[y].Bone;
    
   
    
    float3 position = BoxPosition(Box2) - BoxPosition(Box1);
    //[branch]
    //if (abs(length(position)) < 0.1f)
    //    return 0;
        
    float3 Box1AxisX = float3(normalize(MatrixRight(Box1)));
    float3 Box1AxisY = float3(normalize(MatrixUp(Box1)));
    float3 Box1AxisZ = float3(normalize(MatrixForward(Box1)));
    
    float3 Box2AxisX = float3(normalize(MatrixRight(Box2)));
    float3 Box2AxisY = float3(normalize(MatrixUp(Box2)));
    float3 Box2AxisZ = float3(normalize(MatrixForward(Box2)));
    
    float3 halfSize1 = BoxScale(Box1)*0.5f;
    float3 halfSize2 = BoxScale(Box2) * 0.5f;
  
    
    float result = 0;
   //  [branch]
   // if (threadY==0)
    { 
        // [branch]
        if (SperatingPlane(position, Box1AxisX,
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
         
            return 0;
        }
       
    }
  //  [branch]
  //  if (threadY == 1)
    {
       // [branch]
        if (SperatingPlane(position, Box1AxisY,
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
          
            return 0;
        }
        
    }
   //   [branch]
   // if (threadY == 2)
    {
       //  [branch]
        if (SperatingPlane(position, Box1AxisZ,
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
          
            return 0;
        }
        
    }
   //  [branch]
   // if (threadY == 3)
    {
       //  [branch]
        if (SperatingPlane(position, Box2AxisX,
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
          
            return 0;
        }
        
    }
   // [branch]
   // if (threadY == 4)
    {
       //  [branch]
        if (SperatingPlane(position, Box2AxisY,
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
          
            return 0;
        }
        
    }
   // [branch]
   // if (threadY == 5)
    {
      //   [branch]
        if (SperatingPlane(position, Box2AxisZ,
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
           
            return 0;
        }
        
    }
   //  [branch]
   // if (threadY == 6)
    {
      //   [branch]
        if (SperatingPlane(position, cross(Box1AxisX, Box2AxisX),
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
          
            return 0;
        }
       
    }
    //[branch]
    //if (threadY == 7)
    {
       //  [branch]
        if (SperatingPlane(position, cross(Box1AxisX, Box2AxisY),
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
          
            return 0;
        }
      
    }
   //  [branch]
   // if (threadY == 8)
    {
       //  [branch]
        if (SperatingPlane(position, cross(Box1AxisX, Box2AxisZ),
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
          
            return 0;
        }
        
    }
   //  [branch]
   // if (threadY == 9)
    {
       // [branch]
        if (SperatingPlane(position, cross(Box1AxisY, Box2AxisX),
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
           
            return 0;
        }
       
    }
   //  [branch]
   // if (threadY == 10)
    {
      //   [branch]
        if (SperatingPlane(position, cross(Box1AxisY, Box2AxisY),
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
          
            return 0;
        }
       
    }
    // [branch]
    //if (threadY == 11)
    {
       //   [branch]
        if (SperatingPlane(position, cross(Box1AxisY, Box2AxisZ),
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
          
            return 0;
        }
       
    }
    // [branch]
    //if (threadY == 12)
    {
       // [branch]
        if (SperatingPlane(position, cross(Box1AxisZ, Box2AxisX),
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
          
            return 0;
        }
       
    }
    // [branch]
    //if (threadY == 13)
    {
       //  [branch]
        if (SperatingPlane(position, cross(Box1AxisZ, Box2AxisY),
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
           
            return 0;
        }
      
    }
    //[branch]
    //if (threadY == 14)
    {
       //  [branch]
        if (SperatingPlane(position, cross(Box1AxisZ, Box2AxisZ),
        Box1AxisX, Box1AxisY, Box1AxisZ,
        Box2AxisX, Box2AxisY, Box2AxisZ,
        halfSize1, halfSize2) == 1)
        {
         
            return 0;
        }
     
            
    }
   
    return 1;
   
   
   
    
}


matrix CreateBox(int index,int BoxType)
{
    matrix Box = 0;
    Box._11_12_13_14 = Input.Load(int3(0+(4*BoxType), index, 0));
    Box._21_22_23_24 = Input.Load(int3(1+(4*BoxType), index, 0));
    Box._31_32_33_34 = Input.Load(int3(2+(4*BoxType), index, 0));
    Box._41_42_43_44 = Input.Load(int3(3 + (4 * BoxType), index, 0));
    
    return Box;

}



[numthreads(20, 1, 1)]
void CS(uint3 groupID : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID)//uint3 GroupID:SV_GroupID,uint GroupIndex:SV_GroupIndex
{
    int result = 0;
  
    if (groupThreadId.x<drawCount)
    {
        matrix Box1 = CreateBox(groupID.x, 0);
        matrix Box2 = CreateBox(groupThreadId.x, 0);
        int Bodyintersection = 0;
        int HeadIntersection = 0;
        float3 direction = 0;
  
        if (groupID.x != groupThreadId.x)
        {
            result = Collison(Box1, Box2, groupThreadId.x);
           
        
            float4 BodyMin = mul(BoundsMin, Box2);
            float4 BodyMax = mul(BoundsMax, Box2);
            Bodyintersection = IntersectionAABB(Position, Direction, BodyMin.xyz, BodyMax.xyz);
    
        
            if (Input[uint2(8, groupThreadId.x)].a == 0)
            {
                matrix HeadMatrix = CreateBox(groupThreadId.x, 1);
                float4 HeadMin = mul(BoundsMin, HeadMatrix);
                float4 HeadMax = mul(BoundsMax, HeadMatrix);
                HeadIntersection = IntersectionAABB(Position, Direction, HeadMin.xyz, HeadMax.xyz);
            }
      
     
    
            if (result == 1)
            {
                direction = Box2._41_42_43 - Box1._41_42_43;
            }
            GroupMemoryBarrierWithGroupSync();
        }
  

      
   
   // CopyOutput[uint2(1, globalThreadId.x)] = float4(Input[uint2(8, globalThreadId.x)].r, Input[uint2(8, globalThreadId.x)].g, result | Bodyintersection, HeadIntersection);
        CopyOutput[groupThreadId.x].Indices.x = Input[uint2(8, groupThreadId.x)].r; //actor Index
        CopyOutput[groupThreadId.x].Indices.y = Input[uint2(8, groupThreadId.x)].g; //Instance Num
        CopyOutput[groupThreadId.x].Indices.z = Input[uint2(8, groupThreadId.x)].b; //culled drawCount
        CopyOutput[groupThreadId.x].Indices.w = Input[uint2(8, groupThreadId.x)].a; //ActorType 0=skin, 1=static
    
        CopyOutput[groupThreadId.x].Factor.x = Bodyintersection;
        CopyOutput[groupThreadId.x].Factor.y = HeadIntersection;
        CopyOutput[groupThreadId.x].Factor.z = result;
        CopyOutput[groupThreadId.x].Factor.w = 0;
     
        CopyOutput[groupThreadId.x].Direction = -float4(direction, 0.0f);
        
    }
  
   
   // Output[globalThreadId.x].Result = Input[globalThreadId.x].Bone;
   // Output[globalThreadId.x].Color = lerp(float4(0, 1, 0, 0.5), float4(0, 0, 1, 0.5), result | intersection);
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