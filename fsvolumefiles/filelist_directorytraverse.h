#pragma once 

#define PM_FODLG_ENDOPERATION     (PM_PRIVATEBASE+0)
#define PM_FODLG_COMPLETE         (PM_PRIVATEBASE+1)

typedef struct _CALLBACK_FILE_INFORMATION
{
	PCWSTR Path;
	PCWSTR RelativePath;
	PCWSTR FileName;
	UNICODE_STRING UnicodeStringFileName;
	FS_FILE_ID_BOTH_DIR_INFORMATION *Information;
} CALLBACK_FILE_INFORMATION;

typedef HRESULT (CALLBACK *DIRECTORYSCANCALLBACK)(ULONG CallbackReason,PVOID CallbackFileInformation,PVOID Context); // todo:

#define FACILITY_FILEOPERATION 128
#define S_FO_SKIP                          MAKE_HRESULT(SEVERITY_SUCCESS,FACILITY_FILEOPERATION,2)
#define S_FO_DIRECTORY_HIERARCHY_TOO_DEEP  MAKE_HRESULT(SEVERITY_SUCCESS,FACILITY_FILEOPERATION,3)

HRESULT
WINAPI
RecursiveEnumDirectoryFiles(
	PCWSTR FullPath,
	PVOID p1, // reserved:
	PVOID p2, // reserved:
	ULONG f1, // reserved:
	DIRECTORYSCANCALLBACK pfnCallback,
	PVOID CallbackContext
	);

typedef struct _DIR_FILECOUNT
{
	ULONG TotalDirectoryCount;
	ULONG TotalFileCount;
	ULONG DirectoryCount;
	ULONG FileCount;
	LARGE_INTEGER TotalSize;
	LARGE_INTEGER TotalAllocationBytes;
} DIR_FILECOUNT;

HRESULT
WINAPI
RecursiveDirectoryFileCount(
	PCWSTR DirectoryPath,
	DIR_FILECOUNT *pdfc,
	DIRECTORYSCANCALLBACK pfnCallback,
	PVOID CallbackContext
	);
