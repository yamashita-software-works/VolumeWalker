//*****************************************************************************
//
//  dialog_launchpad.cpp
//
//  PURPOSE: LaunchPad Dialog
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-06-22 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "resource.h"
#include "uilayout.h"
#include "basewindow.h"
#include "launchpad.h"

enum {
_PAGE_APPLAUNCH=0,
_PAGE_MAX,
_PAGE_BLANK_ITEM,
};

typedef struct _LAUNCHPAD_PARAM
{
	int start_page;
	int pages[16];
	int page_count;
	HWND hWndDialog;
	HWND hWndOwner;
	HANDLE hHostThread;
	DWORD dwThreadId;
	BOOL Center;
} LAUNCHPAD_PARAM;

struct CLaunchPadPage : public CDialogWindow
{
	virtual VOID OnInitPage(PVOID) = 0;
};

#include "page_launch_applaunch.h"

struct CLaunchPadWindow : public CDialogWindow
{
	CUILayout m_Layout;

	HWND m_hWndTree;

	CLaunchPadPage *m_pCtrlPane;
	CLaunchPadPage *m_page[_PAGE_MAX];

	CLaunchPadWindow(int mode=CDialogWindow::Modal)
	{
		m_hWndTree = NULL;
		m_pCtrlPane = NULL;
		ZeroMemory(m_page,sizeof(m_page));
		m_dialogmode = mode; /* modal modeless persistent */
	}

	~CLaunchPadWindow()
	{
		for(int i = 0; i < ARRAYSIZE(m_page); i++)
		{
			if( m_page[i] )
			{
				delete m_page[i];
				m_page[i] = NULL;
			}
		}
	}

	VOID CloseDialog(int code)
	{
		EndDialog(m_hWnd,(INT_PTR)code);
	}

	VOID InitizlizePageSelector(int cPages,int idPages[])
	{
		TreeView_SetExtendedStyle(m_hWndTree,TVS_EX_DOUBLEBUFFER|TVS_EX_AUTOHSCROLL,TVS_EX_DOUBLEBUFFER|TVS_EX_AUTOHSCROLL);

		int cyItem = TreeView_GetItemHeight(m_hWndTree);
		TreeView_SetItemHeight(m_hWndTree,(int)((double)cyItem * 1.5));

		TreeView_SetBkColor(m_hWndTree,RGB(248,248,248));

		TVINSERTSTRUCT tvi = {0};

		tvi.itemex.mask  = TVIF_TEXT|TVIF_PARAM;
		tvi.hInsertAfter = TVI_LAST;

		for(int i = 0; i < cPages; i++)
		{
			switch( idPages[i] )
			{
				case _PAGE_APPLAUNCH:
					tvi.itemex.pszText = L"External Tools";
					break;
				case _PAGE_BLANK_ITEM:
					InsertBlank(TVI_ROOT,tvi.hInsertAfter);
					continue;
			}
			tvi.itemex.lParam = idPages[i];
			TreeView_InsertItem(m_hWndTree,&tvi);	
		}
 	}

	HTREEITEM InsertBlank(HTREEITEM hParent,HTREEITEM hInsert)
	{
		TVINSERTSTRUCT tvi = {0};
		tvi.hInsertAfter = TVI_LAST;
		tvi.hParent =  TVI_ROOT;
		tvi.itemex.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_PARAM|TVIF_INTEGRAL;
		tvi.itemex.pszText = L"";
		tvi.itemex.lParam = (LPARAM)_PAGE_BLANK_ITEM;
		tvi.itemex.iImage = tvi.itemex.iExpandedImage = I_IMAGENONE;
		tvi.itemex.iIntegral = 2;  // blank line height
		tvi.itemex.uStateEx = 0;
		HTREEITEM h = TreeView_InsertItem(m_hWndTree, &tvi);
		TVITEMEX tviex = {0};
		tviex.mask = TVIF_STATEEX;
		tviex.hItem = h;
		tviex.uStateEx = TVIS_EX_DISABLED;
		TreeView_SetItem( m_hWndTree, &tviex );
		return h;
	}

	HTREEITEM FindItem(LPARAM PageId)
	{
		HTREEITEM htvi;
		htvi = TreeView_GetRoot(m_hWndTree);
		if( htvi )
		{
			do
			{
				LPARAM lParam = (LPARAM)TreeView_GetItemData(m_hWndTree,htvi);
				if( lParam == PageId )
					return htvi;
			}
			while( (htvi = TreeView_GetNextSibling(m_hWndTree,htvi)) != NULL );
		}
		return NULL;
	}

	BOOL CreatePage(HWND hDlg,int page)
	{
		CLaunchPadPage *pNewPane = NULL;
		CLaunchPadPage *pPrevPane = NULL;

		if( page >= _countof(m_page) )
			return FALSE;

		pPrevPane = m_pCtrlPane;

		if( m_page[page] )
		{
			pNewPane = m_page[page];
		}
		else
		{
			switch( page )
			{
				case _PAGE_APPLAUNCH:
					pNewPane = static_cast<CLaunchPadPage*>(new CAppLaunchPage);
					break;
			}
	
			if( pNewPane == NULL )
				return FALSE;
	
			pNewPane->Create(hDlg,pNewPane->GetResId());
		}
	
		// mapping the pane window to placeholder location.
		RECT rcOptionPane;
		if( IsWindow(GetDlgItem(hDlg,IDC_PLACEHOLDER)) )
		{
			HWND hwndPlaceHolder = GetDlgItem(hDlg,IDC_PLACEHOLDER);
			GetWindowRect(hwndPlaceHolder,&rcOptionPane);
			MapWindowPoints(NULL,hDlg,(LPPOINT)&rcOptionPane,2);
			HWND hwndPrev = GetWindow(hwndPlaceHolder,GW_HWNDPREV);
			DestroyWindow(hwndPlaceHolder);
			SetWindowPos(pNewPane->m_hWnd,hwndPrev,rcOptionPane.left,rcOptionPane.top, 
				_RECT_WIDTH(rcOptionPane),_RECT_HIGHT(rcOptionPane),0);
		}	
		else
		{
			HWND hwndPrev = GetWindow(pPrevPane->m_hWnd,GW_HWNDPREV);
			GetWindowRect(GetDlgItem(hDlg,0),&rcOptionPane);
			MapWindowPoints(NULL,hDlg,(LPPOINT)&rcOptionPane,2);
			SetWindowPos(pNewPane->m_hWnd,hwndPrev,rcOptionPane.left,rcOptionPane.top, 
				_RECT_WIDTH(rcOptionPane),_RECT_HIGHT(rcOptionPane),0);
		}

		if( pPrevPane )
		{
			EnableWindow(pPrevPane->m_hWnd,FALSE);
			ShowWindow(pPrevPane->m_hWnd,SW_HIDE);
		}

		EnableWindow(pNewPane->m_hWnd,TRUE);
		ShowWindow(pNewPane->m_hWnd,SW_SHOW);

		m_page[page] = m_pCtrlPane = pNewPane;

		m_pCtrlPane->OnInitPage(NULL);

		return TRUE;
 	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		LAUNCHPAD_PARAM *pdlgParam = (LAUNCHPAD_PARAM *)lParam;
	
		pdlgParam->hWndDialog = hDlg;

		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		//
		// Attach Tree Control
		//
		m_hWndTree = GetDlgItem(hDlg,IDC_TREE);

		//
		// Init Tree
		//
		InitizlizePageSelector(pdlgParam->page_count,pdlgParam->pages);

		//
		// Create initial page
		//
		CreatePage(hDlg,pdlgParam->start_page);

		HTREEITEM htvi = FindItem( pdlgParam->start_page );
		if( htvi )
			TreeView_Select(m_hWndTree,htvi,TVGN_CARET);

		//
		// Initialize Layout
		//
		m_Layout.Initialize(hDlg);			
		m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_BOTTOMLEFT,IDC_TREE,FALSE);
		m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_BOTTOMRIGHT,0,FALSE);

		//
		// Remove the icon, the minimize and the restore buttons 
		// from the window has resizable border.
		//
		DWORD dw = GetWindowLong(hDlg,GWL_EXSTYLE);
		dw |= WS_EX_DLGMODALFRAME;
		SetWindowLong(hDlg,GWL_EXSTYLE,dw);
		if( (HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0) == NULL )
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)NULL);

		// Window location from the MainFrame relative
		if( pdlgParam->Center )
			_CenterWindow( hDlg, GetParent(hDlg) );

		return FALSE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		LAUNCHPAD_PARAM *pdlgParam = (LAUNCHPAD_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		SetWindowLongPtr(hDlg,DWLP_USER,NULL);

		PostMessage(pdlgParam->hWndOwner,WM_NOTIFY_MESSAGE,MAKEWPARAM(UI_NOTIFY_CLOSED,0),(LPARAM)hDlg);

		// When deleting all items, prevent to change the item selection  notification.
		TreeView_Select(m_hWndTree,NULL,TVGN_CARET);

		TreeView_DeleteAllItems(m_hWndTree);

		delete pdlgParam;

		if( m_dialogmode == CDialogWindow::Persistent )
			PostQuitMessage(0); // Exit UI thread message loop

		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		LAUNCHPAD_PARAM *pdlgParam = (LAUNCHPAD_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		CloseDialog(IDCLOSE);
		return 0;
	}
	
	LRESULT OnSize(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		m_Layout.AdjustControls(cx,cy);
		return 0;
	}

	LRESULT OnSetCursor(LPNMMOUSE lpnmm)
	{
		GetCursorPos(&lpnmm->pt);

		TVHITTESTINFO tvht = {0};
		tvht.pt    = lpnmm->pt;
		ScreenToClient(m_hWndTree,&tvht.pt);
		tvht.flags = TVHT_ONITEM|TVHT_ONITEMSTATEICON|TVHT_ONITEMRIGHT;
		tvht.hItem = NULL;
		HTREEITEM hItem = TreeView_HitTest(m_hWndTree,&tvht);
		UINT ItemType = (UINT)TreeView_GetItemData(m_hWndTree,hItem);
		if( ItemType == _PAGE_BLANK_ITEM )
		{
			SetCursor( LoadCursor(NULL,IDC_ARROW) );
			return 1;
		}
		return 0;
	}

	LRESULT OnSelChanging(NMTREEVIEW *pnmtv)
	{
		TVITEMEX tiex={0};
		tiex.mask = TVIF_STATEEX;
		tiex.hItem = pnmtv->itemNew.hItem;
		TreeView_GetItem(m_hWndTree,&tiex);
		if( tiex.uStateEx & TVIS_EX_DISABLED )
		{
			if( pnmtv->action == 2 )
			{
				BOOL bPageKey = FALSE;

				BYTE keystate[256] = {0};
				GetKeyboardState(keystate);

				SetRedraw(m_hWndTree,FALSE);

				if( (keystate[VK_HOME] & 0x80) || (keystate[VK_PRIOR] & 0x80) || (keystate[VK_NEXT] & 0x80) )
					bPageKey = TRUE;

				HTREEITEM hNextItem = NULL;

				if( bPageKey )
				{
					TreeView_Select(m_hWndTree,(hNextItem = TreeView_GetNextVisible(m_hWndTree,pnmtv->itemNew.hItem)),TVGN_CARET);
				}
				else
				{
					if( GetKeyState(VK_DOWN) < 0 )
						hNextItem = TreeView_GetNextVisible(m_hWndTree,pnmtv->itemNew.hItem);
					else
						hNextItem = TreeView_GetPrevVisible(m_hWndTree,pnmtv->itemNew.hItem);

					if( hNextItem )
						TreeView_Select(m_hWndTree,hNextItem,TVGN_CARET);
				}

				if( hNextItem )
				{
					TreeView_EnsureVisible(m_hWndTree, hNextItem);
				}
				SetRedraw(m_hWndTree,TRUE);
			}
			return TRUE;
		}
		return 0;
	}

	LRESULT OnCustomDraw(LPNMLVCUSTOMDRAW pcd)
	{
		if( pcd->nmcd.dwDrawStage == CDDS_PREPAINT )
			return CDRF_NOTIFYITEMDRAW;

		UINT ItemType = (UINT)pcd->nmcd.lItemlParam;
		if( ItemType != _PAGE_BLANK_ITEM )
			return CDRF_DODEFAULT;

		if( pcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			pcd->clrTextBk = TreeView_GetBkColor(m_hWndTree);
			return CDRF_NEWFONT;
		}

		return CDRF_DODEFAULT;
	}

	LRESULT OnNotify(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;
		LRESULT lResult;

		switch( pnmhdr->code )
		{
			case NM_CUSTOMDRAW:
			{
				SetWindowLongPtr(hDlg,DWLP_MSGRESULT,OnCustomDraw((LPNMLVCUSTOMDRAW)pnmhdr));
				return TRUE;
			}
			case TVN_ITEMCHANGED:
			{
				NMTVITEMCHANGE *pnmtvic = (NMTVITEMCHANGE *)pnmhdr;
				if( pnmtvic->uStateNew & TVIS_SELECTED )
				{
					CreatePage( hDlg, (int)pnmtvic->lParam );
				}
				break;
			}
			case NM_SETCURSOR:
			{
				lResult = OnSetCursor((LPNMMOUSE)pnmhdr);
				SetWindowLongPtr(hDlg,DWLP_MSGRESULT,lResult);
				return TRUE;
			}
			case TVN_SELCHANGING:
			{
				lResult = OnSelChanging((LPNMTREEVIEW)pnmhdr);
				SetWindowLongPtr(hDlg,DWLP_MSGRESULT,lResult);
				return TRUE;
			}
		}
		return 0;
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case IDOK:
				if( m_pCtrlPane )
					SendMessage(m_pCtrlPane->GetHwnd(),WM_COMMAND,IDOK,0);
				break;
			case IDCLOSE:
				CloseDialog(IDCLOSE);
				break;
			case IDCANCEL:
				CloseDialog(IDCANCEL);
				break;
		}
		return 0;
	}

	//---------------------------------------------------------------------------
	//
	//  DialogProc()
	//
	//  PURPOSE:
	//
	//---------------------------------------------------------------------------
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch( uMsg )
		{
			case WM_INITDIALOG:
				return OnInitDialog(hDlg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hDlg,wParam,lParam);
			case WM_SIZE:
				return OnSize(hDlg,wParam,lParam);
			case WM_CLOSE:
				return OnClose(hDlg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hDlg,wParam,lParam);
			case WM_NOTIFY:
				return OnNotify(hDlg,wParam,lParam);
		}
		return 0;
	}
};

//----------------------------------------------------------------------------
//
//  LaunchPadDialog()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
LaunchPadDialog(
	HWND hWnd,
	UINT InitialPage, // Reserved
	UINT ToolPadType, // Reserved
	PVOID Reserved    // Reserved
	)
{
	HRESULT hr;
	if( (hr = IsExternalToolsFileExists()) != S_OK )
	{
		MsgBox(hWnd,L"External tools definitions file not found.",L"LaunchPad",MB_OK|MB_ICONSTOP);
		return hr;
	}

	LAUNCHPAD_PARAM *pParam = new LAUNCHPAD_PARAM;
	if( pParam == NULL )
	{
		return E_OUTOFMEMORY;
	}

	ZeroMemory(pParam,sizeof(LAUNCHPAD_PARAM));

	switch( ToolPadType )
	{
		case LPAD_TYPE_ALL:
		{
			pParam->page_count = 0;
			pParam->pages[ pParam->page_count++ ] = _PAGE_APPLAUNCH;
			break;
		}
		default:
		{
			delete pParam;
			return E_INVALIDARG;
		}
	}

#ifndef _WIN64
	PVOID OldValue;
	Wow64DisableWow64FsRedirection(&OldValue);
#endif

	pParam->hWndOwner = hWnd;
	pParam->start_page = InitialPage;

	CLaunchPadWindow *dlg = new CLaunchPadWindow(CDialogWindow::Modal);

	if( dlg )
	{
		dlg->DoModal(hWnd,IDD_LAUNCHPAD_WINDOW,(LPARAM)pParam,_GetResourceInstance());

		delete dlg;
	}

#ifndef _WIN64
	Wow64RevertWow64FsRedirection(OldValue);
#endif

	return S_OK;
}
