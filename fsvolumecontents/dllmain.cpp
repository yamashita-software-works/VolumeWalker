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
#include "dirfileswindow.h"
#include "changejournalwindow.h"

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

//////////////////////////////////////////////////////////////////////////////

static HIMAGELIST m_himl = NULL;
static int m_iImageUpDir = I_IMAGENONE;

INT GetUpDirImageIndex()
{
	return m_iImageUpDir;
}

HIMAGELIST GetGlobalShareImageList()
{
	if( m_himl == NULL )
	{
		//
		// The image lists retrieved through this function are 
		// global system image lists;
		// do not call ImageList_Destroy using them.
		//
		Shell_GetImageLists(NULL,&m_himl);

#ifdef _DEBUG
		int cImages;
		cImages = ImageList_GetImageCount(m_himl);
		_TRACE("System image count=%u\n",cImages);
#endif

		int cx,cy;
		ImageList_GetIconSize(m_himl,&cx,&cy);

		HICON hIcon = (HICON)LoadImage(GetModuleHandle(L"shell32"), MAKEINTRESOURCE(46), IMAGE_ICON, cx, cy, 0);
		m_iImageUpDir = ImageList_AddIcon(m_himl,hIcon);
		DestroyIcon(hIcon);
	}
	return m_himl;
}

int GetImageListIndex(PCWSTR pszPath,PCWSTR pszFileName,DWORD dwFileAttributes)
{
	SHFILEINFO sfi = {0};
	int iImage = I_IMAGENONE;

	if( wcscmp(pszFileName,L"..") == 0 )
	{
		iImage = GetUpDirImageIndex();
		return iImage;
	}
	else if( wcscmp(pszFileName,L".") == 0 )
	{
		SHSTOCKICONINFO sii = {sizeof(sii)};
		SHGetStockIconInfo(SIID_FOLDER,SHGSI_SYSICONINDEX|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
		iImage = sii.iSysImageIndex;
		return iImage;
	}

	if( pszPath )
	{
		if( pszPath[0] == L'\\' && pszPath[1] == L'?' && pszPath[2] == L'?' && pszPath[3] == L'\\' &&
			iswalpha(pszPath[4]) && pszPath[5] == L':' )
		{
			UINT fOverlay = 0;
			if( dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
				fOverlay |= (SHGFI_LINKOVERLAY|SHGFI_OVERLAYINDEX);

			SHGetFileInfo(&pszPath[4],dwFileAttributes,&sfi,sizeof(sfi),SHGFI_SMALLICON|SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|fOverlay);

			if( sfi.hIcon != NULL )
				DestroyIcon(sfi.hIcon);
			iImage = sfi.iIcon;
		}
	}

	if( iImage == I_IMAGENONE )
	{
		if( NtPathIsRootDirectory(pszFileName) )
		{
			SHSTOCKICONINFO sii = {sizeof(sii)};
			SHGetStockIconInfo(SIID_DRIVEFIXED,SHGSI_SYSICONINDEX|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
			iImage = sii.iSysImageIndex;
		}
		else
		{
			UINT fOverlay = 0;
			if( dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
				fOverlay |= (SHGSI_LINKOVERLAY|SHGFI_OVERLAYINDEX);
			SHGetFileInfo(PathFindFileName(pszFileName),dwFileAttributes,&sfi,sizeof(sfi),
					SHGFI_ICON|SHGFI_SMALLICON|SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|fOverlay);
			iImage = sfi.iIcon;
			if( sfi.hIcon != NULL )
				DestroyIcon(sfi.hIcon);
		}
	}

	return iImage;
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
