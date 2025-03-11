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

#if _ENABLE_DARK_MODE_TEST
#include "darkmode.h"
#pragma comment(lib, "uxtheme.lib")
#endif

#define UILAYOUT_IMPL
#include "uilayout.h"

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
InitializeVolumeConsole(
	DWORD dwFlags
	)
{
	if( GetModuleHandle(L"fltlib.dll") == NULL )	
	{
		LoadFltLibDll(NULL);
	}

	_libmisc_set_langage_id(GetThreadUILanguage());

#if _ENABLE_DARK_MODE_TEST
	if( dwFlags & VOLUME_DLL_FLAG_ENABLE_DARK_MODE )
	{
		InitDarkMode();
		EnableDarkMode(TRUE);
		return S_OK;
	}
#endif
	return S_OK;
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
		case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
			hwndViewBase = CreateFileSystemStatisticsWindow(hwnd);
			break;
		case VOLUME_CONSOLE_SIMPLEHEXDUMP:
			hwndViewBase = CreateSimpleHexDumpWindow(hwnd);
			break;
		case VOLUME_CONSOLE_FILTERDRIVER:
			hwndViewBase = CreateFilterDriverWindow(hwnd);
			break;
		case VOLUME_CONSOLE_DISKPERFORMANCE:
			hwndViewBase = CreateDiskPerformanceWindow(hwnd);
			break;
		case VOLUME_CONSOLE_VOLUMEMOUNTPOINT:
			hwndViewBase = CreateVolumeMountPointWindow(hwnd);
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

HFONT GetGlobalFont(HWND hWnd)
{
#if 1
	HFONT hFont = NULL;
	HDC hdc = GetWindowDC(hWnd);
	LOGFONT lf = {0};
	lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	lf.lfCharSet = DEFAULT_CHARSET;
	StringCchCopy(lf.lfFaceName,_countof(lf.lfFaceName),L"Consolas");
	hFont = CreateFontIndirect( &lf );
	ReleaseDC(hWnd,hdc);
	return hFont;
#else
	return GetIconFont();
#endif
}

HFONT GetIconFont()
{
	HFONT hFontIcon;
	LOGFONT lf;
	SystemParametersInfo(SPI_GETICONTITLELOGFONT,sizeof(LOGFONT),&lf,0);
	hFontIcon = CreateFontIndirect(&lf);
	return hFontIcon;
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

static COLUMN_NAME column_name_map[] = {
	{ COLUMN_Name,               L"Name",               0 },
	{ COLUMN_CreationTime,       L"CreationTime",       0 }, 
	{ COLUMN_Size,               L"Size",               0 }, 
	{ COLUMN_Free,               L"Free",               0 }, 
	{ COLUMN_Usage,              L"Useage",             0 }, 
	{ COLUMN_UsageRate,          L"UsageRate",          0 }, 
	{ COLUMN_Format,             L"Format",             0 }, 
	{ COLUMN_Guid,               L"Guid",               0 }, 
	{ COLUMN_Drive,              L"Drive",              0 }, 
	{ COLUMN_VendorId,           L"VendorId",           0 }, 
	{ COLUMN_ProductId,          L"ProductId",          0 }, 
	{ COLUMN_PartitionStyle,     L"PartitionStyle",     0 }, 
	{ COLUMN_BusType,            L"BusType",            0 }, 
	{ COLUMN_DeviceId,           L"DeviceId",           0 }, 
	{ COLUMN_Identifier,         L"Identifier",         0 }, 
	{ COLUMN_OriginalDevice,     L"OriginalDevice",     0 }, 
	{ COLUMN_OriginalVolume,     L"OriginalVolume",     0 }, 
	{ COLUMN_SnapshotId,         L"SnapshotId",         0 }, 
	{ COLUMN_SnapshotSetId,      L"SnapshotSetId",      0 }, 
	{ COLUMN_Attributes,         L"Attributes",         0 }, 
	{ COLUMN_VolumeLabel,        L"VolumeLabel",        0 }, 
	{ COLUMN_Path,               L"Path",               0 }, 
	{ COLUMN_Type,               L"Type",               0 }, 
	{ COLUMN_FileSystemType,     L"FileSystemType",     0 },   
	{ COLUMN_Flags,              L"Flags",              0 }, 
	{ COLUMN_Address,            L"Address",            0 }, 
	{ COLUMN_Offset,             L"Offset",             0 }, 
	{ COLUMN_DumpHex,            L"DumpHex",            0 }, 
	{ COLUMN_DumpChar,           L"DumpChar",           0 }, 
	{ COLUMN_FilterName,         L"FilterName",         0 }, 
	{ COLUMN_FilterInstanceName, L"FilterInstanceName", 0 }, 
	{ COLUMN_FilterAltitude,     L"FilterAltitude",     0 }, 
	{ COLUMN_FilterVolumeName,   L"FilterVolumeName",   0 }, 
	{ COLUMN_FilterFrameId,      L"FilterFrameId",      0 }, 
	{ COLUMN_InstallDate,        L"InstallDate",        0 }, 
	{ COLUMN_FirstInstallDate,   L"FirstInstallDate",   0 }, 
	{ COLUMN_LastArrivalDate,    L"LastArrivalDate",    0 }, 
	{ COLUMN_LastRemovalDate,    L"LastRemovalDate",    0 }, 
};

const COLUMN_NAME *GetColumnNameTable()
{
	return column_name_map;
}

const int GetColumnNameTableItemCount()
{
	return _countof(column_name_map);
}

const int GetColumnNameTableInfo(COLUMN_NAME **Names,SIZE_T *BufferSize)
{
	*Names = column_name_map;
	if( BufferSize )
		*BufferSize = GetColumnNameTableItemCount() * sizeof(COLUMN_NAME);
	return GetColumnNameTableItemCount();
}
