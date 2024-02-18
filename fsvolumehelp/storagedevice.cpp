//***************************************************************************
//*                                                                         *
//*  storagedevice.cpp                                                      *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2023-10-26                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include <setupapi.h>
#include "volumehelp.h"
#include "win32wdkdef.h"
#include "ntvolumehelp.h"
#include "ntnativehelp.h"
#include "ntobjecthelp.h"
#include "storagedevice.h"
#include "debug.h"
#define INITGUID
#include <devpkey.h>
#include <devguid.h>

static HMACHINE m_hMachine = NULL; // reserved

static
BOOL 
_GetRelationInformation(
	HMACHINE hMachine,
	LPCTSTR DeviceId,
	ULONG Flags,
	PWCHAR *ppBuffer
	)
{
	ULONG len = 0;

	if( CM_Get_Device_ID_List_Size_Ex(&len,DeviceId,Flags,hMachine) == CR_SUCCESS )
	{
		if( len != 0 )
		{
			PWCHAR pBuffer = (PWCHAR)_MemAlloc( len * sizeof(WCHAR) );

			if( CM_Get_Device_ID_List_Ex(
					DeviceId,
					pBuffer,
					len,
					Flags,
					hMachine ) == CR_SUCCESS )
			{
				*ppBuffer = pBuffer;
				return TRUE;
			}
		}
	}

	return FALSE;
}

static
PWSTR
_GetIdList_Relation(
	PCWSTR pszDeviceInstanceId,
	ULONG Flag
	)
{
	// Get relation device information. (assigned physical device object path)
	//
	PWSTR pDevIds;

	if( _GetRelationInformation(NULL,pszDeviceInstanceId,Flag,&pDevIds) )
	{
		return pDevIds;
	}
	return NULL;
}

static
int 
_GetDeviceLocationInfo(
	HMACHINE hMachine,
	DEVNODE dn,
	LPWSTR *ppClassGuid,
	LPWSTR *ppName,
	LPWSTR *ppFriendlyName,
	LPWSTR *ppNtDeviceName,
	LPWSTR *ppLocation
	)
{
	WCHAR wbuf[4096];
	ULONG cb;

	// Friendly Name
	if( ppFriendlyName != NULL )
	{
		cb = 1024;
		wbuf[0] = 0;
		if( CM_Get_DevNode_Registry_Property_Ex(dn,CM_DRP_FRIENDLYNAME,NULL,wbuf,&cb,0,hMachine) == 0 )
		{
			*ppFriendlyName = _MemAllocString( wbuf );
		}
		else
		{
			*ppFriendlyName = NULL;
		}
	}

	// Description
	if( ppName != NULL )
	{
		cb = 1024;
		wbuf[0] = 0;
		if( CM_Get_DevNode_Registry_Property_Ex(dn,CM_DRP_DEVICEDESC,NULL,wbuf,&cb,0,hMachine) == 0 )
		{
			*ppName = _MemAllocString( wbuf );
		}
		else
		{
			*ppName = NULL;
		}
	}

	// Class GUID
	if( ppClassGuid != NULL )
	{
		cb = 1024;
		wbuf[0] = 0;
		if( CM_Get_DevNode_Registry_Property_Ex(dn,CM_DRP_CLASSGUID,NULL,wbuf,&cb,0,hMachine) == 0 )
		{
			*ppClassGuid = _MemAllocString( wbuf );
		}
		else
		{
			*ppClassGuid = NULL;
		}
	}

	// NT Device name
	if( ppNtDeviceName != NULL )
	{
		cb = 1024;
		wbuf[0] = 0;
		if( CM_Get_DevNode_Registry_Property_Ex(dn,CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME,NULL,wbuf,&cb,0,hMachine) == 0 )
		{
			*ppNtDeviceName = _MemAllocString( wbuf );
		}
		else
		{
			*ppNtDeviceName = NULL;
		}
	}

	// Location
	if( ppLocation != NULL )
	{
		cb = 1024;
		wbuf[0] = 0;
		if( CM_Get_DevNode_Registry_Property_Ex(dn,CM_DRP_LOCATION_INFORMATION,NULL,wbuf,&cb,0,hMachine) == 0 )
		{
			*ppLocation = _MemAllocString( wbuf );
		}
		else
		{
			*ppLocation = NULL;
		}
	}

	return 0;
}

static
void
_GetDiskDriveInformation(
	FS_STORAGE_DEVICE_NAME *pDeviceName,
	LPCTSTR DeviceId
	)
{
	DEVNODE dn;
	DWORD dwFlags = CM_LOCATE_DEVNODE_NORMAL|CM_LOCATE_DEVNODE_PHANTOM;

	if( CM_Locate_DevNode_Ex(&dn,(PWSTR)DeviceId,dwFlags,m_hMachine) != 0 )
	{
		return;
	}

	LPWSTR pFriendlyName = NULL;
	LPWSTR pLocation = NULL;
	LPWSTR pNtDeviceName = NULL;
	LPWSTR pName = NULL;

	_GetDeviceLocationInfo(m_hMachine,dn,NULL,&pName,&pFriendlyName,&pNtDeviceName,&pLocation);

	if( pFriendlyName != NULL )
		pDeviceName->FriendlyName = _MemAllocString(pFriendlyName);
	else
		pDeviceName->FriendlyName = _MemAllocString(pName);

	if( pLocation != NULL )
		pDeviceName->Location = _MemAllocString(pLocation);

	if( pNtDeviceName != NULL )
		pDeviceName->NtDeviceName = _MemAllocString(pNtDeviceName);

	pDeviceName->DeviceId = _MemAllocString(DeviceId);

	_SafeMemFree( pName );
	_SafeMemFree( pFriendlyName );
	_SafeMemFree( pLocation );
	_SafeMemFree( pNtDeviceName );

	return;
}

static
void
insertDosDrive(
	PWCHAR VolumeName
	)
{
    DWORD  CharCount = MAX_PATH + 1;
    PWCHAR Names     = NULL;
    PWCHAR NameIdx   = NULL;
    BOOL   Success   = FALSE;

    for (;;) 
    {
        //  Allocate a buffer to hold the paths.
        Names = new WCHAR [CharCount];

        if ( !Names ) 
        {
            //  If memory can't be allocated, return.
            return;
        }

        //  Obtain all of the paths
        //  for this volume.
		if( (Success = GetVolumePathNamesForVolumeName(VolumeName, Names, CharCount, &CharCount)) == TRUE )
		{
            break;
        }

        if ( GetLastError() != ERROR_MORE_DATA ) 
        {
            break;
        }

        //  Try again with the
        //  new suggested size.
        delete [] Names;
        Names = NULL;
    }

    if ( Success )
    {
		int count = 0;

        //  Display the various paths.
        for ( NameIdx = Names; 
              NameIdx[0] != L'\0'; 
              NameIdx += wcslen(NameIdx) + 1 ) 
        {
			count++;
        }
    }

    if ( Names != NULL ) 
    {
        delete [] Names;
        Names = NULL;
    }

    return;
}

static
void
_GetVolumeInformation(
	LPCTSTR DeviceId,
	FS_STORAGE_DEVICE_NAME *pStorageDevice
	)
{
	DEVNODE dn;
	DWORD dwFlags = CM_LOCATE_DEVNODE_NORMAL|CM_LOCATE_DEVNODE_PHANTOM;

	if( CM_Locate_DevNode_Ex(&dn,(PWSTR)DeviceId,dwFlags,NULL) != 0 )
	{
		return;
	}

	LPWSTR pClassGuid;
	LPWSTR pName;
	LPWSTR pFriendlyName;
	LPWSTR pNtDeviceName;
	LPWSTR pLocation;

	_GetDeviceLocationInfo(NULL,dn,&pClassGuid,&pName,&pFriendlyName,&pNtDeviceName,&pLocation);

	int index = pStorageDevice->PhysicalDeviceObjectCount;
	pStorageDevice->PhysicalDeviceObjectCount++;

	pStorageDevice->PhysicalDeviceObject = (FS_STORAGE_PHYSICAL_DEVICE_OBJECT**)_MemReAlloc( pStorageDevice->PhysicalDeviceObject, sizeof(FS_STORAGE_PHYSICAL_DEVICE_OBJECT*) * pStorageDevice->PhysicalDeviceObjectCount );
	pStorageDevice->PhysicalDeviceObject[index] = (FS_STORAGE_PHYSICAL_DEVICE_OBJECT*)_MemAlloc( sizeof(FS_STORAGE_PHYSICAL_DEVICE_OBJECT) );

	RtlZeroMemory(pStorageDevice->PhysicalDeviceObject[index]->DosDrive,sizeof(pStorageDevice->PhysicalDeviceObject[0]->DosDrive));

	WCHAR szWin32VolumeName[MAX_PATH];
	if( LookupVolumeGuidName(pNtDeviceName,szWin32VolumeName,_countof(szWin32VolumeName)) == STATUS_SUCCESS )
	{
		StringCchCat(szWin32VolumeName,_countof(szWin32VolumeName),L"\\");
		insertDosDrive( szWin32VolumeName );
	}

	pStorageDevice->PhysicalDeviceObject[index]->NtObjectName = _MemAllocString(pNtDeviceName);
	pStorageDevice->PhysicalDeviceObject[index]->VolumeName = _MemAllocString(szWin32VolumeName);

	_SafeMemFree( pClassGuid );
	_SafeMemFree( pName );
	_SafeMemFree( pFriendlyName );
	_SafeMemFree( pNtDeviceName );
	_SafeMemFree( pLocation );

	return;
}

static
BOOL
_MakeVolumeInfoList(
	FS_STORAGE_DEVICE_NAME_LIST **pReturnStorageDeviceNames
	)
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
	HTREEITEM hRoot = NULL;
    DWORD i;
	BOOL bRet = FALSE;

	hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_VOLUME,0,0,DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        return bRet;
    }

	ULONG cDiskDeviceCount = 0;
	FS_STORAGE_DEVICE_NAME_LIST *pStorageDeviceNames = NULL,*pNewAlloc = NULL;

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for(i = 0; SetupDiEnumDeviceInfo(hDevInfo,i,&DeviceInfoData); i++)
    {
        DWORD cbBufferSize = 0;

		TCHAR szDeviceInstanceId[4096];
		if( !SetupDiGetDeviceInstanceId(hDevInfo,&DeviceInfoData, szDeviceInstanceId, _countof(szDeviceInstanceId),&cbBufferSize) )
		{
			continue;
		}

		pNewAlloc = (FS_STORAGE_DEVICE_NAME_LIST *)_MemReAlloc(pStorageDeviceNames ,sizeof(FS_STORAGE_DEVICE_NAME_LIST) + sizeof(FS_STORAGE_DEVICE_NAME) * cDiskDeviceCount);

		if( pNewAlloc == NULL )
		{
			// error
			_MemFree( pStorageDeviceNames );
			break;
		}

		pStorageDeviceNames = pNewAlloc;

		RtlZeroMemory(&pStorageDeviceNames->StorageDevice[cDiskDeviceCount],sizeof(FS_STORAGE_DEVICE_NAME));

		// Get device information, friendly name and so on.
		//
		_GetDiskDriveInformation(&pStorageDeviceNames->StorageDevice[cDiskDeviceCount],szDeviceInstanceId);

		// Get relation device information. (assigned physical device object path)
		//
		LPTSTR pDevIds;
		LPTSTR p;
		ULONG Flag[] = {
//			CM_GETIDLIST_FILTER_REMOVALRELATIONS, // Relations for removable
//			CM_GETIDLIST_FILTER_BUSRELATIONS,     // Relations for bus
//			CM_GETIDLIST_FILTER_EJECTRELATIONS,   // Relations for eject
			CM_GETIDLIST_FILTER_POWERRELATIONS,   // Relations for power
		};
		int rt;
		for( rt = 0; rt < _countof(Flag); rt++)
		{
			if( _GetRelationInformation(NULL,szDeviceInstanceId,Flag[rt],&pDevIds) )
			{
				p = pDevIds;
				do
				{
					_GetVolumeInformation(p,&pStorageDeviceNames->StorageDevice[cDiskDeviceCount]);
					p += (wcslen(p) + 1);
				} while( *p != 0 );

				_MemFree( pDevIds );
			}
		}

		cDiskDeviceCount++;
	}

	if( pStorageDeviceNames != NULL )
	{
		pStorageDeviceNames->StorageDeviceCount = cDiskDeviceCount;
		*pReturnStorageDeviceNames = pStorageDeviceNames;
		bRet = TRUE;
	}

    SetupDiDestroyDeviceInfoList(hDevInfo);

	return bRet;
}

static
ULONG
_AppendStorageDeviceInfoList(
	const GUID *pGuid,
	FS_STORAGE_DEVICE_NAME_LIST **pReturnStorageDeviceNames,
	BOOL bCheckRelation
	)
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
	HTREEITEM hRoot = NULL;
    DWORD i;
	BOOL bRet = FALSE;

	hDevInfo = SetupDiGetClassDevs(pGuid,0,0,DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        return bRet;
    }

	ULONG cDiskDeviceCount = 0;
	FS_STORAGE_DEVICE_NAME_LIST *pStorageDeviceNames = NULL,*pNewAlloc = NULL;

	pStorageDeviceNames = *pReturnStorageDeviceNames;
	cDiskDeviceCount = pStorageDeviceNames->StorageDeviceCount;

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for(i = 0; SetupDiEnumDeviceInfo(hDevInfo,i,&DeviceInfoData); i++)
    {
        DWORD cbBufferSize = 0;

		TCHAR szDeviceInstanceId[4096];
		if( !SetupDiGetDeviceInstanceId(hDevInfo,&DeviceInfoData, szDeviceInstanceId, _countof(szDeviceInstanceId),&cbBufferSize) )
		{
			continue;
		}

		pNewAlloc = (FS_STORAGE_DEVICE_NAME_LIST *)_MemReAlloc(pStorageDeviceNames ,sizeof(FS_STORAGE_DEVICE_NAME_LIST) + sizeof(FS_STORAGE_DEVICE_NAME) * cDiskDeviceCount);

		if( pNewAlloc == NULL )
		{
			// error
			_MemFree( pStorageDeviceNames );
			break;
		}

		pStorageDeviceNames = pNewAlloc;

		RtlZeroMemory(&pStorageDeviceNames->StorageDevice[cDiskDeviceCount],sizeof(FS_STORAGE_DEVICE_NAME));

		// Get device information.
		//
		_GetDiskDriveInformation(&pStorageDeviceNames->StorageDevice[cDiskDeviceCount],szDeviceInstanceId);

		// Get new device property, later Windows Vista only.
		//
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
		if( LOBYTE(LOWORD(GetVersion())) >= 0x06 ) // (Ver >= 0x06)
		{
			DEVPROPTYPE dpt = 0;
			DWORD cb;
			SetupDiGetDeviceProperty(hDevInfo,&DeviceInfoData,&DEVPKEY_Device_InstallDate,&dpt,
					(PBYTE)&pStorageDeviceNames->StorageDevice[cDiskDeviceCount].ftInstallDate,sizeof(FILETIME),
					&cb,0);
		}
#endif
		// Get relation device information. (assigned physical device object path)
		// Required for GUID_DEVCLASS_DISKDRIVE.
		//
		if( bCheckRelation )
		{
			LPTSTR pDevIds;
			LPTSTR p;
			ULONG Flag[] = {
				CM_GETIDLIST_FILTER_REMOVALRELATIONS, // Relations for removable
				CM_GETIDLIST_FILTER_BUSRELATIONS,     // Relations for bus
				CM_GETIDLIST_FILTER_EJECTRELATIONS,   // Relations for eject
			};
			int rt;
			for( rt = 0; rt < _countof(Flag); rt++)
			{
				if( _GetRelationInformation(NULL,szDeviceInstanceId,Flag[rt],&pDevIds) )
				{
					p = pDevIds;
					do
					{
						_GetVolumeInformation(p,&pStorageDeviceNames->StorageDevice[cDiskDeviceCount]);
						p += (wcslen(p) + 1);
					} while( *p != 0 );

					_MemFree( pDevIds );
				}
			}
		}

		cDiskDeviceCount++;
	}

	if( pStorageDeviceNames != NULL )
	{
		pStorageDeviceNames->StorageDeviceCount = cDiskDeviceCount;
		*pReturnStorageDeviceNames = pStorageDeviceNames;
		bRet = TRUE;
	}

    SetupDiDestroyDeviceInfoList(hDevInfo);

	return cDiskDeviceCount;
}

static
PBYTE
_GetRegistryProperty(
	HDEVINFO hDevInfo,
	SP_DEVINFO_DATA* DevInfoData,
	DWORD Property,
	PULONG pulRegPropDataType,
	PULONG pcbData
	)
{
	DWORD PropertyRegDataType;
	DWORD dwErrorCode;
	DWORD cb;
	PBYTE pBuffer = NULL;

	do
	{
		SetupDiGetDeviceRegistryProperty(hDevInfo,DevInfoData,Property,NULL,NULL,0,&cb);

		if( GetLastError() != ERROR_INSUFFICIENT_BUFFER  )
		{
			return NULL; // invalid call or no data
		}

		pBuffer = new BYTE[ cb ];

		if( pBuffer == NULL )
		{
			return NULL;
		}

		if( SetupDiGetDeviceRegistryProperty(hDevInfo,DevInfoData,Property,
					&PropertyRegDataType,pBuffer,cb,&cb) )
		{
			dwErrorCode = ERROR_SUCCESS;
			if( pcbData )
				*pcbData = cb;
			if( pulRegPropDataType )
			{
				switch( PropertyRegDataType )
				{
					case REG_SZ:
					case REG_EXPAND_SZ:
						*pulRegPropDataType = DEVPROP_TYPE_STRING;
						break;
					case REG_MULTI_SZ:
						*pulRegPropDataType = DEVPROP_TYPE_STRING_LIST;
						break;
					case REG_DWORD:
						*pulRegPropDataType = DEVPROP_TYPE_UINT32;
						break;
					default:
						*pulRegPropDataType = 0xFFFFFFFF;
						break;
				}
			}
		}
		else
		{
			dwErrorCode = GetLastError();
			delete[] pBuffer;
			pBuffer = NULL;
		}
	}
	while( dwErrorCode == ERROR_INSUFFICIENT_BUFFER );

	SetLastError(dwErrorCode);

	return pBuffer;
}

static 
PBYTE
_GetPropertyData(
	HDEVINFO hDevInfo,
	SP_DEVINFO_DATA* DevInfoData,
	const DEVPROPKEY& DevPropKey,
	DWORD Property,
	PULONG pulPropertyType,
	PULONG pcbDataSize
	)
{

	if( Property != 0 )
	{
		return _GetRegistryProperty(hDevInfo,DevInfoData,Property,pulPropertyType,pcbDataSize);
	}

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
	DEVPROPTYPE PropertyType;
	DWORD dwErrorCode;
	DWORD cb;
	PBYTE pBuffer = NULL;
	do
	{
		SetupDiGetDeviceProperty(hDevInfo,DevInfoData,
                    &DevPropKey,&PropertyType,
                    NULL,0,&cb,0);

		if( GetLastError() != ERROR_INSUFFICIENT_BUFFER  )
		{
			return NULL; // invalid call or no data
		}

		pBuffer = new BYTE[ cb ];

		if( pBuffer == NULL )
		{
			return NULL;
		}

		if( SetupDiGetDeviceProperty(hDevInfo,DevInfoData,
				&DevPropKey,&PropertyType,
				pBuffer,cb,&cb,0) )
		{
			dwErrorCode = ERROR_SUCCESS;

			if( pcbDataSize )
				*pcbDataSize = cb;
			if( pulPropertyType )
				*pulPropertyType = PropertyType;
		}
		else
		{
			dwErrorCode = GetLastError();
			delete[] pBuffer;
			pBuffer = NULL;
		}
	}
	while( dwErrorCode == ERROR_INSUFFICIENT_BUFFER );

	SetLastError( dwErrorCode );

	return pBuffer;
#else
	SetLastError( ERROR_INVALID_PARAMETER );
	return NULL;
#endif
}

static
BOOL
_GetProperty(
	HDEVINFO hDevInfo,
	SP_DEVINFO_DATA* DevInfoData,
	const DEVPROPKEY& DevPropKey,
	ULONG DevRegPropKey,
	PVOID pData,
	ULONG RequirePropType
	)
{
	ULONG cbData;
	ULONG PropertyType;
	PBYTE pBuffer = _GetPropertyData(hDevInfo,DevInfoData,DevPropKey,DevRegPropKey,
							&PropertyType,&cbData);

	if( pBuffer == NULL )
	{
		return FALSE;
	}

	BOOL bSuccess = TRUE;

	if( PropertyType == RequirePropType )
	{
		switch( PropertyType )
		{
			case DEVPROP_TYPE_STRING:
			{
				PWSTR *pStr = (PWSTR *)pData;
				*pStr = (PWSTR)_MemAlloc(cbData);
				RtlCopyMemory(*pStr,pBuffer,cbData);
				break;
			}
			case DEVPROP_TYPE_STRING_LIST:
			{
				PWSTR *pStr = (PWSTR *)pData;
				*pStr = (PWSTR)_MemAlloc(cbData);
				RtlCopyMemory(*pStr,pBuffer,cbData);
				break;
			}
			case DEVPROP_TYPE_FILETIME:
			{
				FILETIME *pft = (FILETIME *)pData;
				*pft = *(FILETIME *)pBuffer;
				bSuccess = TRUE;
				break;
			}
			case DEVPROP_TYPE_UINT32:
			{
				*((PULONG)pData) = *((PULONG)pBuffer);
				break;
			}
			default:
			{
				bSuccess = FALSE;
				break;
			}
		}
	}
	else
	{
		bSuccess = FALSE;
	}

	_SafeMemFree(pBuffer);

	return bSuccess;
}

static int __cdecl _compare(const void *a, const void *b)   
{
	FS_HARDWARE_PRODUCT *set1 = *(FS_HARDWARE_PRODUCT **)a;
	FS_HARDWARE_PRODUCT *set2 = *(FS_HARDWARE_PRODUCT **)b;
	if( set1->FriendlyName == NULL && set2->FriendlyName != NULL )
		return -1;
	if( set1->FriendlyName != NULL && set2->FriendlyName == NULL )
		return 1;
	if( set1->FriendlyName == NULL && set2->FriendlyName == NULL )
		return 0;
	return _wcsicmp(set1->FriendlyName,set2->FriendlyName);
}

//----------------------------------------------------------------------------
//
//  GetKnownHardwareProducts()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
BOOL
WINAPI
GetKnownHardwareProducts(
	HANDLE *phProductList,
	const GUID *DevGuid,
	ULONG OptionFlags
	)
{
	BOOL bSuccess = FALSE;

	if( phProductList == NULL || DevGuid == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	HDPA hdpa = DPA_Create(64);
	ULONG c_hdpa = 0;

	if( hdpa == NULL )
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

    HDEVINFO hDevInfo;
    hDevInfo = SetupDiGetClassDevsEx(DevGuid,NULL,NULL,DIGCF_PROFILE,NULL,NULL,NULL);

    if( hDevInfo != INVALID_HANDLE_VALUE )
    {
        SP_DEVINFO_DATA DevInfoData = {0};
        DevInfoData.cbSize = sizeof(DevInfoData);

        DWORD dwIndex = 0;
        while( SetupDiEnumDeviceInfo(hDevInfo,dwIndex,&DevInfoData) )
        {
			FS_HARDWARE_PRODUCT *pItem = (FS_HARDWARE_PRODUCT *)_MemAllocZero(sizeof(FS_HARDWARE_PRODUCT));

			// InstanceId
			//
			// When could not acquire friendly name, instead get the Instance ID as a friendly name.
			// Possible cause is the target device deleted after call SetupDiEnumDeviceInfo().
			//
			if (!_GetProperty(hDevInfo,&DevInfoData,
								DEVPKEY_Device_InstanceId,0,
								&pItem->InstanceId,DEVPROP_TYPE_STRING))
			{
				TCHAR szDeviceInstanceId[4096];
				DWORD cb;
				if( SetupDiGetDeviceInstanceId(hDevInfo,&DevInfoData,szDeviceInstanceId,_countof(szDeviceInstanceId),&cb) )
				{
					pItem->InstanceId = _MemAllocString(szDeviceInstanceId);
				}
			}

			// HardwareIds
			//
			_GetProperty(hDevInfo,&DevInfoData,
								DEVPKEY_Device_HardwareIds,SPDRP_HARDWAREID,
								&pItem->HardwareIds,DEVPROP_TYPE_STRING_LIST);

			// Device Description
			//
			_GetProperty(hDevInfo,&DevInfoData,
								DEVPKEY_Device_DeviceDesc,SPDRP_DEVICEDESC,
								&pItem->DeviceDesc,DEVPROP_TYPE_STRING);

			// Friendly Name
			//
			_GetProperty(hDevInfo,&DevInfoData,
								DEVPKEY_Device_FriendlyName,SPDRP_FRIENDLYNAME,
								&pItem->FriendlyName,DEVPROP_TYPE_STRING);

			// PDO(Physical Device Object) Name
			//
			_GetProperty(hDevInfo,&DevInfoData,
								DEVPKEY_Device_PDOName,SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
								&pItem->PysicalDeviceObjectName,DEVPROP_TYPE_STRING);

			// Install date
			//
			_GetProperty(hDevInfo,&DevInfoData,
								DEVPKEY_Device_InstallDate,0,
								&pItem->InstallDate,DEVPROP_TYPE_FILETIME);

			// First Install date
			//
			_GetProperty(hDevInfo,&DevInfoData,
								DEVPKEY_Device_FirstInstallDate,0,
								&pItem->FirstInstallDate,DEVPROP_TYPE_FILETIME);

			// RemovalRelations
			//
			if( !_GetProperty(hDevInfo,&DevInfoData,
								DEVPKEY_Device_RemovalRelations,0,
								&pItem->RemovalRelations,DEVPROP_TYPE_STRING_LIST) )
			{
				pItem->RemovalRelations = _GetIdList_Relation(pItem->InstanceId,
													CM_GETIDLIST_FILTER_REMOVALRELATIONS);
				if( pItem->RemovalRelations == NULL )
					pItem->RemovalRelations = _GetIdList_Relation(pItem->InstanceId,
														CM_GETIDLIST_FILTER_BUSRELATIONS);
			}

			// DeviceNodeStatus
			//
			if( !_GetProperty(hDevInfo,&DevInfoData,
								DEVPKEY_Device_DevNodeStatus,0,
								&pItem->DevNodeStatus,DEVPROP_TYPE_UINT32) )
			{
				pItem->DevNodeStatus = INVALID_PROPERTY_ULONG_VALUE;
			}

			// InstallState
			//
			if( !_GetProperty(hDevInfo,&DevInfoData,
								DEVPKEY_Device_InstallState,SPDRP_INSTALL_STATE,
								&pItem->InstallState,DEVPROP_TYPE_UINT32) )
			{
				pItem->InstallState = INVALID_PROPERTY_ULONG_VALUE;
			}

			// DPA_InsertPtr(hdpa, DA_LAST, pitem)
			c_hdpa = DPA_AppendPtr(hdpa,pItem);

			dwIndex++;
		}

        SetupDiDestroyDeviceInfoList(hDevInfo);

		// sort by friendly name
	    //
		if( DPA_GetPtrCount(hdpa) > 0 )
			qsort(DPA_GetPtrPtr(hdpa),DPA_GetPtrCount(hdpa),sizeof(FS_HARDWARE_PRODUCT *),_compare);

		*phProductList = hdpa;

		SetLastError(ERROR_SUCCESS);
		bSuccess = TRUE;
	}
	else
	{
		DWORD dwError = GetLastError();

		DPA_Destroy(hdpa);
		*phProductList = NULL;

		SetLastError(dwError);
		bSuccess = FALSE;
	}

	return bSuccess;
}

//----------------------------------------------------------------------------
//
//  FreeKnownHardwareProducts()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
BOOL
WINAPI
FreeKnownHardwareProducts(
	HANDLE hProductList
	)
{
	HDPA hdpa = (HDPA)hProductList;

	int i,c;
	c = DPA_GetPtrCount(hdpa);
	for(i = 0; i < c; i++)
	{
		FS_HARDWARE_PRODUCT *pItem = (FS_HARDWARE_PRODUCT *)DPA_GetPtr(hdpa,i);

		if( pItem )
		{
			_SafeMemFree(pItem->RemovalRelations);
			_SafeMemFree(pItem->FriendlyName);
			_SafeMemFree(pItem->DeviceDesc);
			_SafeMemFree(pItem->HardwareIds);
			_SafeMemFree(pItem->InstanceId);
			_SafeMemFree(pItem->PysicalDeviceObjectName);

			_MemFree(pItem);
		}
	}

	DPA_Destroy(hdpa);

	return TRUE;
}

//----------------------------------------------------------------------------
//
//  GetKnownHardwareProductsCount()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
GetKnownHardwareProductsCount(
	HANDLE hProductList
	)
{
	HDPA hdpa = (HDPA)hProductList;
	return (ULONG)DPA_GetPtrCount(hdpa);
}

//----------------------------------------------------------------------------
//
//  GetKnownHardwareProductPointer()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
BOOL
WINAPI
GetKnownHardwareProductPointer(
	HANDLE hProductList,
	ULONG Index,
	const FS_HARDWARE_PRODUCT **pProductInfo
	)
{
	HDPA hdpa = (HDPA)hProductList;
	if( (int)Index < DPA_GetPtrCount(hdpa) )
	{
		*pProductInfo = (const FS_HARDWARE_PRODUCT *)DPA_GetPtr(hdpa,Index);
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//
//  FindVolumeObjectPath()
//
//  PURPOSE: Find the device object path (NT device path) by device id.
//
//----------------------------------------------------------------------------
EXTERN_C
LONG
WINAPI
FindVolumeObjectPath(
	PCWSTR pszDeviceInstanceId,
	PWSTR pszVolume,
	DWORD cchVolume
	)
{
    LONG Status;

    Status = ERROR_PATH_NOT_FOUND;
    *pszVolume = L'\0';

    HDEVINFO hDevInfo;
    hDevInfo = SetupDiGetClassDevsEx(&GUID_DEVCLASS_VOLUME,NULL,NULL,DIGCF_PROFILE,NULL,NULL,NULL);

    if( hDevInfo != INVALID_HANDLE_VALUE )
    {
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
        SP_DEVINFO_DATA DeviceInfoData = {0};
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if( SetupDiOpenDeviceInfo(hDevInfo,pszDeviceInstanceId,NULL,0,&DeviceInfoData) )
        {
            WCHAR szVolume[MAX_PATH];
            DEVPROPTYPE PropertyType;

            if( SetupDiGetDeviceProperty(hDevInfo,&DeviceInfoData,
	                &DEVPKEY_Device_PDOName,&PropertyType,
		            (PBYTE)szVolume,sizeof(szVolume),NULL,0) )
			{
                if( SUCCEEDED(StringCchCopy(pszVolume,cchVolume,szVolume)) )
	            {
		            Status = ERROR_SUCCESS;
			    }
			}
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
#else
		DEVNODE dn;
		DWORD dwFlags = CM_LOCATE_DEVNODE_NORMAL|CM_LOCATE_DEVNODE_PHANTOM;

		if( CM_Locate_DevNode_Ex(&dn,(PWSTR)pszDeviceInstanceId,dwFlags,m_hMachine) != 0 )
		{
			PWSTR pPDO = NULL;
			_GetDeviceLocationInfo(NULL,dn,NULL,NULL,NULL,&pPDO,NULL);
			if( pPDO != NULL )
			{
                if( SUCCEEDED(StringCchCopy(pszVolume,cchVolume,pPDO)) )
	            {
		            Status = ERROR_SUCCESS;
			    }
				_SafeMemFree( pPDO );
			}
		}
#endif
    }

    return Status;
}

//----------------------------------------------------------------------------
//
//  FindDeviceIdFromVolumeClass()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
BOOL
WINAPI
FindDeviceIdFromVolumeClass(
	PCWSTR pszVolumePath,
	PWSTR *ppszDeviceId,
	ULONG RelationDevice
	)
{
	BOOL bSuccess = FALSE;

    HDEVINFO hDevInfo;
    hDevInfo = SetupDiGetClassDevsEx(&GUID_DEVCLASS_VOLUME,NULL,NULL,DIGCF_PRESENT,NULL,NULL,NULL);

    if( hDevInfo != INVALID_HANDLE_VALUE )
    {
        SP_DEVINFO_DATA DevInfoData = {0};
        DevInfoData.cbSize = sizeof(DevInfoData);

        DWORD dwIndex = 0;
        while( SetupDiEnumDeviceInfo(hDevInfo,dwIndex,&DevInfoData) )
        {
			PWSTR pBuffer;

			pBuffer = (PWSTR)_GetRegistryProperty(hDevInfo,&DevInfoData,SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,NULL,NULL);

			if( pBuffer )
			{
				if( _wcsicmp(pszVolumePath,pBuffer) == 0 )
				{
					_MemFree(pBuffer);

					DWORD cchBufferSize;
					PWSTR pszDeviceId;

					SetupDiGetDeviceInstanceId(hDevInfo,&DevInfoData,NULL,0,&cchBufferSize);

					pszDeviceId = (PWSTR)_MemAlloc(((cchBufferSize+1) * sizeof(WCHAR)));

					if( SetupDiGetDeviceInstanceId(hDevInfo,&DevInfoData,pszDeviceId,cchBufferSize,&cchBufferSize) )
					{
						if( RelationDevice == 0 )
						{
							*ppszDeviceId = StrDup(pszDeviceId);
							_SafeMemFree(pszDeviceId);
							bSuccess = TRUE;
							break;
						}
						else
						{
							LPWSTR pDevIds = NULL;
							_GetRelationInformation(NULL,pszDeviceId,CM_GETIDLIST_FILTER_POWERRELATIONS,&pDevIds);
							_MemFree(pszDeviceId);
							if( pDevIds )
							{
								*ppszDeviceId = StrDup(pDevIds);
								bSuccess = TRUE;
								break;
							}
						}
					}
					else
					{
						_SafeMemFree(pszDeviceId);
					}
				}
			}

			dwIndex++;
		}

        SetupDiDestroyDeviceInfoList(hDevInfo);
	}

	return bSuccess;
}

//----------------------------------------------------------------------------
//
//  CreateDeviceLocationList()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
CreateDeviceLocationList(
	FS_STORAGE_DEVICE_NAME_LIST **ppStorageDeviceNames
	)
{
	if( ppStorageDeviceNames == NULL )
		return E_INVALIDARG;

	*ppStorageDeviceNames = NULL;

	int cInitialAlloc = 1;
	FS_STORAGE_DEVICE_NAME_LIST *pNewAlloc = (FS_STORAGE_DEVICE_NAME_LIST *)_MemAllocZero(sizeof(FS_STORAGE_DEVICE_NAME_LIST) + sizeof(FS_STORAGE_DEVICE_NAME) * cInitialAlloc);
	if( pNewAlloc == NULL )
	{
		SetLastError( ERROR_NOT_ENOUGH_MEMORY );
		return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}
	pNewAlloc->StorageDeviceCount = 0;
	*ppStorageDeviceNames = pNewAlloc;

	_AppendStorageDeviceInfoList(&GUID_DEVCLASS_DISKDRIVE,ppStorageDeviceNames,TRUE);
	_AppendStorageDeviceInfoList(&GUID_DEVCLASS_CDROM,ppStorageDeviceNames,FALSE);
	_AppendStorageDeviceInfoList(&GUID_DEVCLASS_FLOPPYDISK,ppStorageDeviceNames,FALSE);
//	_AppendStorageDeviceInfoList(&GUID_DEVCLASS_VOLUME,ppStorageDeviceNames,FALSE);

	SetLastError( NO_ERROR );

	return S_OK;
}

//----------------------------------------------------------------------------
//
//  FreeDeviceLocationList()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
BOOL
WINAPI
FreeDeviceLocationList(
	FS_STORAGE_DEVICE_NAME_LIST *pStorageDeviceNames
	)
{
	if( pStorageDeviceNames == NULL )
		return FALSE;
	
	ULONG i,j;
	for(i = 0; i < pStorageDeviceNames->StorageDeviceCount; i++)
	{
		_SafeMemFree(pStorageDeviceNames->StorageDevice[i].FriendlyName);
		_SafeMemFree(pStorageDeviceNames->StorageDevice[i].Location);
		_SafeMemFree(pStorageDeviceNames->StorageDevice[i].NtDeviceName);
		_SafeMemFree(pStorageDeviceNames->StorageDevice[i].DeviceId);

		for(j = 0; j < pStorageDeviceNames->StorageDevice[i].PhysicalDeviceObjectCount; j++)
		{
			_SafeMemFree( pStorageDeviceNames->StorageDevice[i].PhysicalDeviceObject[j]->NtObjectName );
			_SafeMemFree( pStorageDeviceNames->StorageDevice[i].PhysicalDeviceObject[j]->VolumeName );
			_SafeMemFree( pStorageDeviceNames->StorageDevice[i].PhysicalDeviceObject[j] );
		}

		_SafeMemFree( pStorageDeviceNames->StorageDevice[i].PhysicalDeviceObject );
	}

	_SafeMemFree( pStorageDeviceNames );
	return TRUE;
}
