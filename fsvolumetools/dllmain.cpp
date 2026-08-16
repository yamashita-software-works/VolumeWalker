//*****************************************************************************
//
//  dllmain.cpp
//
//  PURPOSE: DllMain
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2024-08-21 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "fsvolumetools.h"
#include "resource.h"

#define UILAYOUT_IMPL
#include "uilayout.h"

#define INITGUID
#include <guiddef.h>
#include <diskguid.h>
#include <devpkey.h>
#include <devguid.h>

static HINSTANCE hInstance = NULL;

HINSTANCE _GetResourceInstance()
{
	return hInstance;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			hInstance = hModule;
			_MemInit();
			break;
		case DLL_PROCESS_DETACH:
			_MemEnd();
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

HFONT CreateGlobalFont(HWND hWnd)
{
	HFONT hFont = NULL;
	HDC hdc = GetWindowDC(hWnd);
	LOGFONT lf = {0};
	lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	lf.lfCharSet = ANSI_CHARSET;
	StringCchCopy(lf.lfFaceName,_countof(lf.lfFaceName),L"Consolas");
	hFont = CreateFontIndirect( &lf );
	ReleaseDC(hWnd,hdc);
	return hFont;
}

BOOL EnablePrivilege(LPTSTR lpszPrivilege, BOOL bEnable)
{
	BOOL             bResult;
	LUID             luid;
	HANDLE           hToken;
	TOKEN_PRIVILEGES tokenPrivileges;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		return FALSE;
	
	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		CloseHandle(hToken);
		return FALSE;
	}

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Luid = luid;
	tokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
	
	bResult = AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	
	CloseHandle(hToken);

	return bResult;
}
