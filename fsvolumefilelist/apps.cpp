//****************************************************************************
//
//  apps.cpp
//
//  Author:  YAMASHITA Katsuhiro
//
//  History: 2024-12-26
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

EXTERN_C
HRESULT
WINAPI
CreateFileLaunchApplicationList(
	void **pObject
	)
{
	CXMLApplicationsReader *p = new CXMLApplicationsReader;
	if( p == NULL )
		return E_OUTOFMEMORY;
	p->AddRef();
	*pObject = (void *)p;
	return S_OK;
}

HRESULT CreateApplicationList(IApplicationsReader **ppApps)
{
	HRESULT hr;

	IApplicationsReader *pApps = NULL;
	hr = CreateFileLaunchApplicationList( (void**)&pApps );
	if( hr != S_OK )
	{
		return hr;
	}

#if 0
	PCWSTR szAppFile = L""; // for debug
#else
	WCHAR szAppFile[MAX_PATH];
	GetModuleFileName(NULL,szAppFile,MAX_PATH);
	PathRemoveFileSpec(szAppFile);
	PathCombine(szAppFile,szAppFile,L"applications.xml");
	if( !PathFileExists(szAppFile) )
	{
		pApps->Release();
		return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
	}
#endif

	hr = pApps->Load( szAppFile );

	if( hr == S_OK )
	{
		*ppApps = pApps;
	}

	return hr;
}
