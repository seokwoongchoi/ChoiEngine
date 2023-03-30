#pragma once
enum class  CHARACTER_ENCODING 
{
	ANSI, 
	Unicode, 
	Unicode_big_endian, 
	UTF8_with_BOM, 
	UTF8_without_BOM
};

struct DeleteData
{
	uint startLine = 0;
	uint startIndex = 0;

	uint endLine = 0;
	uint endIndex = 0;
};
struct Data
{
	vector<pair<uint,uint>> sentences;
	uint wordCount=0;
};
class TextReader
{
public:
	TextReader();
	~TextReader();

	void ReadText(wstring file);

private:
	
	void GetEncoding(const wstring& file);
	//bool ForwardSearch(wstring* lines,uint lineIndex,uint selectedWordPos);
	//bool BackwardSearch(wstring* lines, uint lineIndex,uint delimiterIndex, uint selectedWordPos, uint oLine);
	bool ForwardSearch(wstring* lines, uint lineIndex, uint selectedWordPos);
	bool BackwardSearch(wstring* lines, uint lineIndex, uint delimiterIndex, uint selectedWordPos, uint oLine);
private:
	unordered_map < wstring, Data> wordToSentence_Indices;
	vector <wstring> lines;//onesentences;
	
	string encoding = "";
	vector<DeleteData>deletedLines;
	
};


