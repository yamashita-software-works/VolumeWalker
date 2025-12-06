#pragma once

#include "fileitemdata.h"

typedef struct _SEARCH_FROM_TO
{
	LARGE_INTEGER From;
	LARGE_INTEGER To;
} SEARCH_RANGE_VALUE;

typedef ULONGLONG SEARCH_FLAGS;

typedef struct _SEARCH_PARAMETER
{
	SEARCH_FLAGS CompareFlag;
	DWORD FileAttributes;
	SEARCH_RANGE_VALUE EndOfFile;
	SEARCH_RANGE_VALUE AllocateSize;
	struct {
		SEARCH_RANGE_VALUE LastWrite;
		SEARCH_RANGE_VALUE Creation;
		SEARCH_RANGE_VALUE LastAccess;
		SEARCH_RANGE_VALUE Change;
	} DateTime;
	ULONG MaxFoundItemCount;
	ULONG MaxDirectoryLevel;
	WCHAR Name[MAX_PATH];
} SEARCH_PARAMETER;

#define SEARCH_FLAG_ATTRIBUTE              0x00000010
#define SEARCH_FLAG_DATETIME_LASTWRITE     0x00000020
#define SEARCH_FLAG_DATETIME_CREATION      0x00000040
#define SEARCH_FLAG_DATETIME_LASTACCESS    0x00000080
#define SEARCH_FLAG_DATETIME_CHANGE        0x00000100
#define SEARCH_FLAG_ENDOFFILE              0x00000200
#define SEARCH_FLAG_ALLOCATIONSIZE         0x00000400
#define SEARCH_FLAG_ALT_STREAM             0x00001000
#define SEARCH_FLAG_EA                     0x00002000
#define SEARCH_FLAG_ATTR_MATCH_WHOLE_BITS  0x40000000
#define SEARCH_FLAG_OR_COMPARE             0x80000000

HRESULT
Search(
	HWND hWnd,
	FS_SELECTED_FILELIST *FileList,
	HANDLE *pHandleMatchedFiles
	);

HRESULT
SearchParamDialog(
	HWND hWnd,
	FS_SELECTED_FILELIST *FileList,
	SEARCH_PARAMETER *SearchParam
	);

inline HRESULT DeleteItemExPtr(FILEITEMEX *pItemEx)
{
	if( pItemEx == NULL )
		return E_INVALIDARG;
	LocalFree( pItemEx->hdr.FileName );
	LocalFree( pItemEx->hdr.Path );
	DeleteFileItemEx(pItemEx);
	return S_OK;
}

inline VOID InitDateTimeRange(SEARCH_RANGE_VALUE *psdt,SYSTEMTIME *pstFrom,SYSTEMTIME *pstTo)
{
	SystemTimeToTimeInteger(pstFrom,&psdt->From);
	SystemTimeToTimeInteger(pstTo,&psdt->To);
}
