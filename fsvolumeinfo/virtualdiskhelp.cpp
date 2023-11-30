//***************************************************************************
//*                                                                         *
//*  virtualdiskhelp.cpp                                                    *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "volumeinfo.h"
#include "ntvolumehelp.h"
#include "ntnativehelp.h"
#define INITGUID
#include <guiddef.h>
#include <virtdisk.h>

//----------------------------------------------------------------------------
//
//  VirtualDiskGetDependencyInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
DWORD
WINAPI
VirtualDiskGetDependencyInformation(
	HANDLE hStorage,
	PSTORAGE_DEPENDENCY_INFO *ppStorageInfo
	)
{
	ULONG SizeUsed = 0;
	DWORD dwResult;

	// GET_STORAGE_DEPENDENCY_FLAG_HOST_VOLUMES

	// GET_STORAGE_DEPENDENCY_FLAG_DISK_HANDLE 
	// The handle (hStorage) provided is to a disk, not a volume or file.
	// 
	GET_STORAGE_DEPENDENCY_FLAG Flag;

	Flag = GET_STORAGE_DEPENDENCY_FLAG_HOST_VOLUMES;

	STORAGE_DEPENDENCY_INFO sdi;
	ZeroMemory(&sdi,sizeof(sdi));
	sdi.Version = STORAGE_DEPENDENCY_INFO_VERSION_2;

	dwResult = GetStorageDependencyInformation(hStorage,Flag,sizeof(sdi),&sdi,&SizeUsed);

	if( dwResult == ERROR_VIRTDISK_NOT_VIRTUAL_DISK )
		return ERROR_VIRTDISK_NOT_VIRTUAL_DISK;

	if( dwResult == ERROR_INSUFFICIENT_BUFFER )
	{
		if( ppStorageInfo != NULL )
		{
			PSTORAGE_DEPENDENCY_INFO pBuffer = (PSTORAGE_DEPENDENCY_INFO)LocalAlloc(LPTR,SizeUsed);
			if( pBuffer != NULL )
			{
				pBuffer->Version = STORAGE_DEPENDENCY_INFO_VERSION_2;

				dwResult = GetStorageDependencyInformation(hStorage,Flag,SizeUsed,pBuffer,&SizeUsed);

				if( dwResult == ERROR_SUCCESS )
				{
					// STORAGE_DEPENDENCY_INFO_TYPE_1 *psdit1 = &pBuffer->Version1Entries[0];
					// STORAGE_DEPENDENCY_INFO_TYPE_2 *psdit2 = &pBuffer->Version2Entries[0];
					*ppStorageInfo = pBuffer;
				}
			}
		}
		else
		{
			dwResult = ERROR_SUCCESS; // This volume is Virtual Disk, however no return data required.
		}
	}
	return dwResult;
}

//----------------------------------------------------------------------------
//
//  VirtualDiskIsVirtualDiskVolume()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
DWORD
WINAPI
VirtualDiskIsVirtualDiskVolume(
	PCWSTR NtDevicePath,
	PSTORAGE_DEPENDENCY_INFO *ppsdi
	)
{
	DWORD dwError = 0;

	WCHAR NtVolumeRootDir[MAX_PATH];
	StringCchCopy(NtVolumeRootDir,MAX_PATH,NtDevicePath);
	StringCchCat(NtVolumeRootDir,MAX_PATH,L"\\");

	BOOL bResult = FALSE;
	HANDLE Handle;
	Handle = OpenDisk(NtVolumeRootDir,0,FILE_READ_ATTRIBUTES|SYNCHRONIZE);

	if( Handle != INVALID_HANDLE_VALUE )
	{
		PSTORAGE_DEPENDENCY_INFO psdi = NULL;
		dwError = VirtualDiskGetDependencyInformation(Handle,&psdi);

		if( ppsdi )
		{
			*ppsdi = psdi;
		}

		CloseHandle(Handle);
	}
	else
	{
		dwError = GetLastError();
	}

	return bResult;
}
