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

// WPARAM LOWORD(f)
enum {
	CTRL_NOTIFY_NONE = 0,
	CTRL_INIT_LAYOUT,
	NOTIFY_VOLUME_SELECTED,
};

typedef struct _SELECT_ITEM
{
	UINT mask;          // Reserved
	UINT Flags;         // Reserved
	PWSTR pszPath;
	PWSTR pszName;
	PWSTR pszCurDir;
	union {
		UINT ViewType;  // Depends an application.
		struct {
			UINT View;  // Reserved
			UINT Page;  // Reserved
		};
	};
} SELECT_ITEM;

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
#define WM_OPEM_MDI_CHILDFRAME   (PRIVATE_MESSAGE_BASE+12)

//
// WM_MDI_CHILDFRAME_CLOSE
//
// wParam -
// lParam -
//
#define WM_MDI_CHILDFRAME_CLOSE   (PRIVATE_MESSAGE_BASE+13)
