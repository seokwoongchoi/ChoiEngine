#pragma once

class Freedom : public Camera
{
public:
	Freedom() :Camera(),  right(1, 0, 0), up(0, 1, 0),  rotation(0,0,0),
		matRotation{},X{},Y{},Z{}
	{
		Move();
		Rotation();
	}
	~Freedom() = default;
public:
	void Update() override;
	void Speed(float move, float rotation);
	
private:
	void Move() override;
	void Rotation();

private:
	Vector3 right;
	Vector3 up;
	Vector3 rotation;
	Matrix matRotation;

	Matrix X, Y, Z;
	
};