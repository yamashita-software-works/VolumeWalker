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
	StringCchPrintf(szDeviceName,MAX_PATH,L"\\\\?\\GlobalRoot%s",pszDeviceName);

	HANDLE hDisk = CreateFile(szDeviceName,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);

	if( hDisk != INVALID_HANDLE_VALUE )
	{
		DWORD cbBytesReturned;

		if( DiskPerf != NULL && cbDiskPerf == sizeof(DISK_PERFORMANCE) ) {
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
		else
		{
			if( DeviceIoControl(hDisk,IOCTL_DISK_PERFORMANCE_OFF,NULL,0,NULL,0,&cbBytesReturned,NULL) )
			{
				hr = S_OK;
			}
			else
			{
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
