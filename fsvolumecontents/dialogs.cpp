#pragma once
//***************************************************************************
//*                                                                         *
//*  dialogs.cpp                                                            *
//*                                                                         *
//*  Dialog Boxes Implement.                                                *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2024-04-22                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "dialogs.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////////

static HWND hWndBasisPos = NULL;

static INT_PTR CALLBACK GotoDirectoryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			_CenterWindow(hDlg,hWndBasisPos);

			SetWindowLongPtr(hDlg,DWLP_USER,lParam);

			SendDlgItemMessage(hDlg,IDC_EDIT,EM_SETLIMITTEXT,_NT_PATH_FULL_LENGTH,0);

			SHAutoComplete(GetDlgItem(hDlg,IDC_EDIT),SHACF_FILESYS_DIRS|SHACF_USETAB);

			return (INT_PTR)TRUE;
		}
		case WM_COMMAND:
		{
			if( LOWORD(wParam) == IDOK )
			{
				WCHAR *pBuffer = (WCHAR *)GetWindowLongPtr(hDlg,DWLP_USER);

				int cchPath = _NT_PATH_FULL_LENGTH;
				CStringBuffer szPath(_NT_PATH_FULL_LENGTH);
				GetDlgItemText(hDlg,IDC_EDIT,szPath,cchPath);

				if( !PathFileExists(szPath) )
				{
					if( !NtPathFileExists(szPath) )
					{
						MessageBeep(MB_ICONSTOP);	
						break;
					}
				}

				StringCchCopy(pBuffer,_NT_PATH_FULL_LENGTH,szPath);

				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			if( LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDCLOSE  )
			{
				EndDialog(hDlg, LOWORD(wParam));
			}
			break;
		}
	}
	return (INT_PTR)FALSE;
}

HRESULT GotoDirectoryDialog(HWND hWnd,PWSTR *ppszNewPath)
{
	HRESULT hr = E_FAIL;

	if( ppszNewPath == NULL )
		return E_INVALIDARG;

	WCHAR *psz = new WCHAR[ _NT_PATH_FULL_LENGTH ];
	if( psz == NULL )
		return E_OUTOFMEMORY;

	hWndBasisPos = hWnd;

	if( DialogBoxParamW(_GetResourceInstance(), MAKEINTRESOURCE(IDD_GOTODIRECTORY),
			hWnd, &GotoDirectoryDlgProc,(LPARAM)psz) == IDOK )
	{
#if 0
		*ppszNewPath = _MemAllocString(psz);
#else // Reserved: Preparing for future implementation.
		*ppszNewPath = (PWSTR)CoTaskMemAlloc( (wcslen(psz) + 1) * sizeof(WCHAR) );
		if( *ppszNewPath )
		{
			wcscpy(*ppszNewPath,psz);
			hr = S_OK;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
#endif
	}
	else
	{
		hr = S_FALSE;
	}

	delete[] psz;

	return hr;
}
