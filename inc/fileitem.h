#pragma once

typedef struct _FILEITEMHEADER
{
	ULONG Flags;
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

#define FIF_UNUSEFLAGS         (0x00000000)
#define FIF_CREATIONTIME       (0x00000010)
#define FIF_LASTACCESSTIME     (0x00000020)
#define FIF_LASTWRITETIME      (0x00000040)
#define FIF_CHANGETIME         (0x00000080)
#define FIF_DATEMASK           (FIF_CREATIONTIME|FIF_LASTACCESSTIME|FIF_LASTWRITETIME|FIF_CHANGETIME)
#define FIF_ENDOFFILE          (0x00000100)
#define FIF_ALLOCATIONSIZE     (0x00000200)
#define FIF_SIZEMASK           (FIF_ALLOCATIONSIZE|FIF_ENDOFFILE)
#define FIF_FILEATTRIBUTES     (0x00000400)
#define FIF_EASIZE             (0x00000800)
#define FIF_FILEINDEX          (0x00001000)
#define FIF_FILEID             (0x00002000)
#define FIF_SHORTNAME          (0x00004000)

#ifdef __cplusplus

class CFileItem : public FILEITEM
{
public:
	ULONG FlagsEx;
	ULONG ItemTypeFlag;
	LARGE_INTEGER FirstLCN;
	LARGE_INTEGER ParentFileId;        // todo: 128bit id not support
    DWORD NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
	BOOLEAN Wof;                       // Overray File
	LARGE_INTEGER PhysicalDriveOffset; // First sector offset length on the physical drive.
	ULONG PhysicalDriveNumber;
	ULONG ReparseTag;
	union {
		struct {
			GUID ObjectId;
			GUID BirthVolumeId;
			GUID BirthObjectId;
			GUID DomainId;
		};
	};

	CFileItem()
	{
		memset(this,0,sizeof(CFileItem));
	}

	CFileItem(PCWSTR pszDirPath,PCWSTR pszFile)
	{
		memset(this,0,sizeof(CFileItem));
#ifdef _CFILELITEM_NO_MEMORY_DEBUG
		if( pszDirPath )
			hdr.Path = StrDupW(pszDirPath);
		if( pszFile )
			hdr.FileName = StrDupW(pszFile);
#else
		if( pszDirPath )
			hdr.Path = _MemAllocString(pszDirPath);
		if( pszFile )
			hdr.FileName = _MemAllocString(pszFile);
#endif
	}

	~CFileItem()
	{
#ifdef _CFILELITEM_NO_MEMORY_DEBUG
		LocalFree(hdr.Path);
		LocalFree(hdr.FileName);
#else
		_SafeMemFree(hdr.Path);
		_SafeMemFree(hdr.FileName);
#endif
	}

	PCWSTR SetFileName(PCWSTR FileName)
	{
#ifdef _CFILELITEM_NO_MEMORY_DEBUG
		LocalFree(hdr.FileName);
		hdr.FileName = StrDupW(FileName);
#else
		_SafeMemFree(hdr.FileName);
		hdr.FileName = _MemAllocString(FileName);
#endif
		if( hdr.FileName )
			this->FileNameLength = (ULONG)(wcslen(hdr.FileName) * sizeof(WCHAR));
		else
			this->FileNameLength = 0;
		return hdr.FileName;
	}

	PCWSTR SetFileName(PCWSTR FileName,int cchFileName)
	{
#ifdef _CFILELITEM_NO_MEMORY_DEBUG
		LocalFree(hdr.FileName);
#else
		_SafeMemFree(hdr.FileName);
#endif
		this->FileNameLength = cchFileName * sizeof(WCHAR);
#ifdef _CFILELITEM_NO_MEMORY_DEBUG
		hdr.FileName = (PWSTR)LocalAlloc(LPTR,this->FileNameLength + sizeof(WCHAR));
#else
		hdr.FileName = (PWSTR)_MemAllocZero(this->FileNameLength + sizeof(WCHAR));
#endif
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
#ifdef _CFILELITEM_NO_MEMORY_DEBUG
		LocalFree(hdr.Path);
		hdr.Path = StrDupW(Path);
#else
		_SafeMemFree(hdr.Path);
		hdr.Path = _MemAllocString(Path);
#endif
		return hdr.Path;
	}
};

#define FIF_EX_DIRECTORY               (0x00000001)
#define FIF_EX_NUMBEROFLINKS           (0x00000002)
#define FIF_EX_DELETEPENDING           (0x00000004)
#define FIF_EX_PARENTFILEID            (0x00000008)
#define FIF_EX_FIRSTLCN                (0x00000010)
#define FIF_EX_FILESETSIGNETURE        (0x00000020)
#define FIF_EX_PHYSICALDRIVEOFFSET     (0x00000040)
#define FIF_EX_PHYSICALDRIVENUMBER     (0x00000080)
#define FIF_EX_WOF                     (0x00000100)
#define FIF_EX_ROOTDIRECTORY           (0x40000000) 
#define FIF_EX_INVALIDITEM             (0x80000000) 

#endif
