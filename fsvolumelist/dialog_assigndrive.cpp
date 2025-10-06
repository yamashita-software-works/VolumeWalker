//*****************************************************************************
//
//  dialog_assigndrive.cpp
//
//  PURPOSE: Assign Drive Letter Dialog.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-09-10 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "fsvolumelist.h"
#include "resource.h"
#include "basewindow.h"
#include "uilayout.h"

static PCWSTR pszTitle = L"Assgin Drive Letter";

static HRESULT _DoAssignDrive(HWND hWnd,PCWSTR pszNtDeviceName,PCWSTR pszDrive)
{
	HRESULT hr;

	if( (hr = DLEditAssignDrive(pszDrive,pszNtDeviceName)) == S_OK )
	{
		MsgBox(hWnd,L"Drive assign succeeded.",pszTitle, MB_OK|MB_ICONINFORMATION);
	}
	else
	{
		WCHAR szMessage[256];
		StringCchPrintf(szMessage,ARRAYSIZE(szMessage),L"%s - %s",pszNtDeviceName,pszDrive);
		_ErrorMessageBoxEx(hWnd,0,pszTitle,szMessage,hr,MB_OK|MB_ICONEXCLAMATION);
	}

	return hr;
}

typedef struct _VOLUMELABELEDIT_DIALOG_PARAM
{
	HWND hWnd;
	WCHAR szNtDeviceName[MAX_PATH];
	WCHAR szDrive[4];
} ASSIGN_DRIVE_DIALOG_PARAM;

struct CAssignDriveDialog : public CDialogWindow
{
	HWND m_hwndCombo;
	HWND m_hwndEdit;

	CAssignDriveDialog()
	{
		m_hwndCombo = NULL;
		m_hwndEdit = NULL;
	}
	
	~CAssignDriveDialog()
	{
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		ASSIGN_DRIVE_DIALOG_PARAM *pdlgParam = (ASSIGN_DRIVE_DIALOG_PARAM *)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		_CenterWindow(hDlg,GetActiveWindow());

		//
		// Remove the icon, the minimize and the restore buttons 
		// from the window has resizable border.
		//
		DWORD dw = GetWindowLong(hDlg,GWL_EXSTYLE);
		dw |= WS_EX_DLGMODALFRAME;
		SetWindowLong(hDlg,GWL_EXSTYLE,dw);
		if( (HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0) == NULL )
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)NULL);

		//
		// NT Device Name EditBox
		//
		m_hwndEdit = GetDlgItem(m_hWnd,IDC_EDIT);
		SendMessage(m_hwndEdit,EM_LIMITTEXT,MAX_PATH,0);
		SetWindowText(m_hwndEdit,pdlgParam->szNtDeviceName);
		if( pdlgParam->szNtDeviceName[0] == L'\0' )
			EnableWindow(GetDlgItem(hDlg,IDOK),FALSE);

		//
		// Drive Letter ComboBox
		//
		m_hwndCombo = GetDlgItem(m_hWnd,IDC_COMBO);


		WCHAR drives[26*4+1]; // "A:\\"
		GetLogicalDriveStrings(ARRAYSIZE(drives),drives);

		int unused_drives[26];
		memset(unused_drives,0,sizeof(unused_drives));

		WCHAR *pdrv = drives;
		while( *pdrv )
		{
			unused_drives[ *pdrv - L'A' ] = 1;
			pdrv += (wcslen(pdrv) + 1);
		}

		//
		// Insert to combobox unused drive letters.
		//
		int iFirstSel = 0;
		for(int i = 0; i < 26; i++)
		{
			if( unused_drives[ i ]  == 0 )
			{
				WCHAR szDrive[4];
				StringCchPrintf(szDrive,ARRAYSIZE(szDrive),L"%c",L'A' + i);
				int iIndex = ComboBox_AddString(m_hwndCombo,szDrive);
				ComboBox_SetItemData(m_hwndCombo,iIndex,(L'A' + i));

				if( i < 2 ) // skip 'A','B' letter
					iFirstSel++;
			}
		}

		ComboBox_SetCurSel(m_hwndCombo,iFirstSel);

		SetFocus(m_hwndCombo);

		return FALSE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		EndDialog(hDlg,IDCLOSE);
		return 0;
	}
	
public:
	VOID OnOK(HWND hDlg)
	{
		ASSIGN_DRIVE_DIALOG_PARAM *pdlgParam = (ASSIGN_DRIVE_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		int iSel = ComboBox_GetCurSel(m_hwndCombo);
		if( iSel == CB_ERR )
			return;

		INT chDrive = (INT)ComboBox_GetItemData(m_hwndCombo,iSel);

		ASSERT( chDrive != 0 );

		StringCchPrintf(pdlgParam->szDrive,ARRAYSIZE(pdlgParam->szDrive),L"%c:",chDrive);

		_DoAssignDrive(hDlg,pdlgParam->szNtDeviceName,pdlgParam->szDrive);

		EndDialog(hDlg,IDOK);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		if( HIWORD(wParam) == EN_UPDATE )
		{
			switch( LOWORD(wParam) )
			{
				case IDC_EDIT:
				{
					EnableWindow( GetDlgItem(hDlg,IDOK), GetWindowTextLength((HWND)lParam) ? TRUE : FALSE );
					break;
				}
			}
		}

		if( HIWORD(wParam) == EN_SETFOCUS )
		{
			SendMessage((HWND)lParam,EM_SETSEL,(WPARAM)-1,(LPARAM)-1);
		}

		switch( LOWORD(wParam) )
		{
			case IDOK:
				OnOK(hDlg);
				break;
			case IDCLOSE:
			case IDCANCEL:
			{
				EndDialog(hDlg,(INT_PTR)LOWORD(wParam));
				break;
			}
		}
		return 0;
	}

	//---------------------------------------------------------------------------
	//
	//  DlgProc()
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
			case WM_CLOSE:
				return OnClose(hDlg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hDlg,wParam,lParam);
		}
		return 0;
	}
};

//---------------------------------------------------------------------------
//
//  AssignDriveLetterDialog()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
/*++
   Quote:
   This program will change drive letter assignments, and the    
   changes persist through reboots. Do not remove drive letters  
   of your hard disks if you do not have this program on floppy  
   disk or you might not be able to access your hard disks again!
--*/
EXTERN_C
HRESULT
WINAPI
AssignDriveLetterDialog(
	__in HWND hWnd,
	__in PCWSTR pszNtDeviceName,
	__in PCWSTR pszDrive,
	__in_opt PWSTR pszAssignedDrive,
	__in_opt DWORD cchAssignedDrive,
	__in DWORD dwFlags
	)
{
	HRESULT hr = S_OK;

	ASSIGN_DRIVE_DIALOG_PARAM *pParam = new ASSIGN_DRIVE_DIALOG_PARAM;
	if( pParam == NULL )
		return E_OUTOFMEMORY;

	ZeroMemory(pParam,sizeof(ASSIGN_DRIVE_DIALOG_PARAM));
	StringCchCopy(pParam->szNtDeviceName,_countof(pParam->szNtDeviceName),pszNtDeviceName);

	CAssignDriveDialog *dlg = new CAssignDriveDialog;

	if( dlg )
	{
		if( dlg->DoModal(hWnd,IDD_ASSIGN_DRIVE,(LPARAM)pParam,_GetResourceInstance()) == IDOK )
		{
			if( pszAssignedDrive )
				StringCchCopy(pszAssignedDrive,cchAssignedDrive,pParam->szDrive);
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
