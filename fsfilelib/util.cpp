//***************************************************************************
//*                                                                         *
//*  util.cpp                                                               *
//*                                                                         *
//*  Win32 API functions                                                    *
//*                                                                         *
//*  Author:  YAMASHITA Katsuhiro                                           *
//*                                                                         *
//*  History: 2024-12-07 Created.                                           * 
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "fsfilelib.h"

//----------------------------------------------------------------------------
//
//  _InternalBackupCopyFileImpl()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
/*++
typedef struct _WIN32_STREAM_ID {
  DWORD         dwStreamId;
  DWORD         dwStreamAttributes;
  LARGE_INTEGER Size;
  DWORD         dwStreamNameSize;
  WCHAR         cStreamName[ ANYSIZE_ARRAY ]; <- To subtracting this member size, if it is 4-byte aligned which need to subtract -4 instead of -2.
} WIN32_STREAM_ID, *LPWIN32_STREAM_ID;

#define WIN32_STREAM_ID_LENGTH  (sizeof(WIN32_STREAM_ID)-sizeof(WCHAR))
--*/
#define WIN32_STREAM_ID_LENGTH  (20) // DWORD+DWORD+LARGE_INTEGER+DWORD

typedef struct _FSOP
{
	LARGE_INTEGER StreamBytesTransferred;
	LARGE_INTEGER TotalBytesTransferred;

	ULONG StreamTypeId;
	LARGE_INTEGER StreamSize;

	PWSTR AltStreamName;
	ULONG AltStreamNameLength;

	ULONG ErrorReason;
	BOOL StreamSwitch;

	struct {
		BOOLEAN Abort;
		BOOLEAN FlushFileBuffers;
		BOOLEAN MoveFileContains;
		BOOLEAN CopyCreationDateTime;
		BOOLEAN CopyACL;
	};

	PWSTR SourcePath;
	PWSTR DestinationPath;
	HANDLE hSourceFile;
	HANDLE hDestinationFile;

} FSOP;

#define FO_ER_SOURCE       0x1
#define FO_ER_DESTINATION  0x2
#define FO_ER_READ         0x4
#define FO_ER_WRITE        0x8
#define FO_ER_USER         0x10

inline void _CopyCallback( FSOP *pfsop )
{
	return; // todo: _Notify(pfsop,0,pfsop->SourcePath,NULL);
}

NTSTATUS
NTAPI
_InternalBackupCopyFileImpl(
	FSOP* pfsop,
	HANDLE hDstFile,
	HANDLE hSrcFile
	)
{
	LONG Status = STATUS_UNSUCCESSFUL;
	BOOL bResult;
#if 0
	ULONG BufferLength = 4096;
#else
	ULONG BufferLength = 4096*16;
#endif
	int  cbReadBytes;
	int  cbWriteBytes;
	PBYTE pBuffer;
	PVOID ContextRead = NULL;
	PVOID ContextWrite = NULL;
	WIN32_STREAM_ID *pStreamData = NULL;
	WIN32_STREAM_ID *pStreamID = NULL;
	__int64 cbStreamBytes;
	DWORD cbStreamNameLength;
	DWORD rwBytes;
	DWORD dwStreamId;
	BOOL bProcessSecurity = (BOOL)pfsop->CopyACL;

	pBuffer = (PBYTE)AllocMemory( BufferLength );
	if( pBuffer == NULL )
	{
		return STATUS_NO_MEMORY;
	}

	__try
	{
		pfsop->StreamBytesTransferred.QuadPart = 0;
		pfsop->TotalBytesTransferred.QuadPart = 0;

		while( TRUE )
		{
			memset(pBuffer,0x0,BufferLength);

			//
			// Parse and copy WIN32_STREAM_ID
			//
			bResult = BackupRead(hSrcFile,pBuffer,WIN32_STREAM_ID_LENGTH,(DWORD*)&cbReadBytes,FALSE,bProcessSecurity,&ContextRead);

			if( bResult &&  cbReadBytes == 0) 
			{ 
				// Exit
				Status = STATUS_SUCCESS;
				break;
			}

			if( !bResult || (WIN32_STREAM_ID_LENGTH != cbReadBytes) )
			{
				pfsop->ErrorReason = (FO_ER_SOURCE|FO_ER_READ);
				Status = NTSTATUS_FROM_WIN32( GetLastError() );
				break;
			}

			//
			// Write WIN32_STREAM_ID
			//
			bResult = BackupWrite(hDstFile,pBuffer,cbReadBytes,(DWORD*)&cbWriteBytes,FALSE,bProcessSecurity,&ContextWrite);

			if( !bResult || (cbReadBytes != cbWriteBytes) )
			{
				pfsop->ErrorReason = (FO_ER_DESTINATION|FO_ER_WRITE);
				Status = NTSTATUS_FROM_WIN32( GetLastError() );
				break;
			}

			//
			// Save WIN32_STREAM_ID contents.
			//
			pStreamID = (WIN32_STREAM_ID *)pBuffer;

			dwStreamId         = pStreamID->dwStreamId;
			cbStreamBytes      = pStreamID->Size.QuadPart;
			cbStreamNameLength = pStreamID->dwStreamNameSize;

			//
			// Setup notify callback information.
			//
			pfsop->StreamSwitch = TRUE;
			pfsop->StreamTypeId = pStreamID->dwStreamId;

			pfsop->StreamSize.QuadPart = cbStreamBytes; // A stream size.
			pfsop->StreamBytesTransferred.QuadPart = cbWriteBytes; // Reset as stream start.
			pfsop->TotalBytesTransferred.QuadPart += cbWriteBytes; // Total stream copied.

			//
			// If stream has name then copy stream name.
			//
			if( cbStreamNameLength != 0 )
			{
				bResult = BackupRead(hSrcFile,pBuffer,cbStreamNameLength,(DWORD*)&cbReadBytes,FALSE,bProcessSecurity,&ContextRead);
				if( !bResult || (cbStreamNameLength != cbReadBytes) )
				{
					pfsop->ErrorReason = (FO_ER_SOURCE|FO_ER_READ);
					Status = NTSTATUS_FROM_WIN32( GetLastError() );
					break;
				}

				bResult = BackupWrite(hDstFile,pBuffer,cbReadBytes,(DWORD*)&cbWriteBytes,FALSE,bProcessSecurity,&ContextWrite);
				if( !bResult || (cbReadBytes != cbWriteBytes) )
				{
					pfsop->ErrorReason = (FO_ER_DESTINATION|FO_ER_WRITE);
					Status = NTSTATUS_FROM_WIN32( GetLastError() );
					break;
				}
				pfsop->AltStreamName = (PWSTR)pBuffer;
				pfsop->AltStreamNameLength = cbStreamNameLength;
			}
			else
			{
				pfsop->AltStreamName = NULL;
				pfsop->AltStreamNameLength = 0;
			}

			pfsop->StreamBytesTransferred.QuadPart += cbWriteBytes;
			pfsop->TotalBytesTransferred.QuadPart += cbWriteBytes;
			_CopyCallback( pfsop );
			if( pfsop->Abort )
			{
				pfsop->ErrorReason = (FO_ER_USER);
				Status = NTSTATUS_FROM_WIN32( ERROR_CANCELLED );
				__leave;
			}

			//
			// Copy data stream.
			//
			if( cbStreamBytes != 0 )
			{
				int rw,rwCount;

				rwCount = (int)(cbStreamBytes / BufferLength);
				if( cbStreamBytes % BufferLength )
					rwCount++;
	
				for( rw = 0; rw < rwCount; rw++ )
				{
					if( rw == (rwCount-1) )
					{
						rwBytes = (DWORD)(cbStreamBytes % BufferLength);
						if( rwBytes == 0 )
							rwBytes = (DWORD)BufferLength;
					}
					else
					{
						rwBytes = (DWORD)BufferLength;
					}

					bResult = BackupRead(hSrcFile,pBuffer,rwBytes,(DWORD*)&cbReadBytes,FALSE,bProcessSecurity,&ContextRead);
					if( !bResult || (rwBytes != cbReadBytes) )
					{
						pfsop->ErrorReason = (FO_ER_SOURCE|FO_ER_READ);
						Status = NTSTATUS_FROM_WIN32( GetLastError() );
						__leave;
					}

					if( BACKUP_OBJECT_ID != dwStreamId )
					{
						bResult = BackupWrite(hDstFile,pBuffer,cbReadBytes,(DWORD*)&cbWriteBytes,FALSE,bProcessSecurity,&ContextWrite);
						if( !bResult || (cbReadBytes != cbWriteBytes) )
						{
							pfsop->ErrorReason = (FO_ER_DESTINATION|FO_ER_WRITE);
							Status = NTSTATUS_FROM_WIN32( GetLastError() );
							__leave;
						}
					}
					else
					{
						//
						// If stream id is BACKUP_OBJECT_ID, not write to destination file.
						//
						cbWriteBytes = cbReadBytes;
						bResult = TRUE;
					}

					// Notification
					pfsop->StreamSwitch = FALSE;
					pfsop->AltStreamName = NULL;
					pfsop->AltStreamNameLength = NULL;
					pfsop->StreamBytesTransferred.QuadPart += cbWriteBytes;
					pfsop->TotalBytesTransferred.QuadPart += cbWriteBytes;
					_CopyCallback( pfsop );
					if( pfsop->Abort )
					{
						pfsop->ErrorReason = (FO_ER_USER);
						Status = NTSTATUS_FROM_WIN32( ERROR_CANCELLED );
						__leave;
					}
				}
			}
		}
	}
	__finally
	{
		BackupRead(hSrcFile,NULL,0,NULL,TRUE,FALSE,&ContextRead);
		BackupWrite(hDstFile,NULL,0,NULL,TRUE,FALSE,&ContextWrite);

		if( pfsop->FlushFileBuffers )
			FlushFileBuffers(hDstFile);

		FreeMemory(pBuffer);
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  _InternalBackupCopy()
//
//  PURPOSE: Copy file using Win32 Backup API
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
_InternalBackupCopy(
	FSOP* pfsop,
	HANDLE hDstFile,
	PCWSTR pDstFilePath,
	HANDLE hSrcFile,
	PCWSTR pSrcFilePath,
	BOOLEAN bDirectory
	)
{
	LONG Status = 0;

	FSOP fsop = {0};

	pfsop = &fsop;

	pfsop->SourcePath = (PWSTR)pSrcFilePath;
	pfsop->DestinationPath = (PWSTR)pDstFilePath;
	pfsop->hSourceFile = hSrcFile;
	pfsop->hDestinationFile = hDstFile;
	pfsop->TotalBytesTransferred.QuadPart = 0;

	Status = _InternalBackupCopyFileImpl(
							pfsop,
							hDstFile,
							hSrcFile
							);

	return Status;
}

//----------------------------------------------------------------------------
//
//  _InternalCopyAttributes()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
_InternalCopyAttributes(
	__in HANDLE hDstFile,
	__in HANDLE hSrcFile,
	__in_opt PVOID pFileAttributeInfo, // Point to NT_FILE_BASIC_INFORMATION
	__in BOOL fAttributesOnly,
	__in BOOL fCopyCreationDateTime
	)
{
	NT_FILE_BASIC_INFORMATION fbi;
	LONG Status;

	if( pFileAttributeInfo == NULL )
	{
		Status = QueryFileBasicInformation(hSrcFile,&fbi);
		if( Status != STATUS_SUCCESS )
			return Status;

		if( fAttributesOnly )
		{
			// Set to -1 to indicate no change.
			// However, the meaning is different in XP UDFs, so this case is not supported.
			fbi.LastWriteTime.QuadPart  = -1;
			fbi.CreationTime.QuadPart   = -1;
			fbi.LastAccessTime.QuadPart = -1;
			fbi.ChangeTime.QuadPart     = -1;
		}
		else
		{
			if( fCopyCreationDateTime == false )
			{
				// If CopyCreationDateTime==true, all attributes will be copied.
				// If false, only fbi.LastWriteTime will be copied, and other dates and times
				// will remain as they were when the file was created.
				//
				// NOTE:
				// In Windows XP UDF, if you set it to -1, NtSetInformationFile 
				// will return an error.
				fbi.CreationTime.QuadPart   = 0;
				fbi.LastAccessTime.QuadPart = 0;
				fbi.ChangeTime.QuadPart     = 0;
			}
		}
	}
	else
	{
		RtlCopyMemory(&fbi,pFileAttributeInfo,sizeof(fbi));
	}

	Status = SetFileBasicInformation(hDstFile,&fbi);

	return Status;
}
