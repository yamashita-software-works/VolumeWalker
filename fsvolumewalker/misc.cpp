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

HICON GetAppropriateVolumeIconByPath(PCWSTR pszPath)
{
	HICON hIcon = NULL;
	if( pszPath[1] == L':' && _DOS_DRIVE_CHAR(pszPath[0]) )
	{
		SHSTOCKICONID siid = (SHSTOCKICONID)0;
		UINT Type = GetDriveType(pszPath);
		switch( Type )
		{
			case DRIVE_REMOVABLE:
				siid = SIID_DRIVEREMOVE;
				break;
			case DRIVE_FIXED:
				siid = SIID_DRIVEFIXED;
				break;
			case DRIVE_REMOTE:
				siid = SIID_DRIVENET;
				break;
			case DRIVE_CDROM:
				siid = SIID_DRIVECD;
				break;
			case DRIVE_RAMDISK:
				siid = SIID_DRIVERAM;
				break;
			default:
				hIcon = GetShellFileIcon(pszPath,SHGFI_SMALLICON);
				break;
		}
		if( siid != (SHSTOCKICONID)0 )
			hIcon = GetShellStockIcon(siid);
	}
	else
	{
		WCHAR szVolume[MAX_PATH];
		NtPathGetVolumeName(pszPath,szVolume,MAX_PATH);
			HANDLE hVolume;
		if( OpenVolume(szVolume,0,&hVolume) == 0 )
		{
			SHSTOCKICONID siid;
			ULONG DeviceType;
			ULONG Characteristics;
			GetVolumeDeviceType(hVolume,&DeviceType,&Characteristics);
			switch( DeviceType )
			{
				case FILE_DEVICE_CD_ROM:
					siid = SIID_DRIVECD;
					break;
				case FILE_DEVICE_DISK:
					if( Characteristics & (FILE_REMOVABLE_MEDIA|FILE_PORTABLE_DEVICE) )
						siid = SIID_DRIVEREMOVE;
					else 
						siid = SIID_DRIVEFIXED;
					break;
				default:
					siid = SIID_DRIVEFIXED;
					break;
			}
			CloseHandle(hVolume);
				hIcon = GetShellStockIcon(siid);
		}
		else
		{
			if( PathIsPrefixDosDeviceDrive(szVolume) )
			{
				hIcon = GetShellFileIcon(&szVolume[4],SHGFI_SMALLICON);
			}
			else
			{
				if( pszPath[1] == L':' && _DOS_DRIVE_CHAR(pszPath[0]) )
					hIcon = GetShellFileIcon(szVolume,SHGFI_SMALLICON);
				else
					hIcon = GetShellStockIcon(SIID_DRIVEFIXED);
			}
		}
	}
	return (HICON)hIcon;
}
