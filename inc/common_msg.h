#ifndef _PRIVATE_COMMON_MESSAGE_
#define _PRIVATE_COMMON_MESSAGE_

//
// Private Message Base
//
#define PM_BASE (WM_APP+0x6600)

//
// PM_GETCURDIR
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_GETCURDIR (PM_BASE+4)

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
	UINT flags;
	UINT reserved;
	PWSTR Path;
} OPEN_MDI_CHILDFRAME_PARAM;

typedef struct _OPEN_MDI_CHILDFRAME_STARTOFFSET
{
	OPEN_MDI_CHILDFRAME_PARAM hdr;
	LARGE_INTEGER StartOffset;
} OPEN_MDI_CHILDFRAME_STARTOFFSET;

//////////////////////////////////////////////////////////////////////////////
// WM_QUERY_MESSAGE application use sub function

#define WQ_GETVOLUMEPATH     (1)
#define WQ_GETSTARTOFFSET    (2)

typedef struct _WQ_PARAM
{
	DWORD dwLength;
	union {
		PWSTR VolumePath;
		LARGE_INTEGER liValue;
	};
} WQ_PARAM;

#endif