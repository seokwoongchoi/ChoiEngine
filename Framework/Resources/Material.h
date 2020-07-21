#pragma once
#define MAX_MODEL_TEXTURE 4
class Material
{
public:
	Material(ID3D11Device* device);
	~Material();

	Material(const Material&) = delete;
	Material& operator=(const Material&) = delete;

	void ApplyMaterial(ID3D11DeviceContext* context);
	void ApplyRefeltionMaterial(ID3D11DeviceContext* context);
	void ClearMaterial(ID3D11DeviceContext* context);
	void ClearReflectionMaterial(ID3D11DeviceContext* context);
public:
	void Name(const wstring& val) { name = val; }
	wstring& Name() { return name; }

public:
	
	void Ambient(const Color& color) {  }
	Color Diffuse() { return colorDesc.Diffuse; }
	void Diffuse(const Color& color) { colorDesc.Diffuse = color; }
	Color Specular() { return colorDesc.Specular; }
	void Specular(const Color& color) { colorDesc.Specular = color; }
	float Roughness() { return  colorDesc.Diffuse.a; }
	void Roughness(const float val) { colorDesc.Diffuse.a = val; }
	float Metallic() { return colorDesc.Specular.a; }
	void Metallic(float val) {  colorDesc.Specular.a = val; }
public:
	void DiffuseMap(wstring file) {  textures[0].Load(device,file); }
	void NormalMap(wstring file) {  textures[1].Load(device,file); }
	void SpecularMap(wstring file) { }
	void RoughnessMap(wstring file) {  textures[2].Load(device,file); }
	void MatallicMap(wstring file) {  textures[3].Load(device,file); }
	

	Texture DiffuseMap() const { return  textures[0]; }
	Texture NormalMap() const { return  textures[1]; }
	Texture RoughnessMap() const { return textures[2]; }
	Texture MatallicMap()const { return textures[3]; }
private:
	wstring name;
private:
	ID3D11Device* device;
	Texture* textures;
	ID3D11ShaderResourceView* arrSRV[5];
	ID3D11ShaderResourceView* srvArray[2];


private:
	struct ColorDesc
	{
		
		Color Diffuse = Color(1, 1, 1, 1);
	    Color Specular = Color(1, 1, 1, 1);

	
	} colorDesc;

	ID3D11Buffer* colorBuffer;
	ID3D11Buffer* nullBuffer;
};

