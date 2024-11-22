#pragma once

#include "wfswof.h"

//
// Helper functions for Win32/NtNative module
//

//
// NtPath Functions
//
EXTERN_C
UINT
APIENTRY
NtPathIsNtDevicePath(
    PCWSTR pszPath
    );

EXTERN_C
BOOL
APIENTRY
NtPathGetVolumeName(
    PCWSTR pszPath,
    PWSTR pszVolumeName,
    ULONG cchVolumeName
    );

EXTERN_C
BOOL
APIENTRY
NtPathGetRootDirectory(
    PCWSTR pszPath,
    PWSTR pszRootDir,
    ULONG cchRootDir
    );

EXTERN_C
BOOL
APIENTRY
NtPathFileExists(
    PCWSTR pszPath
    );

//
// Utility
//
EXTERN_C
ULONG
APIENTRY
_StringFromGUID(
    const GUID *pGuid,
    PWSTR pszGuid,
    ULONG cchMax
    );

EXTERN_C
BOOL
APIENTRY
NtPathIsRootDirectory(
    PCWSTR pszPath
    );

EXTERN_C
BOOL
APIENTRY
_HasPrefix(
    PCWSTR pszPrefix,
    PCWSTR pszPath
    );
//
// Reparse Point
//
typedef struct _FS_REPARSE_POINT_INFORMATION_EX
{
    ULONG Flags;
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union
    {
        struct {
            PWSTR TargetPath;
            ULONG TargetPathLength;
            PWSTR PrintPath;
            ULONG PrintPathLength;
        } MountPoint;

        struct {
            PWSTR TargetPath;
            ULONG TargetPathLength;
            PWSTR PrintPath;
            ULONG PrintPathLength;
            ULONG Flags;
        } SymLink;

        struct {
            ULONG Version;	  // Currently version 3
            PWSTR PackageID;  // L"Microsoft.WindowsTerminal_8wekyb3d8bbwe"
            PWSTR EntryPoint; // L"Microsoft.WindowsTerminal_8wekyb3d8bbwe!App"
            PWSTR Executable; // L"C:\Program Files\WindowsApps\Microsoft.WindowsTerminal_1.4.3243.0_x64__8wekyb3d8bbwe\wt.exe"
            PWSTR ApplicType; // "0" Integer as ASCII. "0" = Desktop bridge application; Else sandboxed UWP application
            PUCHAR Buffer;
        } AppExecLink;

        struct {
            PUCHAR Buffer;
        } GenericReparse;
    };
} FS_REPARSE_POINT_INFORMATION_EX;

//
// NT/DOS Path Helper
//
EXTERN_C
BOOL
APIENTRY
NtPathToDosPath(
    PCWSTR pszNtPath,
    PWSTR pszPath,
    ULONG cchPath
    );

EXTERN_C
BOOL
APIENTRY
NtPathToDosPathEx(
    PCWSTR pszNtPath,
    PWSTR pszPath,
    ULONG cchPath,
    ULONG Flags
    );

EXTERN_C
BOOL
APIENTRY
NtPathToGuidPath(
    PCWSTR pszNtPath,
    PWSTR pszPath,
    ULONG cchPath,
    ULONG Flags
    );
/*
#define PATHTYPE_GUID             1
#define PATHTYPE_DOSNAMESPACE     2
#define PATHTYPE_DOSDRIVE         3
*/
#define PTF_GUID                   0x1
#define PTF_DOSDRIVE               0x2
#define PTF_DOSNAMESPACE           0x4
#define PTF_TYPE_MASK              (0x1|0x2|0x4)
#define PTF_NTDOSNAMESPACEPREFIX   0x100
#define PTF_NTWIN32NAMESPACEPREFIX PTF_NTDOSNAMESPACEPREFIX
#define PTF_WIN32PATHPREFIX        0x200
#define PTF_PREFIX_MASK            (0x100|0x200)
#define PTF_NO_PREFIX              0x0

EXTERN_C
BOOL
APIENTRY
NtPathTranslatePath(
    PCWSTR pszNtPath,
    ULONG Flags,
    PWSTR pszPath,
    ULONG cchPath
    );

EXTERN_C
BOOL
APIENTRY
DosPathToNtDevicePath(
    PCWSTR pszDosPath,
    PWSTR pszNtPathBuffer,
    ULONG cchNtPathBuffer,
    ULONG Flags
    );

EXTERN_C
DWORD
APIENTRY
NtPathGetLongPathNameFromHandle(
    HANDLE hFile,
    PWSTR *LongFileName,
    PULONG pcbLongFileName
    );

//----------------------------------------------------------------------------
//
//  Overray File
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
GetWofInformation(
    HANDLE FileHandle,
    PVOID *ExternalInfo,
    PVOID *ProviderInfo
    );

EXTERN_C
HRESULT 
APIENTRY
FreeWofInformation(
    PVOID ExternalInfo,
    PVOID ProviderInfo
    );

//----------------------------------------------------------------------------
//
//  Directory Watch
//
//----------------------------------------------------------------------------
typedef struct _DIRWATCHNOTIFYEVENT
{
    ULONG cbNotifyBufferLength;
    FILE_NOTIFY_INFORMATION *pNotifyBuffer;
    PVOID Context;
	UINT InformationClass;
} DIRWATCHNOTIFYEVENT;

typedef HRESULT (CALLBACK *PFNDIRWATCHNOTIFYPROC)(DIRWATCHNOTIFYEVENT *Event);

typedef struct _DIRECTORY_WATCH_STRUCT
{
    HANDLE hDirectoryHandle;

    HANDLE hChgDirEvent;
    HANDLE hChangedUp;
    HANDLE hExitEvent;

    INT EnterChangeDirectorySection;
    HANDLE hThread;

    PFNDIRWATCHNOTIFYPROC pfnNotifyCallback;
    PVOID NotifyCallbackContext;

    ULONG cbNotifyBufferSize;
    PVOID pNotifyBuffer;

	UINT InformationClass;

} DIRECTORY_WATCH_STRUCT;

enum {
	DirectoryWatchNotifyInformation = 1,     // DirectoryNotifyInformation,FILE_NOTIFY_INFORMATION
    DirectoryWatchNotifyExtendedInformation, // DirectoryNotifyExtendedInformation,FILE_NOTIFY_EXTENDED_INFORMATION
    DirectoryWatchNotifyFullInformation,     // DirectoryNotifyFullInformation,FILE_NOTIFY_FULL_INFORMATION
	DirectoryWatchNotifyMaxInformation
};

EXTERN_C
HRESULT
WINAPI
StartDirectoryWatchEx(
    HANDLE *pHandle,
	UINT InformationClass,
    PFNDIRWATCHNOTIFYPROC pfnNotifyCallback,
    PVOID Context
    );

EXTERN_C
HRESULT
WINAPI
StartDirectoryWatch(
    HANDLE *pHandle,
    PFNDIRWATCHNOTIFYPROC pfnNotifyCallback,
    PVOID Context
    );

EXTERN_C
HRESULT
WINAPI
StopDirectoryWatch(
    HANDLE Handle
    );

EXTERN_C
HRESULT
WINAPI
SetWatchDirectory(
    HANDLE hWatch,
    PCWSTR pszDirectory
    );

//----------------------------------------------------------------------------
//
//  Cluster Information
//
//----------------------------------------------------------------------------
typedef enum _FS_CLUSTER_INFORMATION_CLASS
{
    ClusterInformationBasic = 0,
    ClusterInformationAll,
	ClusterInformationBasicWithPhysicalLocation,
} FS_CLUSTER_INFORMATION_CLASS;

// Physical offset information
typedef struct _FS_VOLUME_PHYSICAL_OFFSET
{
    ULONG DiskNumber;
    LONGLONG Offset;
} FS_VOLUME_PHYSICAL_OFFSET;

typedef struct _FS_VOLUME_PHYSICAL_OFFSETS
{
    ULONG NumberOfPhysicalOffsets;
    FS_VOLUME_PHYSICAL_OFFSET PhysicalOffset[ANYSIZE_ARRAY];
} FS_VOLUME_PHYSICAL_OFFSETS;

// Cluster partially information
typedef struct _FS_CLUSTER_INFORMATION_BASIC
{
    LARGE_INTEGER FirstLcn;
    ULONG FirstCount;
    ULONG Split;
    ULONG SectorsPerCluster;
    ULONG BytesPerSector;
} FS_CLUSTER_INFORMATION_BASIC;

typedef struct _FS_CLUSTER_INFORMATION_BASIC_EX
{
    LARGE_INTEGER FirstLcn;
    ULONG FirstCount;
    ULONG Split;
    ULONG SectorsPerCluster;
    ULONG BytesPerSector;
	ULONG DiskNumber;
	LARGE_INTEGER PhysicalLocation;
} FS_CLUSTER_INFORMATION_BASIC_EX;

// Volume cluster information
typedef struct _FS_CLUSTER_LOCATION
{
    LARGE_INTEGER Vcn;
    LARGE_INTEGER Lcn;
    LARGE_INTEGER Count;
    FS_VOLUME_PHYSICAL_OFFSETS *PhysicalOffsets;
} FS_CLUSTER_LOCATION;

typedef struct _FS_CLUSTER_INFORMATION
{
    ULONG BytesPerCluster;
    ULONG ExtentCount;
    ULONG BytesPerSector;
    RETRIEVAL_POINTER_BASE ClusterHeapBase; // The volume-relative sector offset to the first allocatable unit on the file system.
    FS_CLUSTER_LOCATION Extents[ANYSIZE_ARRAY];
} FS_CLUSTER_INFORMATION;

EXTERN_C
LONG
WINAPI
ReadFileClusterInformaion( 
    HANDLE hVolume,
    HANDLE hFile, 
    PCWSTR pszVolumeRootDirectoryName,
    FS_CLUSTER_INFORMATION_CLASS Class,
    PVOID Data,
    ULONG cbData
    );

EXTERN_C
LONG
WINAPI
FreeClusterInformation( 
    PVOID Buffer
    );

//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//  NTFS Special Files (provisional)
//
//----------------------------------------------------------------------------

typedef struct _FS_NTFS_SPECIAL_FILE_ITEM
{
    ULONG Length;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastModificationTime;
    LARGE_INTEGER LastChangeTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER AllocatedLength;
    LARGE_INTEGER FileSize;
    ULONG FileAttributes;
    ULONG PackedEaSize;
    USN Usn;
    LARGE_INTEGER ParentDirectory;
    LARGE_INTEGER FileId;
    USHORT ShortNameLength;
    USHORT NameLength;
    WCHAR ShortName[12];
    WCHAR Name[MAX_PATH];
} FS_NTFS_SPECIAL_FILE_ITEM;

typedef struct _FS_NTFS_SPECIAL_FILE_LIST
{
    HANDLE Handle;
    ULONG cItemListCount;
    FS_NTFS_SPECIAL_FILE_ITEM **pItemList;
} FS_NTFS_SPECIAL_FILE_LIST;

EXTERN_C
ULONG
WINAPI
GetNtfsSpecialFiles(
    PCWSTR pszVolumeName,
    LONGLONG FileId,
    BOOL bEnumItem,
    FS_NTFS_SPECIAL_FILE_LIST *FileList
    );

EXTERN_C
ULONG
WINAPI
FreeNtfsSpecialFiles(
    FS_NTFS_SPECIAL_FILE_LIST *FileList
    );


typedef struct _FILE_INFORMATION_ALTSTREAM
{
	PWSTR Name;
	LARGE_INTEGER Size;
	LARGE_INTEGER AllocSize;
} FILE_INFORMATION_ALTSTREAM;

typedef struct _FILE_INFORMATON_EA_DATA
{
    UCHAR  Flags;
    UCHAR  NameLength;
    USHORT ValueLength;
	CHAR  *Name;
	UCHAR *Value;
} FILE_INFORMATON_EA_DATA;

typedef struct _FILE_INFORMATON_EA_BUFFER
{
	ULONG EaCount;
	FILE_INFORMATON_EA_DATA Ea[1];
} FILE_INFORMATON_EA_BUFFER;

typedef struct _FILE_INFORMATION_STRUCT
{
	PWSTR Name;
	PWSTR ShortName;
    LARGE_INTEGER FileReferenceNumber;

    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;

	struct {
	    LARGE_INTEGER CreationTime;
		LARGE_INTEGER LastAccessTime;
	    LARGE_INTEGER LastWriteTime;
		LARGE_INTEGER ChangeTime;
	} DirectoryEnrty;

	struct {
		INT cAltStreamName;
		FILE_INFORMATION_ALTSTREAM *AltStreamName;
	} AltStream;

	ULONG EaSize;
	FILE_INFORMATON_EA_BUFFER *EaBuffer;

	struct {
		UCHAR ObjectId[16];
	    union {
		    struct {
			    UCHAR BirthVolumeId[16];
				UCHAR BirthObjectId[16];
	            UCHAR DomainId[16];
		    };
			UCHAR ExtendedInfo[48];
		};
	} ObjectId;

	WCHAR FileSystemName[16];
	ULONG FileSystemAttributes;
	ULONG MaximumComponentNameLength;

	FS_REPARSE_POINT_INFORMATION_EX ReparsePointInfo;

	struct {
		WOF_EXTERNAL_INFO *ExternalInfo;
		union {
			PVOID GenericPtr;
			FILE_PROVIDER_EXTERNAL_INFO_V1 *FileInfo;
			WIM_PROVIDER_EXTERNAL_INFO *WimInfo;
		};
	} Wof;

	struct
	{
		ULONG ObjectId : 1;
		ULONG ReparsePoint : 1;
		ULONG Wof : 1;
	} State;

} FILE_INFORMATION_STRUCT;

EXTERN_C
HRESULT
APIENTRY
NTFile_GatherFileInformation(
	HANDLE hFile,
	FILE_INFORMATION_STRUCT **pfi
	);

EXTERN_C
HRESULT
APIENTRY
NTFile_FreeFileInformation(
	FILE_INFORMATION_STRUCT *pfi
	);

EXTERN_C
HRESULT
APIENTRY
NTFile_OpenFile(
	HANDLE *phFile,
	PCWSTR FilePath,
	ULONG DesiredAccess,
	ULONG ShareAccess,
	ULONG OpenOptions
	);

EXTERN_C
HRESULT
APIENTRY
NTFile_CloseFile(
	HANDLE hFile
	);

EXTERN_C
BOOL
APIENTRY
NTFile_GetAttributeString(
	DWORD Attributes,
	LPWSTR String,
	int cchString
	);

EXTERN_C
BOOL
APIENTRY
GetAttributeString(
	DWORD Attributes,
	LPWSTR String,
	int cchString
	);
