//****************************************************************************
//*                                                                          *
//*  dialog_altstream.cpp                                                    *
//*                                                                          *
//*  Stream select dialog                                                    *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2025-12-06 Created                                             *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "stdafx.h"
#include "resource.h"
#include "uilayout.h"
#include "fsvolumefilelist.h"
#include "ntobjecthelp.h"

typedef struct _STREAM_DIALOG_PARAM
{
	HWND hWndDialog;
	HWND hWndList;
	HWND hWndHost;
	VFS_FILE_STREAM_INFORMATION *pStreamInformation;
	ULONG StreamCount;
} STREAM_DIALOG_PARAM;

typedef struct _STREAM_LIST_ITEM
{
	const STREAM_DIALOG_PARAM *pdlgParam;
	VFS_FILE_STREAM_INFORMATION *pStreamInformation;
	PWSTR pszDisplayName;
} STREAM_LIST_ITEM;

//////////////////////////////////////////////////////////////////////////////
// Dialog Implementation

struct CStreamNameSelectDialog : public CDialogWindowEx
{
	HWND m_hWndList;
	PCWSTR m_pszFileName;
	DWORD m_dwFlags;

	CUILayout m_Layout;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	typedef struct _SORT_PARAM {
		UINT id;
		INT direction; // must 1 or -1, do not use 0
	} SORT_PARAM;

	VFS_FILE_STREAM_INFORMATION *m_pVSI;

	CStreamNameSelectDialog()
	{
		m_hWndList = NULL;
		m_pVSI = NULL;
		m_pszFileName = NULL;
		m_dwFlags = 0;
	}
	
	~CStreamNameSelectDialog()
	{
	}

	void UpdateOKButton()
	{	
		EnableWindow(GetDlgItem(m_hWnd,IDOK),
					ListView_GetSelectedCount(m_hWndList));
	}

	int InsertItem(HWND hWndList,STREAM_DIALOG_PARAM *pdlgParam,int iItem,int iType,VFS_FILE_STREAM_INFORMATION *pSI,PVOID)
	{
		STREAM_LIST_ITEM *pItem = new STREAM_LIST_ITEM;
		if( pItem == NULL )
			return -1;

		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);
	
		pItem->pdlgParam = pdlgParam;
		pItem->pStreamInformation = pSI;

		WCHAR szName[MAX_PATH];
		StringCchCopy(szName,MAX_PATH,pSI->StreamName);

		if( wcscmp(szName,L"::$DATA") == 0 )
		{
			pItem->pszDisplayName = _MemAllocString( L"(Default Data Stream)" );
		}
		else
		{
			pItem->pszDisplayName = &szName[1];
			WCHAR *pSep = wcschr(pItem->pszDisplayName,L':');
			if( pSep != NULL )
			{
				*pSep = L'\0';
				pItem->pszDisplayName = _MemAllocString(pItem->pszDisplayName);
			}
			else
			{
				pItem->pszDisplayName = _MemAllocString( L"(Invalid Name)" );
			}
		}

		LVITEM lvi = {0};
		lvi.mask    = LVIF_PARAM|LVIF_TEXT|LVIF_IMAGE;
		lvi.pszText = (PWSTR)LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;
		lvi.iItem   = iItem;

		iItem = ListView_InsertItem(hWndList,&lvi);
	
		return iItem;
	}

	VOID FillListItems(HWND hWndList,STREAM_DIALOG_PARAM *pdlgParam,PVOID)
	{
		ULONG index;
		for(index = 0; index < pdlgParam->StreamCount; index++)
		{
			InsertItem(hWndList,pdlgParam,index,0,&pdlgParam->pStreamInformation[index],0);
		}
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		STREAM_DIALOG_PARAM *pdlgParam = (STREAM_DIALOG_PARAM *)lParam;
	
		pdlgParam->hWndDialog = hDlg;

		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		//
		// Initialize Layout
		//
		m_Layout.Initialize(hDlg);			
		m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_BOTTOMRIGHT,IDC_LIST,FALSE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMRIGHT,CUILayout::AP_BOTTOMRIGHT,IDOK,FALSE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMRIGHT,CUILayout::AP_BOTTOMRIGHT,IDCLOSE,FALSE);

		DWORD dw = GetWindowLong(hDlg,GWL_EXSTYLE);
		dw |= WS_EX_DLGMODALFRAME;
		SetWindowLong(hDlg,GWL_EXSTYLE,dw);
		if( (HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0) == NULL )
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)NULL);

		_CenterWindow( hDlg, pdlgParam->hWndHost );

		//
		// Initialize Target File ListView
		//
		m_hWndList = GetDlgItem(hDlg,IDC_LIST);

		ASSERT( (GetWindowLong(m_hWndList, GWL_STYLE) & LVS_SHAREIMAGELISTS) );

		pdlgParam->hWndList = m_hWndList;

		/* todo: optional */
		{DWORD dw = GetWindowLong(m_hWndList,GWL_STYLE);
		dw |= LVS_SINGLESEL;
		SetWindowLong(m_hWndList,GWL_STYLE,dw);}
		/* todo: optional */

		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)GetGlobalFont(hDlg),0);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_INFOTIP|LVS_EX_LABELTIP);
	
		_EnableVisualThemeStyle(m_hWndList);

		RECT rc;
		GetClientRect(m_hWndList,&rc);
		int cx = _RECT_WIDTH(rc);
	
		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH;

		struct {
			PWSTR Text;
			int fmt;
		} Columns[] = {
			{L"Name",            LVCFMT_LEFT},
			{L"Size",            LVCFMT_RIGHT},
			{L"Allocation Size", LVCFMT_RIGHT},
		};

		{
			lvc.fmt = LVCFMT_LEFT;
			lvc.pszText = L"";
			lvc.cx = 0;
			lvc.iSubItem = 0;
			lvc.iOrder = 0;
			ListView_InsertColumn(m_hWndList,0,&lvc);
		}

		for(int i = 0; i < ARRAYSIZE(Columns); i++)
		{
			lvc.fmt = Columns[i].fmt;
			lvc.pszText = Columns[i].Text;
			lvc.cx = (int)((double)cx * 0.3);
			lvc.iSubItem = i+1;
			ListView_InsertColumn(m_hWndList,i+1,&lvc);
		}

		// If a column is added to a list-view control with index 0 (the leftmost column),
		// it is always LVCFMT_LEFT. Setting other flags on column 0 does not override that alignment. 
		// Therefore if you keep inserting columns with index 0, the text in all columns are left-aligned. 
		// If you want the first column to be right-aligned or centered you can make a dummy column, 
		// then insert one or more columns with index 1 or higher and specify the alignment you require.
		// Finally delete the dummy column
		ListView_DeleteColumn(m_hWndList,0);

		FillListItems(m_hWndList,pdlgParam,NULL);

		ListView_SetColumnWidth(m_hWndList,0,DPI_SIZE_CX(360));
		ListView_SetColumnWidth(m_hWndList,1,DPI_SIZE_CX(160));
		ListView_SetColumnWidth(m_hWndList,2,DPI_SIZE_CX(160));

		m_Sort.CurrentSubItem = -1;
		m_Sort.Direction = -1;

		UpdateOKButton();

		SetDlgItemText(hDlg,IDC_TEXT,m_pszFileName);

		if( m_dwFlags & FSSDF_NOOPENBUTTON )
		{
			ShowWindow(GetDlgItem(hDlg,IDOK),SW_HIDE);
			SetDlgItemText(hDlg,IDC_DESCRIPTION,L"Alternate Stream Name and Size.");
			SetWindowText(hDlg,L"Stream Information");
		}

		RECT rcList;
		GetWindowRect(m_hWndList,&rcList);
		MapWindowPoints(NULL,hDlg,(LPPOINT)&rcList,2);
		CDialogWindowEx::m_cyHeaderHight = rcList.top;

		CDialogWindowEx::OnInitDialog(hDlg);

		return FALSE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		STREAM_DIALOG_PARAM *pdlgParam = (STREAM_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		CDialogWindowEx::OnDestory(hDlg);

		HFONT hFont = (HFONT)SendMessage(m_hWndList,WM_GETFONT,0,0);
		DeleteObject(hFont);

		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		STREAM_DIALOG_PARAM *pdlgParam = (STREAM_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		return 0;
	}
	
	LRESULT OnSize(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		m_Layout.AdjustControls(cx,cy);
		return 0;
	}

	LRESULT OnGetDispInfo(NMLVDISPINFO* pnmlvdi)
	{
		if( pnmlvdi->item.mask & LVIF_IMAGE )
		{
			; // reserved
		}
		if( pnmlvdi->item.mask & LVIF_TEXT )
		{
			STREAM_LIST_ITEM *pItem = (STREAM_LIST_ITEM *)pnmlvdi->item.lParam;
	
			*pnmlvdi->item.pszText = L'\0';

			switch( pnmlvdi->item.iSubItem )
			{
				case 0:
					StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s",pItem->pszDisplayName);
					break;
				case 1:
					_CommaFormatString(pItem->pStreamInformation->StreamSize.QuadPart,pnmlvdi->item.pszText);
					break;
				case 2:
					_CommaFormatString(pItem->pStreamInformation->StreamAllocationSize.QuadPart,pnmlvdi->item.pszText);
					break;
			}
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMLISTVIEW* pnmlv)
	{
		STREAM_LIST_ITEM *p = (STREAM_LIST_ITEM*)pnmlv->lParam;
		_SafeMemFree(p->pszDisplayName);
		delete p;
		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;
		if( pnmia->iItem != -1 )
		{
			STREAM_LIST_ITEM *pItem = (STREAM_LIST_ITEM *)ListViewEx_GetItemData(m_hWndList,pnmia->iItem);
			OnOK(m_hWnd);
		}
		return 0;
	}

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		VFS_FILE_STREAM_INFORMATION *pSI = (VFS_FILE_STREAM_INFORMATION *)pnmlv->lParam;

		DoSort(pnmlv->iSubItem,TRUE);

		return 0;
	}

	void DoSort(int iSubItem=-1,BOOL bToggle=FALSE)
	{
		if( m_Sort.CurrentSubItem != -1 )  //  previous sort column
			ListViewEx_SetHeaderArrow(m_hWndList,m_Sort.CurrentSubItem,0); // clear previous column header mark

		if( bToggle )
		{
			if( m_Sort.CurrentSubItem != iSubItem )
				m_Sort.Direction = 1;     // current column changed: new current column always ascend sort.
			else
				m_Sort.Direction *= -1;   // toggle.
		}

		SORT_PARAM op = {0};
		op.id              = iSubItem;
		op.direction       = m_Sort.Direction; // must 1 or -1, do not use 0
		ListView_SortItems(m_hWndList,&CStreamNameSelectDialog::CompareProc,&op);

		ListViewEx_SetHeaderArrow(m_hWndList,iSubItem,m_Sort.Direction);

		m_Sort.CurrentSubItem = iSubItem;
	}

	static int CALLBACK CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		STREAM_LIST_ITEM *pItem1 = (STREAM_LIST_ITEM *)lParam1;
		STREAM_LIST_ITEM *pItem2 = (STREAM_LIST_ITEM *)lParam2;

		SORT_PARAM *psp = (SORT_PARAM *)lParamSort;

		int iRet = 0;

		switch( psp->id )
		{
			case 0:
				iRet = wcscmp(pItem1->pStreamInformation->StreamName,pItem2->pStreamInformation->StreamName);
				break;
			case 1:
				iRet = _COMP(pItem1->pStreamInformation->StreamSize.QuadPart,pItem2->pStreamInformation->StreamSize.QuadPart);
				break;
			case 2:
				iRet = _COMP(pItem1->pStreamInformation->StreamAllocationSize.QuadPart,pItem2->pStreamInformation->StreamAllocationSize.QuadPart);
				break;
		}

		iRet *= psp->direction; 

		return iRet;
	}

	LRESULT OnNotify(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		switch( ((NMHDR *)lParam)->code )
		{
			case LVN_GETDISPINFO:
				OnGetDispInfo((NMLVDISPINFO*)lParam);
				return TRUE;
			case LVN_DELETEITEM:
				OnDeleteItem((NMLISTVIEW*)lParam);
				return TRUE;
			case LVN_ITEMACTIVATE:
				return OnItemActivate((NMHDR*)lParam);
			case LVN_COLUMNCLICK:
				return OnColumnClick((NMHDR*)lParam);
			case LVN_ITEMCHANGED:
				UpdateOKButton();
				return 0;
		}
		return 0;
	}

	LRESULT OnContextMenu(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		HMENU hMenu = CreatePopupMenu();

		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Line Text");

		HMENU hSubMenu = CreatePopupMenu();
		{
			PWSTR *pColumns;
			int cColumns;
			ListViewEx_GetTextColumns(m_hWndList,&pColumns,&cColumns,NULL);

			for(int i = 0; i < cColumns; i++)
			{
				AppendMenu(hSubMenu,MF_STRING,ID_EDIT_COPY_COUMN_FIRST+i,pColumns[i]);
			}
			CoTaskMemFree(pColumns);
		}

		AppendMenu(hMenu,MF_POPUP,(UINT_PTR)hSubMenu,L"Copy Co&lumn Text");

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,(HWND)wParam,hMenu,pt,0);

		DestroyMenu(hMenu);

		return 0;
	}

	VOID OnOK(HWND hDlg)
	{
		STREAM_DIALOG_PARAM *pdlgParam = (STREAM_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return ;

		if( !(m_dwFlags & FSSDF_NOOPENBUTTON) )
		{
			STREAM_LIST_ITEM *pItem = (STREAM_LIST_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

			m_pVSI = pItem->pStreamInformation;

			EndDialog(hDlg,IDOK);
		}
	}

	VOID OnCancel(HWND hDlg)
	{
		STREAM_DIALOG_PARAM *pdlgParam = (STREAM_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCANCEL);
	}

	VOID OnCmdClose(HWND hDlg)
	{
		STREAM_DIALOG_PARAM *pdlgParam = (STREAM_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCLOSE);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case IDOK:
				OnOK(hDlg);
				break;
			case IDCANCEL:
				OnCancel(hDlg);
				break;
			case IDCLOSE:
				OnCmdClose(hDlg);
				break;
			case ID_EDIT_COPY:
				SetClipboardTextFromListView(m_hWndList,SCTEXT_UNICODE);
				break;
			default:
				SetClipboardTextFromListViewColumn(m_hWndList,SCTEXT_FORMAT_SELECTONLY,(LOWORD(wParam)-ID_EDIT_COPY_COUMN_FIRST));
				break;
		}
		return 0;
	}

	virtual INT_PTR OnCtlColor(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_CTLCOLORBTN:
			case WM_CTLCOLORDLG:
				return (INT_PTR)m_hbrBackground;
			case WM_CTLCOLORSTATIC:
				SetBkMode((HDC)wParam,TRANSPARENT);
				return (INT_PTR)m_hbrHeader;
		}
		return 0;
	}

	//---------------------------------------------------------------------------
	//
	//  DialogProc()
	//
	//  PURPOSE: Dialog Window Procedure
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
			case WM_CONTEXTMENU:
				return OnContextMenu(hDlg,wParam,lParam);
		}
		return CDialogWindowEx::DlgProc(hDlg,uMsg,wParam,lParam);
	}
};

//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//  FileSelectStreamDialog()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
FileSelectStreamDialog(
	HWND hWnd,
	PCWSTR pszFilePath,
	PWSTR *ppwszFileStreamName,
	LPDWORD pcchFileStreamName,
	DWORD dwFlags
	)
{
	HRESULT hr;

	VFS_FILE_STREAM_INFORMATION *pStmNames = NULL;
	int cStmNames = 0;

	if( pszFilePath == NULL )
		return E_INVALIDARG;

	hr = GetAlternateStream( pszFilePath, &pStmNames, &cStmNames );

	if( ERROR_NO_MORE_ITEMS == HRESULT_CODE(hr) )
	{
		return S_SSD_NO_STREAM;
	}

	if( FAILED(hr) )
	{
		return hr;
	}

	if( cStmNames == 0 )
	{
		return S_SSD_NO_STREAM;
	}

	if( cStmNames == 1 && wcscmp(pStmNames->StreamName,L"::$DATA") == 0 )
	{
		return S_SSD_DEFAULT_STREAM_ONLY;
	}

	STREAM_DIALOG_PARAM *pParam = new STREAM_DIALOG_PARAM;
	if( pParam == NULL )
		return E_OUTOFMEMORY;

	ZeroMemory(pParam,sizeof(STREAM_DIALOG_PARAM));

	pParam->hWndHost = hWnd;
	pParam->pStreamInformation = pStmNames;
	pParam->StreamCount = cStmNames;

	CStreamNameSelectDialog *dlg = new CStreamNameSelectDialog;
	if( dlg ) 
	{
		dlg->m_pszFileName = FindFileName_W( pszFilePath );
		dlg->m_dwFlags = dwFlags;

		if( dlg->DoModal(hWnd,IDD_FILE_STREAM_SELECT,(LPARAM)pParam,_GetResourceInstance()) == IDOK )
		{
			if( ppwszFileStreamName )
			{
				SIZE_T cch = 0;
				PWSTR pName = dlg->m_pVSI->StreamName;
	
				if( dwFlags & FSSDF_MAKEFULLPATH )
				{
					cch = wcslen( pName );
					cch += wcslen( pszFilePath );
					cch += 1; // terminate null
	
					*ppwszFileStreamName = (PWSTR)LocalAlloc(LPTR,cch*sizeof(WCHAR));
	
					if( *ppwszFileStreamName )
					{
						hr = StringCchCopy(*ppwszFileStreamName,cch,pszFilePath);
						if( SUCCEEDED(hr) )
							hr = StringCchCat(*ppwszFileStreamName,cch,pName);
					}
					else
					{
						hr = E_OUTOFMEMORY;
						cch = 0;
					}
				}
				else if( dwFlags & FSSDF_FILENAMEWITHSTREAMNAME )
				{
					PCWSTR pFileName = FindFileName_W( pszFilePath );
	
					cch = wcslen(pFileName) + 1;
					cch += wcslen( pszFilePath );
					cch += 1;
	
					*ppwszFileStreamName = (PWSTR)LocalAlloc(LPTR,cch*sizeof(WCHAR));
	
					if( *ppwszFileStreamName )
					{
						hr = StringCchCopy(*ppwszFileStreamName,cch,pFileName);
						if( SUCCEEDED(hr) )
							hr = StringCchCat(*ppwszFileStreamName,cch,pName);
					}
					else
					{
						hr = E_OUTOFMEMORY;
						cch = 0;
					}
				}
				else
				{
					*ppwszFileStreamName = StrDup(pName);
	
					if( *ppwszFileStreamName )
					{
						if( pcchFileStreamName )
							cch = wcslen(*ppwszFileStreamName);
						hr = S_OK;
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
	
				if( pcchFileStreamName )
					*pcchFileStreamName = (DWORD)cch;
			}
			else
			{
				hr = S_OK;
			}
		}
		else
		{
			hr = S_FALSE;
		}

		delete dlg;
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	delete pParam;

	CoTaskMemFree(pStmNames);

	return hr;
}
