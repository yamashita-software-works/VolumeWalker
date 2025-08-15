//***************************************************************************
//*                                                                         *
//*  repoarsepointinfo.cpp                                                  *
//*                                                                         *
//*  NT native reperse point, junction point functions                      *
//*                                                                         *
//*  Create: 2025-01-11                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include <ntifs.h>
#include <windef.h>
#include <ntstatus.h>
#include <winerror.h>
#include <strsafe.h>
#include <malloc.h>
#include "libntwdk.h"
#include "ntnativeapi.h"
#include "ntobjecthelp.h"
#include "wfswof.h"
#include "fsfilelib.h"
#include "..\libntwdk\ntnativeapi.h"

// Win32 kernel32.dll compatible
EXTERN_C
BOOL NTAPI DeviceIoControl(
  __in        HANDLE hDevice,
  __in        DWORD dwIoControlCode,
  __in_opt    LPVOID lpInBuffer,
  __in        DWORD nInBufferSize,
  __out_opt   LPVOID lpOutBuffer,
  __in        DWORD nOutBufferSize,
  __out_opt   LPDWORD lpBytesReturned,
  __inout_opt LPVOID lpOverlapped
);

//----------------------------------------------------------------------------
//
//  GetReparsePointInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------

#define _REPARSE_DATA_BUFFER_LENGTH  (sizeof(REPARSE_DATA_BUFFER) + _NT_PATH_FULL_LENGTH_BYTES)

EXTERN_C
BOOL
NTAPI
GetReparsePointInformation(
	HANDLE hRoot,
	PCWSTR FilePath,
	ULONG InformationClass,
	PVOID InformationBuffer,
	ULONG InformationBufferLength
	)
{
	BOOL bSuccess = FALSE;
	HANDLE hFile;

	if( FilePath != NULL )
	{
		NTSTATUS Status;
		UNICODE_STRING ustrPath;
		RtlInitUnicodeString(&ustrPath,FilePath);
		Status = OpenFile_U(&hFile,hRoot,&ustrPath,
					FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,
					FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT);
		if( Status != STATUS_SUCCESS )
		{
			RtlSetLastWin32Error( RtlNtStatusToDosError(Status) );
			return FALSE;
		}
	}
	else if( hRoot != NULL && FilePath == NULL )
	{
		hFile = hRoot;
	}
	else
	{
		RtlSetLastWin32Error( RtlNtStatusToDosError(STATUS_INVALID_PARAMETER) );
		return FALSE;
	}

	ULONG cbDataLength = _REPARSE_DATA_BUFFER_LENGTH;
	REPARSE_DATA_BUFFER *pBuffer = (REPARSE_DATA_BUFFER *)AllocMemory( cbDataLength );

	if( pBuffer != NULL && hFile != INVALID_HANDLE_VALUE )
	{
		DWORD cb;
        bSuccess = DeviceIoControl(hFile,
						FSCTL_GET_REPARSE_POINT,
						NULL,0,
						pBuffer,cbDataLength,
						&cb,NULL);
	}

	if( FilePath != NULL )
		NtClose(hFile);

	if( pBuffer == NULL )
	{
		_SetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
		bSuccess = FALSE;
	}

	if( bSuccess )
	{
		FS_REPARSE_POINT_INFORMATION *pInfo = (FS_REPARSE_POINT_INFORMATION *)InformationBuffer;
		WCHAR *pName;
		ULONG Result = 0;
		PWSTR pszTargetPath = NULL;
		ULONG cchTargetPath = 0;
		PWSTR pszPrintPath = NULL;
		ULONG cchPrintPath = 0;
		ULONG cchNameLength;

		if( InformationClass == FsReparsePointTargetPath )
		{
			pszTargetPath = (PWSTR)InformationBuffer;
			cchTargetPath = WCHAR_LENGTH(InformationBufferLength);
		}
		else if( InformationClass == FsReparsePointPrintPath )
		{
			pszPrintPath = (PWSTR)InformationBuffer;
			cchPrintPath = WCHAR_LENGTH(InformationBufferLength);
		}
		else
		{
			pszTargetPath = pInfo->TargetPath;
			cchTargetPath = pInfo->TargetPathLength;

			pszPrintPath = pInfo->PrintPath;
			cchPrintPath = pInfo->PrintPathLength;
		}

		switch( pBuffer->ReparseTag )
		{
			case IO_REPARSE_TAG_SYMLINK:
			{
				if( InformationClass == FsReparsePointDetail )
				{
					pInfo->ReparseTag = pBuffer->ReparseTag;
					pInfo->Flags = pBuffer->SymbolicLinkReparseBuffer.Flags;
				}

				// Get target path
				if( pszTargetPath )
				{
					cchNameLength = WCHAR_CHARS( pBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength );
					if( cchTargetPath > cchNameLength  )
					{
						pName = (pBuffer->SymbolicLinkReparseBuffer.PathBuffer + (pBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset/sizeof(WCHAR)));
	
						memcpy(pszTargetPath,pName,cchNameLength*sizeof(WCHAR));
						pszTargetPath[cchNameLength] = L'\0';
					}
					else
					{
						_SetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
						bSuccess = FALSE;
					}
				}

				// Get print path
				if( pszPrintPath )
				{
					cchNameLength = WCHAR_CHARS( pBuffer->SymbolicLinkReparseBuffer.PrintNameLength );
					if( cchPrintPath > cchNameLength  )
					{
						pName = (pBuffer->SymbolicLinkReparseBuffer.PathBuffer + (pBuffer->SymbolicLinkReparseBuffer.PrintNameOffset/sizeof(WCHAR)));
	
						memcpy(pszPrintPath,pName,cchNameLength*sizeof(WCHAR));
						pszPrintPath[cchNameLength] = L'\0';
					}
					else
					{
						_SetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
						bSuccess = FALSE;
					}
				}
				break;
			}
			case IO_REPARSE_TAG_MOUNT_POINT:
			{
				if( InformationClass == FsReparsePointDetail )
				{
					pInfo->ReparseTag = pBuffer->ReparseTag;
				}

				cchNameLength = pBuffer->MountPointReparseBuffer.SubstituteNameLength/sizeof(WCHAR);
				if( cchTargetPath > cchNameLength  )
				{
					pName = (pBuffer->MountPointReparseBuffer.PathBuffer + (pBuffer->MountPointReparseBuffer.SubstituteNameOffset/sizeof(WCHAR)));

					memcpy(pszTargetPath,pName,cchNameLength*sizeof(WCHAR));
					pszTargetPath[cchNameLength] = L'\0';
				}
				else
				{
					_SetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
					bSuccess = FALSE;
				}

				// Get print path
				if( pszPrintPath )
				{
					cchNameLength = WCHAR_CHARS( pBuffer->SymbolicLinkReparseBuffer.PrintNameLength );
					if( cchPrintPath > cchNameLength  )
					{
						pName = (pBuffer->MountPointReparseBuffer.PathBuffer + (pBuffer->SymbolicLinkReparseBuffer.PrintNameOffset/sizeof(WCHAR)));
	
						memcpy(pszPrintPath,pName,cchNameLength*sizeof(WCHAR));
						pszPrintPath[cchNameLength] = L'\0';
					}
					else
					{
						_SetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
						bSuccess = FALSE;
					}
				}
				break;
			}
			case IO_REPARSE_TAG_APPEXECLINK:
			{
				pInfo->ReparseTag = pBuffer->ReparseTag;

				APPEXECLINK_READ_BUFFER *pal = (APPEXECLINK_READ_BUFFER *)pBuffer;

				cchNameLength = pal->ReparseDataLength;///sizeof(WCHAR);
				if( cchTargetPath > cchNameLength  )
				{
					WCHAR *p = pal->StringList;

					// 0:"Package ID"
					// 1:"Entry Point"
					// 2:"Executable"
					// 3:"Application Type" Integer as ASCII. "0" = Desktop bridge application; Else sandboxed UWP application
					int iIndex = 0;
					while( *p )
					{
						if( iIndex == 2 )
						{
							wcsncpy(pszTargetPath,p,cchTargetPath);
						}
						iIndex++;
						p += (wcslen(p)+1);
					}
				}
				else
				{
					_SetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
					bSuccess = FALSE;
				}
				break;
			}
			case IO_REPARSE_TAG_HSM:
			case IO_REPARSE_TAG_HSM2:
			case IO_REPARSE_TAG_SIS:
			case IO_REPARSE_TAG_WIM:
			case IO_REPARSE_TAG_CSV:
			case IO_REPARSE_TAG_DFS:
			case IO_REPARSE_TAG_DFSR:
			case IO_REPARSE_TAG_DEDUP:
			case IO_REPARSE_TAG_NFS:
			case IO_REPARSE_TAG_FILE_PLACEHOLDER:
			case IO_REPARSE_TAG_WOF:
			case IO_REPARSE_TAG_WCI:
			case IO_REPARSE_TAG_WCI_1:
			case IO_REPARSE_TAG_GLOBAL_REPARSE:
			case IO_REPARSE_TAG_CLOUD:
			case IO_REPARSE_TAG_CLOUD_1:
			case IO_REPARSE_TAG_CLOUD_2:
			case IO_REPARSE_TAG_CLOUD_3:
			case IO_REPARSE_TAG_CLOUD_4:
			case IO_REPARSE_TAG_CLOUD_5:
			case IO_REPARSE_TAG_CLOUD_6:
			case IO_REPARSE_TAG_CLOUD_7:
			case IO_REPARSE_TAG_CLOUD_8:
			case IO_REPARSE_TAG_CLOUD_9:
			case IO_REPARSE_TAG_CLOUD_A:
			case IO_REPARSE_TAG_CLOUD_B:
			case IO_REPARSE_TAG_CLOUD_C:
			case IO_REPARSE_TAG_CLOUD_D:
			case IO_REPARSE_TAG_CLOUD_E:
			case IO_REPARSE_TAG_CLOUD_F:
			case IO_REPARSE_TAG_CLOUD_MASK:
			case IO_REPARSE_TAG_PROJFS:
			case IO_REPARSE_TAG_STORAGE_SYNC:
			case IO_REPARSE_TAG_WCI_TOMBSTONE:
			case IO_REPARSE_TAG_UNHANDLED:
			case IO_REPARSE_TAG_ONEDRIVE:
			case IO_REPARSE_TAG_PROJFS_TOMBSTONE:
			case IO_REPARSE_TAG_AF_UNIX:
			case IO_REPARSE_TAG_WCI_LINK:
			case IO_REPARSE_TAG_WCI_LINK_1:
				pInfo->ReparseTag = pBuffer->ReparseTag;
				_SetLastWin32Error( ERROR_SUCCESS );
				*pszTargetPath = 0;
				bSuccess = TRUE;
				break;

			default:
				_SetLastWin32Error( ERROR_INVALID_REPARSE_DATA );
				*pszTargetPath = 0;
				bSuccess = FALSE;
				break;
		}
	}

	FreeMemory(pBuffer);
	pBuffer = NULL;

	return bSuccess;
}

//----------------------------------------------------------------------------
//
//  SetReparsePointInformation()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
SetReparsePointInformation(
	HANDLE hFile,
	PCWSTR FilePath,
	PCWSTR PrintFilePath,
	ULONG Flags
	)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus = {0};
	ULONG cbFilePath = 0;
	ULONG cbBuffer;

	cbFilePath += ((ULONG)wcslen(FilePath) * sizeof(WCHAR));
	cbFilePath += ((ULONG)wcslen(PrintFilePath) * sizeof(WCHAR));

	cbBuffer = sizeof(REPARSE_DATA_BUFFER) + cbFilePath;

	REPARSE_DATA_BUFFER *p = (REPARSE_DATA_BUFFER *)AllocMemory( cbBuffer );
	if( p == NULL )
		return STATUS_NO_MEMORY;

	p->ReparseTag = IO_REPARSE_TAG_SYMLINK;

	// Size, in bytes, of the reparse data in the DataBuffer member.
	p->ReparseDataLength = (USHORT)(sizeof(p->SymbolicLinkReparseBuffer) + cbFilePath);

	// Offset, in bytes, of the substitute name string in the PathBuffer array. 
	// Note that this offset must be divided by sizeof(WCHAR) to get the array index. 
	p->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;

	// Length, in bytes, of the substitute name string. If this string is NULL-terminated, 
	// SubstituteNameLength does not include space for the UNICODE_NULL character. 
	p->SymbolicLinkReparseBuffer.SubstituteNameLength = (USHORT)(wcslen(FilePath) * sizeof(WCHAR));

	// Offset, in bytes, of the print name string in the PathBuffer array. 
	// Note that this offset must be divided by sizeof(WCHAR) to get the array index. 
	p->SymbolicLinkReparseBuffer.PrintNameOffset = p->SymbolicLinkReparseBuffer.SubstituteNameLength;

	// Length, in bytes, of the print name string. If this string is NULL-terminated, 
	// PrintNameLength does not include space for the UNICODE_NULL character. 
	p->SymbolicLinkReparseBuffer.PrintNameLength = (USHORT)(wcslen(PrintFilePath) * sizeof(WCHAR));

	p->SymbolicLinkReparseBuffer.Flags = Flags; // absolute/relative  symbolic link.

	RtlCopyMemory(
		&p->SymbolicLinkReparseBuffer.PathBuffer[ p->SymbolicLinkReparseBuffer.SubstituteNameOffset/sizeof(WCHAR) ],
		FilePath, p->SymbolicLinkReparseBuffer.SubstituteNameLength);

	RtlCopyMemory(
		&p->SymbolicLinkReparseBuffer.PathBuffer[ (p->SymbolicLinkReparseBuffer.PrintNameOffset/sizeof(WCHAR)) ],
		PrintFilePath, p->SymbolicLinkReparseBuffer.PrintNameLength);

	Status = NtFsControlFile(hFile,NULL,NULL,NULL,&IoStatus,FSCTL_SET_REPARSE_POINT,p,cbBuffer,	NULL,0);

	FreeMemory(p);

	return Status;
}

//----------------------------------------------------------------------------
//
//  SetJunctionPointInformation()
//
//----------------------------------------------------------------------------
#define REPARSE_MOUNTPOINT_HEADER_SIZE   8

#pragma pack(4)
typedef struct _REPARSE_MOUNTPOINT_DATA_BUFFER {
	ULONG  ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	struct {
		USHORT SubstituteNameOffset;
		USHORT SubstituteNameLength;
		USHORT PrintNameOffset;
		USHORT PrintNameLength;
		WCHAR  PathBuffer[1];
	} MountPoint;
} REPARSE_MOUNTPOINT_DATA_BUFFER;
#pragma pack()

EXTERN_C
NTSTATUS
NTAPI
SetJunctionPointInformation(
	HANDLE hDirectory,
	PCWSTR TargetPath,
	PCWSTR TargetPrintName,
	ULONG /*Flags*/
	)
{
	ULONG cchTarget = (ULONG)wcslen(TargetPath);
	ULONG cchPrintName = (ULONG)wcslen(TargetPrintName);

	USHORT cbTarget = (USHORT)(cchTarget * sizeof(WCHAR));
	USHORT cbPrintName = (USHORT)(cchPrintName * sizeof(WCHAR));
	
	USHORT cbStructSize = sizeof(REPARSE_MOUNTPOINT_DATA_BUFFER); // must be 20 bytes. 
	USHORT cbDataHeaderLength = sizeof(REPARSE_MOUNTPOINT_DATA_BUFFER) - sizeof(ULONG) - sizeof(USHORT) - sizeof(USHORT); // must be 12 bytes. 

	ULONG cbBuffer = cbStructSize + cbTarget +  cbPrintName;

	REPARSE_MOUNTPOINT_DATA_BUFFER *ReparseBuffer;
	ReparseBuffer = (REPARSE_MOUNTPOINT_DATA_BUFFER *)AllocMemory( cbBuffer );
	if( ReparseBuffer == NULL )
		return STATUS_NO_MEMORY;

	ReparseBuffer->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
	ReparseBuffer->ReparseDataLength = cbDataHeaderLength + cbTarget + cbPrintName;

	ReparseBuffer->MountPoint.SubstituteNameOffset = 0;
	ReparseBuffer->MountPoint.SubstituteNameLength = cbTarget;
	ReparseBuffer->MountPoint.PrintNameOffset = cbTarget + sizeof(WCHAR);
	ReparseBuffer->MountPoint.PrintNameLength = cbPrintName;

	memcpy(&ReparseBuffer->MountPoint.PathBuffer[0],TargetPath,cbTarget);
	memcpy(&ReparseBuffer->MountPoint.PathBuffer[cchTarget+1],TargetPrintName,cbPrintName);

	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;

	Status = NtFsControlFile(hDirectory,
					NULL,
					NULL,
					NULL,
					&IoStatusBlock,
					FSCTL_SET_REPARSE_POINT,
					ReparseBuffer,cbBuffer,
					NULL,0);

	FreeMemory(ReparseBuffer);

	return Status;
}

//----------------------------------------------------------------------------
//
//  GetReparsePointType()
//
//----------------------------------------------------------------------------
EXTERN_C
LONG
NTAPI
GetReparsePointType(
	HANDLE hFile,
	ULONG *pulReparsePointType
	)
{
	NTSTATUS Status = STATUS_SUCCESS;

	BYTE *pBuffer = (BYTE *)AllocMemory( MAXIMUM_REPARSE_DATA_BUFFER_SIZE );
	if( pBuffer )
	{
		BOOL bSuccess;
		DWORD cb;

        bSuccess = DeviceIoControl(hFile,
						FSCTL_GET_REPARSE_POINT,
						NULL,0,
						pBuffer,MAXIMUM_REPARSE_DATA_BUFFER_SIZE,
						&cb,NULL);

		if( bSuccess )
		{
			*pulReparsePointType = *((PULONG)pBuffer);
		}
	}
	else
	{
		_SetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
	}

	return RtlGetLastWin32Error();
}

//----------------------------------------------------------------------------
//
//  GetAppLinkInformation()
//
//----------------------------------------------------------------------------
EXTERN_C
BOOL
NTAPI
GetAppLinkInformation(
	HANDLE hRoot,
	PCWSTR FilePath,
	PWSTR pszPath,
	ULONG cchPath,
	PWSTR pszPackageID,
	ULONG cchPackageID,
	PWSTR pszEntryPoint,
	ULONG cchEntryPoint,
	PWSTR pszApplicationType,
	ULONG cchApplicationType
	)
{
	BOOL bSuccess = FALSE;
	HANDLE hFile;

	if( FilePath != NULL )
	{
		NTSTATUS Status;
		UNICODE_STRING ustrPath;
		RtlInitUnicodeString(&ustrPath,FilePath);
		Status = OpenFile_U(&hFile,hRoot,&ustrPath,
					FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,
					FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT);
		if( Status != STATUS_SUCCESS )
		{
			_SetLastWin32Error( RtlNtStatusToDosError(Status) );
			return FALSE;
		}
	}
	else if( hRoot != NULL && FilePath == NULL )
	{
		hFile = hRoot;
	}
	else
	{
		_SetLastWin32Error( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	ULONG cbDataLength = _REPARSE_DATA_BUFFER_LENGTH;
	REPARSE_DATA_BUFFER *pBuffer = (REPARSE_DATA_BUFFER *)AllocMemory( cbDataLength );

	if( pBuffer != NULL && hFile != INVALID_HANDLE_VALUE )
	{
		DWORD cb;
        bSuccess = DeviceIoControl(hFile,
						FSCTL_GET_REPARSE_POINT,
						NULL,0,
						pBuffer,cbDataLength,
						&cb,NULL);
	}

	if( FilePath != NULL && hFile != INVALID_HANDLE_VALUE )
	{
		NtClose(hFile);
	}

	if( pBuffer == NULL )
	{
		_SetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
		bSuccess = FALSE;
	}

	if( bSuccess )
	{
		if( pBuffer->ReparseTag == IO_REPARSE_TAG_APPEXECLINK )
		{
			if( pszPackageID || pszEntryPoint || pszPath || pszApplicationType )
			{
				APPEXECLINK_READ_BUFFER *pal = (APPEXECLINK_READ_BUFFER *)pBuffer;

				WCHAR *psz = pal->StringList;

				// 0:"Package ID"
				// 1:"Entry Point"
				// 2:"Executable"
				// 3:"Application Type" Integer as ASCII. "0" = Desktop bridge application; Else sandboxed UWP application
				int iIndex = 0;
				while( *psz )
				{
					switch( iIndex )
					{
						case 0:
							if( pszPackageID )
								wcscpy_s(pszPackageID,cchPackageID,psz);
							break;
						case 1:
							if( pszEntryPoint )
								wcscpy_s(pszEntryPoint,cchEntryPoint,psz);
							break;
						case 2:
							if( pszPath )
								wcscpy_s(pszPath,cchPath,psz);
							break;
						case 3:
							if( pszApplicationType )
								wcscpy_s(pszApplicationType,cchApplicationType,psz);
							break;
					}
					iIndex++;
					psz += (wcslen(psz) + 1);
				}
			}
		}
		else
		{
			_SetLastWin32Error( ERROR_REPARSE_TAG_MISMATCH );
			bSuccess = FALSE;
		}
	}

	return bSuccess;
}

//----------------------------------------------------------------------------
//
//  IsAppLinkFile()
//
//----------------------------------------------------------------------------
EXTERN_C
BOOL
NTAPI
IsAppLinkFile(
	HANDLE hRoot,
	PCWSTR FilePath,
	PWSTR pszPath,
	ULONG cchPath
	)
{
	return GetAppLinkInformation(hRoot,FilePath,pszPath,cchPath,NULL,0,NULL,0,NULL,0);
}
