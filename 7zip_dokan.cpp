
#include "7zip_explore.h"
#include "dokan/dokan.h"
#include "getopt.h"

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// #define DRV_PREFIX A7Zip
// #define SPACE_HOLDER
// #define SET_DOKAN_ENTRY(dokanOps,entry) SET_DOKAN_ENTRY_PREFIX(dokanOps,##entry,DRV_PREFIX)
// #define SET_DOKAN_ENTRY_PREFIX(dokanOps,entry,prefix) REAL_SET_DOKAN_ENTRY(dokanOps,##entry,prefix)
// #define REAL_SET_DOKAN_ENTRY(dokanOps,entry,prefix) REAL_SET_DOKAN_ENTRY_SPACEHOLDER(dokanOps,##entry,prefix,SPACE_HOLDER)
// #define REAL_SET_DOKAN_ENTRY_SPACEHOLDER(dokanOps,entry,prefix,holder) TRUE_SET_DOKAN_ENTRY(dokanOps,##entry,prefix,holder)
// #define TRUE_SET_DOKAN_ENTRY(dokanOps,entry,prefix,holder) (dokanOps)->holder##entry = prefix##entry

//#define SINGLE_ARCHIVE_MODE_HANDLE ((A7ZIP_ARCHIVE_HANDLE)DokanFileInfo->DokanOptions->GlobalContext)

C7ZipExplore A7Zip;
#define A7ZIP_FS_SINGLE_ARCHIVE 0
#define A7ZIP_FS_DIRECTORY 1
int g_FSMode=A7ZIP_FS_SINGLE_ARCHIVE;

// todo: ref count on archive file, when no ref into its files, close archive
typedef struct Context_Archive
{
	Context_Archive(const wchar_t* a_path,A7ZIP_ARCHIVE_HANDLE ha)
	:inStream(a_path)
	{
		pArchive=ha;
		refCnt=1; // decre when close handle
	}
	A7ZIP_ARCHIVE_HANDLE pArchive;
	TestInStream inStream;
	volatile LONG refCnt;
}Context_Archive,*HANDLE_Archive;

Context_Archive *g_SINGLE_ARCHIVE_MODE_HANDLE;

typedef struct Context_ArchiveItem
{
// 64m
#define EXTRACT_MAX_SIZE (64<<20)
	Context_ArchiveItem(A7ZIP_ITEM_HANDLE pfile,HANDLE_Archive ha)
	{
		extracted=false;
		pArchiveItem=pfile;
		hArchive=ha;
		InterlockedIncrement(&hArchive->refCnt);
	}
	~Context_ArchiveItem()
	{
		if(hArchive)
			InterlockedDecrement(&hArchive->refCnt);
	}
	
	bool do_extract()
	{
		if(extracted) return true;
		ULONG64 fsz=pArchiveItem->GetSize();
		if(fsz>EXTRACT_MAX_SIZE) return false;
		// ostream set size
		outStream.SetBufferSize(fsz);
		return (extracted=hArchive->pArchive->Extract(pArchiveItem,&outStream));
	}
	void read(LPVOID Buffer,DWORD BufferLength,LPDWORD ReadLength,LONGLONG Offset)
	{
		outStream.Read(Buffer,BufferLength,ReadLength,Offset);
	}
	
	A7ZIP_ITEM_HANDLE pArchiveItem;
	MemOutStream outStream;
	bool extracted;
	HANDLE_Archive hArchive;
}Context_ArchiveItem,*HANDLE_ArchiveItem;

static WCHAR MountPoint[MAX_PATH] = L"M:";
static WCHAR ArchivePath[MAX_PATH] = {0};
BOOL g_UseStdErr;
BOOL g_DebugMode;

static void DbgPrint(LPCWSTR format, ...)
{
	if (g_DebugMode) {
		WCHAR buffer[512];
		va_list argp;
		va_start(argp, format);
		vswprintf_s(buffer, sizeof(buffer)/sizeof(WCHAR), format, argp);
		va_end(argp);
		if (g_UseStdErr) {
			fwprintf(stderr, buffer);
		} else {
			OutputDebugStringW(buffer);
		}
	}
}
static void DbgPrint(char* format, ...)
{
	if (g_DebugMode) {
		char buffer[512];
		va_list argp;
		va_start(argp, format);
		vsprintf_s(buffer, sizeof(buffer), format, argp);
		va_end(argp);
		if (g_UseStdErr) {
			fprintf(stderr, buffer);
		} else {
			OutputDebugStringA(buffer);
		}
	}
}

char* unicode_to_ansi(LPCWSTR wstr)
{
	char* buf=0;
	int sz=WideCharToMultiByte(CP_ACP,NULL,wstr,-1,buf,0,NULL,NULL);
	buf=(char*)malloc(sizeof(char)*(sz+4));
	WideCharToMultiByte(CP_ACP,NULL,wstr,-1,buf,sz+4,NULL,NULL);
	buf[sz]=0;
	return buf;
}
WCHAR* ansi_to_unicode(const char* str)
{
	WCHAR* buf=0;
	int sz=MultiByteToWideChar(CP_ACP,NULL,str,-1,buf,0);
	buf=(WCHAR*)malloc(sizeof(WCHAR)*(sz+4));
	MultiByteToWideChar(CP_ACP,NULL,str,-1,buf,sz+4);
	buf[sz]=L'\0';
	return buf;
}




static int DOKAN_CALLBACK A7ZipCreateFile(
	LPCWSTR					FileName,
	DWORD					AccessMode,
	DWORD					ShareMode,
	DWORD					CreationDisposition,
	DWORD					FlagsAndAttributes,
	PDOKAN_FILE_INFO		DokanFileInfo)
{
	DbgPrint(L"CreateFile: %s\n",FileName);
	A7ZIP_ITEM_HANDLE file=A7Zip.find(g_SINGLE_ARCHIVE_MODE_HANDLE->pArchive,FileName);
	if(!file)
	{
		DbgPrint("file not found\n");
		return DOKAN_ERROR;
	}
	DbgPrint("found file [size]: %d\n",file->GetSize());
	
	HANDLE_ArchiveItem file_handle=new Context_ArchiveItem(file,g_SINGLE_ARCHIVE_MODE_HANDLE);
	
	if(file->IsDir())//|| is supported archive format
	{
		DokanFileInfo->IsDirectory=(UCHAR)1;
	}
	
	DokanFileInfo->Context = (ULONG64)file_handle;

	return 0;
}

static int DOKAN_CALLBACK A7ZipCreateDirectory (
	LPCWSTR					FileName,
	PDOKAN_FILE_INFO		DokanFileInfo)
{
	DbgPrint(L"CreateDirectory not support: %s\n",FileName);
	return DOKAN_ERROR;
}

static int DOKAN_CALLBACK A7ZipOpenDirectory (
	LPCWSTR					FileName,
	PDOKAN_FILE_INFO		DokanFileInfo)
{
	DbgPrint(L"OpenDirectory: %s\n",FileName);
	A7ZIP_ITEM_HANDLE file=A7Zip.find(g_SINGLE_ARCHIVE_MODE_HANDLE->pArchive,FileName);
	if(!file)
	{
		DbgPrint("file not found\n");
		return DOKAN_ERROR;
	}
	DbgPrint("found file [size]: %d\n",file->GetSize());

	if(!file->IsDir())//|| is supported archive format
	{
		DbgPrint("error: not dir, when A7ZipOpenDirectory \n");
		DokanFileInfo->IsDirectory=(UCHAR)0;
		return DOKAN_ERROR;
	}
	
	HANDLE_ArchiveItem file_handle=new Context_ArchiveItem(file,g_SINGLE_ARCHIVE_MODE_HANDLE);
	DokanFileInfo->Context = (ULONG64)file_handle;

	return 0;
}

static int DOKAN_CALLBACK A7ZipCloseFile(
	LPCWSTR					FileName,
	PDOKAN_FILE_INFO		DokanFileInfo)
{
	DbgPrint(L"CloseFile: %s\n", FileName);
	
	if (DokanFileInfo->Context) {
		DbgPrint(L"File not cleanup, when A7ZipCloseFile\n");
		delete (HANDLE_ArchiveItem)DokanFileInfo->Context;
		DokanFileInfo->Context = 0;
		return DOKAN_ERROR;
	}
	return 0;
}

static int DOKAN_CALLBACK A7ZipCleanup(
	LPCWSTR					FileName,
	PDOKAN_FILE_INFO		DokanFileInfo)
{
	DbgPrint(L"Cleanup: %s\n", FileName);
	
	// if is archive, decrement
	HANDLE_ArchiveItem file_handle=(HANDLE_ArchiveItem)DokanFileInfo->Context;
	if(file_handle)
	{
		delete file_handle; // decrement parchive ref_cnt
		DokanFileInfo->Context=0;
	}
	
	return 0;
// we do nothing
/*
	if (DokanFileInfo->Context) {
		DbgPrint(L"Cleanup: %s\n\n", FileName);
		delete//CloseHandle((HANDLE)DokanFileInfo->Context);
		DokanFileInfo->Context = 0;

		if (DokanFileInfo->DeleteOnClose) {
			DbgPrint(L"\tDeleteOnClose\n");
			if (DokanFileInfo->IsDirectory) {
				DbgPrint(L"  DeleteDirectory ");
				if (!RemoveDirectory(filePath)) {
					DbgPrint(L"error code = %d\n\n", GetLastError());
				} else {
					DbgPrint(L"success\n\n");
				}
			} else {
				DbgPrint(L"  DeleteFile ");
				if (DeleteFile(filePath) == 0) {
					DbgPrint(L" error code = %d\n\n", GetLastError());
				} else {
					DbgPrint(L"success\n\n");
				}
			}
		}

	} else {
		DbgPrint(L"Cleanup: %s\n\tinvalid handle\n\n", FileName);
		return DOKAN_ERROR;
	}
*/
	return 0;
}

static int DOKAN_CALLBACK A7ZipReadFile(
	LPCWSTR			FileName,
	LPVOID				Buffer,
	DWORD				BufferLength,
	LPDWORD			ReadLength,
	LONGLONG			Offset,
	PDOKAN_FILE_INFO	DokanFileInfo)
{
	//A7ZIP_ARCHIVE_HANDLE pArchive=SINGLE_ARCHIVE_MODE_HANDLE;
	HANDLE_ArchiveItem file_handle=(HANDLE_ArchiveItem)DokanFileInfo->Context;
	
	if(!file_handle||!file_handle->hArchive)
	{
		DbgPrint(L"A7ZipReadFile, invalid file handle, might have been closed %s\n",FileName);
		return DOKAN_ERROR;
	}
	
	if(!file_handle->do_extract())
	{
		DbgPrint(L"A7ZipReadFile, extract failed %s\n",FileName);
		return DOKAN_ERROR;
	}
	
	file_handle->read(Buffer,BufferLength,ReadLength,Offset);
	
// 	if(!pArchive||(!pArchiveItem&&!(pArchiveItem=A7Zip.find(SINGLE_ARCHIVE_MODE_HANDLE,FileName))))
// 	{
// 		DbgPrint(L"A7ZipReadFile, file not found: %s\n",FileName);
// 		return DOKAN_ERROR;
// 	}
// 	//if(!pArchive||!pArchiveItem) return DOKAN_ERROR;
// 	TestOutStream oStream(L"ttt.txt");
// 	pArchive->Extract(pArchiveItem, &oStream);
// 	memset(Buffer,'a',BufferLength);
// 	ULONG64 ffsszz=pArchiveItem->GetSize();
// 	*ReadLength=(Offset<ffsszz)?min(ffsszz-Offset,BufferLength):0;//pArchiveItem->GetSize();
	return 0;
}


static int DOKAN_CALLBACK A7ZipWriteFile(
			  LPCWSTR		FileName,
			  LPCVOID		Buffer,
			  DWORD		NumberOfBytesToWrite,
			  LPDWORD		NumberOfBytesWritten,
			  LONGLONG			Offset,
			  PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("WriteFile not support\n");
	return DOKAN_ERROR;
}

static int DOKAN_CALLBACK A7ZipFlushFileBuffers(
					 LPCWSTR		FileName,
					 PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("FlushFileBuffers not support\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipGetFileInformation(
	LPCWSTR							FileName,
	LPBY_HANDLE_FILE_INFORMATION	HandleFileInformation,
	PDOKAN_FILE_INFO				DokanFileInfo)
{
// 	A7ZIP_ITEM_HANDLE file = (A7ZIP_ITEM_HANDLE)DokanFileInfo->Context;
// 	if(!file&&!(file=A7Zip.find(SINGLE_ARCHIVE_MODE_HANDLE,FileName)))
// 	{
// 		DbgPrint(L"GetFileInfo, file not found: %s\n",FileName);
// 		return DOKAN_ERROR;
// 	}
	
	HANDLE_ArchiveItem file_handle=(HANDLE_ArchiveItem)DokanFileInfo->Context;

	if(!file_handle||!file_handle->hArchive)
	{
		DbgPrint(L"A7ZipGetFileInformation, invalid file handle, might have been closed %s\n",FileName);
		return DOKAN_ERROR;
	}
	A7ZIP_ITEM_HANDLE file=file_handle->pArchiveItem;
	
	DWORD file_attr=FILE_ATTRIBUTE_READONLY;
	if(file->IsDir()) // or supported archive format
		file_attr|=FILE_ATTRIBUTE_DIRECTORY;
	HandleFileInformation->dwFileAttributes = file_attr;
	file->GetFileTimeProperty(lib7zip::kpidCTime,*((ULONG64*)&HandleFileInformation->ftCreationTime));
	file->GetFileTimeProperty(lib7zip::kpidATime,*((ULONG64*)&HandleFileInformation->ftLastAccessTime));
	file->GetFileTimeProperty(lib7zip::kpidMTime,*((ULONG64*)&HandleFileInformation->ftLastWriteTime));
	ULONG64 file_sz=file->GetSize();
	HandleFileInformation->nFileSizeLow = DWORD(file_sz&0xffffffff);
	HandleFileInformation->nFileSizeHigh = DWORD(file_sz>>32);
	HandleFileInformation->dwVolumeSerialNumber=0x19910425;
	HandleFileInformation->nNumberOfLinks=1;// todo

	return 0;
}

static int DOKAN_CALLBACK A7ZipFindFiles(
			  LPCWSTR				FileName,
			  PFillFindData		FillFindData, // function pointer
			  PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint(L"FindFiles :%s\n", FileName);
	A7ZIP_ITEM_HANDLE file=(A7ZIP_ITEM_HANDLE)A7Zip.find(g_SINGLE_ARCHIVE_MODE_HANDLE->pArchive,FileName);
	if(!file)
	{
		DbgPrint(L"file not found, when FindFiles\n", FileName);
		return DOKAN_ERROR;
	}
	
	WIN32_FIND_DATAW	findData;

	if(!file->IsDir())
	{
		findData.dwFileAttributes = FILE_ATTRIBUTE_READONLY;
		file->GetFileTimeProperty(lib7zip::kpidCTime,*((ULONG64*)&findData.ftCreationTime));
		file->GetFileTimeProperty(lib7zip::kpidATime,*((ULONG64*)&findData.ftLastAccessTime));
		file->GetFileTimeProperty(lib7zip::kpidMTime,*((ULONG64*)&findData.ftLastWriteTime));
		ULONG64 file_sz=file->GetSize();
		findData.nFileSizeLow = DWORD(file_sz&0xffffffff);
		findData.nFileSizeHigh = DWORD(file_sz>>32);
		wcscpy_s(findData.cFileName,MAX_PATH,FileName);
		
		FillFindData(&findData, DokanFileInfo);
	}
	else
	{
		file=file->child;
		while(file)
		{
			DWORD file_attr=FILE_ATTRIBUTE_READONLY;
			if(file->IsDir())
				file_attr|=FILE_ATTRIBUTE_DIRECTORY;
			findData.dwFileAttributes = file_attr;
			file->GetFileTimeProperty(lib7zip::kpidCTime,*((ULONG64*)&findData.ftCreationTime));
			file->GetFileTimeProperty(lib7zip::kpidATime,*((ULONG64*)&findData.ftLastAccessTime));
			file->GetFileTimeProperty(lib7zip::kpidMTime,*((ULONG64*)&findData.ftLastWriteTime));
			ULONG64 file_sz=file->GetSize();
			findData.nFileSizeLow = DWORD(file_sz&0xffffffff);
			findData.nFileSizeHigh = DWORD(file_sz>>32);
			wcscpy_s(findData.cFileName,MAX_PATH,file->GetName().c_str());

			FillFindData(&findData, DokanFileInfo);

			file=file->next_sibling;
		}
	}

	return 0;
}


static int DOKAN_CALLBACK A7ZipDeleteFile(
			   LPCWSTR				FileName,
			   PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("DeleteFile not support\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipDeleteDirectory(
					LPCWSTR				FileName,
					PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("DeleteDirectory not support\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipMoveFile(
			 LPCWSTR				FileName, // existing file name
			 LPCWSTR				NewFileName,
			 BOOL				ReplaceIfExisting,
			 PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("MoveFile not support\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipLockFile(
			 LPCWSTR				FileName,
			 LONGLONG			ByteOffset,
			 LONGLONG			Length,
			 PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("LockFile not support\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipSetEndOfFile(
				 LPCWSTR				FileName,
				 LONGLONG			ByteOffset,
				 PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("SetEndOfFile not support\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipSetAllocationSize(
					  LPCWSTR				FileName,
					  LONGLONG			AllocSize,
					  PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("SetAllocationSize not support\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipSetFileAttributes(
					  LPCWSTR				FileName,
					  DWORD				FileAttributes,
					  PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("SetFileAttributes not support\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipSetFileTime(
				LPCWSTR				FileName,
				CONST FILETIME*		CreationTime,
				CONST FILETIME*		LastAccessTime,
				CONST FILETIME*		LastWriteTime,
				PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("SetFileTime not support\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipUnlockFile(
			   LPCWSTR				FileName,
			   LONGLONG			ByteOffset,
			   LONGLONG			Length,
			   PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("UnlockFile not support\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipGetFileSecurity(
	LPCWSTR					FileName,
	PSECURITY_INFORMATION	SecurityInformation,
	PSECURITY_DESCRIPTOR	SecurityDescriptor,
	ULONG				BufferLength,
	PULONG				LengthNeeded,
	PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("GetFileSecurity not supported\n");
	return DOKAN_ERROR;
}


static int DOKAN_CALLBACK A7ZipSetFileSecurity(
					LPCWSTR					FileName,
					PSECURITY_INFORMATION	SecurityInformation,
					PSECURITY_DESCRIPTOR	SecurityDescriptor,
					ULONG				SecurityDescriptorLength,
					PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint("SetFileSecurity not support\n");
	return DOKAN_ERROR;
}

static int DOKAN_CALLBACK A7ZipGetVolumeInformation(
						 LPWSTR		VolumeNameBuffer,
						 DWORD		VolumeNameSize,
						 LPDWORD		VolumeSerialNumber,
						 LPDWORD		MaximumComponentLength,
						 LPDWORD		FileSystemFlags,
						 LPWSTR		FileSystemNameBuffer,
						 DWORD		FileSystemNameSize,
						 PDOKAN_FILE_INFO	DokanFileInfo)
{
	//if(!root) return DOKAN_ERROR;
	wcscpy_s(VolumeNameBuffer, VolumeNameSize / sizeof(WCHAR),L"Archive Explore");
	*VolumeSerialNumber = 0x19910425;
	*MaximumComponentLength = 256;
	*FileSystemFlags = FILE_CASE_SENSITIVE_SEARCH | 
		FILE_CASE_PRESERVED_NAMES | 
		FILE_SUPPORTS_REMOTE_STORAGE |
		FILE_UNICODE_ON_DISK |
		FILE_PERSISTENT_ACLS;
	wcscpy_s(FileSystemNameBuffer, FileSystemNameSize / sizeof(WCHAR), L"ArchiveFS");

	return 0;
}


static int DOKAN_CALLBACK A7ZipUnmount(
			PDOKAN_FILE_INFO	DokanFileInfo)
{
	DbgPrint(L"Unmount\n");
	return 0;
}

// see Win32 API GetDiskFreeSpaceEx
static int DOKAN_CALLBACK A7ZipGetDiskFreeSpace(
		PULONGLONG FreeBytesAvailable,
		PULONGLONG TotalNumberOfBytes,
		PULONGLONG TotalNumberOfFreeBytes,
		PDOKAN_FILE_INFO DokanFileInfo)
{
	*FreeBytesAvailable=0;
	*TotalNumberOfFreeBytes=0;
	
	g_SINGLE_ARCHIVE_MODE_HANDLE->pArchive->GetUInt64Property(lib7zip::kpidPhySize,*TotalNumberOfBytes);
	return 0;
}

int __cdecl wmain(ULONG argc, PWCHAR argv[])
{
	int status;
	ULONG command;
	PDOKAN_OPERATIONS dokanOperations =
		(PDOKAN_OPERATIONS)malloc(sizeof(DOKAN_OPERATIONS));
	PDOKAN_OPTIONS dokanOptions =
		(PDOKAN_OPTIONS)malloc(sizeof(DOKAN_OPTIONS));

	g_DebugMode = FALSE;
	g_UseStdErr = FALSE;

	ZeroMemory(dokanOptions, sizeof(DOKAN_OPTIONS));
	dokanOptions->Version = DOKAN_VERSION;
	dokanOptions->ThreadCount = 1; // use default
	// no need, we store it as global var?
	//dokanOptions->GlobalContext = (ULONG64)ext2.get_root();

	static struct option_w long_options[] =
	{
		{L"debug", ARG_NONE, 0, L'd'},
		{L"stderr",   ARG_NONE, 0, L'e'},
		{L"threads",     ARG_REQ, 0, L't'},
		{L"mount_point",  ARG_REQ, 0, L'm'},
		{L"source",  ARG_REQ,  0, L's'},
		{ ARG_NULL , ARG_NULL , ARG_NULL , ARG_NULL }
	};
	int option_index = 0,c;
	while ((c = getopt_long(argc, argv, _T("det:m:s:h"), long_options, &option_index))!=-1)
	{
		// Handle options
		switch (c)
		{
		case L'd':
			g_DebugMode = TRUE;
			//dokanOptions->Options |= DOKAN_OPTION_DEBUG;
			break;
		case L'e':
			g_UseStdErr = TRUE;
			//dokanOptions->Options |= DOKAN_OPTION_STDERR;
			break;
		case L't':
			dokanOptions->ThreadCount = (USHORT)_wtoi(optarg_w);
			break;
		case L'm':
			wcscpy_s(MountPoint, sizeof(MountPoint)/sizeof(WCHAR), optarg_w);
			dokanOptions->MountPoint = MountPoint;
			break;
		case L's':
			wcscpy_s(ArchivePath, sizeof(ArchivePath)/sizeof(WCHAR), optarg_w);
			break;
		case L'h':
			fprintf(stderr, "archive_drv\n"
				"  --mount_point (-m) DriveLetter, e.g. -m K:\n"
				"  --source \t(-s) target archive, e.g. -a D:\\1.zip\n"
				"  --threads \t(-t) ThreadCount, e.g. -t 5\n"
				"  --debug \t(-d) enable debug output\n"
				"  --stderr \t(-e) use stderr for output\n"
				"  --help \t(-h) show this information\n"
				);
			break;
		case L'?':
			/* getopt_long already printed an error message. */
			break;

			//default:
			//	abort();
		}
	}
	
	assert(ArchivePath[0]);	

	DWORD fileAttr = GetFileAttributesW(ArchivePath);
	if(fileAttr==INVALID_FILE_ATTRIBUTES)
	{
		wprintf(L"Invalid source, %s, get file attr failed\n",ArchivePath);
		exit(0);
	}
	else if(!(fileAttr&FILE_ATTRIBUTE_DIRECTORY)&&A7Zip.is_supported_archive(wstring(ArchivePath)))
	{
		g_FSMode=A7ZIP_FS_SINGLE_ARCHIVE;
		
		g_SINGLE_ARCHIVE_MODE_HANDLE=new Context_Archive(ArchivePath,0);
		A7ZIP_ARCHIVE_HANDLE p_single_archive=A7Zip.open_archive(&g_SINGLE_ARCHIVE_MODE_HANDLE->inStream);
		if(!p_single_archive)
		{
			DbgPrint(L"open archive fail, exiting\n");
			delete g_SINGLE_ARCHIVE_MODE_HANDLE;
			exit(0);
		}
		g_SINGLE_ARCHIVE_MODE_HANDLE->pArchive=p_single_archive;
	}
	else
	{
		wprintf(L"Invalid source, %s, only support single archive file\n",ArchivePath);
		exit(0);
	}
	
	
	DbgPrint("after arg\n");
	dokanOptions->Options |= DOKAN_OPTION_KEEP_ALIVE;
	
// 	TestOutStream totalOStream(L"D:/test.exe");
// 	single_archive_mode_handle->Extract((unsigned int)0,&totalOStream);
	
	dokanOptions->GlobalContext = (ULONG64)g_SINGLE_ARCHIVE_MODE_HANDLE;

	ZeroMemory(dokanOperations, sizeof(DOKAN_OPERATIONS));

	dokanOperations->CreateFile = A7ZipCreateFile;
	dokanOperations->OpenDirectory = A7ZipOpenDirectory;
	dokanOperations->CreateDirectory = A7ZipCreateDirectory;
	dokanOperations->Cleanup = A7ZipCleanup;
	dokanOperations->CloseFile = A7ZipCloseFile;
	dokanOperations->ReadFile = A7ZipReadFile;
	dokanOperations->WriteFile = A7ZipWriteFile;
	dokanOperations->FlushFileBuffers = A7ZipFlushFileBuffers;
	dokanOperations->GetFileInformation = A7ZipGetFileInformation;
	dokanOperations->FindFiles = A7ZipFindFiles;
	dokanOperations->SetFileAttributes = A7ZipSetFileAttributes;
	dokanOperations->SetFileTime = A7ZipSetFileTime;
	dokanOperations->DeleteFile = A7ZipDeleteFile;
	dokanOperations->DeleteDirectory = A7ZipDeleteDirectory;
	dokanOperations->MoveFile = A7ZipMoveFile;
	dokanOperations->SetEndOfFile = A7ZipSetEndOfFile;
	dokanOperations->SetAllocationSize = A7ZipSetAllocationSize;	
	dokanOperations->LockFile = A7ZipLockFile;
	dokanOperations->UnlockFile = A7ZipUnlockFile;
	dokanOperations->GetFileSecurity = A7ZipGetFileSecurity;
	dokanOperations->SetFileSecurity = A7ZipSetFileSecurity;
	dokanOperations->GetDiskFreeSpace = A7ZipGetDiskFreeSpace;
	dokanOperations->GetVolumeInformation = A7ZipGetVolumeInformation;
	dokanOperations->Unmount = A7ZipUnmount;

	status = DokanMain(dokanOptions, dokanOperations);
	switch (status) {
	case DOKAN_SUCCESS:
		fprintf(stderr, "Success\n");
		break;
	case DOKAN_ERROR:
		fprintf(stderr, "Error\n");
		break;
	case DOKAN_DRIVE_LETTER_ERROR:
		fprintf(stderr, "Bad Drive letter\n");
		break;
	case DOKAN_DRIVER_INSTALL_ERROR:
		fprintf(stderr, "Can't install driver\n");
		break;
	case DOKAN_START_ERROR:
		fprintf(stderr, "Driver something wrong\n");
		break;
	case DOKAN_MOUNT_ERROR:
		fprintf(stderr, "Can't assign a drive letter\n");
		break;
	case DOKAN_MOUNT_POINT_ERROR:
		fprintf(stderr, "Mount point error\n");
		break;
	default:
		fprintf(stderr, "Unknown error: %d\n", status);
		break;
	}

	free(dokanOptions);
	free(dokanOperations);
	return 0;
}