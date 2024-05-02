#pragma once
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
APIPRIVATE
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
//  Cluster Information
//
//----------------------------------------------------------------------------
typedef enum _FS_CLUSTER_INFORMATION_CLASS
{
	ClusterInformationBasic = 0,
	ClusterInformationAll,
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
	HANDLE hRoot,
	HANDLE hFilePart, 
	PCWSTR pszVolumeName,
	PCWSTR pszFilePath,
	FS_CLUSTER_INFORMATION_CLASS Class,
	PVOID Data,
	ULONG cbData
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
