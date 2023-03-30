#pragma once
class DebugBox
{
	friend class DebugLine;
public:
	DebugBox(ID3D11Device* device, const Vector3& boxMin,const Vector3& boxMax);
	~DebugBox();


	void Render(ID3D11DeviceContext* context, const Matrix& VP);

	

	Matrix boxWorld;

	
private:
	
	ID3D11Buffer* DebugVertexBuffer;
	ID3D11Buffer* DebugIndexBuffer;
	ID3D11Buffer* debugBuffer;

};

