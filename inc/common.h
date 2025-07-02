#pragma once

#define PRIVATE_MESSAGE_BASE (0x6100)

//
// WM_CONTROL_MESSAGE
//
// wParam - LOWORD : Control Code (CODE_xxx)
//          HIWORD : must be zero
//
// lParam - A pointer to a structure that contains control code specific data.
//          Its format depends on the value of the LOWORD(wParam) parameter.
//          For more information, refer to the documentation for each application.
//
#define WM_CONTROL_MESSAGE  (PRIVATE_MESSAGE_BASE+0)

#define WM_NOTIFY_MESSAGE   (PRIVATE_MESSAGE_BASE+1)

#define WM_QUERY_MESSAGE    (PRIVATE_MESSAGE_BASE+2)

enum {
	UI_INIT_LAYOUT              = 0x1001,
//	OBSOLETE                    = 0x1002,
	UI_SELECT_ITEM              = 0x1003,
	UI_SET_FILE                 = 0x1004,
	UI_SET_DIRECTORY            = 0x1005,
	UI_CHANGE_DIRECTORY         = 0x1006,
	UI_SET_TITLE                = 0x1007,
	UI_SET_ICON                 = 0x1008,
//	OBSOLETE                    = 0x1009,
	UI_GET_SUBPANE              = 0x100A,
	UI_SET_SUBPANE              = 0x100B,
	UI_SET_FILELIST             = 0x100C,
	UI_SELECT_PAGE              = 0x100D,
	UI_SELECT_FILE              = 0x100E,
	UI_NOTIFY_ITEM_SELECTED     = 0x2001,
	UI_NOTIFY_VOLUME_SELECTED   = 0x2002,
//	OBSOLETE                    = 0x2003,
//	OBSOLETE                    = 0x2004,
	UI_NOTIFY_ITEM_ACTIVATED    = 0x2005,
//	OBSOLETE                    = 0x3001,
	UI_QUERY_CURRENTITEMNAME    = 0x3002,
	UI_QUERY_SELECTEDITEM       = 0x3003,
};

interface __declspec(novtable)  IInterface {
	virtual VOID Release(void);
};

interface __declspec(novtable)  ISaveViewConfig : public IInterface {
	virtual INT WriteValue(HWND hwndView,PCWSTR KeyName,PWSTR Value) = 0;
	virtual INT WriteValueInt(HWND hwndView,PCWSTR KeyName,INT Value) = 0;
};

interface __declspec(novtable)  ILoadViewConfig : public IInterface {
	virtual INT ReadValue(HWND hwndView,PCWSTR KeyName,PWSTR Value,UINT ValueLength) = 0;
	virtual INT ReadValueInt(HWND hwndView,PCWSTR KeyName,INT DefaultValue) = 0;
};

interface __declspec(novtable)  IApplication : public IInterface {
	virtual HRESULT QueryInterface(UINT InterfaceId,void **pv) = 0;
};

enum {
	I_UNKNOWN = 0,
	I_LOADCONFIG,
	I_SAVECONFIG,
};

typedef struct _PAGE_CONTEXT {
	UINT Flags;
	union {
		IApplication *MainApp;
	};
} PAGE_CONTEXT;

//
// UI_SELECT_ITEM
//
typedef struct _SELECT_ITEM
{
	UINT mask;          // Reserved
	UINT Flags;
	PWSTR pszPath;
	PWSTR pszName;
	PWSTR pszCurDir;
	union {             // Volume, Physical Drive, Storage Device
		PWSTR pszVolume;
		PWSTR pszPhysicalDrive;
	};
	FILE_ID_DESCRIPTOR FileId;
	UINT ViewType;
	GUID Guid;
	PAGE_CONTEXT *Context;
} SELECT_ITEM;

#define SI_MASK_PATH     0x1
#define SI_MASK_NAME     0x2
#define SI_MASK_CURDIR   0x4
#define SI_MASK_VIEWTYPE 0x8
#define SI_MASK_FILEID   0x10
#define SI_MASK_CONTEXT  0x20
#define SI_MASK_VOLUME   0x100

#define SI_FLAG_NOT_ADD_TO_HISTORY 0x1
#define SI_FLAG_ROOT_DIRECTORY     0x2
#define SI_FLAG_NO_UPDATE          0x8
#define SI_FLAG_CHANGE_PAGE_ONLY   (SI_FLAG_NO_UPDATE)
#define SI_FLAG_CREATE_PAGE        0x1000

typedef struct _SELECT_OFFSET_ITEM
{
	SELECT_ITEM hdr;
	LARGE_INTEGER liStartOffset;
} SELECT_OFFSET_ITEM;

#define SI_MASK_START_OFFSET  0x8000

//
// UI_SELECT_PAGE
//
typedef struct _UIS_PAGE
{
	UINT ConsoleTypeId;
	GUID ConsoleGuid;
	PWSTR pszPath;
	PWSTR pszFileName;
	DWORD dwFlags;
	PAGE_CONTEXT *Context;
} UIS_PAGE;

//
// UI_QUERY_CURRENTITEMNAME and more String message
//
typedef struct _STRING_STRUCT
{
	ULONG Length;        // length is cb, not cch
	ULONG MaximumLength; // length is cb, not cch
	PWSTR Buffer;
} STRING_STRUCT;

//
// UI_NOTIFY_ITEM_ACTIVATED
//
typedef struct _UIS_ITEM_ACTIVATED
{
	HWND hwndFrom;
	UINT ConsoleTypeId;
	PWSTR Path;
} UIS_ITEM_ACTIVATED;

//
// UI_QUERY_SELECTEDITEM
//
typedef struct _UIS_SELECTEDITEM
{
	UINT ConsoleTypeId;
	PWSTR Path;
	int cchPath;
} UIS_SELECTEDITEM;

//
// WM_QUERY_CMDSTATE
//
// wParam - LOWORD : Command ID
//          HIWORD : 0
// lParam - Pointer to UINT that receives the state (UPDUI_xxx) flag.
//
#define WM_QUERY_CMDSTATE   (PRIVATE_MESSAGE_BASE+10)

enum {
    UPDUI_ENABLED  = 0x00000000,
    UPDUI_DISABLED = 0x00000100,
    UPDUI_CHECKED  = 0x00000200,
    UPDUI_CHECKED2 = 0x00000400,
    UPDUI_RADIO	   = 0x00000800,
};

//
// WM_PRETRANSLATEMESSAGE
//
// wParam -
// lParam - Pointer to MSG structure.
//
#define WM_PRETRANSLATEMESSAGE   (PRIVATE_MESSAGE_BASE+11)

//
// WM_OPEM_MDI_CHILDFRAME
//
// wParam -
// lParam -
//
#define WM_OPEN_MDI_CHILDFRAME   (PRIVATE_MESSAGE_BASE+12)

//
// WM_MDI_CHILDFRAME_CLOSE
//
// wParam -
// lParam -
//
#define WM_MDI_CHILDFRAME_CLOSE   (PRIVATE_MESSAGE_BASE+13)

//
// WM_MDI_CHILDFRAME_NCDESTROY
//
// wParam -
// lParam -
//
#define WM_MDI_CHILDFRAME_NCDESTROY   (PRIVATE_MESSAGE_BASE+14)

