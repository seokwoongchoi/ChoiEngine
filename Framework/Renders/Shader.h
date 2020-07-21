#pragma once
#include <d3dx11effect.h>
#pragma comment(lib, "Effects11d.lib")
class Shader
{
public:
	friend struct Pass;
	
public:
	explicit Shader(ID3D11Device* device,const wstring& file);
	~Shader();

	Shader(const Shader& rhs) = delete;
	Shader& operator=(const Shader& rhs) = delete;

	wstring GetFile() { return file; }

	ID3DX11Effect* Effect() { return effect; }

	inline void Draw(ID3D11DeviceContext* context,UINT technique, UINT pass, UINT vertexCount, UINT startVertexLocation = 0){
		
		techniques[technique].Passes[pass].context = context;
		techniques[technique].Passes[pass].Draw(vertexCount, startVertexLocation);}
	inline void DrawIndexed(ID3D11DeviceContext* context, UINT technique, UINT pass, UINT indexCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0){
		techniques[technique].Passes[pass].context = context;
		techniques[technique].Passes[pass].DrawIndexed(indexCount, startIndexLocation, baseVertexLocation); }
	inline void DrawInstanced(UINT technique, UINT pass, UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation = 0, UINT startInstanceLocation = 0){
		techniques[technique].Passes[pass].DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation); }
	inline void DrawIndexedInstanced(UINT technique, UINT pass, UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0, UINT startInstanceLocation = 0){
		
		
		techniques[technique].Passes[pass].DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation); }
	inline void DrawInstancedIndirect(UINT technique, UINT pass, ID3D11Buffer* buffer, UINT offset = 0){
		techniques[technique].Passes[pass].DrawInstancedIndirect(buffer, offset); }
	inline void Dispatch(UINT technique, UINT pass, UINT x, UINT y, UINT z){
		techniques[technique].Passes[pass].Dispatch(x, y, z); }



	ID3DX11EffectVariable* Variable(string name);
	ID3DX11EffectScalarVariable* AsScalar(string name);
	ID3DX11EffectVectorVariable* AsVector(string name);
	ID3DX11EffectMatrixVariable* AsMatrix(string name);
	ID3DX11EffectStringVariable* AsString(string name);
	ID3DX11EffectShaderResourceVariable* AsSRV(string name);
	ID3DX11EffectRenderTargetViewVariable* AsRTV(string name);
	ID3DX11EffectDepthStencilViewVariable* AsDSV(string name);
	ID3DX11EffectUnorderedAccessViewVariable* AsUAV(string name);
	ID3DX11EffectConstantBuffer* AsConstantBuffer(string name);
	ID3DX11EffectShaderVariable* AsShader(string name);
	ID3DX11EffectBlendVariable* AsBlend(string name);
	ID3DX11EffectDepthStencilVariable* AsDepthStencil(string name);
	ID3DX11EffectRasterizerVariable* AsRasterizer(string name);
	ID3DX11EffectSamplerVariable* AsSampler(string name);


private:
	void CreateEffect();
	ID3D11InputLayout* CreateInputLayout(const ID3DBlob* fxBlob, const D3DX11_EFFECT_SHADER_DESC* effectVsDesc, const vector<D3D11_SIGNATURE_PARAMETER_DESC>& params);

private:
	ID3D11Device* device;
	wstring file;

	ID3DX11Effect* effect;
	D3DX11_EFFECT_DESC effectDesc;

private:
	struct StateBlock
	{
		ID3D11RasterizerState * RSRasterizerState;

		ID3D11BlendState* OMBlendState;
		FLOAT OMBlendFactor[4];
		UINT OMSampleMask;
		ID3D11DepthStencilState* OMDepthStencilState;
		UINT OMStencilRef;
	};
	StateBlock* initialStateBlock;

private:
	struct Pass
	{
		wstring Name;
		ID3DX11EffectPass* IPass;
		D3DX11_PASS_DESC Desc;

		ID3D11InputLayout* InputLayout;
		D3DX11_PASS_SHADER_DESC PassVsDesc;
		D3DX11_EFFECT_SHADER_DESC EffectVsDesc;
		vector<D3D11_SIGNATURE_PARAMETER_DESC> SignatureDescs;

		D3DX11_STATE_BLOCK_MASK StateBlockMask;
		StateBlock* StateBlock;
		ID3D11DeviceContext* context;
		void Draw(UINT vertexCount, UINT startVertexLocation = 0);
		
		void DrawIndexed(UINT indexCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0);
		void DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation = 0, UINT startInstanceLocation = 0);
		void DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation = 0, INT baseVertexLocation = 0, UINT startInstanceLocation = 0);
		
		void DrawInstancedIndirect(ID3D11Buffer* buffer, uint offset=0);

		void BeginDraw();
		void EndDraw();

		void IndirectBeginDraw();
		void IndirectEndDraw();

		void Dispatch(UINT x, UINT y, UINT z);
	};
	struct Technique
	{
		wstring Name;
		D3DX11_TECHNIQUE_DESC Desc;
		ID3DX11EffectTechnique* ITechnique;

		vector<Pass> Passes = {};
	};
	vector<Technique> techniques = {};

};

class Shaders
{
public:
	friend class Shader;
	struct ShaderDesc
	{
		ID3DBlob* blob;
		ID3DX11Effect* effect;
	};

	typedef pair<wstring, ShaderDesc> Pair;

public:
	static void Delete();
	static void GetEffect(wstring fileName, ID3DBlob** blob, ID3DX11Effect** effect);

private:
	static unordered_map<wstring, ShaderDesc> shaders;
};