//***************************************************************************
//*                                                                         *
//*  disk.cpp                                                               *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2024-02-16                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "volumehelp.h"

//----------------------------------------------------------------------------
//
//  QueryDiskPerformance()
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
QueryDiskPerformance(
	PCWSTR pszDeviceName,
	DISK_PERFORMANCE *DiskPerf,
	INT cbDiskPerf
	)
{
	HRESULT hr;
	WCHAR szDeviceName[MAX_PATH];

	StringCchCopy(szDeviceName,MAX_PATH,pszDeviceName);

	HANDLE hDisk = CreateFile(szDeviceName,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);

	if( hDisk != INVALID_HANDLE_VALUE )
	{
		DWORD cbBytesReturned;

		if( DiskPerf != NULL && cbDiskPerf == sizeof(DISK_PERFORMANCE) )
		{
			DISK_PERFORMANCE dp = {0};
			DWORD cbOutBufferSize = cbDiskPerf;
			LPVOID lpOutBuffer = (LPVOID)DiskPerf;

			if( DeviceIoControl(hDisk,IOCTL_DISK_PERFORMANCE,NULL,0,lpOutBuffer,cbOutBufferSize,&cbBytesReturned,NULL) )
			{
				hr = S_OK;
			}
			else
			{
				ZeroMemory(DiskPerf,sizeof(DISK_PERFORMANCE));
				hr = HRESULT_FROM_WIN32( GetLastError() );
			}
		}

		CloseHandle(hDisk);
	}
	else
	{
		hr = HRESULT_FROM_WIN32( GetLastError() );
	}

	return hr;
}

//----------------------------------------------------------------------------
//
//  StopDiskPerformance()
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
StopDiskPerformance(
	PCWSTR pszDeviceName
	)
{
	HRESULT hr;
	WCHAR szDeviceName[MAX_PATH];

	StringCchCopy(szDeviceName,MAX_PATH,pszDeviceName);

	HANDLE hDisk = CreateFile(szDeviceName,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);

	if( hDisk != INVALID_HANDLE_VALUE )
	{
		DWORD cbBytesReturned;

		if( DeviceIoControl(hDisk,IOCTL_DISK_PERFORMANCE_OFF,NULL,0,NULL,0,&cbBytesReturned,NULL) )
		{
			hr = S_OK;
		}
		else
		{
			hr = HRESULT_FROM_WIN32( GetLastError() );
		}

		CloseHandle(hDisk);
	}
	else
	{
		hr = HRESULT_FROM_WIN32( GetLastError() );
	}

	return hr;
}

//----------------------------------------------------------------------------
//
//  StopDiskPerformanceAll()
//
//----------------------------------------------------------------------------
static HRESULT StopPhysicalDrives(ULONG Flags)
{
	HRESULT hr;
	WCHAR szVolumeName[MAX_PATH];
	int Failed = 0;

	PHYSICALDRIVE_NAME_STRING_ARRAY *PhysicalDriveNames;
	EnumPhysicalDriveNames( &PhysicalDriveNames );

	for(ULONG index = 0; index < PhysicalDriveNames->Count; index++)
	{
		StringCchPrintf(szVolumeName,MAX_PATH,L"\\\\?\\%s",PhysicalDriveNames->Drive[index].PhysicalDriveName);

		hr = StopDiskPerformance(szVolumeName);

		if( hr != S_OK )
		{
			Failed++;
			if( (Flags & SDPF_NO_BREAK_ON_ERROR) != 0 )
				break;
		}
	}

	FreePhysicalDriveNames(PhysicalDriveNames);

	if( (Flags & SDPF_NO_BREAK_ON_ERROR) && (Failed != 0) )
		hr = S_FALSE;

	return hr;
}

static HRESULT StopDiskVolumes(ULONG Flags)
{
	HRESULT hr;
	WCHAR szVolumeName[MAX_PATH];
	int Failed = 0;

	VOLUME_NAME_STRING_ARRAY *VolumeNames = NULL;
	EnumVolumeNames( &VolumeNames );

	for(ULONG index = 0; index < VolumeNames->Count; index++)
	{
		ULONG t = 0;		
		GetDeviceTypeByVolumeName(VolumeNames->Volume[index].NtVolumeName,&t,NULL);

		StringCchPrintf(szVolumeName,MAX_PATH,L"\\\\?\\GlobalRoot%s",VolumeNames->Volume[index].NtVolumeName);

		hr = StopDiskPerformance(szVolumeName);

		if( hr != S_OK )
		{
			Failed++;
			if( (Flags & SDPF_NO_BREAK_ON_ERROR) != 0 )
				break;
		}
	}

	FreeVolumeNames( VolumeNames );

	if( (Flags & SDPF_NO_BREAK_ON_ERROR) && (Failed != 0) )
		hr = S_FALSE;

	return hr;
}

EXTERN_C
HRESULT
WINAPI
StopDiskPerformanceAll(
	ULONG Flags
	)
{
	HRESULT hr;

	if( Flags & SDPF_STOP_PHYSICAL_DRIVES )
	{
		hr = StopPhysicalDrives(Flags);
	}
	else
	{
		hr = StopDiskVolumes(Flags);
	}

	return hr;
}
