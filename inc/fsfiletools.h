#ifndef _FSFILETOOLS_
#define _FSFILETOOLS_

#ifndef _TFM_FIRST
#define _TFM_FIRST 6660
#endif

//
// File tools window class name
//
#define _TOOLHOST_WINDOWCLASS  L"FileToolsWindow"

//
// FTM_SETPATH
//
//
// wParam - zero (reserved)
//
// lParam - A pointer to a null-terminated buffer 
//          containing the fully-qualified path of the file.
//
// Return -
//
#define FTM_SETPATH       (WM_USER+_TFM_FIRST+2)

//
// FTM_GETPATH
//
//
// wParam -
//
// lParam -
//
// Return - 
//
#define FTM_GETPATH       (WM_USER+_TFM_FIRST+3)

//
// FTM_FILEOPERATION
//
//
// wParam - Pointer to a FO_PARAM structure.
//
// lParam - A command dependent parameter.
//
// Return - Returns HRESULT code.
//
#define FTM_FILEOPERATION   (WM_USER+_TFM_FIRST+10)  

typedef struct _FO_PARAM {
	HWND hwnd;
	UINT cmd;
	UINT Flags;
	HRESULT hr;
} FO_PARAM;

enum {
	FO_COPY_FILE = 1,
	FO_DELETE_FILE,
	FO_RENAME_FILE,
	FO_MKDIR,
	FO_SET_FILE_ATTRIBUTES,
	FO_SEARCH,
};

#define FileTool_FileOperation(hwndFT,fop,fl) ((HRESULT)SendMessage(hwndFT,FTM_FILEOPERATION,(WPARAM)fop,(LPARAM)fl))

#define FOF_FILEOPERATIONLIST  0x0
#define FOF_SELECTEDFILELIST   0x1

//
// FTM_NOTIFY_CLOSE
//
//
// wParam - 
//
// lParam - 
//
#define FTM_NOTIFY_CLOSE   (WM_USER+_TFM_FIRST+11)  

//
// File Operation Dialog
//
#include "fopfilelist.h"

EXTERN_C
HRESULT
WINAPI
FileOperationDialog(
	IN HWND hWnd,
	IN UINT cmd,
	IN IFileOperationList *pFiles
	);

//
// File Rename Dialog
//
EXTERN_C
HRESULT
WINAPI
FileRenameDialog(
	IN HWND hWnd,
	IN UINT cmd,
	IN IFileOperationList *pFiles
	);

//
// Make Directory Dialog
//
EXTERN_C
HRESULT
WINAPI
MakeDirectoryDialog(
	IN HWND hWnd,
	IN UINT cmd,
	IN PCWSTR SourcePath,
	IN PCWSTR TemplateDirName OPTIONAL,
	IN OUT PWSTR NewDirName OPTIONAL,
	IN INT cchNewDirName
	);

//
// Generic Result Viewer Dialog
//
EXTERN_C
HRESULT
WINAPI
ResultViewDialog(
	HWND hWnd,
	UINT cmd,
	IFileOperationList *pFiles
	);
#endif // _FSFILETOOLS_
