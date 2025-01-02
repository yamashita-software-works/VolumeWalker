//***************************************************************************
//*                                                                         *
//*  dllmain.cpp                                                            *
//*                                                                         *
//*  DLL entry point                                                        *
//*                                                                         *
//*  Create: 2023-07-11                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "fsvolumefilelist.h"
#include "fileswindow.h"
#include "ntobjecthelp.h"
#if _ENABLE_DARK_MODE_TEST
#include "darkmode.h"
#pragma comment(lib, "uxtheme.lib")
#endif

#define UILAYOUT_IMPL
#include "UILayout.h"

HINSTANCE hInstance = NULL;

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
			InitializeLibMisc(hInstance,GetUserDefaultUILanguage());
			InitLongPathBox(hInstance);
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

EXTERN_C
HRESULT
WINAPI
InitializeVolumeFilesConsole(
	DWORD dwFlags
	)
{
#if _ENABLE_DARK_MODE_TEST
	if( dwFlags & VOLUME_DLL_FLAG_ENABLE_DARK_MODE )
	{
		InitDarkMode();
		EnableDarkMode(TRUE);
		return S_OK;
	}
	return E_FAIL;
#else
	return E_NOTIMPL;
#endif
}

HICON SetWindowIcon(HWND hWnd,SHSTOCKICONID ssii)
{
    SHSTOCKICONINFO sii = {0};
	sii.cbSize = sizeof(sii);
	SHGetStockIconInfo(ssii,SHGSI_ICON|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
	DestroyIcon((HICON)SendMessage(hWnd,WM_GETICON,ICON_SMALL,0));
	SendMessage(hWnd,WM_SETICON,ICON_SMALL,(LPARAM)sii.hIcon);
	return sii.hIcon;
}

HFONT GetGlobalFont(HWND hWnd)
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

HFONT GetIconFont()
{
	HFONT hFontIcon;
	LOGFONT lf;
	SystemParametersInfo(SPI_GETICONTITLELOGFONT,sizeof(LOGFONT),&lf,0);
	hFontIcon = CreateFontIndirect(&lf);
	return hFontIcon;
}

//////////////////////////////////////////////////////////////////////////////

EXTERN_C
HWND
WINAPI
CreateVolumeFileList(
	HWND hwnd,
	UINT ConsoleId,
	DWORD dwOptionFlags,
	LPARAM lParam
	)
{
	return _CreateVolumeFileListWindow(hwnd,ConsoleId,dwOptionFlags,lParam);
}
