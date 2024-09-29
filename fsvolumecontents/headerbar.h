#pragma once
//***************************************************************************
//*                                                                         *
//*  headerbar.h                                                            *
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
#include "ntvolumenames.h"
#include "ntobjecthelp.h"

#define PM_CHANGEPATH (WM_APP+5000)

#define _USE_COMBOBOX_EX  1

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

#define IBM_SETCOLOR (PM_USERBASE+0)

typedef struct _HEADERBARITEM
{
	UINT Flags;
	PWSTR NtDeviceName;
	PWSTR Drive;
} HEADERBARITEM;

#define HDIF_DRIVE_ITEM  0x1

class CHeaderBarWindow : public CBaseWindow
{
	PWSTR m_pszText;
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
	HWND m_hWndPathBox;
	HWND m_hwndComboBox;

public:
	CHeaderBarWindow()
	{
		m_pszText = NULL;
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
	}

	~CHeaderBarWindow()
	{
		_SafeMemFree( m_pszText );
		if( m_hFont )
			DeleteObject(m_hFont);
	}

	HRESULT SetText(PCWSTR pszText)
	{
#if 0
		_SafeMemFree(m_pszText);

		if( pszText )
			m_pszText = _MemAllocString(pszText);

		SetWindowText(m_hWndPathBox,m_pszText);

		RedrawWindow(m_hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);

		return S_OK;
#else
		return E_NOTIMPL;
#endif
	}

	HRESULT SetPath(PCWSTR pszPath)
	{
		_SafeMemFree(m_pszText);

		UNICODE_STRING usPath;
		PWSTR pszRoot;

		RtlInitUnicodeString(&usPath,pszPath);

		FindRootDirectory_U(&usPath,&pszRoot);

		SetWindowText(m_hWndPathBox,pszRoot);

		UNICODE_STRING usVolumeName;
		RtlInitUnicodeString(&usVolumeName,pszPath);
		GetVolumeName_U(&usVolumeName);

		PWSTR pszVolumeName = AllocateSzFromUnicodeString(&usVolumeName);
		if( pszVolumeName )
		{
			Select(pszVolumeName);
			FreeMemory(pszVolumeName);
		}

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
		DWORD dw = (DWORD)SendMessage( m_hWndToolbar,TB_GETBUTTONSIZE,0,0);
		return HIWORD(dw) + DPI_SIZE_CY(8);
	}

	int GetToolbarButtonHeight()
	{
		DWORD dw = (DWORD)SendMessage( m_hWndToolbar,TB_GETBUTTONSIZE,0,0);
		int cy = (int)HIWORD(dw);
		return cy;
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		InitFont();

		m_hWndToolbar = CreateToolbarButtons(hWnd,0);
		m_hWndMenubar = CreateToolbarButtons(hWnd,1);

		//
		// Create Volume ComboBox
		//
#if _USE_COMBOBOX_EX
		INITCOMMONCONTROLSEX icex;
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC = ICC_USEREX_CLASSES;
		InitCommonControlsEx(&icex);
		m_hwndComboBox = CreateWindowEx(0, WC_COMBOBOXEX, NULL,
					WS_CHILD | WS_BORDER | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
					// No size yet--resize after setting image list.
					0,    // Vertical position of Combobox
					0,    // Horizontal position of Combobox
					0,    // Sets the width of Combobox
					2048, // Sets the height of Combobox
					hWnd,
					NULL,
					_GetResourceInstance(),
					NULL);
#else
		m_hwndComboBox = CreateWindow(L"COMBOBOX", L"", 
					CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
					0, 0, 0, 2048,
					hWnd, (HMENU)0, _GetResourceInstance(), NULL); 

		SendMessage(m_hwndComboBox,WM_SETFONT,(WPARAM)m_hFont,0);
#endif
		InitComboBox();

		//
		// Create Path Box 
		//
		m_hWndPathBox = CreateWindowEx(0,LPBC_LONGPATHBOX_NAME,L"",
							WS_VISIBLE|WS_CHILD|LPBS_FLAT_BORDER|LPBS_NO_TEXTSELECTION|LPBS_NO_FOCUS,
							0,0,0,0,m_hWnd,(HMENU)100,_GetResourceInstance(),0);

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

		int cxtb,cytb;
		CalcToolBarSize(m_hWndToolbar,cxtb,cytb);

		int cxtbMenu,cytbMenu;
		CalcToolBarSize(m_hWndMenubar,cxtbMenu,cytbMenu);

		SetWindowPos(m_hWndToolbar,NULL,
				cx-cxtb-cxtbMenu,
				(cy-cytb)/2,
				cxtb,
				cytb,
				SWP_NOZORDER);

		SetWindowPos(m_hWndMenubar,NULL,
				cx-cxtbMenu,
				(cy-cytbMenu)/2,
				cxtbMenu,
				cytbMenu,
				SWP_NOZORDER);

		const int margin = 4;
		int cxComboBox = _DPI_Adjust_X( 220 );
		int cyPathBox = GetToolbarButtonHeight();

		RECT rcComboBox;
		GetWindowRect(m_hwndComboBox,&rcComboBox);

		SetWindowPos(m_hwndComboBox,NULL,
				margin,
				(cy-_RECT_HIGHT(rcComboBox))/2,
				cxComboBox,
				_RECT_HIGHT(rcComboBox),
				SWP_NOZORDER);

		SetWindowPos(m_hWndPathBox,NULL,
				margin + cxComboBox + margin,
				(cy-cytbMenu)/2,
				cx-cxtb-cxtbMenu-(margin*2)-cxComboBox-margin,
				cyPathBox,
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
			}
		}
		else if( pnmhdr->hwndFrom == m_hwndComboBox  )
		{
			switch( pnmhdr->code )
			{
				case CBEN_DELETEITEM:
				{
					NMCOMBOBOXEX *pnmcbex = (NMCOMBOBOXEX *)pnmhdr;
					if( pnmcbex->ceItem.lParam )
					{
						CBEXITEM *pItem = (CBEXITEM *)pnmcbex->ceItem.lParam;
						_SafeMemFree(pItem->NtDevicePath);
						delete pItem;
					}
					break;
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
				SetText((PCWSTR)lParam);
				return 0;
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_COMMAND:
			{
				if( (HWND)lParam == m_hwndComboBox && CBN_SELCHANGE == HIWORD(wParam) )
				{
					int iItem = (int)SendMessage(m_hwndComboBox,CB_GETCURSEL,0,0);
					if( iItem != CB_ERR )
					{
						CBEXITEM *pItem = (CBEXITEM *)SendMessage(m_hwndComboBox,CB_GETITEMDATA,iItem,0);

						HEADERBARITEM hbi = {0};
						hbi.Flags = pItem->Flags;
						hbi.Drive = pItem->Drive;
						hbi.NtDeviceName = pItem->NtDevicePath;
						SendMessage(GetParent(hWnd),PM_CHANGEPATH,0,(LPARAM)&hbi);
					}
					return 0;
				}
				else if( (HWND)lParam == this->m_hWndPathBox )
				{
					return 0;
				}
				return SendMessage(GetActiveWindow(),uMsg,wParam,lParam);
			}
			case IBM_SETCOLOR:
			{
				switch( LOWORD(wParam) )
				{
					case 1:
					{
						m_crBack = m_crHighlightBack;
						m_crText = m_crHighlightText;
						EnableWindow(m_hWndToolbar,TRUE);
						EnableWindow(m_hWndMenubar,TRUE);
						LONGPATHBOXINFO lpbi = {0};
						lpbi.crBack   = m_crBoxBack;
						lpbi.crBorder = m_crBoxBorder;
						SendMessage(m_hWndPathBox,LPBM_SETINFO,0,(LPARAM)&lpbi);
						break;
					}
					case 0:
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
						break;
					}
					case 2:
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
					case 3:
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
				TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_AUTOSIZE |TBSTYLE_TRANSPARENT,
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
				{ MAKELONG(1, 0), ID_UP_DIR,   TBSTATE_ENABLED, TBSTYLE_BUTTON,   {0}, 0, 0 },
				{ MAKELONG(2, 0), ID_GOTO,     TBSTATE_ENABLED, TBSTYLE_BUTTON,   {0}, 0, 0 },
				{ MAKELONG(0, 0), 0,           0,               BTNS_SEP,         {0}, 0, 0 },
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
				{ MAKELONG(0, 0), ID_MENU,     TBSTATE_ENABLED, TBSTYLE_DROPDOWN,       {0}, 0, 0 },
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

private:

typedef struct CBEXITEM {
	UINT Flags;
	PWSTR NtDevicePath;  // pointer to device string like as "\\Device\\HarddiskVolumeX"
	WCHAR Drive[3];      // "X:"
} CBEXITEM;

	VOID InitComboBox()
	{
		WCHAR szName[MAX_PATH];
		WCHAR szDrive[8];

		LoadFltLibDll(NULL);

		VOLUME_NAME_STRING_ARRAY *VolumeNames;
		EnumVolumeNames( &VolumeNames );

		for(ULONG i = 0; i < VolumeNames->Count; i++)
		{
			if( GetVolumeDosName(VolumeNames->Volume[i].NtVolumeName,szDrive,ARRAYSIZE(szDrive)) != S_OK )
			{
				szDrive[0] = 0;
			}

			CBEXITEM *pItem = new CBEXITEM;

			pItem->Flags = 0;

			pItem->NtDevicePath = _MemAllocString( VolumeNames->Volume[i].NtVolumeName );
			pItem->Drive[0] = szDrive[0];
			pItem->Drive[1] = L':';
			pItem->Drive[2] = 0;

			PWSTR pName = L"";
			if( HasPrefix(L"\\Device\\",pItem->NtDevicePath) )
			{
				pName = &pItem->NtDevicePath[8];
			}

			if( szDrive[0] )
			{
				StringCchPrintf(szName,MAX_PATH,L"%s (%s)",pName,szDrive);
			}
			else
			{
				StringCchPrintf(szName,MAX_PATH,L"%s",pName);
			}
#if _USE_COMBOBOX_EX
			COMBOBOXEXITEM cbei = {0};
			cbei.mask = CBEIF_TEXT|CBEIF_LPARAM;
			cbei.iItem = i;
			cbei.pszText = szName;
			cbei.lParam = (LPARAM)pItem;
			SendMessage(m_hwndComboBox,CBEM_INSERTITEM,0,(LPARAM)&cbei);
#else
			int iIndex = ComboBox_AddString(m_hwndComboBox,szName);
			ComboBox_SetItemData(m_hwndComboBox,iIndex,(LPARAM)pItem);
#endif
		}
	
		FreeVolumeNames( VolumeNames );

		UnloadFltLibDll(0);

		PCWSTR *pDosDriveMap;
		GetNtDeviceNameAssignedDosDrive( &pDosDriveMap );

		int cItems = (int)SendMessage(m_hwndComboBox,CB_GETCOUNT,0,0);

		for(int i = 0; i < cItems; i++)
		{
			CBEXITEM *pItem = (CBEXITEM *)SendMessage(m_hwndComboBox,CB_GETITEMDATA,i,0);
			 
			for(int drv=0; drv < 26; drv++)
			{
				if( pDosDriveMap[drv] && (pItem->Drive[0] == (L'A' + drv)) )
				{
					pDosDriveMap[drv] = NULL;
					break;
				}
			}
		}

		int iItem = cItems;
		for(int drv = 0; drv < 26; drv++)
		{
			if( pDosDriveMap[drv] )
			{
				StringCchPrintf(szName,MAX_PATH,L"%c: (%s)",L'A'+drv,pDosDriveMap[drv]);

				CBEXITEM *pItem = new CBEXITEM;

				pItem->Flags = HDIF_DRIVE_ITEM;

				pItem->NtDevicePath = _MemAllocString( pDosDriveMap[drv] );
				pItem->Drive[0] = L'A' + drv;
				pItem->Drive[1] = L':';
				pItem->Drive[2] = 0;
#if _USE_COMBOBOX_EX
				COMBOBOXEXITEM cbei = {0};
				cbei.mask = CBEIF_TEXT|CBEIF_LPARAM;
				cbei.iItem = iItem++;
				cbei.pszText = szName;
				cbei.lParam = (LPARAM)pItem;
				SendMessage(m_hwndComboBox,CBEM_INSERTITEM,0,(LPARAM)&cbei);
#else
				int iIndex = ComboBox_AddString(m_hwndComboBox,szName);
				ComboBox_SetItemData(m_hwndComboBox,iIndex,(LPARAM)pItem);
#endif
			}
		}

		FreeMemory(pDosDriveMap);
	}

	void GetNamePart(PCWSTR pszVolumeName)
	{
		PCWSTR pszName;

		if( pszVolumeName[0] == L'\\' && pszVolumeName[1] == L'\\' )
		{
			;// "\\ComputerName\SharePoint" 
		}
		if( PathIsPrefixDosDeviceDrive(pszVolumeName) )
		{
			;// "\??\d:"
		}
		else if( PathIsPrefixDosDevice(pszVolumeName) )
		{
			;// "\??\DosDeviceName"
			;// "\??\unc\ComputerName\SharePoint" 
		}
		else
		{
			if( HasPrefix(L"\\Device\\LanmanRedirector",pszVolumeName) ||
			    HasPrefix(L"\\Device\\Mup",pszVolumeName) )
			{
				pszName = wcschr(&pszVolumeName[8],L'\\');
				if( pszName )
					pszName++;
				else
					pszName = pszVolumeName;
			}
			else if( HasPrefix(L"\\Device\\",pszVolumeName) )
			{
				pszName = &pszVolumeName[8];
			}
			else
			{
				pszName = pszVolumeName;
			}
		}
	}

	void Select(PCWSTR pszVolumeName)
	{
		int i,cItems = (int)SendMessage(m_hwndComboBox,CB_GETCOUNT,0,0);
		int comp = 0;

		if( PathIsPrefixDosDeviceDrive(pszVolumeName) )
		{
			comp = 1;
		}


#if 0
		{
			UNICODE_STRING Root;
			RtlInitUnicodeString(&Root,pszVolumeName);
			GetRootDirectory_U(&Root);

			UNICODE_STRING Volume;
			RtlInitUnicodeString(&Volume,pszVolumeName);
			GetVolumeName_U(&Volume);

			PWSTR p1 = AllocateSzFromUnicodeString(&Root);
			PWSTR p2 = AllocateSzFromUnicodeString(&Volume);
			FreeMemory(p1);
			FreeMemory(p2);
		}
#endif

		for(i = 0; i < cItems; i++)
		{
			CBEXITEM *pItem = (CBEXITEM *)SendMessage(m_hwndComboBox,CB_GETITEMDATA,i,0);

			if( (comp == 0 && (wcsicmp(pItem->NtDevicePath,pszVolumeName) == 0)) ||
				(comp == 1 && (pItem->Drive[0] == towupper(pszVolumeName[4])))	)
			{
				SendMessage(m_hwndComboBox,CB_SETCURSEL,i,0);
				return;
			}
		}

		SendMessage(m_hwndComboBox,CB_SETCURSEL,(WPARAM)-1,0);
	}
};
