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

//
// Storage Properties
//
namespace STGCTL {

typedef enum _STORAGE_PROPERTY_ID_EX {
#if 0
  StorageDeviceProperty = 0,
  StorageAdapterProperty,
  StorageDeviceIdProperty,
  StorageDeviceUniqueIdProperty,
  StorageDeviceWriteCacheProperty,
  StorageMiniportProperty,
  StorageAccessAlignmentProperty,
  StorageDeviceSeekPenaltyProperty,        // Win7
  StorageDeviceTrimProperty,               // Win7
  StorageDeviceWriteAggregationProperty,   // Win7
#endif
  StorageDeviceDeviceTelemetryProperty=10, // Win8
  StorageDeviceLBProvisioningProperty,     // Win8
  StorageDevicePowerProperty,              // Win8
  StorageDeviceCopyOffloadProperty,        // Win8
  StorageDeviceResiliencyProperty,
  StorageDeviceMediumProductType,
  StorageAdapterRpmbProperty,
  StorageAdapterCryptoProperty,
  StorageDeviceIoCapabilityProperty = 48,
  StorageAdapterProtocolSpecificProperty,
  StorageDeviceProtocolSpecificProperty,
  StorageAdapterTemperatureProperty,
  StorageDeviceTemperatureProperty,
  StorageAdapterPhysicalTopologyProperty,
  StorageDevicePhysicalTopologyProperty,
  StorageDeviceAttributesProperty,
  StorageDeviceManagementStatus,
  StorageAdapterSerialNumberProperty,
  StorageDeviceLocationProperty,
  StorageDeviceNumaProperty,
  StorageDeviceZonedDeviceProperty,
  StorageDeviceUnsafeShutdownCount,
  StorageDeviceEnduranceProperty,
  StorageDeviceLedStateProperty,
  StorageDeviceSelfEncryptionProperty = 64,
  StorageFruIdProperty,
  StorageStackProperty
} STORAGE_PROPERTY_ID_EX, *PSTORAGE_PROPERTY_ID_EX;

};

typedef struct _STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR
{
    DWORD Version; // sizeof(STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR)
    DWORD Size;    // 
    DWORD MediumProductType;
} STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR, *PSTORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR;

/*++
00h         Not indicated
01h         CFast
02h         CompactFlash
03h         Memory Stick
04h         MultiMediaCard
05h         Secure Digital Card (SD Card)
06h         QXD
07h         Universal Flash Storage
08h to EFh 	Reserved
F0h to FFh 	Vendor-specific
--*/
enum {
    MediumProductType_NotIndicated=0x00,
    MediumProductType_CFast=0x01,
    MediumProductType_CompactFlash=0x02,
    MediumProductType_MemoryStick=0x03,
    MediumProductType_MultiMediaCard=0x04,
    MediumProductType_MMC=MediumProductType_MultiMediaCard,
    MediumProductType_SecureDigitalCard=0x05,
    MediumProductType_SDCard=MediumProductType_SecureDigitalCard,
    MediumProductType_SDCard_QXD=0x06,
    MediumProductType_UniversalFlashStorage=0x07,
};

namespace STGCTL
{
typedef enum _STORAGE_PORT_CODE_SET {
#if 0
    StoragePortCodeSetReserved  = 0,
    StoragePortCodeSetStorport  = 1,
    StoragePortCodeSetSCSIport  = 2,
#endif
    StoragePortCodeSetSpaceport = 3,
    StoragePortCodeSetATAport   = 4,
    StoragePortCodeSetUSBport   = 5,
    StoragePortCodeSetSBP2port  = 6,
    StoragePortCodeSetSDport    = 7
} STORAGE_PORT_CODE_SET, *PSTORAGE_PORT_CODE_SET;
};

namespace STGCTL
{
typedef struct _STORAGE_ADAPTER_DESCRIPTOR_EX
{
    DWORD Version;
    DWORD Size;
    DWORD MaximumTransferLength;
    DWORD MaximumPhysicalPages;
    DWORD AlignmentMask;
    BOOLEAN AdapterUsesPio;
    BOOLEAN AdapterScansDown;
    BOOLEAN CommandQueueing;
    BOOLEAN AcceleratedTransfer;
    BYTE  BusType;
    WORD  BusMajorVersion;
    WORD  BusMinorVersion;
    BYTE  SrbType;        // NTDDI_WIN8
    BYTE  AddressType;    // NTDDI_WIN8
} STORAGE_ADAPTER_DESCRIPTOR_EX, *PSTORAGE_ADAPTER_DESCRIPTOR_EX;
};
