//***************************************************************************
//*                                                                         *
//*  flthelp.cpp                                                            *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2024-04-10                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include <fltuser.h>

#define DEF_FS_TYPE(t)  {t,#t}

typedef struct _FLTFSNAME {
	PSTR Name;
	struct {
		UINT  FSType;
		PSTR  FSTypeName;
	};
	PSTR Desc;
	PSTR DeviceRef;
} FLTFSNAME;

// FLT_FILESYSTEM_TYPE - Windows 8 later additions.
enum {
	FLT_FSTYPE_NPFS=25,
    FLT_FSTYPE_MSFS,
    FLT_FSTYPE_CSVFS,
    FLT_FSTYPE_REFS,
    FLT_FSTYPE_OPENAFS,
    FLT_FSTYPE_CIMFS,
};

FLTFSNAME defnames[] = {
	{"Unknown",    DEF_FS_TYPE(FLT_FSTYPE_UNKNOWN),         "UNKNOWN file system type",               ""},
    {"RAW",        DEF_FS_TYPE(FLT_FSTYPE_RAW),             "Microsoft's RAW file system",             "(\\FileSystem\\RAW)"},
    {"NTFS",       DEF_FS_TYPE(FLT_FSTYPE_NTFS),            "Microsoft's NTFS file system",           "(\\FileSystem\\Ntfs)"},
    {"FAT",        DEF_FS_TYPE(FLT_FSTYPE_FAT),             "Microsoft's FAT file system",            "(\\FileSystem\\Fastfat)"},
    {"CDFS",       DEF_FS_TYPE(FLT_FSTYPE_CDFS),            "Microsoft's CDFS file system",           "(\\FileSystem\\Cdfs)"},
    {"UDFS",       DEF_FS_TYPE(FLT_FSTYPE_UDFS),            "Microsoft's UDFS file system",           "(\\FileSystem\\Udfs)"},
    {"LANMAN",     DEF_FS_TYPE(FLT_FSTYPE_LANMAN),          "Microsoft's LanMan Redirector",          "(\\FileSystem\\MRxSmb)"},
    {"WABDAV",     DEF_FS_TYPE(FLT_FSTYPE_WEBDAV),          "Microsoft's WebDav redirector",          "(\\FileSystem\\MRxDav)"},
    {"RDPDR",      DEF_FS_TYPE(FLT_FSTYPE_RDPDR),           "Microsoft's Terminal Server redirector", "(\\Driver\\rdpdr)"},
    {"NFS",        DEF_FS_TYPE(FLT_FSTYPE_NFS),             "Microsoft's NFS file system",            "(\\FileSystem\\NfsRdr)"},
    {"MS NETWARE", DEF_FS_TYPE(FLT_FSTYPE_MS_NETWARE),      "Microsoft's NetWare redirector",         "(\\FileSystem\\nwrdr)"},
    {"NETWARE",    DEF_FS_TYPE(FLT_FSTYPE_NETWARE),         "Novell's NetWare redirector",            ""},
    {"BSUDF",      DEF_FS_TYPE(FLT_FSTYPE_BSUDF),           "The BsUDF CD-ROM driver",                "(\\FileSystem\\BsUDF)"},
    {"MUP",        DEF_FS_TYPE(FLT_FSTYPE_MUP),             "Microsoft's Mup redirector",             "(\\FileSystem\\Mup)"},
    {"RSFX",       DEF_FS_TYPE(FLT_FSTYPE_RSFX),            "Microsoft's WinFS redirector",           "(\\FileSystem\\RsFxDrv)"},
    {"UDF1",       DEF_FS_TYPE(FLT_FSTYPE_ROXIO_UDF1),      "Roxio's UDF writeable file system",      "(\\FileSystem\\cdudf_xp)"},
    {"UDF2",       DEF_FS_TYPE(FLT_FSTYPE_ROXIO_UDF2),      "Roxio's UDF readable file system",       "(\\FileSystem\\UdfReadr_xp)"},
    {"UDF3",       DEF_FS_TYPE(FLT_FSTYPE_ROXIO_UDF3),      "Roxio's DVD file system",                "(\\FileSystem\\DVDVRRdr_xp)"},
    {"TACIT",      DEF_FS_TYPE(FLT_FSTYPE_TACIT),           "Tacit FileSystem",                       "(\\Device\\TCFSPSE)"},
    {"REC",        DEF_FS_TYPE(FLT_FSTYPE_FS_REC),          "Microsoft's File system recognizer",     "(\\FileSystem\\Fs_rec)"},
    {"INCD",       DEF_FS_TYPE(FLT_FSTYPE_INCD),            "Nero's InCD file system",                "(\\FileSystem\\InCDfs)"},
    {"FAT",        DEF_FS_TYPE(FLT_FSTYPE_INCD_FAT),        "Nero's InCD FAT file system",            "(\\FileSystem\\InCDFat)"},
    {"ExFAT",      DEF_FS_TYPE(FLT_FSTYPE_EXFAT),           "Microsoft's EXFat FILE SYSTEM",          "(\\FileSystem\\exfat)"},
    {"PSFS",       DEF_FS_TYPE(FLT_FSTYPE_PSFS),            "PolyServ's file system",                 "(\\FileSystem\\psfs)"},
    {"GPFS",       DEF_FS_TYPE(FLT_FSTYPE_GPFS),            "IBM General Parallel File System",       "(\\FileSystem\\gpfs)"},
    {"NPFS",       DEF_FS_TYPE(FLT_FSTYPE_NPFS),            "Microsoft's Named Pipe file system",     "(\\FileSystem\\npfs)"},
    {"MSFS",       DEF_FS_TYPE(FLT_FSTYPE_MSFS),            "Microsoft's Mailslot file system",       "(\\FileSystem\\msfs)"},
    {"CSVFS",      DEF_FS_TYPE(FLT_FSTYPE_CSVFS),           "Microsoft's Cluster Shared Volume",      "(\\FileSystem\\csvfs)"},
    {"ReFS",       DEF_FS_TYPE(FLT_FSTYPE_REFS),            "Microsoft's ReFS file system",           "(\\FileSystem\\Refs;\\FileSystem\\Refsv1)"},
    {"OPENAFS",    DEF_FS_TYPE(FLT_FSTYPE_OPENAFS),         "OpenAFS file system",                    "(\\Device\\AFSRedirector)"},
    {"CIMFS" ,     DEF_FS_TYPE(FLT_FSTYPE_CIMFS),           "Composite Image file system",            "(\\FileSystem\\cimfs)"},
};

EXTERN_C
PCSTR
WINAPI
GetFilterFileSystemTypeString(
	INT Type
	)
{
	if( Type < 0 || Type >= ARRAYSIZE(defnames) )
		return "";
	
	return defnames[Type].Name;
}

EXTERN_C
HRESULT
WINAPI
FindFirstVolumeInstance(
	PCWSTR pszVolumeName,
	INSTANCE_INFORMATION_CLASS dwInformationClass,
	LPVOID *lpReturnedBuffer,
	LPHANDLE lpVolumeInstanceFind
	)
{
	HRESULT hr;
	HANDLE hVolumeInstanceFind;
	DWORD BytesReturned;

	PVOID lpBuffer = NULL;
	DWORD dwBufferSize = 0;

	for(;;)
	{
		hr = FilterVolumeInstanceFindFirst(
				pszVolumeName,
				dwInformationClass,
				lpBuffer,
				dwBufferSize,
				&BytesReturned,
				&hVolumeInstanceFind
				);

		if( HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER )
		{
			if( lpBuffer )
				LocalFree(lpBuffer);
			lpBuffer = LocalAlloc(LPTR,BytesReturned);
			if( lpBuffer == NULL )
			{
				hr = E_OUTOFMEMORY;
				break;
			}
			dwBufferSize = BytesReturned;

			continue;
		}
		else
		{
			break;
		}
	}

	if( hr == S_OK && lpBuffer != NULL )
	{
		*lpReturnedBuffer = lpBuffer;
		*lpVolumeInstanceFind = hVolumeInstanceFind;
	}
	else
	{
		*lpReturnedBuffer = NULL;
		*lpVolumeInstanceFind = NULL;

		if( lpBuffer != NULL )
			LocalFree(lpBuffer);
	}

	return hr;
}

EXTERN_C
HRESULT
WINAPI
FindNextVolumeInstance(
	HANDLE hVolumeInstanceFind,
	INSTANCE_INFORMATION_CLASS dwInformationClass,
	LPVOID *lpReturnedBuffer
	)
{
	HRESULT hr;
	DWORD BytesReturned;

	PVOID lpBuffer = NULL;
	DWORD dwBufferSize = 0;

	for(;;)
	{
		hr = FilterVolumeInstanceFindNext(
				hVolumeInstanceFind,
				dwInformationClass,
				lpBuffer,
				dwBufferSize,
				&BytesReturned
				);

		if( HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER )
		{
			if( lpBuffer )
				LocalFree(lpBuffer);
			lpBuffer = LocalAlloc(LPTR,BytesReturned);
			if( lpBuffer == NULL )
			{
				hr = E_OUTOFMEMORY;
				break;
			}
			dwBufferSize = BytesReturned;

			continue;
		}
		else
		{
			break;
		}
	}

	if( hr == S_OK && lpBuffer != NULL )
	{
		*lpReturnedBuffer = lpBuffer;
	}
	else
	{
		*lpReturnedBuffer = NULL;

		if( lpBuffer != NULL )
			LocalFree(lpBuffer);
	}
	return hr;
}
