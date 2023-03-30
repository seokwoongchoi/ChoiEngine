#pragma once
class ParticleSimulation
{
public:
	ParticleSimulation(ID3D11Device* device);
	~ParticleSimulation();

	virtual int InitBodies(const wstring& path) = 0;
	virtual void CreateShaders(ID3D11Device* device) = 0;
	virtual void CreateBuffers(ID3D11Device* device) = 0;


	virtual void Update(ID3D11DeviceContext* context) = 0;
	virtual void Render(ID3D11DeviceContext* context,ID3D11ShaderResourceView* positionSRV) = 0;

	virtual void PreviewRender(ID3D11DeviceContext* context, const Matrix & view, const Matrix & proj)=0;
	virtual void Reset(const uint& drawCount) = 0;
	
	void SetParticleType(const ReadParticleType& type) { this->type= type; }
	const ReadParticleType& GetParticleType() { return type; }
protected:


	ID3D11Device* device;

protected:
	ID3D11Buffer			  *StructuredBuffer;
	ID3D11ShaderResourceView  *StructuredBufferSRV;
	ID3D11UnorderedAccessView *StructuredBufferUAV;


	
protected:
	ReadParticleType type;
	ID3D11VertexShader* VS;
	ID3D11VertexShader* maintainVS;
	ID3D11VertexShader* preivewVS;
	ID3D11GeometryShader* GS;
	ID3D11PixelShader* PS;
	ID3D11ComputeShader* updateCS;

	ID3D11SamplerState* sampler;

	class Texture* particleTexture;
	int id = -1;

};

