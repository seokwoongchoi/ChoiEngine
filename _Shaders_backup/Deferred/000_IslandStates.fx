

cbuffer QuadObject
{
    const float2 QuadVertices[4] =
    {
        { -1.0, -1.0 },
        { 1.0, -1.0 },
        { -1.0, 1.0 },
        { 1.0, 1.0 }
    };

    const float2 QuadTexCoordinates[4] =
    {
        { 0.0, 1.0 },
        { 1.0, 1.0 },
        { 0.0, 0.0 },
        { 1.0, 0.0 }
    };
}



/////////////////////////////////////states/////////////////////////////
SamplerState SamplerPointClamp
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState SamplerLinearClamp
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState SamplerLinearWrap
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState SamplerAnisotropicWrap
{
    Filter = ANISOTROPIC;
    AddressU = Wrap;
    AddressV = Wrap;
    MaxAnisotropy = 16;
};

SamplerState SamplerCube
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

SamplerState SamplerLinearMirror
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Mirror;
    AddressV = Mirror;
};

SamplerState SamplerLinearBorderBlack
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
    AddressV = Border;
    AddressW = Border;
    BorderColor = float4(0, 0, 0, 0);
};

SamplerComparisonState SamplerBackBufferDepth
{
    Filter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    AddressU = Border;
    AddressV = Border;
    BorderColor = float4(1, 1, 1, 1);
    ComparisonFunc = LESS_EQUAL;
};

SamplerComparisonState SamplerDepthAnisotropic
{
    Filter = COMPARISON_ANISOTROPIC;
    AddressU = Border;
    AddressV = Border;
    ComparisonFunc = LESS;
    BorderColor = float4(1, 1, 1, 1);
    MaxAnisotropy = 16;
};

RasterizerState CullBack
{
    CullMode = Back;
    FrontCounterClockwise = TRUE;
};

RasterizerState CullBackMS
{
    CullMode = Back;
    FrontCounterClockwise = TRUE;
  //  MultisampleEnable = TRUE;
};

RasterizerState CullFrontNoClip
{
    CullMode = Front;
    FrontCounterClockwise = TRUE;
    DepthClipEnable = FALSE;
};

RasterizerState CullFrontMS
{
    CullMode = Front;
    FrontCounterClockwise = TRUE;
    MultisampleEnable = TRUE;
};

RasterizerState NoCull
{
    CullMode = NONE;
};

RasterizerState NoCullMS
{
    CullMode = NONE;
   // MultisampleEnable = TRUE;
};

RasterizerState Wireframe
{
    CullMode = NONE;
    FillMode = WIREFRAME;
};

RasterizerState WireframeMS
{
    CullMode = NONE;
    FillMode = WIREFRAME;
    MultisampleEnable = TRUE;
};

DepthStencilState DepthNormal
{
    DepthFunc = LESS_EQUAL;
};

DepthStencilState NoDepthStencil
{
    DepthEnable = FALSE;
};

DepthStencilState ReadDepthNoStencil
{
    DepthWriteMask = ZERO;
};

DepthStencilState DepthShadingPass
{
    DepthFunc = EQUAL;
    DepthWriteMask = ZERO;
};

BlendState NoBlending
{
    BlendEnable[0] = FALSE;
};

BlendState BlendingAdd
{
    BlendEnable[0] = TRUE;
    SrcBlend = ONE;
    DestBlend = ONE;
    BlendOp = ADD;
    SrcBlendAlpha = ZERO;
    DestBlendAlpha = DEST_ALPHA;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

BlendState AlphaBlending
{
    BlendEnable[0] = TRUE;
    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;
    SrcBlendAlpha = ZERO;
    DestBlendAlpha = DEST_ALPHA;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};
//BlendState blendState
//{
  
//    BlendEnable[0] = true;
//    DestBlend[0] = one;
  
//    SrcBlend[0] = ONE;
//    BlendOp[0] = Add;
//    DestBlendAlpha[0] = ONE;
  
//    SrcBlendAlpha[0] = ONE;
//    BlendOpAlpha[0] = Add;
//    RenderTargetWriteMask[0] = 15;
//};