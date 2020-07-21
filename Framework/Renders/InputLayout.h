#pragma once

class InputLayout final
{
public:
	InputLayout();
	~InputLayout();

	InputLayout(const InputLayout& rhs) = delete;
	InputLayout& operator=(const InputLayout& rhs) = delete;

	inline operator ID3D11InputLayout*() { return inputLayout; }

	ID3D11InputLayout* GetLayout() { return inputLayout; }

	void Create
	(
		ID3D11Device* device,
		D3D11_INPUT_ELEMENT_DESC* descs,
		const uint& count,
		ID3DBlob* blob
	);
	void Create(ID3D11Device* device, ID3DBlob* blob);
	void Clear();
	

private:
	
	ID3D11InputLayout* inputLayout;
};