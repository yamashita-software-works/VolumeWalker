#pragma once

//
// Physical Device
//
typedef struct _FS_STORAGE_PHYSICAL_DEVICE_OBJECT
{
	PWSTR NtObjectName;
	PWSTR VolumeName;
	WCHAR DosDrive[ 2 * 26 + 1 ]; // ex) "C:D:E:Q:"
} FS_STORAGE_PHYSICAL_DEVICE_OBJECT;

typedef struct _FS_STORAGE_DEVICE_NAME
{
	PWSTR FriendlyName;
	PWSTR Location;
	PWSTR NtDeviceName;
	PWSTR DeviceId;
	ULONG PhysicalDeviceObjectCount;
	FILETIME ftInstallDate; // vista
	FS_STORAGE_PHYSICAL_DEVICE_OBJECT **PhysicalDeviceObject;
} FS_STORAGE_DEVICE_NAME;

typedef struct _FS_STORAGE_DEVICE_NAME_LIST
{
	ULONG StorageDeviceCount;
	FS_STORAGE_DEVICE_NAME StorageDevice[1];
} FS_STORAGE_DEVICE_NAME_LIST;

EXTERN_C
HRESULT
WINAPI
CreateDeviceLocationList(
	FS_STORAGE_DEVICE_NAME_LIST **ppStorageDeviceNames
	);


// Mounted device identifier to the volume.
typedef struct _FS_VOL_MOUNTED_DEVICE
{
	PWSTR VolumeName;
	USHORT DataType;
	union
	{
		PWSTR VolMgrSymbolicLinkName;
		struct {
			ULONG Signiture;
			LARGE_INTEGER StartOffset;
		} VolMgrLocationInfo;
		GUID PartitionGuid;
	};
} FS_VOL_MOUNTED_DEVICE;

#define FS_VMDT_NT4INFO      0x0
#define FS_VMDT_WCHAR        0x1
#define FS_VMDT_GPT_PARTITION 0x2 

// 'DMIO:ID:'
#define _IS_DMIO_SIGNATURE(d) (  (((char *)d)[0] == 'D') && \
                                 (((char *)d)[1] == 'M') && \
                                 (((char *)d)[2] == 'I') && \
                                 (((char *)d)[3] == 'O') && \
                                 (((char *)d)[4] == ':') && \
                                 (((char *)d)[5] == 'I') && \
                                 (((char *)d)[6] == 'D') && \
                                 (((char *)d)[7] == ':') )

typedef struct _FS_VOL_MOUNTED_DEVICE_LIST
{
	ULONG Count;
	FS_VOL_MOUNTED_DEVICE Device[1];
} FS_VOL_MOUNTED_DEVICE_LIST;


EXTERN_C
ULONG
WINAPI
GetMountedDeviceList(
	FS_VOL_MOUNTED_DEVICE_LIST **ppMountedDevices
	);

EXTERN_C
ULONG
WINAPI
FreeMountedDeviceList(
	FS_VOL_MOUNTED_DEVICE_LIST *pMountedDevices
	);

EXTERN_C
ULONG
WINAPI
GetMountedDeviceInfo(
	PCWSTR pszVolumeName,
	FS_VOL_MOUNTED_DEVICE *pDeviceInfo
	);

EXTERN_C
ULONG
WINAPI
FreeMountedDeviceInfo(
	FS_VOL_MOUNTED_DEVICE *pDeviceInfo
	);

//
// Storage Device Information
//

typedef struct _FS_HARDWARE_PRODUCT
{
	PWSTR InstanceId;
	PWSTR HardwareIds;
	PWSTR FriendlyName;
	PWSTR DeviceDesc;
	ULONG InstallState;
	ULONG DevNodeStatus;
	FILETIME InstallDate;
	FILETIME FirstInstallDate;
	PWSTR RemovalRelations;
	PWSTR PysicalDeviceObjectName;
} FS_HARDWARE_PRODUCT;

#define INVALID_PROPERTY_ULONG_VALUE ((ULONG)-1)

EXTERN_C
BOOL
WINAPI
GetKnownHardwareProducts(
	HANDLE *phProductList,
	const GUID *DevGuid,
	ULONG OptionFlags
	);

EXTERN_C
BOOL
WINAPI
FreeKnownHardwareProducts(
	HANDLE hProductList
	);

EXTERN_C
ULONG
WINAPI
GetKnownHardwareProductsCount(
	HANDLE hProductList
	);

EXTERN_C
BOOL
WINAPI
GetKnownHardwareProductPointer(
	HANDLE hProductList,
	ULONG Index,
	const FS_HARDWARE_PRODUCT **pProductInfo
	);

EXTERN_C
LONG
WINAPI
FindVolumeObjectPath(
	PCWSTR pszDeviceInstanceId,
	PWSTR pszVolume,
	DWORD cchVolume
	);
