//****************************************************************************
//*                                                                          *
//*  vdshelp.cpp                                                             *
//*                                                                          *
//*  VDS Managment Helper                                                    *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2025-09-26                                                     *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include <xmllite.h>
#include "vdshelp.h"

LPCWSTR VDS_GetBusTypeText(VDS_STORAGE_BUS_TYPE BusType)
{
	static struct {
		int value;
		PCWSTR busName;
	} bustype[] = {
		{ VDSBusTypeUnknown,  L"Unknown" },                   // 0x0
		{ VDSBusTypeScsi,     L"SCSI" },                      // 0x1
		{ VDSBusTypeAtapi,    L"ATAPI" },                     // 0x2
		{ VDSBusTypeAta,      L"ATA" },	                      // 0x3,
		{ VDSBusType1394,     L"1394" },                      // 0x4,
		{ VDSBusTypeSsa,      L"SSA" },	                      // 0x5
		{ VDSBusTypeFibre,    L"Fibre" },                     // 0x6,
		{ VDSBusTypeUsb,      L"USB" },	                      // 0x7,
		{ VDSBusTypeRAID,     L"RAID" },                      // 0x8,
		{ VDSBusTypeiScsi,    L"iScsi" },                     // 0x9,
		{ VDSBusTypeSas,      L"SAS" },	                      // 0xa,
		{ VDSBusTypeSata,     L"SATA" },                      // 0xb,
		{ VDSBusTypeSd,       L"SD" },                        // 0xc,
		{ VDSBusTypeMmc,      L"MMC" },	                      // 0xd,
		{ VDSBusTypeVirtual,  L"Virtual" },                   // 0xe
		{ VDSBusTypeFileBackedVirtual,L"FileBackedVirtual" }, // 0xf
		{ VDSBusTypeSpaces,   L"Spaces"},                     // 0x10
		{ VDSBusTypeNVMe,     L"NVMe"},                       // 0x11
		{ VDSBusTypeScm,      L"Scm"},                        // 0x12
		{ VDSBusTypeUfs,      L"Ufs"},                        // 0x13
	};

	if( 0 <= BusType && BusType < _countof(bustype) )
	{
		SetLastError(ERROR_SUCCESS);
		return bustype[BusType].busName;
	}

	SetLastError(ERROR_FILE_NOT_FOUND);
	return L"Unknown";
}

LPCWSTR VDS_FileSystemTypeText(VDS_FILE_SYSTEM_TYPE FsType)
{
	static struct {
		int value;
		PCWSTR fsName;
	} fstype[] = {
		{ VDS_FST_UNKNOWN, L"Unknown" },
		{ VDS_FST_RAW,     L"RAW" },
		{ VDS_FST_FAT,     L"FAT" },
		{ VDS_FST_FAT32,   L"FAT32" },
		{ VDS_FST_NTFS,    L"NTFS" },
		{ VDS_FST_CDFS,    L"CDFS" },
		{ VDS_FST_UDF,     L"UDF" },
		{ VDS_FST_EXFAT,   L"exFAT" },
		{ VDS_FST_CSVFS,   L"CSVFS" },
		{ VDS_FST_REFS,    L"ReFS" },
	};

	if( 0 <= FsType && FsType < _countof(fstype) )
	{
		SetLastError(ERROR_SUCCESS);
		return fstype[FsType].fsName;
	}

	SetLastError(ERROR_FILE_NOT_FOUND);
	return L"Unknown";
}

LPCWSTR VDS_GetVolumeTypeText(VDS_VOLUME_TYPE VolumeType)
{
	static struct {
		int value;
		PCWSTR vtName;
	} voltype[] = {
		{ VDS_VT_UNKNOWN,  L"Unknown" },
		{ VDS_VT_SIMPLE,   L"Simple" }, // 10
		{ VDS_VT_SPAN,     L"Span"   }, // 11
		{ VDS_VT_STRIPE,   L"Stripe" }, // 12
		{ VDS_VT_MIRROR,   L"Mirror" }, // 13
		{ VDS_VT_PARITY,   L"Parity" }, // 14
	};

	int volType = VolumeType;

	if( volType >= VDS_VT_SIMPLE )
		volType -= (VDS_VT_SIMPLE-1);

	if( 0 <= volType && volType < _countof(voltype) )
	{
		SetLastError(ERROR_SUCCESS);
		return voltype[volType].vtName;
	}

	SetLastError(ERROR_FILE_NOT_FOUND);
	return L"Unknown";
}

#define _DET_FLGTEXT(f,t,d) { f, {L#f, t, d} }
LPCWSTR VDS_GetDiskExtentTypeDescription(VDS_DISK_EXTENT_TYPE extType,int descTextType)
{
    struct {
        int value;
		PCWSTR text[3];
    } exttype[] = {
        _DET_FLGTEXT(VDS_DET_UNKNOWN,  L"Unknown",  L"Unknown Partition" ),               // 0
        _DET_FLGTEXT(VDS_DET_FREE,     L"Free",     L"Free Space" ),                      // 1
        _DET_FLGTEXT(VDS_DET_DATA,     L"Data",     L"Data Partition" ),                  // 2
        _DET_FLGTEXT(VDS_DET_OEM,      L"OEM",      L"OEM Partition" ),                   // 3
        _DET_FLGTEXT(VDS_DET_ESP,      L"ESP",      L"EFI System Partition" ),            // 4
        _DET_FLGTEXT(VDS_DET_MSR,      L"MSR",      L"Microsoft Reserved Partition" ),    // 5
        _DET_FLGTEXT(VDS_DET_LDM,      L"LDM",      L"Logical Disk Manager Partition" ),  // 6
        _DET_FLGTEXT(VDS_DET_CLUSTER,  L"Cluster",  L"Cluster Metadata Partition" ),      // 7
        _DET_FLGTEXT(VDS_DET_UNUSABLE, L"Unusable", L"Unusable Space" ),                  // 0x7FFF
    };

	if( descTextType < 0 || descTextType > 2 )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return L"";
	}

	SetLastError(ERROR_SUCCESS);

	if( VDS_DET_UNUSABLE == extType )
	{
		return exttype[8].text[descTextType];
	}
	else
	{
		if( VDS_DET_UNKNOWN <= extType && extType <= VDS_DET_CLUSTER )
		{
			return exttype[extType].text[descTextType];
		}
	}

	SetLastError(ERROR_FILE_NOT_FOUND);
	return L"<Invalid Type>";
}

LPCWSTR VDS_GetStatusText(VDS_DISK_STATUS status)
{
	switch( status )
	{
		case VDS_DS_ONLINE:
			return L"Online";
		case VDS_DS_NOT_READY:
			return L"Not Ready";
		case VDS_DS_NO_MEDIA:
			return L"No Media";
		case VDS_DS_FAILED:
			return L"Failed";
		case VDS_DS_MISSING:
			return L"Missing";
		case VDS_DS_OFFLINE:
			return L"Offline";
	}
	return L"Unknown";
}

LPCWSTR VDS_GetPartitionStyleText(VDS_PARTITION_STYLE style)
{
	LPCWSTR pszText;
	switch( style )
	{
		case VDS_PST_MBR:
			pszText = L"MBR";
			break;
		case VDS_PST_GPT:
			pszText = L"GPT";
			break;
		default:
			pszText = L"Unknown";
			break;
	}
	return pszText;
}

LPCWSTR VDS_GetVolumeStatusText(VDS_VOLUME_STATUS status)
{
	LPCWSTR pszText;
	switch( status )
	{
		case VDS_VS_ONLINE:
			pszText = L"Online";
			break;
		case VDS_VS_NO_MEDIA:
			pszText = L"Media";
			break;
		case VDS_VS_FAILED:
			pszText = L"Failed";
			break;
		case VDS_VS_OFFLINE:
			pszText = L"Offline";
			break;
		default:
			pszText = L"Unknown";
			break;
	}
	return pszText;
}

#define _DEF_FLAG(f,s)  { f, L#f, s }

LPCWSTR
VDS_GetDiskFlagText(
	__in VDS_DISK_FLAG flags,
	__in_opt LPWSTR pszFriendlyName,
	__in_opt int cchFriendlyName,
	__in_opt LPWSTR pszFlagName,
	__in_opt int cchFlagName
	)
{
	static struct {
		UINT flag;
		PCWSTR flagName;
		PCWSTR friendlyName;
	} flgstr[] = {
		_DEF_FLAG(VDS_DF_AUDIO_CD,               L"Audio CD"),              // 0x1
		_DEF_FLAG(VDS_DF_HOTSPARE,               L"Hot Spare"),             // 0x2
		_DEF_FLAG(VDS_DF_RESERVE_CAPABLE,        L"Reserve Capable"),       // 0x4
		_DEF_FLAG(VDS_DF_MASKED,                 L"Masked"),                // 0x8
		_DEF_FLAG(VDS_DF_STYLE_CONVERTIBLE,      L"Style Convertible"),     // 0x10
		_DEF_FLAG(VDS_DF_CLUSTERED,              L"Clustered"),             // 0x20
		_DEF_FLAG(VDS_DF_READ_ONLY,              L"Read Only"),             // 0x40
		_DEF_FLAG(VDS_DF_SYSTEM_DISK,            L"System Disk"),           // 0x80
		_DEF_FLAG(VDS_DF_BOOT_DISK,              L"Boot Disk"),             // 0x100
		_DEF_FLAG(VDS_DF_PAGEFILE_DISK,          L"Pagefile Disk"),         // 0x200
		_DEF_FLAG(VDS_DF_HIBERNATIONFILE_DISK,   L"Hibernation File Disk"), // 0x400
		_DEF_FLAG(VDS_DF_CRASHDUMP_DISK,         L"Crash Dump Disk"),       // 0x800
		_DEF_FLAG(VDS_DF_HAS_ARC_PATH,           L"Has Arc Path"),          // 0x1000
		_DEF_FLAG(VDS_DF_DYNAMIC,                L"Dynamic"),               // 0x2000
		_DEF_FLAG(VDS_DF_BOOT_FROM_DISK,         L"Boot From Disk"),        // 0x4000
		_DEF_FLAG(VDS_DF_CURRENT_READ_ONLY,      L"Current Read Only"),     // 0x8000
		_DEF_FLAG(VDS_DF_REFS_NOT_SUPPORTED,     L"ReFS Not Supported"),    // 0x10000
	};

	for(int i = 0; i < _countof(flgstr); i++)
	{
		if( (flags & flgstr[i].flag) == flgstr[i].flag )
		{
			if( pszFriendlyName )
				StringCchCopy(pszFriendlyName,cchFriendlyName,flgstr[i].friendlyName);

			if( pszFlagName )
				StringCchCopy(pszFlagName,cchFlagName,flgstr[i].friendlyName);

			return flgstr[i].friendlyName;
		}
	}

	if( pszFriendlyName && cchFriendlyName > 0 )
		*pszFriendlyName = L'\0';
	if( pszFlagName && cchFlagName > 0 )
		*pszFlagName = L'\0';

	return L"";
}

LPCWSTR
VDS_GetVolumeFlagText(
	__in VDS_VOLUME_FLAG flags,
	__in_opt LPWSTR pszFriendlyName,
	__in_opt int cchFriendlyName,
	__in_opt LPWSTR pszFlagName,
	__in_opt int cchFlagName
	)
{
	static struct {
		UINT flag;
		PCWSTR flagName;
		PCWSTR friendlyName;
	} flgstr[] = {
		_DEF_FLAG(VDS_VF_SYSTEM_VOLUME,        L"System Volume"),              // 0x1
		_DEF_FLAG(VDS_VF_BOOT_VOLUME,          L"Boot Volume"),                // 0x2
		_DEF_FLAG(VDS_VF_ACTIVE,               L"Active"),                     // 0x4
		_DEF_FLAG(VDS_VF_READONLY,             L"Read Only"),                  // 0x8
		_DEF_FLAG(VDS_VF_HIDDEN,               L"Hidden"),                     // 0x10
		_DEF_FLAG(VDS_VF_CAN_EXTEND,           L"Extend"),                     // 0x20
		_DEF_FLAG(VDS_VF_CAN_SHRINK,           L"Shrink"),                     // 0x40
		_DEF_FLAG(VDS_VF_PAGEFILE,             L"Pagefile"),                   // 0x80
		_DEF_FLAG(VDS_VF_HIBERNATION,          L"Hibernation"),                // 0x100
		_DEF_FLAG(VDS_VF_CRASHDUMP,            L"Crash Dump"),                 // 0x200
		_DEF_FLAG(VDS_VF_INSTALLABLE,          L"Installable"),                // 0x400
		_DEF_FLAG(VDS_VF_LBN_REMAP_ENABLED,    L"LBN Remap Enabled"),          // 0x800
		_DEF_FLAG(VDS_VF_FORMATTING,           L"Formatting"),                 // 0x1000
		_DEF_FLAG(VDS_VF_NOT_FORMATTABLE,      L"Not Formattable"),            // 0x2000
		_DEF_FLAG(VDS_VF_NTFS_NOT_SUPPORTED,   L"NTFS Not Supported"),         // 0x4000
		_DEF_FLAG(VDS_VF_FAT32_NOT_SUPPORTED,  L"FAT32 Not Supported"),        // 0x8000
		_DEF_FLAG(VDS_VF_FAT_NOT_SUPPORTED,    L"FAT Not Supported"),          // 0x10000
		_DEF_FLAG(VDS_VF_NO_DEFAULT_DRIVE_LETTER, L"No Default Drive Letter"), // 0x20000
		_DEF_FLAG(VDS_VF_PERMANENTLY_DISMOUNTED, L"Permanently Dismounted"),   // 0x40000
		_DEF_FLAG(VDS_VF_PERMANENT_DISMOUNT_SUPPORTED, L"Permanent Dismount Supported"), // 0x80000
		_DEF_FLAG(VDS_VF_SHADOW_COPY,          L"Shadow Copy"),                // 0x100000
		_DEF_FLAG(VDS_VF_FVE_ENABLED,          L"FVE Enabled"),                // 0x200000
		_DEF_FLAG(VDS_VF_DIRTY,                L"Dirty"),                      // 0x400000
		_DEF_FLAG(VDS_VF_REFS_NOT_SUPPORTED,   L"ReFS Not Supported" ),        // 0x800000
		_DEF_FLAG(VDS_VF_BACKS_BOOT_VOLUME,    L"Backs_Boot_Volume" ),         // 0x1000000
	};

	for(int i = 0; i < _countof(flgstr); i++)
	{
		if( (flags & flgstr[i].flag) == flgstr[i].flag )
		{
			if( pszFriendlyName )
				StringCchCopy(pszFriendlyName,cchFriendlyName,flgstr[i].friendlyName);

			if( pszFlagName )
				StringCchCopy(pszFlagName,cchFlagName,flgstr[i].friendlyName);

			return flgstr[i].friendlyName;
		}
	}

	if( pszFriendlyName && cchFriendlyName > 0 )
		*pszFriendlyName = L'\0';
	if( pszFlagName && cchFlagName > 0 )
		*pszFlagName = L'\0';

	return L"";
}
