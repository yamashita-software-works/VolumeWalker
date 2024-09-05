//***************************************************************************
//*                                                                         *
//*  ntenumfiles.cpp                                                        *
//*                                                                         *
//*  Create: 2021-04-12                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include <ntifs.h>
#include <stdio.h>
#include <malloc.h>
#include <strsafe.h>
#include <locale.h>
#include <conio.h>
#include <winerror.h>
#include "ntnativeapi.h"
#include "libntwdk.h"

#define _PAGESIZE 4096

//---------------------------------------------------------------------------
//
//  EnumFiles()
//
//  PURPOSE: Simple enumerate files with NT native API. 
//
//---------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
EnumFiles(
    HANDLE hRoot,
    PCWSTR pszDirectoryPath,
    PCWSTR pszFileName,
    ENUMFILESCALLBACK pfnCallback,
    ULONG_PTR CallbackContext
    )
{
    HANDLE hDirectory;
    NTSTATUS Status = 0;
    BOOLEAN bRestartScan = TRUE;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING NtPathName;
    UNICODE_STRING FileName;
    PVOID pBuffer = NULL;
    ULONG cbBuffer = _PAGESIZE * 16;

    if( pszDirectoryPath == NULL )
        return STATUS_INVALID_PARAMETER;
    
    if( pfnCallback == NULL )
        return STATUS_INVALID_PARAMETER;

    if( pszFileName == NULL )
        RtlZeroMemory(&FileName,sizeof(FileName));
    else
        RtlInitUnicodeString(&FileName,pszFileName);

    pBuffer = AllocMemory( cbBuffer );

    if( pBuffer == NULL )
        return STATUS_NO_MEMORY;

    RtlInitUnicodeString(&NtPathName,pszDirectoryPath);

    InitializeObjectAttributes(&ObjectAttributes,&NtPathName,0,hRoot,NULL);

    Status = NtOpenFile(&hDirectory,
                FILE_LIST_DIRECTORY|FILE_TRAVERSE|SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT
                // FILE_OPEN_REPARSE_POINT bypass reparse point processing for the file. 
                );

    if( Status == STATUS_SUCCESS )
    {
        do
        {
            Status = NtQueryDirectoryFile(hDirectory,
                                NULL,NULL,NULL,
                                &IoStatus,
                                pBuffer,cbBuffer,
                                FileIdBothDirectoryInformation,
                                FALSE,
                                FileName.Length == 0 ? NULL : & FileName,
                                bRestartScan
                                );

            if( Status == STATUS_SUCCESS )
            {
                FILE_ID_BOTH_DIR_INFORMATION *pFileInfo = (FILE_ID_BOTH_DIR_INFORMATION *)pBuffer;

                for(;;)
                {
                    if( pfnCallback(hDirectory,NtPathName.Buffer,pFileInfo,CallbackContext) == false )
                    {
                        Status = STATUS_CANCELLED;
                        break;
                    }

                    if( pFileInfo->NextEntryOffset == 0 )
                    {
                        break;
                    }

                    ((ULONG_PTR&)pFileInfo) += pFileInfo->NextEntryOffset;
                }
            }

            // NOTE: The RestartScan parameter is currently ignored.
            bRestartScan = FALSE;
        }
        while( Status == STATUS_SUCCESS );

        if( STATUS_NO_MORE_FILES == Status )
            Status = STATUS_SUCCESS;

        NtClose(hDirectory);
    }

    FreeMemory(pBuffer);

    // NOTE:
    // status STATUS_NO_MORE_FILES is return through as it is.
    return Status;
}

//---------------------------------------------------------------------------
//
//  WinEnumFiles()
//
//---------------------------------------------------------------------------
typedef struct _INTERNAL_CALLBACK_BUFFER
{
    PVOID Callback;
    PVOID Context;
    ULONG Flags;
} INTERNAL_CALLBACK_BUFFER;

static
BOOLEAN
CALLBACK
EnumFilesCallback(
    HANDLE hDirectory,
    PCWSTR DirectoryName,
    PVOID FileInfo,
    ULONG_PTR CallbackContext
    )
{
    HRESULT hr;
    INTERNAL_CALLBACK_BUFFER *pcb = (INTERNAL_CALLBACK_BUFFER *)CallbackContext;

    FSDIRENUMCALLBACKINFO fsdecbi;
    fsdecbi.DirectoryHandle = hDirectory;
    fsdecbi.Path = DirectoryName;

    FILE_ID_BOTH_DIR_INFORMATION *pBoth = (FILE_ID_BOTH_DIR_INFORMATION *)FileInfo;

    UNICODE_STRING usName;
    usName.Length = usName.MaximumLength = (USHORT)pBoth->FileNameLength;
    usName.Buffer = pBoth->FileName;

    hr = ((FSHELPENUMCALLBACKPROC)pcb->Callback)(0,FileInfo,&fsdecbi,pcb->Context);

    return hr == S_OK ? TRUE : FALSE;
}

EXTERN_C
HRESULT
NTAPI
EnumFiles_W(
    PCWSTR Path,
    PCWSTR FileNameFilter,
    ULONG Flags,
    FSHELPENUMCALLBACKPROC Callback,
    PVOID Context
    )
{
	HRESULT hr;
    NTSTATUS Status;
    UNICODE_STRING usRoot;
    PWSTR pR=NULL,pP=NULL;
    ULONG cchR=0,cchP=0;

    if( (hr = SplitRootPath_W(Path,&pR,&cchR,&pP,&cchP)) != S_OK )
		return hr;

    RtlInitUnicodeString(&usRoot,pR);

    HANDLE hRoot = NULL;
    if( OpenRootDirectory(pR,0,&hRoot) != STATUS_SUCCESS )
    {
		FreeMemory(pP);

		pP = DuplicateString(Path);

        hRoot = NULL;
    }

    INTERNAL_CALLBACK_BUFFER cb;
    cb.Callback = Callback;
    cb.Context  = Context;

    Status = EnumFiles(hRoot,pP,FileNameFilter,&EnumFilesCallback,(ULONG_PTR)&cb);

    NtClose(hRoot);

    FreeMemory(pR);
    FreeMemory(pP);

    return HRESULT_FROM_WIN32( RtlNtStatusToDosError(Status) );
}
