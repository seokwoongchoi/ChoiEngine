#include "Framework.h"
#include "Sky.h"
#include "Scattering.h"
Sky::Sky(ID3D11Device* device)
{
	scattering = new Scattering(device);
}


Sky::~Sky()
{
	
}

void Sky::Update()
{
}

void Sky::Render(ID3D11DeviceContext* context)
{
	scattering->Render(context);
}
