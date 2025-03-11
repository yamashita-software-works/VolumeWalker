//***************************************************************************
//*                                                                         *
//*  virtualdiskhelp.cpp                                                    *
//*                                                                         *
//*  Author:  YAMASHITA Katsuhiro                                           *
//*                                                                         *
//*  History: 2023-10-26 Created.                                           *
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

//----------------------------------------------------------------------------
//
//  AttachVirtualDiskFile()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
AttachVirtualDiskFile(
	HANDLE *phVhd,
	PCWSTR pszFilename,
	ULONG AccessMask,
	ULONG AttachFlags
	)
{
	HANDLE hVhd = NULL;

	DWORD Status = 0;

	VIRTUAL_STORAGE_TYPE storageType;
	VIRTUAL_DISK_ACCESS_MASK accessMask;

	BOOL bReadOnly = FALSE;
	BOOL bNoDriveLetter = FALSE;

	storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;

	if( AccessMask != 0 )
	{
		accessMask = (VIRTUAL_DISK_ACCESS_MASK)AccessMask;
	}
	else
	{
		// CurrentallyCurrently, checks only the extension.
		PCWSTR pszExtension;
		pszExtension = PathFindExtension(pszFilename);

		if( wcsicmp(pszExtension,L".vhd") == 0 )
		{
			accessMask = VIRTUAL_DISK_ACCESS_ALL;
		}
		else if( wcsicmp(pszExtension,L".vhdx") == 0 )
		{
			accessMask = VIRTUAL_DISK_ACCESS_ALL;
		}
		else if( wcsicmp(pszExtension,L".iso") == 0 )
		{
			accessMask = VIRTUAL_DISK_ACCESS_ATTACH_RO|VIRTUAL_DISK_ACCESS_DETACH|VIRTUAL_DISK_ACCESS_GET_INFO;
			bReadOnly = TRUE;
		}
		else
		{
			accessMask = VIRTUAL_DISK_ACCESS_ALL;
		}
	}

	storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;

	OPEN_VIRTUAL_DISK_FLAG flags;
	flags = OPEN_VIRTUAL_DISK_FLAG_NONE;

    OPEN_VIRTUAL_DISK_PARAMETERS parameters;
	parameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
	parameters.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;

	Status = OpenVirtualDisk(&storageType,pszFilename,accessMask,flags,&parameters,&hVhd);

	if( Status == ERROR_SUCCESS )
	{
		*phVhd = hVhd;

		ATTACH_VIRTUAL_DISK_FLAG Flags = ATTACH_VIRTUAL_DISK_FLAG_NONE;

		if( AttachFlags != 0 )
		{
			Flags = (ATTACH_VIRTUAL_DISK_FLAG)AttachFlags;
		}
		else
		{
			ULONG Option = 0;

			if( 1 )
				Flags |= ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME;

			if( bReadOnly )
				Flags |= ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY;

			if( bNoDriveLetter )
				Flags |= ATTACH_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER;

			if( 0 )
				Flags |= ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST;
		}

		Status = AttachVirtualDisk(hVhd,NULL,Flags,0,0,NULL);
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  DetachVirtualDiskFile()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
DetachVirtualDiskFile(
	HANDLE hVhd,
	PCWSTR pszFilename
	)
{
	DWORD Status = 0;

	if( hVhd == NULL && pszFilename != NULL )
	{
		VIRTUAL_STORAGE_TYPE storageType;
#if 0
		PCWSTR pszExtension;
		pszExtension = FsPathFindExtension(pszFilename);

		if( wcsicmp(pszExtension,L".vhd") == 0 )
			storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD;
		else if( wcsicmp(pszExtension,L".vhdx") == 0 )
			storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHDX;
		else if( wcsicmp(pszExtension,L".iso") == 0 )
			storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_ISO;
		else
			storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_VHD;
#else
		storageType.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_UNKNOWN;
#endif
		storageType.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;

		VIRTUAL_DISK_ACCESS_MASK accessMask;
		accessMask = VIRTUAL_DISK_ACCESS_DETACH;

		OPEN_VIRTUAL_DISK_FLAG flags;
		flags = OPEN_VIRTUAL_DISK_FLAG_NONE;

		OPEN_VIRTUAL_DISK_PARAMETERS parameters;
		parameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
		parameters.Version1.RWDepth = OPEN_VIRTUAL_DISK_RW_DEPTH_DEFAULT;

		Status = OpenVirtualDisk(&storageType,pszFilename,accessMask,flags,&parameters,&hVhd);
	}
	else
	{
		Status = ERROR_SUCCESS;
	}

	if( Status == ERROR_SUCCESS )
	{
		Status = DetachVirtualDisk(hVhd,DETACH_VIRTUAL_DISK_FLAG_NONE,0);
	}

	if( hVhd != NULL && pszFilename != NULL )
	{
		CloseHandle(hVhd);
	}

	return Status;
}
