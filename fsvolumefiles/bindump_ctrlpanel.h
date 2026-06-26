#pragma once
//*****************************************************************************
//
//  bindump_ctrlpanel.h
//
//  PURPOSE: Binary Dump Control Panel
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-05-19 Created
//
//*****************************************************************************

#define _CX_TB_MARGIN 16
#define _CX_PROPLV_MARGIN 8

#define CLR_PANE_ACTIVE_BACKGROUND   RGB(240,244,249)
#define CLR_PANE_INACTIVE_BACKGROUND RGB(243,243,243)

#define EDITBOX_MAX_CHARS 22

class CQuickBinDumpCtrlPanel : public CDialogWindow
{
	HWND m_hWndToolbar;
	HWND m_hWndRefreshBtn;
	HWND m_hWndEdit;
	HWND m_hWndProp;
	HFONT m_hFont;
	HFONT m_hFontProp;
	HBRUSH m_hbrBackground;
	HBRUSH m_hbrInactiveBack;

	struct {
		int Size;
		int SizeKB;
		int PageSize;
		int Page; 
		int MaxPage;
		int Cursor;
		int CursorHex;
#ifdef _DEBUG_MAXWIDTH
		int MaxDec;
		int MaxHex;
#endif
	} m_propItem;

public:
	int cx_edit;
	int cy_edit;

public:
	DEFDIALOGRESID(IDD_PAGED_BINARY_DUMP_CTRLPANE)

	CQuickBinDumpCtrlPanel()
	{
		m_hWndToolbar = NULL;
		m_hWndRefreshBtn = NULL;
		m_hWndEdit = NULL;
		m_hWndProp = NULL;
		m_hbrBackground = NULL;
		m_hbrInactiveBack = NULL;
	}

	~CQuickBinDumpCtrlPanel()
	{
	}

	virtual VOID OnInitPane(PVOID) {}
	virtual VOID OnStartOperation(PVOID) {}
	virtual VOID OnEndOperation(PVOID) {}

	int GetWidth() const
	{
		const int cButtons = 4;
		DWORD dw = (DWORD)SendMessage(m_hWndToolbar,TB_GETBUTTONSIZE,0,0);
		return (int)LOWORD(dw) * cButtons + _CX_TB_MARGIN + _CX_TB_MARGIN;
	}

	LONGLONG GetOffset() const
	{
		LONGLONG offset;
		WCHAR buf[256];
		GetWindowText(m_hWndEdit,buf,ARRAYSIZE(buf));
		StrToInt64Ex (buf,STIF_SUPPORT_HEX,&offset);
		return offset;
	}

	VOID OnInitData(ULONG DataType=0,PCWSTR pszDataName=NULL,ULONG dwFlags=0)
	{
		; // reserved
	}

	VOID OnUpdateColor(BOOL bActive=true)
	{
		ListView_SetBkColor(m_hWndProp,bActive ? CLR_PANE_ACTIVE_BACKGROUND : CLR_PANE_INACTIVE_BACKGROUND);
		ListView_SetTextBkColor(m_hWndProp,bActive ? CLR_PANE_ACTIVE_BACKGROUND : CLR_PANE_INACTIVE_BACKGROUND);
	}

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

	int appropriateSmallIconSize()
	{
		int cxcyIcon = 24;
		int per = GetTextParcent();
		if( per <= 100 )
			cxcyIcon = 20;
		else if( per <= 150 )
			cxcyIcon = 24;
		else if( per <= 200 )
			cxcyIcon = 32;
		return cxcyIcon;
	}

	HWND CreateNavigationToolbar(HWND hWnd)
	{
		HWND hWndToolbar = CreateWindowEx(WS_EX_CONTROLPARENT, TOOLBARCLASSNAME, NULL, 
				WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
				CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN | 
				TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE,
				0, 0, 0, 0,
				m_hWnd, NULL, 
				_GetResourceInstance(),
				NULL);

	    if (hWndToolbar == NULL)
		{
			return NULL;
		}

		SendMessage(hWndToolbar, CCM_SETVERSION, (WPARAM) 5, 0); 

		int cxcyIcon = appropriateSmallIconSize();

		HIMAGELIST hImageList = ImageList_Create(cxcyIcon,cxcyIcon,
									ILC_COLOR32 | ILC_MASK, // ensures transparent background.
									8, 0);
		SendMessage(hWndToolbar,TB_SETIMAGELIST, 0, (LPARAM)hImageList);

		int idxFirst = LoadToolbarIcon(hImageList,IDI_SECTOR_FIRST,cxcyIcon);
		int idxPrev  = LoadToolbarIcon(hImageList,IDI_SECTOR_PREV,cxcyIcon);
		int idxNext  = LoadToolbarIcon(hImageList,IDI_SECTOR_NEXT,cxcyIcon);
		int idxLast  = LoadToolbarIcon(hImageList,IDI_SECTOR_LAST,cxcyIcon);
		int idxUpdate = LoadToolbarIcon(hImageList,IDI_SECTOR_UPDATE,cxcyIcon);

		TBBUTTON tbButtons[] = 
		{
			{ MAKELONG(idxFirst,0), ID_FIRST, TBSTATE_ENABLED, TBSTYLE_BUTTON|TBSTATE_WRAP, {0}, 0, (INT_PTR)L"First" },
			{ MAKELONG(idxPrev, 0), ID_BACK,  TBSTATE_ENABLED, TBSTYLE_BUTTON|TBSTATE_WRAP, {0}, 0, (INT_PTR)L"Prev" },
			{ MAKELONG(idxNext, 0), ID_NEXT,  TBSTATE_ENABLED, TBSTYLE_BUTTON|TBSTATE_WRAP, {0}, 0, (INT_PTR)L"Next" },
			{ MAKELONG(idxLast, 0), ID_LAST,  TBSTATE_ENABLED, TBSTYLE_BUTTON|TBSTATE_WRAP, {0}, 0, (INT_PTR)L"Last" },
		};

		SendMessage(hWndToolbar,CCM_SETVERSION, (WPARAM) 6, 0); 
		SendMessage(hWndToolbar,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS);
		SendMessage(hWndToolbar,TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 

#if _ENABLE_DARK_MODE_TEST
		SendMessage(hWndToolbar,TB_SETWINDOWTHEME,0,
			(LPARAM)(_IsDarkModeEnabled() ?  L"DarkMode" : L"Toolbar")); // currently not supported
#endif

		SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)ARRAYSIZE(tbButtons),(LPARAM)&tbButtons);

		DWORD dw = (DWORD)SendMessage(hWndToolbar,TB_GETBUTTONSIZE,0,0);
		SendMessage(hWndToolbar,TB_SETBUTTONSIZE,0,MAKELPARAM(_DPI_Adjust_X(48),_DPI_Adjust_Y(48)));

		return hWndToolbar;
	}

	HWND CreateRefreshButton(HWND hWnd)
	{
		HWND hWndToolbar = CreateWindowEx(WS_EX_CONTROLPARENT, TOOLBARCLASSNAME, NULL, 
				WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
				CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN | 
				TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_AUTOSIZE,
				0, 0, 0, 0,
				m_hWnd, NULL, 
				_GetResourceInstance(),
				NULL);

	    if (hWndToolbar == NULL)
		{
			return NULL;
		}

		SendMessage(hWndToolbar, CCM_SETVERSION, (WPARAM) 5, 0); 

		int cxcyIcon = appropriateSmallIconSize();

		HIMAGELIST hImageList = ImageList_Create(cxcyIcon,cxcyIcon,
									ILC_COLOR32 | ILC_MASK, // ensures transparent background.
									1, 0);
		SendMessage(hWndToolbar,TB_SETIMAGELIST, 0, (LPARAM)hImageList);

		LoadToolbarIcon(hImageList,IDI_SECTOR_UPDATE,cxcyIcon);

		TBBUTTON tbButtons[] = 
		{
	        { MAKELONG(0, 0), ID_GOTO, TBSTATE_ENABLED, TBSTYLE_BUTTON|TBSTATE_WRAP, {0}, 0, (INT_PTR)0 },
		};

		SendMessage(hWndToolbar,CCM_SETVERSION, (WPARAM) 6, 0); 
		SendMessage(hWndToolbar,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS);
		SendMessage(hWndToolbar,TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 
		SendMessage(hWndToolbar,TB_ADDBUTTONS, (WPARAM)ARRAYSIZE(tbButtons),(LPARAM)&tbButtons);
		SendMessage(hWndToolbar,TB_AUTOSIZE,0,0);

		PCWSTR pszTheme;
		if( _IsDarkModeEnabled() )
			pszTheme = L"DarkMode";
		else
			pszTheme = L"Toolbar";
		SendMessage(hWndToolbar,TB_SETWINDOWTHEME,0,(LPARAM)pszTheme);

		return hWndToolbar;
	}

	HWND CreateEditBox(HWND hWnd)
	{
		m_hFont= GetGlobalFont(hWnd);

		TBBUTTONINFO tbi = {0};
		tbi.cbSize = sizeof(tbi);
		tbi.dwMask = TBIF_BYINDEX|TBIF_SIZE;
		SendMessage(m_hWndRefreshBtn,TB_GETBUTTONINFO,0,(LPARAM)&tbi);
		DWORD dw = (DWORD)SendMessage(m_hWndRefreshBtn,TB_GETBUTTONSIZE,0,0);

		HDC hdc = GetDC(m_hWnd);
		HFONT prevFold = (HFONT)SelectObject(hdc,m_hFont);
		TEXTMETRIC tm;
		GetTextMetrics(hdc,&tm);
		SelectObject(hdc,prevFold);
		ReleaseDC(m_hWnd,hdc);

		const int maxChars = EDITBOX_MAX_CHARS;
		cx_edit = tm.tmAveCharWidth * maxChars;
		cy_edit = HIWORD(dw) - 1;

		// Create the edit control child window.    
		HWND hWndEdit = CreateWindowEx(0L, L"Edit", NULL, 
							WS_CHILD | WS_BORDER | WS_TABSTOP| WS_VISIBLE | ES_LEFT | ES_AUTOVSCROLL, 
							0,
							0,
							cx_edit,
							cy_edit, 
							hWnd, (HMENU) 3123, _GetResourceInstance(), 0 );

		SendMessage(hWndEdit,WM_SETFONT,(WPARAM)m_hFont,0);

		SendMessage(hWndEdit,EM_LIMITTEXT,maxChars,0);

		return hWndEdit;
	}

	void InitPropList()
	{
		// set font to prop-list 
		{
			HDC hdc;
			hdc = GetWindowDC(m_hWndProp);

			LOGFONT lf = {};
			GetObject((HFONT)SendMessage(m_hWndProp,WM_GETFONT,0,0),sizeof(LOGFONT),&lf);

			lf.lfHeight -= (-1);
			lf.lfCharSet = ANSI_CHARSET;
			m_hFontProp = CreateFontIndirect( &lf );

			SendMessage(m_hWndProp,WM_SETFONT,(WPARAM)m_hFontProp,0);

			ReleaseDC(m_hWndProp,hdc);
		}

		LVCOLUMN lvc = {};
		lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;

		lvc.fmt     = LVCFMT_LEFT;
		lvc.pszText = L"";
		ListView_InsertColumn(m_hWndProp,0,&lvc);

		lvc.pszText = L"";
		lvc.fmt     = LVCFMT_LEFT;
		ListView_InsertColumn(m_hWndProp,1,&lvc);

		int index = 0;
		m_propItem.Page      = InsertItem(index++,L"Page");
		m_propItem.MaxPage   = InsertItem(index++,L"Last Page");
		m_propItem.PageSize  = InsertItem(index++,L"Page Size");
		m_propItem.Size      = InsertItem(index++,L"Data Size");
		m_propItem.SizeKB    = InsertItem(index++,L"");
		m_propItem.Cursor    = InsertItem(index++,L"Cursor");
		m_propItem.CursorHex = InsertItem(index++,L"");
#ifdef _DEBUG_MAXWIDTH
		m_propItem.MaxDec    = InsertItem(index++,L"Max Dec");
		m_propItem.MaxHex    = InsertItem(index++,L"Max Hex");
		SetPropValue(m_propItem.MaxDec,L"9,223,372,036,854,775,807");
		SetPropValue(m_propItem.MaxHex,L"0xFFFFFFFFFFFFFFFF");
#endif
		AdjustColumnWidth();

		SetWindowPos(m_hWndProp,m_hWndEdit,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	}

	int InsertItem(int iItem,PCWSTR pszName)
	{
		LVITEM lvi = {};
		lvi.mask    = LVIF_TEXT;
		lvi.iItem   = iItem;
		lvi.pszText = (PWSTR)pszName;
		return ListView_InsertItem(m_hWndProp,&lvi);
	}

	int SetPropValue(int iItem,PCWSTR pszValue)
	{
		ListView_SetItemText(m_hWndProp,iItem,1,(PWSTR)pszValue);
		return 0;
	}

	void AdjustColumnWidth()
	{
		RECT rcProp;
		GetClientRect(m_hWndProp,&rcProp);

		int cxTitle;
		ListView_SetColumnWidth(m_hWndProp,0,LVSCW_AUTOSIZE_USEHEADER);
        cxTitle = ListView_GetColumnWidth(m_hWndProp,0);

		ListView_SetColumnWidth(m_hWndProp,1,LVSCW_AUTOSIZE_USEHEADER);
	}

	void SetToolbarButtonState(UINT uCmdId,BOOL Enable)
	{
		SendMessage(m_hWndToolbar,TB_SETSTATE,(WPARAM)uCmdId,(LPARAM)(Enable ? TBSTATE_ENABLED : 0));
	}

	int GetToolbarButtonState(UINT uCmdId)
	{
		return (int)SendMessage(m_hWndToolbar,TB_GETSTATE,(WPARAM)uCmdId,0);
	}

	LRESULT OnBinDumpUpdate(NMBINDUMP *pnmbd)
	{
		SetToolbarButtonState(ID_BACK,(pnmbd->ReadOffset.QuadPart > 0));
		SetToolbarButtonState(ID_NEXT,(pnmbd->ReadOffset.QuadPart < (pnmbd->Size.QuadPart - pnmbd->BufferLength)));
		SetToolbarButtonState(ID_FIRST,(pnmbd->Size.QuadPart > 0) && (pnmbd->ReadOffset.QuadPart > 0));
		SetToolbarButtonState(ID_LAST,(pnmbd->Size.QuadPart > 0) && (pnmbd->ReadOffset.QuadPart < (pnmbd->Size.QuadPart - pnmbd->BufferLength)));
		SetToolbarButtonState(ID_GOTO,(pnmbd->Size.QuadPart > 0) && (pnmbd->Buffer != NULL) );

		WCHAR szText[48];
		StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64x",pnmbd->CursorPos.QuadPart);
		SetWindowText(m_hWndEdit,szText);

		_CommaFormatString(pnmbd->ClusterSize,szText);
		SetPropValue(m_propItem.PageSize,szText);

		_CommaFormatString(pnmbd->Size.QuadPart,szText);
		SetPropValue(m_propItem.Size,szText);

		StrFormatByteSize64(pnmbd->Size.QuadPart,szText,ARRAYSIZE(szText));
		SetPropValue(m_propItem.SizeKB,szText);

		WCHAR szPage[24];
		WCHAR szMaxPage[24];
		_CommaFormatString(pnmbd->CurPage,szPage);
		_CommaFormatString(pnmbd->MaxPage,szMaxPage);
		SetPropValue(m_propItem.Page,szPage);
		SetPropValue(m_propItem.MaxPage,szMaxPage);

		return 0;
	}

	LRESULT OnBinDumpCursorMoved(NMBINDUMP *pnmbd)
	{
		WCHAR szText[48];

		StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64x",pnmbd->CursorPos.QuadPart);
		SetWindowText(m_hWndEdit,szText);

		_CommaFormatString(pnmbd->CursorPos.QuadPart,szText);
		SetPropValue(m_propItem.Cursor,szText);

		StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",pnmbd->CursorPos.QuadPart);
		SetPropValue(m_propItem.CursorHex,szText);

		return 0;
	}

	LRESULT OnBinDumpSetError(NMBINDUMP *pnmbd)
	{
		SetToolbarButtonState(ID_BACK,(pnmbd->ReadOffset.QuadPart > 0));
		SetToolbarButtonState(ID_NEXT,(pnmbd->ReadOffset.QuadPart < (pnmbd->Size.QuadPart - pnmbd->BufferLength)));
		SetToolbarButtonState(ID_FIRST,(pnmbd->Size.QuadPart > 0) && (pnmbd->ReadOffset.QuadPart > 0));
		SetToolbarButtonState(ID_LAST,(pnmbd->Size.QuadPart > 0) && (pnmbd->ReadOffset.QuadPart < (pnmbd->Size.QuadPart - pnmbd->BufferLength)));
		SetToolbarButtonState(ID_GOTO,(pnmbd->Size.QuadPart > 0) && (pnmbd->Buffer != NULL) );

		EnableWindow(m_hWndEdit,FALSE);
		SendMessage(m_hWndRefreshBtn,TB_SETSTATE,(WPARAM)ID_GOTO,(LPARAM)(pnmbd->Size.QuadPart > 0) ? 1 : 0);

		SetWindowText(m_hWndEdit,L"");

		ListView_DeleteAllItems(m_hWndProp);

		return 0;
	}

	LRESULT OnGetToolbarInfoTip(NMHDR *pnmhdr)
	{
		NMTBGETINFOTIP *pnmtbit = (NMTBGETINFOTIP *)pnmhdr;

		PWSTR pszText = NULL;
		switch( pnmtbit->iItem )
		{
			case ID_BACK:
				pszText = L"Previous Page (Alt+Left Arrow)";
				break;
			case ID_NEXT:
				pszText = L"Next Page (Alt+Right Arrow)";
				break;
			case ID_FIRST:
				pszText = L"First Page (Alt+Home)";
				break;
			case ID_LAST:
				pszText = L"Last Page (Alt+End)";
				break;
			case ID_GOTO:
				pszText = L"Goto Offset";
				break;
		}

		if( pszText )
			StringCchCopy(pnmtbit->pszText,pnmtbit->cchTextMax,pszText);

		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;

		switch( pnmhdr->code )
		{
			case TBN_GETINFOTIP:
				return OnGetToolbarInfoTip(pnmhdr);
			case BDN_UPDATE:
				return OnBinDumpUpdate((NMBINDUMP*)pnmhdr);
		}
		return 0;
	}

	virtual INT_PTR DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( uMsg )
		{
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_CTLCOLORDLG:
			{
				return (LRESULT)((GetForegroundWindow() == GetParent(hWnd)) ? m_hbrBackground : m_hbrInactiveBack);
			}
			case WM_CTLCOLORSTATIC:
			{
				BOOL bActive = (GetForegroundWindow() == GetParent(hWnd));
				SetBkColor((HDC)wParam,bActive ? CLR_PANE_ACTIVE_BACKGROUND : CLR_PANE_INACTIVE_BACKGROUND);
				return (LRESULT)(bActive ? m_hbrBackground : m_hbrInactiveBack);
			}
			case WM_SIZE:
			{
				int cx = GET_X_LPARAM(lParam);
				int cy = GET_Y_LPARAM(lParam);
				int _cy_space = (8+8);

				// Resize Toolbar
				DWORD dw = (DWORD)SendMessage(m_hWndToolbar,TB_GETBUTTONSIZE,0,0);
				int cxBtn = LOWORD(dw);
				int cyBtn = HIWORD(dw);
				int cyToolbar = cyBtn;
				SetWindowPos(m_hWndToolbar,NULL,
						_CX_TB_MARGIN,
						0,
						cxBtn * 4,
						cyToolbar,SWP_NOZORDER);

				// Resize and reposition Refresh button
				dw = (DWORD)SendMessage(m_hWndRefreshBtn,TB_GETBUTTONSIZE,0,0);
				SetWindowPos(m_hWndRefreshBtn,NULL,
						cx - _CX_TB_MARGIN - LOWORD(dw),
						cyToolbar + _cy_space,
						LOWORD(dw),
						HIWORD(dw),SWP_NOZORDER);

				// Resize and reposition Edit box
				RECT rcEdit;
				GetClientRect(m_hWndEdit,&rcEdit);

				SetWindowPos(m_hWndEdit,NULL,
						_CX_TB_MARGIN + 8,
						cyToolbar + _cy_space,
						cx-(_CX_TB_MARGIN + LOWORD(dw) + 2 + _CX_TB_MARGIN + 8),
						_RECT_HIGHT(rcEdit),
						SWP_NOZORDER);

				// Resize and reposition Proprty ListView
				{
					RECT rc = {};
					RECT rcItem = {};
					GetClientRect(m_hWndProp,&rc);
					int cItems = ListView_GetItemCount(m_hWndProp);
					ListView_GetItemRect(m_hWndProp,0,&rcItem,LVIR_BOUNDS);
					int cxlv = cx - 2;
					int cylv = _RECT_HIGHT(rcItem) * cItems + GetSystemMetrics(SM_CYVSCROLL);
					SetWindowPos(m_hWndProp,NULL,
							_CX_PROPLV_MARGIN,
							cyToolbar + (_CX_PROPLV_MARGIN + _CX_PROPLV_MARGIN) + _RECT_HIGHT(rcEdit) + 32,
							cxlv,
							cylv,
							SWP_NOZORDER);
				}
				break;
			}
			case WM_INITDIALOG:
			{
				m_hWndToolbar = CreateNavigationToolbar(hWnd);
				m_hWndRefreshBtn = CreateRefreshButton(hWnd);
				m_hWndEdit = CreateEditBox(hWnd);
				m_hbrBackground = CreateSolidBrush( CLR_PANE_ACTIVE_BACKGROUND );
				m_hbrInactiveBack = CreateSolidBrush( CLR_PANE_INACTIVE_BACKGROUND );
				m_hWndProp = GetDlgItem(hWnd,IDC_LIST);
				InitPropList();

				OnUpdateColor( GetForegroundWindow() == GetParent(m_hWnd) );

				break;
			}
			case WM_DESTROY:
			{
				if( m_hbrBackground )
				{
					DeleteObject(m_hbrBackground);
					m_hbrBackground = NULL;
				}
				if( m_hbrInactiveBack )
				{
					DeleteObject(m_hbrInactiveBack);
					m_hbrInactiveBack = NULL;
				}

				if( m_hFont )
					DeleteObject(m_hFont);

				if( m_hFontProp )
					DeleteObject(m_hFontProp);

				ImageList_Destroy( (HIMAGELIST)SendMessage(m_hWndToolbar,TB_GETIMAGELIST,0,0) );
				ImageList_Destroy( (HIMAGELIST)SendMessage(m_hWndRefreshBtn,TB_GETIMAGELIST,0,0) );

				break;
			}
			case WM_COMMAND:
			{
				SendMessage(GetParent(hWnd),WM_COMMAND,wParam,lParam);
				break;
			}
		}
		return CDialogWindow::DlgProc(hWnd,uMsg,wParam,lParam);
	}
};
