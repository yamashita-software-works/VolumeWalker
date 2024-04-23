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
#define OPEN_BACKUP_INTENT 0x4

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
    VOLFS_LABEL_INFORMATION = 2,
    VOLFS_SIZE_INFORMATION = 3,
	VOLFS_DEVICE_INFORMATION = 4,
    VOLFS_ATTRIBUTE_INFORMATION = 5,
	VOLFS_CONTROL_INFORMATION = 6,
	VOLFS_FULL_SIZE_INFORMATION = 7,
    VOLFS_OBJECTID_INFORMATION = 8,
	VOLFS_DRIVER_PATH_INFORMATION = 9,
    VOLFS_VOLUME_FLAGS_INFORMATION = 10,
    VOLFS_SECTOR_SIZE_INFORMATION = 11,
	VOLFS_DATA_COPY_INFORMATION = 12,
	VOLFS_METADATA_SIZE_INFORMATION = 13,
	VOLFS_FULL_SIZE_INFORMATION_EX = 14,
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

typedef struct _VOLUME_FS_CONTROL_INFORMATION
{
    LARGE_INTEGER FreeSpaceStartFiltering;
    LARGE_INTEGER FreeSpaceThreshold;
    LARGE_INTEGER FreeSpaceStopFiltering;
    LARGE_INTEGER DefaultQuotaThreshold;
    LARGE_INTEGER DefaultQuotaLimit;
    ULONG FileSystemControlFlags;
} VOLUME_FS_CONTROL_INFORMATION, *PVOLUME_FS_CONTROL_INFORMATION;

#ifndef FILE_VC_QUOTA_NONE
#define FILE_VC_QUOTA_NONE                  0x00000000
#define FILE_VC_QUOTA_TRACK                 0x00000001
#define FILE_VC_QUOTA_ENFORCE               0x00000002
#define FILE_VC_QUOTA_MASK                  0x00000003
#define FILE_VC_CONTENT_INDEX_DISABLED      0x00000008
#define FILE_VC_LOG_QUOTA_THRESHOLD         0x00000010
#define FILE_VC_LOG_QUOTA_LIMIT             0x00000020
#define FILE_VC_LOG_VOLUME_THRESHOLD        0x00000040
#define FILE_VC_LOG_VOLUME_LIMIT            0x00000080
#define FILE_VC_QUOTAS_INCOMPLETE           0x00000100
#define FILE_VC_QUOTAS_REBUILDING           0x00000200
#define FILE_VC_VALID_MASK                  0x000003ff
#endif

typedef struct _QUOTA_INFORMATION
{
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER QuotaUsed;
    LARGE_INTEGER QuotaThreshold;
    LARGE_INTEGER QuotaLimit;
    ULONG SidLength;
    PSID Sid;
} QUOTA_INFORMATION;

typedef struct _VOLUME_FS_QUOTA_INFORMATION_LIST
{
    ULONG ItemCount;
    QUOTA_INFORMATION QuataUser[1];
} VOLUME_FS_QUOTA_INFORMATION_LIST;

EXTERN_C
HRESULT
NTAPI
GetQuotaInformation(
	HANDLE Handle,
	VOLUME_FS_QUOTA_INFORMATION_LIST **QuataInfoList
	);

EXTERN_C
HRESULT
NTAPI
FreeQuotaInformation(
	VOLUME_FS_QUOTA_INFORMATION_LIST *QuataInfoList
	);

typedef struct _VOLUME_FS_SECTOR_SIZE_INFORMATION
{
    ULONG LogicalBytesPerSector;
    ULONG PhysicalBytesPerSectorForAtomicity;
    ULONG PhysicalBytesPerSectorForPerformance;
    ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
    ULONG Flags;
    ULONG ByteOffsetForSectorAlignment;
    ULONG ByteOffsetForPartitionAlignment;
} VOLUME_FS_SECTOR_SIZE_INFORMATION, *PVOLUME_FS_SECTOR_SIZE_INFORMATION;

#ifndef SSINFO_FLAGS_ALIGNED_DEVICE
#define SSINFO_FLAGS_ALIGNED_DEVICE                 0x00000001
#define SSINFO_FLAGS_PARTITION_ALIGNED_ON_DEVICE    0x00000002
#define SSINFO_FLAGS_NO_SEEK_PENALTY                0x00000004
#define SSINFO_FLAGS_TRIM_ENABLED                   0x00000008
#define SSINFO_FLAGS_BYTE_ADDRESSABLE               0x00000010
#define SSINFO_OFFSET_UNKNOWN (0xffffffff)
#endif

//
// USN Journal Information Structure
//
typedef struct {
	DWORDLONG UsnJournalID;
	USN       FirstUsn;
	USN       NextUsn;
	USN       LowestValidUsn;
	USN       MaxUsn;
	DWORDLONG MaximumSize;
	DWORDLONG AllocationDelta;
	USHORT    MinSupportedMajorVersion;
	USHORT    MaxSupportedMajorVersion;
	ULONG     Flags;
	DWORDLONG RangeTrackChunkSize;
	LONGLONG  RangeTrackFileSizeThreshold;
} VOLUME_FS_USN_JOURNAL_DATA, *PVOLUME_FS_USN_JOURNAL_DATA;
