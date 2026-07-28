//****************************************************************************
//
//  externaltools.cpp
//
//  Author:  YAMASHITA Katsuhiro
//
//  History: 2025-02-12 Created
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#pragma warning(disable : 4995)
#pragma warning(disable : 4996)
#include <atlbase.h>  // ATL (base class only) for CComBSTR in "xmlapplications.h"
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <strsafe.h>
#include "mem.h"
#include "debug.h"
#include "stringbuffer.h"
#include "common.h"
#include "libmisc.h"
#include "xmlapplications.h"
#include "externaltools.h"

static void _GetDefinitionsFilePath(PWSTR pszExtTools,int cchExtTools)
{
	GetModuleFileName(NULL,pszExtTools,cchExtTools);
	PathRemoveFileSpec(pszExtTools);
	PathCombine(pszExtTools,pszExtTools,L"exttools.xml");
#ifdef _DEBUG
	PCWSTR pszDbgExtTools = L"";
	if( PathFileExists(pszDbgExtTools) )
	{
		StringCchCopy(szExtTools,cchExtTools,pszDbgExtTools);
	}
#endif
}

HRESULT CreateExternalTools(IExternalTools **pTools)
{
	WCHAR szExtTools[MAX_PATH];
	_GetDefinitionsFilePath(szExtTools,MAX_PATH);

	if( !PathFileExists(szExtTools) )
	{
		return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
	}

	CExternalToolsMenu *pExternalTools = new CExternalToolsMenu;
	if( pExternalTools == NULL )
		return E_OUTOFMEMORY;

	pExternalTools->AddRef();

	HRESULT hr;
	if( pExternalTools->Load( szExtTools ) )
	{
		*pTools = static_cast<IExternalTools*>(pExternalTools);
		hr = S_OK;
	}
	else
	{
		hr = E_FAIL;
	}

	return hr;
}

HRESULT IsExternalToolsFileExists()
{
	WCHAR szExtTools[MAX_PATH];
	_GetDefinitionsFilePath(szExtTools,MAX_PATH);

	if( !PathFileExists(szExtTools) )
	{
		SetLastError(ERROR_FILE_NOT_FOUND);
		return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
	}

	return S_OK;
}
