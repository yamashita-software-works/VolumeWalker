//****************************************************************************
//*                                                                          *
//*  dialog_recyclebin_files.cpp                                             *
//*                                                                          *
//*  RecycleBin file dialog window.                                          *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2026-06-16 Created                                             *
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
#include "page_shell_recyclebin_files.h"

#define ID_PANE  1

static PCWSTR g_pszTitle =  L"RecycleBin Files";

typedef struct _RECYCLEBIN_FILES_DIALOG_PARAM
{
	HWND hWndDialog;
	HWND hWndHost;
	PWSTR pszPath;
	PWSTR pszVolume;
} RECYCLEBIN_FILES_DIALOG_PARAM;

//////////////////////////////////////////////////////////////////////////////
// Dialog Implementation

struct CRecycleBinFilesDialog : public CDialogWindowEx
{
	HWND m_hWndList;
	CUILayout m_Layout;
	HWND m_hWndFileList;
	CRecycleBinFilesPage *m_pPage;

	CRecycleBinFilesDialog()
	{
		m_hWndList = NULL;
		m_pPage = NULL;
	}
	
	~CRecycleBinFilesDialog()
	{
	}
	
	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		RECYCLEBIN_FILES_DIALOG_PARAM *pdlgParam = (RECYCLEBIN_FILES_DIALOG_PARAM *)lParam;
	
		pdlgParam->hWndDialog = hDlg;

		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		CRecycleBinFilesPage *pobj = new CRecycleBinFilesPage;
		m_hWndFileList = pobj->Create(hDlg,(int)1,0,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);

		SELECT_ITEM sel = {0};
		sel.ViewType  = VOLUME_CONSOLE_SHELL_RECYCLEBIN_FILES;
		sel.pszVolume = NULL;
		sel.pszCurDir = NULL;
		sel.pszPath   = pdlgParam->pszPath; // drive path
		sel.pszName   = NULL;
		sel.Flags     = 0;
		sel.Context   = NULL;
		pobj->OnInitPage(&sel,0,nullptr);

		m_pPage = pobj;

		// mapping the pane window to placeholder location.
		{
			RECT rcOptionPane;
			HWND hwndPlaceHolder = GetDlgItem(hDlg,IDC_PLACEHOLDER);
			GetWindowRect(hwndPlaceHolder,&rcOptionPane);
			MapWindowPoints(NULL,hDlg,(LPPOINT)&rcOptionPane,2);
			HWND hwndPrev = GetWindow(hwndPlaceHolder,GW_HWNDPREV);
			DestroyWindow(hwndPlaceHolder);
			SetWindowPos(m_hWndFileList,hwndPrev,rcOptionPane.left,rcOptionPane.top, 
				_RECT_WIDTH(rcOptionPane),_RECT_HIGHT(rcOptionPane),SWP_SHOWWINDOW);
		}

		//
		// Initialize Layout
		//
		m_Layout.Initialize(hDlg);			
		m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_BOTTOMRIGHT,ID_PANE,FALSE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMRIGHT,CUILayout::AP_BOTTOMRIGHT,IDCLOSE,FALSE);

		DWORD dw = GetWindowLong(hDlg,GWL_EXSTYLE);
		dw |= WS_EX_DLGMODALFRAME;
		SetWindowLong(hDlg,GWL_EXSTYLE,dw);
		if( (HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0) == NULL )
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)NULL);

		_CenterWindow( hDlg, pdlgParam->hWndHost );

		RECT rcList;
		GetWindowRect(m_hWndList,&rcList);
		MapWindowPoints(NULL,hDlg,(LPPOINT)&rcList,2);
		CDialogWindowEx::m_cyHeaderHight = rcList.top;

		CDialogWindowEx::OnInitDialog(hDlg);

		return FALSE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		CDialogWindowEx::OnDestroy(hDlg);

		RECYCLEBIN_FILES_DIALOG_PARAM *pdlgParam = (RECYCLEBIN_FILES_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		HFONT hFont = (HFONT)SendMessage(m_hWndList,WM_GETFONT,0,0);
		DeleteObject(hFont);

		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		RECYCLEBIN_FILES_DIALOG_PARAM *pdlgParam = (RECYCLEBIN_FILES_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		return 0;
	}
	
	LRESULT OnSize(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		m_Layout.AdjustControls(cx,cy);
		return 0;
	}

	VOID OnOK(HWND hDlg)
	{
		RECYCLEBIN_FILES_DIALOG_PARAM *pdlgParam = (RECYCLEBIN_FILES_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDOK);
	}

	VOID OnCancel(HWND hDlg)
	{
		RECYCLEBIN_FILES_DIALOG_PARAM *pdlgParam = (RECYCLEBIN_FILES_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCANCEL);
	}

	VOID OnCmdClose(HWND hDlg)
	{
		RECYCLEBIN_FILES_DIALOG_PARAM *pdlgParam = (RECYCLEBIN_FILES_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCLOSE);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		UINT uCmdId = LOWORD(wParam);

		m_pPage->InvokeCommand(uCmdId);

		switch( uCmdId )
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
		}
		return CDialogWindowEx::DlgProc(hDlg,uMsg,wParam,lParam);
	}
};

//---------------------------------------------------------------------------
//
//  RecycleBinFilesDialog()
//
//  PURPOSE: 
//      Recycle bin files Dialog.
//
//  PARAMETERS:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
RecycleBinFilesDialog(
	HWND hWnd,
	PCWSTR pszDriveName,
	PCWSTR pszVolumeName, // unuse,reserved
	PCWSTR pszDeviceName, // unuse
	DWORD dwFlags
	)
{
	HRESULT hr;

	CRecycleBinFilesDialog *dlg = new CRecycleBinFilesDialog;
	if( dlg )
	{
		RECYCLEBIN_FILES_DIALOG_PARAM param = {};
		FILEITEMEX fiex = {};

		GetFileAttributes_W(NULL,pszDriveName,&fiex.FileAttributes);

		param.hWndHost  = hWnd;
		param.pszPath   = (PWSTR)pszDriveName;
		param.pszVolume = (PWSTR)pszVolumeName;

		if( dlg->DoModal(hWnd,IDD_FILE_RECYCLE_BIN_FILES,(LPARAM)&param,_GetResourceInstance()) == IDOK )
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

	return hr;
}
