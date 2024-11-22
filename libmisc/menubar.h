#include "toolbarctrl.h"
#include "simplevalarray.h"
#include "strbuf.h"

#ifndef SPI_GETMENUUNDERLINES
#define SPI_GETMENUUNDERLINES  0x100A
#endif

#ifndef DT_HIDEPREFIX
#define DT_HIDEPREFIX               0x00100000
#define DT_PREFIXONLY               0x00200000
#endif

typedef struct _STRINGID
{
    CStringBuffer buf;
    int iString;
} STRINGID,*PSTRINGID;

typedef struct _NMMBINFO
{
    NMHDR hdr;
    POINT Point;
    int Index;
} NMMBINFO; 

#define MBN_CHECKBUTTONVISIBLE (NM_FIRST+1)

class CMenuBar : public CToolBarCtrl
{
    static CMenuBar *m_pThis;
    static LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    LRESULT WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    VOID SubclassToolBar();

    CValArray<STRINGID*> m_StringID;

	HFONT m_hFontIcon;

protected:
    enum TRACKINGSTATE
    {
        // menubar has three states:
        TRACK_NONE = 0,   // * normal, not tracking anything
        TRACK_BUTTON,     // * tracking buttons (F10/Alt mode)
        TRACK_POPUP       // * tracking popups
    };

    HMENU    m_hMenu;
    BOOL     m_bEscapeWasPressed;
    int      m_iPopupTracking;          // which popup I'm tracking if any
    int      m_iNewPopup;               // next menu to track
    BOOL     m_bProcessRightArrow;      // process l/r arrow keys?
    BOOL     m_bProcessLeftArrow;       // ...
    BOOL     m_bAutoRemoveFrameMenu;
    POINT    m_ptMouse;                 // mouse location when tracking popup
    HMENU    m_hMenuTracking;           // current popup I'm tracking
    DWORD    m_dwStyle;
    BOOL     m_bMDISysMenuInMenuBar;    // MDI Child window system menu button.
    TRACKINGSTATE m_iTrackingState;     // current tracking state
    int      m_fDoubleAltUp;            // 990322
    int      m_iOpenMenuIndex;          //
    BOOL     m_bActive;
    DWORD    m_dwDrawFlags;             // 010301
    BOOL     m_fIgnoreAltUp;
    int      m_nPopupLoop;              // 011201
    BOOL     m_bSysMenu;
public:
    CMenuBar();
    ~CMenuBar();
    HWND    Create(HWND hwndOwner,HINSTANCE hInst,int nID,DWORD dwStyle);
    HMENU   LoadMenu(LPCTSTR pszMenuName);
    HMENU   LoadMenu(HMENU hMenu);
    BOOL    TranslateFrameMessage(MSG* pMsg);
    LRESULT OnMenuSelect(HMENU hmenu, UINT iItem, UINT flags);
    LRESULT OnInitMenuPopup(HMENU hMenu,UINT uPos,BOOL bSysMenu);
    VOID    OnActivate(BOOL bActive);
    LRESULT OnCustomDraw(NMTBCUSTOMDRAW *pnmhdr);
    void    RecomputeMenuLayout();
    void    UpdateFont();
    BOOL    ChangeLayout();
    BOOL    DestroyMenu();
    BOOL    IsUnderLineMode();
    void    SetUnderLineMode(BOOL b);

public:
    inline int GetOpenMenu()
    {
        return m_iOpenMenuIndex;
    }

    inline HMENU GetMenu()
    {
        return m_hMenu;
    }

    inline BOOL IsAltKeyTrack()
    {
        return (m_dwDrawFlags & DT_HIDEPREFIX) == DT_HIDEPREFIX ? FALSE : TRUE;
    }

    inline TRACKINGSTATE GetTrackState()
    {
        return m_iTrackingState;
    }

    inline int GetPopupCount()
    {
        return m_nPopupLoop;
    }

protected:
    void    TrackPopup(int iButton,BOOL bSelectItem);
    void    SetTrackingState(TRACKINGSTATE iState, int iButton = -1);
    POINT   ComputeMenuTrackPoint(const RECT& rcButn, TPMPARAMS& tpm);
    POINT   AdjustComputeMenuTrackPoint(HMENU hMenuPopup,RECT& rcButton,POINT& pt,RECT& rcExclude,UINT& uFlags);
    BOOL    OnMenuInput(MSG& m);
    int     GetNextOrPrevButton(int iButton, BOOL bPrev);
    void    CancelMenuAndTrackNewOne(int iNewPopup);
    void    ToggleTrackButtonMode();
    void    RecomputeToolbarSize();
    BOOL    IsValidButton(int iButton)
    { return 0 <= iButton && iButton < GetButtonCount(); }
    static LRESULT CALLBACK MenuInputFilter(int code, WPARAM wp, LPARAM lp);
    HMENU   InternalLoadMenu(HMENU hmenu);
    BOOL    IsButtonVisible(POINT* pt,int iButton);

protected:
    // Message Hook handler
    LRESULT OnLButtonDown(WPARAM nFlags,LPARAM lParam);
    LRESULT OnMouseMove(WPARAM nFlags,LPARAM lParam);

protected:
    // for MDI 
    HIMAGELIST m_hMenuImages;
    int  m_cxMDIMenu;
    HWND m_hwndMDIActive;
public:
    int  OnCreate(LPCREATESTRUCT lpCreateStruct); 
    void OnNmGetDispInfo( NMHDR * pnmhdr, LRESULT* result );
    void EnableMDISysMenuItem(BOOL bEnable);
    void SetMDIActiveWindow(HWND hWnd) {  m_hwndMDIActive = hWnd; }
    HWND GetMDIActiveWindow(HWND hWnd) {  return m_hwndMDIActive; }
    void UpdateIcon();
    BOOL ReplaceMDISysMenu(  HWND hwndMDIChild );
    BOOL HasMDIMenu() { return m_bMDISysMenuInMenuBar; }
    VOID ActiveSystemMenu(HMENU hMenu,BOOL bSysMenu);
    void RestructButtons();

private:
    HFONT GetMenuFont(VOID)
    {
        if( m_hFontIcon == NULL )
        {
            UpdateMenuFont();
        }
        return m_hFontIcon;
    }
    HFONT UpdateMenuFont(VOID)
    {
        if( m_hFontIcon != NULL )
        {
            DeleteObject(m_hFontIcon);
            m_hFontIcon = NULL;
        }
        LOGFONT lf;
        SystemParametersInfo(SPI_GETICONTITLELOGFONT,sizeof(LOGFONT),&lf,0);
        m_hFontIcon = CreateFontIndirect(&lf);
        return m_hFontIcon;
    }
};

/* Menu help API */
HMENU CopyMenu(HMENU hDst,HMENU hSrc,BOOL bSetStyle);
BOOL IsTopSysMenu(HMENU hMenu);
