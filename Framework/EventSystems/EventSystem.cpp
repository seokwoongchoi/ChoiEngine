#include "Framework.h"
#include "EventSystem.h"

EventSystem::EventSystem(ID3D11Device* device)
	:animator(nullptr), collider(nullptr), effects(nullptr), physics(nullptr)
{
	
}

EventSystem::~EventSystem()
{
}
