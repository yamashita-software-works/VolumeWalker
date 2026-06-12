#pragma once

HINSTANCE _GetResourceInstance();
HICON SetWindowIcon(HWND hWnd,SHSTOCKICONID ssii);
VOID DrawFocusFrame(HWND hWnd,HDC hdc,RECT *prc,BOOL bDrawFocus=FALSE,COLORREF crBorder=RGB(80,110,190));
PWSTR GetIniFilePath();
HFONT GetGlobalFont(HWND hWnd);
HFONT GetIconFont();


#include "interface.h"
HRESULT CreateApplicationList(IApplicationsReader **ppApps);
HRESULT DestroyApplicationList();





