//***************************************************************************
//*                                                                         *
//*  volumehelp.cpp                                                         *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create:  2022-04-02 created.                                           *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "dllmain.h"
#include "volumehelp.h"
#include "win32wdkdef.h"
#include "ntvolumehelp.h"
#include "ntnativehelp.h"
#include "storagedevice.h"
using namespace STGCTL;

#define INITGUID
#include <guiddef.h>
#include <diskguid.h>

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////////

EXTERN_C
PVOID
WINAPI
VolumeMemAlloc(
	SIZE_T cb
	)
{
	return AllocMemory(cb);
}

EXTERN_C
PWSTR
WINAPI
VolumeDuplicateString(
	PWSTR psz
	)
{
	return DuplicateString(psz);
}

EXTERN_C
LONG
WINAPI
VolumeMemFree(
	PVOID ptr
	)
{
	if( ptr != NULL )
	{
		FreeMemory(ptr);
	}
	return 0;
}

EXTERN_C
PVOID
WINAPI
StorageMemAlloc(
	SIZE_T cb
	)
{
	return VolumeMemAlloc(cb);
}

EXTERN_C
PVOID
WINAPI
StorageMemReAlloc(
	PVOID ptr,
	SIZE_T cb
	)
{
	return ReallocMemory(ptr,cb);
}

EXTERN_C
ULONG
WINAPI
StorageMemFree(
	PVOID ptr
	)
{
	return (ULONG)VolumeMemFree(ptr);
}

//////////////////////////////////////////////////////////////////////////////

typedef struct _FS_VOLUME_ATTIBUTE_DESCRIPTION
{
	ULONG Flag;
	PCWSTR Name;
} FS_VOLUME_ATTIBUTE_DESCRIPTION;

#define _FS_VOLUME_ATTIBUTE_DESCRIPTION(f)  { f, L#f }

FS_VOLUME_ATTIBUTE_DESCRIPTION attrDesc[] = {
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_CASE_SENSITIVE_SEARCH ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_CASE_PRESERVED_NAMES ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_UNICODE_ON_DISK ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_PERSISTENT_ACLS ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_READ_ONLY_VOLUME ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_VOLUME_IS_COMPRESSED ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_RETURNS_CLEANUP_RESULT_INFO ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_DAX_VOLUME ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_NAMED_STREAMS ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_OBJECT_IDS ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_FILE_COMPRESSION ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_ENCRYPTION ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_TRANSACTIONS ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_EXTENDED_ATTRIBUTES ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_OPEN_BY_FILE_ID ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_HARD_LINKS ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_REPARSE_POINTS ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SEQUENTIAL_WRITE_ONCE ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_USN_JOURNAL ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_VOLUME_QUOTAS ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_SPARSE_FILES ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_REMOTE_STORAGE ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_INTEGRITY_STREAMS ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_POSIX_UNLINK_RENAME ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_SPARSE_VDL ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_GHOSTING ),
	_FS_VOLUME_ATTIBUTE_DESCRIPTION( FILE_SUPPORTS_BLOCK_REFCOUNTING ),
};

BOOL GetVolumeAttributeFlag(int iIndex,DWORD *pdwFlag)
{
	if( iIndex < 0 || iIndex >= _countof(attrDesc) )
		return FALSE;

	if( pdwFlag == NULL )
		return FALSE;

	*pdwFlag = attrDesc[iIndex].Flag;

	return TRUE;
}

int	WINAPI GetVolumeAttributeString(int iIndex,PWSTR psz,DWORD cch)
{
	if( iIndex < 0 || iIndex >= _countof(attrDesc) )
		return 0;

	if( psz == NULL )
		return 0;

	StringCchCopy(psz,cch,attrDesc[iIndex].Name);

	return (int)wcslen(psz);
}

//////////////////////////////////////////////////////////////////////////////

typedef struct _FS_FS_DEVICE_CHARACTERISTICS_NAME
{
	ULONG Flag;
	PWSTR Name;
} FS_DEVICE_CHARACTERISTICS_NAME;

#define _DEF_FS_DEVICE_CHARACTERISTICS_ITEM(f)  { f, L#f  }

FS_DEVICE_CHARACTERISTICS_NAME DeviceCharacteristics[] = {
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_DEVICE_IS_MOUNTED ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_REMOVABLE_MEDIA ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_READ_ONLY_DEVICE ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_WRITE_ONCE_MEDIA ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_PORTABLE_DEVICE ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_REMOTE_DEVICE ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_FLOPPY_DISKETTE ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_VIRTUAL_VOLUME ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_AUTOGENERATED_DEVICE_NAME ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_DEVICE_SECURE_OPEN ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_CHARACTERISTIC_PNP_DEVICE ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_CHARACTERISTIC_TS_DEVICE ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_CHARACTERISTIC_WEBDAV_DEVICE ),
	_DEF_FS_DEVICE_CHARACTERISTICS_ITEM( FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL ),
};

BOOL WINAPI GetDeviceCharacteristicsFlag(int iIndex,DWORD *pdwFlag)
{
	if( iIndex < 0 || iIndex >= _countof(DeviceCharacteristics) )
		return FALSE;

	if( pdwFlag == NULL )
		return FALSE;

	*pdwFlag = DeviceCharacteristics[iIndex].Flag;

	return TRUE;
}

INT	WINAPI GetDeviceCharacteristicsString(int iIndex,PWSTR psz,DWORD cch)
{
	if( iIndex < 0 || iIndex >= _countof(attrDesc) )
		return 0;

	if( psz == NULL )
		return 0;

	StringCchCopy(psz,cch,DeviceCharacteristics[iIndex].Name);

	return (int)wcslen(psz);
}

//////////////////////////////////////////////////////////////////////////////

typedef struct _FS_FILE_DEVICE_TYPE_NAME
{
	ULONG DeviceType;
	WCHAR *DeviceTypeName;
} FS_FILE_DEVICE_TYPE_NAME;

#define DEVICE_TYPE_DEF(x)  {x, L#x}

#define FILE_DEVICE_MT_COMPOSITE        0x00000042
#define FILE_DEVICE_MT_TRANSPORT        0x00000043
#define FILE_DEVICE_BIOMETRIC	        0x00000044
#define FILE_DEVICE_PMI                 0x00000045

FS_FILE_DEVICE_TYPE_NAME g_DeviceType[] = 
{
	DEVICE_TYPE_DEF( FILE_DEVICE_BEEP ),
	DEVICE_TYPE_DEF( FILE_DEVICE_CD_ROM ),
	DEVICE_TYPE_DEF( FILE_DEVICE_CD_ROM_FILE_SYSTEM ),
	DEVICE_TYPE_DEF( FILE_DEVICE_CONTROLLER ),
	DEVICE_TYPE_DEF( FILE_DEVICE_DATALINK ),
	DEVICE_TYPE_DEF( FILE_DEVICE_DFS ),
	DEVICE_TYPE_DEF( FILE_DEVICE_DISK ),
	DEVICE_TYPE_DEF( FILE_DEVICE_DISK_FILE_SYSTEM ),
	DEVICE_TYPE_DEF( FILE_DEVICE_FILE_SYSTEM ),
	DEVICE_TYPE_DEF( FILE_DEVICE_INPORT_PORT ),
	DEVICE_TYPE_DEF( FILE_DEVICE_KEYBOARD ),
	DEVICE_TYPE_DEF( FILE_DEVICE_MAILSLOT ),
	DEVICE_TYPE_DEF( FILE_DEVICE_MIDI_IN ),
	DEVICE_TYPE_DEF( FILE_DEVICE_MIDI_OUT ),
	DEVICE_TYPE_DEF( FILE_DEVICE_MOUSE ),
	DEVICE_TYPE_DEF( FILE_DEVICE_MULTI_UNC_PROVIDER ),
	DEVICE_TYPE_DEF( FILE_DEVICE_NAMED_PIPE ),
	DEVICE_TYPE_DEF( FILE_DEVICE_NETWORK ),
	DEVICE_TYPE_DEF( FILE_DEVICE_NETWORK_BROWSER ),
	DEVICE_TYPE_DEF( FILE_DEVICE_NETWORK_FILE_SYSTEM ),
	DEVICE_TYPE_DEF( FILE_DEVICE_NULL ),
	DEVICE_TYPE_DEF( FILE_DEVICE_PARALLEL_PORT ),
	DEVICE_TYPE_DEF( FILE_DEVICE_PHYSICAL_NETCARD ),
	DEVICE_TYPE_DEF( FILE_DEVICE_PRINTER ),
	DEVICE_TYPE_DEF( FILE_DEVICE_SCANNER ),
	DEVICE_TYPE_DEF( FILE_DEVICE_SERIAL_MOUSE_PORT ),
	DEVICE_TYPE_DEF( FILE_DEVICE_SERIAL_PORT ),
	DEVICE_TYPE_DEF( FILE_DEVICE_SCREEN ),
	DEVICE_TYPE_DEF( FILE_DEVICE_SOUND ),
	DEVICE_TYPE_DEF( FILE_DEVICE_STREAMS ),
	DEVICE_TYPE_DEF( FILE_DEVICE_TAPE ),
	DEVICE_TYPE_DEF( FILE_DEVICE_TAPE_FILE_SYSTEM ),
	DEVICE_TYPE_DEF( FILE_DEVICE_TRANSPORT ),
	DEVICE_TYPE_DEF( FILE_DEVICE_UNKNOWN ),
	DEVICE_TYPE_DEF( FILE_DEVICE_VIDEO ),
	DEVICE_TYPE_DEF( FILE_DEVICE_VIRTUAL_DISK ),
	DEVICE_TYPE_DEF( FILE_DEVICE_WAVE_IN ),
	DEVICE_TYPE_DEF( FILE_DEVICE_WAVE_OUT ),
	DEVICE_TYPE_DEF( FILE_DEVICE_8042_PORT ),
	DEVICE_TYPE_DEF( FILE_DEVICE_NETWORK_REDIRECTOR ),
	DEVICE_TYPE_DEF( FILE_DEVICE_BATTERY ),
	DEVICE_TYPE_DEF( FILE_DEVICE_BUS_EXTENDER ),
	DEVICE_TYPE_DEF( FILE_DEVICE_MODEM ),
	DEVICE_TYPE_DEF( FILE_DEVICE_VDM ),
	DEVICE_TYPE_DEF( FILE_DEVICE_MASS_STORAGE ),
	DEVICE_TYPE_DEF( FILE_DEVICE_SMB ),
	DEVICE_TYPE_DEF( FILE_DEVICE_KS ),
	DEVICE_TYPE_DEF( FILE_DEVICE_CHANGER ),
	DEVICE_TYPE_DEF( FILE_DEVICE_SMARTCARD ),
	DEVICE_TYPE_DEF( FILE_DEVICE_ACPI ),
	DEVICE_TYPE_DEF( FILE_DEVICE_DVD ),
	DEVICE_TYPE_DEF( FILE_DEVICE_FULLSCREEN_VIDEO ),
	DEVICE_TYPE_DEF( FILE_DEVICE_DFS_FILE_SYSTEM ),
	DEVICE_TYPE_DEF( FILE_DEVICE_DFS_VOLUME ),
	DEVICE_TYPE_DEF( FILE_DEVICE_SERENUM ),
	DEVICE_TYPE_DEF( FILE_DEVICE_TERMSRV ),
	DEVICE_TYPE_DEF( FILE_DEVICE_KSEC ),
	DEVICE_TYPE_DEF( FILE_DEVICE_FIPS ),
	DEVICE_TYPE_DEF( FILE_DEVICE_INFINIBAND ),
	DEVICE_TYPE_DEF( FILE_DEVICE_VMBUS ),
	DEVICE_TYPE_DEF( FILE_DEVICE_CRYPT_PROVIDER ),
	DEVICE_TYPE_DEF( FILE_DEVICE_WPD ),
	DEVICE_TYPE_DEF( FILE_DEVICE_BLUETOOTH ),
	DEVICE_TYPE_DEF( FILE_DEVICE_MT_COMPOSITE ),
	DEVICE_TYPE_DEF( FILE_DEVICE_MT_TRANSPORT ),
	DEVICE_TYPE_DEF( FILE_DEVICE_BIOMETRIC ),
	DEVICE_TYPE_DEF( FILE_DEVICE_PMI )
};

int GetDeviceTypeString(DWORD DeviceType,PWSTR pszText,int cchTextMax)
{
	if( DeviceType != 0 && DeviceType <= _countof(g_DeviceType) )
	{
		StringCchCopy(pszText,cchTextMax,g_DeviceType[ DeviceType - 1 ].DeviceTypeName);
		return (int)wcslen(pszText);
	}
	return 0;
}

LPCWSTR WINAPI GetPartitionStyleText(LONG PartitionStyle)
{
	switch( PartitionStyle )
	{
		case PARTITION_STYLE_MBR:
			return L"MBR";
		case PARTITION_STYLE_GPT:
			return L"GPT";
		case PARTITION_STYLE_RAW:
			return L"RAW";
	}

	return L"???";
}

LPCWSTR WINAPI GetPartitionTypeText(LONG PartitionType)
{
	switch( PartitionType & ~VALID_NTFT ) // ~0xC0
	{
		case PARTITION_ENTRY_UNUSED:
			return L"UNUSED"; // 0x00 An unused entry partition.
 
		case PARTITION_EXTENDED:
			return L"EXTENDED"; // 0x05 An extended partition.
 
		case PARTITION_FAT_12:
			return L"FAT12"; // 0x01 A FAT12 file system partition.
 
		case PARTITION_FAT_16:
			return L"FAT16"; // 0x04 A FAT16 file system partition.
 
		case PARTITION_FAT32:
			return L"FAT32"; // 0x0B A FAT32 file system partition.
 
		case PARTITION_FAT32_XINT13:
			return L"FAT32 XINT13"; // 0x0C FAT32 using extended int13 services

		case PARTITION_IFS:
			return L"IFS"; // 0x07 An IFS partition.
 
		case PARTITION_LDM:
			return L"LDM"; // 0x42 A logical disk manager (LDM) partition.
 
		case PARTITION_NTFT:
			return L"NTFT"; // 0x80 An NTFT partition.
 
		case VALID_NTFT:
			return L"NTFT"; // 0xC0 A valid NTFT partition.

		case PARTITION_HUGE:
			return L"HUGE"; // 0x06 Huge partition MS-DOS V4

		case PARTITION_PREP:
			return L"PREP";  // 0x41 PowerPC Reference Platform (PReP) Boot Partition

		case PARTITION_OS2BOOTMGR:
			return L"OS2";  // 0x0A  OS/2 Boot Manager/OPUS/Coherent swap

		case PARTITION_XINT13:
			return L"XINT13"; // 0x0E Win95 partition using extended int13 services

		case PARTITION_XINT13_EXTENDED:
			return L"XINT13_EXTENDED"; //  0x0F Same as type 5 but uses extended int13 services

		case PARTITION_XENIX_1:
			return L"XENIX1"; // 0x02 Xenix

		case PARTITION_XENIX_2:
			return L"XENIX2"; // 0x02 Xenix

		case PARTITION_UNIX:
			return L"UNIX"; // 0x63 Unix

		case 0x27:
			// Set CreatePartitionType to Primary, and then set ModifyPartition<code>TypeID to 0x27.
			// Recovery files such as push-button recovery images and other system utilities.
			return L"RECOVERY/UTIL";

		case 0x12:
		case 0x84:
		case 0xDE:
		case 0xFE:
		case 0xA0:
			return L"OEM";
	}
	return L"???";
}

EXTERN_C PWSTR WINAPI GetGPTPartitionTypeString( GUID& Guid, LPWSTR pszBuf, int cchBuf )
{
	if( IsEqualGUID(PARTITION_BASIC_DATA_GUID,Guid) )
	{
		return L"PARTITION_BASIC_DATA_GUID";
	}
	if( IsEqualGUID(PARTITION_ENTRY_UNUSED_GUID,Guid) )
	{
		return L"PARTITION_ENTRY_UNUSED_GUID";
	}
	if( IsEqualGUID(PARTITION_SYSTEM_GUID,Guid) )
	{
		return L"PARTITION_SYSTEM_GUID";
	}
	if( IsEqualGUID(PARTITION_MSFT_RESERVED_GUID,Guid) )
	{
		return L"PARTITION_MSFT_RESERVED_GUID";
	}
	if( IsEqualGUID(PARTITION_LDM_METADATA_GUID,Guid) )
	{
		return L"PARTITION_LDM_METADATA_GUID";
	}
	if( IsEqualGUID(PARTITION_LDM_DATA_GUID,Guid) )
	{
		return L"PARTITION_LDM_DATA_GUID";
	}
	if( IsEqualGUID(PARTITION_MSFT_RECOVERY_GUID,Guid) )
	{
		return L"PARTITION_MSFT_RECOVERY_GUID";
	}
	if( IsEqualGUID(PARTITION_CLUSTER_GUID,Guid) )
	{
		return L"PARTITION_CLUSTER_GUID";
	}
	return _MakeGUIDString(Guid,pszBuf,cchBuf);
}

PWSTR WINAPI GetGPTAttribute( DWORD64 Attributes, LPWSTR pszBuf, int cchBuf )
{
	switch( Attributes )
	{
		case GPT_ATTRIBUTE_PLATFORM_REQUIRED:
			StringCchCopy(pszBuf,cchBuf,L"GPT_ATTRIBUTE_PLATFORM_REQUIRED");
			break;
		case GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER:
			StringCchCopy(pszBuf,cchBuf,L"GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER");
			break;
		case GPT_BASIC_DATA_ATTRIBUTE_HIDDEN:
			StringCchCopy(pszBuf,cchBuf,L"GPT_BASIC_DATA_ATTRIBUTE_HIDDEN");
			break;
		case GPT_BASIC_DATA_ATTRIBUTE_SHADOW_COPY:
			StringCchCopy(pszBuf,cchBuf,L"GPT_BASIC_DATA_ATTRIBUTE_SHADOW_COPY");
			break;
		case GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY:
			StringCchCopy(pszBuf,cchBuf,L"GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY");
			break;
		default:
			StringCchPrintf(pszBuf,cchBuf,L"0x%016I64X",Attributes);
			break;
	}
	return pszBuf;
}

PWSTR WINAPI GetMediaCharacteristicsString( DWORD Attributes, PWSTR pszBuf, int cchBuf )
{
	switch( Attributes )
	{
		case MEDIA_CURRENTLY_MOUNTED: // 0x80000000
			StringCchCopy(pszBuf,cchBuf,L"MEDIA_CURRENTLY_MOUNTED");
			break;
		case MEDIA_ERASEABLE:         // 0x00000001
			StringCchCopy(pszBuf,cchBuf,L"MEDIA_ERASEABLE");
			break;
		case MEDIA_WRITE_ONCE:        // 0x00000002
			StringCchCopy(pszBuf,cchBuf,L"MEDIA_WRITE_ONCE");
			break;
		case MEDIA_READ_ONLY:         // 0x00000004
			StringCchCopy(pszBuf,cchBuf,L"MEDIA_READ_ONLY");
			break;
		case MEDIA_READ_WRITE:        // 0x00000008
			StringCchCopy(pszBuf,cchBuf,L"MEDIA_READ_WRITE");
			break;
		case MEDIA_WRITE_PROTECTED:   // 0x00000100
			StringCchCopy(pszBuf,cchBuf,L"MEDIA_WRITE_PROTECTED");
			break;
		default:
			StringCchPrintf(pszBuf,cchBuf,L"0x%016I64X",Attributes);
			break;
	}
	return pszBuf;
}

typedef struct _FS_STORAGE_BUS_TYPE_STRING
{
	ULONG Type;
	PWSTR Name;
	PWSTR Desc;
} FS_STORAGE_BUS_TYPE_STRING;

enum {
	BusTypeSpaces = 16,
	BusTypeNvme,
	BusTypeSCM,
	BusTypeUfs,
};

#define _DEF_FS_STORAGE_BUS_TYPE_STRING(f,d)  { f, L#f, L##d }

FS_STORAGE_BUS_TYPE_STRING StorageBusType[] = 
{
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeUnknown,           "Unknown"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeScsi,              "SCSI"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeAtapi,             "ATAPI"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeAta,               "ATA"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusType1394,              "IEEE 1394"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeSsa,               "SSA"),
    _DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeFibre,             "Fiber Channel"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeUsb,               "USB"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeRAID,              "RAID"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeiScsi,             "iSCSI"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeSas,               "Serial-Attached SCSI (SAS)"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeSata,              "SATA"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeSd,                "Secure Digital (SD)"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeMmc,               "Multimedia Card (MMC)"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeVirtual,           "Virtual"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeFileBackedVirtual, "Virtual (File Backed)"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeSpaces,            "Spaces"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeNvme,              "NVMe"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeSCM,               "SCM"),
	_DEF_FS_STORAGE_BUS_TYPE_STRING(BusTypeUfs,               "Ufs"),
};

PWSTR GetStorageBusTypeDescString(DWORD BusType,LPWSTR pszBuf,int cchBuf)
{
	if( BusType < _countof(StorageBusType) )
	{
		StringCchCopy(pszBuf,cchBuf,StorageBusType[BusType].Desc);
	}
	else
	{
		*pszBuf = L'\0';
	}
	return pszBuf;
}

typedef struct _FS_STORAGE_MEDIA_TYPE_STRING
{
	ULONG Type;
	PSTR pszName;
	PSTR pszType;
	PSTR pszDesc;
} FS_STORAGE_MEDIA_TYPE_STRING;

#define _DEF_FS_STORAGE_MEDIA_TYPE_STRING_U(f,t,d)  { f, L#f, L##t, L##d }
#define _DEF_FS_STORAGE_MEDIA_TYPE_STRING(f,t,d)  { f, #f, t, d }

FS_STORAGE_MEDIA_TYPE_STRING MediaTypeString[] = {
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(Unknown,           "",                     "Unknown"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F5_1Pt2_512,       "5\" 1.2MB FD",         "5.25\", 1.2MB,  512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_1Pt44_512,      "3.5\" 1.44MB FD",      "3.5\",  1.44MB, 512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_2Pt88_512,      "3.5\" 2.88MB FD",      "3.5\",  2.88MB, 512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_20Pt8_512,      "3.5\" 20.8MB FD",      "3.5\",  20.8MB, 512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_720_512,        "3.5\" 720KB FD",       "3.5\",  720KB,  512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F5_360_512,        "3.5\" 360KB FD",       "5.25\", 360KB,  512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F5_320_512,        "3.5\" 320KB FD",       "5.25\", 320KB,  512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F5_320_1024,       "5\" FD 320KB",         "5.25\", 320KB,  1024 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F5_180_512,        "5\" FD 180KB",         "5.25\", 180KB,  512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F5_160_512,        "5\" FD 160KB",         "5.25\", 160KB,  512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(RemovableMedia,    "Removable Media",      "Removable media"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(FixedMedia,        "Fixed Disk",           "Fixed hard disk media"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_120M_512,       "3.5\" 120MB FD",       "3.5\", 120M Floppy"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_640_512,        "3.5\" 640KB FD",       "3.5\" ,  640KB,  512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F5_640_512,        "5\" 60KB FD",          "5.25\",  640KB,  512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F5_720_512,        "5\" 720KB FD",         "5.25\",  720KB,  512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_1Pt2_512,       "3.5\" 1.2MB FD",       "3.5\" ,  1.2Mb,  512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_1Pt23_1024,     "3.5\" 1.23MB FD",      "3.5\" ,  1.23Mb, 1024 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F5_1Pt23_1024,     "5\" 1.23MB FD",        "5.25\",  1.23MB, 1024 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_128Mb_512,      "3.5\" 128MB  MO",      "3.5\" MO 128Mb   512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_230Mb_512,      "F3.5\" 230MB  MO",     "3.5\" MO 230Mb   512 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F8_256_128,        "8\" 256KB FD ",        "8\",     256KB,  128 bytes/sector"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_200Mb_512,      "3.5\" 200MB  HiFD",    "3.5\",   200M Floppy (HiFD)"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_240M_512,       "3.5\" 240MB  HiFD",    "3.5\",   240Mb Floppy (HiFD)"),
    _DEF_FS_STORAGE_MEDIA_TYPE_STRING(F3_32M_512,        "3.5\" 32MB F D",       "3.5\",   32Mb Floppy"),
}; 
 
FS_STORAGE_MEDIA_TYPE_STRING StorageMediaTypeString[] = { 
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DDS_4mm,           "Tape",       "DAT DDS1,2,... (all vendors)"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(MiniQic,           "Tape",       "miniQIC Tape"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(Travan,            "Tape",       "Travan TR-1,2,3,..."),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(QIC,               "Tape",       "QIC"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(MP_8mm,            "Tape",       "8mm Exabyte Metal Particle"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(AME_8mm,           "Tape",       "8mm Exabyte Advanced Metal Evap"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(AIT1_8mm,          "Tape",       "8mm Sony AIT"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DLT,               "Tape",       "DLT Compact IIIxt, IV"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(NCTP,              "Tape",       "Philips NCTP"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(IBM_3480,          "Tape",       "IBM 3480"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(IBM_3490E,         "Tape",       "IBM 3490E"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(IBM_Magstar_3590,  "Tape",       "IBM Magstar 3590"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(IBM_Magstar_MP,    "Tape",       "IBM Magstar MP"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(STK_DATA_D3,       "Tape",       "STK Data D3"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(SONY_DTF,          "Tape",       "Sony DTF"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DV_6mm,            "Tape",       "6mm Digital Video"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DMI,               "Tape",       "Exabyte DMI and compatibles"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(SONY_D2,           "Tape",       "Sony D2S and D2L"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(CLEANER_CARTRIDGE, "Cleaner",    "All Drive types that support Drive Cleaners"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(CD_ROM,            "Opt_Disk",   "CD"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(CD_R,              "Opt_Disk",   "CD-Recordable (Write Once)"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(CD_RW,             "Opt_Disk",   "CD-Rewriteable"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DVD_ROM,           "Opt_Disk",   "DVD-ROM"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DVD_R,             "Opt_Disk",   "DVD-Recordable (Write Once)"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DVD_RW,            "Opt_Disk",   "DVD-Rewriteable"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(MO_3_RW,           "Opt_Disk",   "3.5\" Rewriteable MO Disk"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(MO_5_WO,           "Opt_Disk",   "MO 5.25\" Write Once"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(MO_5_RW,           "Opt_Disk",   "MO 5.25\" Rewriteable (not LIMDOW)"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(MO_5_LIMDOW,       "Opt_Disk",   "MO 5.25\" Rewriteable (LIMDOW)"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(PC_5_WO,           "Opt_Disk",   "Phase Change 5.25\" Write Once Optical"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(PC_5_RW,           "Opt_Disk",   "Phase Change 5.25\" Rewriteable"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(PD_5_RW,           "Opt_Disk",   "Phase Change Dual Rewriteable"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(ABL_5_WO,          "Opt_Disk",   "Ablative 5.25\" Write Once Optical"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(PINNACLE_APEX_5_RW,"Opt_Disk",   "Pinnacle Apex 4.6GB Rewriteable Optical"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(SONY_12_WO,        "Opt_Disk",   "Sony 12\" Write Once"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(PHILIPS_12_WO,     "Opt_Disk",   "Philips/LMS 12\" Write Once"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(HITACHI_12_WO,     "Opt_Disk",   "Hitachi 12\" Write Once"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(CYGNET_12_WO,      "Opt_Disk",   "Cygnet/ATG 12\" Write Once"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(KODAK_14_WO,       "Opt_Disk",   "Kodak 14\" Write Once"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(MO_NFR_525,        "Opt_Disk",   "Near Field Recording (Terastor)"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(NIKON_12_RW,       "Opt_Disk",   "Nikon 12\" Rewriteable"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(IOMEGA_ZIP,        "Mag_Disk",   "Iomega Zip"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(IOMEGA_JAZ,        "Mag_Disk",   "Iomega Jaz"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(SYQUEST_EZ135,     "Mag_Disk",   "Syquest EZ135"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(SYQUEST_EZFLYER,   "Mag_Disk",   "Syquest EzFlyer"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(SYQUEST_SYJET,     "Mag_Disk",   "Syquest SyJet"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(AVATAR_F2,         "Mag_Disk",   "2.5\" Floppy"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(MP2_8mm,           "Tape",       "8mm Hitachi"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DST_S,             "Tape",       "Ampex DST Small Tapes"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DST_M,             "Tape",       "Ampex DST Medium Tapes"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DST_L,             "Tape",       "Ampex DST Large Tapes"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(VXATape_1,         "Tape",       "Ecrix 8mm Tape"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(VXATape_2,         "Tape",       "Ecrix 8mm Tape"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(STK_9840,          "Tape",       "STK 9840"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(LTO_Ultrium,       "Tape",       "IBM, HP, Seagate LTO Ultrium"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(LTO_Accelis,       "Tape",       "IBM, HP, Seagate LTO Accelis"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(DVD_RAM,           "Opt_Disk",   "DVD-RAM"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(AIT_8mm,           "Tape",       "AIT2 or higher"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(ADR_1,             "Tape",       "OnStream ADR Mediatypes"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(ADR_2,             "Tape",       "OnStream ADR Mediatypes"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(STK_9940,          "Tape",       "STK 9940"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(SAIT,              "Tape",       "SAIT Tapes"),
	_DEF_FS_STORAGE_MEDIA_TYPE_STRING(VXATape,           "Tape",       "VXA (Ecrix 8mm) Tape"),
};

PWSTR WINAPI GetMediaTypeString(ULONG Type,PWSTR pszType,int cchType,PWSTR pszDesc,int cchDesc,PWSTR pszName, int cchName)
{
	if( Type < _countof(MediaTypeString) )
	{
		if( pszType )
			MultiByteToWideChar(CP_ACP,0,MediaTypeString[Type].pszType,-1,pszType,cchType);
		if( pszDesc )
			MultiByteToWideChar(CP_ACP,0,MediaTypeString[Type].pszDesc,-1,pszDesc,cchDesc);
		if( pszName )
			MultiByteToWideChar(CP_ACP,0,MediaTypeString[Type].pszName,-1,pszName,cchName);
	}
	else if( (Type - 0x20) < _countof(StorageMediaTypeString) )
	{
		Type -= 0x20;
		if( pszType )
			MultiByteToWideChar(CP_ACP,0,StorageMediaTypeString[Type].pszType,-1,pszType,cchType);
		if( pszDesc )
			MultiByteToWideChar(CP_ACP,0,StorageMediaTypeString[Type].pszDesc,-1,pszDesc,cchDesc);
		if( pszName )
			MultiByteToWideChar(CP_ACP,0,StorageMediaTypeString[Type].pszName,-1,pszName,cchName);
	}
	else
	{
		*pszDesc = L'\0';
		if( pszType )
			*pszType = L'\0';
	}

	return pszDesc;
}

typedef struct _FS_DEPENDENT_DISK_FLAG_STRING
{
	ULONG Flag;
	PWSTR Name;
} FS_DEPENDENT_DISK_FLAG_STRING;

#define _DEF_FS_DEPENDENT_DISK_FLAG_STRING(f)  { f, L#f }

FS_DEPENDENT_DISK_FLAG_STRING DependentDiskFlagString[] = {
	_DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_NONE),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_MULT_BACKING_FILES),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_FULLY_ALLOCATED),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_READ_ONLY),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_REMOTE),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_SYSTEM_VOLUME),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_SYSTEM_VOLUME_PARENT),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_REMOVABLE),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_NO_DRIVE_LETTER),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_PARENT),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_NO_HOST_DISK),
    _DEF_FS_DEPENDENT_DISK_FLAG_STRING(DEPENDENT_DISK_FLAG_PERMANENT_LIFETIME),
};

PWSTR GetDependentDiskFlagString(DWORD dw,PWSTR psz,DWORD cch)
{
	*psz = L'\0';

	int i;
	for(i = 0; i < _countof(DependentDiskFlagString); i++)
	{
		if( dw == DependentDiskFlagString[i].Flag )
		{
			StringCchCopy(psz,cch,DependentDiskFlagString[i].Name);
			break;
		}
	}

	return psz;
}

//////////////////////////////////////////////////////////////////////////////

LONG MountdevQueryUniqueId(HANDLE hVolume,PMOUNTDEV_UNIQUE_ID *pUniqueId)
{
	LONG Status;
	BOOL bSuccess;
	DWORD cb = 0;

	DWORD cbSize = 256;
	PVOID puid = StorageMemAlloc(cbSize);

	for(;;)
	{
		bSuccess = DeviceIoControl(hVolume, 
								IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, 
								NULL,0,
								puid,cbSize,
								&cb,
								NULL);

		if( !bSuccess )
		{
			if(	GetLastError() == ERROR_INSUFFICIENT_BUFFER || GetLastError() == ERROR_MORE_DATA )
			{
				cbSize += 256;
				PVOID pv = StorageMemReAlloc(puid,cbSize);
				if( pv != NULL )
				{
					puid = pv;
					continue;
				}
			}

			Status = GetLastError();

			StorageMemFree(puid);
		}
		else
		{
			Status = ERROR_SUCCESS;

			if( cb < cbSize )
				puid = StorageMemReAlloc(puid,cb);

			*pUniqueId = (PMOUNTDEV_UNIQUE_ID)puid;
		}
		break;
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  StorageGetDeviceNumber()
//
//  PURPOSE:
//
//  NOTE:
//  If the volume is onto configured by multiple physical drive extent, 
//  function returns ERROR_INVALID_FUNCTION.
//
//----------------------------------------------------------------------------
EXTERN_C LONG WINAPI StorageGetDeviceNumber(HANDLE hDevice, PSTORAGE_DEVICE_NUMBER DeviceNumber)
{
	BOOL bSuccess;
	DWORD cb;

	bSuccess = DeviceIoControl( hDevice, 
                             IOCTL_STORAGE_GET_DEVICE_NUMBER, 
                             0,
                             NULL,
                             DeviceNumber,
                             sizeof(STORAGE_DEVICE_NUMBER), 
                             &cb,
                             NULL);

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

//----------------------------------------------------------------------------
//
//  StorageGetDeviceIdDescriptor()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C LONG WINAPI StorageGetDeviceIdDescriptor(HANDLE hDevice,PSTORAGE_DEVICE_DESCRIPTOR *pDeviceDescriptor)
{
	LONG Status = 0;
	BOOL bSuccess;
	DWORD cb;

	STORAGE_PROPERTY_QUERY Query;
	Query.QueryType  = PropertyStandardQuery;
	Query.PropertyId = StorageDeviceProperty;

	STORAGE_DESCRIPTOR_HEADER sdh = {0};

	bSuccess = DeviceIoControl( hDevice, 
							IOCTL_STORAGE_QUERY_PROPERTY, 
							&Query, 
							sizeof(STORAGE_PROPERTY_QUERY), 
							&sdh,
							sizeof(STORAGE_DESCRIPTOR_HEADER), 
							&cb,
							NULL);

	if( bSuccess )
	{
		STORAGE_DEVICE_DESCRIPTOR *psdd = (STORAGE_DEVICE_DESCRIPTOR *)StorageMemAlloc(sdh.Size);
		if( psdd == NULL )
			return ERROR_NOT_ENOUGH_MEMORY;

		bSuccess = DeviceIoControl( hDevice, 
							IOCTL_STORAGE_QUERY_PROPERTY, 
                            &Query, 
                            sizeof(STORAGE_PROPERTY_QUERY), 
                            psdd,
                            sdh.Size, 
                            &cb,
                            NULL);
		if( bSuccess )
		{
			*pDeviceDescriptor = psdd;
			Status = ERROR_SUCCESS;
		}
		else
		{
			Status = GetLastError();
			StorageMemFree(psdd);
		}
	}
	else
	{
		Status = GetLastError();
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  StorageGetDeviceUniqueIdentifier()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
LONG StorageGetDeviceUniqueIdentifier(HANDLE hDevice,PSTORAGE_DEVICE_UNIQUE_IDENTIFIER *pDeviceDescriptor)
{
	LONG Status = 0;
	BOOL bSuccess;
	DWORD cb;

	STORAGE_PROPERTY_QUERY Query;
	ZeroMemory(&Query,sizeof(Query));
	Query.QueryType  = PropertyStandardQuery;
	Query.PropertyId = StorageDeviceUniqueIdProperty;

	STORAGE_DESCRIPTOR_HEADER sdh = {0};

	bSuccess = DeviceIoControl( hDevice, 
							IOCTL_STORAGE_QUERY_PROPERTY, 
							&Query, 
							sizeof(STORAGE_PROPERTY_QUERY), 
							&sdh,
							sizeof(STORAGE_DESCRIPTOR_HEADER), 
							&cb,
							NULL);

	if( bSuccess )
	{
		STORAGE_DEVICE_UNIQUE_IDENTIFIER *psdd = (STORAGE_DEVICE_UNIQUE_IDENTIFIER *)StorageMemAlloc(sdh.Size);
		if( psdd == NULL )
			return ERROR_NOT_ENOUGH_MEMORY;

		bSuccess = DeviceIoControl( hDevice, 
							IOCTL_STORAGE_QUERY_PROPERTY, 
                            &Query, 
                            sizeof(STORAGE_PROPERTY_QUERY), 
                            psdd,
                            sdh.Size, 
                            &cb,
                            NULL);
		if( bSuccess )
		{
			*pDeviceDescriptor = psdd;
			Status = ERROR_SUCCESS;
		}
		else
		{
			Status = GetLastError();
			_MemFree(psdd);
		}
	}
	else
	{
		Status = GetLastError();
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  StorageDetectSectorSize()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C DWORD WINAPI StorageDetectSectorSize(HANDLE hDevice, PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR pAlignmentDescriptor)
{
	BOOL bSuccess;
	DWORD cb;

	STORAGE_PROPERTY_QUERY Query;
	Query.QueryType  = PropertyStandardQuery;
	Query.PropertyId = StorageAccessAlignmentProperty;
      
	bSuccess = DeviceIoControl( hDevice, 
                             IOCTL_STORAGE_QUERY_PROPERTY, 
                             &Query, 
                             sizeof(STORAGE_PROPERTY_QUERY), 
                             pAlignmentDescriptor,
                             sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR), 
                             &cb,
                             NULL);

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

//----------------------------------------------------------------------------
//
//  StorageMediumProductTypeDescriptor()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C ULONG WINAPI StorageMediumProductTypeDescriptor(HANDLE hDevice,LPDWORD pdwMediumProductType)
{
	BOOL bSuccess;
	DWORD cb;

	STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR Descriptor={0};
	STORAGE_PROPERTY_QUERY Query;
	Query.QueryType  = PropertyStandardQuery;
	Query.PropertyId = (STORAGE_PROPERTY_ID)StorageDeviceMediumProductType;
      
	bSuccess = DeviceIoControl( hDevice, 
                             IOCTL_STORAGE_QUERY_PROPERTY, 
                             &Query, 
                             sizeof(STORAGE_PROPERTY_QUERY), 
                             &Descriptor,
                             sizeof(STORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR), 
                             &cb,
                             NULL);

	if( bSuccess )
	{
		if( pdwMediumProductType )
			*pdwMediumProductType = Descriptor.MediumProductType;
	}

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

//----------------------------------------------------------------------------
//
//  StorageAdapterDescriptor()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
//  pBuffer
//	 - STORAGE_ADAPTER_DESCRIPTOR -or-
//	 - STCCTL::STORAGE_ADAPTER_DESCRIPTOR_EX
//
EXTERN_C ULONG WINAPI StorageAdapterDescriptor(HANDLE hDevice,PVOID pBuffer,ULONG cbBuffer)
{
	BOOL bSuccess;
	DWORD cb;

	STORAGE_PROPERTY_QUERY Query;
	Query.QueryType  = PropertyStandardQuery;
	Query.PropertyId  = StorageAdapterProperty;

	bSuccess = DeviceIoControl( hDevice, 
                             IOCTL_STORAGE_QUERY_PROPERTY, 
                             &Query, 
                             sizeof(Query),
                             pBuffer,
                             cbBuffer, 
                             &cb,
                             NULL);

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

//----------------------------------------------------------------------------
//
//  StorageDeviceIdDescriptor()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
ULONG WINAPI StorageDeviceIdDescriptor(HANDLE hDevice, STORAGE_DEVICE_ID_DESCRIPTOR **DeviceId)
{
	BOOL bSuccess;
	DWORD cb;
	ULONG ulLastError;

	STORAGE_DEVICE_ID_DESCRIPTOR *pBuffer;

	STORAGE_PROPERTY_QUERY Query;
	Query.QueryType  = PropertyStandardQuery;
	Query.PropertyId  = StorageDeviceIdProperty;

	STORAGE_DESCRIPTOR_HEADER hdr;

	bSuccess = DeviceIoControl(hDevice, 
                             IOCTL_STORAGE_QUERY_PROPERTY, 
                             &Query,sizeof(Query),
                             &hdr,sizeof(hdr),
                             &cb,NULL);
	if( bSuccess )
	{
		pBuffer = (STORAGE_DEVICE_ID_DESCRIPTOR *)StorageMemAlloc(hdr.Size);

		if( pBuffer == NULL )
			return ERROR_NOT_ENOUGH_MEMORY;

		bSuccess = DeviceIoControl( hDevice, 
                             IOCTL_STORAGE_QUERY_PROPERTY, 
                             &Query, 
                             sizeof(Query),
                             pBuffer,
                             hdr.Size, 
                             &cb,
                             NULL);

		if( bSuccess )
		{
			if( DeviceId )
			{
				*DeviceId = pBuffer;
			}
			else
			{
				_MemFree(DeviceId);
			}

			ulLastError = ERROR_SUCCESS;
		}
		else
		{
			ulLastError = GetLastError();
		}
	}
	else
	{
		ulLastError = GetLastError();
	}

	return ulLastError;
}

//----------------------------------------------------------------------------
//
//  StorageGetDeviceCapacity()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
ULONG WINAPI StorageGetDeviceCapacity(HANDLE hDevice,PSTORAGE_READ_CAPACITY DeviceCapacity)
{
	BOOL bSuccess;
	DWORD cb;

	bSuccess = DeviceIoControl( hDevice, 
                             IOCTL_STORAGE_READ_CAPACITY, 
                             0,
                             NULL,
                             DeviceCapacity,
                             sizeof(STORAGE_READ_CAPACITY), 
                             &cb,
                             NULL);

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

//----------------------------------------------------------------------------
//
//  StorageGetMediaTypes()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
ULONG WINAPI StorageGetMediaTypes(HANDLE hDevice, PGET_MEDIA_TYPES *pMediaTypes)
{
	ULONG Status;
	BOOL bSuccess;
	DWORD cb = 0;

    GET_MEDIA_TYPES *pmt;
	DWORD cbmt = sizeof(GET_MEDIA_TYPES);

	pmt = (GET_MEDIA_TYPES *)StorageMemAlloc(cbmt);

	for(;;)
	{
		bSuccess = DeviceIoControl( hDevice, 
								IOCTL_STORAGE_GET_MEDIA_TYPES_EX, 
								NULL, 
								0,
								pmt,
								cbmt,
								&cb,
								NULL);

		if( !bSuccess )
		{
			if(	GetLastError() == ERROR_INSUFFICIENT_BUFFER )
			{
				cbmt += 1024;
				PVOID pv = StorageMemReAlloc(pmt,cbmt);
				if( pv != NULL )
				{
					pmt = (GET_MEDIA_TYPES *)pv;
					continue;
				}
			}

			Status = GetLastError();

			StorageMemFree(pmt);
		}
		else
		{
			Status = ERROR_SUCCESS;

			if( cb < cbmt )
				pmt = (GET_MEDIA_TYPES*)StorageMemReAlloc(pmt,cb); // shrink only

			*pMediaTypes = pmt;
		}

		break;
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  QueryOnDiskVolumeInfo()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
//
// must be FILE_READ_DATA required (usually need running under Admin user)
//
LONG QueryOnDiskVolumeInfo(HANDLE hDevice,FILE_QUERY_ON_DISK_VOL_INFO_BUFFER *pudfDiskVolumeInfo)
{
	BOOL bSuccess;
	DWORD cb;

	bSuccess = DeviceIoControl( hDevice, 
                             FSCTL_QUERY_ON_DISK_VOLUME_INFO,
                             0,
                             NULL,
                             pudfDiskVolumeInfo,
                             sizeof(FILE_QUERY_ON_DISK_VOL_INFO_BUFFER), 
                             &cb,
                             NULL);

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

//----------------------------------------------------------------------------
//
//  GetNtfsVolumeData()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
LONG GetNtfsVolumeData(HANDLE hVolume,NTFS_VOLUME_DATA_BUFFER *pNtfsVolumeData,ULONG cbNtfsVolumeData)
{
	BOOL bSuccess;
	DWORD cb;

	// hVolume
	// A handle to a volume from which the file record information is to be retrieved. 
	// To open a volume and retrieve a volume handle, call the CreateFile function. 
	// Either FILE_READ_ATTRIBUTES or FILE_WRITE_ATTRIBUTES access is sufficient when
	// opening the volume. 
	//
	// ... as described by the document of FSCTL_GET_NTFS_VOLUME_DATA.
	// However, actually had need to the FILE_READ_DATA. Therefore this function 
	// successes under running administrator.
	//
	bSuccess = DeviceIoControl( hVolume, 
                             FSCTL_GET_NTFS_VOLUME_DATA,
                             0,
                             NULL,
                             pNtfsVolumeData,
                             cbNtfsVolumeData, 
                             &cb,
                             NULL);

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

LONG GetRecognitionInformation(HANDLE hVolume,FILE_SYSTEM_RECOGNITION_INFORMATION *RecognitionInfo)
{
	BOOL bSuccess;
	DWORD cb;

	bSuccess = DeviceIoControl( hVolume, 
                             FSCTL_QUERY_FILE_SYSTEM_RECOGNITION,
                             0,
                             NULL,
                             RecognitionInfo,
                             sizeof(FILE_SYSTEM_RECOGNITION_INFORMATION), 
                             &cb,
                             NULL);

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

LONG GetRecognitionStructure(HANDLE hVolume,FILE_SYSTEM_RECOGNITION_STRUCTURE *RecognitionStructure)
{
	BOOL bSuccess;
	DWORD cb;

	bSuccess = DeviceIoControl( hVolume, 
                             FSCTL_QUERY_FILE_SYSTEM_RECOGNITION,
                             0,
                             NULL,
                             RecognitionStructure,
                             sizeof(FILE_SYSTEM_RECOGNITION_STRUCTURE), 
                             &cb,
                             NULL);

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

LONG GetRetrievalPointerBase(HANDLE hVolume,RETRIEVAL_POINTER_BASE *RetrievalPointerBase)
{
	// Get the sector offset to the first logical cluster number (LCN) of 
	// the file system relative to the start of the volume.
	// The volume-relative sector offset to the first allocatable unit
	// on the file system, also referred to as the base of the cluster heap.
	//
	BOOL bSuccess;
	DWORD dwBytesReturned;
	bSuccess = DeviceIoControl(hVolume,
							FSCTL_GET_RETRIEVAL_POINTER_BASE,
							NULL,0,
							RetrievalPointerBase,sizeof(RETRIEVAL_POINTER_BASE),
							&dwBytesReturned,
							NULL);

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

BOOL
GetDiskExtents(
	HANDLE hVolume,
	PVOLUME_DISK_EXTENTS *DiskExtents,
	PDISK_GEOMETRY_EX **DiskGeometryPtrArray,
	PDRIVE_LAYOUT_INFORMATION_EX **DriveLayoutInfoPtrArray
	)
{
	BOOL bSuccess = FALSE;
	DWORD cb = 0;
	PVOID pOutBuffer = NULL;
	DWORD cbOutBuffer = sizeof(VOLUME_DISK_EXTENTS);
	BOOL bRet;

	//
	// Get volume extents on each physical disk(s)
	//
	for(;;)
	{
		pOutBuffer = StorageMemAlloc( cbOutBuffer );
		if( pOutBuffer == NULL )
			break;

		bRet = DeviceIoControl(hVolume,
						IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
						NULL,0,
						pOutBuffer,cbOutBuffer,
						&cb,NULL);

		DWORD dwError = GetLastError();
		if( !bRet && (dwError == ERROR_INSUFFICIENT_BUFFER || dwError == ERROR_MORE_DATA) )
		{
			StorageMemFree( pOutBuffer );

			cbOutBuffer += 4096;

			pOutBuffer = StorageMemAlloc( cbOutBuffer );
			if( pOutBuffer )
				continue;
		}
		break;
	}

	//
	// Copy volume extents and get disk geometry information
	//
	if( pOutBuffer && cb != 0 )
	{
		PVOLUME_DISK_EXTENTS pVolumeDiskExtents = NULL;
		PDISK_GEOMETRY_EX *pDiskGeometryPtrArray = NULL;
		PDRIVE_LAYOUT_INFORMATION_EX *pDriveLayoutInfoPtrArray = NULL;

		pVolumeDiskExtents = (PVOLUME_DISK_EXTENTS)StorageMemAlloc( cb );

		if( pVolumeDiskExtents )
		{
			CopyMemory(pVolumeDiskExtents,pOutBuffer,cb);

			// allocate PDISK_GEOMETRY_EX pointer array
			pDiskGeometryPtrArray = (PDISK_GEOMETRY_EX *)StorageMemAlloc( sizeof(PDISK_GEOMETRY_EX) * pVolumeDiskExtents->NumberOfDiskExtents );

			pDriveLayoutInfoPtrArray = (PDRIVE_LAYOUT_INFORMATION_EX *)StorageMemAlloc( sizeof(PDRIVE_LAYOUT_INFORMATION_EX) * pVolumeDiskExtents->NumberOfDiskExtents );

			HANDLE hDisk;
			PDISK_GEOMETRY_EX pGeometry;
			ULONG cbGeometry;
			WCHAR szPhysicalDrive[64];
			DWORD i;
			for(i = 0; i < pVolumeDiskExtents->NumberOfDiskExtents; i++)
			{
#if 0
				hDisk = OpenDisk(NULL,pVolumeDiskExtents->Extents[i].DiskNumber,0);
#else
				swprintf_s(szPhysicalDrive,_countof(szPhysicalDrive),L"PhysicalDrive%u",pVolumeDiskExtents->Extents[i].DiskNumber);
				hDisk = OpenDisk(szPhysicalDrive,0,0);
#endif
				if( INVALID_HANDLE_VALUE != hDisk )
				{
					// Geometry information
					GetDiskDriveGeometryEx(hDisk,&pGeometry,&cbGeometry);
					pDiskGeometryPtrArray[i] = pGeometry;

					// Layout information
					PDRIVE_LAYOUT_INFORMATION_EX pDriveLayoutBuffer;
					GetDiskDriveLayoutEx(hDisk,&pDriveLayoutBuffer);
					pDriveLayoutInfoPtrArray[i] = pDriveLayoutBuffer;

					CloseHandle(hDisk);
				}
			}
		}

		// retval
		*DiskExtents = pVolumeDiskExtents;
		*DiskGeometryPtrArray = pDiskGeometryPtrArray;
		*DriveLayoutInfoPtrArray = pDriveLayoutInfoPtrArray;
		bSuccess = TRUE;
	}
	
	StorageMemFree( pOutBuffer );

	return bSuccess;
}

BOOL
FreeDiskExtents(
	PVOLUME_DISK_EXTENTS DiskExtents,
	PDISK_GEOMETRY_EX *DiskGeometryPtrArray,
	PDRIVE_LAYOUT_INFORMATION_EX *DriveLayoutInfoPtrArray
	)
{
	if( DiskExtents == NULL )
		return FALSE;

	for(DWORD dw = 0; dw < DiskExtents->NumberOfDiskExtents; dw++)
	{
		if( DiskGeometryPtrArray )
			StorageMemFree(DiskGeometryPtrArray[dw]);
		if( DriveLayoutInfoPtrArray )
			StorageMemFree(DriveLayoutInfoPtrArray[dw]);
	}

	StorageMemFree(DiskGeometryPtrArray);
	StorageMemFree(DriveLayoutInfoPtrArray);
	StorageMemFree(DiskExtents);

	return TRUE;
}

LONG
GetReFSVolumeData(
	HANDLE hVolume,
	REFS_VOLUME_DATA_BUFFER *pBuffer
	)
{
	BOOL bSuccess;
	DWORD cb;

	pBuffer->ByteCount = sizeof(REFS_VOLUME_DATA_BUFFER);

	bSuccess = DeviceIoControl(hVolume,
                             FSCTL_GET_REFS_VOLUME_DATA,
                             0,
                             NULL,
                             pBuffer,
                             pBuffer->ByteCount,
                             &cb,
                             NULL);

	return bSuccess ? ERROR_SUCCESS : GetLastError();
}

//////////////////////////////////////////////////////////////////////////////

EXTERN_C HANDLE WINAPI OpenDisk(PCWSTR pszDeviceName,DWORD dwDiskNumber,DWORD dwDesired)
{
	WCHAR szOpenDevice[MAX_PATH];

	if( pszDeviceName )
	{
		if( *pszDeviceName == L'\\' )
		{
			if( wcsnicmp(pszDeviceName,L"\\\\?\\GlobalRoot",14) == 0 )
				swprintf_s(szOpenDevice,_countof(szOpenDevice),L"%s",pszDeviceName);
			else
				swprintf_s(szOpenDevice,_countof(szOpenDevice),L"\\\\?\\GlobalRoot%s",pszDeviceName);
		}
		else
		{
			swprintf_s(szOpenDevice,_countof(szOpenDevice),L"\\\\.\\%s",pszDeviceName);
		}
	}
	else
	{
#if 0
		swprintf_s(szOpenDevice,_countof(szOpenDevice),L"\\\\.\\PhysicalDrive%u",dwDiskNumber);
#else
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
#endif
	}

	if( dwDesired == 0 )
	{
		if( IsUserAnAdmin() )
			dwDesired = STANDARD_RIGHTS_READ|FILE_READ_DATA;
		else
			dwDesired = STANDARD_RIGHTS_READ|FILE_READ_ATTRIBUTES;
	}

	HANDLE hDisk;

	hDisk = CreateFile(szOpenDevice,dwDesired|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);

	if( hDisk == INVALID_HANDLE_VALUE )
	{
		return INVALID_HANDLE_VALUE;
	}

	return hDisk;
}

/*++

PDISK_GEOMETRY_EX pGeometry;
ULONG cbGeometry;
GetDiskDriveGeometryEx(hVolumeDevice,&pGeometry,&cbGeometry);

PDISK_DETECTION_INFO p1;
			p1 = DiskGeometryGetDetect( pGeometry );

PDISK_PARTITION_INFO p2;
			p2 = DiskGeometryGetPartition( pGeometry );

--*/
EXTERN_C LONG WINAPI GetDiskDriveGeometryEx(HANDLE hDisk, PDISK_GEOMETRY_EX *pGeometry,ULONG *pcb)
{
	LONG Status;
	BOOL bSuccess;
	DWORD cb = 0;

	if( pGeometry == NULL )
		return ERROR_INVALID_PARAMETER;

	if( pcb )
		*pcb = 0;

    DISK_GEOMETRY_EX *pg;
	DWORD cbSize = sizeof(DISK_GEOMETRY_EX) + ((sizeof(DISK_PARTITION_INFO) + sizeof(PDISK_DETECTION_INFO)) * 64);

	pg = (DISK_GEOMETRY_EX *)StorageMemAlloc(cbSize);
	if( pg == NULL )
		cbSize = 0;

	for(;;)
	{
		bSuccess = DeviceIoControl(hDisk, 
								IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, 
								NULL, 
								0,
								pg,
								cbSize,
								&cb,
								NULL);

		if( !bSuccess )
		{
			if(	GetLastError() == ERROR_INSUFFICIENT_BUFFER )
			{
				cbSize += 4096;
				PVOID pv = StorageMemReAlloc(pg,cbSize);
				if( pv != NULL )
				{
					pg = (DISK_GEOMETRY_EX *)pv;
					continue;
				}
			}

			Status = GetLastError();

			StorageMemFree(pg);

			*pGeometry = NULL;
			if( pcb )
				*pcb = 0;
		}
		else
		{
			Status = ERROR_SUCCESS;

			if( cb < cbSize )
				pg = (DISK_GEOMETRY_EX *)StorageMemReAlloc(pg,cb); // shrink only

			*pGeometry = pg;
			if( pcb )
				*pcb = cb;
		}
		break;
	}

	return Status;
}

BOOL GetDiskDriveLayoutEx(HANDLE hDisk,PDRIVE_LAYOUT_INFORMATION_EX *DriveLayoutBuffer)
{
	BOOL bRet;
	DWORD cbReturned = 0;
	DRIVE_LAYOUT_INFORMATION_EX *pDriveLayout = NULL;
    DWORD cbOutBufferSize = 0;

	*DriveLayoutBuffer = NULL;

	cbOutBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) + 4096;
	pDriveLayout = (DRIVE_LAYOUT_INFORMATION_EX*)StorageMemAlloc(cbOutBufferSize);

	for(;;)
	{
		if( pDriveLayout == NULL )
			break;

		bRet = DeviceIoControl(hDisk,
						IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
						NULL,0,
						pDriveLayout,cbOutBufferSize,
						&cbReturned,NULL);

		DWORD dwError = GetLastError();
		if( !bRet && (dwError == ERROR_INSUFFICIENT_BUFFER || dwError == ERROR_MORE_DATA) )
		{
			StorageMemFree( pDriveLayout );

			cbOutBufferSize += 4096;

			pDriveLayout = (DRIVE_LAYOUT_INFORMATION_EX*)StorageMemAlloc( cbOutBufferSize );
			if( pDriveLayout )
				continue;
		}
		break;
	}

	if( pDriveLayout != NULL )
	{
		*DriveLayoutBuffer = (DRIVE_LAYOUT_INFORMATION_EX*)StorageMemReAlloc(pDriveLayout,cbReturned);
	}

	return ((*DriveLayoutBuffer) != NULL);
}

BOOL GetCacheInformation(HANDLE hDisk,PDISK_CACHE_INFORMATION CacheInformaion)
{
	BOOL bRet;
	DWORD cbReturned;

	bRet = DeviceIoControl(hDisk,
				IOCTL_DISK_GET_CACHE_INFORMATION,
				NULL,0,
				CacheInformaion,sizeof(DISK_CACHE_INFORMATION),
				&cbReturned,NULL);

	return bRet;
}

//---------------------------------------------------------------------------
//
//  GetVolumeUsnJornalDataInformation()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
NTAPI
GetVolumeUsnJornalDataInformation(
	HANDLE Handle,
	__in VOLUME_FS_USN_JOURNAL_DATA *QuataInfoList,
	__inout ULONG *pcbQuataInfoList
	)
{
	DWORD cbBytesReturned;
	DWORD cbBuffer;

	cbBuffer = *pcbQuataInfoList;

	DeviceIoControl(Handle,
				FSCTL_QUERY_USN_JOURNAL, 
				NULL,
				0,
				QuataInfoList,
				cbBuffer,
				&cbBytesReturned,
				NULL);

	*pcbQuataInfoList = cbBytesReturned;

	return HRESULT_FROM_WIN32( GetLastError() );
}

//////////////////////////////////////////////////////////////////////////////

LPWSTR WINAPI _FormatByteSize(LONGLONG qdw, __out_ecount(cchBuf) LPWSTR pszBuf, UINT cchBuf)
{
	if( qdw < 1024 )
		StringCchPrintf(pszBuf,cchBuf,L"%I64u Bytes",qdw);
	else
		StrFormatByteSizeW(qdw,pszBuf,cchBuf);
		// StrFormatByteSize64()
	return pszBuf;
}

LPWSTR WINAPI _MakeGUIDString(GUID& Guid,LPWSTR pszBuf,int cchBuf)
{
	StringFromGUID2(Guid,pszBuf,cchBuf);
	return pszBuf;
}

#ifdef __cplusplus
};
#endif

//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//  GetDeviceTypeByVolumeName()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
GetDeviceTypeByVolumeName(
	PCWSTR pszVolumeName,
	PULONG pDeviceType,
	PULONG pCharacteristics
	)
{
	NTSTATUS Status;
	HANDLE Handle;
	if( (Status = OpenVolume(pszVolumeName,0,&Handle)) == STATUS_SUCCESS )
	{
		GetVolumeDeviceType(Handle,pDeviceType,pCharacteristics);

		CloseHandle(Handle);
	}
	return HRESULT_FROM_NT(Status);
}

//----------------------------------------------------------------------------
//
//  GetVolumeTypeString()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C 
PWSTR
WINAPI
GetVolumeTypeString(
	UINT Type,
	ULONG Val,
	PWSTR pszText,
	int cchTextMax
	)
{
	switch ( Type )
	{
		case CSTR_DEVICETYPE:
			GetDeviceTypeString(Val,pszText,cchTextMax);
			return pszText;
		case CSTR_MEDIATYPE:
			GetMediaTypeString(Val,pszText,cchTextMax,NULL,0,NULL,0);
			return pszText;
	}

	return L"";
}

//----------------------------------------------------------------------------
//
//  TryOpenVolume()
//
//  PURPOSE: 
//
//----------------------------------------------------------------------------
static 
NTSTATUS
TryOpenVolume(
	HANDLE *pHandle,
	PCWSTR pszVolumeName
	)
{
	NTSTATUS Status;
	Status = OpenVolume(pszVolumeName,OPEN_READ_DATA,pHandle);
	if( Status != STATUS_SUCCESS )
	{
		Status = OpenVolume(pszVolumeName,OPEN_GENERIC_READ,pHandle);
		if( Status != STATUS_SUCCESS )
		{
			Status = OpenVolume(pszVolumeName,0,pHandle);
		}
	}
	return Status;
}

//----------------------------------------------------------------------------
//
//  CreateVolumeInformationBuffer()
//
//  PURPOSE: Create information buffer and gathering volume/disk information.
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
CreateVolumeInformationBuffer(
	PCWSTR pszVolumeName,
	ULONG /*InformationClass*/,
	ULONG /*OpenFlags*/,
	PVOID *InformaionBuffer
	)
{
	NTSTATUS Status;
	HRESULT hr = E_FAIL;

	//
	// Volume Device 
	//
	HANDLE hVolume = NULL;
	HANDLE hRootDirectory = NULL;
	UINT PreviousErrorMode = 0;
	VOLUME_DEVICE_INFORMATION *pVolumeInfo = NULL;

	__try
	{
		PreviousErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

		//
		// Create and Initialize VOLUME_DEVICE_INFORMATION binded data structure.
		//
		pVolumeInfo = new VOLUME_DEVICE_INFORMATION;
		RtlZeroMemory(pVolumeInfo,sizeof(VOLUME_DEVICE_INFORMATION));

		pVolumeInfo->RetrievalPointerBase.FileAreaOffset.QuadPart = -1;

		//
		// Open File System through the Root Directory on volume.
		// if no cd-rom media, fail open root directory.
		//
		Status = OpenRootDirectory(pszVolumeName,OPEN_VOLUME_READ_DATA,&hRootDirectory);

		if( Status != STATUS_SUCCESS )
		{
			// No file system volume, ex) RAW drive 
			Status = TryOpenVolume(&hRootDirectory,pszVolumeName);
		}

		if( hRootDirectory != INVALID_HANDLE_VALUE )
		{
			//
			// Gathering NT Volume Information
			//
			GatherNtVolumeDeviceInformation( hRootDirectory, pVolumeInfo );

			if( wcsicmp(pVolumeInfo->FileSystemName,L"ntfs") == 0 )
			{
				if( GetNtfsVolumeData(hRootDirectory,(NTFS_VOLUME_DATA_BUFFER *)&pVolumeInfo->ntfs,sizeof(pVolumeInfo->ntfs)) )
					pVolumeInfo->State.NtfsData = TRUE;
			}

			if( wcsicmp(pVolumeInfo->FileSystemName,L"ReFS") == 0 )
			{
				if( GetReFSVolumeData(hRootDirectory,&pVolumeInfo->refs.data) == 0 )
					pVolumeInfo->State.RefsData = TRUE;
			}
		}

		//
		// Open Volume Device
		//
		Status = TryOpenVolume(&hVolume,pszVolumeName);

		if( hVolume != INVALID_HANDLE_VALUE )
		{
			// STORAGE_DEVICE_NUMBER
			StorageGetDeviceNumber(hVolume,&pVolumeInfo->DeviceNumber);

			// PSTORAGE_DEVICE_DESCRIPTOR
			StorageGetDeviceIdDescriptor(hVolume,&pVolumeInfo->pDeviceDescriptor);

			// STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR
			StorageDetectSectorSize(hVolume,&pVolumeInfo->AlignmentDescriptor);

			// PGET_MEDIA_TYPES
			StorageGetMediaTypes(hVolume,&pVolumeInfo->pMediaTypes);

			// PMOUNTDEV_UNIQUE_ID
			MountdevQueryUniqueId(hVolume,&pVolumeInfo->pUniqueId);

			//
			// Get physical disk information for this volume
			//
			GetDiskExtents(hVolume,
					&pVolumeInfo->pVolumeDiskExtents,
					&pVolumeInfo->pDiskGeometryPtrList,
					&pVolumeInfo->pDriveLayoutPtrList);

			//
			// UDF (admin only)
			//
			if( wcsicmp(pVolumeInfo->FileSystemName,L"udf") == 0 )
			{
				if ( QueryOnDiskVolumeInfo(hVolume,&pVolumeInfo->udf.DiskVolumeInfo) == ERROR_SUCCESS )
					pVolumeInfo->State.UdfData = TRUE;
			}

			//
			// Retrieval Pointer Base
			//
			GetRetrievalPointerBase(hVolume,&pVolumeInfo->RetrievalPointerBase);

			//
			// Dirty Bit
			//
			hr = IsSetDirtyBit(hVolume);
			if( hr == S_OK )
			{
				pVolumeInfo->DirtyBit = (hr == S_OK);
			}
			else
			{
				pVolumeInfo->DirtyBit = -1;
			}
		}
		else
		{
			pVolumeInfo->DirtyBit = -1;
		}

		//
		// Virtual Disk Information
		//
		PWSTR pszRootDir = CombinePath(pszVolumeName,L"\\");

		if( pszRootDir )
		{
			pVolumeInfo->VirtualDiskVolume = (CHAR)VirtualDisk_GetDependencyInformation(pszRootDir,(PSTORAGE_DEPENDENCY_INFO *)&pVolumeInfo->VirtualHardDiskInformation);
			FreeMemory(pszRootDir);
		}

		hr = S_OK;
	}
	__finally
	{
		if( hVolume != INVALID_HANDLE_VALUE )
			CloseHandle(hVolume);
		if( hRootDirectory != INVALID_HANDLE_VALUE )
			CloseHandle(hRootDirectory);

		SetErrorMode(PreviousErrorMode);

		*InformaionBuffer = pVolumeInfo;
	}
	return hr;
}

//----------------------------------------------------------------------------
//
//  DestroyVolumeInformationBuffer()
//
//  PURPOSE: Free information data structure memory.
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
DestroyVolumeInformationBuffer(
	PVOID InformaionBuffer
	)
{
	VOLUME_DEVICE_INFORMATION *pVolumeInfo = (VOLUME_DEVICE_INFORMATION *)InformaionBuffer;

	FreeDiskExtents(
		pVolumeInfo->pVolumeDiskExtents,
		pVolumeInfo->pDiskGeometryPtrList,
		pVolumeInfo->pDriveLayoutPtrList
		);

	StorageMemFree( pVolumeInfo->pDeviceDescriptor );
	StorageMemFree( pVolumeInfo->pMediaTypes );
	StorageMemFree( pVolumeInfo->pUniqueId );

	LocalFree( pVolumeInfo->VirtualHardDiskInformation );

	FreeMemory(pVolumeInfo->VolumeLabel);

	delete pVolumeInfo;

	return 0;
}
