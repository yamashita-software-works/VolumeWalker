#pragma once
//
// physicaldriveinformationclass.h
//
// 2024-02-09
//
// note:
// This file is toplevel header file to be include for Win32 source code. 
// Ported code from fsworkbench's dll.
//

#ifdef __cplusplus
//
// Physical Drive Information Data Class
//
struct CPhysicalDriveInformation
{
    DRIVE_LAYOUT_INFORMATION_EX *pDriveLayout;    // Process Heap
    PSTORAGE_DEVICE_DESCRIPTOR pDeviceDescriptor; // LocalAlloc
    DISK_GEOMETRY_EX *pGeometry;                  // LocalAlloc
    STORAGE_READ_CAPACITY DeviceCapacity; 
    STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR Alignment;
    DWORD m_dwDiskNumber;

	STGCTL::STORAGE_ADAPTER_DESCRIPTOR_EX AdapterDescriptor;

    // Reference pointer (need not free memory)
    PDISK_DETECTION_INFO Detection;
    PDISK_PARTITION_INFO Partition;

    HANDLE m_hDisk;

	DWORD m_dwDriveLayoutStatus;

    CPhysicalDriveInformation()
    {
        memset(this,0,sizeof(CPhysicalDriveInformation));
        m_hDisk = NULL;
        pDriveLayout = NULL;
        pDeviceDescriptor = NULL;
        pGeometry = NULL;
        m_dwDiskNumber = (DWORD)-1;
		m_dwDriveLayoutStatus = 0;
    }

    ~CPhysicalDriveInformation()
    {
        if( pDriveLayout )
            Free(pDriveLayout);
		_SafeMemFree(pDeviceDescriptor);
		_SafeMemFree(pGeometry);
        CloseDiskHandle();
    }

    VOID CloseDiskHandle()
    {
        if( m_hDisk )
        {
            CloseHandle(m_hDisk);
            m_hDisk = NULL;
        }
    }

    // memory helper
    //
    PVOID Alloc(ULONG cb)
    {
		return _MemAlloc(cb);
//      return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,cb);
    }

    PVOID Shrink(PVOID pv,ULONG cb)
    {
		return _MemReAlloc(pv,cb);
//      return HeapReAlloc(GetProcessHeap(),0, pv, cb);
    }

    VOID Free(PVOID pv)
    {
		_MemFree(pv);
//		HeapFree(GetProcessHeap(),0,pv);
    }

    // Physical Disk APIs
    //
    HRESULT OpenDisk(PCWSTR pszDeviceName,DWORD dwDiskNumber=0,DWORD dwDesired=0)
    {
        if( m_hDisk != NULL )
            CloseDiskHandle();

        m_dwDiskNumber = dwDiskNumber;

        m_hDisk = ::OpenDisk(pszDeviceName,dwDiskNumber,dwDesired);

        return m_hDisk != INVALID_HANDLE_VALUE ? S_OK : HRESULT_FROM_WIN32( GetLastError() );
    }

    HRESULT GetGeometry(HANDLE hDisk=NULL)
    {
        ULONG cb;

        if( hDisk == NULL )
            hDisk = m_hDisk;

        GetDiskDriveGeometryEx(hDisk,&pGeometry,&cb);

        ULONG cbBasic = sizeof(DISK_GEOMETRY) + sizeof(LARGE_INTEGER);
        if( cb > cbBasic )
        {
            Detection = DiskGeometryGetDetect( pGeometry );
            Partition = DiskGeometryGetPartition( pGeometry );
        }

        return S_OK;
    }

    HRESULT GetDriveLayout(HANDLE hDisk=NULL,DWORD numDiskCount=512)
    {
        HRESULT hr = E_FAIL;

        if( hDisk == NULL )
            hDisk = m_hDisk;

        DWORD dwOutBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * numDiskCount;
        pDriveLayout = (DRIVE_LAYOUT_INFORMATION_EX*)Alloc(dwOutBufferSize);

        if( pDriveLayout )
        {
            DWORD BytesReturned;;
            if( DeviceIoControl(hDisk,
                          IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                          NULL,0,
                          pDriveLayout,dwOutBufferSize,
                          &BytesReturned,
                          NULL) )
            {
				DWORD dwError = GetLastError();

                Shrink(pDriveLayout,BytesReturned);

				if( dwError == ERROR_SUCCESS )
	                hr = S_OK;
				else if( dwError == ERROR_NOT_READY )
		            hr = HRESULT_FROM_WIN32(dwError); // Not ready drive
				else
		            hr = HRESULT_FROM_WIN32(dwError);
            }
        }
		else
		{
			hr = E_OUTOFMEMORY;
		}

		m_dwDriveLayoutStatus = HRESULT_CODE(hr);

        return hr;
    }

    HRESULT GetDeviceIdDescriptor(HANDLE hDisk=NULL)
    {
        if( hDisk == NULL )
            hDisk = m_hDisk;

        HRESULT hr = E_FAIL;
        if( StorageGetDeviceIdDescriptor(hDisk,&pDeviceDescriptor) == NO_ERROR )
        {
            hr = S_OK;
        }
        return hr;
    }

    HRESULT GetDetectSectorSize(HANDLE hDisk=NULL)
    {
        if( hDisk == NULL )
            hDisk = m_hDisk;

        DWORD dwError;

        if( (dwError = StorageDetectSectorSize(hDisk,&Alignment)) != NO_ERROR )
        {
            Alignment.Version = (DWORD)-1;
            Alignment.Size = (DWORD)dwError;
            return HRESULT_FROM_WIN32(dwError);
        }

        return S_OK;
    }

	HRESULT GetAdapterDescriptor(HANDLE hDisk=NULL)
	{
        if( hDisk == NULL )
            hDisk = m_hDisk;

		DWORD dwError;

		dwError = StorageAdapterDescriptor(hDisk,&AdapterDescriptor,sizeof(AdapterDescriptor));

		if( dwError != ERROR_SUCCESS )
		{
            AdapterDescriptor.Version = (DWORD)-1;
            AdapterDescriptor.Size = (DWORD)dwError;
            return HRESULT_FROM_WIN32(dwError);
		}

		return S_OK;
	}

#ifdef _DEBUG
    HRESULT DoTest(HANDLE hDisk=NULL)
    {
        if( hDisk == NULL )
            hDisk = m_hDisk;
		return S_OK;
	}
#endif

    BOOL IsValidAlignment()
    {
        return (Alignment.Version != (DWORD)-1 && Alignment.Version != (DWORD)0);
    }
};
#endif
