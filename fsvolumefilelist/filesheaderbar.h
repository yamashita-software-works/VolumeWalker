#pragma once
//***************************************************************************
//*                                                                         *
//*  filesheaderbar.h                                                       *
//*                                                                         *
//*  File List Header Bar                                                   *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  History: 2024-05-01 created                                            *
//*           2024-08-23 modified                                           *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "longpathbox.h"
#include "resource.h"

#define ID_PATHBOX 100

#define PM_CHANGEPATH             (WM_APP+5000)

typedef struct _HEADERBARCOLOR
{
	COLORREF crHighlightBack;
	COLORREF crHighlightText;
	COLORREF crNormalBack;
	COLORREF crNormalText;
	COLORREF crBoxBack;
	COLORREF crBoxBorder;
	COLORREF crBoxInactiveBack;
	COLORREF crBoxInactiveBorder;
} HEADERBARCOLOR;

#define HBM_COLOR (PM_USERBASE+0)

#define HBC_INACTIVE   (0)
#define HBC_ACTIVE     (1)
#define HBC_SETCOLOR   (2)
#define HBC_GETCOLOR   (3)

typedef struct _HEADERBARITEM
{
	UINT Flags;
	PWSTR NtDeviceName;
	PWSTR Drive;
} HEADERBARITEM;

#define HDIF_DRIVE_ITEM  0x1

class CHeaderBar : public CBaseWindow
{
	HFONT m_hFont;
	LONG m_cyTextHeight;

	COLORREF m_crBack;
	COLORREF m_crText;

	COLORREF m_crHighlightBack;
	COLORREF m_crHighlightText;
	COLORREF m_crNormalBack;
	COLORREF m_crNormalText;

	COLORREF m_crBoxBack;
	COLORREF m_crBoxBorder;
	COLORREF m_crBoxInactiveBack;
	COLORREF m_crBoxInactiveBorder;

	HWND m_hWndToolbar;
	HWND m_hWndMenubar;
	HWND m_hWndVolumeBox;
	HWND m_hWndPathBox;

	DWORD m_dwStyleFlags;

	HIMAGELIST m_himl;

public:
	CHeaderBar()
	{
		m_hFont = NULL;
		m_cyTextHeight = 0;
		m_crBack = 0;
		m_crText = 0;
		m_hWndToolbar = NULL;
		m_hWndMenubar = NULL;
		m_hWndPathBox = NULL;
		m_crHighlightBack = m_crNormalBack = GetSysColor(COLOR_3DFACE);
		m_crHighlightText = m_crNormalText = GetSysColor(COLOR_WINDOWTEXT);
		m_crBoxBack = m_crHighlightBack;
		m_crBoxBorder = m_crHighlightBack;
		m_crBoxInactiveBack = m_crBoxBack;
		m_crBoxInactiveBorder = m_crBoxInactiveBack;
		m_dwStyleFlags = 0;
		m_himl = NULL;
	}

	~CHeaderBar()
	{
		if( m_hFont )
			DeleteObject(m_hFont);
	}

	DWORD GetStyle() const
	{
		return m_dwStyleFlags;
	}

	VOID SetStyle(DWORD dwStyle)
	{
		m_dwStyleFlags = dwStyle;
	}

	HRESULT SetPath(PCWSTR pszPath)
	{
		PWSTR pszRoot;

		FindRootDirectory_W(pszPath,&pszRoot);
		
		SetWindowText(m_hWndPathBox,pszRoot);
#if 0
		WCHAR szText[MAX_PATH];
		WCHAR szDevice[MAX_PATH];
		WCHAR szDosDrive[8];
		PWSTR pszDeviceName;
		
		memset(szDevice,0,sizeof(szDevice));
		
		NtPathParseDeviceName(pszPath,szDevice,ARRAYSIZE(szDevice),NULL,0);
		
		pszDeviceName = wcsrchr(szDevice,L'\\');
		if( pszDeviceName )
			pszDeviceName++;
		else
			pszDeviceName = szDevice;
		
		if( NtPathToDosPath(szDevice,szDosDrive,ARRAYSIZE(szDosDrive)) )
			StringCchPrintf(szText,ARRAYSIZE(szText),L"%s (%s)",pszDeviceName,szDosDrive);
		else
			StringCchPrintf(szText,ARRAYSIZE(szText),L"%s",pszDeviceName);

		SetWindowText(m_hWndVolumeBox,szText);
#else
		WCHAR szVolumeName[MAX_PATH];
		ZeroMemory(szVolumeName,sizeof(szVolumeName));

		if( NtPathGetVolumeName(pszPath,szVolumeName,ARRAYSIZE(szVolumeName)) )
		{
			if( HasPrefix(L"\\??\\",szVolumeName) )
			{
				if( szVolumeName[4] != L'\0' && szVolumeName[5] == L':' )
					szVolumeName[4] = towupper(szVolumeName[4]);
			}
		}

		SetWindowText(m_hWndVolumeBox,szVolumeName);
#endif
		
		
		return S_OK;
	}

	HRESULT SetText(PCWSTR pszTitle1,PCWSTR pszTitle2)
	{
		SetWindowText(m_hWndVolumeBox,pszTitle1);
		SetWindowText(m_hWndPathBox,pszTitle2);
		return S_OK;
	}

	void
	SetColors(
		COLORREF crNormalBack,
		COLORREF crNormalText,
		COLORREF crHighlightBack,
		COLORREF crHighlightText
		)
	{
		m_crHighlightBack = crHighlightBack;
		m_crHighlightText = crHighlightText;
		m_crNormalBack = crNormalBack;
		m_crNormalText = crNormalText;
	}

	void
	GetColors(
		COLORREF& crNormalBack,
		COLORREF& crNormalText,
		COLORREF& crHighlightBack,
		COLORREF& crHighlightText
		)
	{
		crHighlightBack = m_crHighlightBack;
		crHighlightText = m_crHighlightText;
		crNormalBack = m_crNormalBack;
		crNormalText = m_crNormalText;
	}

	void
	SetBoxColors(
		COLORREF crBoxBack,
		COLORREF crBoxBorder,
		COLORREF crBoxInactiveBack,
		COLORREF crBoxInactiveBorder
		)
	{
		m_crBoxBack = crBoxBack;
		m_crBoxBorder = crBoxBorder;
		m_crBoxInactiveBack = crBoxInactiveBack;
		m_crBoxInactiveBorder = crBoxInactiveBorder;
	}

	void
	GetBoxColors(
		COLORREF& crBoxBack,
		COLORREF& crBoxBorder,
		COLORREF& crBoxInactiveBack,
		COLORREF& crBoxInactiveBorder
		)
	{
		crBoxBack = m_crBoxBack;
		crBoxBorder = m_crBoxBorder;
		crBoxInactiveBack = m_crBoxInactiveBack;
		crBoxInactiveBorder = m_crBoxInactiveBorder;
	}

	void EnableButton(UINT id,BOOL bEnable)
	{
		DWORD state = (DWORD)SendMessage(m_hWndToolbar,TB_GETSTATE,(WPARAM)id,0);
		state &= ~TBSTATE_ENABLED;
		state |= (bEnable ? TBSTATE_ENABLED : 0);
		SendMessage(m_hWndToolbar,TB_SETSTATE,(WPARAM)id,(LPARAM)state);
	}

	void InitFont()
	{
		if( m_hFont )
		{
			DeleteObject(m_hFont);
			m_hFont = NULL;
		}

		LOGFONT lf;
		SystemParametersInfo(SPI_GETICONTITLELOGFONT,sizeof(LOGFONT),&lf,0);
		m_cyTextHeight = abs( lf.lfHeight );
		m_hFont = CreateFontIndirect(&lf);
	}

	int GetHeight()
	{
		DWORD dw = (DWORD)SendMessage(m_hWndToolbar,TB_GETBUTTONSIZE,0,0);
		return (HIWORD(dw) + DPI_SIZE_CY(4)) * 2;
	}

	int GetToolbarButtonHeight()
	{
		DWORD dw = (DWORD)SendMessage(m_hWndToolbar,TB_GETBUTTONSIZE,0,0);
		int cy = (int)HIWORD(dw);
		return cy;
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		InitFont();

		m_hWndToolbar = CreateToolbarButtons(hWnd,0);
		m_hWndMenubar = CreateToolbarButtons(hWnd,1);

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() ) {
			SetWindowTheme(m_hWndToolbar,L"DarkMode",NULL);
			SetWindowTheme(m_hWndMenubar,L"DarkMode",NULL);
		}
#endif
		m_himl = ImageList_Create(
							GetSystemMetrics(SM_CXSMICON),
							GetSystemMetrics(SM_CYSMICON),
							ILC_COLOR32 | ILC_MASK, // ensures transparent background.
							8, 0);

		SHSTOCKICONINFO sii = {0};
		sii.cbSize = sizeof(sii);

		SHGetStockIconInfo(SIID_DRIVEFIXED,SHGSI_ICON|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
		ImageList_AddIcon(m_himl,sii.hIcon);

		SHGetStockIconInfo(SIID_DRIVEREMOVE,SHGSI_ICON|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
		ImageList_AddIcon(m_himl,sii.hIcon);

		m_hWndVolumeBox = CreateWindowEx(0,LPBC_LONGPATHBOX_NAME,L"",
							WS_VISIBLE|WS_CHILD|LPBS_FLAT_BORDER|LPBS_NO_TEXTSELECTION|LPBS_NO_FOCUS,
							0,0,0,0,m_hWnd,(HMENU)ID_PATHBOX,_GetResourceInstance(),0);

		m_hWndPathBox = CreateWindowEx(0,LPBC_LONGPATHBOX_NAME,L"",
							WS_VISIBLE|WS_CHILD|LPBS_FLAT_BORDER|LPBS_NO_TEXTSELECTION|LPBS_NO_FOCUS,
							0,0,0,0,m_hWnd,(HMENU)ID_PATHBOX,_GetResourceInstance(),0);

		SendMessage(m_hWndVolumeBox,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(m_hWndPathBox,WM_SETFONT,(WPARAM)m_hFont,0);

		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HIMAGELIST himl;
		himl = (HIMAGELIST)SendMessage(m_hWndToolbar,TB_GETDISABLEDIMAGELIST, 0,0);
		ImageList_Destroy(himl);

		himl = (HIMAGELIST)SendMessage(m_hWndToolbar,TB_GETIMAGELIST, 0, 0);
		ImageList_Destroy(himl);

		return 0;
	}

	LRESULT OnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HDC hdcMem;
		HBITMAP hBmp,hOrgBmp;
		PAINTSTRUCT ps;
		RECT rc;

		GetClientRect(hWnd,&rc);

		HDC hdc = BeginPaint(hWnd,&ps);

		hdcMem = CreateCompatibleDC(hdc);
		hBmp = CreateCompatibleBitmap(hdc,rc.right-rc.left,rc.bottom-rc.top);

		hOrgBmp = (HBITMAP)SelectObject(hdcMem,hBmp);

		HBRUSH hbr = CreateSolidBrush(m_crBack);
		FillRect(hdcMem,&rc,hbr);
		DeleteObject(hbr);

		BitBlt(hdc,
			0,0,
			rc.right-rc.left,
			rc.bottom-rc.top,
			hdcMem,0,0,SRCCOPY);

		SelectObject(hdcMem,hOrgBmp);

		DeleteObject(hBmp);
		DeleteDC(hdcMem);

		EndPaint(hWnd,&ps);

		return 0;
	}

	LRESULT OnEraseBkGnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HDC hdc = (HDC)wParam;
		HDC hdcMem;
		HBITMAP hBmp,hOrgBmp;
		RECT rc;

		GetClientRect(hWnd,&rc);

		hdcMem = CreateCompatibleDC(hdc);
		hBmp = CreateCompatibleBitmap(hdc,rc.right-rc.left,rc.bottom-rc.top);

		hOrgBmp = (HBITMAP)SelectObject(hdcMem,hBmp);

		HBRUSH hbr = CreateSolidBrush(m_crBack);
		FillRect(hdcMem,&rc,hbr);
		DeleteObject(hbr);

		BitBlt(hdc,
			0,0,
			rc.right-rc.left,
			rc.bottom-rc.top,
			hdcMem,0,0,SRCCOPY);

		SelectObject(hdcMem,hOrgBmp);

		DeleteObject(hBmp);
		DeleteDC(hdcMem);

		return TRUE;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);

		int cyTopMargin = 2;
		int cyMargin = 2;

		int cxMargin = 0;
		int cyPathBox = GetToolbarButtonHeight() - 1;
		const int cxPathBoxMargin = 6;

		int cxtb,cytb;
		CalcToolBarSize(m_hWndToolbar,cxtb,cytb);

		int cxtbMenu,cytbMenu;
		CalcToolBarSize(m_hWndMenubar,cxtbMenu,cytbMenu);

		SetWindowPos(m_hWndVolumeBox,NULL,
				cxPathBoxMargin + cxMargin + cxMargin,
				cyTopMargin,
				cx-cxtb-cxtbMenu-(cxMargin*2)-cxMargin - cxPathBoxMargin,
				cyPathBox,
				SWP_NOZORDER);

		SetWindowPos(m_hWndPathBox,NULL,
				cxPathBoxMargin + cxMargin + cxMargin,
				cyTopMargin + cytbMenu + cyMargin,
				cx-cxtb-cxtbMenu-(cxMargin*2)-cxMargin - cxPathBoxMargin,
				cyPathBox,
				SWP_NOZORDER);

		SetWindowPos(m_hWndToolbar,NULL,
				cx-cxtb-cxtbMenu,
				cyTopMargin + cytbMenu + cyMargin,
				cxtb,
				cyPathBox,
				SWP_NOZORDER);

		SetWindowPos(m_hWndMenubar,NULL,
				cx-cxtbMenu,
				cyTopMargin + cytbMenu + cyMargin,
				cxtbMenu,
				cytbMenu,
				SWP_NOZORDER);

		RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
		RedrawWindow(m_hWndToolbar,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;

		if( pnmhdr->hwndFrom == m_hWndToolbar || pnmhdr->hwndFrom == m_hWndMenubar  )
		{
			switch( pnmhdr->code )
			{
				case TBN_DROPDOWN:
					return OnDropdown((LPNMTOOLBAR)pnmhdr);
				case TBN_GETINFOTIP:
				{
					NMTBGETINFOTIP *ptbit = (NMTBGETINFOTIP *)lParam;
					LoadString(_GetResourceInstance(),ptbit->iItem,ptbit->pszText,ptbit->cchTextMax);
					return 0;
				}
			}
		}
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			case WM_PAINT:
				return OnPaint(hWnd,uMsg,wParam,lParam);
			case WM_ERASEBKGND:
				return OnEraseBkGnd(hWnd,uMsg,wParam,lParam);
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_SETTEXT:
				SetPath((PCWSTR)lParam);
				return 0;
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_COMMAND:
				return SendMessage(GetActiveWindow(),uMsg,wParam,lParam);
			case HBM_COLOR:
			{
				switch( LOWORD(wParam) )
				{
					case HBC_ACTIVE:
					{
						m_crBack = m_crHighlightBack;
						m_crText = m_crHighlightText;
						EnableWindow(m_hWndToolbar,TRUE);
						EnableWindow(m_hWndMenubar,TRUE);
						LONGPATHBOXINFO lpbi = {0};
						lpbi.crBack   = m_crBoxBack;
						lpbi.crBorder = m_crBoxBorder;
						lpbi.crText   = m_crText;
						SendMessage(m_hWndPathBox,LPBM_SETINFO,0,(LPARAM)&lpbi);
						SendMessage(m_hWndVolumeBox,LPBM_SETINFO,0,(LPARAM)&lpbi);
						break;
					}
					case HBC_INACTIVE:
					{
						m_crBack = m_crNormalBack;
						m_crText = m_crNormalText;
						EnableWindow(m_hWndToolbar,FALSE);
						EnableWindow(m_hWndMenubar,FALSE);
						LONGPATHBOXINFO lpbi = {0};
						lpbi.crBack   = m_crBoxInactiveBack;
						lpbi.crBorder = m_crBoxInactiveBorder;
						lpbi.crText   = m_crText;
						SendMessage(m_hWndPathBox,LPBM_SETINFO,0,(LPARAM)&lpbi);
						SendMessage(m_hWndVolumeBox,LPBM_SETINFO,0,(LPARAM)&lpbi);
						break;
					}
					case HBC_SETCOLOR:
					{
						if( lParam )
						{
							HEADERBARCOLOR *pIC = (HEADERBARCOLOR *)lParam;
							SetColors(
								pIC->crNormalBack,
								pIC->crNormalText,
								pIC->crHighlightBack,
								pIC->crHighlightText
								);
							SetBoxColors(
								pIC->crBoxBack,
								pIC->crBoxBorder,
								pIC->crBoxInactiveBack,
								pIC->crBoxInactiveBorder
								);
						}
						break;
					}	
					case HBC_GETCOLOR:
					{
						if( lParam )
						{
							HEADERBARCOLOR *pIC = (HEADERBARCOLOR *)lParam;
							GetColors(
								pIC->crNormalBack,
								pIC->crNormalText,
								pIC->crHighlightBack,
								pIC->crHighlightText
								);
							GetBoxColors(
								pIC->crBoxBack,
								pIC->crBoxBorder,
								pIC->crBoxInactiveBack,
								pIC->crBoxInactiveBorder
								);
						}
						break;
					}	
				}
				RedrawWindow(hWnd,NULL,NULL,RDW_UPDATENOW|RDW_INVALIDATE);
			}
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

private:
	int LoadToolbarIcon(HIMAGELIST himl,UINT id,int cxcy)
	{
		HICON hIcon;
		int iIndex = -1;
		hIcon = (HICON)LoadImage(_GetResourceInstance(),
							MAKEINTRESOURCE(id),
							IMAGE_ICON,
							cxcy,
							cxcy,
							LR_DEFAULTCOLOR);
		if( hIcon )
		{
			iIndex = ImageList_AddIcon(himl,hIcon);
			DestroyIcon(hIcon);
		}
		return iIndex;
	}

	HWND CreateToolbarButtons(HWND hWndParent,int BarType)
	{
		// Create the toolbar.
		HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
				WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
				CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN | 
				TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |TBSTYLE_AUTOSIZE | TBSTYLE_TOOLTIPS,
				0, 0, 0, 0,
				hWndParent, NULL, 
				_GetResourceInstance(),
				NULL);

	    if (hWndToolbar == NULL)
		{
			return NULL;
		}

		SendMessage(hWndToolbar, CCM_SETVERSION, (WPARAM) 5, 0); 

		int cxcyIcon = 24;
		int per = GetTextParcent();
		if( per <= 100 )
			cxcyIcon = 20;
		else if( per <= 150 )
			cxcyIcon = 24;
		else if( per <= 200 )
			cxcyIcon = 32;

		HIMAGELIST hImageList = ImageList_Create(cxcyIcon,cxcyIcon,
									ILC_COLOR32 | ILC_MASK, // ensures transparent background.
									8, 0);
		SendMessage(hWndToolbar,TB_SETIMAGELIST, 0, (LPARAM)hImageList);

		HIMAGELIST himlDisabled = ImageList_Create(cxcyIcon,cxcyIcon,
									ILC_COLOR32 | ILC_MASK, // ensures transparent background.
									8, 0);
		SendMessage(hWndToolbar,TB_SETDISABLEDIMAGELIST, 0, (LPARAM)himlDisabled);

		LoadToolbarIcon(hImageList,IDI_MENU,cxcyIcon);
		LoadToolbarIcon(himlDisabled,IDI_MENU_DISABLED,cxcyIcon);

		LoadToolbarIcon(hImageList,IDI_UP,cxcyIcon);
		LoadToolbarIcon(himlDisabled,IDI_UP_DISABLED,cxcyIcon);

		LoadToolbarIcon(hImageList,IDI_FOLDER_OPEN,cxcyIcon);
		LoadToolbarIcon(himlDisabled,IDI_FOLDER_OPEN_DISABLED,cxcyIcon);

		if( BarType == 0 )
		{
			TBBUTTON tbButtons[] = 
			{
				{ MAKELONG(1, 0), ID_UP_DIR,  TBSTATE_ENABLED, TBSTYLE_BUTTON,   {0}, 0, 0 },
				{ MAKELONG(2, 0), ID_GOTO,    TBSTATE_ENABLED, TBSTYLE_BUTTON,   {0}, 0, 0 },
			};

			SendMessage(hWndToolbar,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS);

			// Add buttons.
			SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON), 0);
			SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)ARRAYSIZE(tbButtons),(LPARAM)&tbButtons);
		}
		else
		{
			TBBUTTON tbButtons[] = 
			{
				{ MAKELONG(0, 0), ID_MENU,    TBSTATE_ENABLED, TBSTYLE_DROPDOWN, {0}, 0, 0 },
			};

			SendMessage(hWndToolbar, TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS);

			// Add buttons.
			SendMessage(hWndToolbar, TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON), 0);
			SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)ARRAYSIZE(tbButtons),(LPARAM)&tbButtons);
		}

	    return hWndToolbar;
	}

	LRESULT OnDropdown(LPNMTOOLBAR pnmtb)
	{
		HMENU hMenu = CreatePopupMenu();

		SendMessage(GetParent(m_hWnd),PM_MAKECONTEXTMENU,pnmtb->iItem,(LPARAM)hMenu);

		RECT rcButton;
		TPMPARAMS tpm = {0};
		UINT uFlags = TPM_RIGHTALIGN|TPM_TOPALIGN;

		SendMessage(pnmtb->hdr.hwndFrom,TB_GETRECT,pnmtb->iItem,(LPARAM)&rcButton);
		MapWindowPoints(pnmtb->hdr.hwndFrom,NULL,(LPPOINT)&rcButton,2);

		int x = rcButton.right;
		int y = rcButton.bottom;

		tpm.cbSize = sizeof(tpm);
		tpm.rcExclude = rcButton;

		TrackPopupMenuEx(hMenu,uFlags,x,y,GetActiveWindow(),&tpm);

		DestroyMenu(hMenu);

		return 0;
	}

	VOID CalcToolBarSize(HWND hwndTB,int &cx,int &cy)
	{
		int i;
		RECT rect;
		cx = cy = 0;
		int cButtons = (int)SendMessage(hwndTB,TB_BUTTONCOUNT,0,0);
		for(i = 0; i < cButtons; i++)
		{
			SendMessage(hwndTB,TB_GETITEMRECT,i,(LPARAM)&rect);
			cx += (rect.right - rect.left);
		}
		cy = (rect.bottom - rect.top);
	}
};
