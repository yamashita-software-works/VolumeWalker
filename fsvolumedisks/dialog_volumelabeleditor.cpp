//*****************************************************************************
//
//  dialog_volumelabel.cpp
//
//  PURPOSE: Volume Label edit dialog.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-02-01 Created
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
#include "libntwdk.h"
#include "ntnativeapi.h"

typedef struct _VOLUMELABELEDIT_DIALOG_PARAM
{
	HWND hWnd;
	WCHAR szVolumeName[MAX_PATH];
	WCHAR szVolumeLabel[MAX_PATH];
} VOLUMELABELEDIT_DIALOG_PARAM;

struct CVolumeLabelEditDialog : public CDialogWindow
{
	HWND m_hwndEdit;

	CVolumeLabelEditDialog()
	{
	}
	
	~CVolumeLabelEditDialog()
	{
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		VOLUMELABELEDIT_DIALOG_PARAM *pdlgParam = (VOLUMELABELEDIT_DIALOG_PARAM *)lParam;
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

		m_hwndEdit = GetDlgItem(m_hWnd,IDC_EDIT);
		SendMessage(m_hwndEdit,EM_LIMITTEXT,MAX_PATH,0);

		SetWindowText(m_hwndEdit,pdlgParam->szVolumeLabel);
#if 0
		EnableWindow(GetDlgItem(hDlg,IDOK),FALSE);
#endif
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
		VOLUMELABELEDIT_DIALOG_PARAM *pdlgParam = (VOLUMELABELEDIT_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		GetWindowText(m_hwndEdit,pdlgParam->szVolumeLabel,_countof(pdlgParam->szVolumeLabel));

		EndDialog(hDlg,IDOK);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
#if 0
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
#endif
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
//  VolumeLabelEditDialog()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
VolumeLabelEditDialog(
	__in HWND hWnd,
	__in PCWSTR pszVolumeName,
	__in DWORD dwFlags,
	__in_opt PCWSTR DefaultLabelName,
	__inout_opt PWSTR VolumeLabelName,
	__in int cchVolumeLabelName
	)
{
	HRESULT hr = S_OK;
	PCWSTR pszDialogTitle = L"Volume Label Editor";
	WCHAR szVolumeName[MAX_PATH];

    if( HasPrefix(L"\\??\\",pszVolumeName) )
		StringCchCopy(szVolumeName,MAX_PATH,pszVolumeName);
	else	
		StringCchPrintf(szVolumeName,MAX_PATH,L"\\??\\%s",pszVolumeName);

	DWORD dwDesired = GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE;

	HANDLE hVolume;
	NTSTATUS Status;
	Status = OpenFile_W(&hVolume,NULL,szVolumeName,
						dwDesired,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						FILE_SYNCHRONOUS_IO_ALERT|FILE_OPEN_FOR_BACKUP_INTENT);

	if( Status != STATUS_SUCCESS )
	{
		WCHAR szBuffer[1024];

		PWSTR pMessage = NULL;
		_GetSystemErrorMessage(Status,&pMessage);

		if( STATUS_ACCESS_DENIED == Status )
		{
			StringCchCopy(szBuffer,_countof(szBuffer),L"Could not open the volume.\n\n");

			SIZE_T cch = wcslen(szBuffer);
			FormatNtStatusErrorMessage(pMessage,&szBuffer[cch],_countof(szBuffer)-cch,0);
		}
		else
		{
			FormatNtStatusErrorMessage(pMessage,szBuffer,_countof(szBuffer),0);

			if( szBuffer[0] == L'\0' )
				StringCchCopy(szBuffer,_countof(szBuffer),L"Unknown error occurred.");
		}

		_FreeSystemErrorMessage(pMessage);

		MsgBox(hWnd,szBuffer,pszDialogTitle,MB_OK|MB_ICONSTOP);

		return HRESULT_FROM_NT(Status);
	}

	VOLUMELABELEDIT_DIALOG_PARAM *pParam = new VOLUMELABELEDIT_DIALOG_PARAM;
	if( pParam == NULL )
		return E_OUTOFMEMORY;

	ZeroMemory(pParam,sizeof(VOLUMELABELEDIT_DIALOG_PARAM));

	StringCchCopy(pParam->szVolumeName,_countof(pParam->szVolumeName),szVolumeName);

	DWORD FileSystemFlags;
	GetVolumeInformationByHandleW(hVolume,pParam->szVolumeLabel,_countof(pParam->szVolumeLabel),NULL,0,&FileSystemFlags,NULL,0);

	CVolumeLabelEditDialog *dlg = new CVolumeLabelEditDialog;

	if( dlg )
	{
		if( dlg->DoModal(hWnd,IDD_VOLUME_LABEL_EDITOR,(LPARAM)pParam,_GetResourceInstance()) == IDOK )
		{
			IO_STATUS_BLOCK IoStatus = {0};

			FILE_FS_LABEL_INFORMATION *pLabel;

			ULONG cbLabel = (ULONG)(wcslen(pParam->szVolumeLabel) * sizeof(WCHAR));
			ULONG cb = sizeof(FILE_FS_LABEL_INFORMATION) + cbLabel;

			pLabel = (FILE_FS_LABEL_INFORMATION *)_MemAllocZero( cb );

			if( pLabel )
			{
				pLabel->VolumeLabelLength = (ULONG)cbLabel;
				memcpy(pLabel->VolumeLabel,pParam->szVolumeLabel,pLabel->VolumeLabelLength);

				Status = NtSetVolumeInformationFile(hVolume,&IoStatus,pLabel,cb,FileFsLabelInformation);

				if( Status == STATUS_SUCCESS )
				{
					if( VolumeLabelName )
						StringCchCopy(VolumeLabelName,cchVolumeLabelName,pLabel->VolumeLabel);

					hr = S_OK;
				}
				else
				{
					PWSTR pMessage;
					_GetSystemErrorMessage(Status,&pMessage);

					CStringBuffer szBuffer(1024);

					FormatNtStatusErrorMessage(pMessage,szBuffer,szBuffer.GetBufferSize(),0);

					MsgBox(hWnd,szBuffer,pszDialogTitle,MB_OK|MB_ICONSTOP);

					_FreeSystemErrorMessage(pMessage);

					hr = HRESULT_FROM_NT(Status);
				}

				_SafeMemFree( pLabel );
			}
			else
			{
				hr = E_OUTOFMEMORY;
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

	CloseHandle(hVolume);

	delete pParam;

	return hr;
}
