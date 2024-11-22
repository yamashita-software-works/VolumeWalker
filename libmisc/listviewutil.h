#pragma once
//
// https://github.com/ysc3839/win32-darkmode
// 2024-06-24 Modified for Windows 7 WDK/Visual Studio 2010 build environment.
//

struct SubclassInfo
{
    SubclassInfo()
    {
        (DWORD)BackColor = -1;
        (DWORD)TextColor = -1;
        (DWORD)HeaderTextColor = -1;
    }
    COLORREF BackColor;
    COLORREF TextColor;
    COLORREF HeaderTextColor;
};

inline LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW)
            {
                LPNMCUSTOMDRAW nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
                switch (nmcd->dwDrawStage)
                {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;
                    case CDDS_ITEMPREPAINT:
                    {
                        SubclassInfo* info = reinterpret_cast<SubclassInfo*>(dwRefData);
                        SetTextColor(nmcd->hdc, info->HeaderTextColor);
                        return CDRF_DODEFAULT;
                    }
                }
            }
            break;
        }
        case WM_THEMECHANGED:
        {
            if( _IsDarkModeSupported() )
            {
                SubclassInfo* info = reinterpret_cast<SubclassInfo*>(dwRefData);

                HWND hHeader = ListView_GetHeader(hWnd);

                AllowDarkModeForWindow(hWnd, _IsDarkModeEnabled());
                AllowDarkModeForWindow(hHeader, _IsDarkModeEnabled());

                HTHEME hTheme;
                hTheme = OpenThemeData(nullptr, L"ItemsView");

                if( hTheme )
                {
                    COLORREF color;
                    if( (DWORD)info->TextColor == -1 )
                        GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color);
                    else
                        color = info->TextColor;

                    ListView_SetTextColor(hWnd, color);

                    if( (DWORD)info->BackColor == -1 )
                        GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color);
                    else        
                        color = info->BackColor;

                    ListView_SetTextBkColor(hWnd, color);
                    ListView_SetBkColor(hWnd, color);

                    CloseThemeData(hTheme);
                }

                hTheme = OpenThemeData(hHeader, L"Header");
                if( hTheme )
                {
                    SubclassInfo *info = reinterpret_cast<SubclassInfo*>(dwRefData);
                    GetThemeColor(hTheme, HP_HEADERITEM, 0, TMT_TEXTCOLOR, &(info->HeaderTextColor));
                    CloseThemeData(hTheme);
                }
                SendMessageW(hHeader, WM_THEMECHANGED, wParam, lParam);

                RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
            }
            break;
        }
        case WM_DESTROY:
        {
            SubclassInfo *info = reinterpret_cast<SubclassInfo*>(dwRefData);
            delete info;
        }
        break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

typedef struct _LVDARKMODECOLOR
{
    COLORREF BackColor;
    COLORREF TextColor;
    COLORREF HeaderTextColor;
} LVDARKMODECOLOR;

inline void InitDarkModeListView(HWND hListView,LVDARKMODECOLOR *pcrset=NULL)
{
    SubclassInfo *pInfo = new SubclassInfo;
    if( pcrset == NULL )
    {
        pInfo->BackColor = RGB(28,28,28);
    }
    else
    {
        pInfo->BackColor =pcrset->BackColor;
        pInfo->TextColor =pcrset->TextColor;
        pInfo->HeaderTextColor =pcrset->HeaderTextColor;
    }
    SetWindowSubclass(hListView, &SubclassProc, 0, reinterpret_cast<DWORD_PTR>(pInfo) );

    HWND hHeader = ListView_GetHeader(hListView);
    SetWindowTheme(hHeader,   L"ItemsView", nullptr); // or DarkMode_ItemsView
    SetWindowTheme(hListView, L"ItemsView", nullptr); // or DarkMode_ItemsView
}
