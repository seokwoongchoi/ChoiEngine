#pragma once
#define MAX_LINE_VERTEX 50000

class DebugLine
{


public:
	static void Create();
	static void Delete();

	static DebugLine* Get();

public:
	
	void RenderLine(Vector3& start, Vector3& end);
	void RenderLine(Vector3& start, Vector3& end, float r, float g, float b);

	void RenderLine(float x, float y, float z, float x2, float y2, float z2);
	void RenderLine(float x, float y, float z, float x2, float y2, float z2, D3DXCOLOR& color);
	void RenderLine(float x, float y, float z, float x2, float y2, float z2, float r, float g, float b);

	void RenderLine(Vector3& start, Vector3& end, Color& color,uint ID=0);
	void RenderLine(const Vector3& start, const Vector3& end, uint ID = 0);
	
	
	inline void SRV(ID3D11ShaderResourceView  *srv)
	{
		this->srv = srv;
	}

	
	void Initiallize(ID3D11Device* device);
	void PreviewRender(ID3D11DeviceContext* context,const Matrix& matrix);
	void Render(ID3D11DeviceContext* context);
	void BoneBoxRender(ID3D11DeviceContext* context,ID3D11ShaderResourceView* srv);


private:
	explicit DebugLine();
	~DebugLine();
	
	DebugLine(const DebugLine&) = delete;
	DebugLine& operator=(const DebugLine&) = delete;

	
private:
	static DebugLine* instance;

private:
	
	uint drawCount;
	ID3D11Buffer* DebugVertexBuffer;
	vector<VertexColor> vertices;
	
private:
	ID3D11DepthStencilState*  DepthStencil;
	

	ID3D11ShaderResourceView  *srv;
	
private:
	ID3D11VertexShader*   DebugVS;
	ID3D11VertexShader*   BoneBoxVS;
	ID3D11PixelShader*    DebugPS;
	class InputLayout* debugInputLayout;


	ID3D11Buffer* debugBuffer;

};
