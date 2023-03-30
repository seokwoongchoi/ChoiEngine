#pragma once

class Camera
{
public:
	Camera() :move(20), R(2), position(512, 0, 512), forward(0, 0, 1), data{}, delta(0.04f)
	{
		D3DXMatrixIdentity(&matView);
		D3DXMatrixIdentity(&proj);
	}
	virtual ~Camera()=default;

	virtual void Update() = 0;
	virtual void Move()=0;

	void Position(Vector3* pos)
	{
		*pos = position;
	}
	
protected:
	float delta;

	float move;
	float R;
	Vector3 forward;
	Vector3 position;

	Matrix matView;
	Matrix proj;

	GlobalViewData data;
};