struct Group
{
    uint index;
    uint3 id;
    float Data;
    float Data2;

};

StructuredBuffer<Group> Input; //렌더링 파이프라인과 별개 srv로 들어온다.
RWStructuredBuffer<Group> Output; //UAV;


struct CS_Input
{
    uint3 GroupID : SV_GroupID;//thread의 그룹ID
    uint3 GroupThreadID : SV_GroupThreadID;
    uint3 DispatchThreadID : SV_DispatchThreadID;
    uint GroupIndex : SV_GroupIndex;

    
};

[numthreads(10,8,3)]
void CS(CS_Input input)
{
    uint index = (input.GroupID.x * 240) + input.GroupIndex;
    Output[index].index = index;
    Output[index].id.x = input.DispatchThreadID.x;
    Output[index].id.y = input.DispatchThreadID.y;
    Output[index].id.z = input.DispatchThreadID.z;
    Output[index].Data = Input[index].Data;
    Output[index].Data2 = Input[index].Data * 10.0f;

    

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