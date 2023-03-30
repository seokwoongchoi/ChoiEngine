struct InputDesc
{
    uint index;//삼각형 번호
    float3 V0;
    float3 V1;
    float3 V2;


};

StructuredBuffer<InputDesc> Input; //렌더링 파이프라인과 별개 srv로 들어온다.

struct OutputDesc
{
    uint Picked;
    float U;
    float V;
    float Distance;
};

RWStructuredBuffer<OutputDesc> Output; //UAV;


cbuffer CB_Ray
{
    float3 Position;
    float3 Padding;

    float3 Direction;
    
};
void Intersection(uint index)
{
    float3 A = Input[index].V0;
    float3 B = Input[index].V1;
    float3 C = Input[index].V2;

    float3 e1 = B - A;
    float3 e2 = C - A;
    
    float3 P, T, Q;
    P = cross(Direction, e2);
    float d = 1.0f / dot(e1, P);
    T = Position - A;
    Output[index].U = dot(T, P) * d;
    Q = cross(T, e1);
    Output[index].V = dot(Direction, Q) * d;
    Output[index].Distance = dot(e2, Q) * d;

    bool b = (Output[index].U >= 0.0f) &&
                (Output[index].V >= 0.0f) &&
                (Output[index].U + Output[index].V <= 1.0f) &&
                (Output[index].Distance >= 0.0f);
    
    Output[index].Picked = b ? 1 : 0;

    //Output[index].U = index;
    //Output[index].V = Input[index].V0.x;
    //Output[index].Distance = Input[index].V0.z;

}
[numthreads(1000, 1, 1)]
void CS(uint GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
    uint index = (GroupID.x * 1000) + GroupIndex;

    //

    Intersection(index);
}


technique11 T0
{
      pass p0
     {
        SetVertexShader(NULL);

        SetPixelShader(NULL);

        SetComputeShader(CompileShader(cs_5_0, CS()));
    }
}