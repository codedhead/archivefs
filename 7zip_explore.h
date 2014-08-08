#ifndef _7ZIP_EXPLORE_H_
#define _7ZIP_EXPLORE_H_

#include "lib7zip/lib7zip.h"

//#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <iostream>

class TestInStream : public C7ZipInStream
{
private:
	FILE * m_pFile;
	wstring m_strFileName;
	wstring m_strFileExt;
	int m_nFileSize;
public:
	TestInStream(wstring fileName) :
	  m_strFileName(fileName),
		  m_strFileExt(L"7z")
	  {

		  wprintf(L"fileName.c_str(): %s\n", fileName.c_str());
		  m_pFile = _wfopen(fileName.c_str(), L"rb");
		  if (m_pFile) {
			  fseek(m_pFile, 0, SEEK_END);
			  m_nFileSize = ftell(m_pFile);
			  fseek(m_pFile, 0, SEEK_SET);

			  int pos = m_strFileName.find_last_of(L".");

			  if (pos != m_strFileName.npos) {
#ifdef _WIN32
				  wstring tmp = m_strFileName.substr(pos + 1);
// 				  int nLen = MultiByteToWideChar(CP_ACP, 0, tmp.c_str(), -1, NULL, NULL);
// 				  LPWSTR lpszW = new WCHAR[nLen];
// 				  MultiByteToWideChar(CP_ACP, 0, 
// 					  tmp.c_str(), -1, lpszW, nLen);
				  m_strFileExt = tmp;
#else
				  m_strFileExt = L"7z";
#endif
			  }
			  wprintf(L"Ext:%ls\n", m_strFileExt.c_str());
		  }
		  else {
			  wprintf(L"fileName.c_str(): %s cant open\n", fileName.c_str());
		  }
	  }

	  virtual ~TestInStream()
	  {
		  fclose(m_pFile);
	  }

public:
	virtual wstring GetExt() const
	{
		wprintf(L"GetExt:%ls\n", m_strFileExt.c_str());
		return m_strFileExt;
	}

	virtual int Read(void *data, unsigned int size, unsigned int *processedSize)
	{
		if (!m_pFile)
			return 1;

		int count = fread(data, 1, size, m_pFile);
		wprintf(L"Read:%d %d\n", size, count);

		if (count >= 0) {
			if (processedSize != NULL)
				*processedSize = count;

			return 0;
		}

		return 1;
	}

	virtual int Seek(__int64 offset, unsigned int seekOrigin, unsigned __int64 *newPosition)
	{
		if (!m_pFile)
			return 1;

		int result = fseek(m_pFile, (long)offset, seekOrigin);

		if (!result) {
			if (newPosition)
				*newPosition = ftell(m_pFile);

			return 0;
		}

		return result;
	}

	virtual int GetSize(unsigned __int64 * size)
	{
		if (size)
			*size = m_nFileSize;
		return 0;
	}
};

class TestOutStream : public C7ZipOutStream
{
private:
	FILE * m_pFile;
	wstring m_strFileName;
	wstring m_strFileExt;
	int m_nFileSize;
public:
	TestOutStream(wstring fileName) :
	  m_strFileName(fileName),
		  m_strFileExt(L"7z")
	  {
		  wprintf(L"OutStream ctor\n");
		  m_nFileSize=0;
		  return;
		  m_pFile = _wfopen(fileName.c_str(), L"wb");
		  m_nFileSize = 0;

		  int pos = m_strFileName.find_last_of(L".");

		  if (pos != m_strFileName.npos)
		  {
#ifdef _WIN32
			  wstring tmp = m_strFileName.substr(pos + 1);
// 			  int nLen = MultiByteToWideChar(CP_ACP, 0, tmp.c_str(), -1, NULL, NULL);
// 			  LPWSTR lpszW = new WCHAR[nLen];
// 			  MultiByteToWideChar(CP_ACP, 0, 
// 				  tmp.c_str(), -1, lpszW, nLen);
			  m_strFileExt = tmp;
#else
			  m_strFileExt = L"7z";
#endif
		  }
		  wprintf(L"Ext:%ls\n", m_strFileExt.c_str());
	  }

	  virtual ~TestOutStream()
	  {
		  wprintf(L"OutStream dtor\n");
		  return;
		  fclose(m_pFile);
	  }

public:
	int GetFileSize() const 
	{
		wprintf(L"OutStream GetFileSize %d\n",m_nFileSize);
		return m_nFileSize;
	}

	virtual int Write(const void *data, unsigned int size, unsigned int *processedSize)
	{
		wprintf(L"Write:%d\n", size);
		m_nFileSize+=size;
		return 0;
		int count = fwrite(data, 1, size, m_pFile);
		

		if (count >= 0)
		{
			if (processedSize != NULL)
				*processedSize = count;

			m_nFileSize += count;
			return 0;
		}

		return 1;
	}

	virtual int Seek(__int64 offset, unsigned int seekOrigin, unsigned __int64 *newPosition)
	{
		wprintf(L"OutStream Seek:%ld %u\n", offset,seekOrigin);
		return 0;
		int result = fseek(m_pFile, (long)offset, seekOrigin);

		if (!result)
		{
			if (newPosition)
				*newPosition = ftell(m_pFile);

			return 0;
		}

		return result;
	}

	virtual int SetSize(unsigned __int64 size)
	{
		wprintf(L"OutStream SetFileSize:%ld\n", size);
		return 0;
	}
};

// can also serve as instream
class MemOutStream : public C7ZipOutStream
{
private:
	char * m_pBuf;
	char * m_p;
	int m_nFileSize;
	int m_nBufSize;
public:
	MemOutStream():m_pBuf(0),m_p(0),m_nBufSize(0),m_nFileSize(0){}
	virtual ~MemOutStream()
	{
		if(m_pBuf) delete[] m_pBuf;
	}

public:
	void SetBufferSize(unsigned __int64 size)
	{
		if(m_pBuf) delete[] m_pBuf;
		m_pBuf=new char[size];
		m_nBufSize=size;
		m_p=m_pBuf;
		//m_nFileSize=size;
	}
	int GetFileSize() const 
	{
		wprintf(L"OutStream GetFileSize %d\n",m_nFileSize);
		return m_nFileSize;
	}
	void Read(LPVOID Buffer,DWORD BufferLength,LPDWORD ReadLength,LONGLONG Offset)
	{
		if(Offset>=m_nBufSize) {*ReadLength=0;return;}
		*ReadLength=min(BufferLength,m_nBufSize-Offset);
		memcpy(Buffer,m_pBuf+Offset,*ReadLength);
	}
	virtual int Write(const void *data, unsigned int size, unsigned int *processedSize)
	{
		wprintf(L"Write:%d\n", size);
		memcpy(m_p,data,size);
		*processedSize=size;
		m_nFileSize+=size;
		m_p+=size;
		return 0;
	}

	virtual int Seek(__int64 offset, unsigned int seekOrigin, unsigned __int64 *newPosition)
	{
		wprintf(L"OutStream Seek:%ld %u\n", offset,seekOrigin);

		switch(seekOrigin)
		{
		case SEEK_CUR:
			m_p+=offset;
			break;
		case SEEK_END:
			m_p=m_pBuf+m_nBufSize+offset;
			break;
		case SEEK_SET:
			m_p=m_pBuf+offset;
			break;
		}
		*newPosition=m_p-m_pBuf;
		return 0;
	}

	virtual int SetSize(unsigned __int64 size)
	{
		wprintf(L"OutStream SetFileSize:%ld\n", size);
		return 0;
	}
};

enum PropTypeEnum {
	kpUInt64,
	kpBool,
	kpString,
	kpTime,
	kpUnknown
};

struct VariantProp
{
	PropTypeEnum type;
	union{
		//std::wstring sVal;
		unsigned __int64 iVal;
		bool bVal;
		unsigned __int64 tVal;
	};
};

void print_archive_prop(int index,VariantProp& val,wstring& sVal);
bool get_archive_prop(C7ZipArchiveItem* pArchiveItem,lib7zip::PropertyIndexEnum index,VariantProp& val,wstring& sVal);
bool get_archive_prop(C7ZipArchive* pArchiveItem,lib7zip::PropertyIndexEnum index,VariantProp& val,wstring& sVal);
wstring& to_lower(wstring& src);
wstring& to_upper(wstring& src);
wstring get_file_ext(wstring& path);

typedef C7ZipArchiveItem* A7ZIP_ITEM_HANDLE;
typedef C7ZipArchive* A7ZIP_ARCHIVE_HANDLE;
class C7ZipExplore
{
public:
	C7ZipExplore();
	~C7ZipExplore();
	A7ZIP_ITEM_HANDLE find(A7ZIP_ARCHIVE_HANDLE pArchive,const wchar_t* fname);
	A7ZIP_ARCHIVE_HANDLE open_archive(TestInStream* stream);
	bool is_supported_archive(wstring& path);

private:
	A7ZIP_ITEM_HANDLE find_file_recur(const wchar_t* fname,int namelen,A7ZIP_ITEM_HANDLE file);
	C7ZipLibrary lib;
	WStringArray supported_exts;
};

#endif