#pragma once
//****************************************************************************
//*                                                                          *
//*  dialog_goto_directory_ex.cpp                                            *
//*                                                                          *
//*  Goto Dialog Box Implements.                                             *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2026-07-05 Created                                             *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "dialog_goto_directory.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////////

static BOOL g_ParseByNtDeciceNamespace = FALSE;

/////////////////////////////////////////////////////////////////////////////////

typedef struct _GOTODIREX_DLG_PARAM
{
	HWND hWndOwner;
	PWSTR PathBuffer;
	SIZE_T PathBufferLength;
	PWSTR pszCurrentDirectory;
	PWSTR pszVolumeName;
	PWSTR pszVolumeRelativePath;
	BOOL ParseByNtDeciceNamespace;
} GOTODIREX_DLG_PARAM;

HANDLE
OpenFileByPathAndId64(
	PCWSTR pszParentVolumeOrDirPath,
	LONGLONG FileId,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	DWORD dwOpenOptions
	)
{
	HANDLE hVolume;
	UNICODE_STRING usVolume;
	UNICODE_STRING usPath;
	NTSTATUS Status;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	SplitVolumeRelativePath(pszParentVolumeOrDirPath,&usVolume,&usPath);

	Status = OpenFile_U(&hVolume,NULL,&usVolume,
				FILE_READ_ATTRIBUTES|SYNCHRONIZE,
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

	if( Status == STATUS_SUCCESS )
	{
		FS_FILE_ID_DESCRIPTOR FileIdDesc;
		FileIdDesc.dwSize = sizeof(FileIdDesc);
		FileIdDesc.Type = FsFileIdType;
		FileIdDesc.FileId.QuadPart = FileId;

		Status = OpenFile_ID(&hFile,hVolume,&FileIdDesc,dwDesiredAccess,dwShareMode,dwOpenOptions);
	}

	SetLastErrorNtStatusToWin32(Status);

	return hFile;
}

VOID OnOK(HWND hDlg)
{
	GOTODIREX_DLG_PARAM *pgp = (GOTODIREX_DLG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

	ZeroMemory(pgp->PathBuffer,pgp->PathBufferLength*sizeof(WCHAR));

	int cchPath = _NT_PATH_FULL_LENGTH;

	CStringBuffer szPath(_NT_PATH_FULL_LENGTH);
	CStringBuffer szTemp(_NT_PATH_FULL_LENGTH);

	if( szPath.c_str() == nullptr || szTemp.c_str() == nullptr )
		return ;

	GetDlgItemText(hDlg,IDC_EDIT,szTemp,cchPath);

	ExpandEnvironmentStrings(szTemp,szPath,szPath.GetBufferSize());

	if( wcscmp(szPath,L".") == 0 )
	{
		GetCurrentDirectory(cchPath,szPath);
	}

	if( wcscmp(szPath,L"..") == 0 )
	{
		//
		// Parent Directory
		//
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
			return;
		}
	}
	else if( szPath.c_str()[0] == L':' && szPath.c_str()[1] == L':' )
	{
		//
		// Specifies File ID. ("::0xXXXXXXXX" style)
		//
		LONGLONG FileId;
		PWSTR pszFileId = &szPath.c_str()[2];

		FileId = _wcstoi64(pszFileId,nullptr,16);

		HANDLE hFile;
		hFile = OpenFileByPathAndId64(pgp->pszVolumeName,FileId,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,0);

		if( hFile != INVALID_HANDLE_VALUE )
		{
			UNICODE_STRING usFileName = {};
			GetFileNameInformation_U(hFile,&usFileName);

			PWSTR psz = CombinePath(pgp->pszVolumeName,usFileName.Buffer);
			if( psz )
			{
				StringCchCopy(pgp->PathBuffer,pgp->PathBufferLength,psz);
			
				FreeMemory(psz);
			}

			CloseHandle(hFile);
		}
	}
	else
	{
		if( (iswprint( ((LPCWSTR)szPath.c_str())[0] ) && (((LPCWSTR)szPath.c_str())[1]) == L':') )
		{
			//
			// DOS-drive style
			//
			// e.g.
			//   "C:"
			//   "C:\"
			//   "C:\Foo\Bar"
			//
			// transform dos-drive to nt-device path.
			// accept the non-alphabetic drive letter (NT's specification).
			//
			if( PathFileExists(szPath) )
			{
				PWSTR ntPath;
				GetNtPath(szPath,&ntPath,0);
				StringCchCopy(pgp->PathBuffer,pgp->PathBufferLength,ntPath);
				FreeNtPath(ntPath);
			}
		}
		else if(HasPrefix(L"\\??\\",szPath) || HasPrefix(L"\\\\?\\",szPath) || HasPrefix(L"\\\\.\\",szPath) )
		{
			//
			// Path with parsing prefix.
			//
			// e.g.
			// "\??\c:"
			// "\??\HarddiskVolume1\windows"
			// "\??\Volume{xxx...xxx}"
			// "\\?\Volume{xxx...xxx}"
			// "\\?\GlobalRoot\Device\HarddiskVoume\Foo\Bar"
			//
			if( PathFileExists_W(szPath,NULL) )
				StringCchCopy(pgp->PathBuffer,pgp->PathBufferLength,szPath);
		}
		else
		{
			//
			// if not parse by nt-device namespace mode, these relative path. 
			// otherwise the nt-device path.
			//
			// e.g.
			// "\foo\bar"
			// "a\b\c"
			//
			if( pgp->ParseByNtDeciceNamespace )
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
						return;
					}
				}

				if( pszFullPath )
					StringCchCopy(pgp->PathBuffer,pgp->PathBufferLength,pszFullPath);

				FreeMemory(pszFullPath);
				pszFullPath = NULL;
			}
			else
			{
		  		// "\foo\bar" -> attach volune name
				// "a\b\c"    -> attach current path
				if( szPath.c_str()[0] == L'\\' )
				{
					StringCchCopy(pgp->PathBuffer,pgp->PathBufferLength,pgp->pszVolumeName);
					StringCchCat(pgp->PathBuffer,pgp->PathBufferLength,szPath);
				}
				else
				{
					StringCchCopy(pgp->PathBuffer,pgp->PathBufferLength,pgp->pszCurrentDirectory);
					AppendBackslash_W(pgp->PathBuffer,(int)pgp->PathBufferLength);
					StringCchCat(pgp->PathBuffer,pgp->PathBufferLength,szPath);
				}

				if( !PathFileExists(pgp->PathBuffer) )
				{
					if( !NtPathFileExists(pgp->PathBuffer) )
					{
						MessageBeep(MB_ICONSTOP);	
						return;
					}
				}
			}
		}
	}

	if( *pgp->PathBuffer )
	{
		EndDialog(hDlg, IDOK);
	}

	return;
}

static INT_PTR CALLBACK GotoDirectoryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			GOTODIREX_DLG_PARAM *pgp = (GOTODIREX_DLG_PARAM *)lParam;
			SetWindowLongPtr(hDlg,DWLP_USER,lParam);

			_CenterWindow(hDlg,pgp->hWndOwner);

			SendDlgItemMessage(hDlg,IDC_EDIT,EM_SETLIMITTEXT,_NT_PATH_FULL_LENGTH,0);

			HWND hwndEdit = GetDlgItem(hDlg,IDC_EDIT);

			SetDlgItemText(hDlg,IDC_TEXT1,pgp->pszVolumeName);
			SetDlgItemText(hDlg,IDC_TEXT2,pgp->pszVolumeRelativePath);

			CheckDlgButton(hDlg,IDC_CHECKBOX,(pgp->ParseByNtDeciceNamespace) ? BST_CHECKED : BST_UNCHECKED);

			return (INT_PTR)TRUE;
		}
		case WM_COMMAND:
		{
			switch( LOWORD(wParam) )
			{
				case IDOK:
					OnOK(hDlg);
					return TRUE;
				case IDCANCEL:
				case IDCLOSE:
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				case IDC_CHECKBOX:
					if( HIWORD(wParam) == BN_CLICKED )
					{
						GOTODIREX_DLG_PARAM *pgp = (GOTODIREX_DLG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
						pgp->ParseByNtDeciceNamespace = (IsDlgButtonChecked(hDlg,IDC_CHECKBOX) == BST_CHECKED);
					}
					return TRUE;
			}
			break;
		}
	}
	return (INT_PTR)FALSE;
}

//----------------------------------------------------------------------------
//
//  GotoDirectoryDialogEx
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
//                 the destination path.
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
//    E_xxx        Some error status.
//
//  NOTE:
//    Not supported relative dot directory and canonicalize.
//    Like as ".","..","..\..\foo".
//
//----------------------------------------------------------------------------
HRESULT
WINAPI
GotoDirectoryDialogEx(
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

	UNICODE_STRING usVolume;
	UNICODE_STRING usRelativePath;
	SplitVolumeRelativePath(pszCurDir,&usVolume,&usRelativePath);

	//
	// Initialize Dialog Parameter
	//
	GOTODIREX_DLG_PARAM gp = {};
	gp.hWndOwner                = hWnd;
	gp.PathBuffer               = pszBuffer;
	gp.PathBufferLength         = _NT_PATH_FULL_LENGTH;
	gp.pszCurrentDirectory      = _MemAllocString(pszCurDir);
	gp.pszVolumeName            = AllocateSzFromUnicodeString(&usVolume);
	gp.pszVolumeRelativePath    = AllocateSzFromUnicodeString(&usRelativePath);
	gp.ParseByNtDeciceNamespace = g_ParseByNtDeciceNamespace;

	//
	// Start DialogBox
	//
	if( DialogBoxParamW(_GetResourceInstance(), MAKEINTRESOURCE(IDD_GOTODIRECTORYEX),
			hWnd, &GotoDirectoryDlgProc,(LPARAM)&gp) == IDOK )
	{
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

		g_ParseByNtDeciceNamespace = gp.ParseByNtDeciceNamespace;
	}
	else
	{
		*ppszNewPath = L'\0';
		hr = S_FALSE;
	}

	_SafeMemFree(gp.pszCurrentDirectory);

	FreeMemory(gp.pszVolumeName);
	FreeMemory(gp.pszVolumeRelativePath);

	if( pszBuffer )
		delete[] pszBuffer;

	return hr;
}
