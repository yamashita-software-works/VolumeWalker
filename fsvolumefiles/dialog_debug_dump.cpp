//****************************************************************************
//*                                                                          *
//*  dialog_debug_dump.cpp                                                   *
//*                                                                          *
//*  General Debug Dump Box Dialog.                                          *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2026-08-31 Created                                             *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "stdafx.h"
#include "resource.h"
#include "uilayout.h"
#include "dialog_debug_dump.h"

typedef struct _NTFSRECORD_DIALOG_PARAM
{
	HWND hWndDialog;
	HWND hWndHost;
	PWSTR pszTitle;
	PWSTR pszText;
	ULONG Flags;
	PVOID Callback;
	ULONG_PTR Context;
} NTFSRECORD_DIALOG_PARAM;

//////////////////////////////////////////////////////////////////////////////
// Dialog Implementation

struct CDebugDumpDialog : public CDialogWindowEx, public IDebugText
{
	CUILayout m_Layout;
	HFONT     m_hFont;
	HWND      m_hWndEdit;

	CDebugDumpDialog()
	{
		m_hWndEdit = NULL;
	}
	
	~CDebugDumpDialog()
	{
	}
	
	virtual HRESULT SetText(PCWSTR pszText)
	{
		SetWindowText(m_hWndEdit,pszText);
		return S_OK;
	}

	virtual HRESULT AddText(PCWSTR pszText)
	{
		SendMessage(m_hWndEdit,EM_SETSEL,(WPARAM)-1,(LPARAM)-1);
		SendMessage(m_hWndEdit,EM_REPLACESEL,0,(LPARAM)pszText);
		return S_OK;
	}

	VOID InitTextBox(NTFSRECORD_DIALOG_PARAM *pdlgParam)
	{
		if( pdlgParam->Callback == NULL )
			return;

		if( pdlgParam->Flags & DDDF_USECALLBACKINTERFACE )
		{
			IDebugTextCallback *pCallback = (IDebugTextCallback *)pdlgParam->Callback;
			pCallback->Initialize( static_cast<IDebugText*>(this) );
		}
		else
		{
			if( pdlgParam->Flags & DDDF_USETEXTINTERFACE )
			{
				PFNDEBUGTEXTCALLBACK pfnCallback = (PFNDEBUGTEXTCALLBACK)pdlgParam->Callback;
				pfnCallback(DebugDumpPhase_Initialize,DebugTextInterface,static_cast<IDebugText*>(this),pdlgParam->Context);
			}
			else
			{
				; // not supported
			}
		}
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		NTFSRECORD_DIALOG_PARAM *pdlgParam = (NTFSRECORD_DIALOG_PARAM *)lParam;
	
		pdlgParam->hWndDialog = hDlg;

		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		m_hWndEdit = GetDlgItem(hDlg,IDC_EDIT);

		m_hFont = GetGlobalFont(m_hWnd);
		SendMessage(m_hWndEdit,WM_SETFONT,(WPARAM)m_hFont,0);

		if( pdlgParam->pszTitle )
		{
			SetWindowText(hDlg,pdlgParam->pszTitle);
		}

		//
		// Initialize dialog layout
		//
		m_Layout.Initialize(hDlg);			
		m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_BOTTOMRIGHT,IDC_EDIT,FALSE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMRIGHT,CUILayout::AP_BOTTOMRIGHT,IDCLOSE,FALSE);

		//
		// Remove icon button on the title bar.
		//
		DWORD dw = GetWindowLong(hDlg,GWL_EXSTYLE);
		dw |= WS_EX_DLGMODALFRAME;
		SetWindowLong(hDlg,GWL_EXSTYLE,dw);
		if( (HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0) == NULL )
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)NULL);

		//
		// Setting the size.
		//
		if( pdlgParam->Flags & DDDF_SETDIALOGSIZE )
		{
			RECT rc;
			GetWindowRect(pdlgParam->hWndHost,&rc);
			SetWindowPos(hDlg,NULL,0,0,_RECT_WIDTH(rc),_RECT_HIGHT(rc),SWP_NOMOVE|SWP_NOZORDER);
		}

		_CenterWindow( hDlg, pdlgParam->hWndHost );

		//
		// Initialize text box
		//
		InitTextBox(pdlgParam);

		//
		// Initialize base class
		//
		CDialogWindowEx::OnInitDialog(hDlg);

		return FALSE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		CDialogWindowEx::OnDestroy(hDlg);

		NTFSRECORD_DIALOG_PARAM *pdlgParam = (NTFSRECORD_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		HFONT hFont = (HFONT)SendMessage(m_hWndEdit,WM_GETFONT,0,0);
		DeleteObject(hFont);

		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		NTFSRECORD_DIALOG_PARAM *pdlgParam = (NTFSRECORD_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
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
		NTFSRECORD_DIALOG_PARAM *pdlgParam = (NTFSRECORD_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDOK);
	}

	VOID OnCancel(HWND hDlg)
	{
		NTFSRECORD_DIALOG_PARAM *pdlgParam = (NTFSRECORD_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCANCEL);
	}

	VOID OnCmdClose(HWND hDlg)
	{
		NTFSRECORD_DIALOG_PARAM *pdlgParam = (NTFSRECORD_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCLOSE);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		if( 1 )
		{
			if( HIWORD(wParam) == EN_SETFOCUS )
			{
				SendMessage((HWND)lParam,EM_SETSEL,(WPARAM)-1,(LPARAM)-1);
			}
		}

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
//  DebugDumpDialog()
//
//  PURPOSE: 
//
//  PARAMETERS:
//      hWnd
//          Owner window handle.
//      pszWindowTitle
//          Window title.
//      pszText
//          Reserved.
//      dwFlags
//          Dialog optional flags.
//      Callback
//          Pointer to the callback function for dialog contents text setting.
//      Context
//          Context value for the callback function.
//      Reserved
//          Feature not available.
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
DebugDumpDialog(
	HWND hWnd,
	PCWSTR pszWindowTitle,
	PCWSTR pszText,
	DWORD dwFlags,
	PVOID Callback,
	ULONG_PTR Context,
	PVOID Reserved
	)
{
	HRESULT hr;

	CDebugDumpDialog *dlg = new CDebugDumpDialog;
	if( dlg )
	{
		NTFSRECORD_DIALOG_PARAM param = {};

		param.hWndHost = hWnd;
		param.pszTitle = (PWSTR)pszWindowTitle;
		param.pszText  = (PWSTR)pszText;
		param.Flags    = dwFlags;
		param.Callback = Callback;
		param.Context  = Context;

		dlg->DoModal(hWnd,IDD_DEBUG_DUMP,(LPARAM)&param,_GetResourceInstance());

		delete dlg;

		hr = S_OK;
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	return hr;
}
