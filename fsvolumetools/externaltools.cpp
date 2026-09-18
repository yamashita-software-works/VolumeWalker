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
#include <atlbase.h>  // ATL (base class only)
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

HRESULT CreateExternalTools(IExternalTools **pTools)
{
	CExternalToolsMenu *pExternalTools = NULL;

	pExternalTools = new CExternalToolsMenu;
	if( pExternalTools == NULL )
		return E_OUTOFMEMORY;

	pExternalTools->AddRef();

	WCHAR szExtTools[MAX_PATH];
	GetModuleFileName(NULL,szExtTools,MAX_PATH);
	PathRemoveFileSpec(szExtTools);
	PathCombine(szExtTools,szExtTools,L"exttools.xml");
#ifdef _DEBUG
	PCWSTR pszDbgExtTools = L"d:\\buildlab\\win32tools\\.config\\exttools.xml";
	if( PathFileExists(pszDbgExtTools) )
	{
		StringCchCopy(szExtTools,MAX_PATH,pszDbgExtTools);
	}
#endif
	if( !PathFileExists(szExtTools) )
	{
		return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
	}

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

HRESULT DestroyExternalTools(CExternalToolsMenu *pTools)
{
	if( pTools == NULL )
		return S_FALSE;

	pTools->Release();

	return S_OK;
}
