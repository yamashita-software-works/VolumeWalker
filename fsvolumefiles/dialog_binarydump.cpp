//****************************************************************************
//*                                                                          *
//*  dialog_binarydump.cpp                                                   *
//*                                                                          *
//*  Simple paged binary dump dialog                                         *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2026-01-27 Created                                             *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "stdafx.h"
#include "resource.h"
#include "uilayout.h"
#include "bindump.h"
#include "bindump_ctrlpanel.h"

static PCWSTR g_pszTitle = L"Simple Binary Dump";

typedef struct _PAGED_BINARY_DUMP_DIALOG_PARAM
{
	HWND hWndDialog;
	HWND hWndList;
	HWND hWndHost;
} PAGED_BINARY_DUMP_DIALOG_PARAM;

//////////////////////////////////////////////////////////////////////////////
// Dialog Implementation

struct CSimpleDumpDialog : public CDialogWindowEx
{
	PCWSTR m_pszFilePath;

	CUILayout m_Layout;

	CQuickBinDump m_dump;
	CQuickBinDumpCtrlPanel m_ctrl;

	CSimpleDumpDialog()
	{
		m_pszFilePath = NULL;
	}
	
	~CSimpleDumpDialog()
	{
	}

	VOID FillListItems(HWND hWndList,PAGED_BINARY_DUMP_DIALOG_PARAM *pdlgParam,PVOID)
	{
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		PAGED_BINARY_DUMP_DIALOG_PARAM *pdlgParam = (PAGED_BINARY_DUMP_DIALOG_PARAM *)lParam;
	
		pdlgParam->hWndDialog = hDlg;

		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		//
		// Set file name
		//
		SetWindowText(hDlg,FindFileName_W(m_pszFilePath));

		//
		// Create control panel
		//
		m_ctrl.Create(hDlg,IDD_PAGED_BINARY_DUMP_CTRLPANE,0,_GetResourceInstance());
		ShowWindow(m_ctrl.m_hWnd,SW_SHOW);

		//
		// Create simple binary dump list window
		//
		m_dump.Create(hDlg,NULL,BDF_BYTEWIDTH_16|BDF_ADDR_QWORD|BDF_OFFSETSTR_QWORD,IDC_LIST);

		// mapping the pane window to placeholder location.
		{
			RECT rcOptionPane;
			HWND hwndPlaceHolder = GetDlgItem(hDlg,IDC_PLACEHOLDER);
			GetWindowRect(hwndPlaceHolder,&rcOptionPane);
			MapWindowPoints(NULL,hDlg,(LPPOINT)&rcOptionPane,2);
			HWND hwndPrev = GetWindow(hwndPlaceHolder,GW_HWNDPREV);
			DestroyWindow(hwndPlaceHolder);
			SetWindowPos(m_dump.m_hWnd,hwndPrev,rcOptionPane.left,rcOptionPane.top, 
				_RECT_WIDTH(rcOptionPane),_RECT_HIGHT(rcOptionPane),SWP_SHOWWINDOW);
		}	

		//
		// Initialize Layout
		//
		m_Layout.Initialize(hDlg);			
		m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_BOTTOMRIGHT,IDC_LIST,FALSE);

		// Adjust the dialog box style: has resize border without icon.
		DWORD dw = GetWindowLong(hDlg,GWL_EXSTYLE);
		dw |= WS_EX_DLGMODALFRAME;
		SetWindowLong(hDlg,GWL_EXSTYLE,dw);
		if( (HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0) == NULL )
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)NULL);

		// Remap control possition and size.
		RECT rcList;
		GetWindowRect(m_dump.m_hWnd,&rcList);
		MapWindowPoints(NULL,hDlg,(LPPOINT)&rcList,2);

		// Initialize dialog header height.
		CDialogWindowEx::m_cyHeaderHight = rcList.top;
		CDialogWindowEx::OnInitDialog(hDlg);

		// Adjust the size of the dialog box to match the width of the dump view.
		m_dump.OnInitPage(0,0,0);

		int cxList = m_dump.GetHexDumpWidth();
		int cxCtrl = m_ctrl.GetWidth();

		RECT rcWnd = {};
		GetClientRect(m_hWnd,&rcWnd);
		rcWnd.right = cxList + cxCtrl + GetSystemMetrics(SM_CXVSCROLL);

		AdjustWindowRectEx(
				&rcWnd,
				GetWindowLong(hDlg,GWL_STYLE),
				FALSE,
				GetWindowLong(hDlg,GWL_EXSTYLE));

		SetWindowPos(m_hWnd,NULL,0,0,_RECT_WIDTH(rcWnd),_RECT_HIGHT(rcWnd),SWP_NOMOVE|SWP_NOZORDER);

		RECT rc;
		GetClientRect(hDlg,&rc);
		OnSize(hDlg,0,MAKELPARAM(_RECT_WIDTH(rc),_RECT_HIGHT(rc)));

		_CenterWindow( hDlg, pdlgParam->hWndHost );

		//
		// open and fill data
		//
		m_dump.Open( m_pszFilePath );

		m_ctrl.OnInitData();

		return FALSE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		PAGED_BINARY_DUMP_DIALOG_PARAM *pdlgParam = (PAGED_BINARY_DUMP_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		CDialogWindowEx::OnDestroy(hDlg);

		m_dump.Close();

		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		PAGED_BINARY_DUMP_DIALOG_PARAM *pdlgParam = (PAGED_BINARY_DUMP_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCLOSE);
		return 0;
	}
	
	LRESULT OnSize(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);

		int cxCtrl = m_ctrl.GetWidth();

		SetWindowPos(m_dump.m_hWnd,NULL,0,0,cx-cxCtrl,cy,SWP_NOZORDER);

		SetWindowPos(m_ctrl.m_hWnd,NULL,cx-cxCtrl,0,cxCtrl,cy,SWP_NOZORDER);

		return 0;
	}

	LRESULT OnNotify(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;
		switch( pnmhdr->code )
		{
			case BDN_UPDATE:
				return m_ctrl.OnBinDumpUpdate((NMBINDUMP*)pnmhdr);
			case BDN_CURSORMOVED:
				return m_ctrl.OnBinDumpCursorMoved((NMBINDUMP*)pnmhdr);
			case BDN_SETERROR:
				return m_ctrl.OnBinDumpSetError((NMBINDUMP*)pnmhdr);
		}
		return 0;
	}

	LRESULT OnContextMenu(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		return 0;
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case IDOK:
				m_dump.GotoOffset( m_ctrl.GetOffset() );
				break;
			case ID_GOTO:
				m_dump.GotoOffset( m_ctrl.GetOffset() );
				break;
			case IDCANCEL:
				OnClose(hDlg,0,0);
				break;
			default:
				if( IsEnableCommand(LOWORD(wParam)) )
					SendMessage(m_dump.m_hWnd,WM_COMMAND,wParam,lParam);
				break;
		}
		return 0;
	}

	BOOL IsEnableCommand(UINT id)
	{
		return ( (m_ctrl.GetToolbarButtonState(id) & TBSTATE_ENABLED) == TBSTATE_ENABLED );
	}

	//---------------------------------------------------------------------------
	//
	//  DlgProc()
	//
	//  PURPOSE: Dialog Window Procedure
	//
	//---------------------------------------------------------------------------
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch( uMsg )
		{
			case WM_NCACTIVATE:
				CDialogWindowEx::DlgProc(hDlg,uMsg,wParam,lParam);
				m_ctrl.OnUpdateColor((BOOL)wParam);
				RedrawWindow(m_ctrl.m_hWnd,0,0,RDW_INVALIDATE|RDW_ERASE|RDW_UPDATENOW|RDW_ERASENOW|RDW_ALLCHILDREN);
				return 0;
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

//----------------------------------------------------------------------------
//
//  QuickBinaryDumpDialog()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
QuickBinaryDumpDialog(
	HWND hWnd,
	PCWSTR pszStreamName,
	PBYTE pb,
	ULONG cb,
	DWORD /*dwFlags*/
	)
{
	HRESULT hr;

	// Precheck the stream data state.
	{
		HANDLE hFile;
		NTSTATUS Status;
		LARGE_INTEGER Size={0},AllocationSize={0};
		NT_FILE_BASIC_INFORMATION ntfbi = {};

		Status = OpenFileEx_W(&hFile,pszStreamName,GENERIC_READ|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_SYNCHRONOUS_IO_NONALERT);

		if( Status == STATUS_SUCCESS )
		{
			if( QueryFileBasicInformation(hFile,&ntfbi) == STATUS_SUCCESS )
			{
				;
			}

			if( GetFileSizeByHandle(hFile,&Size,&AllocationSize) == STATUS_SUCCESS )
			{
				;
			}

			CloseHandle(hFile);
		}

		if( Status != STATUS_SUCCESS )
		{
			WCHAR szMessageText[MAX_PATH];
			PWSTR pSystemErrorMessage = NULL;
			_GetSystemErrorMessage(Status,&pSystemErrorMessage);

			if( pSystemErrorMessage )
				StringCchCopy(szMessageText,ARRAYSIZE(szMessageText),pSystemErrorMessage);
			else
				StringCchPrintf(szMessageText,ARRAYSIZE(szMessageText),L"Error occurred : 0x%08X",Status);

			MsgBox(hWnd,szMessageText,g_pszTitle,MB_OK|MB_ICONSTOP);

			_FreeSystemErrorMessage(pSystemErrorMessage);

			return E_FAIL;
		}
		else
		{
			if( AllocationSize.QuadPart == 0 )
			{
				MsgBox(hWnd,L"The data is empty.",g_pszTitle,MB_OK|MB_ICONINFORMATION);
				return E_FAIL;
			}
		}

		// ToDo: Currently, other errors are pass through and displayed in the dump window.
	}

	PAGED_BINARY_DUMP_DIALOG_PARAM *pParam = new PAGED_BINARY_DUMP_DIALOG_PARAM;
	if( pParam == NULL )
		return E_OUTOFMEMORY;

	ZeroMemory(pParam,sizeof(PAGED_BINARY_DUMP_DIALOG_PARAM));

	pParam->hWndHost = hWnd;

	CSimpleDumpDialog *dlg = new CSimpleDumpDialog;
	if( dlg ) 
	{
		dlg->m_pszFilePath = pszStreamName;

		if( dlg->DoModal(hWnd,IDD_PAGED_BINARY_DUMP,(LPARAM)pParam,_GetResourceInstance()) == IDOK )
		{
			hr = S_OK;
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

	return hr;
}
