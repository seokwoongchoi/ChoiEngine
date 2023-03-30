#include "stdafx.h"
#include "TextReader.h"

TextReader::TextReader()
{
}

TextReader::~TextReader()
{
}

void TextReader::ReadText(wstring file)
{
	GetEncoding(file);
	locale::global(locale(encoding));//encoding 설정

	wifstream ifile;
	ifile.open(file);  // 파일 열기
	
	if (ifile.is_open())
	{
		string directory = Path::GetDirectoryName(String::ToString(file));
		string output = directory + "output.txt";
		wofstream ofile(output);

		bool bfirst = false;
		if (encoding == ".UTF-8")
		{
			bfirst = true;
		}

		wstring line = L"";
		uint lineCount = 0;
		const wchar_t delimit[] = L" \t\n-_,.‘’'\"?!—“”:();";
		
		while (!ifile.eof()) // 메모장이 끝날때 까지
		{
			getline(ifile, line);//한줄 씩
			if (bfirst == true)//bom 제거
			{
				//unsigned char bom[] = { 0xEF,0xBB,0xBF };
				line = line.substr(1, line.length());
				bfirst = false;
			}
			wchar_t *token = nullptr;
			lines.emplace_back(line);
			token = wcstok(const_cast<wchar_t*>(line.c_str()), delimit);
			wstring lastWord = L"";
			uint tokenIndex = 0;
			uint lastWordLength = 0;
			while (token != nullptr)
			{
				 const wstring& temp= token;
			
				 lastWordLength = temp.length()+1;
				 wstring lower = L"";
				 for (const auto& character : temp)
					 lower += tolower(character);
						
				if (lastWord == L"un")
				{
					 lower = lastWord + lower;
				}
				lastWord = lower;
				
				wordToSentence_Indices[lower].sentences.emplace_back(make_pair(lineCount, tokenIndex));
				wordToSentence_Indices[lower].wordCount++;
				tokenIndex++;
				tokenIndex += lastWordLength;
				token = wcstok(nullptr, delimit);
				
			}
			lineCount++;
			wcout << line << endl;
			ofile << line << endl;
			
		}
		ofile.close();
		ifile.close(); // 파일 닫기
	}
	const wstring& temp = L"Alice";
	wstring select = L"";
	for (const auto& character : temp)
		select += tolower(character);

	if (wordToSentence_Indices.count(select) > 0)
	{
		for (uint i = 0; i < wordToSentence_Indices[select].sentences.size(); i++)
		{
			uint currSentenceline = wordToSentence_Indices[select].sentences[i].first;

			if (deletedLines.empty()==false&&deletedLines.back().endLine >= currSentenceline)
				continue;
			uint cursorIndex = wordToSentence_Indices[select].sentences[i].second;
			ForwardSearch(&lines[0], currSentenceline, cursorIndex);
			int a = 0;
		}

	}
	int a = 0;
}

void TextReader::GetEncoding(const wstring & file)
{
	unsigned char uniTxt[] = { 0xFF, 0xFE };// Unicode file header
	unsigned char endianTxt[] = { 0xFE, 0xFF };// Unicode big endian file header
	unsigned char utf8Txt[] = { 0xEF, 0xBB };// UTF_8 file header
	DWORD dwBytesRead = 0;

	HANDLE hFile = CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		hFile = NULL;
		CloseHandle(hFile);
		throw runtime_error("cannot open file");
	}


	BYTE *lpHeader = new BYTE[2];
	ReadFile(hFile, lpHeader, 2, &dwBytesRead, NULL);
	CloseHandle(hFile);

	

	if (lpHeader[0] == uniTxt[0] && lpHeader[1] == uniTxt[1])// Unicode file 
		encoding = ".Unicode";// CHARACTER_ENCODING::Unicode;
	else if (lpHeader[0] == endianTxt[0] && lpHeader[1] == endianTxt[1])// Unicode big endian file 
		encoding = ".Unicode";//CHARACTER_ENCODING::Unicode_big_endian;
	else if (lpHeader[0] == utf8Txt[0] && lpHeader[1] == utf8Txt[1])// UTF-8 file
		encoding = ".UTF-8";//CHARACTER_ENCODING::UTF8_with_BOM;
	else
		encoding = ".ANSI";// CHARACTER_ENCODING::ANSI; //Ascii 

	SafeDelete(lpHeader);
	
	return ;
}

bool TextReader::ForwardSearch(wstring * lines, uint lineIndex, uint selectedWordPos)
{
	const wstring& delimiters = L"\"‘'“(.";
	uint pos = string::npos;
	uint currLine = lineIndex;
	
	uint lastFind = 0;
	if (currLine == 100)
	{
		int a = 0;
	}
	while (pos == string::npos)
	{
		
		for (uint i = 0; i < delimiters.size(); i++)
		{
			pos = lines[currLine].find_first_of(delimiters[i], lastFind);
			if (pos != string::npos&&pos< selectedWordPos)
			{
				if (BackwardSearch(lines, currLine, i, selectedWordPos, lineIndex))
				{
					deletedLines.back().startLine = currLine;
					deletedLines.back().startIndex = pos;
					return true;
				}
				lastFind = pos + 1;
				currLine++;
				pos = string::npos;
			}
		}
		if (currLine > 0)
		{
			currLine--;
			if (lines[currLine].empty() == true)
			{
				currLine++;
				DeleteData temp;
				temp.startLine = currLine;
				temp.startIndex = 0;
				deletedLines.emplace_back(temp);
				if (BackwardSearch(lines, currLine, 5, selectedWordPos, lineIndex))
					return true;
				else
				{
					return false;
				}
			}

		}
		else
		{
			DeleteData temp;
			
			temp.startLine = currLine;
			temp.startIndex = 0;
			deletedLines.emplace_back(temp);
			if (BackwardSearch(lines, currLine, 5, selectedWordPos, lineIndex))
				return true;
			else
			{
				return false;
			}
		}
		
		
	}
	return false;
}
bool TextReader::BackwardSearch(wstring * lines, uint lineIndex, uint delimiterIndex, uint selectedWordPos, uint oLine)
{
	const wstring& delimiters = L"\"’'”).";

	uint searchLine = lineIndex;
	uint pos = string::npos;
	bool bNeedData = false;
	while (pos == string::npos)
	{
		
		pos = lines[searchLine].find_last_of(delimiters[delimiterIndex]);
		if (pos != string::npos)
		{
			if (searchLine < oLine ||
					searchLine == oLine&& pos <= selectedWordPos)
				{
				    bNeedData = true;
				    int a = 0;
					
				}
			else
			{
				deletedLines.back().endLine = searchLine;
				deletedLines.back().endIndex = pos;
				/*DeleteData temp;
				temp.endLine = searchLine;
				temp.endIndex = pos;
				deletedLines.emplace_back(temp);*/
				return true;
			
			}
			
			
		}

		if (lines[++searchLine].empty() == true)
		{
			searchLine--;
			if (bNeedData)
			{
				DeleteData temp;
				temp.endLine = searchLine;
				temp.endIndex = lines[searchLine].length();
				deletedLines.emplace_back(temp);
				bNeedData = false;
			}
			else
			{
				deletedLines.back().endLine = searchLine;
				deletedLines.back().endIndex = lines[searchLine].length();
			}
			
			return true;
		}
		
	}
	return false;
}

