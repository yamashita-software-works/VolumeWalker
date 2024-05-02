//***************************************************************************
//*                                                                         *
//*  fsfilelib.cpp                                                          *
//*                                                                         *
//*  NT native API functions                                                *
//*                                                                         *
//*  Create: 2023-04-26                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include <ntifs.h>
#include <windef.h>
#include <ntstatus.h>
#include <winerror.h>
#include <strsafe.h>
#include <malloc.h>
#include "libntwdk.h"
#include "ntnativeapi.h"
#include "ntobjecthelp.h"
#include "fsfilelib.h"

//
// Utility
//
EXTERN_C
ULONG
APIENTRY
_StringFromGUID(
	const GUID *pGuid,
	PWSTR pszGuid,
	ULONG cchMax
	)
{
	return RtlNtStatusToDosError( StringFromGUID(pGuid,pszGuid,cchMax) );
}

EXTERN_C
BOOL
APIENTRY
NtPathIsRootDirectory(
	PCWSTR pszPath
	)
{
	UNICODE_STRING us;
	RtlInitUnicodeString(&us,pszPath);
	return IsRootDirectory_U(&us) ? TRUE : FALSE;
}

EXTERN_C
BOOL
APIENTRY
_HasPrefix(
	PCWSTR pszPrefix,
	PCWSTR pszPath
	)
{
	return HasPrefix(pszPrefix,pszPath);
}

//
// NtPath
//
EXTERN_C
UINT
APIENTRY
NtPathIsNtDevicePath(
	PCWSTR pszPath
	)
{
	return (UINT)IsNtDevicePath(pszPath);
}

EXTERN_C
BOOL
APIENTRY
NtPathGetRootDirectory(
	PCWSTR pszPath,
	PWSTR pszRootDir,
	ULONG cchRootDir
	)
{
	if( pszPath == NULL || pszRootDir == NULL || cchRootDir == 0)
		return FALSE;

	UNICODE_STRING usPath;
	RtlInitUnicodeString(&usPath,pszPath);
	if( GetRootDirectory_U(&usPath) == false )
	{
		return FALSE;
	}

	if( WCHAR_LENGTH(usPath.Length) >= cchRootDir )
	{
		// Copy as much as possible.
		memcpy(pszRootDir,usPath.Buffer,WCHAR_BYTES(cchRootDir));
		pszRootDir[ cchRootDir-1 ] = UNICODE_NULL;
		return FALSE;
	}

	memcpy(pszRootDir,usPath.Buffer,usPath.Length);
	pszRootDir[ WCHAR_LENGTH(usPath.Length) ] = UNICODE_NULL;

	return TRUE;
}

EXTERN_C
BOOL
APIENTRY
NtPathGetVolumeName(
	PCWSTR pszPath,
	PWSTR pszVolumeName,
	ULONG cchVolumeName
	)
{
	if( pszPath == NULL || pszVolumeName == NULL || cchVolumeName == 0)
		return FALSE;

	UNICODE_STRING usPath;
	RtlInitUnicodeString(&usPath,pszPath);
	if( GetVolumeName_U(&usPath) == false )
	{
		return FALSE;
	}

	if( WCHAR_LENGTH(usPath.Length) >= cchVolumeName )
	{
		return FALSE;
	}

	memcpy(pszVolumeName,usPath.Buffer,usPath.Length);

	pszVolumeName[ WCHAR_LENGTH(usPath.Length) ] = UNICODE_NULL;

	return TRUE;
}

EXTERN_C
BOOL
APIENTRY
NtPathFileExists(
	PCWSTR pszPath
	)
{
	return PathFileExists_W(pszPath,NULL);
}

EXTERN_C
BOOL
APIENTRY
NtPathQueryVolumeDeviceName(
	PCWSTR pszGrobalRootPrefixedPath,
	PWSTR pszBuffer,
	ULONG cchBuffer
	)
{
	UNICODE_STRING us;
	RtlInitUnicodeString(&us,pszGrobalRootPrefixedPath);
	if( PathIsPrefixDosDevice_U(&us) )
	{
		GetVolumeName_U(&us);
		QuerySymbolicLinkObject(NULL,NULL,&us,pszBuffer,cchBuffer);
	}
	return TRUE;
}

static BOOLEAN _compare_target_name(HANDLE hObjDir,WCHAR *SymName,UNICODE_STRING *NtVolumeDeviceName)
{
	WCHAR buf[32];
    UNICODE_STRING usLinkTarget;
	usLinkTarget.Length = 0;
	usLinkTarget.MaximumLength = sizeof(buf);
	usLinkTarget.Buffer = buf;
	if( QuerySymbolicLinkObject_U(hObjDir,SymName,&usLinkTarget,FALSE) == 0 )
	{
		if( RtlCompareUnicodeString(&usLinkTarget,NtVolumeDeviceName,TRUE) == 0 )
		{
			return TRUE;
		}
	}
	return FALSE;
}

static bool __stdcall _lookup_guid(const WCHAR *SymName,const UNICODE_STRING*)
{
	return ((_wcsnicmp(SymName,L"Volume{",7) == 0 && (SymName[43] == L'}' && SymName[44] == '\0')));
}

static bool __stdcall _lookup_dos_drive(const WCHAR *SymName,const UNICODE_STRING*)
{
	return (iswalpha(SymName[0]) && SymName[1] == L':');
}

static bool __stdcall _lookup_from_nt_devicename(const WCHAR *SymName,const UNICODE_STRING *NtPath)
{
	// &NtPath->Buffer[8] skip "\Device\" and compare trailing string.
	// e.g.
	//  "\Device\HarddiskVolume1"
	//  "\Device\Cdrom1"
	//
	return (_wcsnicmp(SymName,&NtPath->Buffer[8],(NtPath->Length/sizeof(WCHAR)-8)) == 0);
}

static bool __stdcall _lookup_from_dos_namespace(const WCHAR *SymName,const UNICODE_STRING *NtPath)
{
	// &NtPath->Buffer[4] skip "\??\" and compare trailing string.
	// e.g.
	//  "\??\C:
	//  "\??\HarddiskVolume1"
	//
	return (_wcsnicmp(SymName,&NtPath->Buffer[4],(NtPath->Length/sizeof(WCHAR)-4)) == 0);
}

typedef bool(__stdcall *COMP_FUNC_PTR)(const WCHAR *,const UNICODE_STRING *);

EXTERN_C
BOOL
APIENTRY
NtPathTranslatePath(
	PCWSTR pszNtPath,
	ULONG Flags,
	PWSTR pszPath,
	ULONG cchPath
	)
{
    HANDLE hObjDir;
    NTSTATUS Status;
	DWORD dwError;
	BOOL bResult = FALSE;
    ULONG Index = 0;
    WCHAR SymName[260];
    UNICODE_STRING usNtPathVolume;
	UNICODE_STRING usVolumeRelativePath;
	UINT PathType = 0;

	if( !SplitVolumeRelativePath(pszNtPath,&usNtPathVolume,&usVolumeRelativePath) )
	{
		_SetLastStatusDos( STATUS_OBJECT_PATH_INVALID );
        return FALSE;
	}

	if( HasPrefix(L"\\??\\",pszNtPath) )
	{
		PathType = 3;
	}
	else
	{
		switch( PTF_TYPE_MASK & Flags )
		{
		case PTF_GUID:
			PathType = 0;
			break;
		case PTF_DOSDRIVE:
			PathType = 1;
			break;
		case PTF_DOSNAMESPACE:
			PathType = 2;
			break;
		default:
			_SetLastStatusDos( STATUS_INVALID_PARAMETER );
			return FALSE;
		}
	}

	static COMP_FUNC_PTR cmp_func_ptr[] = {
		&_lookup_guid,
		&_lookup_dos_drive,
		&_lookup_from_nt_devicename,
		&_lookup_from_dos_namespace,
	};
	COMP_FUNC_PTR cmp_fnc = cmp_func_ptr[PathType];

#ifdef _DEBUG
	PWSTR p1 = AllocateSzFromUnicodeString(&usNtPathVolume);
	PWSTR p2 = AllocateSzFromUnicodeString(&usVolumeRelativePath);
	FreeMemory(p2);
	FreeMemory(p1);
#endif

    Status = OpenObjectDirectory( L"\\GLOBAL??", &hObjDir );

    if( Status == STATUS_SUCCESS )
    {
        while( (dwError = QueryObjectDirectory(hObjDir,&Index,SymName,ARRAYSIZE(SymName),NULL,0)) == ERROR_SUCCESS )
        {
			if( cmp_fnc(SymName,&usNtPathVolume) )
            {
				if( (PathType == 3) || _compare_target_name(hObjDir,SymName,&usNtPathVolume) )
				{
					*pszPath = UNICODE_NULL;
					if( Flags & PTF_NTDOSNAMESPACEPREFIX )
						StringCchCat(pszPath,cchPath,L"\\??\\");  // NT namespace win32 global root
					else if( Flags & PTF_WIN32PATHPREFIX )
						StringCchCat(pszPath,cchPath,L"\\\\?\\"); // win32 file namespace
					StringCchCat(pszPath,cchPath,SymName);
					if( usVolumeRelativePath.Buffer )
						StringCchCat(pszPath,cchPath,usVolumeRelativePath.Buffer);
					bResult = TRUE;
					break;
				}
			}
        }

        CloseObjectDirectory( hObjDir );
    }

	_SetLastStatusDos( bResult ? STATUS_SUCCESS : STATUS_OBJECT_NAME_NOT_FOUND );

    return bResult;
}

EXTERN_C
BOOL
APIENTRY
NtPathToDosPath(
	PCWSTR pszNtPath,
	PWSTR pszPath,
	ULONG cchPath
	)
{
	return NtPathTranslatePath(pszNtPath,PTF_DOSDRIVE,pszPath,cchPath);
}

EXTERN_C
BOOL
APIENTRY
NtPathToDosPathEx(
	PCWSTR pszNtPath,
	PWSTR pszPath,
	ULONG cchPath,
	ULONG Flags
	)
{
	if( (Flags & PTF_TYPE_MASK) == 0 )
		Flags |= PTF_DOSDRIVE;

	return NtPathTranslatePath(pszNtPath,
				Flags | (Flags & PTF_PREFIX_MASK),
				pszPath,cchPath);
}

EXTERN_C
BOOL
APIENTRY
NtPathToGuidPath(
	PCWSTR pszNtPath,
	PWSTR pszPath,
	ULONG cchPath,
	ULONG Flags
	)
{
	return NtPathTranslatePath(pszNtPath,PTF_GUID|(Flags & PTF_PREFIX_MASK),
			pszPath,cchPath);
}


EXTERN_C
DWORD
APIENTRY
NtPathGetLongPathNameFromHandle(
	HANDLE hFile,
	PWSTR *LongPathName,
	PULONG pcbLongPathName
	)
{
	NTSTATUS Status;
	Status = GetLongPathNameFromHandle(hFile,LongPathName,pcbLongPathName);
	RtlSetLastWin32Error( Status );
	return RtlNtStatusToDosError(Status);
}

//---------------------------------------------------------------------------
//
//  DosPathToNtDevicePath()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
BOOL
APIENTRY
DosPathToNtDevicePath(
	PCWSTR pszDosPath,
	PWSTR pszNtPathBuffer,
	ULONG cchNtPathBuffer,
	ULONG Flags
	)
{
	UNICODE_STRING VolumeName;
	UNICODE_STRING RootPath;
	BOOL bSuccess = FALSE;

	if( !SplitVolumeRelativePath(pszDosPath,&VolumeName,&RootPath) )
	{
		_SetLastStatusDos(STATUS_INVALID_PARAMETER);
		return FALSE;
	}

	PWSTR pVolume = AllocateSzFromUnicodeString(&VolumeName);
	PWSTR pRootPath = AllocateSzFromUnicodeString(&RootPath);

	if( pVolume && pRootPath )
	{
		WCHAR szNtVolume[64];
		RtlZeroMemory(szNtVolume,sizeof(szNtVolume));

		if( QuerySymbolicLinkObjectName(L"\\Global??",pVolume,szNtVolume,ARRAYSIZE(szNtVolume),NULL) != 0 )
		{
			if( QuerySymbolicLinkObjectName(L"\\??",pVolume,szNtVolume,ARRAYSIZE(szNtVolume),NULL) != 0 )
			{
				;
			}
		}

		if( szNtVolume[0] != L'\0' )
		{
			PWSTR pszNtDevicePath;
			pszNtDevicePath = CombinePath(szNtVolume,pRootPath);

			if( pszNtDevicePath )
			{
				HRESULT hr;
				hr = StringCchCopy(pszNtPathBuffer,cchNtPathBuffer,pszNtDevicePath);
				if( hr == S_OK )
				{
					_SetLastWin32Error( ERROR_SUCCESS );
					bSuccess = TRUE;
				}
				else
				{
					_SetLastWin32Error( HRESULT_CODE(hr) );
				}
			}
			else
			{
				_SetLastStatusDos( STATUS_NO_MEMORY );
			}
		}
	}
	else
	{
		_SetLastStatusDos( STATUS_NO_MEMORY );
	}

	FreeMemory(pVolume);
	FreeMemory(pRootPath);

	return bSuccess;
}
