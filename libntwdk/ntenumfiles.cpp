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
    ULONG Flags,
    ENUMFILESCALLBACK pfnCallback,
    ULONG_PTR CallbackContext
    )
{
    HANDLE hDirectory;
    NTSTATUS Status = 0;
    BOOLEAN bRestartScan = TRUE;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING NtPathName = {0};
    UNICODE_STRING FileName = {0};
    PVOID pBuffer = NULL;
    ULONG cbBuffer = _PAGESIZE * 16;

    if( hRoot == NULL && pszDirectoryPath == NULL )
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

    if( pszDirectoryPath != NULL )
    {
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
    }
    else
    {
        hDirectory = hRoot;

        Status = STATUS_SUCCESS;
    }

    if( Status == STATUS_SUCCESS )
    {
        FILE_INFORMATION_CLASS InfoClass;

        InfoClass = FileIdBothDirectoryInformation;

        do
        {
            Status = NtQueryDirectoryFile(hDirectory,
                                NULL,NULL,NULL,
                                &IoStatus,
                                pBuffer,cbBuffer,
                                InfoClass,
                                FALSE,
                                FileName.Length == 0 ? NULL : & FileName,
                                bRestartScan
                                );

            if( Status == STATUS_SUCCESS )
            {
                FILE_ID_BOTH_DIR_INFORMATION *pFileInfo = (FILE_ID_BOTH_DIR_INFORMATION *)pBuffer;
    
                for(;;)
                {
                    if( pfnCallback(hDirectory,NtPathName.Buffer,0,pFileInfo,CallbackContext) == false )
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

        if( pszDirectoryPath != NULL )
            NtClose(hDirectory);
    }

    FreeMemory(pBuffer);

    return Status;
}

//---------------------------------------------------------------------------
//
//  EnumFiles_W()
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
    ULONG Flags,
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

    hr = ((FSDIRENUMCALLBACKPROC)pcb->Callback)(0,FileInfo,&fsdecbi,pcb->Context);

    return hr == S_OK ? TRUE : FALSE;
}

EXTERN_C
HRESULT
NTAPI
EnumDirectoryFiles_W(
    PCWSTR Path,
    PCWSTR FileNameFilter,
    ULONG Flags,
    FSDIRENUMCALLBACKPROC Callback,
    PVOID Context
    )
{
    HRESULT hr;
    NTSTATUS Status;
    PWSTR pR=NULL,pP=NULL;
    ULONG cchR=0,cchP=0;
    HANDLE hRoot = NULL;

    if( (hr = SplitRootPath_W(Path,&pR,&cchR,&pP,&cchP)) != S_OK )
        return hr;

    if( cchP > 0 )
    {
        if( OpenRootDirectory(pR,0,&hRoot) != STATUS_SUCCESS )
        {
            FreeMemory(pP);
            pP = DuplicateString(Path);
        }
    }
    else
    {
        FreeMemory(pP);
        pP = DuplicateString(Path);
    }

    INTERNAL_CALLBACK_BUFFER cb;
    cb.Callback = Callback;
    cb.Context  = Context;

    Status = EnumFiles(hRoot,pP,FileNameFilter,Flags,&EnumFilesCallback,(ULONG_PTR)&cb);

    NtClose(hRoot);

    FreeMemory(pR);
    FreeMemory(pP);

    return HRESULT_FROM_WIN32( RtlNtStatusToDosError(Status) );
}
