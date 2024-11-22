//****************************************************************************
//*                                                                          *
//*   MenuBar.cpp                                                            *
//*                                                                          *
//*   PURPOSE: Cool menubar class                                            *
//*                                                                          *
//*   AUTHOR:  Katuhiro Yamashita                                            *
//*                                                                          *
//*   HISTORY: 1999.12.09 Create from MSJ article                            *
//*            2001.03.03 Update                                             *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
/*++
   Original code: MSJ 

   2024-06-26 Cleanup and refactoring legacy code (in the 90~00's).
              Comment text translated Japanese to English.
              This code may not available in MDI menu because any part and
              resource removed for open sourcing.
--*/
#include "stdafx.h"
#include "MenuBar.h"

#ifndef countof
#define countof(x)  (sizeof(x)/sizeof(x[0]))
#endif

#define _COPYMODE 

inline HMODULE _GetResourceModule()
{
    return LIBMISC::g_ResourceInstance;
}

inline HWND _OwnerWindow()
{
    return GetActiveWindow(); // todo:
}

static int CXGAP = 8;               // num pixels between button and text
static int CXTEXTMARGIN = 8;        // num pixels after hilite to start text
static SIZE m_sizeButton = { 16, 16 };

//////////////////////////////////////////////////////////////////////////////
// NT 5.0 Compatible definition (copy from Platform SDK)
//
#if(WINVER < 0x0500)

#define MNS_NOCHECK          0x80000000
#define MNS_MODELESS         0x40000000
#define MNS_DRAGDROP         0x20000000
#define MNS_AUTODISMISS      0x10000000
#define MNS_NOTIFYBYPOS      0x08000000
#define MNS_CHECKORBMP       0x04000000

#define MIM_MAXHEIGHT        0x00000001
#define MIM_BACKGROUND       0x00000002
#define MIM_HELPID           0x00000004
#define MIM_MENUDATA         0x00000008
#define MIM_STYLE            0x00000010
#define MIM_APPLYTOSUBMENUS  0x80000000
typedef struct tagMENUINFO
{
    DWORD   cbSize;
    DWORD   fMask;
    DWORD   dwStyle;
    UINT    cyMax;
    HBRUSH  hbrBack;
    DWORD   dwContextHelpID;
    ULONG_PTR dwMenuData;
}   MENUINFO, FAR *LPMENUINFO;
typedef MENUINFO CONST FAR *LPCMENUINFO;

#define MIIM_STRING      0x00000040
#define MIIM_BITMAP      0x00000080
#define MIIM_FTYPE       0x00000100

#else  // WINVER < 0x0500

#ifdef UNICODE
  #define MENUITEMINFOW_NT5 MENUITEMINFOW
#endif

#endif // WINVER < 0x0500

#if _WIN32_WINNT < 0x500
typedef struct tagMENUITEMINFOW_NT5
{
    UINT     cbSize;
    UINT     fMask;
    UINT     fType;         // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
    UINT     fState;        // used if MIIM_STATE
    UINT     wID;           // used if MIIM_ID
    HMENU    hSubMenu;      // used if MIIM_SUBMENU
    HBITMAP  hbmpChecked;   // used if MIIM_CHECKMARKS
    HBITMAP  hbmpUnchecked; // used if MIIM_CHECKMARKS
    ULONG_PTR dwItemData;   // used if MIIM_DATA
    LPWSTR   dwTypeData;    // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    UINT     cch;           // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    HBITMAP  hbmpItem;      // used if MIIM_BITMAP (NT 5.0)
} MENUITEMINFOW_NT5;
#endif

typedef struct tagMENUITEMINFOA_NT5
{
    UINT     cbSize;
    UINT     fMask;
    UINT     fType;         // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
    UINT     fState;        // used if MIIM_STATE
    UINT     wID;           // used if MIIM_ID
    HMENU    hSubMenu;      // used if MIIM_SUBMENU
    HBITMAP  hbmpChecked;   // used if MIIM_CHECKMARKS
    HBITMAP  hbmpUnchecked; // used if MIIM_CHECKMARKS
    ULONG_PTR dwItemData;   // used if MIIM_DATA
    LPSTR    dwTypeData;    // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    UINT     cch;           // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    HBITMAP  hbmpItem;      // used if MIIM_BITMAP (NT 5.0)
} MENUITEMINFOA_NT5;

typedef struct tagMENUITEMINFOA_NT4
{
    UINT     cbSize;
    UINT     fMask;
    UINT     fType;         // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
    UINT     fState;        // used if MIIM_STATE
    UINT     wID;           // used if MIIM_ID
    HMENU    hSubMenu;      // used if MIIM_SUBMENU
    HBITMAP  hbmpChecked;   // used if MIIM_CHECKMARKS
    HBITMAP  hbmpUnchecked; // used if MIIM_CHECKMARKS
    ULONG_PTR dwItemData;   // used if MIIM_DATA
    LPSTR    dwTypeData;    // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    UINT     cch;           // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
} MENUITEMINFOA_NT4;

typedef struct tagMENUITEMINFOW_NT4
{
    UINT     cbSize;
    UINT     fMask;
    UINT     fType;         // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
    UINT     fState;        // used if MIIM_STATE
    UINT     wID;           // used if MIIM_ID
    HMENU    hSubMenu;      // used if MIIM_SUBMENU
    HBITMAP  hbmpChecked;   // used if MIIM_CHECKMARKS
    HBITMAP  hbmpUnchecked; // used if MIIM_CHECKMARKS
    ULONG_PTR dwItemData;   // used if MIIM_DATA
    LPWSTR   dwTypeData;    // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    UINT     cch;           // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
} MENUITEMINFOW_NT4;

#ifdef _UNICODE
#define MENUITEMINFO_NT4 MENUITEMINFOW_NT4
#define MENUITEMINFO_NT5 MENUITEMINFOW_NT5  
#else
#define MENUITEMINFO_NT4 MENUITEMINFOA_NT4
#define MENUITEMINFO_NT5 MENUITEMINFOA_NT5
#endif

typedef BOOL(WINAPI *SETMENUINFO)(HMENU,LPCMENUINFO);
SETMENUINFO pfnSetMenuInfo = NULL;

void _SetSystemMenuStyle(HMENU hMenu)
{
    if( pfnSetMenuInfo )
    {
        MENUINFO mi;
        memset(&mi,0,sizeof(mi));
        mi.cbSize  = sizeof(MENUINFO);
        mi.fMask   = MIM_STYLE;
        mi.dwStyle = MNS_CHECKORBMP;//MNS_NOCHECK;
        pfnSetMenuInfo(hMenu,&mi);
    }
}

//////////////////////////////////////////////////////////////////////////////
// CMenuBar Implements

CMenuBar *CMenuBar::m_pThis;

CMenuBar::CMenuBar()
{
    m_iTrackingState = TRACK_NONE;       // initial state: not tracking 
    m_iPopupTracking = m_iNewPopup = -1; // invalid
    m_hMenu          = NULL;
    m_bSysMenu       = FALSE;
    m_iOpenMenuIndex = -1;
    m_pThis          = this;
    m_bActive        = FALSE;
    m_cxMDIMenu      = GetSystemMetrics(SM_CXMENUSIZE);
    m_hwndMDIActive  = NULL;
    m_bMDISysMenuInMenuBar = FALSE;   // 990320 MDI1.0
    m_dwDrawFlags    = 0;             // 010301
    m_nPopupLoop     = 0;             // 011201

    m_hMenuImages    = ImageList_Create(16,16,/*ILC_COLOR32|*/ILC_MASK|ILC_COLORDDB,16,8); // REP:031224
    UpdateFont();

    pfnSetMenuInfo   = (SETMENUINFO)GetProcAddress(GetModuleHandle(TEXT("USER32.DLL")),"SetMenuInfo");
    m_fIgnoreAltUp   = FALSE;

    m_hFontIcon = NULL;
}

CMenuBar::~CMenuBar()
{
    DestroyMenu();

    for(int i = 0;i < m_StringID.GetCount(); i++)
    {
        STRINGID* p = m_StringID[i];
        delete p;
    }

    if( m_hFontIcon )
        DeleteObject(m_hFontIcon);
}

HWND CMenuBar::Create(HWND hwndOwner,HINSTANCE hInst,int nID,DWORD dwStyle)
{
    m_hWnd = CreateToolbarEx(
                    hwndOwner,
                    dwStyle,
                    nID,
                    0,
                    hInst,
                    (UINT)-1,
                    NULL,0, // LPTBBUTTON,count
                    0,0,    // Button Size
                    0,0,    // Bitmap Image Size 
                    sizeof(TBBUTTON));

    SubclassToolBar();

    UpdateFont();

    return m_hWnd;
}

BOOL CMenuBar::DestroyMenu()
{
    ::DestroyMenu(GetMenu());
    return TRUE;
}

HMENU CMenuBar::InternalLoadMenu(HMENU hmenu)
{
    UINT nMenuItems = GetMenuItemCount(hmenu);

    SetBitmapSize(0,0);
    SetButtonSize(0,0);
//  SetBitmapSize(0,0);
//  SetButtonSize(0,30); BUGBUG?:050929 - If make this call on Windows Vista Beta2 (Build 5219), 
//                                        the menu bar will become abnormally hight? (such as 120 pixels).

    //
    // Delete existing buttons.
    //
    int nCount = GetButtonCount();
    while (nCount--)
    {
        SendMessage(m_hWnd,TB_DELETEBUTTON,0,0);
    }

    // Add buttons
    //
//++050930:Windows Vista - If delete all the buttons, the font set with WM_SETFONT will be deleted and revert to the default font.
    UpdateFont();
//--050930

    for(UINT i=0; i < nMenuItems; i++)
    {
        TCHAR name[64];

        ZeroMemory(name,sizeof(name));

        if( GetMenuString(hmenu,i,name,countof(name)-1,MF_BYPOSITION) )
        {
            TBBUTTON tb;

            ZeroMemory(&tb,sizeof(tb));

            tb.idCommand = GetMenuItemID(hmenu,i);

            // WARNING!!
            // As can be seen from the return value of AddStrings(), 
            // the index of the added string is increasing. This may mean 
            // that deleting a button with TB_DELETEBUTTON does not remove it
            // from memory. This may cause a memory leak when the main menu needs
            // to be reloaded many times, for example, in MDI.
            int iString = -1;

            for(int n = 0; n < (int)m_StringID.GetCount(); n++)
            {
                if( lstrcmpi(m_StringID[n]->buf,name) == 0 )
                {
                    iString = m_StringID[n]->iString;
                    break;
                }
            }

            if( iString == -1 )
            {
                iString = AddStrings(name);

                if( iString != -1 )
                {
                    STRINGID *pstrid = new STRINGID;
                    pstrid->buf = name;
                    pstrid->iString = iString;
                    m_StringID.Add( pstrid );
                }
            }

            if( iString != -1 )
            {
                tb.iString = iString;
                tb.fsState = TBSTATE_ENABLED;
                tb.fsStyle = TBSTYLE_AUTOSIZE;
                tb.iBitmap = -1;
                tb.idCommand = i;
                AddButtons(1,&tb);
            }
        }
    }

    AutoSize();

    m_hMenu = hmenu;

    return hmenu;
}

HMENU CMenuBar::LoadMenu(LPCTSTR pszMenuName)
{
    HMENU hMenu;

    if( m_hMenu )               // BUGBUG:001102 Object leak!
        ::DestroyMenu(m_hMenu); // BUGBUG:001102

    hMenu = InternalLoadMenu(::LoadMenu(_GetResourceModule(),pszMenuName));
    if( hMenu == NULL )
        return NULL;

    if( m_bMDISysMenuInMenuBar )
        EnableMDISysMenuItem(TRUE);

    return hMenu;
}

HMENU CMenuBar::LoadMenu(HMENU hNewMenu)
{
    HMENU hMenu;

    if( m_hMenu )               // BUGBUG:001102 Object leak!
        ::DestroyMenu(m_hMenu); // BUGBUG:001102

    hMenu = InternalLoadMenu( hNewMenu );
    if( hMenu == NULL )
        return NULL;

    if( m_bMDISysMenuInMenuBar )
        EnableMDISysMenuItem(TRUE);

    return hMenu;
}

void CMenuBar::UpdateFont()
{
    static HFONT hFont = 0;

    NONCLIENTMETRICS info;

    info.cbSize = sizeof(info);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(info), &info, 0);

    if ((HFONT)hFont)
    {
        DeleteObject(hFont);
    }
    hFont = CreateFontIndirect(&info.lfMenuFont);

    SendMessage(m_hWnd,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE, 0));
}

LRESULT CMenuBar::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch( message )
    {
        case WM_LBUTTONDOWN:
            return OnLButtonDown(wParam,lParam);
        case WM_MOUSEMOVE:
            return OnMouseMove(wParam,lParam);
    }

    return 0;
}

static WNDPROC pfnOldProc = NULL;

LRESULT CALLBACK CMenuBar::SubclassWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if( m_pThis->WindowProc(hwnd,message,wParam,lParam) )
    {
        return 0;
    }

    return (CallWindowProc ((WNDPROC)(LPVOID)(pfnOldProc), hwnd, message, wParam, lParam));
}

VOID CMenuBar::SubclassToolBar()
{
    pfnOldProc = (WNDPROC)GetWindowLongPtr (m_hWnd, GWLP_WNDPROC);
    SetWindowLongPtr (m_hWnd, GWLP_WNDPROC,  (LONG_PTR)SubclassWndProc);
}

//////////////////////////////////////////////////////////////////////////////

static CMenuBar*    g_pMenuBar = NULL;
static HHOOK        g_hMsgHook = NULL;

/////////////////////////////////////////////////////////////////////////////
// CMenuBar

BOOL CMenuBar::TranslateFrameMessage(MSG* pMsg)
{
    ASSERT(pMsg);

    UINT msg = pMsg->message;

    if (WM_LBUTTONDOWN <= msg && msg <= WM_MOUSELAST)
    {
        if (pMsg->hwnd != m_hWnd && m_iTrackingState > 0)
        {
            // user clicked outside menu bar: exit tracking mode
            SetTrackingState(TRACK_NONE);
        }
    }
    else if ( msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_KEYDOWN )
    {

        BOOL bAlt = HIWORD(pMsg->lParam) & KF_ALTDOWN; // Alt key down

        TCHAR vkey = (TCHAR)pMsg->wParam;              // get virt key

        if (vkey == VK_MENU || 
           (vkey == VK_F10 && !((GetKeyState(VK_SHIFT) & 0x80000000)||(GetKeyState(VK_CONTROL) & 0x80000000)||bAlt)))
        {
            // key is VK_MENU or F10 with no alt/ctrl/shift: toggle menu mode
            //
            // We need to check with in WM_SYSKEYUP because the menu 
            // will react first when you use combinations such as Alt+Tab.

            // Same action the F10 key for Alt Down/Up.
            if( vkey == VK_F10 )            // 990322
            {                               // 990322
                if (msg == WM_SYSKEYDOWN)   // 990322
                    bAlt = TRUE;            // 990322
                else                        // 990322
                    bAlt = FALSE;           // 990322
            }                               // 990322

            if (msg == WM_SYSKEYUP)
            {
                // Fixed a bug where the menu would not be deactivated
                // when activating a menu with the ALT key, popping up a menu, 
                // tracking it, and then closing the menu with the ALT key.
//++Q2401
                if( !m_fIgnoreAltUp ) // 010303
                {
                    ToggleTrackButtonMode();
                }
                else
                {
                    m_fIgnoreAltUp = FALSE; // 010303
                }
//--Q2401
            }
            if( bAlt )                  // 010301
                SetUnderLineMode(TRUE); // 010301

            return TRUE;
        }
        else if ((msg == WM_SYSKEYDOWN || msg == WM_KEYDOWN))
        {
            if (m_iTrackingState == TRACK_BUTTON)
            {
                // I am tracking: handle left/right/up/down/space/Esc
                switch (vkey)
                {
                case VK_LEFT:
                case VK_RIGHT:
                    // left or right-arrow: change hot button if tracking buttons
                    SetHotItem(GetNextOrPrevButton(GetHotItem(), vkey==VK_LEFT));
                    return TRUE;

//              case VK_SPACE:  // (personally, I like SPACE to enter menu too)
                case VK_UP:
                case VK_DOWN:
                    // up or down-arrow: move into current menu, if any
                    TrackPopup(GetHotItem(),TRUE);
                    return TRUE;

                case VK_ESCAPE:
                    // escape key: exit tracking mode
                    SetTrackingState(TRACK_NONE);
                    SetUnderLineMode(FALSE); // 010301 Clear Under Line
                    return TRUE;
                }
            }

            // Handle alphanumeric key: invoke menu. Note that Alt-X
            // chars come through as WM_SYSKEYDOWN, plain X as WM_KEYDOWN.
            UINT c = MapVirtualKey( vkey, 2 );  // MSJ-BUGBUG:990409
//          if ((bAlt || m_iTrackingState == TRACK_BUTTON) && (isalnum(vkey) || isspace(vkey)))  // MSJ-BUGBUG:990409
            if ((bAlt || m_iTrackingState == TRACK_BUTTON) && (isalnum(c) || isspace(c) || c == '-')) // 990624
            {
                // Alt-X, or else X while in tracking mode
                UINT nID = 0;

                // Opens the MDI system menu only when the MDI child window is maximized.
                //
                BOOL bMDISysMenu = FALSE;                     // 990624
                if( m_bMDISysMenuInMenuBar && c == '-' )      // 990624
                    bMDISysMenu = TRUE;                       // 990624
                
//              if (MapAccelerator(vkey,&nID) || bMDISysMenu) // 990624
                if (MapAccelerator((TCHAR)c,&nID) || bMDISysMenu) // 990624,BUGFIX:001115
                {
//++040319
                    if( IsButtonVisible(NULL,nID) )
                        TrackPopup(nID,TRUE);    // found menu mnemonic: track it
//--040319
                    return TRUE; // handled
                }
                else if (m_iTrackingState == TRACK_BUTTON && !bAlt)
                {
                    ASSERT(0);
                    return TRUE;
                }
            }
            // Default for any key not handled so far: return to no-menu state
            if (m_iTrackingState > 0)
            {
                SetTrackingState(TRACK_NONE);
            }
        }
    }
    return FALSE; // not handled, pass along
}

void CMenuBar::TrackPopup(int iButton,BOOL bSelectItem)
{
    m_nPopupLoop = 0; // 011201

    while (iButton >= 0) // while user selects another menu
    {
        BOOL bMenuOpen;

        m_iNewPopup = -1;                // assume quit after this
        PressButton(iButton, TRUE);      // press the button
        UpdateWindow(m_hWnd);            // and force repaint now

        // NOTE:
        // post a simulated arrow-down into the message stream
        // so TrackPopupMenu will read it and move to the first item
        //
        // This behavior should only occur when the menu is opened with a keyboard shortcut,
        // or when an open popup menu is navigated or modified with the left and right keyboard keys. 
        // When selecting a popup menu by tracking the mouse, the first item should not be select.
        if( bSelectItem )
        {
            PostMessage(_OwnerWindow(),WM_KEYDOWN, VK_DOWN, 1);
            PostMessage(_OwnerWindow(),WM_KEYUP, VK_DOWN, 1);
        }

        SetTrackingState(TRACK_POPUP, iButton); // enter tracking state

        // Need to install a hook to trap menu input in order to make
        // left/right-arrow keys and "hot" mouse tracking work.
        //
        // get submenu and display it beneath button
        TPMPARAMS tpm;
        RECT      rcButton;
        GetRect(iButton, &rcButton);
        rcButton.right = rcButton.right - rcButton.left;
        ClientToScreen(m_hWnd,(LPPOINT)&rcButton);
        rcButton.right += rcButton.left;
        POINT pt = ComputeMenuTrackPoint(rcButton, tpm);

        HMENU hMenuPopup = ::GetSubMenu(m_hMenu, iButton);
#ifdef _COPYMODE
        hMenuPopup = CopyMenu(NULL,hMenuPopup,TRUE);
#endif
        ASSERT(hMenuPopup);

        // 010401
        UINT uFlags = TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL|TPM_TOPALIGN;
        pt = AdjustComputeMenuTrackPoint(hMenuPopup,rcButton,pt,tpm.rcExclude,uFlags);

        bMenuOpen = TRUE;
#if 0 
        // An example of exiting the menu loop when an overflowing menu item is selected. 
        // Used when using a chevron menu.
        RECT rc;
        GetWindowRect(m_hWnd,&rc);
        if( rc.right < pt.x )
        {
            bMenuOpen = FALSE;
        } 
#endif
        if( bMenuOpen )
        {
            ASSERT(g_pMenuBar == NULL);
            g_pMenuBar = this; // 011217

            ASSERT(g_hMsgHook == NULL);
            g_hMsgHook = SetWindowsHookEx(WH_MSGFILTER,MenuInputFilter,NULL,::GetCurrentThreadId()); // 011217

            m_iOpenMenuIndex = iButton; // 000127

            m_nPopupLoop++; // 011201

            // Show the Popup-Menu!
            TrackPopupMenuEx(hMenuPopup,uFlags,pt.x,pt.y,_OwnerWindow(),&tpm);

            // uninstall hook.
            ::UnhookWindowsHookEx(g_hMsgHook);
            g_hMsgHook = NULL;
            g_pMenuBar = NULL;
        }

#ifdef _COPYMODE
        ::DestroyMenu(hMenuPopup);
#endif
        PressButton(iButton, FALSE);     // un-press button
        UpdateWindow(m_hWnd);            // and force repaint now

        // If the user exited the menu loop by pressing Escape,
        // return to track-button state; otherwise normal non-tracking state.
        SetTrackingState(m_bEscapeWasPressed ? TRACK_BUTTON : TRACK_NONE, iButton);

        // If the user moved mouse to a new top-level popup (eg from File to
        // Edit button), I will have posted a WM_CANCELMODE to quit
        // the first popup, and set m_iNewPopup to the new menu to show.
        // Otherwise, m_iNewPopup will be -1 as set above.
        // So just set iButton to the next popup menu and keep looping...
        iButton = m_iNewPopup;
    }

    if( m_iTrackingState == TRACK_NONE ) // @@@MARKED
        SetUnderLineMode(FALSE); // 010301 Clear Under Line @@@MARKED
}

void CMenuBar::SetTrackingState(TRACKINGSTATE iState, int iButton)
{
    if (iState != m_iTrackingState) 
    {
        if (iState == TRACK_NONE)
            iButton = -1;

        SetHotItem(iButton);                 // could be none (-1)

        if (iState==TRACK_POPUP)
        {
            // set related state stuff
            m_bEscapeWasPressed  = FALSE;    // assume Esc key not pressed
            m_bProcessRightArrow = TRUE;     // assume left/right arrow..
            m_bProcessLeftArrow  = TRUE;     // ..will move to prev/next popup
            m_iPopupTracking     = iButton;  // which popup I'm tracking
            m_hMenuTracking      = NULL;     // YAMA:981219
        }

        m_iTrackingState = iState;
    }
}

//++030614
static VOID _GetMonitorRectFromRect(const RECT *prc,RECT *prcResult,ULONG Flags,BOOLEAN bWorkspace)
{
    HMONITOR hmon;
    hmon = MonitorFromRect(prc,Flags);

    MONITORINFO MonInfo = {0};
    MonInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hmon,&MonInfo);

    if( bWorkspace )
        *prcResult = MonInfo.rcWork;
    else
        *prcResult = MonInfo.rcMonitor;
}
//--030614

POINT CMenuBar::ComputeMenuTrackPoint(const RECT& rcButn, TPMPARAMS& tpm)
{
    tpm.cbSize = sizeof(tpm);
    POINT pt;
    RECT& rcExclude = (RECT&)tpm.rcExclude;
    rcExclude = rcButn;

//++030614
    _GetMonitorRectFromRect(&rcButn,&rcExclude,MONITOR_DEFAULTTONEAREST,FALSE);

    pt.x = rcButn.left;
    pt.y = rcButn.top + rcButn.bottom;
//--030614
    return pt;
}

POINT CMenuBar::AdjustComputeMenuTrackPoint(HMENU hMenuPopup,RECT& rcButton,POINT& pt,RECT& rcExclude,UINT& uFlags)
{
    int c = GetMenuItemCount(hMenuPopup);
    int i;
    int cyMenu = 0;
    int cxMenu = 0;
    TCHAR sz[256];
    BOOLEAN bInTab = FALSE;

    for(i = 0; i < c; i++)
    {
        MENUITEMINFO mii;
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask  = MIIM_TYPE | MIIM_STATE;// | MIIM_STRING;
        mii.cch = sizeof(sz);
        mii.dwTypeData = sz;
        GetMenuItemInfo(hMenuPopup,i,TRUE,&mii);

        if ( mii.fType & MFT_SEPARATOR)
        {
            cyMenu += GetSystemMetrics(SM_CYMENU)>>1;
        }
        else 
        {
//          extern HFONT GetMenuFont(VOID); // define in CIconMenu
            extern int CXGAP;               // num pixels between button and text
            extern int CXTEXTMARGIN;        // num pixels after hilite to start text
            extern SIZE m_sizeButton;

            // compute size of text: use DrawText with DT_CALCRECT
            HDC hdc;
            hdc = GetWindowDC(NULL);

            RECT rcText;
            SetRect(&rcText,0,0,0,0);

            HFONT OldFont = (HFONT)SelectObject(hdc,GetMenuFont());
            DrawText(hdc,sz,lstrlen(sz),&rcText,DT_SINGLELINE|DT_LEFT|DT_VCENTER|DT_CALCRECT);
            SelectObject(hdc,OldFont);

            // height of item is just height of a standard menu item
            int n;
            n = max( GetSystemMetrics(SM_CYMENU), (rcText.bottom-rcText.top) );
            n = max( (int)m_sizeButton.cy, (int)n );
            cyMenu += n;

            // width is width of text plus a bunch of stuff
            int cx = rcText.right - rcText.left;    // text width 
            cx += CXTEXTMARGIN<<1;          // L/R margin for readability
            cx += CXGAP;                    // space between button and menu text
            cx += m_sizeButton.cx << 1;     // button width (L=button; R=empty margin)

            if(_tcschr(sz,_T('\t'))!=NULL)
                bInTab = TRUE;

            if( cxMenu < cx )
                cxMenu = cx;

            ReleaseDC(NULL,hdc);
        }
    }

    // Q005--BUGBUG
    // What to do when top menu items extend beyond the window frame.
    // These overflowing menus can be opened with keyboard navigation.
    //
    //  EX)
    //  +------------+
    //  | File(F)  Edit(E) View(V) Tools(T) Help(H)
    //  +------------+ 
    //  |            | If you do not adjust it, the pop-up menu will open 
    //  |            | in a position that is not displayed due to overflowing menu items, 
    //  +------------+ so adjust the position to the right edge of the toolbar control.
    //  ^------------^ 
    //  Width of window freme
//++030614
    // If it extends beyond the left edge of the screen, adjust it to the left.
    //
    if( rcButton.left < rcExclude.left )
    {
        pt.x = rcExclude.left;
    }

    // The top and bottom menu frame borders are 4 pixels.
    //  +--- 2
    //  |
    //  +--- 2
    int cmy = 2 + 2;
    if( (pt.y + cyMenu + cmy) >= rcExclude.bottom )
    {
        pt.y = rcButton.top;
        uFlags &= TPM_TOPALIGN;
        uFlags |= TPM_BOTTOMALIGN;
    }

    // The top and bottom menu frame borders are 4 pixels.
    // 3         3
    // +---------+
    // |         |

    // whatever value I return in lpms->itemWidth, Windows will add the
    // width of a menu checkmark, so I must subtract to defeat Windows. Argh.
    //
    int cmx = 8;
    if( bInTab )
        cmx += GetSystemMetrics(SM_CXMENUCHECK)-1;
    if( rcExclude.right <= (pt.x + cxMenu + cmx) )
    {
        // If the right edge of the menu extends beyond the screen, 
        // always specify the right edge of the screen as the basis
        // for right-justified display of the menu.
        pt.x = rcExclude.right - 1; // In the Monitor API, the size is stored in rc.right, so we subtract right-1.
        uFlags &= ~TPM_LEFTALIGN;
        uFlags |= TPM_RIGHTALIGN;
    }
//--030614

    return pt;
}

LRESULT CALLBACK CMenuBar::MenuInputFilter(int code, WPARAM wp, LPARAM lp)
{
    if( code == MSGF_MENU && g_pMenuBar &&  g_pMenuBar->OnMenuInput(*((MSG*)lp)))
    {
        return TRUE;
    }
    return CallNextHookEx(g_hMsgHook, code, wp, lp);
}

BOOL CMenuBar::OnMenuInput(MSG& m)
{
    int msg = m.message;

    if (msg == WM_KEYDOWN )
    {
        // handle left/right-arow.
        //
        int vkey = (int)m.wParam;

        if ((vkey == VK_LEFT  && m_bProcessLeftArrow) ||
            (vkey == VK_RIGHT && m_bProcessRightArrow)) 
        {
//++040319
            int Next = GetNextOrPrevButton(m_iPopupTracking, vkey==VK_LEFT);
            if( !IsButtonVisible(NULL,Next) )
            {
                // If try to open a menu outside the rebar control's band, 
                // tracking will stop and the menu will close.
                CancelMenuAndTrackNewOne(m_iPopupTracking);//-1);
            }
            else
            {
//--040319
                CancelMenuAndTrackNewOne(Next);
            }
            return TRUE; // eat it

        }
        else if (vkey == VK_ESCAPE)
        {
            m_bEscapeWasPressed = TRUE; // menu will abort itself
        }
    }
    else if (msg == WM_MOUSEMOVE || msg == WM_LBUTTONDOWN)
    {
        // handle mouse move or click
        //
        POINT pt;
        pt.x = GET_X_LPARAM(m.lParam);
        pt.y = GET_Y_LPARAM(m.lParam);
        ScreenToClient(m_hWnd,&pt);

        if (msg == WM_MOUSEMOVE)
        {
            if( m_iPopupTracking != -1 ) // -1 is system menu Q006:030516
            {
                if ((pt.x != m_ptMouse.x) || (pt.y != m_ptMouse.y))
                {
                    int iButton = HitTest((LPPOINT)&pt);

                    if (IsValidButton(iButton) && iButton != m_iPopupTracking)
                    {
//++040319
                        if( !IsButtonVisible(0,iButton) )
                        {
                            // If try to open a menu outside the rebar control's band, 
                            // tracking will stop and the menu will close.
                            CancelMenuAndTrackNewOne(m_iPopupTracking);
                        }
                        else
                        {
                            // user moved mouse over a different button: track its popup
                            CancelMenuAndTrackNewOne(iButton);
                        }
//--040319
                    }

                    m_ptMouse = pt;
                }
            } // Q006:030516
        }
        else if (msg == WM_LBUTTONDOWN)
        {
            if (HitTest((LPPOINT)&pt) == m_iPopupTracking)
            {
                // user clicked on same button I am tracking: cancel menu
                CancelMenuAndTrackNewOne(-1);
                return TRUE; // eat it
            }
        }
    }
    else if( msg == WM_SYSKEYDOWN )
    {
        TCHAR vkey = (TCHAR)m.wParam;  // get virt key

        if (vkey == VK_MENU )
        {
            SetUnderLineMode(FALSE); // 010301 Clear Under Line
            m_fIgnoreAltUp = TRUE;
        }
    }
    else if( msg == WM_SYSKEYUP )
    {
        TCHAR vkey = (TCHAR)m.wParam;  // get virt key

        if (vkey == VK_MENU )
        {
            ASSERT(0);
        }
    }

    return FALSE; // not handled
}

void CMenuBar::CancelMenuAndTrackNewOne(int iNewPopup)
{
    if (iNewPopup != m_iPopupTracking)
    {
        PostMessage(_OwnerWindow(),WM_CANCELMODE,0,0);
        m_iNewPopup = iNewPopup;                     // go to this popup (-1 = quit)
    }
}

int CMenuBar::GetNextOrPrevButton(int iButton, BOOL bPrev)
{
    TBBUTTONINFO tb = {0};
    if (bPrev)
    {
        iButton--;
//++
        while( TRUE )
        {
            tb.cbSize = sizeof(tb);
            tb.dwMask = TBIF_STATE;
            SendMessage(m_hWnd,TB_GETBUTTONINFO,iButton,(LPARAM)&tb);
            if( !(tb.fsState & TBSTATE_HIDDEN) )
                break;
            iButton--;          
            if (iButton < 0)
                break;
        }
//--
        if (iButton < 0)
            iButton = GetButtonCount() - 1;
    }
    else
    {
        iButton++;
//++
        while( TRUE )
        {
            tb.cbSize = sizeof(tb);
            tb.dwMask = TBIF_STATE;
            SendMessage(m_hWnd,TB_GETBUTTONINFO,iButton,(LPARAM)&tb);
            if( !(tb.fsState & TBSTATE_HIDDEN) )
                break;
            iButton++;          
            if (iButton >= GetButtonCount())
                break;
        }
//--
        if (iButton >= GetButtonCount())
            iButton = 0;
    }

    return iButton;
}

void CMenuBar::ToggleTrackButtonMode()
{
    if (m_iTrackingState == TRACK_NONE || m_iTrackingState == TRACK_BUTTON)
    {
        int iToggleStart = 0;       // XP 011019
        if( IsTopSysMenu(m_hMenu) ) // XP 011019
            iToggleStart++;         // XP 011019

        SetTrackingState(m_iTrackingState == TRACK_NONE ? TRACK_BUTTON : TRACK_NONE, iToggleStart); // 990320 MDI1.0

//      SetTrackingState(m_iTrackingState == TRACK_NONE ? TRACK_BUTTON : TRACK_NONE,     //  990320 MDI1.0 
//                       m_bMDISysMenuInMenuBar ? 1 : 0);                                //  990320 MDI1.0

        SetUnderLineMode(m_iTrackingState == TRACK_NONE ? FALSE : TRUE); // 010309
    }
}

LRESULT CMenuBar::OnLButtonDown(WPARAM nFlags, LPARAM lParam) 
{
    POINT point;

    point.x = GET_X_LPARAM(lParam); // BUGBUG:030614 
    point.y = GET_Y_LPARAM(lParam); // BUGBUG:030614 

    int iButton = HitTest(&point);

    if (iButton >= 0 && iButton < GetButtonCount()) // if mouse is over a button:
    {
        TrackPopup(iButton,FALSE); //   track it
        return 1;
    }

    return 0;
}

LRESULT CMenuBar::OnMouseMove(WPARAM nFlags,LPARAM lParam)
{
    POINT point;

    point.x = GET_X_LPARAM(lParam); // BUGBUG:030614 
    point.y = GET_Y_LPARAM(lParam); // BUGBUG:030614 

    if (m_iTrackingState==TRACK_BUTTON)
    {
        // In button-tracking state, ignore mouse-over to non-button area.
        // Normally, the toolbar would de-select the hot item in this case.
        // 
        // Only change the hot item if the mouse has actually moved.
        // This is necessary to avoid a bug where the user moves to a different
        // button from the one the mouse is over, and presses arrow-down to get
        // the menu, then Esc to cancel it. Without this code, the button will
        // jump to wherever the mouse is--not right.

        int iHot = HitTest(&point);
        if (IsValidButton(iHot) && ((point.x != m_ptMouse.x) || (point.y != m_ptMouse.y)) )
            SetHotItem(iHot);
        return 1;      // don't let toolbar get it
    }

    m_ptMouse = point; // remember point

    return 0;
}

void CMenuBar::RecomputeMenuLayout()
{
    SetWindowPos(m_hWnd,NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE |
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

#if 0
LRESULT CMenuBar::OnInitMenuPopup(HMENU hMenu,UINT uPos,BOOL bSysMenu)
{
    return 0;
}

VOID CMenuBar::ActiveSystemMenu(HMENU hMenu,BOOL bActive)
{
    if( bActive )
    {
        m_bSysMenu = TRUE; // Enter System-menu Mode

        SetTrackingState(TRACK_POPUP);

        if( g_pMenuBar == NULL )
        {
            g_pMenuBar = this;
            g_hMsgHook = SetWindowsHookEx(WH_MSGFILTER,MenuInputFilter,NULL,::GetCurrentThreadId());
        }
    }
}
#endif

LRESULT CMenuBar::OnMenuSelect(HMENU hmenu, UINT iItem, UINT flags)
{
    if (m_iTrackingState > 0)
    {
        if ( m_hMenuTracking == NULL )
        {
            m_hMenuTracking = hmenu;
        }

        // process right-arrow iff item is NOT a submenu
        m_bProcessRightArrow = (::GetSubMenu(hmenu, iItem) == NULL);
        // process left-arrow iff curent menu is one I'm tracking
        m_bProcessLeftArrow = (hmenu == m_hMenuTracking);
#if 0
        if( flags == 0xffff && m_bSysMenu )
        {
            if( g_pMenuBar != NULL )
            {
                ::UnhookWindowsHookEx(g_hMsgHook);
                g_pMenuBar = NULL;
                g_hMsgHook = NULL;
            }

            // menu track mode to off
            //
            SetTrackingState(TRACK_NONE);

            // When the MDI system menu is displayed in the menu 
            // (i.e. when an MDI child window is maximized), 
            // the MDI system menu is discarded and then reloaded.
            if( HasMDIMenu() )
            {
                EnableMDISysMenuItem(FALSE);
                EnableMDISysMenuItem(TRUE);
            }

            m_bSysMenu = FALSE; // Leave System Menu mode
        }
#endif
    }

    return 0;
}

VOID CMenuBar::OnActivate(BOOL bActive)
{
    m_bActive = bActive;
    if( IsWindow(m_hWnd) )
    {
        // Reset mode
        SetTrackingState(TRACK_NONE);
        SetUnderLineMode(FALSE);

        InvalidateRect(m_hWnd,NULL,FALSE);
        UpdateWindow(m_hWnd);
    }
}

LRESULT CMenuBar::OnCustomDraw( NMTBCUSTOMDRAW * pnmhdr )
{
    LPNMTBCUSTOMDRAW lpNMCustomDraw = (LPNMTBCUSTOMDRAW) pnmhdr;

    // Grays out menu text when the application that owns the menu is in the background.
    if( !m_bActive )
    {
        if( m_bMDISysMenuInMenuBar && lpNMCustomDraw->nmcd.dwItemSpec == 0)
        {
            ;
        }
        else
        {
            if( lpNMCustomDraw->nmcd.dwDrawStage == CDDS_PREPAINT)
            {
                return CDRF_NOTIFYITEMDRAW;
            }
            if( lpNMCustomDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
            {
//              extern HFONT GetMenuFont(VOID); // define in CIconMenu
                SelectObject(lpNMCustomDraw->nmcd.hdc,GetMenuFont()); // 010903
                lpNMCustomDraw->clrText = GetSysColor(COLOR_3DSHADOW);
                SetTextColor(lpNMCustomDraw->nmcd.hdc,lpNMCustomDraw->clrText);
                return CDRF_NEWFONT;
            }
        }
    }

    //
    // Draws the MDI child window's icon in the main menu.
    //
    if(lpNMCustomDraw->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
        return CDRF_NOTIFYITEMDRAW;
    }

    if(lpNMCustomDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
    {
        if( m_bMDISysMenuInMenuBar )
        {
            if(lpNMCustomDraw->nmcd.dwItemSpec != 0)
            {
                return CDRF_DODEFAULT;
            }

            int cyIcon;
            int cxIcon;
            ImageList_GetIconSize(m_hMenuImages,&cxIcon,&cyIcon);
            ImageList_Draw(m_hMenuImages,0,lpNMCustomDraw->nmcd.hdc,
                            lpNMCustomDraw->nmcd.rc.left,
                            lpNMCustomDraw->nmcd.rc.top 
                            + (((lpNMCustomDraw->nmcd.rc.bottom - lpNMCustomDraw->nmcd.rc.top) - cyIcon) / 2),
                            ILD_TRANSPARENT );

            return CDRF_SKIPDEFAULT;
        }
    }

    return 0;
}

int CMenuBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    m_hMenuImages = ImageList_Create(16,16,ILC_MASK|ILC_COLORDDB,1,1); // 031224
    UpdateFont();
    return 0;
}

BOOL CMenuBar::ChangeLayout()
{
    UpdateFont();
    RecomputeMenuLayout();
    SendMessage(m_hWnd,WM_WININICHANGE,0,0);
    return TRUE;
}

BOOL IsTopSysMenu(HMENU hMenu)
{
    HMENU hSub = ::GetSubMenu(hMenu,0);
    UINT  id;

    for(int i = 0; i < GetMenuItemCount(hSub); i++)
    {
        id = GetMenuItemID(hSub,i);
        if( id != 0 && id != 0xFFFF && id != 0xFFFFFFFF )
        {
            if( id >= 0xF000 )
                return TRUE;
            else
                return FALSE;
        }
    }
    return FALSE;
}

HMENU CopyMenu(HMENU hDst,HMENU hSrc,BOOL bSetStyle)
{
    int i;
    int c;
    MENUITEMINFO_NT5 mii;
    TCHAR sz[100];
    HMENU hNew;

    if( hDst == NULL )
        hNew = CreatePopupMenu();
    else
        hNew = hDst;

    if( bSetStyle )
        _SetSystemMenuStyle(hNew);

    c = GetMenuItemCount(hSrc);

    for(i = 0; i < c; i++)
    {
        memset(&mii,0,sizeof(mii));
/*++    if( _StdLib.m_MajorVersion == 4 )
        {
            if( IsWinNT() )
            {
                // WinNT
                mii.cbSize = sizeof(MENUITEMINFO_NT4);
                mii.fMask  = MIIM_FTYPE|MIIM_BITMAP|MIIM_ID|MIIM_STATE|MIIM_SUBMENU|MIIM_STRING|MIIM_CHECKMARKS|MIIM_TYPE;
                mii.fType  = MFT_STRING;
                mii.cch    = sizeof(sz);
                mii.dwTypeData = sz;
                GetMenuItemInfo(hSrc,i,TRUE,(MENUITEMINFO*)&mii);
            }
            else
            {
                // Win98
                mii.cbSize = sizeof(MENUITEMINFO_NT4);
                mii.fMask  = MIIM_FTYPE|MIIM_BITMAP|MIIM_ID|MIIM_STATE|MIIM_SUBMENU|MIIM_STRING|MIIM_CHECKMARKS;
                mii.fType  = MFT_STRING|MFT_BITMAP;
                mii.cch    = sizeof(sz);
                mii.dwTypeData = sz;
                GetMenuItemInfo(hSrc,i,TRUE,(MENUITEMINFO*)&mii);
            }
        }
        else --*/
        {
            mii.cbSize = sizeof(MENUITEMINFO_NT5);
            mii.fMask  = MIIM_FTYPE|MIIM_BITMAP|MIIM_ID|MIIM_STATE|MIIM_SUBMENU|MIIM_STRING|MIIM_CHECKMARKS;
            mii.cch = sizeof(sz);
            mii.dwTypeData = sz;
            GetMenuItemInfo(hSrc,i,TRUE,(MENUITEMINFO*)&mii);
        }

        if( mii.hSubMenu != NULL )
        {
            HMENU h;
            h = CreatePopupMenu();
            CopyMenu(h,mii.hSubMenu,bSetStyle);
            mii.hSubMenu = h;
        }

        if( !InsertMenuItem(hNew,i,TRUE,(MENUITEMINFO*)&mii) )
        {
            ::DestroyMenu(hNew);
            hNew = NULL;
            break;
        }
    }
    return hNew;
}

void CMenuBar::EnableMDISysMenuItem(BOOL bEnable)
{
    TBBUTTON tbb;

    if( bEnable )
    {
        TRACE("CM: Insert MDI System menu in Main manu\n");

        if( IsTopSysMenu(m_hMenu) )
        {
            return;
        }

        //
        // Append MDI System Menu Button
        //
        tbb.iString   = 0;
        tbb.fsState   = TBSTATE_ENABLED;
        tbb.fsStyle   = TBSTYLE_BUTTON;//|TBSTYLE_AUTOSIZE; OUT:040119
        tbb.iBitmap   = 0;
        tbb.idCommand = 1024;
        SendMessage(m_hWnd,TB_INSERTBUTTON,0,(LPARAM)&tbb);

        //
        // Change Button size to MDI sub-menu.
        //
        TBBUTTONINFO tbs;
        ZeroMemory(&tbs,sizeof(TBBUTTONINFO));
        tbs.cbSize  = sizeof(TBBUTTONINFO);
        tbs.dwMask  = TBIF_TEXT;              // 990623
        tbs.pszText = _T("");                 // 990623
        tbs.cchText = 0;                      // 990623
        SendMessage(m_hWnd,TB_SETBUTTONINFO,1024,(LPARAM)&tbs);

        // Hack!-
        // When I call SetButtonInfo with TBIF_TEXT|TBIF_SIZE, 
        // tbs.cx is ignored for some reason?  
        // I have no choice but to call it twice.
        ZeroMemory(&tbs,sizeof(TBBUTTONINFO));
        tbs.cbSize  = sizeof(TBBUTTONINFO);
        tbs.dwMask  = TBIF_SIZE;
        tbs.cx      = 16; // todo: icon size
        SendMessage(m_hWnd,TB_SETBUTTONINFO,1024,(LPARAM)&tbs);

        //
        // Insert MDI sub-menu into default menu and re-align
        //
        MENUITEMINFO menuinfo;

        ASSERT(m_hwndMDIActive != NULL);

        HMENU hMenu = GetSystemMenu(m_hwndMDIActive,FALSE);

        ASSERT(hMenu != NULL);

        menuinfo.cbSize     = sizeof(MENUITEMINFO);
        menuinfo.fMask      = MIIM_TYPE|MIIM_SUBMENU;
        menuinfo.fType      = MFT_STRING;
        menuinfo.hSubMenu   = hMenu;
        menuinfo.dwTypeData = _T("");
        menuinfo.cch        = 0;
        InsertMenuItem(m_hMenu,0,TRUE,&menuinfo);

        //
        // The Toolbar IDs are assigned sequentially starting from 0,
        // so renumbering them including the MDI menu.
        //
        int i,c = GetButtonCount();

        for(i = 0; i < c; i++)
        {
            SendMessage(m_hWnd,TB_SETCMDID,i,i);
        }

        UpdateIcon();

        m_bMDISysMenuInMenuBar = TRUE;
    }
    else
    {
        if(  bEnable == FALSE && m_bMDISysMenuInMenuBar )
        {
            if( !IsTopSysMenu(m_hMenu) )
            {
                return;
            }

            // Delete the button that has the MDI menu assigned to it.
            SendMessage(m_hWnd,TB_DELETEBUTTON,0,0);

            // Delete MDI menu.
            RemoveMenu(m_hMenu,0,MF_BYPOSITION);

            // The MDI menu has been deleted, so the CmdIDs will be reassigned.
            int i,c = GetButtonCount();

            for(i = 0; i < c; i++)
            {
                SendMessage(m_hWnd,TB_SETCMDID,i,i);
            }

            // Remove MDI child icon from imagelist.
            ImageList_Remove(m_hMenuImages,-1);

            m_bMDISysMenuInMenuBar = FALSE;
        }
    }
}

void CMenuBar::UpdateIcon()
{
    ASSERT(m_hwndMDIActive);
    BOOL bLoadFromFile = FALSE;

    if( m_hwndMDIActive == NULL  )
        return ;

    ImageList_Remove(m_hMenuImages,-1);

#define ICON_SMALL2        2
    HICON hIcon;
//  if( _StdLib.m_MajorVersion >= 5 && _StdLib.m_MinorVersion >= 1 )
        hIcon = (HICON)SendMessage(m_hwndMDIActive,WM_GETICON,ICON_SMALL2,0);
//  else
//      hIcon = (HICON)SendMessage(m_hwndMDIActive,WM_GETICON,ICON_SMALL,0);
    if( hIcon == NULL )
    {
        hIcon = (HICON)GetClassLongPtr(m_hwndMDIActive,GCLP_HICON);
        if( hIcon == NULL )
        {
            hIcon = (HICON)LoadImage(NULL,(TCHAR *)105,IMAGE_ICON,16,16,LR_DEFAULTCOLOR);           
            bLoadFromFile = TRUE;
        }
    }

    if( hIcon )
    {
        ImageList_AddIcon(m_hMenuImages,hIcon);
        if( bLoadFromFile ) // delete on load from file or resource.
            DestroyIcon(hIcon);
    }
}

BOOL CMenuBar::ReplaceMDISysMenu( HWND hwndMDIChild )
{
    // 
    // If there are no MDI menu items in the main menu, no processing occurs.
    //
    if( !m_bMDISysMenuInMenuBar )
        return FALSE;

    //
    // Delete existing MDI child system menu item
    //
    if( !RemoveMenu(m_hMenu,0,MF_BYPOSITION) )
    {
        ASSERT( FALSE );
        return FALSE;
    }

    //
    // Insert new MDI child system menu item
    //
    MENUITEMINFO menuinfo;

    HMENU hMenu = GetSystemMenu(hwndMDIChild,FALSE);

    ASSERT(hMenu != NULL);

    menuinfo.cbSize     = sizeof(MENUITEMINFO);
    menuinfo.fMask      = MIIM_TYPE|MIIM_SUBMENU;
    menuinfo.fType      = MFT_STRING;
    menuinfo.hSubMenu   = hMenu;
    menuinfo.dwTypeData = _T("");
    menuinfo.cch        = 0;

    if( !InsertMenuItem(m_hMenu,0,TRUE,&menuinfo) )
    {
        ASSERT( FALSE );
        return FALSE;
    }

    return TRUE;
}

void CMenuBar::OnNmGetDispInfo( NMHDR * pnmhdr, LRESULT* result )
{
    NMTBDISPINFO *pDispInfo = (NMTBDISPINFO *)pnmhdr;

    if( pDispInfo->dwMask & TBNF_IMAGE )
    {
        pDispInfo->iImage = -1;
    }

    *result = 0;
}

//
// Menu Accelerator Text 
//

BOOL CMenuBar::IsUnderLineMode()
{
    BOOL b;
    SystemParametersInfo(SPI_GETMENUUNDERLINES,0,&b,0);
    return ((b != 0) ? TRUE  : FALSE);
}

void CMenuBar::SetUnderLineMode(BOOL b)
{
    if( IsUnderLineMode() )
        return ;

    DWORD dwFlags = DT_SINGLELINE|DT_LEFT|DT_VCENTER|DT_HIDEPREFIX;
    DWORD dwMasks = 0;
    if( b )
        dwMasks = dwFlags & ~DT_HIDEPREFIX;
    else
        dwMasks = dwFlags;

    if( m_dwDrawFlags != dwMasks )
    {
        SendMessage(m_hWnd,TB_SETDRAWTEXTFLAGS,dwMasks,dwFlags);
        InvalidateRect(m_hWnd,NULL,TRUE);
        UpdateWindow(m_hWnd);                    // and force repaint now
        m_dwDrawFlags = dwMasks;
    }
}

//++040319
BOOL CMenuBar::IsButtonVisible(POINT* pt,int iButton)
{
    NMMBINFO nmmb;
    int iRet;

    nmmb.hdr.hwndFrom = m_hWnd;
    nmmb.hdr.idFrom   = GetWindowLongPtr(m_hWnd,GWLP_ID);
    nmmb.hdr.code     = MBN_CHECKBUTTONVISIBLE;

    nmmb.Index = iButton;
    if( pt )
        nmmb.Point = *pt;
    else
        nmmb.Point.x = nmmb.Point.y = 0;    

    iRet = (int)SendMessage(_OwnerWindow(),WM_NOTIFY,nmmb.hdr.idFrom,(LPARAM)&nmmb);

    return (iRet == 0) ? TRUE : FALSE;
}
//--040319

//++080125
void CMenuBar::RestructButtons()
{
    TBBUTTON tb;
    int i,c;
    int iDel;

    c = GetButtonCount();

    if( m_bMDISysMenuInMenuBar )
    {
        iDel = 1;
        c--;
    }
    else
    {
        iDel = 0;
    }

    for(i = 0; i < c; i++)
    {
        SendMessage(m_hWnd,TB_GETBUTTON,iDel,(LPARAM)&tb);
        SendMessage(m_hWnd,TB_DELETEBUTTON,iDel,(LPARAM)&tb);
        tb.fsStyle = TBSTYLE_AUTOSIZE;
        SendMessage(m_hWnd,TB_ADDBUTTONS,1,(LPARAM)&tb);
    }
}
//--080125
