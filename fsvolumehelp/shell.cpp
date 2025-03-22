//*****************************************************************************
//
//  shell.cpp
//
//  PURPOSE: Volume related shell helper functions.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-03-14 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "volumehelp.h"
#include "libntwdk.h"
#include "volumehelp.h"
#include <shellapi.h>

EXTERN_C
HICON 
WINAPI
GetDiskDeviceIcon(
	PCWSTR DeviceName
	)
{
	WCHAR szDeviceName[32];
	WCHAR szBoot[32];
	QueryDosDevice(L"BootPartition",szBoot,ARRAYSIZE(szBoot));

	if( HasPrefix(L"\\??\\",DeviceName) || HasPrefix(L"\\\\?\\",DeviceName) )
	{
		WCHAR szSymName[64];
		StringCchCopy(szSymName,ARRAYSIZE(szSymName),&DeviceName[4]);
		RemoveBackslash(szSymName);
		QueryDosDevice(szSymName,szDeviceName,ARRAYSIZE(szDeviceName));
	}
	else
	{
		StringCchCopy(szDeviceName,ARRAYSIZE(szDeviceName),DeviceName);
		RemoveBackslash(szDeviceName);
	}

	if( _wcsicmp(szDeviceName,szBoot) == 0 )
	{
		HMODULE hmod = LoadLibrary(L"imageres.dll");
		HICON hIcon = (HICON)LoadImage(hmod,MAKEINTRESOURCE(36),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
		FreeLibrary(hmod);
		return hIcon;
	}

	ULONG DeviceType = 0;
	ULONG Characteristics = 0;
	PGET_MEDIA_TYPES pMediaTypes = NULL;
	NTSTATUS Status;
	HANDLE Handle;
	if( (Status = OpenVolume(DeviceName,0,&Handle)) == STATUS_SUCCESS )
	{
		StorageGetMediaTypes(Handle,&pMediaTypes);

		GetVolumeDeviceType(Handle,&DeviceType,&Characteristics);

		CloseHandle(Handle);
	}

	if( pMediaTypes == NULL )
		return NULL;

	SHSTOCKICONID idIcon = SIID_DRIVEFIXED;
	switch( pMediaTypes->DeviceType )
	{
		case FILE_DEVICE_DVD:
			idIcon = SIID_DRIVEDVD;
			break;
		case FILE_DEVICE_CD_ROM:
			idIcon = SIID_DRIVECD;
			break;
		case FILE_DEVICE_DISK:
			idIcon = SIID_DRIVEFIXED;
			break;
		default:
			idIcon = SIID_DRIVEFIXED;
			break;
	}

	StorageMemFree(pMediaTypes);

	SHSTOCKICONINFO sii = {0};
	sii.cbSize = sizeof(sii);
	SHGetStockIconInfo(idIcon,SHGSI_ICON|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);

	return sii.hIcon;
}
