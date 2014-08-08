
#include "7zip_explore.h"
#include <stdlib.h>

const wchar_t * index_names[] = {
	L"kpidPackSize", //(Packed Size)
	L"kpidAttrib", //(Attributes)
	L"kpidCTime", //(Created)
	L"kpidATime", //(Accessed)
	L"kpidMTime", //(Modified)
	L"kpidSolid", //(Solid)
	L"kpidEncrypted", //(Encrypted)
	L"kpidUser", //(User)
	L"kpidGroup", //(Group)
	L"kpidComment", //(Comment)
	L"kpidPhySize", //(Physical Size)
	L"kpidHeadersSize", //(Headers Size)
	L"kpidChecksum", //(Checksum)
	L"kpidCharacts", //(Characteristics)
	L"kpidCreatorApp", //(Creator Application)
	L"kpidTotalSize", //(Total Size)
	L"kpidFreeSpace", //(Free Space)
	L"kpidClusterSize", //(Cluster Size)
	L"kpidVolumeName", //(Label)
	L"kpidPath", //(FullPath)
	L"kpidIsDir", //(IsDir)
	L"kpidSize", //(Uncompressed Size)
};

const wchar_t * prop_names[] = {
	L"Packed Size",
	L"Attributes",
	L"Created Time",
	L"Accessed Time",
	L"Modified Time",
	L"Solid)",
	L"Encrypted",
	L"User",
	L"Group",
	L"Comment",
	L"Physical Size",
	L"Headers Size",
	L"Checksum",
	L"Characteristics",
	L"Creator Application",
	L"Total Size",
	L"Free Space",
	L"Cluster Size",
	L"Label",
	L"FullPath",
	L"IsDir",
	L"Uncompressed Size",
};

// PropTypeEnum PropTypeTable[] = {
// 	kpUInt64,//kpidPackSize = PROP_INDEX_BEGIN, //(Packed Size)
// 	kpUnknown,//kpidAttrib, //(Attributes)
// 	kpTime,//kpidCTime, //(Created)
// 	kpTime,//kpidATime, //(Accessed)
// 	kpTime,//kpidMTime, //(Modified)
// 	kpUnknown,//kpidSolid, //(Solid)
// 	kpBool,//kpidEncrypted, //(Encrypted)
// 	kpUnknown,//kpidUser, //(User)
// 	kpUnknown,//kpidGroup, //(Group)
// 	kpString,//kpidComment, //(Comment)
// 	kpUInt64,//kpidPhySize, //(Physical Size)
// 	kpUInt64,//kpidHeadersSize, //(Headers Size)
// 	kpUnknown,//kpidChecksum, //(Checksum)
// 	kpUnknown,//kpidCharacts, //(Characteristics)
// 	kpUnknown,//kpidCreatorApp, //(Creator Application)
// 	kpUInt64,//kpidTotalSize, //(Total Size)
// 	kpUInt64,//kpidFreeSpace, //(Free Space)
// 	kpUInt64,//kpidClusterSize, //(Cluster Size)
// 	kpString,//kpidVolumeName, //(Label)
// 	kpString,//kpidPath, //(FullPath)
// 	kpBool,//kpidIsDir, //(IsDir)
// 	kpUInt64,//kpidSize, //(Uncompressed Size)
// };
PropTypeEnum PropTypeTable[] = {
	kpUnknown,//kpidPackSize = PROP_INDEX_BEGIN, //(Packed Size)
	kpUnknown,//kpidAttrib, //(Attributes)
	kpUnknown,//kpidCTime, //(Created)
	kpUnknown,//kpidATime, //(Accessed)
	kpUnknown,//kpidMTime, //(Modified)
	kpUnknown,//kpidSolid, //(Solid)
	kpUnknown,//kpidEncrypted, //(Encrypted)
	kpUnknown,//kpidUser, //(User)
	kpUnknown,//kpidGroup, //(Group)
	kpUnknown,//kpidComment, //(Comment)
	kpUnknown,//kpidPhySize, //(Physical Size)
	kpUnknown,//kpidHeadersSize, //(Headers Size)
	kpUnknown,//kpidChecksum, //(Checksum)
	kpUnknown,//kpidCharacts, //(Characteristics)
	kpUnknown,//kpidCreatorApp, //(Creator Application)
	kpUnknown,//kpidTotalSize, //(Total Size)
	kpUnknown,//kpidFreeSpace, //(Free Space)
	kpUnknown,//kpidClusterSize, //(Cluster Size)
	kpUnknown,//kpidVolumeName, //(Label)
	kpUnknown,//kpidPath, //(FullPath)
	kpUnknown,//kpidIsDir, //(IsDir)
	kpUnknown,//kpidSize, //(Uncompressed Size)
};

C7ZipExplore::C7ZipExplore()
{
	if(!lib.Initialize())
	{
		wprintf(L"7zip lib initialize fail!\n");
		exit(0);
	}
	if (!lib.GetSupportedExts(supported_exts)) {
		wprintf(L"get supported exts fail\n");
		exit(0);
	}
	size_t size = supported_exts.size();
	for(size_t i = 0; i < size; i++)
	{
		to_lower(supported_exts[i]);
	}
// 	size_t size = exts.size();
// 
// 	for(size_t i = 0; i < size; i++) {
// 		wstring ext = exts[i];
// 
// 		for(size_t j = 0; j < ext.size(); j++) {
// 			wprintf(L"%c", (char)(ext[j] &0xFF));
// 		}
// 
// 		wprintf(L"\n");
// 	}
}
C7ZipExplore::~C7ZipExplore()
{
	lib.Deinitialize();
}
bool C7ZipExplore::is_supported_archive(wstring& path)
{
	wstring ext=get_file_ext(path);
	size_t size = supported_exts.size();
	for(size_t i = 0; i < size; i++)
	{
		if(supported_exts[i].compare(ext)==0) return true;
	}
	return false;
}
A7ZIP_ITEM_HANDLE C7ZipExplore::find_file_recur(const wchar_t* fname,int namelen,A7ZIP_ITEM_HANDLE file)
{
	if(!file) return NULL;
	wstring wmyname=file->GetName();
	const wchar_t* myname=wmyname.c_str();
	int len=wmyname.length();
	if(len>namelen) return NULL;
	else if(len==namelen)
	{
		return (wcsncmp(fname,myname,len)==0)? file : NULL;
	}
	else
	{
		char delim=fname[len];
		if((delim=='/'||delim=='\\')&&
			wcsncmp(fname,myname,len)==0)
		{
			if(!file->IsDir()) // not dir
				return NULL;
			if(namelen==len+1)
				return file;

			A7ZIP_ITEM_HANDLE ret=0;
			
			file=file->child;
			while(file&&!(ret=find_file_recur(fname+len+1,namelen-len-1,file)))
			{
				file=file->next_sibling;
			}
			return ret;
		}
	}
	return NULL;
}

A7ZIP_ITEM_HANDLE C7ZipExplore::find(A7ZIP_ARCHIVE_HANDLE pArchive,const wchar_t* fname)
{
	//assert(pArchive,fname);
	if(!pArchive||!fname) return 0;
	
	int nameLen=wcslen(fname);
	return find_file_recur(fname,nameLen,pArchive->GetRootItem());
}
A7ZIP_ARCHIVE_HANDLE C7ZipExplore::open_archive(TestInStream* stream)//const wchar_t* apath)
{
	// todo: new stream?????? save it !!!!!
	//TestInStream* stream=new TestInStream(apath);
	//TestOutStream oStream("TestResult.txt");
	A7ZIP_ARCHIVE_HANDLE pArchive=0;
	if (lib.OpenArchive(stream, &pArchive)) {
		unsigned int numItems = 0;
		pArchive->GetItemCount(&numItems);
		wprintf(L"files in archive: %d\n\n", numItems);

		VariantProp propval;
		wstring sVal;
		
// 		A7ZIP_ITEM_HANDLE tttttt;
// 		pArchive->GetItemInfo(0,&tttttt);
// 		sVal=tttttt->GetFullPath();
// 		sVal=tttttt->GetName();
		
		for(lib7zip::PropertyIndexEnum index = lib7zip::PROP_INDEX_BEGIN;
			index < lib7zip::PROP_INDEX_END;
			index = (lib7zip::PropertyIndexEnum)(index + 1)) {
				if(get_archive_prop(pArchive,index,propval,sVal))
					print_archive_prop(index,propval,sVal);
		}
	}
	return pArchive;
}

void print_archive_prop(int index,VariantProp& val,wstring& sVal)
{
	if(index<lib7zip::PROP_INDEX_BEGIN||index>=lib7zip::PROP_INDEX_END||val.type==kpUnknown) return;
	wprintf(L"[%s]:\t\t ",prop_names[index]);
	switch(val.type)
	{
	case kpUInt64:
		wprintf(L"%u\n",val.iVal);
		break;
	case kpBool:
		wprintf(L"%s\n",val.bVal?L"True":L"False");
		break;
	case kpString:
		wprintf(L"%s\n",sVal.c_str());
		break;
	case kpTime:
		{
			FILETIME ltm;
			FileTimeToLocalFileTime((FILETIME*)&val.tVal,&ltm);
			SYSTEMTIME stm;
			FileTimeToSystemTime(&ltm,&stm);
			wprintf(L"%04d/%d/%d %02d:%02d\n",stm.wYear,stm.wMonth,stm.wDay,stm.wHour,stm.wMinute);
			break;
		}
	}
}
bool get_archive_prop(C7ZipArchiveItem* pArchiveItem,lib7zip::PropertyIndexEnum index,VariantProp& val,wstring& sVal)
{
	if(index<lib7zip::PROP_INDEX_BEGIN||index>=lib7zip::PROP_INDEX_END) return false;
	PropTypeEnum ptype=PropTypeTable[index];
	bool result=false;
	val.type=ptype;
	switch(ptype)
	{
	case kpUInt64:
		result = pArchiveItem->GetUInt64Property(index, val.iVal);
		break;
	case kpBool:
		result = pArchiveItem->GetBoolProperty(index, val.bVal);
		break;
	case kpString:
		result = pArchiveItem->GetStringProperty(index, sVal);
		break;
	case kpTime:
		result = pArchiveItem->GetFileTimeProperty(index, val.tVal);
		break;
	default:
		val.type=kpUInt64;
		result = pArchiveItem->GetUInt64Property(index, val.iVal);
		if(!result)
		{
			val.type=kpBool;
			result = pArchiveItem->GetBoolProperty(index, val.bVal);
		}
		if(!result)
		{
			val.type=kpString;
			result = pArchiveItem->GetStringProperty(index, sVal);
		}
		if(!result)
		{
			val.type=kpTime;
			result = pArchiveItem->GetFileTimeProperty(index, val.tVal);
		}
		break;
	}
	return result;
}
bool get_archive_prop(C7ZipArchive* pArchiveItem,lib7zip::PropertyIndexEnum index,VariantProp& val,wstring& sVal)
{
	if(index<lib7zip::PROP_INDEX_BEGIN||index>=lib7zip::PROP_INDEX_END) return false;
	PropTypeEnum ptype=PropTypeTable[index];
	bool result=false;
	val.type=ptype;
	switch(ptype)
	{
	case kpUInt64:
		result = pArchiveItem->GetUInt64Property(index, val.iVal);
		break;
	case kpBool:
		result = pArchiveItem->GetBoolProperty(index, val.bVal);
		break;
	case kpString:
		result = pArchiveItem->GetStringProperty(index, sVal);
		break;
	case kpTime:
		result = pArchiveItem->GetFileTimeProperty(index, val.tVal);
		break;
	default:
		val.type=kpUInt64;
		result = pArchiveItem->GetUInt64Property(index, val.iVal);
		if(!result)
		{
			val.type=kpBool;
			result = pArchiveItem->GetBoolProperty(index, val.bVal);
		}
		if(!result)
		{
			val.type=kpString;
			result = pArchiveItem->GetStringProperty(index, sVal);
		}
		if(!result)
		{
			val.type=kpTime;
			result = pArchiveItem->GetFileTimeProperty(index, val.tVal);
		}
		break;
	}
	return result;
}

wstring& to_lower(wstring& src)
{
	for (wstring::iterator i=src.begin(); i!=src.end(); ++i)
		*i|=(wchar_t)32;
	return src;
}
wstring& to_upper(wstring& src)
{
	for (wstring::iterator i=src.begin(); i!=src.end(); ++i)
		*i&=(wchar_t)(~32);
	return src;
}
wstring get_file_ext(wstring& path)
{
	int i=path.find_last_of(L'.');
	return i==-1?L"no_ext":to_lower(path.substr(i+1));
}