//***************************************************************************
//*                                                                         *
//*  volumentifs.cpp                                                        *
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
#include "dllmain.h"
#include "mem.h"
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
		pVolDevInfo->State.VolumeInformaion = _BIT_ON;
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
		pVolDevInfo->State.SizeInformation = _BIT_ON;
		FreeMemory(pSizeInfo);
	}

	// Attribute Information
	FILE_FS_ATTRIBUTE_INFORMATION *pAttrInfo;
	if( GetVolumeFsInformation(Handle,VOLFS_ATTRIBUTE_INFORMATION,(void**)&pAttrInfo) == STATUS_SUCCESS )
	{
		pVolDevInfo->FileSystemAttributes = pAttrInfo->FileSystemAttributes;
		pVolDevInfo->MaximumComponentNameLength = pAttrInfo->MaximumComponentNameLength;
		memcpy_s(pVolDevInfo->FileSystemName,ARRAYSIZE(pVolDevInfo->FileSystemName),pAttrInfo->FileSystemName,pAttrInfo->FileSystemNameLength);
		pVolDevInfo->State.AttributeInformation = _BIT_ON;
		FreeMemory(pAttrInfo);
	}

	// Control Information
	FILE_FS_CONTROL_INFORMATION *pCtrlInfo;
	if( GetVolumeFsInformation(Handle,VOLFS_CONTROL_INFORMATION,(void**)&pCtrlInfo) == STATUS_SUCCESS )
	{
		pVolDevInfo->Control.FreeSpaceStartFiltering = pCtrlInfo->FreeSpaceStartFiltering;
	    pVolDevInfo->Control.FreeSpaceStopFiltering  = pCtrlInfo->FreeSpaceStopFiltering;
		pVolDevInfo->Control.FreeSpaceThreshold      = pCtrlInfo->FreeSpaceThreshold;
		pVolDevInfo->Control.DefaultQuotaThreshold   = pCtrlInfo->DefaultQuotaThreshold;
		pVolDevInfo->Control.DefaultQuotaLimit       = pCtrlInfo->DefaultQuotaLimit;
		pVolDevInfo->Control.FileSystemControlFlags  = pCtrlInfo->FileSystemControlFlags;
		pVolDevInfo->State.ControlInformation = _BIT_ON;
		FreeMemory(pCtrlInfo);
	}

	// Srctor Size Information
	FILE_FS_SECTOR_SIZE_INFORMATION *pSectorSize;
	if( GetVolumeFsInformation(Handle,VOLFS_SECTOR_SIZE_INFORMATION,(void**)&pSectorSize) == STATUS_SUCCESS )
	{
		pVolDevInfo->SectorSize.ByteOffsetForPartitionAlignment = pSectorSize->ByteOffsetForPartitionAlignment;
		pVolDevInfo->SectorSize.ByteOffsetForSectorAlignment = pSectorSize->ByteOffsetForSectorAlignment;
		pVolDevInfo->SectorSize.FileSystemEffectivePhysicalBytesPerSectorForAtomicity = pSectorSize->FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
		pVolDevInfo->SectorSize.Flags = pSectorSize->Flags;
		pVolDevInfo->SectorSize.LogicalBytesPerSector = pSectorSize->LogicalBytesPerSector;
		pVolDevInfo->SectorSize.PhysicalBytesPerSectorForAtomicity = pSectorSize->PhysicalBytesPerSectorForAtomicity;
		pVolDevInfo->SectorSize.PhysicalBytesPerSectorForPerformance = pSectorSize->PhysicalBytesPerSectorForPerformance;
		pVolDevInfo->State.SectorSizeInformation = _BIT_ON;
		FreeMemory(pSectorSize);
	}

	// Device Type
	if( GetVolumeDeviceType(Handle,&pVolDevInfo->DeviceType,&pVolDevInfo->Characteristics) == STATUS_SUCCESS )
	{
		pVolDevInfo->State.DeviceType = _BIT_ON;
	}

	// Device ObjectId
	if( GetVolumeObjectId(Handle,(VOLUME_FS_OBJECTID_INFORMATION *)&pVolDevInfo->ObjectId) == STATUS_SUCCESS )
	{
		pVolDevInfo->State.ObjectId = _BIT_ON;
	}
}

//---------------------------------------------------------------------------
//
//  GetQuotaInformation()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
#include "simplevalarray.h"

HRESULT
WINAPI
GetQuotaInformation(
	HANDLE Handle,
	VOLUME_FS_QUOTA_INFORMATION_LIST **QuotaInfoList
	)
{
	BOOLEAN RestartScan = TRUE;
	IO_STATUS_BLOCK IoStatus;
	ULONG cbBufferLength = 65536;
	PVOID pBuffer = NULL;
	NTSTATUS Status;
	HRESULT hr;

	CValArray<QUOTA_INFORMATION*> va;

	pBuffer = AllocMemory( cbBufferLength );
	if( pBuffer == NULL )
		return E_OUTOFMEMORY;

	do
	{
		Status = NtQueryQuotaInformationFile(Handle,&IoStatus,pBuffer,cbBufferLength,FALSE,NULL,0,NULL,RestartScan);

		if( Status == STATUS_SUCCESS )
		{
			FILE_QUOTA_INFORMATION *pq = (FILE_QUOTA_INFORMATION  *)pBuffer;

			for(;;)
			{
				QUOTA_INFORMATION *pqi = (QUOTA_INFORMATION *)AllocMemory(sizeof(QUOTA_INFORMATION));

				if( pqi )
				{
					pqi->ChangeTime     = pq->ChangeTime;
					pqi->QuotaLimit     = pq->QuotaLimit;
					pqi->QuotaUsed      = pq->QuotaUsed;
					pqi->QuotaThreshold = pq->QuotaThreshold;
					pqi->SidLength      = pq->SidLength;

					pqi->Sid = AllocMemory(pq->SidLength);
					RtlCopyMemory(pqi->Sid,&pq->Sid,pq->SidLength);

					va.Add( pqi );
				}
				else
				{
					Status = STATUS_NO_MEMORY;
					break;
				}

				if( pq->NextEntryOffset == 0 )
					break;

				pq = (FILE_QUOTA_INFORMATION *)((ULONG_PTR)pq + pq->NextEntryOffset);
			}
		}

		RestartScan = 0;
	}
	while( Status == STATUS_SUCCESS );

	if( !NT_SUCCESS(Status) )
	{
		if( STATUS_NO_MORE_ENTRIES != Status )
		{
			int cItems = va.GetCount();
			for(int i = 0; i < cItems; i++)
				FreeMemory( va[i] );
			FreeMemory(pBuffer);
			return HRESULT_FROM_NT( Status );
		}

		Status = STATUS_SUCCESS;
	}

	int cItems = va.GetCount();
	if( cItems > 0 )
	{
		VOLUME_FS_QUOTA_INFORMATION_LIST *pMemBlock = (VOLUME_FS_QUOTA_INFORMATION_LIST *)AllocMemory( sizeof(VOLUME_FS_QUOTA_INFORMATION_LIST) + (sizeof(QUOTA_INFORMATION) * cItems));

		for(int i = 0; i < cItems; i++)
		{
			pMemBlock->QuataUser[i] = *(va[i]);
			FreeMemory( va[i] );
		}

		pMemBlock->ItemCount = cItems;;

		*QuotaInfoList = pMemBlock;
		hr = S_OK;
	}
	else
	{
		hr = S_FALSE;
	}

	FreeMemory(pBuffer);

	return hr;
}

//---------------------------------------------------------------------------
//
//  FreeQuotaInformation()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
HRESULT
WINAPI
FreeQuotaInformation(
	VOLUME_FS_QUOTA_INFORMATION_LIST *QuotaInfoList
	)
{
	for(ULONG q = 0; q < QuotaInfoList->ItemCount; q++)
	{
		FreeMemory(QuotaInfoList->QuataUser[q].Sid);
	}

	FreeMemory(QuotaInfoList);

	return S_OK;
}
