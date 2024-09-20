//****************************************************************************
//
//  misc.cpp
//
//  Miscellaneous functions
//
//  Author: YAMASHITA Katsuhiro
//
//  History: 2024-09-16 Created
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "misc.h"

LONG WINAPI GetComputerInformation(ULONG InfoClass, UINT Flags, PVOID Buffer, ULONG BufferLength)
{
	LONG Result;
	PTSTR SubKeyName = NULL;
	PTSTR EntryName = NULL;

	switch( InfoClass )
	{
		case 1: // System Root path
			SubKeyName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
			EntryName  = _T("SystemRoot");
			break;
		case 2: // System version
			SubKeyName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
			EntryName  = _T("CurrentVersion");
			break;
		case 3: // CurrentBuildNumber
			SubKeyName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
			EntryName  = _T("CurrentBuildNumber");
			break;
		case 4: // CSDVersion
			SubKeyName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
			EntryName  = _T("CSDVersion");
			break;
		case 5: // Product name
			SubKeyName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
			EntryName  = _T("ProductName");
			break;
		case 6: // Install date
			SubKeyName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
			EntryName  = _T("InstallDate");
			break;
		case 7: // Inf directory name
			SubKeyName = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
			EntryName  = _T("DevicePath");
			break;
		case 8: // System directory path
			SubKeyName = _T("SYSTEM\\CurrentControlSet\\Control\\Windows");
			EntryName  = _T("SystemDirectory");
			break;
		case 9: // Install time
			SubKeyName = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
			EntryName  = _T("InstallTime");
			break;
	}

	if ( SubKeyName == NULL || EntryName == NULL )
		return ERROR_INVALID_PARAMETER;

	REGSAM samDesired = KEY_READ;

	if( Flags & CIF_REG_WOW64_64KEY )
		samDesired |= KEY_WOW64_64KEY;

	if( Flags & CIF_REG_WOW64_32KEY )
		samDesired |= KEY_WOW64_32KEY;

	HKEY hKey;
	if( (Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,SubKeyName,0,samDesired,&hKey)) == 0 )
	{
		DWORD DataLength;
		DWORD Type;

		DataLength = BufferLength;

		Result = RegQueryValueEx(hKey,EntryName,NULL,&Type,(BYTE*)Buffer,&DataLength);

		RegCloseKey( hKey );
	}

	return Result;
}
