//***************************************************************************
//*                                                                         *
//*  clusterreader.cpp                                                      *
//*                                                                         *
//*  Volume Cluster Reader                                                  *
//*                                                                         *
//*  Create: 2015-08-31                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "fsfilelib.h"
#include <ntddvol.h>

static
BOOL
_GetPhysicalLocation(
	HANDLE hVolume,
	LONGLONG LcnOffset,
	VOLUME_PHYSICAL_OFFSETS **pResult
	)
{
	//
	// Lcn to physical cluster number
	//
	VOLUME_LOGICAL_OFFSET logicalOffset = {0};
	logicalOffset.LogicalOffset = LcnOffset;

	VOLUME_PHYSICAL_OFFSETS *pPhysicalDriveOffsets;
	DWORD cbPhysicalDriveOffsets = sizeof(VOLUME_PHYSICAL_OFFSETS);
	DWORD cbBytesReturned = 0;

	for(;;)
	{
		pPhysicalDriveOffsets = (VOLUME_PHYSICAL_OFFSETS *)_MemAllocZero(cbPhysicalDriveOffsets);
		if( pPhysicalDriveOffsets == NULL )
		{
			SetLastError( ERROR_NOT_ENOUGH_MEMORY );
			return FALSE;
		}

		if( !DeviceIoControl(
				hVolume,
				IOCTL_VOLUME_LOGICAL_TO_PHYSICAL,
				&logicalOffset,
				sizeof(VOLUME_LOGICAL_OFFSET),
				pPhysicalDriveOffsets,
				cbPhysicalDriveOffsets,
				&cbBytesReturned,
				NULL) )
		{
			if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
			{
				_MemFree(pPhysicalDriveOffsets);
				cbPhysicalDriveOffsets = cbBytesReturned;
				continue;
			}

			_SafeMemFree(pPhysicalDriveOffsets);
			*pResult = NULL;

			return FALSE;
		}

		break;
	}

	*pResult = pPhysicalDriveOffsets;

	return TRUE;
}

static
BOOL
_GetClustersInformation(
	HANDLE hVolume,
	HANDLE hFile,
	DWORD dwFlags,
	DWORD dwBytesPerCluster,
	DWORD dwBytesPerSector,
	FS_CLUSTER_INFORMATION **ppClusters
	)
{
	FS_CLUSTER_INFORMATION *pClusters = NULL;
    STARTING_VCN_INPUT_BUFFER inputVcn = {0};
    RETRIEVAL_POINTERS_BUFFER rpBuf = {0};
    DWORD dwErrorCode = NO_ERROR;
    BOOL bSuccess = false; 
	ULONG i;
	DWORD dwBytesReturned;
	RETRIEVAL_POINTER_BASE rpb = {0};

	dwBytesReturned = 0;

	if( !DeviceIoControl(hVolume,
						FSCTL_GET_RETRIEVAL_POINTER_BASE,
						NULL,0,
						&rpb,sizeof(rpb),
						&dwBytesReturned,
						NULL) )
	{
		rpb.FileAreaOffset.QuadPart = 0;  // invalid value / unavailable retrieval pointer base
	}

	inputVcn.StartingVcn.QuadPart = 0; // start at the beginning 

	do
	{ 
		dwBytesReturned = 0;

        bSuccess = DeviceIoControl(hFile,
			            FSCTL_GET_RETRIEVAL_POINTERS,
						&inputVcn,sizeof(STARTING_VCN_INPUT_BUFFER),
						&rpBuf,sizeof(RETRIEVAL_POINTERS_BUFFER),
			            &dwBytesReturned,
						NULL); 

		if( bSuccess )
	        dwErrorCode = NO_ERROR;
		else
	        dwErrorCode = GetLastError();

        switch (dwErrorCode)
		{
	        case ERROR_HANDLE_EOF:
	            bSuccess = true;
		        break;
	        case ERROR_MORE_DATA:
	            inputVcn.StartingVcn = rpBuf.Extents[0].NextVcn;
				// through, falls down.
	        case NO_ERROR:
			{
				if( rpBuf.ExtentCount > 0 )
				{
					if( pClusters == NULL )
					{
						pClusters = (FS_CLUSTER_INFORMATION *)_MemAllocZero( sizeof(FS_CLUSTER_INFORMATION) + ((rpBuf.ExtentCount - 1) * sizeof(FS_CLUSTER_INFORMATION)) );

						// copy cluster heap base secotor offset
						pClusters->ClusterHeapBase = rpb;
					}
					else
					{
						FS_CLUSTER_INFORMATION *pTemp;
						ULONG c;
						c = pClusters->ExtentCount + rpBuf.ExtentCount;
						pTemp = (FS_CLUSTER_INFORMATION *)_MemReAlloc(pClusters,sizeof(FS_CLUSTER_INFORMATION) + ((c-1) * sizeof(FS_CLUSTER_INFORMATION)));
						if( pTemp != NULL )
							pClusters = pTemp;
						else
							_SafeMemFree( pClusters );
					}

					if( pClusters == NULL )
					{
						dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
						break;
					}

					for(i = 0; i < rpBuf.ExtentCount; i++)
					{
						FS_CLUSTER_LOCATION *pLoc = &pClusters->Extents[pClusters->ExtentCount + i];

						pLoc->Vcn.QuadPart = rpBuf.StartingVcn.QuadPart;
						pLoc->Lcn.QuadPart = rpBuf.Extents[i].Lcn.QuadPart;
						pLoc->Count.QuadPart = rpBuf.Extents[i].NextVcn.QuadPart - rpBuf.StartingVcn.QuadPart;
					} 

					if( dwFlags )
					{
						for(i = 0; i < rpBuf.ExtentCount; i++)
						{
							if( rpBuf.Extents[i].Lcn.QuadPart != ULLONG_MAX )
							{
								LONGLONG LcnOffset = 0;

								LcnOffset = rpBuf.Extents[i].Lcn.QuadPart * dwBytesPerCluster;
								LcnOffset += (rpb.FileAreaOffset.QuadPart * dwBytesPerSector);

								if( !_GetPhysicalLocation(hVolume,
										LcnOffset,
										(VOLUME_PHYSICAL_OFFSETS **)&pClusters->Extents[pClusters->ExtentCount+i].PhysicalOffsets))
								{
									dwErrorCode = GetLastError();
									bSuccess = false;
									break;
								}
							}
							else
							{
								pClusters->Extents[pClusters->ExtentCount+i].PhysicalOffsets = 0;
							}
						} 
					}

					pClusters->ExtentCount += rpBuf.ExtentCount;

					*ppClusters = pClusters;

					bSuccess = true;
				}
				else
				{
					*ppClusters = NULL;
				}

	            break; 
			}
			default:
				break;
		} 
	}
	while (dwErrorCode == ERROR_MORE_DATA);

    return bSuccess;
}

EXTERN_C
LONG
WINAPI
ReadFileClusterInformaion_U( 
	HANDLE /*hVolume*/,  // unuse reserved
	HANDLE hFile, 
	UNICODE_STRING *VolumeRootDirectoryName,
	FS_CLUSTER_INFORMATION_CLASS Class,
	PVOID Data,
	ULONG cbData
	)
{
	UNICODE_STRING usVolumeRoot;
	HANDLE hVolume;
	HANDLE hVolumeRoot;
	NTSTATUS Status;

	DWORD dwBytesPerCluster = 0;
	DWORD dwBytesPerSector = 0;

	SplitRootRelativePath(VolumeRootDirectoryName->Buffer,&usVolumeRoot,NULL);

#ifdef _DEBUG
	PWSTR psz = AllocateSzFromUnicodeString(&usVolumeRoot);
	FreeMemory(psz);
#endif

	if( (Status = OpenFile_U(&hVolumeRoot,NULL,&usVolumeRoot,
					FILE_READ_ATTRIBUTES,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					0)) != STATUS_SUCCESS )
	{
		//
		// could not open the volume, try open use the path.
		//
		usVolumeRoot = *VolumeRootDirectoryName;

		Status = OpenFile_U(&hVolumeRoot,NULL,&usVolumeRoot,
					FILE_READ_ATTRIBUTES,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					0);
	}

	if( Status == STATUS_SUCCESS )
	{
		VOLUME_FS_SIZE_INFORMATION *SizeInfo;
		if( GetVolumeFsInformation(hVolumeRoot,VOLFS_SIZE_INFORMATION,(void **)&SizeInfo) == STATUS_SUCCESS )
		{
			dwBytesPerCluster = SizeInfo->SectorsPerAllocationUnit * SizeInfo->BytesPerSector;
			dwBytesPerSector  = SizeInfo->BytesPerSector;
			FreeMemory(SizeInfo);
		}
		CloseHandle(hVolumeRoot);
	}

	//
	// Open volume
	//
	if( hFile != NULL )
	{
		hVolume = NULL;

		FS_CLUSTER_INFORMATION *pClusters = NULL;

		UNICODE_STRING usVolumeDevice;
		usVolumeDevice = usVolumeRoot;
		GetVolumeName_U(&usVolumeDevice);

		for(ULONG fOption = FILE_NON_DIRECTORY_FILE;;)
		{
			if( IsUserAnAdmin() )
			{
				// Cluster read
				Status = OpenFile_U(&hVolume,NULL,&usVolumeDevice,
							GENERIC_READ|SYNCHRONIZE,
							FILE_SHARE_READ|FILE_SHARE_WRITE,
							fOption|FILE_SYNCHRONOUS_IO_ALERT|FILE_NO_INTERMEDIATE_BUFFERING);
			}
			else
			{
				// Get Lcn only
				Status = OpenFile_U(&hVolume,NULL,&usVolumeDevice,
							FILE_READ_ATTRIBUTES|SYNCHRONIZE,
							FILE_SHARE_READ|FILE_SHARE_WRITE,
							fOption|FILE_SYNCHRONOUS_IO_ALERT|FILE_NO_INTERMEDIATE_BUFFERING);
			}

			if( Status == STATUS_FILE_IS_A_DIRECTORY )
			{
				fOption &= ~FILE_NON_DIRECTORY_FILE;
				continue;
			}

			break;
		}

		if( Status == 0 )
		{
			if(!_GetClustersInformation(
							hVolume, 
							hFile, 
							(Class == ClusterInformationAll) ? 0x1 : 0x0,
							dwBytesPerCluster,
							dwBytesPerSector,
							&pClusters
							))
			{
				Status = GetLastError();
			}
			else
			{
				if( pClusters == NULL )
					Status = ERROR_NO_MORE_ITEMS;
			}
		}

		if( hVolume != NULL )
			CloseHandle(hVolume);

		if( pClusters != NULL )
		{
			if( Class == ClusterInformationBasic )
			{
				((FS_CLUSTER_INFORMATION_BASIC *)Data)->FirstLcn = pClusters->Extents[0].Lcn;
				((FS_CLUSTER_INFORMATION_BASIC *)Data)->FirstCount = pClusters[0].ExtentCount;
				((FS_CLUSTER_INFORMATION_BASIC *)Data)->Split = pClusters->ExtentCount;
				((FS_CLUSTER_INFORMATION_BASIC *)Data)->SectorsPerCluster = dwBytesPerCluster;
				((FS_CLUSTER_INFORMATION_BASIC *)Data)->BytesPerSector = dwBytesPerSector;
				_MemFree( pClusters );
			}
			else
			{
				pClusters->BytesPerCluster = dwBytesPerCluster;
				pClusters->BytesPerSector = dwBytesPerSector;
				*((FS_CLUSTER_INFORMATION **)Data) = pClusters;
			}
			Status = ERROR_SUCCESS;
		}
	}

	return Status;
}

EXTERN_C
LONG
WINAPI
ReadFileClusterInformaion( 
	HANDLE hVolume, // unuse reserved
	HANDLE hFile, 
	PCWSTR pszVolumeRootDirectoryName,
	FS_CLUSTER_INFORMATION_CLASS Class,
	PVOID Data,
	ULONG cbData
	)
{
	UNICODE_STRING usVolumeRootDirectoryName;
	RtlInitUnicodeString(&usVolumeRootDirectoryName,pszVolumeRootDirectoryName);

	return ReadFileClusterInformaion_U(hVolume,hFile,&usVolumeRootDirectoryName,
			Class,Data,cbData);
}

EXTERN_C
LONG
WINAPI
FreeClusterInformation( 
	PVOID Buffer
	)
{
	FS_CLUSTER_INFORMATION *pClusters = (FS_CLUSTER_INFORMATION *)Buffer;

	ULONG i;

	for(i = 0; i < pClusters->ExtentCount; i++)
	{
		if( pClusters->Extents[i].PhysicalOffsets )
		{
			_MemFree( pClusters->Extents[i].PhysicalOffsets );
		}
	}

	_MemFree( pClusters );

	return 0;
}
