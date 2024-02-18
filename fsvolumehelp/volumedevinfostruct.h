#pragma once

#define MAX_NAME_LENGTH  260

#ifndef _NTIFS_
#include <winioctl.h>
#else
#include <ntddstor.h>
#endif
#include <mountdev.h>
#include "winfsctl.h"
#include "ntvolumehelp.h"

#define _BIT_ON  1
#define _BIT_OFF 0

typedef struct _VOLUME_DEVICE_INFORMATION
{
	struct {
		ULONG VolumeInformaion : 1;
		ULONG SizeInformation : 1;
		ULONG AttributeInformation : 1;
		ULONG NtfsData : 1;
		ULONG FatData : 1;
		ULONG UdfData : 1;
		ULONG RefsData : 1;
		ULONG DeviceType : 1;
		ULONG ObjectId : 1;
		ULONG ControlInformation : 1;
		ULONG SectorSizeInformation : 1;
	} State;

	// FILE_FS_VOLUME_INFORMATION
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG VolumeLabelLength;
    BOOLEAN SupportsObjects;
    WCHAR *VolumeLabel;

	// FILE_FS_SIZE_INFORMATION
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;

	// FILE_FS_DEVICE_INFORMATION
	DEVICE_TYPE DeviceType;
	ULONG Characteristics;

	// FILE_FS_ATTRIBUTE_INFORMATION
    ULONG FileSystemAttributes;
    LONG MaximumComponentNameLength;

	// FILE_FS_OBJECTID_INFORMATION
	struct {
		UCHAR ObjectId[16];
		UCHAR ExtendedInfo[48];
	} ObjectId;

	// Names
	WCHAR FileSystemName[MAX_NAME_LENGTH];
	WCHAR VolumeName[MAX_NAME_LENGTH]; // Win32 style volume name

	WCHAR *HardwareTypeString;   // msz reserved
	WCHAR *HardwareInstanceId;   // msz reserved
	WCHAR *HardwareDeviceObject; // msz reserved

	ULONG VolumeDirtyFlags; // admin only. (ULONG)-1 is not available.

	STORAGE_DEVICE_NUMBER DeviceNumber;
	PSTORAGE_DEVICE_DESCRIPTOR pDeviceDescriptor;
	STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR AlignmentDescriptor;
	PGET_MEDIA_TYPES pMediaTypes;

	PMOUNTDEV_UNIQUE_ID pUniqueId;

	PDISK_GEOMETRY_EX* pDiskGeometryPtrList;               // PDISK_GEOMETRY_EX*[]
	PDRIVE_LAYOUT_INFORMATION_EX *pDriveLayoutPtrList;     // PDRIVE_LAYOUT_INFORMATION_EX *[]
	PVOLUME_DISK_EXTENTS pVolumeDiskExtents;               // PVOLUME_DISK_EXTENTS
	RETRIEVAL_POINTER_BASE RetrievalPointerBase;           // RETRIEVAL_POINTER_BASE

	VOLUME_FS_CONTROL_INFORMATION Control;                 // Same as FS_CONTROL_INFORMATION
	VOLUME_FS_SECTOR_SIZE_INFORMATION SectorSize;          // Same as FS_SECTOR_SIZE_INFORMATION

	CHAR DirtyBit;
	CHAR VirtualDiskVolume;
    CHAR RecognitionFileSystem[9];

	union
	{
		struct {
			NTFS_VOLUME_DATA_BUFFER data;
			NTFS_EXTENDED_VOLUME_DATA extdata;
		} ntfs;
		struct {
			FILE_QUERY_ON_DISK_VOL_INFO_BUFFER DiskVolumeInfo;
		} udf;
		struct {
			REFS_VOLUME_DATA_BUFFER data;
		} refs;
	};

} VOLUME_DEVICE_INFORMATION, *PVOLUME_DEVICE_INFORMATION;
