ByteAddressBuffer Input; //렌더링 파이프라인과 별개 srv로 들어온다.

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
    uint GroupID : SV_GroupID;//thread의 그룹ID
    uint GroupThreadID : SV_GroupThreadID;
    uint DispatchThreadID : SV_DispatchThreadID;
    uint GroupIndex : SV_GroupIndex;

    
};

[numthreads(10,8,3)]
void CS(CS_Input input)
{
    Group group;
    group.GroupID = asint(input.GroupID); //byte Address를 쓸때 int형으로 캐스팅해줘야한다.
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
    //내보낼때에도 반드시 int형으로 나가야한다.
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