struct Group
{
    uint index;
    uint3 id;
    float Data;
    float Data2;

};

StructuredBuffer<Group> Input; //������ ���������ΰ� ���� srv�� ���´�.
RWStructuredBuffer<Group> Output; //UAV;


struct CS_Input
{
    uint3 GroupID : SV_GroupID;//thread�� �׷�ID
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