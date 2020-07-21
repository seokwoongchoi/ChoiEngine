#pragma once

class Camera
{
public:
	Camera() :move(20), R(2), position(0, 0, 0), forward(0, 0, 1), data{}, delta(0.008f)
	{
		D3DXMatrixIdentity(&matView);
		D3DXMatrixIdentity(&proj);
	}
	virtual ~Camera()=default;

	virtual void Update() = 0;
	virtual void Move()=0;

	
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