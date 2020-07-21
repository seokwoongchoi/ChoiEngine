#pragma once
#include "Framework.h"
class Loader
{
public:
	Loader() = default;
	virtual ~Loader() = default;

	virtual void Load(const wstring& file) = 0;

};

