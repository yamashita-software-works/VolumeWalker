#ifndef _PRIVATE_COMMON_MESSAGE_
#define _PRIVATE_COMMON_MESSAGE_

#include "console_id.h"
#include "common_resid.h"
#include "filelistbuffer.h"

//
// Private Message Base
//
#define PM_BASE (WM_APP+0x6600)

//
// PM_GETSELECTEDFILE
//   wParam  -
//   lParam  -
//   lResult -
//
// NOTE:
// The FS_SELECTED_FILE.pszPath field is must free buffer memory use LocalFree().
//
#define PM_GETSELECTEDFILE (PM_BASE+1)

typedef struct _FS_SELECTED_FILE
{
	ULONG FileAttributes;
	ULONG cchPath; // Reserved.
	PWSTR pszPath;
} FS_SELECTED_FILE;

//
// PM_GETSELECTEDFILELIST
//   wParam  -
//   lParam  -
//   lResult -
//
#ifndef _NO_NT_FILESYSTEM
#define PM_GETSELECTEDFILELIST (PM_BASE+2)

typedef struct _FS_SELECTED_FILELIST
{
	FS_FILELISTBUFFER *FileListBuffer;
	PWSTR PathFrom;
	PWSTR PathTo;
} FS_SELECTED_FILELIST;
#endif

//
// PM_GETWORKINGDIRECTORY
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_GETWORKINGDIRECTORY (PM_BASE+3)

//
// PM_GETCURDIR
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_GETCURDIR (PM_BASE+4)

//
// PM_GETSELECTEDFILEATTRIBUTE
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_GETSELECTEDFILEATTRIBUTE (PM_BASE+5)

//
// PM_QUERYSELECTEDITEMSTATE
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_QUERYSELECTEDITEMSTATE (PM_BASE+6)

#define QSIS_SELECTED          (0x1)
#define QSIS_MULTIITEMSELECTED (0x2)
#define QSIS_EMPTYLIST         (0x4)

//
// PM_FILEOPERATION
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_FILEOPERATION (PM_BASE+10)

//
// PM_FINDITEM
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_FINDITEM (PM_BASE+20)

#define FIND_QUERYOPENDIALOG  0
#define FIND_OPENDIALOG       1
#define FIND_CLOSEDIALOG      2
#define FIND_SEARCH           3
#define FIND_SEARCH_NEXT      4

//
// PM_MAKECONTEXTMENU
//   wParam  - MAKEWPARAM(ConsoleTypeId,0)
//   lParam  - Menu handle
//   lResult - If updated to the menu, return TRUE otherwise FALSE.
//
#define PM_MAKECONTEXTMENU (PM_BASE+21)

//
// PM_GETCONFIGINFORMATION
//   wParam  - HWND
//   lParam  - Pointer to CONFIG_STRUCT
//   lResult - If exist information, return Pointer to CONFIG_STRUCT.
//
#define PM_GETCONFIGINFORMATION  (PM_BASE+22)

typedef struct _CONFIG_STRUCT
{
	DWORD dwFlags;
	PWSTR SectionName;
	PWSTR IniFileName;
	PWSTR Columns;
	PWSTR SortColumn;
} CONFIG_STRUCT;

inline VOID FreeConfigStruct(CONFIG_STRUCT *pcs)
{
	if( pcs ) {
		if( pcs->IniFileName ) {
			CoTaskMemFree(pcs->IniFileName);
			pcs->IniFileName = NULL;
		}
		if( pcs->SectionName ) {
			CoTaskMemFree(pcs->SectionName);
			pcs->SectionName = NULL;
		}
		if( pcs->Columns ) {
			CoTaskMemFree(pcs->Columns);
			pcs->Columns = NULL;
		}
		if( pcs->SortColumn ) {
			CoTaskMemFree(pcs->SortColumn);
			pcs->Columns = NULL;
		}
	}
}

//
// PM_GETFILETOOLSOBJECT
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_GETFILETOOLSOBJECT  (PM_BASE+23)

//
// PM_GETFILETOOLSOBJECT
//   wParam  - 0
//   lParam  - Pointer to PMS_DESCRIPTION_TEXT structure.
//   lResult -
//
#define PM_SET_DESCRIPTION_TEXT (PM_BASE+24)

typedef struct _PMS_DESCRIPTION_TEXT
{
	PWSTR pszTitle;
	PWSTR pszDescription;
} PMS_DESCRIPTION_TEXT,*PPMS_DESCRIPTION_TEXT;

//
// PM_USERBASE
//
// User private message base
//
#define PM_USERBASE      (PM_BASE+100)
#define PM_PRIVATEBASE   PM_USERBASE

//
// PM_GETSELECTEDFILE
//   wParam  -
//   lParam  -
//   lResult -
//
// NOTE:
// The FS_SELECTED_FILE.pszPath field is must free buffer memory use LocalFree().
//
//#define PM_GETSELECTEDFILE (PM_BASE+1)
//
//typedef struct _FS_SELECTED_FILE
//{
//	ULONG FileAttributes;
//	ULONG cchPath; // Reserved.
//	PWSTR pszPath;
//} FS_SELECTED_FILE;

//
// PM_UPDATETITLE
//   wParam  -
//   lParam  - Path or VolumeName
//   lResult -
//
// NOTE:
// Update frame window title.
//
#define PM_UPDATETITLE (PM_BASE+2)

//////////////////////////////////////////////////////////////////////////////
// old compatible message
// refer to FSFileList/FSUtilGUI document.
//
#define PM_SELECT_ON_FILELIST (PM_BASE+8100)

//////////////////////////////////////////////////////////////////////////////
// WM_OPEN_MDI_CHILDFRAME application use structure

typedef struct _OPEN_MDI_CHILDFRAME_PARAM
{
	HWND hwndFrom;
	UINT Flags;
	union {
		PWSTR Path;
		HANDLE Handle;
	};
	LARGE_INTEGER StartOffset;
} OPEN_MDI_CHILDFRAME_PARAM;

#define OMCPF_USE_PATH       0x0
#define OMCPF_USE_HANDLE     0x1

//////////////////////////////////////////////////////////////////////////////
// WM_QUERY_MESSAGE application use sub function

#define QMT_GETVOLUMEPATH     (1)
#define QMT_GETSTARTOFFSET    (2)
#define QMT_GETFILEPATH       (3)

typedef struct _QM_PARAM
{
	DWORD dwLength;
	union {
		PWSTR VolumePath;
		LARGE_INTEGER liValue;
	};
} QM_PARAM;

//////////////////////////////////////////////////////////////////////////////
// IViewBaseWindow::GetString/GetStringBstrWM_QUERY_MESSAGE application use sub function

enum {
	VIEW_STR_TITLE=1,
};
#endif