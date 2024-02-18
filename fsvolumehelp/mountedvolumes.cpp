//***************************************************************************
//*                                                                         *
//*  mountedvolumes.cpp                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2014-08-23,2023-10-26                                          *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "volumehelp.h"
#include "win32wdkdef.h"
#include "ntvolumehelp.h"
#include "ntnativehelp.h"
#include "storagedevice.h"
#include "debug.h"

//----------------------------------------------------------------------------
//
//  GetMountedDeviceList()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
GetMountedDeviceList(
	FS_VOL_MOUNTED_DEVICE_LIST **ppMountedDevices
	)
{
	LONG lResult;
	HKEY hKey = NULL;

	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SYSTEM\\MountedDevices", 0, KEY_QUERY_VALUE, &hKey );
	if( lResult != 0 )
	{
		return lResult;
	}

	DWORD cValues;
	DWORD cMaxValueNameLen;
	DWORD cMaxValueLen;

	RegQueryInfoKey(
			hKey,
			NULL,NULL, // lpClass,lpcClass
			NULL,      // lpReserved
			NULL,      // lpcSubKeys
			NULL,      // lpcMaxSubKeyLen
			NULL,      // lpcMaxClassLen
			&cValues,
			&cMaxValueNameLen,
			&cMaxValueLen,
			NULL,     // lpcbSecurityDescriptor
			NULL      // lpftLastWriteTime
			);

	ULONG cbList = sizeof(FS_VOL_MOUNTED_DEVICE_LIST) + sizeof(FS_VOL_MOUNTED_DEVICE) * (cValues-1);
	FS_VOL_MOUNTED_DEVICE_LIST *pList = (FS_VOL_MOUNTED_DEVICE_LIST *)_MemAllocZero(cbList);
	if( pList == NULL )
	{
		RegCloseKey(hKey);
		SetLastError( ERROR_NOT_ENOUGH_MEMORY );
		return 0;
	}

	DWORD dwIndex;
	DWORD dwDevice;
	WCHAR szValueName[256];
	DWORD cchValueName;
	BYTE Data[4096];
	DWORD cbData;

	dwIndex = dwDevice = 0;

	while( lResult != ERROR_NO_MORE_ITEMS )
	{
		cchValueName = 256;
		cbData = 4096;

		lResult = RegEnumValue(hKey,dwIndex++,szValueName,&cchValueName,NULL,NULL,Data,&cbData);

		if( lResult == 0 )
		{
			PWSTR pszObjectVolumeName = NULL;

			if( szValueName[0] == L'\\' && szValueName[1] == L'\\'
				&&  szValueName[2] == L'?' &&  szValueName[3] == L'\\' )
			{
				// Prefix : "\\?\" -> "\??\"
				// "\??\" NT dos object namespace 
				// "\\?\" Win32 recognized
				//
				SIZE_T cch = 4 + wcslen(&szValueName[4]) + 1;
				pszObjectVolumeName = _MemAllocStringBuffer( cch );
				if( pszObjectVolumeName != NULL )
				{
					wcscpy_s(pszObjectVolumeName,cch,L"\\??\\");
					wcscat_s(pszObjectVolumeName,cch,&szValueName[4]);
				}
			}

			pList->Device[dwDevice].VolumeName = _MemAllocString( pszObjectVolumeName != NULL ?  pszObjectVolumeName : szValueName );

			if( cbData == 12 )
			{
				pList->Device[dwDevice].DataType = FS_VMDT_NT4INFO;
				pList->Device[dwDevice].VolMgrLocationInfo.Signiture = *((ULONG *)Data);
				memcpy(&pList->Device[dwDevice].VolMgrLocationInfo.StartOffset.QuadPart,&Data[4],8);
			}
			else if( _IS_DMIO_SIGNATURE(Data) )
			{
				pList->Device[dwDevice].DataType = FS_VMDT_GPT_PARTITION;
				memcpy(&pList->Device[dwDevice].PartitionGuid,&Data[8],sizeof(GUID));
			}
			else
			{
				pList->Device[dwDevice].DataType = FS_VMDT_WCHAR;
				pList->Device[dwDevice].VolMgrSymbolicLinkName = _MemAllocStringBuffer( (cbData / sizeof(WCHAR)) + 1 );
				memcpy(pList->Device[dwDevice].VolMgrSymbolicLinkName,Data,cbData);
				pList->Device[dwDevice].VolMgrSymbolicLinkName[ cbData / sizeof(WCHAR) ] = L'\0';
			}

			_SafeMemFree( pszObjectVolumeName );

			dwDevice++;
		}
	}


	RegCloseKey(hKey);

	pList->Count = dwDevice;
	*ppMountedDevices = pList;

	return dwDevice;
}

//----------------------------------------------------------------------------
//
//  FreeMountedDeviceInfo()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
FreeMountedDeviceInfo(
	FS_VOL_MOUNTED_DEVICE *pDeviceInfo
	)
{
	if( pDeviceInfo->DataType == FS_VMDT_WCHAR )
	{
		_SafeMemFree( pDeviceInfo->VolMgrSymbolicLinkName );
	}

	_SafeMemFree( pDeviceInfo->VolumeName );

	return 0;
}

//----------------------------------------------------------------------------
//
//  FreeMountedDeviceList()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
FreeMountedDeviceList(
	FS_VOL_MOUNTED_DEVICE_LIST *pMountedDevices
	)
{
	DWORD dwIndex;
	for( dwIndex = 0; dwIndex < pMountedDevices->Count; dwIndex++ )
	{
		FreeMountedDeviceInfo( &pMountedDevices->Device[dwIndex] );
	}

	_MemFree( pMountedDevices );

	return 0;
}

//----------------------------------------------------------------------------
//
//  GetMountedDeviceInfo()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
//
// The pszVolumeName's style are following :
//  "\??\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" (Not trailing backslash)
//  "\DosDevices\C:"
//  "#{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
//
EXTERN_C
ULONG
WINAPI
GetMountedDeviceInfo(
	PCWSTR pszVolumeName,
	FS_VOL_MOUNTED_DEVICE *pDeviceInfo
	)
{
	ULONG lResult;
	HKEY hKey = NULL;
	
	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SYSTEM\\MountedDevices",0,KEY_QUERY_VALUE, &hKey);

	if( lResult != 0 )
	{
		return lResult;
	}

	DWORD cbData = 4096;
	BYTE Data[4096];

	PWSTR pszObjectVolumeName = NULL;

	if( pszVolumeName[0] == L'\\' && pszVolumeName[1] == L'\\'
		&&  pszVolumeName[2] == L'?' &&  pszVolumeName[3] == L'\\' )
	{
		// Prefix : "\\?\" -> "\??\"
		// "\??\" NT dos object namespace 
		// "\\?\" Win32 recognized
		//
		SIZE_T cch = 4 + wcslen(&pszVolumeName[4]) + 1;
		pszObjectVolumeName = _MemAllocStringBuffer( cch );
		if( pszObjectVolumeName != NULL )
		{
			wcscpy_s(pszObjectVolumeName,cch,L"\\??\\");
			wcscat_s(pszObjectVolumeName,cch,&pszVolumeName[4]);
		}
	}

	lResult = RegQueryValueEx(hKey,
					pszObjectVolumeName != NULL ? pszObjectVolumeName : pszVolumeName,
					NULL,NULL,Data,&cbData);

	if( lResult == 0 )
	{
		pDeviceInfo->VolumeName = _MemAllocString( pszObjectVolumeName != NULL ? pszObjectVolumeName : pszVolumeName );

		if( cbData == 12 )
		{
			pDeviceInfo->DataType = FS_VMDT_NT4INFO;
			pDeviceInfo->VolMgrLocationInfo.Signiture = *((ULONG *)Data);
			memcpy(&pDeviceInfo->VolMgrLocationInfo.StartOffset.QuadPart,&Data[4],8);
		}
		else
		{
			pDeviceInfo->DataType = FS_VMDT_WCHAR;
			pDeviceInfo->VolMgrSymbolicLinkName = _MemAllocStringBuffer( (cbData / sizeof(WCHAR)) + 1 );
			memcpy(pDeviceInfo->VolMgrSymbolicLinkName,Data,cbData);
			pDeviceInfo->VolMgrSymbolicLinkName[ cbData / sizeof(WCHAR) ] = L'\0';
		}
	}

	RegCloseKey(hKey);

	if( pszObjectVolumeName != NULL )
		_MemFree( pszObjectVolumeName );

	return lResult;
}
