//***************************************************************************
//*                                                                         *
//*  filelibw32.cpp                                                         *
//*                                                                         *
//*  Win32 API functions                                                    *
//*                                                                         *
//*  Create: 2024-12-07                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "fsfilelib.h"

EXTERN_C
HRESULT
APIENTRY
NtPathParseDeviceName(
	PCWSTR pszPath,
	PWSTR pszDeviceName,
	int cchDeviceName,
	PWSTR pszDosDeviceName,
	int cchDosDeviceName
	)
{
	HRESULT hr;
	WCHAR szVolume[MAX_PATH];
	SIZE_T cch;

	cch = wcslen(pszPath);

	if( pszDeviceName )
		*pszDeviceName = L'\0';

	if( pszDosDeviceName )
		*pszDosDeviceName = L'\0';
	
	if( (cch >= 4 && pszPath[0] == L'\\' && pszPath[1] == L'?' && pszPath[2] == L'?' && pszPath[3] == L'\\') ||
		(cch >= 4 && pszPath[0] == L'\\' && pszPath[1] == L'\\' && (pszPath[2] == L'.' || pszPath[2] == L'?') && pszPath[3] == L'\\') ||
		(cch >= 8 && (wcsnicmp(pszPath,L"\\Device\\",8) == 0)) )
	{
		// NOTE: UNC path not supports

		WCHAR *pSep = NULL;
		WCHAR *pHead = NULL;

		if( iswalpha(pszPath[4]) && pszPath[5] == L':' )
		{
			// "\??\C:"
			// "\\?\C:"
			pHead = (WCHAR*)&pszPath[4];
		}
		else if( cch >= 48 && pszPath[10] == L'{' && pszPath[47] == L'}' )
		{
			// "\??\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
			// "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
			pHead = (WCHAR*)&pszPath[4];
		}
		else if( pszPath[2] == L'?' || pszPath[2] == L'.' )
		{
			// "\??\HardiskVolume1"
			// "\??\Cdrom1"
			// "\\?\C:"
			// "\\.\C:"
			pHead = (WCHAR*)&pszPath[4];
		}
		else if( pszPath[2] != L'?' )
		{
			if( _wcsnicmp(&pszPath[8],L"LanmanRedirector",16) == 0 )
			{
				// "\Device\LanmanRedirector\xxxxxxxx"
				if( pszDeviceName )
				{
					if( FindDeviceNameFromPath(pszPath,pszDeviceName,cchDeviceName,pszDosDeviceName,cchDosDeviceName) != -1 )
					{
						pHead = NULL;
						pSep  = NULL;
						hr = S_OK;
					}
				}
			}
			else
			{
				// "\Device\xxxxxxxx"
				pHead = (WCHAR*)&pszPath[8];
			}
		}
		else
		{
			// invalid/unknown path
			pHead = (WCHAR*)pszPath;
		}

		if( pHead != NULL )
		{
			pSep = wcspbrk(pHead,L"\\/");

			if( pSep )
			{
				SIZE_T cb = (((SIZE_T)(pSep - pHead)) * sizeof(WCHAR));
				memcpy_s(szVolume,sizeof(szVolume),pHead,cb);
				szVolume[ WCHAR_LENGTH(cb) ] = L'\0';
			}
			else
			{
				StringCchCopy(szVolume,MAX_PATH,pHead);
			}

			if( QueryDosDevice(szVolume,pszDeviceName,cchDeviceName) == 0 )
			{
				memset(pszDeviceName,0,WCHAR_BYTES(cchDeviceName));
			}

			if( pszDosDeviceName )
			{
				StringCchCopy(pszDosDeviceName,cchDosDeviceName,szVolume);
			}

			hr = S_OK;
		}
	}
	else
	{
		hr = S_FALSE;
	}

	return hr;
}

#define _MAX_TARGET_PATH_BUFFER_SIZE 32768

EXTERN_C
HRESULT
APIENTRY
DosDriveFromNtDevicePath(
	PCWSTR NtDevicePath,
	PWSTR DosDrive,
	ULONG cchDosDrive,
	ULONG Flags,
	PCWSTR *RootDirectoryPart
	)
{
	WCHAR drive_buffer[ (26 * (3 + 1)) + 1 ]; // "'A:\'\0 ... 'Z:\'\0\0"
	WCHAR *drive;
	const DWORD cchTargetPathBuffer = _MAX_TARGET_PATH_BUFFER_SIZE;
	WCHAR szTargetPath[_MAX_TARGET_PATH_BUFFER_SIZE];
	DWORD cchTargetPath;
	HRESULT hr = S_FALSE;

	GetLogicalDriveStrings(ARRAYSIZE(drive_buffer),drive_buffer);

	*DosDrive = L'\0';

	drive = drive_buffer;

	while( *drive )
	{
		// "C:\" -> "C:"
		drive[2] = L'\0';
		cchTargetPath = QueryDosDevice(drive,szTargetPath,cchTargetPathBuffer);
		drive[2] = L'\\';

		int iret;
		if( Flags & DDNTF_DEVICENAME_COMPARE )
		{
			iret = HasPrefix(szTargetPath,NtDevicePath) ? 0 : 1;

			if( iret == 0 && RootDirectoryPart && (Flags && DDNTF_RETURN_ROOTDIRECTORY_POINT) )
			{
				SIZE_T cchTargetPath = wcslen(szTargetPath);
				SIZE_T cchNtDevicePath = wcslen(NtDevicePath);

				if( cchNtDevicePath >= cchTargetPath )
				{
					*RootDirectoryPart = &NtDevicePath[ cchTargetPath ];
				}
			}
		}
		else
		{
			iret = wcsicmp(szTargetPath,NtDevicePath);
		}

		if( iret == 0 )
		{
			if( (Flags & DDNTF_RETURN_DRIVE_ROOT) == 0 )
				drive[2] = L'\0';
			hr = StringCchCopy(DosDrive,cchDosDrive,drive);
			break;
		}

		drive += (wcslen(drive) + 1);
	}

	return hr;
}
