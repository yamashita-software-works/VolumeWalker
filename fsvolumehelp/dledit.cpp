//*****************************************************************************
//
//  DLEDIT.cpp
//
//  PURPOSE: Drive Letter Assignment Editor
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-08-26 Created based on DLEDIT.exe
//
//*****************************************************************************
#include "stdafx.h"
#include "libntwdk.h"

#ifdef _DEBUG
   static void DebugPrint (LPCSTR pszMsg, DWORD dwErr);
   #define DEBUG_PRINT(pszMsg, dwErr) DebugPrint(pszMsg, dwErr)
#else
   #define DEBUG_PRINT(pszMsg, dwErr) NULL
#endif

#ifdef _DEBUG
void DebugPrint (LPCSTR pszMsg, DWORD dwErr)
{
	if( dwErr )
		DbgPrint("%s: %lu\n", pszMsg, dwErr);
	else
		DbgPrint("%s\n", pszMsg);
}
#endif

//----------------------------------------------------------------------------
//
//  DLEditRemoveDrive()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
DLEditRemoveDrive(
	PCWSTR pszDriveLetter
	)
{
	WCHAR szDriveLetterAndSlash[4];
	WCHAR szDriveLetter[3];
	BOOL  fResult;
	HRESULT hr;

	// GetVolumeNameForVolumeMountPoint, SetVolumeMountPoint, and
	// DeleteVolumeMountPoint require drive letters to have a trailing 
	// backslash. However, DefineDosDevice requires that the trailing 
	// backslash be absent. So, use:
	// 
	//    szDriveLetterAndSlash     for the mounted folder functions
	//    szDriveLetter             for DefineDosDevice
	// 
	// This way, command lines (argument) that use a: or a:\ 
	// for drive letters can be accepted without writing back to the original argument.

	szDriveLetter[0] = pszDriveLetter[0];
	szDriveLetter[1] = L':';
	szDriveLetter[2] = L'\0';

	szDriveLetterAndSlash[0] = pszDriveLetter[0];
	szDriveLetterAndSlash[1] = L':';
	szDriveLetterAndSlash[2] = L'\\';
	szDriveLetterAndSlash[3] = L'\0';

	fResult = DeleteVolumeMountPoint( szDriveLetterAndSlash );

	if( fResult )
	{
		hr = S_OK;
	}
	else
	{
		hr = HRESULT_FROM_WIN32( GetLastError() );
	}

	return hr;
}

//----------------------------------------------------------------------------
//
//  DLEditAssignDrive()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
DLEditAssignDrive(
	PCWSTR pszDriveLetter,
	PCWSTR pszNTDevice
	)
{
	TCHAR szUniqueVolumeName[MAX_PATH];
	TCHAR szDriveLetterAndSlash[4];
	TCHAR szDriveLetter[3];
	BOOL  fResult;
	HRESULT hr;

	// GetVolumeNameForVolumeMountPoint, SetVolumeMountPoint, and
	// DeleteVolumeMountPoint require drive letters to have a trailing 
	// backslash. However, DefineDosDevice requires that the trailing 
	// backslash be absent. So, use:
	// 
	//    szDriveLetterAndSlash     for the mounted folder functions
	//    szDriveLetter             for DefineDosDevice
	// 
	// This way, command lines that use a: or a:\ 
	// for drive letters can be accepted without writing back to the original command-
	// line argument.

	szDriveLetter[0] = pszDriveLetter[0];
	szDriveLetter[1] = TEXT(':');
	szDriveLetter[2] = TEXT('\0');

	szDriveLetterAndSlash[0] = pszDriveLetter[0];
	szDriveLetterAndSlash[1] = TEXT(':');
	szDriveLetterAndSlash[2] = TEXT('\\');
	szDriveLetterAndSlash[3] = TEXT('\0');


	// To add a drive letter that persists through reboots, use
	// SetVolumeMountPoint. This requires the volume GUID path 
	// of the device to which the new drive letter will refer. 
	// To get the volume GUID path, use 
	// GetVolumeNameForVolumeMountPoint; it requires the drive 
	// letter to already exist. So, first define the drive 
	// letter as a symbolic link to the device name. After  
	// you have the volume GUID path the new drive letter will 
	// point to, you must delete the symbolic link because the 
	// mount manager allows only one reference to a device at a 
	// time (the new one to be added).

	fResult = DefineDosDevice (DDD_RAW_TARGET_PATH, szDriveLetter,
                                 pszNTDevice);

	if( fResult )
	{
		// If GetVolumeNameForVolumeMountPoint fails, then 
		// SetVolumeMountPoint will also fail. However, 
		// DefineDosDevice must be called to remove the temporary symbolic link. 
		// Therefore, set szUniqueVolume to a known empty string.

		if( !GetVolumeNameForVolumeMountPoint (szDriveLetterAndSlash,
				szUniqueVolumeName,
				MAX_PATH) )
		{
			DEBUG_PRINT("GetVolumeNameForVolumeMountPoint failed",GetLastError());
			szUniqueVolumeName[0] = '\0';
		}

		fResult = DefineDosDevice( 
						DDD_RAW_TARGET_PATH|DDD_REMOVE_DEFINITION|
						DDD_EXACT_MATCH_ON_REMOVE, szDriveLetter,
						pszNTDevice);

		if( fResult )
		{
			fResult = SetVolumeMountPoint(szDriveLetterAndSlash,szUniqueVolumeName);

			if( fResult )
			{
				hr = S_OK;
			}
			else
			{
				DEBUG_PRINT("SetVolumeMountPoint failed",GetLastError());
				hr = HRESULT_FROM_WIN32( GetLastError() );
			}
		}
		else
		{
			DEBUG_PRINT("DefineDosDevice failed",GetLastError());
			hr = HRESULT_FROM_WIN32( GetLastError() );
		}
	}
	else
	{
		DEBUG_PRINT("GetVolumeNameForVolumeMountPoint failed",GetLastError());
		hr = HRESULT_FROM_WIN32( GetLastError() );
	}

	return hr;
}
