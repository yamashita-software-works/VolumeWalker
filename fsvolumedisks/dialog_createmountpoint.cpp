//*****************************************************************************
//
//  dialog_createmountpoint.cpp
//
//  PURPOSE: Create Mount Point Dialog.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-03-17 Created
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

static PCWSTR pszTitle = L"Create Mount Point";

static HRESULT _CreateMountPoint(HWND hWnd,PCWSTR pszVolumeName,PCWSTR pszMountPointPath)
{
	HRESULT hr = E_FAIL;

	if( (hr = MountVolumeToFolder(pszMountPointPath,pszVolumeName)) == S_OK )
	{
		MsgBox(hWnd,L"Mount point created.",pszTitle, MB_OK|MB_ICONINFORMATION);
	}
	else
	{
		WCHAR szMessage[MAX_PATH];
		StringCchPrintf(szMessage,ARRAYSIZE(szMessage),L"%s",pszVolumeName);
		_ErrorMessageBoxEx(hWnd,0,pszTitle,szMessage,hr,MB_OK|MB_ICONSTOP);
	}

	return hr;
}

typedef struct _CREATE_MOUNT_POINT_DIALOG_PARAM
{
	HWND hWnd;
	WCHAR *pszVolumeName;
	WCHAR *pszMountPointPath;
} CREATE_MOUNT_POINT_DIALOG_PARAM;

struct CCreateMountPointDialog : public CDialogWindow
{
	HWND m_hwndEditVolume;
	HWND m_hwndEditPath;

	CCreateMountPointDialog()
	{
		m_hwndEditVolume = NULL;
		m_hwndEditPath = NULL;
	}
	
	~CCreateMountPointDialog()
	{
	}

	__forceinline BOOL IsEmptyEditBoxes()
	{
		return (GetWindowTextLength(m_hwndEditVolume) != 0 && GetWindowTextLength(m_hwndEditPath) != 0);
	}

	__forceinline VOID UpdateOKButton()
	{
		EnableWindow( GetDlgItem(m_hWnd,IDOK),IsEmptyEditBoxes() );
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		CREATE_MOUNT_POINT_DIALOG_PARAM *pdlgParam = (CREATE_MOUNT_POINT_DIALOG_PARAM *)lParam;
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

		m_hwndEditVolume = GetDlgItem(m_hWnd,IDC_EDIT1);
		SendMessage(m_hwndEditVolume,EM_LIMITTEXT,MAX_PATH,0);
		if( pdlgParam->pszVolumeName )
			SetWindowText(m_hwndEditVolume,pdlgParam->pszVolumeName);

		m_hwndEditPath = GetDlgItem(m_hWnd,IDC_EDIT2);
		SendMessage(m_hwndEditPath,EM_LIMITTEXT,MAX_PATH,0);
		if( pdlgParam->pszMountPointPath )
			SetWindowText(m_hwndEditVolume,pdlgParam->pszMountPointPath);

		SHAutoComplete(m_hwndEditPath,SHACF_FILESYS_ONLY|SHACF_FILESYS_DIRS);

		UpdateOKButton();

		SetFocus(m_hwndEditPath);

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
		CREATE_MOUNT_POINT_DIALOG_PARAM *pdlgParam = (CREATE_MOUNT_POINT_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		WCHAR szVolumeName[MAX_PATH];
		WCHAR *pszMounePointPath;
		int cchMounePointPath = _NT_PATH_FULL_LENGTH;

		pszMounePointPath = _MemAllocStringBuffer( cchMounePointPath );
		GetWindowText(m_hwndEditPath,pszMounePointPath,cchMounePointPath);

		GetWindowText(m_hwndEditVolume,szVolumeName,MAX_PATH);

		HRESULT hr = E_FAIL;
		if( wcslen(szVolumeName) != 0 && wcslen(pszMounePointPath) != 0 )
		{
			AppendBackslash_W(szVolumeName,MAX_PATH);
			AppendBackslash_W(pszMounePointPath,cchMounePointPath);

			hr = _CreateMountPoint(hDlg,szVolumeName,pszMounePointPath);
		}

		_SafeMemFree(pszMounePointPath);

		if( hr == S_OK )
			EndDialog(hDlg,IDOK);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		if( HIWORD(wParam) == EN_UPDATE )
		{
			switch( LOWORD(wParam) )
			{
				case IDC_EDIT1:
				case IDC_EDIT2:
				{
					UpdateOKButton();
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
//  CreateMountPointDialog()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
CreateMountPointDialog(
	__in HWND hWnd,
	__in PCWSTR pszVolumeName,
	__in PCWSTR pszPointPointFolderPath,
	__in DWORD dwFlags
	)
{
	WCHAR szWin32VolumeName[MAX_PATH];
	HRESULT hr = S_OK;

	if( pszVolumeName == NULL )
		return E_INVALIDARG;

	ZeroMemory(szWin32VolumeName,sizeof(szWin32VolumeName));

	if( dwFlags & MPDF_APPENDPREFIX_NT )
	{
		StringCchPrintf(szWin32VolumeName,MAX_PATH,L"\\??\\%s",pszVolumeName);
	}
	else
	{
		StringCchCopy(szWin32VolumeName,MAX_PATH,pszVolumeName);
	}

	CREATE_MOUNT_POINT_DIALOG_PARAM *pParam = new CREATE_MOUNT_POINT_DIALOG_PARAM;
	if( pParam == NULL )
		return E_OUTOFMEMORY;

	ZeroMemory(pParam,sizeof(CREATE_MOUNT_POINT_DIALOG_PARAM));
	
	if( pszVolumeName )
		pParam->pszVolumeName = _MemAllocString( szWin32VolumeName );

	if( pszPointPointFolderPath )
		pParam->pszMountPointPath = _MemAllocString( pszPointPointFolderPath );

	CCreateMountPointDialog *dlg = new CCreateMountPointDialog;

	if( dlg )
	{
		if( dlg->DoModal(hWnd,IDD_ASSIGN_PATH,(LPARAM)pParam,_GetResourceInstance()) == IDOK )
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

	_SafeMemFree( pParam->pszVolumeName );
	_SafeMemFree( pParam->pszMountPointPath );

	delete pParam;

	return hr;
}
