

/////////////////////////////////////states/////////////////////////////


RasterizerState RS
{
   // FillMode = WireFrame;
    CullMode = Front;
};


DepthStencilState SkyDSS
{
    DepthEnable = false;
    
};

BlendState skyBlendState
{
  
    BlendEnable[0] = true;
    BlendOp[0] = Add;
  
    SrcBlend[0] = ONE;
    SrcBlendAlpha[0] = ONE;
    DestBlend[0] = one;
    DestBlendAlpha[0] = ONE;
  
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;
};

RasterizerState terrainRS
{
    FillMode = WireFrame;
    //CullMode = Front;
};

RasterizerState cascadedRS
{
    CullMode = front;
    DepthBias =6;
    SlopeScaledDepthBias = 3.0f;
    DepthClipEnable = false;
};

DepthStencilState cascadedDSSLess
{
    DepthEnable = true;
    DepthWriteMask = All;
    DepthFunc = Less;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;
   // DepthFunc = Less;
   
};
DepthStencilState cascadedDSSGreater
{
    DepthEnable = true;
    DepthWriteMask = All;
    DepthFunc = Greater_Equal;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;
   // DepthFunc = Less;
   
};



DepthStencilState pointDSS
{
    DepthEnable = true;
    DepthWriteMask = Zero;
    DepthFunc = Greater_Equal;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;
 
   
};

DepthStencilState spotDSS
{
    DepthEnable = true;
    DepthWriteMask = Zero;
    DepthFunc = Greater_Equal;

   // DepthFunc = Less_Equal;

    StencilEnable = true;
    StencilReadMask = 0x00;
    StencilWriteMask = 0x00;

    //FrontFaceStencilFunc = Equal;
    //FrontFaceStencilPass = Keep;
    //FrontFaceStencilDepthFail = Keep;
    //FrontFaceStencilFail = Keep;

    //BackFaceStencilFunc = Equal;
    //BackFaceStencilPass = Keep;
    //BackFaceStencilDepthFail = Keep;
    //BackFaceStencilFail = Keep;

 
   
};
BlendState SpotblendState
{
    BlendEnable[0] = true;
    DestBlend[0] = ONE;
//SRC_COLOR;
    SrcBlend[0] = ONE;
    BlendOp[0] = Add;

    DestBlendAlpha[0] = ONE;
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;
};


DepthStencilState reflectionDSS
{
    DepthEnable = true;
    DepthWriteMask = Zero;
    DepthFunc = Less_Equal;

    StencilEnable = false;
   
 
   
};

RasterizerState OceanRS
{
   // FillMode = WireFrame;
    CullMode = None;
    //FrontCounterClockwise = false;
    DepthBias = 0;
    SlopeScaledDepthBias = 0.0f;
    DepthBiasClamp = 0.0f;
    DepthClipEnable = true;
    ScissorEnable = false;
    MultisampleEnable = true;
    AntialiasedLineEnable = false;
};
BlendState OceanblendState
{
  
    BlendEnable[0] = false;
    DestBlend[0] = SRC_COLOR;
//SRC_COLOR;
    SrcBlend[0] = ONE;
    BlendOp[0] = Add;

    DestBlendAlpha[0] = ONE;
    SrcBlendAlpha[0] = Zero;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;
};
BlendState DestZeroblendState
{

    BlendEnable[0] = true;
    SrcBlendAlpha[0] = ONE;
    SrcBlend[0] = One;
   

    DestBlend[0] = SRC_COLOR;
    DestBlendAlpha[0] = zero;

    BlendOp[0] = Add;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;
};

DepthStencilState NoDepthWLessDSS
{
    DepthEnable = true;
    DepthWriteMask = Zero;
    DepthFunc = Less;

    StencilEnable = true;

    FrontFaceStencilFunc = Equal;
    FrontFaceStencilPass = Keep;
    FrontFaceStencilDepthFail = Keep;
    FrontFaceStencilFail = Keep;

    BackFaceStencilFunc = Equal;
    BackFaceStencilPass = Keep;
    BackFaceStencilDepthFail = Keep;
    BackFaceStencilFail = Keep;
};
DepthStencilState DepthMarkDSS
{
    DepthEnable = true;
    DepthWriteMask = All;
    DepthFunc = Less;

    StencilEnable = true;
    StencilReadMask = 0xff;
    StencilWriteMask = 0xff;

    FrontFaceStencilFunc = Always;
    FrontFaceStencilPass = Replace;
    FrontFaceStencilDepthFail = Replace;
    FrontFaceStencilFail = Replace;

    BackFaceStencilFunc = Always;
    BackFaceStencilPass = Replace;
    BackFaceStencilDepthFail = Replace;
    BackFaceStencilFail = Replace;
};

BlendState TransparencyblendState
{
  
    BlendEnable[0] = true;
    //DestBlend[0] = SRC_COLOR;
    DestBlend[0] = ONE;
    //SRC_COLOR;
    SrcBlend[0] = SRC_COLOR;
   // SrcBlend[0] = ONE;
    BlendOp[0] = Add;
    DestBlendAlpha[0] = ONE;
    //SrcBlendAlpha[0] = Zero;
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;
};

BlendState ForwardblendState
{
  
    BlendEnable[0] = true;
    //DestBlend[0] = SRC_COLOR;
    DestBlend[0] = ONE;
    //SRC_COLOR;
    //SrcBlend[0] = SRC_COLOR;
    SrcBlend[0] = ONE;
    BlendOp[0] = Add;
    DestBlendAlpha[0] = ONE;
    //SrcBlendAlpha[0] = Zero;
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;
};
BlendState blendState
{
  
    BlendEnable[0] = true;
    DestBlend[0] = one;
  
    SrcBlend[0] = ONE;
    BlendOp[0] = Add;
    DestBlendAlpha[0] = ONE;
  
    SrcBlendAlpha[0] = ONE;
    BlendOpAlpha[0] = Add;
    RenderTargetWriteMask[0] = 15;
};
