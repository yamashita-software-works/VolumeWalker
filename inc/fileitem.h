#pragma once

typedef struct _FILEITEMHEADER
{
	ULONG Reserved;
	PWSTR Path;
	PWSTR FileName;
} FILEITEMHEADER;

typedef struct _FILEITEM
{
	FILEITEMHEADER hdr;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    ULONG FileIndex;
    LARGE_INTEGER FileId;
    WCHAR ShortName[14];
} FILEITEM,*PFILEITEM;

#ifdef __cplusplus

class CFileItem : public FILEITEM
{
public:
	ULONG ItemTypeFlag;
	LARGE_INTEGER FirstLCN;
	LARGE_INTEGER ParentFileId; // todo: 128bit id not support
    DWORD NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
	BOOLEAN Wof; // Overray File
	ULONG FileSetSigneture;
	LARGE_INTEGER PhysicalDriveOffset; // First sector offset length on the physical drive.
	ULONG PhysicalDriveNumber;

	CFileItem()
	{
		memset(this,0,sizeof(CFileItem));
	}

	CFileItem(PCWSTR pszDirPath,PCWSTR pszFile)
	{
		memset(this,0,sizeof(CFileItem));
		if( pszDirPath )
			hdr.Path = _MemAllocString(pszDirPath);
		if( pszFile )
			hdr.FileName = _MemAllocString(pszFile);
	}

	~CFileItem()
	{
		_SafeMemFree(hdr.Path);
		_SafeMemFree(hdr.FileName);
	}

	PCWSTR SetFileName(PCWSTR FileName)
	{
		_SafeMemFree(hdr.FileName);
		hdr.FileName = _MemAllocString(FileName);
		if( hdr.FileName )
			this->FileNameLength = (ULONG)(wcslen(hdr.FileName) * sizeof(WCHAR));
		else
			this->FileNameLength = 0;
		return hdr.FileName;
	}

	PCWSTR SetFileName(PCWSTR FileName,int cchFileName)
	{
		_SafeMemFree(hdr.FileName);
		this->FileNameLength = cchFileName * sizeof(WCHAR);
		hdr.FileName = (PWSTR)_MemAllocZero(this->FileNameLength + sizeof(WCHAR));
		if( hdr.FileName )
		{
			memcpy(hdr.FileName,FileName,this->FileNameLength);
		}
		else
		{
			this->FileNameLength = 0;
		}
		return hdr.FileName;
	}

	PCWSTR SetPath(PCWSTR Path)
	{
		_SafeMemFree(hdr.Path);
		hdr.Path = _MemAllocString(hdr.Path);
		return hdr.Path;
	}
};

#endif
