#pragma once
//
// volumeinfo.h
//
// note:
// This file is toplevel header file to be include for Win32 source code. 
// Ported code from fsworkbench's dll.
//
#include <virtdisk.h>
#include <winioctl.h>
#include "ntvolumenames.h"
#include "storagedevice.h"
#include "volumedevinfostruct.h"
#include "newfsctldef.h"

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
    DWORD dwDiskNumber,
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

LONG
StorageGetMediaTypes(
    HANDLE hFile,
    PGET_MEDIA_TYPES *pMediaTypes
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
VirtualDiskGetDependencyInformation(
    HANDLE hStorage,
    PSTORAGE_DEPENDENCY_INFO *ppStorageInfo
    );

DWORD
WINAPI
VirtualDiskIsVirtualDiskVolume(
    PCWSTR NtDevicePath,
    PSTORAGE_DEPENDENCY_INFO *ppsdi
    );

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
GetherVolumeInformation(
    PCWSTR VolumeName,
    ULONG InformationClass,
    ULONG Reserved,
    PVOID *InformaionBuffer
    );

#define OPEN_VOLUME_READ_DATA   OPEN_READ_DATA

HRESULT
WINAPI
FreeVolumeInformation(
    PVOID InformaionBuffer
    );

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
//
// Physical Disk Information Data
//
struct CPhysicalDriveInformation
{
    DRIVE_LAYOUT_INFORMATION_EX *pDriveLayout;    // Process Heap
    PSTORAGE_DEVICE_DESCRIPTOR pDeviceDescriptor; // LocalAlloc
    DISK_GEOMETRY_EX *pGeometry;                  // LocalAlloc -> _MemAPI
    STORAGE_READ_CAPACITY DeviceCapacity; 
    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR Alignment;
    DWORD m_dwDiskNumber;

    // Reference pointer (need not free memory)
    PDISK_DETECTION_INFO Detection;
    PDISK_PARTITION_INFO Partition;

    HANDLE m_hDisk;

    CPhysicalDriveInformation()
    {
        memset(this,0,sizeof(CPhysicalDriveInformation));
        m_hDisk = NULL;
        pDriveLayout = NULL;
        pDeviceDescriptor = NULL;
        pGeometry = NULL;
        m_dwDiskNumber = (DWORD)-1;
    }

    ~CPhysicalDriveInformation()
    {
        if( pDriveLayout )
            Free(pDriveLayout);
        if( pDeviceDescriptor )
            LocalFree(pDeviceDescriptor);
        if( pGeometry )
            _MemFree(pGeometry); // LocalFree(pGeometry);
        CloseDiskHandle();
    }

    VOID CloseDiskHandle()
    {
        if( m_hDisk )
        {
            CloseHandle(m_hDisk);
            m_hDisk = NULL;
        }
    }

    // memory helper
    //
    PVOID Alloc(ULONG cb)
    {
        return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,cb);
    }

    PVOID Shrink(PVOID pv,ULONG cb)
    {
        return HeapReAlloc(GetProcessHeap(),0, pv, cb);
    }

    VOID Free(PVOID pv)
    {
        HeapFree(GetProcessHeap(),0,pv);
    }

    // Physical Disk APIs
    //
    HRESULT OpenDisk(PCWSTR pszDeviceName,DWORD dwDiskNumber=0,DWORD dwDesired=0)
    {
        if( m_hDisk != NULL )
            CloseDiskHandle();

        m_dwDiskNumber = dwDiskNumber;

        m_hDisk = ::OpenDisk(pszDeviceName,dwDiskNumber,dwDesired);

        return m_hDisk != INVALID_HANDLE_VALUE ? S_OK : HRESULT_FROM_WIN32( GetLastError() );
    }

    HRESULT GetGeometry(HANDLE hDisk=NULL)
    {
        ULONG cb;

        if( hDisk == NULL )
            hDisk = m_hDisk;

        GetDiskDriveGeometryEx(hDisk,&pGeometry,&cb);

        ULONG cbBasic = sizeof(DISK_GEOMETRY) + sizeof(LARGE_INTEGER);
        if( cb > cbBasic )
        {
            Detection = DiskGeometryGetDetect( pGeometry );
            Partition = DiskGeometryGetPartition( pGeometry );
        }

        return S_OK;
    }

    HRESULT GetDriveLayout(HANDLE hDisk=NULL,DWORD numDiskCount=512)
    {
        HRESULT hr = E_FAIL;

        if( hDisk == NULL )
            hDisk = m_hDisk;

        DWORD dwOutBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * numDiskCount;
        pDriveLayout = (DRIVE_LAYOUT_INFORMATION_EX*)Alloc(dwOutBufferSize);

        if( pDriveLayout )
        {
            DWORD BytesReturned;;
            if( DeviceIoControl(hDisk,
                          IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                          NULL,0,
                          pDriveLayout,dwOutBufferSize,
                          &BytesReturned,
                          NULL) )
            {
                Shrink(pDriveLayout,BytesReturned);

                hr = S_OK;
            }
        }

        return hr;
    }

    HRESULT GetDeviceIdDescriptor(HANDLE hDisk=NULL)
    {
        if( hDisk == NULL )
            hDisk = m_hDisk;

        HRESULT hr = E_FAIL;
        if( StorageGetDeviceIdDescriptor(hDisk,&pDeviceDescriptor) == NO_ERROR )
        {
            hr = S_OK;
        }
        return hr;
    }

    HRESULT GetDetectSectorSize(HANDLE hDisk=NULL)
    {
        if( hDisk == NULL )
            hDisk = m_hDisk;

        DWORD dwError;

        if( (dwError = StorageDetectSectorSize(hDisk,&Alignment)) != NO_ERROR )
        {
            Alignment.Version = (DWORD)-1;
            Alignment.Size = (DWORD)dwError;
            return HRESULT_FROM_WIN32(dwError);
        }

        return S_OK;
    }

    BOOL IsValidAlignment()
    {
        return (Alignment.Version != (DWORD)-1 && Alignment.Version != (DWORD)0);
    }
};
#endif
