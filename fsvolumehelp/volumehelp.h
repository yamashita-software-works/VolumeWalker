#pragma once
//
// volumehelp.h
//
// note:
// This file is toplevel header file to be include for Win32 source code. 
// Ported code from fsworkbench's dll.
//
#include <virtdisk.h>
#include <winioctl.h>
#include "ntvolumehelp.h"
#include "ntvolumenames.h"
#include "storagedevice.h"
#include "winfsctl.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <mountdev.h> // Requires path to DDK includes.

LONG
MountdevQueryUniqueId(
    HANDLE hVolume,
    PMOUNTDEV_UNIQUE_ID *pUniqueId
    );

BOOL
GetVolumeAttributeFlag(
    int iIndex,
    DWORD *pdwFlag
    );

int
GetDeviceTypeString(
    DWORD DeviceType,
    PWSTR pszText,
    int cchTextMax
    );

LPCWSTR
WINAPI
GetPartitionStyleText(
    LONG PartitionStyle
    );

LPCWSTR
WINAPI
GetPartitionTypeText(
    LONG PartitionType
    );

PWSTR
WINAPI
GetGPTPartitionTypeString(
    GUID& Guid,
    PWSTR pszBuf,
    int cchBuf
    );

PWSTR
WINAPI
GetGPTAttribute(
    DWORD64 Attributes,
    PWSTR pszBuf,
    int cchBuf
    );

PWSTR
GetStorageBusTypeDescString(
    DWORD BusType,
    LPWSTR pszBuf,
    int cchBuf
    );

PWSTR
GetDependentDiskFlagString(
    DWORD Flag,
    PWSTR psz,
    DWORD cch
    );

LPWSTR
WINAPI
_FormatByteSize(
    LONGLONG qdw,
    __out_ecount(cchBuf) LPWSTR pszBuf, 
    UINT cchBuf
    );

LPWSTR
WINAPI
_MakeGUIDString(
    GUID& Guid,
    LPWSTR pszBuf,
    int cchBuf
    );

HANDLE
WINAPI
OpenDisk(
    PCWSTR pszDeviceName,
    DWORD dwDiskNumber, /* OBSOLETE */
    DWORD dwDesired
    );

LONG
WINAPI
GetDiskDriveGeometryEx(
    HANDLE hDisk,
    PDISK_GEOMETRY_EX *pGeometry,
    ULONG *pcb
    );

BOOL
GetDiskDriveLayoutEx(
    HANDLE hDisk,
    PDRIVE_LAYOUT_INFORMATION_EX *DriveLayoutBuffer
    );

LONG
GetRecognitionInformation(
    HANDLE hVolume,
    FILE_SYSTEM_RECOGNITION_INFORMATION *RecognitionInfo
    );

LONG
GetRecognitionStructure(
    HANDLE hVolume,
    FILE_SYSTEM_RECOGNITION_STRUCTURE *RecognitionStructure
    );

LONG
GetRetrievalPointerBase(
    HANDLE hVolume,
    RETRIEVAL_POINTER_BASE *RetrievalPointerBase
    );

BOOL
GetDiskExtents(
    HANDLE hVolume,
    PVOLUME_DISK_EXTENTS *pDiskExrents,
    PDISK_GEOMETRY_EX **pDiskGeometryPtrArray,
    PDRIVE_LAYOUT_INFORMATION_EX **DriveLayoutInfoPtrArray
    );

BOOL
FreeDiskExtents(
    PVOLUME_DISK_EXTENTS pDiskExrents,
    PDISK_GEOMETRY_EX *pDiskGeometryPtrArray,
    PDRIVE_LAYOUT_INFORMATION_EX *DriveLayoutInfoPtrArray
    );

//
// Storage APIs
//
typedef struct _STORAGE_DEVICE_UNIQUE_IDENTIFIER {
    ULONG Version;
    ULONG Size;
    ULONG StorageDeviceIdOffset;
    ULONG StorageDeviceOffset;
    ULONG DriveLayoutSignatureOffset;
} STORAGE_DEVICE_UNIQUE_IDENTIFIER, *PSTORAGE_DEVICE_UNIQUE_IDENTIFIER;

LONG
WINAPI
StorageGetDeviceNumber(
    HANDLE hFile,
    PSTORAGE_DEVICE_NUMBER DeviceNumber
    );

LONG
WINAPI
StorageGetDeviceIdDescriptor(
    HANDLE hFile,
    PSTORAGE_DEVICE_DESCRIPTOR *pDeviceDescriptor
    );

DWORD
WINAPI
StorageDetectSectorSize(
    HANDLE hFile,
    PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR pAlignmentDescriptor
    );

ULONG
WINAPI
StorageMediumProductTypeDescriptor(
	HANDLE hDevice,
	LPDWORD pdwMediumProductType
	);

ULONG
WINAPI
StorageGetMediaTypes(
    HANDLE hFile,
    PGET_MEDIA_TYPES *pMediaTypes
    );

ULONG
WINAPI
StorageGetDeviceCapacity(
	HANDLE hDevice,
	PSTORAGE_READ_CAPACITY DeviceCapacity
	);

EXTERN_C
PVOID
WINAPI
StorageMemAlloc(
	SIZE_T cb
	);

EXTERN_C
PVOID
WINAPI
StorageMemReAlloc(
	PVOID ptr,
	SIZE_T cb
	);

ULONG
WINAPI
StorageMemFree(
	PVOID pv
	);

LONG
WINAPI
VolumeMemFree(
	PVOID pv
	);

PVOID
WINAPI
VolumeMemAlloc(
	SIZE_T cb
	);

PWSTR
WINAPI
VolumeDuplicateString(
	PWSTR psz
	);


//
// pBuffer
//	 - STORAGE_ADAPTER_DESCRIPTOR Win7
//	 - STCCTL::STORAGE_ADAPTER_DESCRIPTOR_EX Win8,Win10,Win11
//
ULONG
WINAPI
StorageAdapterDescriptor(
	HANDLE hDevice,
	PVOID pBuffer,
	ULONG cbBuffer
	);

ULONG
WINAPI
StorageDeviceIdDescriptor(
	HANDLE hDevice,
	STORAGE_DEVICE_ID_DESCRIPTOR **DeviceId
	);

//
// FSCTL wrapper functions
//
LONG
QueryOnDiskVolumeInfo(
    HANDLE hDevice,
    FILE_QUERY_ON_DISK_VOL_INFO_BUFFER *pudfDiskVolumeInfo
    );

LONG
GetNtfsVolumeData(
    HANDLE hVolume,
    NTFS_VOLUME_DATA_BUFFER *pNtfsVolumeData,
    ULONG cbNtfsVolumeData
    );

LONG
GetReFSVolumeData(
    HANDLE hVolume,
    REFS_VOLUME_DATA_BUFFER *pBuffer
    );

HRESULT
WINAPI
IsSetDirtyBit(
	HANDLE Handle
	);

//
// IOCTL wrapper functions
//
HRESULT
WINAPI
QueryDiskPerformance(
	PCWSTR pszDeviceName,
	DISK_PERFORMANCE *DiskPerf,
	INT cbDiskPerf
	);

//
// Legacy Product Compatible API
//

HRESULT
WINAPI
GetDeviceTypeByVolumeName(
    PCWSTR pszVolumeName,
    PULONG pDeviceType,
    PULONG pCharacteristics
    );

enum {
    CSTR_DEVICETYPE,
    CSTR_MEDIATYPE,
};

PWSTR
WINAPI
GetVolumeTypeString(
    UINT Type,
    ULONG Val,
    PWSTR pszText,
    int cchTextMax
    );

BOOL
WINAPI
GetDeviceCharacteristicsFlag(
    int iIndex,
    DWORD *pdwFlag
    );

PWSTR
WINAPI
GetMediaTypeString(
    ULONG Type,
    PWSTR pszType,
    int cchType,
    PWSTR pszDesc,
    int cchDesc,
    PWSTR pszName,
    int cchName
    );

PWSTR
WINAPI
GetMediaCharacteristicsString(
    DWORD Attributes,
    PWSTR pszBuf,
    int cchBuf
    );

INT
WINAPI
GetDeviceCharacteristicsString(
    int iIndex,
    PWSTR psz,
    DWORD cch
    );

BOOL
WINAPI
VolumeFs_GetVolumeAttributeFlag(
    int iIndex,
    DWORD *pdwFlag
    );

INT
WINAPI
GetVolumeAttributeString(
    int iIndex,
    PWSTR psz,
    DWORD cch
    );

DWORD
WINAPI
VirtualDisk_GetDependencyInformationByHandle(
    HANDLE hStorage,
    PSTORAGE_DEPENDENCY_INFO *ppStorageDependencyInfo
    );

BOOL
WINAPI
VirtualDisk_GetDependencyInformation(
    PCWSTR NtDevicePath,
    PSTORAGE_DEPENDENCY_INFO *ppStorageDependencyInfo
    );

#ifndef VIRTUAL_STORAGE_TYPE_DEVICE_VHDX
#define VIRTUAL_STORAGE_TYPE_DEVICE_VHDX  3
#endif

//
// MS-DOS drive information helper
//
typedef struct _DOS_DRIVE_INFORMATION {
    WCHAR szDrive[3];                       // "C:"
    WCHAR szDriveRoot[4];                   // "C:\"
    UINT DriveType;
    ULARGE_INTEGER FreeBytesAvailable;
    ULARGE_INTEGER TotalNumberOfBytes;
    ULARGE_INTEGER TotalNumberOfFreeBytes;
    DWORD VolumeSerialNumber;
    DWORD MaximumComponentLength;
    DWORD FileSystemFlags;
    struct {
        ULONG Information : 1;
        ULONG Size : 1;
    } State;
    WCHAR szVolumeName[50];               // "\\?\Volume{xxxxxxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxx}\"
    PWSTR Device;
    PWSTR VolumeLabel;                    
    PWSTR FileSystemName;
	CHAR DirtyBit;
	BOOLEAN VirtualDiskVolume;
} DOS_DRIVE_INFORMATION;

typedef struct _DOS_DRIVE_INFORMATION_ARRAY {
    ULONG Count;
    DOS_DRIVE_INFORMATION Drive[1];
} DOS_DRIVE_INFORMATION_ARRAY;

HRESULT
WINAPI
EnumDosDriveItems(
    DOS_DRIVE_INFORMATION_ARRAY **DosDrivesTable
    );

HRESULT
WINAPI
FreeDosDriveItems(
    DOS_DRIVE_INFORMATION_ARRAY *DosDrivesTable
    );

HRESULT
WINAPI
GetVolumeDrivePathsString(
    PWCHAR VolumeName,
    PWSTR Paths,
    DWORD cchPaths
    );

//
// API Function that get the volume information at once all.
//
HRESULT
WINAPI
CreateVolumeInformationBuffer(
    PCWSTR VolumeName,
    ULONG InformationClass,
    ULONG Reserved,
    PVOID *InformaionBuffer
    );

#define OPEN_VOLUME_READ_DATA   OPEN_READ_DATA

HRESULT
WINAPI
DestroyVolumeInformationBuffer(
    PVOID InformaionBuffer
    );

//
// Statistics
//
HRESULT
WINAPI
GetStatisticsData(
	PCWSTR pszVolumeRoot,
	PBYTE *Buffer,
	SIZE_T *BufferSize,
	PFILESYSTEM_STATISTICS **StatisticsPtrArray,
	SIZE_T *StatisticsPtrArraySize,
	PULONG StatisticsPtrArrayCount,
	PFILESYSTEM_STATISTICS_EX *StatisticsEx,
	SIZE_T *StatisticsExSize
	);

HRESULT
WINAPI
FreeStatisticsData(
	PBYTE Buffer,
	PFILESYSTEM_STATISTICS *StatisticsPtrArray,
	PFILESYSTEM_STATISTICS_EX StatisticsEx
	);

#define GetNTFSStatistics(p) ((NTFS_STATISTICS *)((PBYTE)p + sizeof(FILESYSTEM_STATISTICS)))
#define GetNTFSStatisticsEx(p) ((NTFS_STATISTICS_EX *)((PBYTE)p + sizeof(FILESYSTEM_STATISTICS_EX)))

#define GetFATStatistics(p) ((FAT_STATISTICS *)((PBYTE)p + sizeof(FILESYSTEM_STATISTICS)))
#define GetFATStatisticsEx(p) ((FAT_STATISTICS *)((PBYTE)p + sizeof(FILESYSTEM_STATISTICS_EX)))

#define GetExFATStatistics(p) ((EXFAT_STATISTICS *)((PBYTE)p + sizeof(FILESYSTEM_STATISTICS)))
#define GetExFATStatisticsEx(p) ((EXFAT_STATISTICS *)((PBYTE)p + sizeof(FILESYSTEM_STATISTICS_EX)))

HRESULT
WINAPI
CalcStatisticsDiffEx(
	PVOID pDiff, 
	PVOID pData1, 
	PVOID pData2
	);

HRESULT
WINAPI
CalcStatisticsDiffNtfsEx(
	PVOID pDiff, 
	PVOID pData1, 
	PVOID pData2
	);

//
// Usn Change Journal Information
//
EXTERN_C
HRESULT
NTAPI
GetVolumeUsnJornalDataInformation(
	HANDLE Handle,
	__in VOLUME_FS_USN_JOURNAL_DATA *QuataInfoList,
	__inout ULONG *pcbQuataInfoList
	);

#ifdef __cplusplus
}
#endif

