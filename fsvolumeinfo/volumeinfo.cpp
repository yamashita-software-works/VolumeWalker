//***************************************************************************
//*                                                                         *
//*  volumeinfo.cpp                                                         *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create:  2022-04-02 created source for wdk build.                      *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include <ntifs.h>
#include <ntdddisk.h>
#include <ntddvol.h>
#include <ntstatus.h>
#include <winerror.h>
#include <strsafe.h>
#include <stdlib.h>
#include <limits.h>
#include "ntnativeapi.h"
#include "ntnativehelp.h"
#include "ntobjecthelp.h"
#include "ntvolumehelp.h"
#include "ntvolumenames.h"
#include "volumedevinfostruct.h"

//---------------------------------------------------------------------------
//
//  GatherNtVolumeDeviceInformation()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
VOID GatherNtVolumeDeviceInformation( HANDLE Handle, VOLUME_DEVICE_INFORMATION *pVolDevInfo )
{
	// Volume Information
	FILE_FS_VOLUME_INFORMATION *pVolumeInfo;
	if( GetVolumeFsInformation(Handle,VOLFS_VOLUME_INFORMATION,(void**)&pVolumeInfo) == STATUS_SUCCESS )
	{
		pVolDevInfo->VolumeCreationTime = pVolumeInfo->VolumeCreationTime;
		pVolDevInfo->VolumeSerialNumber = pVolumeInfo->VolumeSerialNumber;
		pVolDevInfo->SupportsObjects = pVolumeInfo->SupportsObjects;
		pVolDevInfo->VolumeLabel = AllocStringLengthCb(pVolumeInfo->VolumeLabel, pVolumeInfo->VolumeLabelLength );
		pVolDevInfo->State.VolumeInformaion = 1;
		FreeMemory(pVolumeInfo);
	}

	// Size Information
	FILE_FS_SIZE_INFORMATION *pSizeInfo;
	if( GetVolumeFsInformation(Handle,VOLFS_SIZE_INFORMATION,(void**)&pSizeInfo) == STATUS_SUCCESS )
	{
		pVolDevInfo->AvailableAllocationUnits = pSizeInfo->AvailableAllocationUnits;
		pVolDevInfo->BytesPerSector = pSizeInfo->BytesPerSector;
		pVolDevInfo->SectorsPerAllocationUnit = pSizeInfo->SectorsPerAllocationUnit;
		pVolDevInfo->TotalAllocationUnits = pSizeInfo->TotalAllocationUnits;
		pVolDevInfo->State.SizeInformation = 1;
		FreeMemory(pSizeInfo);
	}

	// Attribute Information
	FILE_FS_ATTRIBUTE_INFORMATION *pAttrInfo;
	if( GetVolumeFsInformation(Handle,VOLFS_ATTRIBUTE_INFORMATION,(void**)&pAttrInfo) == STATUS_SUCCESS )
	{
		pVolDevInfo->FileSystemAttributes = pAttrInfo->FileSystemAttributes;
		pVolDevInfo->MaximumComponentNameLength = pAttrInfo->MaximumComponentNameLength;
		memcpy_s(pVolDevInfo->FileSystemName,ARRAYSIZE(pVolDevInfo->FileSystemName),pAttrInfo->FileSystemName,pAttrInfo->FileSystemNameLength);
		pVolDevInfo->State.AttributeInformation = 1;
		FreeMemory(pAttrInfo);
	}

	// Device Type
	if( GetVolumeDeviceType(Handle,&pVolDevInfo->DeviceType,&pVolDevInfo->Characteristics) == STATUS_SUCCESS )
	{
		pVolDevInfo->State.DeviceType = 1;
	}

	// Device ObjectId
	if( GetVolumeObjectId(Handle,(VOLUME_FS_OBJECTID_INFORMATION *)&pVolDevInfo->ObjectId) == STATUS_SUCCESS )
	{
		pVolDevInfo->State.ObjectId = 1;
	}
}
