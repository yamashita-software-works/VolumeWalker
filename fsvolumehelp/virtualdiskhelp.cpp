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
#include "volumehelp.h"
#include "ntvolumehelp.h"
#include "ntnativehelp.h"
#define INITGUID
#include <guiddef.h>
#include <virtdisk.h>

//----------------------------------------------------------------------------
//
//  VirtualDisk_GetDependencyInformationByHandle()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
DWORD
WINAPI
VirtualDisk_GetDependencyInformationByHandle(
	HANDLE hStorage,
	PSTORAGE_DEPENDENCY_INFO *ppStorageInfo
	)
{
	ULONG SizeUsed = 0;
	DWORD dwResult;

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
#ifdef _DEBUG
					STORAGE_DEPENDENCY_INFO_TYPE_1 *psdit1 = &pBuffer->Version1Entries[0];
					STORAGE_DEPENDENCY_INFO_TYPE_2 *psdit2 = &pBuffer->Version2Entries[0];
#endif
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
//  VirtualDisk_GetDependencyInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
BOOL
WINAPI
VirtualDisk_GetDependencyInformation(
	PCWSTR NtDevicePath,
	PSTORAGE_DEPENDENCY_INFO *ppsdi
	)
{
	DWORD dwError = 0;

	if( GetOSVersion() <= 0x600 )
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		return FALSE;
	}

	BOOL bResult = FALSE;
	HANDLE Handle;
	Handle = OpenDisk(NtDevicePath,0,FILE_READ_ATTRIBUTES|SYNCHRONIZE);

	if( Handle != INVALID_HANDLE_VALUE )
	{
		PSTORAGE_DEPENDENCY_INFO psdi = NULL;
		dwError = VirtualDisk_GetDependencyInformationByHandle(Handle,&psdi);

		if( dwError == ERROR_SUCCESS )
		{
			if( ppsdi )
			{
				*ppsdi = psdi;
			}

			bResult = TRUE;
		}

		CloseHandle(Handle);

		SetLastError(dwError);
	}

	return bResult;
}
