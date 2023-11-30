#pragma once

#include "..\libntwdk\ntvolumenames.h"

#include "dsarray.h"

class CDiskFindVolume
{
	VOLUME_NAME_STRING_ARRAY *pVolumeNames;

	typedef struct _VOLUME_EXTENT_INFO
	{
		VOLUME_NAME_STRING *pVolume;
		VOLUME_DISK_EXTENTS *pDiskExtents;
	} VOLUME_EXTENT_INFO;

	DSArray<VOLUME_EXTENT_INFO> aVolNames;

public:
	CDiskFindVolume()
	{
		pVolumeNames = NULL;
		aVolNames.Create(8);
	}

	~CDiskFindVolume()
	{
		Free();
		aVolNames.Destroy();
	}

	void Enum()
	{
		if( EnumVolumeNames(&pVolumeNames) == 0 )
		{
			Make();
		}
	}

	void Free()
	{
		INT i;
		INT c = aVolNames.GetCount();
		for(i = 0; i < c; i++)
		{
			VOLUME_EXTENT_INFO vi;
			aVolNames.GetItem(i,&vi);

			_SafeMemFree(vi.pDiskExtents);
		}
		aVolNames.DeleteAll();

		if( pVolumeNames != NULL )
		{
			FreeVolumeNames(pVolumeNames);
			pVolumeNames = NULL;
		}
	}

	BOOLEAN Find(DWORD DiskNumber,LARGE_INTEGER& StartingOffset,LARGE_INTEGER ExtentLength,PCWSTR& pszVolumeName)
	{
		INT i;
		INT c = aVolNames.GetCount();
		for(i = 0; i < c; i++)
		{
			VOLUME_EXTENT_INFO vi;
			aVolNames.GetItem(i,&vi);

			DWORD dw;
			for(dw = 0; dw < vi.pDiskExtents->NumberOfDiskExtents; dw++)
			{
				DISK_EXTENT& disk = vi.pDiskExtents->Extents[dw];
			    if( disk.DiskNumber == DiskNumber &&
					disk.StartingOffset.QuadPart == StartingOffset.QuadPart &&
					disk.ExtentLength.QuadPart == ExtentLength.QuadPart )
				{
					pszVolumeName = vi.pVolume->NtVolumeName;
					return TRUE;
				}
			}
		}
		return FALSE;
	}

private:
	void Make()
	{
		DWORD i;
		for(i = 0; i < pVolumeNames->Count; i++)
		{
			VOLUME_EXTENT_INFO vi;
			vi.pVolume = &pVolumeNames->Volume[i];
			vi.pDiskExtents = GetVolumeDiskExtents(vi.pVolume->NtVolumeName);
			if( vi.pDiskExtents != NULL )
				aVolNames.Add( &vi );
		}
	}

	PVOLUME_DISK_EXTENTS GetVolumeDiskExtents(PCWSTR pszVolumeName)
	{
        UNICODE_STRING usVolumeName;
		PVOID pOutBuffer = NULL;
		DWORD cbOutBuffer = sizeof(VOLUME_DISK_EXTENTS);
		NTSTATUS Status;
		HANDLE hVolumeDevice;

		RtlInitUnicodeString(&usVolumeName,pszVolumeName);

		ULONG DesiredAccess = (IsUserAnAdmin() ? FILE_READ_DATA : 0)|STANDARD_RIGHTS_READ|FILE_READ_ATTRIBUTES|SYNCHRONIZE;

		Status = OpenFile_U(&hVolumeDevice,NULL,&usVolumeName,
						DesiredAccess,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						FILE_OPEN_FOR_BACKUP_INTENT|FILE_SYNCHRONOUS_IO_NONALERT);

		if( Status == STATUS_SUCCESS )
		{
			DWORD cb = 0;
			BOOL bRet;

			for(;;)
			{
				pOutBuffer = _MemAllocZero( cbOutBuffer );
				if( pOutBuffer == NULL )
					break;

				bRet = DeviceIoControl(hVolumeDevice,
								IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
								NULL,0,
								pOutBuffer,cbOutBuffer,
								&cb,NULL);

				DWORD dwError = GetLastError();
				if( !bRet && (dwError == ERROR_INSUFFICIENT_BUFFER || dwError == ERROR_MORE_DATA) )
				{
					_SafeMemFree( pOutBuffer );

					cbOutBuffer += 4096;

					pOutBuffer = _MemAllocZero( cbOutBuffer );
					if( pOutBuffer )
						continue;
				}
				break;
			}

			CloseHandle(hVolumeDevice);
        }

		return (PVOLUME_DISK_EXTENTS)pOutBuffer;
	}
};
