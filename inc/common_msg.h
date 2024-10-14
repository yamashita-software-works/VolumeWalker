#ifndef _PRIVATE_COMMON_MESSAGE_
#define _PRIVATE_COMMON_MESSAGE_

//
// Private Message Base
//
#define PM_BASE (WM_APP+0x6600)

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
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_MAKECONTEXTMENU (PM_BASE+21)

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
#define PM_GETSELECTEDFILE (PM_BASE+1)

typedef struct _FS_SELECTED_FILE
{
	ULONG FileAttributes;
	ULONG cchPath; // Reserved.
	PWSTR pszPath;
} FS_SELECTED_FILE;

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
// WM_MDI_SAVECONFIGURATION application use structure

typedef struct _CONFIG_STRUCT
{
	PWSTR SectionName;
	PWSTR IniFileName;
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
	}
}

//////////////////////////////////////////////////////////////////////////////
// WM_OPEM_MDI_CHILDFRAME application use structure

typedef struct _OPEN_MDI_CHILDFRAME_PARAM
{
	HWND hwndFrom;
	UINT Flags;
	PWSTR Path;
	LARGE_INTEGER StartOffset;
} OPEN_MDI_CHILDFRAME_PARAM;

//////////////////////////////////////////////////////////////////////////////
// WM_QUERY_MESSAGE application use sub function

#define QMT_GETVOLUMEPATH     (1)
#define QMT_GETSTARTOFFSET    (2)

typedef struct _QM_PARAM
{
	DWORD dwLength;
	union {
		PWSTR VolumePath;
		LARGE_INTEGER liValue;
	};
} QM_PARAM;

#endif