//*****************************************************************************
//*                                                                           *
//*  utilnt.cpp                                                               *
//*                                                                           *
//*  NT native API functions                                                  *
//*                                                                           *
//*  Create: 2024-03-21                                                       *
//*                                                                           *
//*  Author: YAMASHITA Katsuhiro                                              *
//*                                                                           *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                  *
//*  Licensed under the MIT License.                                          *
//*                                                                           *
//*****************************************************************************
#include <ntifs.h>
#include <windef.h>
#include <ntstatus.h>
#include <winerror.h>
#include <strsafe.h>
#include <malloc.h>
#include "libntwdk.h"
#include "ntnativeapi.h"
//#include "ntobjecthelp.h"
//#include "wfswof.h"
#include "fsfilelib.h"
#include "..\libntwdk\ntnativeapi.h"
#include "..\libntwdk\ntpathcomponent.h"
#include "ntobjecthelp.h"

//----------------------------------------------------------------------------
//
//  NtMoveDirectoryEntry()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
LONG
NTAPI
NtMoveDirectoryEntry(
	HANDLE hExistingDirectory,
	PCWSTR pszExistingFileName,
	HANDLE hDestinationDirectory,
	PCWSTR pszNewFileName,
	BOOLEAN ReplaceIfExists
	)
{
	NTSTATUS Status;
	HANDLE hSrcFile;
	UNICODE_STRING us;

	if( hExistingDirectory == NULL || pszExistingFileName == NULL || 
		hDestinationDirectory == NULL || pszNewFileName == NULL )
	{
		_SetLastStatusDos( STATUS_INVALID_PARAMETER );
		return STATUS_INVALID_PARAMETER;
	}

	ULONG cbMoveBufferLength = sizeof(FILE_RENAME_INFORMATION) + WIN32_MAX_PATH_BYTES;
	FILE_RENAME_INFORMATION *pMoveBuffer = (FILE_RENAME_INFORMATION *)AllocMemory( cbMoveBufferLength );

	RtlInitUnicodeString(&us,pszExistingFileName);

	//
	// Target source file handle is must be source directory relative open.
	//
	Status = OpenFile_U(&hSrcFile,hExistingDirectory,
					&us,
					FILE_GENERIC_READ|FILE_GENERIC_WRITE|DELETE,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					FILE_OPEN_FOR_BACKUP_INTENT);

	if( Status == STATUS_SUCCESS )
	{
		IO_STATUS_BLOCK IoStatus;

		pMoveBuffer->ReplaceIfExists = ReplaceIfExists;
		pMoveBuffer->RootDirectory   = hDestinationDirectory;
		pMoveBuffer->FileNameLength  = us.Length;
		RtlCopyMemory(pMoveBuffer->FileName,us.Buffer,us.Length);

		Status = NtSetInformationFile(hSrcFile,&IoStatus,pMoveBuffer,cbMoveBufferLength,FileRenameInformation);

		NtClose(hSrcFile);
	}

	_SetLastStatusDos( Status );

	return Status;
}

//---------------------------------------------------------------------------
//
//  NtPathLookupDeviceNameFromPath()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
ULONG
APIENTRY
NtPathLookupDeviceNameFromPath(
	HANDLE *phspa,
    PCWSTR Path,
	ULONG Flags
    )
{
    HANDLE hObjDir;
    LONG Status;
    ULONG Index = 0;
    UNICODE_STRING usNtDeviceName;
	WCHAR NtDeviceName[WIN32_MAX_PATH];
	WCHAR DosDeviceName[WIN32_MAX_PATH];
	UNICODE_STRING usPath;

	if( phspa == NULL || Path == NULL )
		return NtStatusToDosError(STATUS_INVALID_PARAMETER);

	HANDLE hspa = SPtrArray_Create(0);
	if( hspa == NULL )
		return NtStatusToDosError(STATUS_NO_MEMORY);

	RtlInitUnicodeString(&usPath,Path);

    Status = OpenObjectDirectory( 
					(Flags & EDDTNF_LOCAL) ? L"\\??" : L"\\GLOBAL??",
					&hObjDir
					);

    if( Status == STATUS_SUCCESS )
    {
        while( QueryObjectDirectory(hObjDir,&Index,DosDeviceName,WIN32_MAX_PATH,NULL,0) == 0)
        {
			usNtDeviceName.Length = 0;
			usNtDeviceName.MaximumLength = sizeof(NtDeviceName);
			usNtDeviceName.Buffer = NtDeviceName;

			if( QuerySymbolicLinkObject_U(hObjDir,DosDeviceName,&usNtDeviceName,FALSE) == 0 )
			{
				// e.g.
				//
				// path:        "\Device\Foo\Bar\Xxx\Yyy"
				// device name: "\Device\Foo\Bar"
				// -> true
				//
				// path:        "\Device\Foo\Bar\Xxx\Yyy"
				// device name: "\Device\Zzz
				// -> false
				//
				if( usNtDeviceName.Length > 0 )
				{
					if( RtlPrefixUnicodeString(&usNtDeviceName,&usPath,TRUE) )
					{
						PWSTR pwsz = DuplicateString(usNtDeviceName.Buffer);
						if( pwsz )
							SPtrArray_Add(hspa,pwsz);
					}
				}
            }
        }

        CloseObjectDirectory( hObjDir );

		if( SPtrArray_GetCount(hspa) == 0 )
		{
			SPtrArray_Destroy(hspa);
			*phspa = NULL;
			Status = STATUS_OBJECT_NAME_NOT_FOUND;
		}
		else
		{
			*phspa = hspa;
			Status = STATUS_SUCCESS;
		}
    }

    return NtStatusToDosError(Status);
}
