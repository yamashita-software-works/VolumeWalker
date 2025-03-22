#pragma once
//***************************************************************************
//*                                                                         *
//*  gotodialog.cpp                                                         *
//*                                                                         *
//*  Goto Dialog Box Implements.                                            *
//*                                                                         *
//*  Author:  YAMASHITA Katsuhiro                                           *
//*                                                                         *
//*  History: 2024-04-22 Created                                            *
//*           2024-12-07 Modified                                           *
//*           2024-12-12 GotoDirectoryOnSameVolumeDialog()                  *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "gotodialog.h"
#include "resource.h"

typedef struct _CALLBACKPARAM {
	HANDLE hpa;
} CALLBACKPARAM;

BOOLEAN
CALLBACK
EnumFilesCallback(
    HANDLE hDirectory,
    PCWSTR DirectoryName,
	ULONG Flags,
    PVOID pFileInfo,
    ULONG_PTR CallbackContext
    )
{
	FS_FILE_ID_BOTH_DIR_INFORMATION *p = (FS_FILE_ID_BOTH_DIR_INFORMATION *)pFileInfo;

	if( p->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		if( !IS_RELATIVE_DIR_NAME_WITH_UNICODE_SIZE(p->FileName,p->FileNameLength) )
		{
			CALLBACKPARAM *pc = (CALLBACKPARAM *)CallbackContext;

			PWSTR psz = (PWSTR)_MemAllocZero( p->FileNameLength + sizeof(WCHAR) );

			if( psz )
			{
				memcpy(psz,p->FileName,p->FileNameLength);

				SPtrArray_Add(pc->hpa,psz);
			}
		}
	}
	return true;
}

class CCustomAutoCompleteSource :
	public IEnumString,
	public IACList
{
	int    m_index;
	ULONG  m_ref;
	HANDLE m_hpa;

	PWSTR pszCurrentDirectory;
	PWSTR pszExpandPart;
public:
	PWSTR pszNtDevice;

	HRESULT	SetCurrentDirectory(PCWSTR pszCurDir)
	{
		return (pszCurrentDirectory = _MemAllocString(pszCurDir)) != NULL ? S_OK : E_OUTOFMEMORY;
	}

	PCWSTR GetCurrentDirectory() const
	{
		return pszCurrentDirectory;
	}

public:
	CCustomAutoCompleteSource()
	{
		_TRACE("CCustomAutoCompleteSource::CCustomAutoCompleteSource()\n");
		m_ref = 0;
		m_index = 0;
		m_hpa = NULL;
		pszExpandPart = NULL;
		pszCurrentDirectory = NULL;
		pszNtDevice = NULL;
	}

	~CCustomAutoCompleteSource()
	{
		_TRACE("CCustomAutoCompleteSource::~CCustomAutoCompleteSource()\n");
		if( m_hpa )
		{
			ResetList();
			SPtrArray_Destroy(m_hpa);
		}

		_SafeMemFree( pszCurrentDirectory );
		_SafeMemFree( pszExpandPart );
	}

	ULONG __stdcall Release()
	{
		ASSERT(m_ref > 0);
		if( m_ref > 0 )
		{
			--m_ref;
			if( m_ref == 0 )
			{
				_TRACE("CCustomAutoCompleteSource::Release = %d\n",m_ref);
				delete this;
				return 0;
			}
		}
		_TRACE("CCustomAutoCompleteSource::Release = %d\n",m_ref);
		return m_ref;
	}

	ULONG __stdcall AddRef()
	{
		++m_ref;
		_TRACE("CCustomAutoCompleteSource::AddRef = %d\n",m_ref);
		return m_ref;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void**ppvObject)
	{
		_TRACE("CCustomAutoCompleteSource::QueryInterface\n");

		if( IsEqualGUID(IID_IUnknown,riid) )
		{
			*ppvObject = static_cast<IEnumString *>(this);
			AddRef();
		}
		else if( IsEqualGUID(IID_IEnumString,riid) )
		{
			*ppvObject = static_cast<IEnumString *>(this);
			AddRef();
		}
		else if( IsEqualGUID(IID_IACList,riid) )
		{
			*ppvObject = static_cast<IACList *>(this);
			AddRef();
		}
		else
		{
			return E_NOTIMPL;
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Reset()
	{
		_TRACE("CCustomAutoCompleteSource::Reset\n");
		m_index = 0;

		_SafeMemFree(pszExpandPart);
		pszExpandPart = _MemAllocString( L"" );

		if( m_hpa == NULL )
		{
			m_hpa = SPtrArray_Create( 1024 );
		}
		else
		{
			ResetList();
		}

		NTSTATUS Status;
		HANDLE hRoot;

		Status = OpenFileEx_W(&hRoot,pszCurrentDirectory,
                    FILE_LIST_DIRECTORY|FILE_TRAVERSE|SYNCHRONIZE,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

		if( Status == 0 )
		{
			CALLBACKPARAM cp;
			cp.hpa = m_hpa;

			EnumFiles(hRoot,NULL,NULL,0,&EnumFilesCallback,(ULONG_PTR)&cp);

			CloseHandle(hRoot);
		}

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Next(ULONG celt,LPOLESTR *rgelt,ULONG *pceltFetched)
	{
		_TRACE("CCustomAutoCompleteSource::Next\n");

		int cItems = SPtrArray_GetCount(m_hpa);
		if( m_index == cItems )
		{
			*pceltFetched = 0;
			return S_FALSE;
		}

		PWSTR pszFileName = (PWSTR)SPtrArray_Get(m_hpa,m_index);
		SIZE_T cch;
		cch = wcslen(pszExpandPart) + wcslen(pszFileName) + 1;
		PWSTR psz = (PWSTR)CoTaskMemAlloc( cch * sizeof(WCHAR) );

		StringCchCopy(psz,cch,pszExpandPart);
		StringCchCat(psz,cch,pszFileName);

		m_index++;
		*rgelt = psz;
		*pceltFetched = 1;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum)
	{
		_TRACE("CCustomAutoCompleteSource::Clone\n");
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Skip(ULONG celt)
	{
		_TRACE("CCustomAutoCompleteSource::Skip\n");
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Expand(PCWSTR pszExpand)
	{
		_TRACE("CCustomAutoCompleteSource::Expand\n");

		_SafeMemFree( pszExpandPart );
		pszExpandPart = _MemAllocString( pszExpand );

		WCHAR szVolumeRoot[MAX_PATH];

		StringCchCopy(szVolumeRoot,MAX_PATH,pszNtDevice);
		AppendBackslash_W(szVolumeRoot,MAX_PATH);

		NTSTATUS Status;
		HANDLE hRoot = NULL;

		PWSTR pszFullPath = NULL;

		if( *pszExpand == L'\\' )
		{
			pszFullPath = CombinePath(pszNtDevice,pszExpand);
		}
		else
		{
			pszFullPath = CombinePath(pszCurrentDirectory,pszExpand);
		}

		if( m_hpa == NULL )
		{
			m_hpa = SPtrArray_Create( 1024 );
		}
		else
		{
			ResetList();
		}

		Status = OpenFileEx_W(&hRoot,pszFullPath,
                    FILE_LIST_DIRECTORY|FILE_TRAVERSE|SYNCHRONIZE,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

		if( Status == 0 )
		{
			CALLBACKPARAM cp;
			cp.hpa = m_hpa;

			EnumFiles(hRoot,NULL,NULL,0,&EnumFilesCallback,(ULONG_PTR)&cp);

			CloseHandle(hRoot);
		}

		FreeMemory(pszFullPath);

		return S_OK;
	}

private:
	void ResetList()
	{
		int i,cItems;
		cItems = SPtrArray_GetCount(m_hpa);
		for(i = 0; i < cItems; i++)
		{
			PVOID p = SPtrArray_Get(m_hpa,i);
			if( p )
				_MemFree( p );
		}
		SPtrArray_DeleteAll(m_hpa);
	}
};

/////////////////////////////////////////////////////////////////////////////////

typedef struct _GTRDIRDLG_PARAM
{
	HWND hWndOwner;
	IAutoComplete2 *autoComplete;
	PWSTR Buffer;
	SIZE_T BufferLength;
	CCustomAutoCompleteSource *pcacs;

	WCHAR szNtDevicePath[MAX_PATH];
	WCHAR szDosDeviceName[MAX_PATH];
	WCHAR szDosDrive[4];
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

			HRESULT hr;
			CCustomAutoCompleteSource *pcacs = pgp->pcacs;

			IUnknown *punk;
			hr = pcacs->QueryInterface(__uuidof(IUnknown),(void **)&punk);

			hr = pgp->autoComplete->Init(hwndEdit, punk, NULL, NULL);
			pgp->autoComplete->SetOptions( ACO_AUTOSUGGEST | ACO_UPDOWNKEYDROPSLIST | ACO_AUTOAPPEND );

			WCHAR szText[MAX_PATH];

			if( pgp->szDosDrive[0] )
				StringCchPrintf(szText,ARRAYSIZE(szText),L"%s (%s)",pgp->szDosDeviceName,pgp->szDosDrive);
			else
				StringCchPrintf(szText,ARRAYSIZE(szText),L"%s",pgp->szDosDeviceName);

			SetDlgItemText(hDlg,IDC_VOLUME_NAME,szText);

			return (INT_PTR)TRUE;
		}
		case WM_COMMAND:
		{
			if( LOWORD(wParam) == IDOK )
			{
				GTRDIRDLG_PARAM *pgp = (GTRDIRDLG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

				*pgp->Buffer = L'\0';

				int cchPath = _NT_PATH_FULL_LENGTH;
				CStringBuffer szPath(_NT_PATH_FULL_LENGTH);
				GetDlgItemText(hDlg,IDC_EDIT,szPath,cchPath);

				if( wcscmp(szPath,L"..") == 0 )
				{
					PWSTR pszTempBuffer = _MemAllocString(pgp->pcacs->GetCurrentDirectory());
					if( pszTempBuffer )
					{
						if( !NtPathIsRootDirectory(pszTempBuffer) )
						{
							RemoveBackslash(pszTempBuffer);
							RemoveFileSpec(pszTempBuffer);
							StringCchCopy(pgp->Buffer,pgp->BufferLength,pszTempBuffer);
						}
						_SafeMemFree(pszTempBuffer);
					}

					if( *pgp->Buffer == L'\0' )
					{
						MessageBeep(MB_ICONSTOP);	
						break;
					}
				}
				else
				{
					PWSTR pszFullPath;
					if( szPath.c_str()[0] == L'\\' )
						pszFullPath = CombinePath(pgp->szNtDevicePath,szPath);
					else
						pszFullPath= CombinePath(pgp->pcacs->GetCurrentDirectory(),szPath);

					if( !PathFileExists(pszFullPath) )
					{
						if( !NtPathFileExists(pszFullPath) )
						{
							MessageBeep(MB_ICONSTOP);	
							break;
						}
					}

					StringCchCopy(pgp->Buffer,pgp->BufferLength,pszFullPath);

					FreeMemory(pszFullPath);
				}

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
//  GotoDirectoryOnSameVolumeDialog
//
//  PURPOSE: Dialog box for specifying a relative directory.
//
//  PARAMETERS:
//
//    hWnd         Owner window.
//
//    pszCurDir    Pointer to a null-terminated current directory path buffer.
//
//    ppszNewPath  Returns a pointer to a null-terminated string containing
//                 the destination path.@
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
GotoDirectoryOnSameVolumeDialog(
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

	CCustomAutoCompleteSource *pcacs = new CCustomAutoCompleteSource();
	if( pcacs == NULL )
		goto _cleanup;

	WCHAR szCurDir[MAX_PATH];
	if( pszCurDir == NULL )
	{
		StringCchCopy(szCurDir,MAX_PATH,L"\\??\\");
		GetCurrentDirectory(MAX_PATH-4,&szCurDir[4]);
		pszCurDir = szCurDir;
	}

	if( pcacs->SetCurrentDirectory(pszCurDir) != S_OK )
		goto _cleanup;

	//
	// Initialize Dialog Parameter
	//
	GTRDIRDLG_PARAM gp;
	gp.Buffer       = pszBuffer;
	gp.BufferLength = _NT_PATH_FULL_LENGTH;
	gp.hWndOwner    = hWnd;
	gp.pcacs        = pcacs;
	ZeroMemory(gp.szNtDevicePath,sizeof(gp.szNtDevicePath));
	ZeroMemory(gp.szDosDeviceName,sizeof(gp.szDosDeviceName));
	ZeroMemory(gp.szDosDrive,sizeof(gp.szDosDrive));

	//
	// Get NT device path and Dos device name
	//
	NtPathParseDeviceName(pszCurDir,gp.szNtDevicePath,ARRAYSIZE(gp.szNtDevicePath),gp.szDosDeviceName,ARRAYSIZE(gp.szDosDeviceName));

	pcacs->pszNtDevice = gp.szNtDevicePath;

	//
	// Get MS-DOS drive
	//
	NtPathToDosPath(gp.szNtDevicePath,gp.szDosDrive,ARRAYSIZE(gp.szDosDrive));

	//
	// Create AutoComplete Object
	//
	IAutoComplete2 *pac;
	hr = CoCreateInstance(CLSID_AutoComplete,NULL, 
		                  CLSCTX_INPROC_SERVER,
		                  __uuidof(IAutoComplete2),(void **)&pac);

	gp.autoComplete = pac;

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

_cleanup:
	if( pcacs )
		pcacs->Release();

	if( pszBuffer )
		delete[] pszBuffer;

	if( pac )
	{
		LONG lret;
		lret = pac->Release();
		ASSERT(lret == 0);
	}

	return hr;
}
