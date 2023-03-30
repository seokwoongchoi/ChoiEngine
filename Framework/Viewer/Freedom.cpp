#include "Framework.h"
#include "Freedom.h"


void Freedom::Update()
{
	
	
		
	if (Keyboard::Get()->Press('W'))
		movement_speed += forward * acceleration*move *delta;
	else if (Keyboard::Get()->Press('S'))
		movement_speed -= forward * acceleration*move *delta;

	if (Keyboard::Get()->Press('D'))
		movement_speed += right * acceleration*move *delta;
	else if (Keyboard::Get()->Press('A'))
		movement_speed -= right * acceleration*move *delta;
	if (Keyboard::Get()->Press('E'))
		movement_speed += up * acceleration*move *delta;
	else if (Keyboard::Get()->Press('Q'))
		movement_speed -= up * acceleration*move *delta;

	if (Mouse::Get()->Press(1) == true)
	{
		auto& moveValue = Mouse::Get()->GetMoveValue();

		rotation.x += moveValue.y*R* delta;
		rotation.y += moveValue.x*R* delta;
	

		Rotation();
	}
	
	position += movement_speed;
	
	movement_speed *= drag * (1.0f - Time::Delta());

	Move();
	
}

void Freedom::Move()
{
	D3DXMatrixLookAtLH(&matView, &position, &(position + forward), &Vector3(0, 1, 0));

	float aspect = static_cast<float>(D3D::Width()) / static_cast<float>(D3D::Height());
	D3DXMatrixPerspectiveFovLH(&proj, static_cast<float>(D3DX_PI)* 0.35f, aspect, 0.1f, 1000.0f);
	data.view = matView;
	data.proj = proj;
	data.pos = position;
	data.dir = forward;
	data.lookAt = position + forward;
	GlobalData::SetWorldViewData(&data);
}

void Freedom::Rotation()
{
	
	D3DXMatrixRotationX(&X, rotation.x);
	D3DXMatrixRotationY(&Y, rotation.y);
	D3DXMatrixRotationZ(&Z, rotation.z);


	matRotation = X * Y*Z;

	D3DXVec3TransformNormal(&forward, &Vector3(0, 0, 1), &matRotation);
	D3DXVec3TransformNormal(&right, &Vector3(1, 0, 0), &matRotation);
	D3DXVec3TransformNormal(&up, &Vector3(0, 1, 0), &matRotation);
}

void Freedom::Speed(float move, float rotation)
{
	this->move = move;
	this->R = rotation;
}
