//*****************************************************************************
//
//  dialog_removedrive.cpp
//
//  PURPOSE: Remove Drive Letter Dialog.
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

static PCWSTR pszTitle = L"Remove Drive Letter";

static HRESULT _DoRemoveDrive(HWND hWnd,PCWSTR pszNtDeviceName,PCWSTR pszDrive)
{
	HRESULT hr;

	if( (hr = DLEditRemoveDrive(pszDrive)) == S_OK )
	{
		MsgBox(hWnd,L"Drive remove succeeded.",pszTitle,MB_OK|MB_ICONINFORMATION);
	}
	else
	{
		WCHAR szMessage[256];
		StringCchPrintf(szMessage,ARRAYSIZE(szMessage),L"%s - %s",pszNtDeviceName,pszDrive);
		_ErrorMessageBoxEx(hWnd,0,pszTitle,szMessage,hr,MB_OK|MB_ICONSTOP);
	}

	return hr;
}

static BOOL ConfirmRemoveMessage(HWND hWnd)
{
	PCWSTR message = 
		L"WARNING!\n"
		L"\n"
		L"If you remove drive letters of volume,  you might not be able to access your hard disks or any storages.\n"
		L"\n"
		L"Do not remove drive letters of system volume.\n"
		L"\n\n"
		L"Do you want to remove this drive letter?";
	int iRet = MsgBox(hWnd,message,pszTitle,MB_YESNO|MB_ICONEXCLAMATION);
	return (iRet == IDYES);
}

typedef struct _REMOVE_DRIVE_DIALOG_PARAM
{
	HWND hWnd;
	WCHAR szNtDeviceName[MAX_PATH];
	WCHAR szDrive[4];
} REMOVE_DRIVE_DIALOG_PARAM;

struct CRemoveDriveDialog : public CDialogWindow
{
	HWND m_hwndCombo;

	CRemoveDriveDialog()
	{
		m_hwndCombo = NULL;
	}
	
	~CRemoveDriveDialog()
	{
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		REMOVE_DRIVE_DIALOG_PARAM *pdlgParam = (REMOVE_DRIVE_DIALOG_PARAM *)lParam;
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
		// Drive Letter ComboBox
		//
		m_hwndCombo = GetDlgItem(m_hWnd,IDC_COMBO);

		WCHAR drives[26*4+1]; // "A:\\"
		GetLogicalDriveStrings(ARRAYSIZE(drives),drives);

		int iIndex = 0;
		WCHAR *pdrv = drives;
		while( *pdrv )
		{
			WCHAR szDrive[4];
			StringCchPrintf(szDrive,ARRAYSIZE(szDrive),L"%c:",*pdrv);
			iIndex = ComboBox_AddString(m_hwndCombo,szDrive);
			ComboBox_SetItemData(m_hwndCombo,iIndex,*pdrv);
			pdrv += (wcslen(pdrv) + 1);
		}

		//
		// Insert to combobox used drive letters.
		//
		int iSelDrive = 0;
		int cItemCount = ComboBox_GetCount(m_hwndCombo);
		for(int i = 0; i < cItemCount; i++)
		{
			INT drv = (INT)ComboBox_GetItemData(m_hwndCombo,i);

			if( *pdlgParam->szDrive == drv )
			{
				iSelDrive = i;
				break;
			}
		}

		ComboBox_SetCurSel(m_hwndCombo,iSelDrive);

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
		if( !ConfirmRemoveMessage(hDlg) )
			return;

		REMOVE_DRIVE_DIALOG_PARAM *pdlgParam = (REMOVE_DRIVE_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		int iSel = ComboBox_GetCurSel(m_hwndCombo);
		if( iSel == CB_ERR )
			return;

		WCHAR chDrive = (WCHAR)ComboBox_GetItemData(m_hwndCombo,iSel);

		StringCchPrintf(pdlgParam->szDrive,ARRAYSIZE(pdlgParam->szDrive),L"%c:",chDrive);

		_DoRemoveDrive(hDlg,pdlgParam->szNtDeviceName,pdlgParam->szDrive);

		EndDialog(hDlg,IDOK);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
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
//  RemoveDriveLetterDialog()
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
RemoveDriveLetterDialog(
	__in HWND hWnd,
	__in PCWSTR pszNtDeviceName,
	__in PCWSTR pszDrive,
	__in_opt PWSTR pszRemovedDrive, // reserved
	__in_opt DWORD cchRemovedDrive, // reserved
	__in DWORD dwFlags // reserved
	)
{
	// todo:
	// No parameters validation.

	HRESULT hr = S_OK;

	if( dwFlags & RDDF_CHOOSE_DRIVE_UI )
	{
		REMOVE_DRIVE_DIALOG_PARAM *pParam = new REMOVE_DRIVE_DIALOG_PARAM;
		if( pParam == NULL )
			return E_OUTOFMEMORY;

		ZeroMemory(pParam,sizeof(REMOVE_DRIVE_DIALOG_PARAM));
		StringCchCopy(pParam->szNtDeviceName,_countof(pParam->szNtDeviceName),pszNtDeviceName);
		StringCchCopy(pParam->szDrive,_countof(pParam->szDrive),pszDrive);

		CRemoveDriveDialog *dlg = new CRemoveDriveDialog;

		if( dlg )
		{
			if( dlg->DoModal(hWnd,IDD_REMOVE_DRIVE,(LPARAM)pParam,_GetResourceInstance()) == IDOK )
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
	}
	else
	{
		if( ConfirmRemoveMessage(hWnd) )
		{
			hr = _DoRemoveDrive(hWnd,pszNtDeviceName,pszDrive);
		}
		else
		{
			hr = S_FALSE;
		}
	}
	return hr;
}
