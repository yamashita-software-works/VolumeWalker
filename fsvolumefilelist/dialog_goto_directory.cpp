#pragma once
//***************************************************************************
//*                                                                         *
//*  dialog_goto_directory.cpp                                              *
//*                                                                         *
//*  Goto Dialog Box Implements.                                            *
//*                                                                         *
//*  Author:  YAMASHITA Katsuhiro                                           *
//*                                                                         *
//*  History: 2024-04-22 Created                                            *
//*           2024-12-07 Modified                                           *
//*           2025-12-03 Modified                                           *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "dialog_traverse_directory.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////////

typedef struct _GTRDIRDLG_PARAM
{
	HWND hWndOwner;
	PWSTR PathBuffer;
	SIZE_T PathBufferLength;
	PWSTR pszCurrentDirectory;
} GTRDIRDLG_PARAM;

static INT_PTR CALLBACK GotoDirectoryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			GTRDIRDLG_PARAM *pgp = (GTRDIRDLG_PARAM *)lParam;
			SetWindowLongPtr(hDlg,DWLP_USER,lParam);

			_CenterWindow(hDlg,pgp->hWndOwner);

			SendDlgItemMessage(hDlg,IDC_EDIT,EM_SETLIMITTEXT,_NT_PATH_FULL_LENGTH,0);

			HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT);

			return (INT_PTR)TRUE;
		}
		case WM_COMMAND:
		{
			if( LOWORD(wParam) == IDOK )
			{
				GTRDIRDLG_PARAM *pgp = (GTRDIRDLG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

				*pgp->PathBuffer = L'\0';

				int cchPath = _NT_PATH_FULL_LENGTH;
				CStringBuffer szPath(_NT_PATH_FULL_LENGTH);
				GetDlgItemText(hDlg,IDC_EDIT,szPath,cchPath);

				if( wcscmp(szPath,L"..") == 0 )
				{
					PWSTR pszTempBuffer = pgp->pszCurrentDirectory;
					if( pszTempBuffer )
					{
						if( !NtPathIsRootDirectory(pszTempBuffer) )
						{
							RemoveBackslash(pszTempBuffer);
							RemoveFileSpec(pszTempBuffer);
							StringCchCopy(pgp->PathBuffer,pgp->PathBufferLength,pszTempBuffer);
						}
						_SafeMemFree(pszTempBuffer);
					}

					if( *pgp->PathBuffer == L'\0' )
					{
						MessageBeep(MB_ICONSTOP);	
						break;
					}
				}
				else
				{
					if( (iswalpha( ((LPCWSTR)szPath.c_str())[0] ) && (((LPCWSTR)szPath.c_str())[1]) == L':') )
					{
						// e.g. "C:"
						if( PathFileExists(szPath) )
						{
							StringCchCopy(pgp->PathBuffer,pgp->PathBufferLength,szPath);
						}
					}
					else if(HasPrefix(L"\\??\\",szPath) || HasPrefix(L"\\\\?\\",szPath) ||
						    HasPrefix(L"\\\\.\\",szPath) || HasPrefix(L"\\Device\\",szPath) )
					{
						if( PathFileExists_W(szPath,NULL) )
							StringCchCopy(pgp->PathBuffer,pgp->PathBufferLength,szPath);
					}
					else
					{
						PWSTR pszFullPath = NULL;

						pszFullPath = DuplicateString(szPath);

						if( !PathFileExists(pszFullPath) )
						{
							if( !NtPathFileExists(pszFullPath) )
							{
								MessageBeep(MB_ICONSTOP);	
								FreeMemory(pszFullPath);
								pszFullPath = NULL;
								break;
							}
						}

						if( pszFullPath )
							StringCchCopy(pgp->PathBuffer,pgp->PathBufferLength,pszFullPath);

						FreeMemory(pszFullPath);
						pszFullPath = NULL;
					}
				}

				if( *pgp->PathBuffer )
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

//----------------------------------------------------------------------------
//
//  GotoDirectoryDialog
//
//  PURPOSE: Dialog box for specifying a directory move to.
//
//  PARAMETERS:
//
//    hWnd         Owner window.
//
//    pszCurDir    Pointer to a null-terminated current directory path buffer.
//
//    ppszNewPath  Returns a pointer to a null-terminated string containing
//                 the destination path.Å@
//                 The returned memory must be freed with CoTaskMemFree.
//
//    dwFlags      Reserved.
//
//  RETURNS:
//
//    S_OK         Succeeded.
//
//    S_FALSE      User cancelled.
//
//  NOTE:
//    Not supported relative dot directory and canonicalize.
//    Like as ".","..","..\..\foo".
//
//----------------------------------------------------------------------------
HRESULT
WINAPI
GotoDirectoryDialog(
	HWND hWnd,
	PCWSTR pszCurDir,
	PWSTR *ppszNewPath,
	DWORD dwFlags
	)
{
	HRESULT hr = E_FAIL;

	if( ppszNewPath == NULL )
		return E_INVALIDARG;

	WCHAR *pszBuffer = new WCHAR[ _NT_PATH_FULL_LENGTH ];
	if( pszBuffer == NULL )
		return E_OUTOFMEMORY;

	WCHAR szCurDir[MAX_PATH];
	if( pszCurDir == NULL )
	{
		StringCchCopy(szCurDir,MAX_PATH,L"\\??\\");
		GetCurrentDirectory(MAX_PATH-4,&szCurDir[4]);
		pszCurDir = szCurDir;
	}

	//
	// Initialize Dialog Parameter
	//
	GTRDIRDLG_PARAM gp;
	gp.hWndOwner           = hWnd;
	gp.PathBuffer          = pszBuffer;
	gp.PathBufferLength    = _NT_PATH_FULL_LENGTH;
	gp.pszCurrentDirectory = _MemAllocString(pszCurDir);

	//
	// Start DialogBox
	//
	if( DialogBoxParamW(_GetResourceInstance(), MAKEINTRESOURCE(IDD_GOTODIRECTORY),
			hWnd, &GotoDirectoryDlgProc,(LPARAM)&gp) == IDOK )
	{
		if( wcscmp(pszBuffer,L".") == 0 )
		{
			; // not support
		}
		else if( wcscmp(pszBuffer,L"..") == 0 )
		{
			StringCchCopy(pszBuffer,_NT_PATH_FULL_LENGTH,pszCurDir);
			RemoveBackslash(pszBuffer);
			RemoveFileSpec(pszBuffer);
		}
		else
		{
#if 0 // alphabet drive letter only
			if( iswalpha(pszBuffer[0]) && pszBuffer[1] == L':' )
#else // accept non-alphabetic drive letter
			if( pszBuffer[1] == L':' )
#endif
			{
				PWSTR NtPath;
				GetNtPath(pszBuffer,&NtPath,0);
				StringCchCopy(pszBuffer,_NT_PATH_FULL_LENGTH,NtPath);
				FreeNtPath(NtPath);
			}
		}

		*ppszNewPath = (PWSTR)CoTaskMemAlloc( (wcslen(pszBuffer) + 1) * sizeof(WCHAR) );
		if( *ppszNewPath )
		{
			wcscpy(*ppszNewPath,pszBuffer);
			hr = S_OK;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		*ppszNewPath = L'\0';
		hr = S_FALSE;
	}

	_SafeMemFree(gp.pszCurrentDirectory);

	if( pszBuffer )
		delete[] pszBuffer;

	return hr;
}
