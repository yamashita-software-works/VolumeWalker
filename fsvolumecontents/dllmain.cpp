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
#include "fsvolumecontents.h"
#include "fileswindow.h"
#include "changejournalwindow.h"

#include "ntobjecthelp.h"

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

HICON SetWindowIcon(HWND hWnd,SHSTOCKICONID ssii)
{
    SHSTOCKICONINFO sii = {0};
	sii.cbSize = sizeof(sii);
	SHGetStockIconInfo(ssii,SHGSI_ICON|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
	DestroyIcon((HICON)SendMessage(hWnd,WM_GETICON,ICON_SMALL,0));
	SendMessage(hWnd,WM_SETICON,ICON_SMALL,(LPARAM)sii.hIcon);
	return sii.hIcon;
}

PWSTR GetIniFilePath()
{
	return NULL; //L""; // todo: reserved, feacher not implement.
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
CreateVolumeContentsBrowserWindow(
	HWND hwnd,
	UINT ConsoleId
	)
{
	if( ConsoleId == VOUUME_CONSOLE_CHANGE_JOURNAL )
	{
		return CreateChangeJournalWindow(hwnd);
	}

	return CreateDirectoryFilesWindow(hwnd);
}
