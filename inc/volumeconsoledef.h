#ifndef _DEF_VOLUME_CONSOLE_DEF
#define _DEF_VOLUME_CONSOLE_DEF

typedef struct _CONSOLE_VIEW_ID {
	UINT wndId;
	GUID wndGuid;
} CONSOLE_VIEW_ID; 

#define _PROP_CONSOLE_VIEW_ID L"ConsoleViewId"

#ifdef __cplusplus
inline CONSOLE_VIEW_ID *_GET_CONSOLE_VIEW_ID(HWND hwndView)
{
	return (CONSOLE_VIEW_ID *)GetProp(hwndView,_PROP_CONSOLE_VIEW_ID);
}
#else
#define _GET_CONSOLE_VIEW_ID(hwndView) ((CONSOLE_VIEW_ID *)GetProp(hwndView,_PROP_CONSOLE_VIEW_ID))
#endif
#endif
