#include "000_Header.fx"
//-----------------------------------------------------------------------------------------
// Occlusion
//-----------------------------------------------------------------------------------------
#define Width 512
#define Factor Width*0.5f
#define EPSILON  0.000001f
cbuffer UpdateDesc
{
    
     float3 Location;
     uint Range;
     uint  BrushShape;
    float RaiseSpeed;
};



RWTexture2D<float4> Update;

Texture2D LodHeightMap;

struct Result
{
    float4 Position;
};
RWStructuredBuffer<Result> Output;

//cbuffer CB_Ray
//{
//    float3 Position;
//    float pad1;
//    float3 Direction;
//    float pad2;
    
//};
//void swap(inout float src, inout float dest)
//{
//    float temp = 0;
//    temp = src;
//    src = dest;
//    dest = temp;
//}
//bool IntersectionAABB(const float3 org, const float3 dir,  inout float3 hit)
//{


//    float t_min = 0;
//    float t_max = 3.402823466e+38f;
//    float3 boundsMin = float3(-Factor, 0, -Factor);
//    float3 boundsMax = float3(Factor, 0, Factor);
//    for (int i = 0; i < 3; i++)
//    {
//        if (abs(dir[i]) < EPSILON)
//        {
//            if (org[i] < boundsMin[i] ||
//				org[i] > boundsMax[i])
//            {
                
//                return false;
//            }

//        }
//        else
//        {
//            float denom = 1.0f / dir[i];
//            float t1 = (boundsMin[i] - org[i]) * denom;
//            float t2 = (boundsMax[i] - org[i]) * denom;

//            if (t1 > t2)
//            {
//                swap(t1, t2);
//            }

//            t_min = max(t_min, t1);
//            t_max = min(t_max, t2);

//            if (t_min > t_max)
//            {
               
//                return false;
//            }


//        }
//    }
   
//    hit = org + t_min * dir;
	

   
//    return true;
//}


void Square(uint4 box,uint dispatch)
{
    
    uint3 CurPixel = uint3(box.x + dispatch % (box.y - box.x), box.w + dispatch / box.z, 0);
    if (CurPixel.y < box.z)
    {
        Update[CurPixel.xy] += float4(RaiseSpeed, 0,0 , 0);
    }
          
       
   
}

void Circle(uint4 box, uint dispatch)
{
       
    uint3 CurPixel = uint3(box.x + dispatch % (box.y - box.x), box.w + dispatch / box.z,0);
    if (CurPixel.y < box.z)
    {
        float dx = CurPixel.x - (Location.x + Factor);
        float dy = CurPixel.y - (Location.z - Factor) * -1;
        dx *= dx;
        dy *= dy;
        float distanceSquared = dx + dy;
        float radiusSquared = Range * Range;

        if (distanceSquared <= radiusSquared)
            Update[CurPixel.xy] += float4(RaiseSpeed, 0, 0, 0);
    }
   
}

[numthreads(Width, 1, 1)]
void UpdateHeight(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float3 hit = 0;
  //  IntersectionAABB(Position, Direction, hit);
    
    //float3 Location = float3(hit);
   
    float l = (Location.x - Range) + Factor;
    float r = (Location.x + Range) + Factor;
    float b = ((Location.z - Range) - Factor) * -1;
    float t = ((Location.z + Range) - Factor) * -1;

    uint lb = clamp(l, 0, Width - 1);
    uint rb = clamp(r, 0, Width - 1);
    uint bb = clamp(b, 0, Width - 1);
    uint tb = clamp(t, 0, Width - 1);
       
    [branch]
        if (BrushShape==1)
        {
            Square(uint4(lb, rb, bb, tb), dispatchThreadId.x);

        }
        else if (BrushShape == 2)
        {
            Circle(uint4(lb, rb, bb, tb), dispatchThreadId.x);

        }
       

      
  
 
    
    
}  

[numthreads(Width, 1, 1)]
void Smoothing(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    
    if (BrushShape == 0)
        return;
   
    float l = (Location.x - Range) + Factor;
    float r = (Location.x + Range) + Factor;
    float b = ((Location.z - Range) - Factor) * -1;
    float t = ((Location.z + Range) - Factor) * -1;

    uint left = clamp(l, 0, Width-1);
    uint right = clamp(r, 0, Width - 1);
    uint bottom = clamp(b, 0, Width - 1);
    uint top = clamp(t, 0, Width - 1);
  
   
   uint4 box = uint4(left, right, bottom, top);
   
    uint3 CurPixel = uint3(box.x + dispatchThreadId.x % (box.y - box.x), box.w + dispatchThreadId.x / box.z, 0);
    if (CurPixel.y < box.z)
    {
        int adjacentSections = 0; // 인접하는곳의 갯수
        float sectionsTotal = 0.0f; // 자신과 인접하는곳(8군데)높이를 다 가져와서 더한걸 여기에 담아둘거임
	    
        sectionsTotal += Update[uint2(CurPixel.x - 1, CurPixel.y)].r;
        adjacentSections++;
       

        sectionsTotal += Update[uint2(CurPixel.x - 1, CurPixel.y - 1)].r;
        adjacentSections++;
      
        
        sectionsTotal += Update[uint2(CurPixel.x - 1, CurPixel.y + 1)].r;
        adjacentSections++;

        
        sectionsTotal += Update[uint2(CurPixel.x + 1, CurPixel.y)].r;
        adjacentSections++;
      

        sectionsTotal += Update[uint2(CurPixel.x + 1, CurPixel.y - 1)].r;
        adjacentSections++;
       
            
        sectionsTotal += Update[uint2(CurPixel.x + 1, CurPixel.y + 1)].r;
        adjacentSections++;
        
        sectionsTotal += Update[uint2(CurPixel.x, CurPixel.y - 1)].r;
        adjacentSections++;
       
        sectionsTotal += Update[uint2(CurPixel.x,  (CurPixel.y + 1))].r;
        adjacentSections++;
       
       
		//자신의 높이에다가, sectionsTotal로 구한 주변높이의 평균을 가감해줘서 부드럽게
       
        Update[CurPixel.xy] = (Update[CurPixel.xy] + float4((sectionsTotal / adjacentSections), 0, 0, 0))*0.5f;
        
    }
  
  
 
}

[numthreads(Width, 1, 1)]
void GetHeight(uint3 dispatchThreadId : SV_DispatchThreadID)
{
   
    
    //float3 location = float3(hit);
   
    float l = (Location.x - Range) + Factor;
    float r = (Location.x + Range) + Factor;
    float b = ((Location.z - Range) - Factor) * -1;
    float t = ((Location.z + Range) - Factor) * -1;

    uint lb = clamp(l, 0, Width - 1);
    uint rb = clamp(r, 0, Width - 1);
    uint bb = clamp(b, 0, Width - 1);
    uint tb = clamp(t, 0, Width - 1);
       
    [branch]
    if (BrushShape == 1)
    {
        Square(uint4(lb, rb, bb, tb), dispatchThreadId.x);

    }
    else if (BrushShape == 2)
    {
        Circle(uint4(lb, rb, bb, tb), dispatchThreadId.x);

    }
       

      
  
 
    
    
}




////////////////////////////////////////////////////////////////////////////////////////////
technique11 T0
{
    pass p0
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        SetComputeShader(CompileShader(cs_5_0, UpdateHeight()));
    }

    pass p1
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        SetComputeShader(CompileShader(cs_5_0, Smoothing()));
    }

  
   
   
}
