//*****************************************************************************
//
//  dialog_virtualdisk_attach.cpp
//
//  PURPOSE: Attach Virtual Disk Image File Dialog.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-02-20 Created
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
#include "volumehelp.h"

static HRESULT _Attach(HWND hWnd,LPCWSTR pszFileName)
{
	HANDLE hVhd = NULL;
	DWORD Status;

	if( (Status = AttachVirtualDiskFile(&hVhd,pszFileName,0,0)) == ERROR_SUCCESS )
	{
		MsgBox(hWnd,L"Attach succeeded.", MB_OK|MB_ICONINFORMATION);
		CloseHandle(hVhd);
	}
	else
	{
		_ErrorMessageBox(hWnd,0,pszFileName,Status,MB_OK|MB_ICONEXCLAMATION);
	}

	return 0;
}

typedef struct _VOLUMELABELEDIT_DIALOG_PARAM
{
	HWND hWnd;
	WCHAR szImageFile[MAX_PATH];
} VIRTUALDISK_ATTACH_DIALOG_PARAM;

HRESULT
OpenFileDialog(
	HWND hwnd,
	LPWSTR FilePath,
	int cchFilePath
	)
{
    IFileDialog *pfd;
    
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, 
								  NULL, 
								  CLSCTX_INPROC_SERVER, 
								  __uuidof(IFileDialog),(void**)&pfd);

    if (SUCCEEDED(hr))
    {
		DWORD dwOptions;
		pfd->GetOptions(&dwOptions);
		pfd->SetOptions(dwOptions|FOS_FORCEFILESYSTEM);

		PWSTR pszInitialPath = L"";
		PWSTR pszInitialFileName = NULL;
		PWSTR pszDefaultExtension = L".vhd";
		PWSTR pszTitle = L"Open Virtual Disk Image File";

		int FileTypeCount = 3;
		int FileTypeIndex = 0;
		COMDLG_FILTERSPEC *fileTypes = new COMDLG_FILTERSPEC[ FileTypeCount ];
		{
			fileTypes[0].pszName = L"VHD File";
			fileTypes[0].pszSpec = L"*.vhd";

			fileTypes[1].pszName = L"VHDX File";
			fileTypes[1].pszSpec = L"*.vhdx";

			fileTypes[2].pszName = L"ISO File";
			fileTypes[2].pszSpec = L"*.iso";

			pfd->SetFileTypes(FileTypeCount,fileTypes);
			pfd->SetFileTypeIndex(FileTypeIndex);

			delete[] fileTypes;
		}

		if( pszInitialPath )
		{
			IShellItem *pShellItem;

			hr = SHCreateItemFromParsingName(pszInitialPath,0,
                                             IID_IShellItem,
                                             reinterpret_cast<void**>(&pShellItem));
			if( SUCCEEDED(hr) )
			{
				pfd->SetFolder(pShellItem);
				pShellItem->Release();
			}
		}

		if( pszInitialFileName )
			pfd->SetFileName( pszInitialFileName );

		if( pszDefaultExtension )
			pfd->SetDefaultExtension( pszDefaultExtension );

		if( pszTitle )
			pfd->SetTitle( pszTitle );

        // Show the dialog
        hr = pfd->Show(hwnd);
        
        if( SUCCEEDED(hr) )
        {
            // Obtain the result of the user's interaction with the dialog.
            IShellItem *psiResult;
            hr = pfd->GetResult(&psiResult);
            
            if( SUCCEEDED(hr) )
            {
				LPWSTR name;

				if( SUCCEEDED(psiResult->GetDisplayName(SIGDN_FILESYSPATH,&name)) )
				{
					StringCchCopy(FilePath,cchFilePath,name);
				}

				CoTaskMemFree(name);

                psiResult->Release();
            }
        }
        pfd->Release();
    }
    return hr;
}

struct CVirtualDiskAttachDialog : public CDialogWindow
{
	HWND m_hwndEdit;

	CVirtualDiskAttachDialog()
	{
	}
	
	~CVirtualDiskAttachDialog()
	{
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		VIRTUALDISK_ATTACH_DIALOG_PARAM *pdlgParam = (VIRTUALDISK_ATTACH_DIALOG_PARAM *)lParam;
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

		EnableWindow(GetDlgItem(hDlg,IDOK),FALSE);

		SetWindowText(m_hwndEdit,pdlgParam->szImageFile);

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
		VIRTUALDISK_ATTACH_DIALOG_PARAM *pdlgParam = (VIRTUALDISK_ATTACH_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		GetWindowText(m_hwndEdit,pdlgParam->szImageFile,_countof(pdlgParam->szImageFile));

		_Attach(hDlg,pdlgParam->szImageFile);

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
			case IDC_BROWSE:
			{
				WCHAR sz[MAX_PATH];
				if( OpenFileDialog(hDlg,sz,_countof(sz)) == S_OK )
				{
					SetWindowText(m_hwndEdit,sz);
				}
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
//  VirualDiskAttachDialog()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
VirtualDiskAttachDialog(
	__in HWND hWnd,
	__in PCWSTR pszImageFileName, // Win32 Path
	__in DWORD dwFlags
	)
{
	HRESULT hr = S_OK;
	PCWSTR pszDialogTitle = L"Attach Virtual Disk Image";
	WCHAR szImageFile[MAX_PATH];

	szImageFile[0] = 0;

	VIRTUALDISK_ATTACH_DIALOG_PARAM *pParam = new VIRTUALDISK_ATTACH_DIALOG_PARAM;
	if( pParam == NULL )
		return E_OUTOFMEMORY;

	ZeroMemory(pParam,sizeof(VIRTUALDISK_ATTACH_DIALOG_PARAM));
	StringCchCopy(pParam->szImageFile,_countof(pParam->szImageFile),szImageFile);

	CVirtualDiskAttachDialog *dlg = new CVirtualDiskAttachDialog;

	if( dlg )
	{
		if( dlg->DoModal(hWnd,IDD_VIRTUALDISK_ATTACH,(LPARAM)pParam,_GetResourceInstance()) == IDOK )
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
