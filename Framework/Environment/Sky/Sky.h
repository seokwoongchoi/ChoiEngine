#pragma once
class Sky
{
public:
	explicit Sky(ID3D11Device* device);
	~Sky();
	Sky(const Sky&) = delete;
	Sky& operator=(const Sky&) = delete;
public:
	void Update();
	void Render(ID3D11DeviceContext* context);

private:
	class Scattering* scattering;

};

