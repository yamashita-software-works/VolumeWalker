//***************************************************************************
//*                                                                         *
//*  dllmain.cpp                                                            *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2022-04-02                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "fsvolumelist.h"

#define INITGUID
#include <guiddef.h>
#include <diskguid.h>
#include <devpkey.h>
#include <devguid.h>

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
			hInstance = (HINSTANCE)hModule;
			_MemInit();

			InitializeLibMisc(hInstance,GetUserDefaultUILanguage());

			if( GetModuleHandle(L"fltlib.dll") == NULL )	
			{
				LoadFltLibDll(NULL);
			}
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
HWND
WINAPI
CreateVolumeConsoleWindow(
	HWND hwnd,
	UINT ConsoleType,
	PVOLUME_CONSOLE_CREATE_PARAM pParam
	)
{
	HWND hwndViewBase = NULL;

	switch( ConsoleType )
	{
		case VOLUME_CONSOLE_VOLUMEINFORMAION:
			hwndViewBase = CreateVolumeInformationWindow(hwnd);
			break;
		case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
			hwndViewBase = CreatePhysicalDriveInformationWindow(hwnd);
			break;
		case VOLUME_CONSOLE_DISKLAYOUT:
			hwndViewBase = CreateDiskLayoutWindow(hwnd);
			break;
		case VOLUME_CONSOLE_STORAGEDEVICE:
			hwndViewBase = CreateStorageDeviceWindow(hwnd);
			break;
		case VOLUME_CONSOLE_MOUNTEDDEVICE:
			hwndViewBase = CreateMountedDeviceWindow(hwnd);
			break;
		case VOLUME_CONSOLE_VOLUMELIST:
			hwndViewBase = CreateVolumeListWindow(hwnd);
			break;
		case VOLUME_CONSOLE_PHYSICALDRIVELIST:
			hwndViewBase = CreatePhysicalDriveListWindow(hwnd);
			break;
		case VOLUME_CONSOLE_SHADOWCOPYLIST:
			hwndViewBase = CreateShadowCopyListWindow(hwnd);
			break;
		case VOLUME_CONSOLE_DOSDRIVELIST:
			hwndViewBase = CreateDosDriveWindow(hwnd);
			break;
		default:
			return NULL;
	}

	if( hwndViewBase != NULL )
	{
		CConsoleWindow *pWnd = (CConsoleWindow *)GetBaseWindowObject(hwndViewBase);
		pWnd->InitData( pParam->pszReserved );
	}

	return hwndViewBase;
}

HFONT GetGlobalFont(HWND hWnd,BOOL bCreate)
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

EXTERN_C
HICON
WINAPI
GetDeviceClassIcon(
	UINT Type,
	const GUID *DevClassGuid
	)
{
	GUID Guid;

	if( DevClassGuid != NULL )
	{
		Guid = *DevClassGuid;
	}
	else
	{
		switch( Type )
		{
			case DEVICE_ICON_VOLUMESNAPSHOT:
				Guid = GUID_DEVCLASS_VOLUMESNAPSHOT;
				break;
			default:
				SetLastError(ERROR_INVALID_PARAMETER);
				return NULL;
		}
	}

	HICON hIcon;

	SP_CLASSIMAGELIST_DATA scd = {0};

	scd.cbSize = sizeof(scd);
	if( SetupDiGetClassImageList(&scd) )
	{
		DWORD dwError = ERROR_SUCCESS;
		int iImage = -1;
		if( SetupDiGetClassImageIndex(&scd,&Guid,&iImage) )
		{
			hIcon = ImageList_GetIcon(scd.ImageList,iImage,ILD_NORMAL);
		}
		else
		{
			dwError = GetLastError();
		}

		SetupDiDestroyClassImageList(&scd);

		SetLastError(dwError);
	}

	return hIcon;
}
