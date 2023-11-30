//***************************************************************************
//*                                                                         *
//*  ntvolumefunctions.cpp                                                  *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//***************************************************************************
//
//  2023.08.19
//  ddk build source code (porting from volumeinfo.cpp)
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include <ntifs.h>
#include <ntdddisk.h>
#include <ntddvol.h>
#include <ntstatus.h>
#include <winerror.h>
#include <strsafe.h>
#include <stdlib.h>
#include <limits.h>
#include "ntnativeapi.h"
#include "ntnativehelp.h"
#include "ntobjecthelp.h"
#include "ntvolumehelp.h"
#include "ntvolumenames.h"

EXTERN_C
NTSTATUS
NTAPI
LdrLoadDll(
    IN PCWSTR DllPath OPTIONAL,
    IN PULONG DllCharacteristics OPTIONAL,
    IN PCUNICODE_STRING DllName,
    OUT PVOID *DllHandle
    );

EXTERN_C
NTSTATUS
NTAPI
LdrUnloadDll(
    IN PVOID DllHandle
    );

EXTERN_C
NTSTATUS
NTAPI
LdrGetProcedureAddress(
    IN PVOID DllHandle,
    IN CONST ANSI_STRING* ProcedureName OPTIONAL,
    IN ULONG ProcedureNumber OPTIONAL,
    OUT PVOID *ProcedureAddress
    );

EXTERN_C
int
WINAPI
StrCmpLogicalW(
	LPCWSTR psz1,
    LPCWSTR psz2
	);

EXTERN_C
ULONG
WINAPI
QueryDosDeviceW(
	LPCWSTR lpDeviceName,
	LPWSTR lpTargetPath,
	DWORD ucchMax
	);

#ifdef _WIN64
typedef INT_PTR (FAR WINAPI *FARPROC)();
#else
typedef int (FAR WINAPI *FARPROC)();
#endif

#ifndef FsPathIsGlobalRootPrefixDosDrive
#define FsPathIsGlobalRootPrefixDosDrive(p) \
		(\
		p != NULL && \
		p[0] == L'\\' && \
		p[1] == L'?' && \
		p[2] == L'?' && \
		p[3] == L'\\' && \
		((L'A' <= p[4] && p[4] <= L'Z') || (L'a' <= p[4] && p[4] <= L'z')) && \
		p[5] == L':')
#endif

#define _IS_ALPHA(ch) ((L'A' <= (ch) &&  (ch) <= L'Z') || (L'a' <= (ch) &&  (ch) <= L'z'))
#define _IS_DRIVE(ach) (_IS_ALPHA((ach)[0]) && (ach)[1] == L':' && (ach)[2] == L'\0')
#define _IS_VOLUME_GUID(ach) ((_wcsnicmp(ach,L"Volume",6) == 0) && ((ach)[6] == L'{' && (ach)[43] == L'}'))

#ifndef MAX_PATH
#define MAX_PATH (260)
#endif

static inline void _RemoveBackslash(PWSTR psz)
{
	int cch = (int)wcslen(psz);
	if( psz[cch-1] == L'\\' )
		psz[cch-1] = L'\0';
}

template<typename T>
T *_allocArray(T *pArray,ULONG cElements)
{
	HANDLE hHeap = GetProcessHeap();

	if( pArray == NULL )
	{
		pArray = (T *)AllocMemory(sizeof(T));
	}
	else
	{
		PVOID pAlloc;
		pAlloc = ReAllocateHeap(pArray,sizeof(T) * cElements);
		if( pAlloc != NULL )
		{
			pArray = (T *)pAlloc;
		}
		else
		{
			HeapFree(hHeap,0,pArray);
			pArray = NULL;
		}
	}
	return pArray;
}

/*
  ex)
  struct {
     ULONG count;
     STRUCT element[...];
  };
*/
template<typename T,typename E>
T *_allocArrayStruct(T *pArray,ULONG cElements)
{
	if( pArray == NULL )
	{
		pArray = (T *)AllocMemory(sizeof(T));
	}
	else
	{
		if( cElements > 1 )
			cElements--;

		PVOID pAlloc;
		pAlloc = ReAllocateHeap(pArray,sizeof(T) + (sizeof(E) * cElements));
		if( pAlloc != NULL )
		{
			pArray = (T *)pAlloc;
		}
		else
		{
			FreeMemory(pArray);
			pArray = NULL;
		}
	}
	return pArray;
}

//---------------------------------------------------------------------------
//
//  GetVolumeNameSet()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
BOOL
WINAPI
GetVolumeNameSet(
	PCWSTR pszInputName,
	PWSTR  pszDevicePath,ULONG cchDevicePath,
	PWSTR  pszVolumeName,ULONG cchVolumeName,
	PWSTR  pszDosDrive,ULONG cchDosDrive
	)
{
	// \??\ --+--\??\C:
	//        |
	//        +--\??\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
	//
	// \\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
	//
	//           1         2         3         4         5
	// 012345678901234567890123456789012345678901234567890
	// Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx} -> volume
	//
	// C: -> drive
	//
	// HarddiskVolume
	//
	WCHAR szTempDevice[MAX_PATH];
	WCHAR szTempVolume[MAX_PATH];
	WCHAR szTempDrive[MAX_PATH];

	RtlZeroMemory(szTempDevice,sizeof(szTempDevice));
	RtlZeroMemory(szTempVolume,sizeof(szTempVolume));
	RtlZeroMemory(szTempDrive,sizeof(szTempDrive));

	*pszDevicePath = L'\0';
	*pszVolumeName = L'\0';
	*pszDosDrive = L'\0';

	// Prefix "\??\"
	//
	if( pszInputName[0] == L'\\' && pszInputName[1] == L'?' && 
		pszInputName[2] == L'?' && pszInputName[3] == L'\\' )
	{
		// "\??\C:"
		if( _IS_ALPHA(pszInputName[4]) && pszInputName[5] == L':' )
		{
			StringCchCopy(szTempDrive,MAX_PATH,&pszInputName[4]);
			_RemoveBackslash(szTempDrive);
		}
		else if( _IS_VOLUME_GUID( &pszInputName[4] ) )
		{
			StringCchCopy(szTempVolume,MAX_PATH,&pszInputName[4]);
			_RemoveBackslash(szTempVolume);
		}
	}
	// Prefix "\\?\"
	//
	else if( pszInputName[0] == L'\\' && pszInputName[1] == L'\\' && 
		     pszInputName[2] == L'?' && pszInputName[3] == L'\\' )
	{
		if( _IS_VOLUME_GUID( &pszInputName[4] ) )
		{
			StringCchCopy(szTempVolume,MAX_PATH,&pszInputName[4]);
			_RemoveBackslash(szTempVolume);
		}
	}
	// Dos drive "C:\"
	//
	else if( _IS_ALPHA(pszInputName[0]) && pszInputName[1] == L':' )
	{
		StringCchCopy(szTempDrive,MAX_PATH,pszInputName);
		_RemoveBackslash(szTempDrive);
	}
	else
	{
		// Other
		// Regarded as the Device Path.
		//
		StringCchCopy(szTempDevice,MAX_PATH,pszInputName);
		_RemoveBackslash(szTempDevice);
	}

	int cchBuffer = 65536;
	WCHAR *pszBuffer = (WCHAR *)AllocStringBuffer( cchBuffer );
	if( pszBuffer == NULL )
	{
		RtlSetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	QueryDosDeviceW(NULL,pszBuffer,cchBuffer);

	WCHAR szTemp[MAX_PATH];
	WCHAR *p;

	if( szTempDrive[0] != L'\0' )
	{
		QueryDosDeviceW(szTempDrive,szTempDevice,MAX_PATH);
	}

	if( szTempVolume[0] != L'\0' )
	{
		QueryDosDeviceW(szTempVolume,szTempDevice,MAX_PATH);
	}

	if( szTempDevice[0] != L'\0' && (szTempDrive[0] == L'\0' || szTempVolume[0] == L'\0') )
	{
		p = pszBuffer;
		while( *p )
		{
			QueryDosDeviceW(p,szTemp,MAX_PATH);

			if( _IS_DRIVE(p) && _wcsicmp(szTemp,szTempDevice) == 0 )
			{
				// Found dos drive from device path
				//
				StringCchCopy(szTempDrive,MAX_PATH,p);
			}

			if( _IS_VOLUME_GUID(p) && _wcsicmp(szTemp,szTempDevice) == 0 ) 
			{
				// Found volume GUID from device path
				//
				StringCchCopy(szTempVolume,MAX_PATH,p);
			}

			p += (wcslen(p)+1);
		}
	}

	FreeMemory(pszBuffer);

	StringCchCopy(pszDevicePath,cchDevicePath,szTempDevice);
	StringCchCopy(pszDosDrive,cchDosDrive,szTempDrive);

	StringCchCopy(pszVolumeName,cchVolumeName,L"\\\\?\\");
	StringCchCat(pszVolumeName,cchVolumeName,szTempVolume);

	RtlSetLastWin32Error(ERROR_SUCCESS);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//  LookupVolumeNameByNtDeviceName()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
ULONG
NTAPI
LookupVolumeNameByNtDeviceName(
	__in PCWSTR DevicePath,
	__inout PWSTR *pRootPos,
	__out PWSTR VolumeSymbolicLink,
	__in ULONG cchVolumeSymbolicLink
	)
{
	HANDLE hObjDir;
	LONG Status;

	Status = OpenObjectDirectory( L"\\GLOBAL??", &hObjDir );

	if( Status == 0 )
	{
		ULONG Index = 0;
		WCHAR VolumeSymName[260];
		WCHAR DeviceName[260];

		Status = STATUS_OBJECT_NAME_NOT_FOUND;

		while( QueryObjectDirectory(hObjDir,&Index,VolumeSymName,260,NULL,0) == 0)
		{
			// Looking for a string which have prefix of "Volume{".
			// If find matched prefix, check a trail of the matched string to 
			// verify that is a GUID style.
			//
			//  0     0                                    4 
			//  0     6                                    3 
			// "Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
			if( _wcsnicmp(VolumeSymName,L"Volume{",7) == 0 && 
				(VolumeSymName[43] == L'}' && VolumeSymName[44] == '\0') )
			{
				if( QuerySymbolicLinkObject(hObjDir,VolumeSymName,NULL,DeviceName,260) == 0 )
				{
					SIZE_T cchDeviceVolume = (int)wcslen(DeviceName);

					if( (_wcsnicmp(DevicePath,DeviceName,cchDeviceVolume) == 0) && 
						(DevicePath[cchDeviceVolume] == L'\\' || DevicePath[cchDeviceVolume] == L'\0') )
					{
						// Root directory position
						if( pRootPos )
							*pRootPos = (PWSTR)&DevicePath[cchDeviceVolume];
	
						// Symbolic link name
						if( VolumeSymbolicLink )
						{
							wcsncpy_s(VolumeSymbolicLink,cchVolumeSymbolicLink,VolumeSymName,cchVolumeSymbolicLink-1);//_TRUNCATE);
						}

						Status = STATUS_SUCCESS;

						break;
					}
				}
			}
		}

		CloseObjectDirectory( hObjDir );
	}

	return RtlNtStatusToDosError(Status);
}

//---------------------------------------------------------------------------
//
//  _compare_proc()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
#define _HARDDISK_VOLUME L"\\Device\\HarddiskVolume"
#define _HARDDISK_VOLUME_LENGTH ((sizeof(_HARDDISK_VOLUME)/sizeof(WCHAR))-1)

static int __cdecl _compare_proc(const void *x, const void *y)
{
	VOLUME_NAME_STRING *item1 = (VOLUME_NAME_STRING *)x;
	VOLUME_NAME_STRING *item2 = (VOLUME_NAME_STRING *)y;

	const WCHAR *sz1 = item1->NtVolumeName;
	const WCHAR *sz2 = item2->NtVolumeName;

	if( sz1[0] == L'\0' && sz2[1] != L'\0' )
		return 1;
	else if( sz1[0] != L'\0' && sz2[1] == L'\0' )
		return -1;
	else
	{
		// check subst assigned volume name
		if( FsPathIsGlobalRootPrefixDosDrive(sz1) && !FsPathIsGlobalRootPrefixDosDrive(sz2) )
			return 1;
		if( !FsPathIsGlobalRootPrefixDosDrive(sz1) && FsPathIsGlobalRootPrefixDosDrive(sz2) )
			return -1;
		BOOLEAN bHarddisk1 = (wcsnicmp(sz1,_HARDDISK_VOLUME,_HARDDISK_VOLUME_LENGTH) == 0);
		BOOLEAN bHarddisk2 = (wcsnicmp(sz2,_HARDDISK_VOLUME,_HARDDISK_VOLUME_LENGTH) == 0);
		if( bHarddisk1 && !bHarddisk2 )
			return -1;
		else if( !bHarddisk1 && bHarddisk2 )
			return 1;
	}

	return StrCmpLogicalW(sz1,sz2);
}

//---------------------------------------------------------------------------
//
//  _lookupDeviceNameString()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
static ULONG _lookupDeviceNameString(VOLUME_NAME_STRING *Array,ULONG cArrayCount,PCWSTR DeviceName)
{
	ULONG i;
	for(i = 0; i < cArrayCount; i++)
	{
		if( wcsicmp(Array[i].NtVolumeName,DeviceName) == 0 )
			return i;
	}
	return ULONG_MAX;
}

//---------------------------------------------------------------------------
//
//  _isTrailerStringAllNumChar()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
static BOOL _isTrailerStringAllNumChar( PCWSTR psz )
{
	while( *psz )
	{
		if( L'0' <= *psz && *psz <= '9' )
			psz++;
		else
			return FALSE; 
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//  EnumHarddiskVolumes()
//
//  PURPOSE: Enumerate HarddiskVolumes name only.
//
//---------------------------------------------------------------------------
EXTERN_C
ULONG
NTAPI
EnumHarddiskVolumes(
	__inout VOLUME_NAME_STRING_ARRAY **VolumeNames // point to pointer to array of VOLUME_NAME_STRING
	)
{
	HANDLE hObjDir;
	LONG Status;

	Status = OpenObjectDirectory( L"\\GLOBAL??", &hObjDir );

	if( Status != ERROR_SUCCESS )
		return Status;

	VOLUME_NAME_STRING_ARRAY *VolNameArray = NULL;
	VOLUME_NAME_STRING *volname = NULL;
	ULONG cVolNames = 0;

	ULONG Index = 0;
	WCHAR VolumeSymName[MAX_PATH];
	WCHAR DeviceName[MAX_PATH];

	Status = STATUS_SUCCESS;

	while( QueryObjectDirectory(hObjDir,&Index,VolumeSymName,ARRAYSIZE(VolumeSymName),NULL,0) == 0)
	{
		// Looking for a string which have prefix of "HarddiskVolumeX".
		// 'X' is must be numeric character.
		//
		if( _wcsnicmp(VolumeSymName,L"HarddiskVolume",14) == 0 && _isTrailerStringAllNumChar( &VolumeSymName[14] ) )
		{
			DeviceName[0] = L'\0';
			QuerySymbolicLinkObject(hObjDir,VolumeSymName,NULL,DeviceName,ARRAYSIZE(DeviceName));

			int iIndex;
			if( (iIndex = _lookupDeviceNameString(VolNameArray->Volume,cVolNames,DeviceName)) == ULONG_MAX )
			{
				VolNameArray = _allocArrayStruct<VOLUME_NAME_STRING_ARRAY,VOLUME_NAME_STRING>(VolNameArray,++cVolNames);

				if( VolNameArray == NULL )
				{
					Status = STATUS_NO_MEMORY;
					break;
				}

				volname = &(VolNameArray->Volume[cVolNames-1]);

				wcscpy_s(volname->VolumeGuidString,ARRAYSIZE(volname->VolumeGuidString),VolumeSymName);
				volname->NtVolumeName = (const WCHAR *)DuplicateString(DeviceName);
			}
		}
	}

	if( cVolNames > 0 )
	{
		qsort(VolNameArray->Volume,cVolNames,sizeof(VOLUME_NAME_STRING),&_compare_proc);

		VolNameArray->Count = cVolNames;
		*VolumeNames = VolNameArray;
	}
	else
	{
		Status = STATUS_OBJECT_NAME_NOT_FOUND;
	}
	
	CloseObjectDirectory( hObjDir );

	return RtlNtStatusToDosError(Status);
}

//---------------------------------------------------------------------------
//
//  EnumVolumeNames()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
ULONG
NTAPI
EnumVolumeNames(
	__inout VOLUME_NAME_STRING_ARRAY **VolumeNames // point to pointer to array of VOLUME_NAME_STRING
	)
{
	HANDLE hObjDir;
	LONG Status;

	Status = OpenObjectDirectory( L"\\GLOBAL??", &hObjDir );

	if( Status != ERROR_SUCCESS )
		return Status;

	VOLUME_NAME_STRING_ARRAY *VolNameArray = NULL;
	VOLUME_NAME_STRING *volname = NULL;
	ULONG cVolNames = 0;

	ULONG Index = 0;
	WCHAR VolumeSymName[MAX_PATH];
	WCHAR DeviceName[MAX_PATH];
	ULONG ulItemIndex;

	Status = STATUS_SUCCESS;

	while( QueryObjectDirectory(hObjDir,&Index,VolumeSymName,ARRAYSIZE(VolumeSymName),NULL,0) == 0)
	{
		// Looking for a string which have prefix of "HarddiskVolumeX".
		// 'X' is must be numeric character.
		//
		if( _wcsnicmp(VolumeSymName,L"HarddiskVolume",14) == 0 && _isTrailerStringAllNumChar( &VolumeSymName[14] ) )
		{
			DeviceName[0] = L'\0';
			QuerySymbolicLinkObject(hObjDir,VolumeSymName,NULL,DeviceName,ARRAYSIZE(DeviceName));

			// If already registered in array, do not add it.
			//
			if( _lookupDeviceNameString(VolNameArray->Volume,cVolNames,DeviceName) == ULONG_MAX )
			{
				VolNameArray = _allocArrayStruct<VOLUME_NAME_STRING_ARRAY,VOLUME_NAME_STRING>(VolNameArray,++cVolNames);

				if( VolNameArray == NULL )
				{
					Status = STATUS_NO_MEMORY;
					break;
				}

				volname = &(VolNameArray->Volume[cVolNames-1]);

				volname->NtVolumeName = (const WCHAR *)DuplicateString(DeviceName);
				RtlZeroMemory(volname->VolumeGuidString,sizeof(volname->VolumeGuidString));
			}
		}

		// Looking for a string which have prefix of "Volume{".
		// If find matched prefix, check a trail of the matched string to 
		// verify that is a GUID style.
		//
		//  0     0                                    4 
		//  0     6                                    3 
		// "Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
		if( _wcsnicmp(VolumeSymName,L"Volume{",7) == 0 && 
			(VolumeSymName[43] == L'}' && VolumeSymName[44] == '\0') )
		{
			DeviceName[0] = L'\0';
			QuerySymbolicLinkObject(hObjDir,VolumeSymName,NULL,DeviceName,ARRAYSIZE(DeviceName));

			// If already registered in array, do not add it.
			//
			// NOTE: ToDo:
			// We need considering to the case multiple GUIDs are assigned to the same volume.
			// ex)
			// \??\Volume{8ba9564c-9bc7-11e8-976e-806e6f6e6963} 0xC32FBC96 0x000000AF00200000 (751,621,373,952)
			// \??\Volume{efbcd68b-a5da-11e8-946a-806e6f6e6963} 0xC32FBC96 0x000000AF00200000 (751,621,373,952)
			// \DosDevices\Z:                                   0xC32FBC96 0x000000AF00200000 (751,621,373,952)
			//
			if( (ulItemIndex = _lookupDeviceNameString(VolNameArray->Volume,cVolNames,DeviceName)) == ULONG_MAX )
			{
				VolNameArray = _allocArrayStruct<VOLUME_NAME_STRING_ARRAY,VOLUME_NAME_STRING>(VolNameArray,++cVolNames);

				if( VolNameArray == NULL )
				{
					Status = STATUS_NO_MEMORY;
					break;
				}

				volname = &(VolNameArray->Volume[cVolNames-1]);

				wcscpy_s(volname->VolumeGuidString,ARRAYSIZE(volname->VolumeGuidString),VolumeSymName);
				volname->NtVolumeName = (const WCHAR *)DuplicateString(DeviceName);
			}
			else
			{
				volname = &(VolNameArray->Volume[ulItemIndex]);
				if( volname->VolumeGuidString[0] == L'\0' )
				{
					wcscpy_s(volname->VolumeGuidString,ARRAYSIZE(volname->VolumeGuidString),VolumeSymName);
				}
			}
		}
	}

	if( cVolNames > 0 )
	{
		qsort(VolNameArray->Volume,cVolNames,sizeof(VOLUME_NAME_STRING),&_compare_proc);

		VolNameArray->Count = cVolNames;
		*VolumeNames = VolNameArray;
	}
	else
	{
		Status = STATUS_OBJECT_NAME_NOT_FOUND;
	}
	
	CloseObjectDirectory( hObjDir );

	return RtlNtStatusToDosError(Status);
}

//---------------------------------------------------------------------------
//
//  FreeVolumeNames()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
ULONG
NTAPI
FreeVolumeNames(
	__inout VOLUME_NAME_STRING_ARRAY *VolumeNames
	)
{
	if( VolumeNames == NULL )
		return 0;

	ULONG i;
	for(i = 0; i < VolumeNames->Count; i++)
	{
		FreeMemory( (void *)VolumeNames->Volume[i].NtVolumeName );
	}

	FreeMemory(VolumeNames);

	return 0;
}

//---------------------------------------------------------------------------
//
//  EnumPhysicalDriveNames()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
ULONG
NTAPI
EnumPhysicalDriveNames(
	__inout PHYSICALDRIVE_NAME_STRING_ARRAY **PhysicalDriveNames // point to pointer to array of PHYSICALDRIVE_NAME_STRING_
	)
{
	HANDLE hObjDir;
	LONG Status;

	Status = OpenObjectDirectory( L"\\GLOBAL??", &hObjDir );

	if( Status != ERROR_SUCCESS )
		return Status;

	PHYSICALDRIVE_NAME_STRING_ARRAY *DriveNameArray = NULL;
	PHYSICALDRIVE_NAME_STRING *drivename = NULL;
	ULONG cDriveNames = 0;

	ULONG Index = 0;
	WCHAR SymLinkName[MAX_PATH];
	WCHAR DeviceName[MAX_PATH];

	Status = STATUS_SUCCESS;

	while( QueryObjectDirectory(hObjDir,&Index,SymLinkName,ARRAYSIZE(SymLinkName),NULL,0) == 0)
	{
		if( _wcsnicmp(SymLinkName,L"PhysicalDrive",13) == 0 )
		{
			DeviceName[0] = L'\0';
			QuerySymbolicLinkObject(hObjDir,SymLinkName,NULL,DeviceName,ARRAYSIZE(DeviceName));

			DriveNameArray = _allocArrayStruct<PHYSICALDRIVE_NAME_STRING_ARRAY,PHYSICALDRIVE_NAME_STRING>(DriveNameArray,++cDriveNames);

			if( DriveNameArray == NULL )
			{
				Status = STATUS_NO_MEMORY;
				break;
			}

			drivename = &(DriveNameArray->Drive[cDriveNames-1]);

			drivename->PhysicalDriveName = (const WCHAR *)DuplicateString(SymLinkName);
			drivename->DevicePath = (const WCHAR *)DuplicateString(DeviceName);
		}
	}

	if( cDriveNames > 0 )
	{
		DriveNameArray->Count = cDriveNames;
		*PhysicalDriveNames = DriveNameArray;
	}
	else
	{
		Status = STATUS_OBJECT_NAME_NOT_FOUND;
	}
	
	CloseObjectDirectory( hObjDir );

	return RtlNtStatusToDosError(Status);
}

//---------------------------------------------------------------------------
//
//  FreePhysicalDriveNames()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
ULONG
NTAPI
FreePhysicalDriveNames(
	__inout PHYSICALDRIVE_NAME_STRING_ARRAY *PhysicalDriveNames
	)
{
	if( PhysicalDriveNames == NULL )
		return 0;

	ULONG i;
	for(i = 0; i < PhysicalDriveNames->Count; i++)
	{
		FreeMemory( (void *)PhysicalDriveNames->Drive[i].PhysicalDriveName );
		FreeMemory( (void *)PhysicalDriveNames->Drive[i].DevicePath );
	}

//	HeapFree(GetProcessHeap(),0,PhysicalDriveNames);
	FreeMemory(PhysicalDriveNames);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////

static HMODULE hFltLib = NULL;
static HRESULT (WINAPI *pfnFilterGetDosName)(
			IN LPCWSTR  lpVolumeName,
			IN OUT LPWSTR  lpDosName,
			IN DWORD  dwDosNameBufferSize
			) = NULL;

//---------------------------------------------------------------------------
//
//  GetVolumeDosName()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
GetVolumeDosName(
    IN LPCWSTR  lpVolumeName,
    IN OUT LPWSTR  lpDosName,
    IN DWORD  dwDosNameBufferSize
    )
{
	if( pfnFilterGetDosName == NULL )
	{
		ANSI_STRING ProcName;
		RtlInitAnsiString(&ProcName,"FilterGetDosName");
		LdrGetProcedureAddress(hFltLib,&ProcName,0,(PVOID*)&pfnFilterGetDosName);
	}

	if( pfnFilterGetDosName )
	{
		return pfnFilterGetDosName(lpVolumeName,lpDosName,dwDosNameBufferSize);
	}

	return E_FAIL;
}

//---------------------------------------------------------------------------
//
//  LoadFltLibDll()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
LoadFltLibDll(
	HMODULE *phModule OPTIONAL
	)
{
	if( hFltLib == NULL )
	{
		NTSTATUS Status;
		UNICODE_STRING usDllName;
		RtlInitUnicodeString(&usDllName,L"fltlib.dll");
		Status = LdrLoadDll(NULL,NULL,&usDllName,&hFltLib);
	}

	if( hFltLib )
	{
		if( phModule )
			*phModule = hFltLib;
	}
	
	return (hFltLib != NULL) ? S_OK : E_FAIL;
}

//---------------------------------------------------------------------------
//
//  UnloadFltLibDll()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
UnloadFltLibDll(
	IN HMODULE hFltLib OPTIONAL
	)
{
	if( hFltLib != NULL )
	{
		LdrUnloadDll(hFltLib);
		pfnFilterGetDosName = NULL;
		hFltLib = NULL;
		return S_OK;
	}
	return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// miscellaneous

HRESULT IsSetDirtyBit(HANDLE Handle)
{
	HRESULT hr;
	//
	// Volume dirty bit flags (available in admin only)
	//
	ULONG DirtyFlags = 0;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;

	Status = NtFsControlFile(
				Handle,       // IN HANDLE  FileHandle,
				NULL,         // IN HANDLE  Event OPTIONAL,
				NULL,         // IN PIO_APC_ROUTINE  ApcRoutine OPTIONAL,
				NULL,         // IN PVOID  ApcContext OPTIONAL,
				&IoStatus,    // OUT PIO_STATUS_BLOCK  IoStatusBlock,
				FSCTL_IS_VOLUME_DIRTY, // IN ULONG  FsControlCode,
				NULL,         // IN PVOID  InputBuffer OPTIONAL,
				0,            // IN ULONG  InputBufferLength,
				&DirtyFlags,  // OUT PVOID  OutputBuffer OPTIONAL,
				sizeof(ULONG) // IN ULONG  OutputBufferLength
				);

	if( Status == STATUS_SUCCESS )
	{
		hr = (DirtyFlags != 0 ? S_OK : S_FALSE);
	}
	else
	{
		hr = HRESULT_FROM_NT(Status);
	}

	return hr;
}
