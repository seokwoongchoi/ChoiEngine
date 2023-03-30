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

	float Roughness() { return  colorDesc.Diffuse.a; }
	void Roughness(const float val) { colorDesc.Diffuse.a = val; }
	float Metallic() { return colorDesc.Specular.a; }
	void Metallic(float val) {  colorDesc.Specular.a = val; }
public:
	void DiffuseMap(wstring file,bool isInclude=false) 
	{ 
		diffuseFile = Path::GetFileName(file);
		textures[0].Load(device,file, nullptr);
	}
	void NormalMap(wstring file, bool isInclude = false)
	{
		normalFile = Path::GetFileName(file);
		textures[1].Load(device,file, nullptr); 
	}

	void RoughnessMap(wstring file, bool isInclude = false)
	{
		roughnessFile = Path::GetFileName(file);
		textures[2].Load(device,file, nullptr); 
	}
	void MetallicMap(wstring file, bool isInclude = false) 
	{
		metallicFile = Path::GetFileName(file);
		textures[3].Load(device,file,nullptr); 
	}

	/*void HeightMap(wstring& file, bool isInclude = false)
	{
		heightFile = Path::GetFileName(file);
		textures[4].Load(device, file, nullptr, isInclude);
	}*/
	

	Texture& DiffuseMap() const { return  textures[0]; }
	Texture& NormalMap() const { return  textures[1]; }
	Texture& RoughnessMap() const { return textures[2]; }
	Texture& MetallicMap()const { return textures[3]; }
	//Texture& HeightMap()const { return textures[4]; }
	
	wstring& DiffuseFile()  { return  diffuseFile; }
	wstring& NormalFile()  { return normalFile; }
	wstring& RoughnessFile()  { return roughnessFile; }
	wstring& MetallicFile() { return metallicFile; }
	//wstring& HeightFile() { return heightFile; }
private:
	wstring name;
private:
	ID3D11Device* device;
	Texture* textures;
	ID3D11ShaderResourceView* arrSRV[MAX_MODEL_TEXTURE];
	ID3D11ShaderResourceView* srvArray[2];

	wstring diffuseFile;
	wstring normalFile;
	wstring roughnessFile;
	wstring metallicFile;
	//wstring heightFile;

private:
	struct ColorDesc
	{
		
		Color Diffuse = Color(1, 1, 1, 1);
	    Color Specular = Color(1, 1, 1, 1);
	

	
	} colorDesc;

	ID3D11Buffer* colorBuffer;
	ID3D11Buffer* nullBuffer;
};

