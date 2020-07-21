#pragma once
#include "../Renders/BasicData/Vertex.h"

enum class StreamMode : uint
{
	Write, Read
};

class FileStream final
{
public:
	FileStream(const std::string& path, const StreamMode& mode);
	~FileStream();
	
    FileStream(const FileStream&) = delete;
	FileStream& operator=(const FileStream&) = delete;

    auto IsOpen() const -> const bool { return bOpen; }

	//==============================================================
	//Write
	template <typename T, typename = typename std::enable_if<
		std::is_same<T, bool>::value ||
		std::is_same<T, char>::value ||
		std::is_same<T, byte>::value ||
		std::is_same<T, int>::value ||
		std::is_same<T, uint>::value ||
		std::is_same<T, long>::value ||
		std::is_same<T, unsigned long>::value ||
		std::is_same<T, float>::value ||
		std::is_same<T, double>::value ||
        std::is_same<T, Vector2>::value ||
        std::is_same<T, Vector3>::value ||
        std::is_same<T, Vector4>::value ||
        std::is_same<T, Quaternion>::value ||
        std::is_same<T, Color>::value ||
        std::is_same<T, Matrix>::value
	>::type> void Write(const T& value)
	{
		out.write(reinterpret_cast<const char*>(&value), sizeof(T));
	}

	void Write(const std::string& value);
	void Write(const std::vector<VertexModel>& value);
	void Write(const std::vector<uint>& value);
    void Write(const std::vector<byte>& value);
    void Write(const void* value, const uint& size);
	//==============================================================


	//==============================================================
	//Read
	template <typename T, typename = typename std::enable_if<
		std::is_same<T, bool>::value ||
		std::is_same<T, char>::value ||
		std::is_same<T, byte>::value ||
		std::is_same<T, int>::value ||
		std::is_same<T, uint>::value ||
		std::is_same<T, long>::value ||
		std::is_same<T, unsigned long>::value ||
		std::is_same<T, float>::value ||
		std::is_same<T, double>::value ||
        std::is_same<T, Vector2>::value ||
        std::is_same<T, Vector3>::value ||
        std::is_same<T, Vector4>::value ||
        std::is_same<T, Quaternion>::value ||
        std::is_same<T, Color>::value ||
        std::is_same<T, Matrix>::value
	>::type> void Read(T& value)
	{
		in.read(reinterpret_cast<char*>(&value), sizeof(T));
	}

	void Read(std::string& value);
	void Read(std::vector<VertexModel>& value);
	void Read(std::vector<uint>& value);
    void Read(std::vector<byte>& value);

    template <typename T, typename = typename std::enable_if<
        std::is_same<T, bool>::value ||
        std::is_same<T, char>::value ||
        std::is_same<T, byte>::value ||
        std::is_same<T, int>::value ||
        std::is_same<T, uint>::value ||
        std::is_same<T, long>::value ||
        std::is_same<T, unsigned long>::value ||
        std::is_same<T, float>::value ||
        std::is_same<T, double>::value ||
        std::is_same<T, Vector2>::value ||
        std::is_same<T, Vector3>::value ||
        std::is_same<T, Vector4>::value ||
        std::is_same<T, Quaternion>::value ||
        std::is_same<T, Color>::value ||
        std::is_same<T, Matrix>::value
    >::type> const T Read()
    {
        T value;
        Read(value);

        return value;
    }
	//==============================================================

private:
	StreamMode streamMode;
	std::ifstream in;
	std::ofstream out;
    bool bOpen;
};
