#pragma once
class Orbit : public Camera
{
public:
	Orbit():Camera(), targetPosition(0,0,0), deltaPos(5,5,5), R(0.375414f, -0.562102f)
	{
		Move();
	}
	~Orbit() = default;
public:
	inline void SetTargetPosition(const Vector3& targetPosition)
	{
		this->targetPosition = targetPosition;
	}

	inline void SetDeltaPosition(const Vector3& deltaPos)
	{
		this->deltaPos = deltaPos;
	}

	void PreviewUpdate();

public:
	void Update() override;
private:
	void Move() override;

private:

	
	Vector3 targetPosition;
	Vector3 deltaPos;

	Vector2 R;
};

