#pragma once 

#ifndef NTAPI
#define NTAPI __stdcall
#endif

EXTERN_C
NTSTATUS
NTAPI
OpenVolume_U(
    UNICODE_STRING *pusVolumeName,
    ULONG Flags,
    HANDLE *pHandle
    );

EXTERN_C
NTSTATUS
NTAPI
OpenVolume(
    PCWSTR VolumeName,
    ULONG Flags,
    HANDLE *pHandle
    );

#define OPEN_READ_DATA     0x1
#define OPEN_GENERIC_READ  0x2

EXTERN_C
NTSTATUS
NTAPI
OpenRootDirectory(
    PCWSTR pszRootDirectory,
    ULONG Flags,
    HANDLE *pHandle
    );

EXTERN_C
ULONG
NTAPI
GetVolumeDeviceType(
    HANDLE Handle,
    PULONG pDeviceType,
    PULONG pCharacteristics
    );

typedef struct _VOLUME_FS_OBJECTID_INFORMATION
{
    UCHAR ObjectId[16];
    UCHAR ExtendedInfo[48];
} VOLUME_FS_OBJECTID_INFORMATION;

EXTERN_C
ULONG
NTAPI
GetVolumeObjectId(
    HANDLE Handle, //  Handle that Volume device or Root directory
    VOLUME_FS_OBJECTID_INFORMATION *ObjectId
    );

typedef enum {
    VOLFS_VOLUME_INFORMATION = 1,
    VOLFS_LABEL_INFORMATION,
    VOLFS_SIZE_INFORMATION,
    VOLFS_ATTRIBUTE_INFORMATION,
    VOLFS_FULLSIZE_INFORMATION,
    VOLFS_OBJECTID_INFORMATION,
    VOLFS_DRIVERPATH_INFORMATION,
    VOLFS_VOLUMEFLAGS_INFORMATION,
    VOLFS_SECTORSIZE_INFORMATION,
} VOLFS_INFORMATION_CLASS;

EXTERN_C
NTSTATUS
NTAPI
GetVolumeFsInformation(
    IN HANDLE Handle,
    IN VOLFS_INFORMATION_CLASS InfoClass,
    OUT PVOID *Buffer
    );

typedef struct _VOLUME_FS_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} VOLUME_FS_SIZE_INFORMATION, *PVOLUME_FS_SIZE_INFORMATION;
