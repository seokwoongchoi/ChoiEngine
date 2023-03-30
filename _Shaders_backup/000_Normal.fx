Matrix World;
Matrix View;
Matrix Projection;

float4 Color;


struct VertexInput
{
    float4 Position : Position0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
};

struct VertexOutput
{
    float4 Position : SV_Position0;
    float3 Normal : Normal0;
    float3 Tangent : Tangent0;
};

VertexOutput VS(VertexInput input)
{
    //WVP ��ȯ�� VS����
    VertexOutput output;
    //mul : ��İ����� ���Ͱ�
    output.Position = mul(input.Position, World);
    output.Position = mul(output.Position, View);
    output.Position = mul(output.Position, Projection);
    
    output.Tangent = input.Tangent;
    output.Normal =input.Normal; //�븻�� ���庯ȯ�ϴ����� ȸ���Ѹ�ŭ ������ �ٽ� �����ϱ�����
    //��ġ���� �ʿ�����Ƿ� 3x3��ķ� ���庯ȯ
    return output;
}

float4 PS(VertexOutput input) : SV_Target0
{
    return float4(input.Normal, 1);
}

RasterizerState RS
{
    FillMode = Wireframe;

};

technique11 T0
{
    pass P0
    {
        SetRasterizerState(RS);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }

    pass P1
    {
       
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}