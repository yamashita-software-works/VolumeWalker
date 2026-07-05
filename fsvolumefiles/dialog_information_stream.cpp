//****************************************************************************
//*                                                                          *
//*  dialog_information_stream.cpp                                           *
//*                                                                          *
//*  File stream information dialog window.                                  *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2026-06-26 Created                                             *
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
#include "pane_clusterinfo.h"
#include "pane_streaminfo.h"

#define ID_PANE  1

static PCWSTR g_pszTitle =  L"Stream Information";

typedef struct _CI_DIALOG_PARAM
{
	HWND hWndDialog;
	HWND hWndHost;
	PWSTR pszPath;
	FILEITEMEX *pFI;
} STMI_DIALOG_PARAM;

//////////////////////////////////////////////////////////////////////////////
// Dialog Implementation

struct CStreamInformationDialog : public CDialogWindowEx
{
	HWND m_hWndList;
	PCWSTR m_pszFileName;
	PCWSTR m_pszPath;
	CUILayout m_Layout;
	HWND m_hWndStreamCluster;

	CStreamInformationDialog()
	{
		m_hWndList = NULL;
		m_pszFileName = NULL;
		m_pszPath = NULL;
	}
	
	~CStreamInformationDialog()
	{
	}
	
	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		STMI_DIALOG_PARAM *pdlgParam = (STMI_DIALOG_PARAM *)lParam;
	
		pdlgParam->hWndDialog = hDlg;

		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		CStreamInformationWindow *pStmCls = new CStreamInformationWindow;
		m_hWndStreamCluster = pStmCls->Create(m_hWnd,ID_PANE,L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);

		// mapping the pane window to placeholder location.
		{
			RECT rcOptionPane;
			HWND hwndPlaceHolder = GetDlgItem(hDlg,IDC_PLACEHOLDER);
			GetWindowRect(hwndPlaceHolder,&rcOptionPane);
			MapWindowPoints(NULL,hDlg,(LPPOINT)&rcOptionPane,2);
			HWND hwndPrev = GetWindow(hwndPlaceHolder,GW_HWNDPREV);
			DestroyWindow(hwndPlaceHolder);
			SetWindowPos(m_hWndStreamCluster,hwndPrev,rcOptionPane.left,rcOptionPane.top, 
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

		SendMessage(m_hWndStreamCluster,PPM_SETPATH,(WPARAM)pdlgParam->pFI,(LPARAM)pdlgParam->pszPath);

		_SetWindowTitleFileName( m_hWnd, FindFileName_W(pdlgParam->pszPath) );

		return FALSE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		CDialogWindowEx::OnDestroy(hDlg);

		STMI_DIALOG_PARAM *pdlgParam = (STMI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		HFONT hFont = (HFONT)SendMessage(m_hWndList,WM_GETFONT,0,0);
		DeleteObject(hFont);

		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		STMI_DIALOG_PARAM *pdlgParam = (STMI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
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
		STMI_DIALOG_PARAM *pdlgParam = (STMI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDOK);
	}

	VOID OnCancel(HWND hDlg)
	{
		STMI_DIALOG_PARAM *pdlgParam = (STMI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCANCEL);
	}

	VOID OnCmdClose(HWND hDlg)
	{
		STMI_DIALOG_PARAM *pdlgParam = (STMI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCLOSE);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		UINT uCmdId = LOWORD(wParam);
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
//  StreamInformationDialog()
//
//  PURPOSE: 
//      File Cluster Information Dialog.
//
//  PARAMETERS:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
StreamInformationDialog(
	HWND hWnd,
	PCWSTR pszFilePath
	)
{
	HRESULT hr;

	CStreamInformationDialog *dlg = new CStreamInformationDialog;
	if( dlg )
	{
		STMI_DIALOG_PARAM param = {};
		FILEITEMEX fiex = {};

		GetFileAttributes_W(NULL,pszFilePath,&fiex.FileAttributes);

		param.hWndHost = hWnd;
		param.pszPath = (PWSTR)pszFilePath;
		param.pFI = &fiex;

		if( dlg->DoModal(hWnd,IDD_FILE_STREAM_INFORMATION,(LPARAM)&param,_GetResourceInstance()) == IDOK )
		{
			;
		}

		delete dlg;

		hr = S_OK;
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	return hr;
}
