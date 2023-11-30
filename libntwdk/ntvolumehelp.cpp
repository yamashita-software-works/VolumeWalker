//***************************************************************************
//*                                                                         *
//*  ntvolumehelp.cpp                                                       *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//***************************************************************************
//
//  2022.04.02
//  ddk build source code
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include <ntifs.h>
#include "ntvolumehelp.h"
#include "ntnativeapi.h"
#include "ntnativehelp.h"

EXTERN_C
NTSTATUS
NTAPI
OpenVolume_U(
    UNICODE_STRING *pusVolumeName,
    ULONG Flags,
    HANDLE *pHandle
    )
{
    NTSTATUS Status;
	UNICODE_STRING usVolumeDeviceName;

	usVolumeDeviceName = *pusVolumeName;
    RemoveBackslash_U(&usVolumeDeviceName);

    ULONG DesiredAccess = STANDARD_RIGHTS_READ|FILE_READ_ATTRIBUTES|SYNCHRONIZE;
    if( Flags & OPEN_READ_DATA )
        DesiredAccess |= FILE_READ_DATA;

	if( Flags & OPEN_GENERIC_READ )
        DesiredAccess |= GENERIC_READ;

    Status = OpenFile_U(pHandle,NULL,&usVolumeDeviceName,
                        DesiredAccess,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        FILE_OPEN_FOR_BACKUP_INTENT|FILE_SYNCHRONOUS_IO_NONALERT);

    RtlSetLastWin32Error( RtlNtStatusToDosError(Status) );

    return Status;
}

EXTERN_C
NTSTATUS
NTAPI
OpenVolume(
    PCWSTR VolumeName,
    ULONG Flags,
    HANDLE *pHandle
    )
{
	UNICODE_STRING usVolumeName;
	RtlInitUnicodeString(&usVolumeName,VolumeName);
	return OpenVolume_U(&usVolumeName,Flags,pHandle);
}

EXTERN_C
NTSTATUS
NTAPI
OpenRootDirectory(
    PCWSTR pszRootDirectory,
    ULONG Flags,
    HANDLE *pHandle
    )
{
    NTSTATUS Status = 0;
    BOOLEAN bOpen = true;
    UNICODE_STRING usRootDirectory;
    PWSTR pszAllocPath = NULL;

    RtlInitUnicodeString(&usRootDirectory,pszRootDirectory);

    if( !GetRootDirectory_U(&usRootDirectory) )
    {
        // try append root directory with path (volume name)
        pszAllocPath = CombinePath(pszRootDirectory,L"\\");
        RtlInitUnicodeString(&usRootDirectory,pszAllocPath);

        if( !GetRootDirectory_U(&usRootDirectory) )
        {
            bOpen = false;
        }
    }

    if( bOpen )
    {
        ULONG DesiredAccess = FILE_READ_ATTRIBUTES;
        if( Flags & OPEN_READ_DATA )
            DesiredAccess |= FILE_READ_DATA;

        Status = OpenFile_U(pHandle,NULL,&usRootDirectory,
                            DesiredAccess,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,0);

        RtlSetLastWin32Error( RtlNtStatusToDosError(Status) );
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    if( pszAllocPath )
        FreeMemory(pszAllocPath);

    return Status;
}

EXTERN_C
ULONG
NTAPI
GetVolumeDeviceType(
    HANDLE Handle, //  Handle that Volume device or Root directory
    PULONG pDeviceType,
    PULONG pCharacteristics
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus = {0};
    FILE_FS_DEVICE_INFORMATION di = {0};
    Status = NtQueryVolumeInformationFile(Handle,&IoStatus,&di,sizeof(di),FileFsDeviceInformation);
    if( Status == STATUS_SUCCESS )
    {
        *pDeviceType = di.DeviceType;
        *pCharacteristics = di.Characteristics;
    }
    return Status;
}

EXTERN_C
ULONG
NTAPI
GetVolumeObjectId(
    HANDLE Handle, //  Handle that Volume device or Root directory
    VOLUME_FS_OBJECTID_INFORMATION *ObjectId
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus = {0};
    FILE_FS_OBJECTID_INFORMATION objid = {0};
    Status = NtQueryVolumeInformationFile(Handle,&IoStatus,&objid,sizeof(objid),FileFsObjectIdInformation);
    if( Status == STATUS_SUCCESS )
    {
        memcpy(ObjectId,&objid,sizeof(VOLUME_FS_OBJECTID_INFORMATION));
    }
    return Status;
}

typedef struct _FILE_FS_SECTOR_SIZE_INFORMATION {
  ULONG LogicalBytesPerSector;
  ULONG PhysicalBytesPerSectorForAtomicity;
  ULONG PhysicalBytesPerSectorForPerformance;
  ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
  ULONG Flags;
  ULONG ByteOffsetForSectorAlignment;
  ULONG ByteOffsetForPartitionAlignment;
} FILE_FS_SECTOR_SIZE_INFORMATION, *PFILE_FS_SECTOR_SIZE_INFORMATION;

typedef struct _FILE_FS_DATA_COPY_INFORMATION
{
  ULONG NumberOfCopies;
} FILE_FS_DATA_COPY_INFORMATION,*PFILE_FS_DATA_COPY_INFORMATION;

typedef struct _FILE_FS_METADATA_SIZE_INFORMATION
{
  LARGE_INTEGER TotalMetadataAllocationUnits;
  ULONG SectorsPerAllocationUnit;
  ULONG BytesPerSector;
} FILE_FS_METADATA_SIZE_INFORMATION,*PFILE_FS_METADATA_SIZE_INFORMATION;

typedef struct _FILE_FS_FULL_SIZE_INFORMATION_EX {
  ULONGLONG ActualTotalAllocationUnits;
  ULONGLONG ActualAvailableAllocationUnits;
  ULONGLONG ActualPoolUnavailableAllocationUnits;
  ULONGLONG CallerTotalAllocationUnits;
  ULONGLONG CallerAvailableAllocationUnits;
  ULONGLONG CallerPoolUnavailableAllocationUnits;
  ULONGLONG UsedAllocationUnits;
  ULONGLONG TotalReservedAllocationUnits;
  ULONGLONG VolumeStorageReserveAllocationUnits;
  ULONGLONG AvailableCommittedAllocationUnits;
  ULONGLONG PoolAvailableAllocationUnits;
  ULONG     SectorsPerAllocationUnit;
  ULONG     BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION_EX, *PFILE_FS_FULL_SIZE_INFORMATION_EX;

#define FileFsSectorSizeInformation   ((FS_INFORMATION_CLASS)11)
#define FileFsDataCopyInformation     12
#define FileFsMetadataSizeInformation 13
#define FileFsFullSizeInformationEx   ((FS_INFORMATION_CLASS)14)

PVOID AllocAndGetInformaion(HANDLE hVolume,FS_INFORMATION_CLASS InfoClass,NTSTATUS& Status)
{
    ULONG cbBuffer = 1024;
    PVOID pBuffer;

    pBuffer = AllocMemory(cbBuffer);
    if( pBuffer == NULL )
	{
		Status = STATUS_NO_MEMORY;
        return NULL;
	}

    IO_STATUS_BLOCK IoStatusBlock = {0};
    do
    {
        Status = NtQueryVolumeInformationFile(hVolume,&IoStatusBlock,pBuffer,cbBuffer,InfoClass);
        if( Status == STATUS_INFO_LENGTH_MISMATCH )
        {
            cbBuffer += 1024;

            FreeMemory(pBuffer);

            pBuffer = AllocMemory(cbBuffer);
            if( pBuffer == NULL )
			{
				Status = STATUS_NO_MEMORY;
                return NULL;
			}

            continue;
        }
    }
    while( 0 );

    if( Status == STATUS_SUCCESS )
    {
        pBuffer = ReAllocateHeap(pBuffer,IoStatusBlock.Information); // shrink
    }
    else
    {
        FreeMemory(pBuffer);
    }

    return pBuffer;
}

EXTERN_C
NTSTATUS
NTAPI
GetVolumeFsInformation(
    IN HANDLE Handle,
    IN VOLFS_INFORMATION_CLASS InfoClass,
    OUT PVOID *Buffer
    )
{
	NTSTATUS Status;
    switch( InfoClass )
    {
        case VOLFS_VOLUME_INFORMATION:
        {
            *Buffer = AllocAndGetInformaion( Handle, FileFsVolumeInformation, Status );
            break;
        }
        case VOLFS_SIZE_INFORMATION:
        {
            *Buffer = AllocAndGetInformaion( Handle, FileFsSizeInformation, Status );
            break;
        }
        case VOLFS_ATTRIBUTE_INFORMATION:
        {
            *Buffer = AllocAndGetInformaion( Handle, FileFsAttributeInformation, Status );
            break;
        }
        case VOLFS_SECTORSIZE_INFORMATION:
        {
            *Buffer = AllocAndGetInformaion( Handle, FileFsSectorSizeInformation, Status );
            break;
        }
        default:
            return STATUS_INVALID_PARAMETER;
    }

    return Status;
}
