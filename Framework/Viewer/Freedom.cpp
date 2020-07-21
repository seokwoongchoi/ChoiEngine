#include "Framework.h"
#include "Freedom.h"


void Freedom::Update()
{
	if (Mouse::Get()->Press(1) == false)
		return;

	
	

	if (Keyboard::Get()->Press('W'))
		position += forward * move *delta;
	else if (Keyboard::Get()->Press('S'))
		position -= forward * move * delta;

	if (Keyboard::Get()->Press('D'))
		position += right * move * delta;
	else if (Keyboard::Get()->Press('A'))
		position -= right * move * delta;

	if (Keyboard::Get()->Press('E'))
		position += up * move * delta;
	else if (Keyboard::Get()->Press('Q'))
		position -= up * move * delta;

	

	
	
	Vector3 val = Mouse::Get()->GetMoveValue();
	rotation.x += val.y*R*delta;
	rotation.y += val.x*R*delta;
	rotation.z = 0.0f;
	Rotation();

	Move();
}

void Freedom::Move()
{
	D3DXMatrixLookAtLH(&matView, &position, &(position + forward), &Vector3(0, 1, 0));

	float aspect = static_cast<float>(D3D::Width()) / static_cast<float>(D3D::Height());
	D3DXMatrixPerspectiveFovLH(&proj, static_cast<float>(D3DX_PI)* 0.25f, aspect, 0.1f, 1000.0f);
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
