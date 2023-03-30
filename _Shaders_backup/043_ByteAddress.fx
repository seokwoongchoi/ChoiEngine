ByteAddressBuffer Input; //������ ���������ΰ� ���� srv�� ���´�.

RWByteAddressBuffer Output; //UAV;

struct Group
{
    uint3 GroupID;
    uint3 GroupThreadID;
    uint3 DispatchThreadID;
    uint GroupIndex;

    //float Random;
    //float RandomOutput;
};

struct CS_Input
{
    uint GroupID : SV_GroupID;//thread�� �׷�ID
    uint GroupThreadID : SV_GroupThreadID;
    uint DispatchThreadID : SV_DispatchThreadID;
    uint GroupIndex : SV_GroupIndex;

    
};

[numthreads(10,8,3)]
void CS(CS_Input input)
{
    Group group;
    group.GroupID = asint(input.GroupID); //byte Address�� ���� int������ ĳ����������Ѵ�.
    group.GroupThreadID = asint(input.GroupThreadID);
    group.DispatchThreadID = asint(input.DispatchThreadID);
    group.GroupIndex = asint(input.GroupIndex);

    uint groupSizeX = input.GroupID.x*240+input.GroupIndex ;
    
    uint fatchAddress = groupSizeX * 10 * 4;
    //group.Random = asfloat(Input.Load(fatchAddress+40.0f));
    //group.RandomOutput = group.Random * 10.0f;

    Output.Store3(fatchAddress + 0, asuint(group.GroupID));
    Output.Store3(fatchAddress + 12, asuint(group.GroupThreadID));
    Output.Store3(fatchAddress + 24, asuint(group.DispatchThreadID));
    Output.Store(fatchAddress + 36, asuint(group.GroupIndex));
    //Output.Store (fatchAddress +  40, asuint(group.Random));
    //������������ �ݵ�� int������ �������Ѵ�.
   // Output.Store(fatchAddress + 44, asuint(group.Random));

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