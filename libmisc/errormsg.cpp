//***************************************************************************
//*                                                                         *
//*  errormsg.cpp                                                           *
//*                                                                         *
//*  Create: 2023-04-27                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "libmisc.h"

int _GetSystemErrorMessageEx(ULONG ErrorCode,PWSTR *ppMessage,DWORD dwLanguageId)
{
	HMODULE hModule = NULL;
	DWORD f = 0;

	if( ( ErrorCode & 0xC0000000) == 0xC0000000 )
	{
		hModule = GetModuleHandle(L"ntdll.dll");
		f = FORMAT_MESSAGE_FROM_HMODULE;
	}

	LPVOID pMessageBuf;
	DWORD cch;
	cch = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS |
				f,
				hModule,
				ErrorCode,
				dwLanguageId,
			    (LPTSTR) &pMessageBuf,
				0,
				NULL 
				);

	if( cch != 0 )
	{
		*ppMessage = (PWSTR)pMessageBuf;
		return cch;
	}
	return 0;
}

int _GetSystemErrorMessage(ULONG ErrorCode,PWSTR *ppMessage)
{
	return _GetSystemErrorMessageEx(ErrorCode,ppMessage,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT));
}

void _FreeSystemErrorMessage(PWSTR pMessage)
{
	LocalFree(pMessage);
}

PWSTR FormatNtStatusErrorMessage(PCWSTR NtStatusErrorMessage,PWSTR Buffer,SIZE_T cchBufferLength,ULONG Flags)
{
	if( NtStatusErrorMessage == NULL || Buffer == NULL )
		return NULL;

	WCHAR *pbe = NULL;
	WCHAR *pbs = wcschr(NtStatusErrorMessage,L'{');
	if( pbs )
	{
		pbs++;
		pbe = wcschr(pbs,L'}');
		if( pbe )
		{
			*pbe = L'\0';
			StringCchCopy(Buffer,cchBufferLength,pbs);
			pbe++;
		}
		else
		{
			pbe = (PWSTR)NtStatusErrorMessage; // invalid format
		}

		StringCchCat(Buffer,cchBufferLength,pbe);
	}
	else
	{
		// Bracket not found
		StringCchCopy(Buffer,cchBufferLength,NtStatusErrorMessage);
	}
	return Buffer;
}

PWSTR _cdecl _MakeMessageString(UINT idFormatRes, LPCWSTR lpszFormat, ...)
{
	BOOLEAN bLoadFormatString = FALSE;
	if( lpszFormat == NULL )
	{
		PWSTR psz;
		int cch = _LoadStringResource(idFormatRes,&psz);
		if( cch == 0 )
			return NULL;
		psz = _MemAllocStringBuffer(cch + 1);
		if( psz == NULL )
			return NULL;
		bLoadFormatString = TRUE;
	}

	va_list args;
	va_start(args, lpszFormat);

	size_t cch = 32768;
	WCHAR *szBuffer = _MemAllocStringBuffer( cch );
	if( szBuffer != NULL )
	{
		int nBuf;
		nBuf = _vsnwprintf_s(szBuffer, cch, cch, lpszFormat, args);

		if( nBuf == -1 )
		{
			_MemFree(szBuffer);
			szBuffer = NULL;
		}
	}

	va_end(args);

	if( bLoadFormatString )
		_MemFree((void *)lpszFormat);

	return szBuffer;
}
