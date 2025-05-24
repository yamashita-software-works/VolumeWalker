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
