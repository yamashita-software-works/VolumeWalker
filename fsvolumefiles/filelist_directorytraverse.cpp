//*****************************************************************************
//
//  filelist_directorytraverse.cpp
//
//  PURPOSE: Directory Traverse.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-04-18 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "stdafx.h"
#include "..\libntwdk\libntwdk.h"
#include "..\libntwdk\ntnativeapi.h"
#include "filelist_directorytraverse.h"

typedef struct _FILE_OPERATION_CALLBACK_CONTEXT
{
	DIRECTORYSCANCALLBACK pfnCallback;
	PVOID Context;
/* ? */
	ULONG TotalDirectoryCount;
	ULONG TotalFileCount;
	LARGE_INTEGER TotalBytes;
	LARGE_INTEGER TotalAllocationBytes;
/* ?? */
	int iIndent;
} FILE_OPERATION_CALLBACK_CONTEXT;

//----------------------------------------------------------------------------
//
//  CallbackEnumDirectoryFiles()
//
//----------------------------------------------------------------------------
NTSTATUS
CALLBACK
CallbackEnumDirectoryFiles(
	ULONG CallbackReason,
	PCWSTR Path,
	PCWSTR RelativePath,
	UNICODE_STRING *FileName,
	NTSTATUS /*ntStatus*/,
	ULONG FileInfoType,  // Reserved always zero
	PVOID FileInfo,      // FILE_ID_BOTH_DIR_INFORMATION
	ULONG_PTR CallbackContext
	)
{
	FILE_OPERATION_CALLBACK_CONTEXT *pfoctx = (FILE_OPERATION_CALLBACK_CONTEXT *)CallbackContext;
	NTSTATUS Status = 0;
	HRESULT hr;

	if( pfoctx->pfnCallback == NULL )
		return STATUS_SUCCESS;

	switch( CallbackReason )
	{
		case FFCBR_FINDFILE:
		{
			FS_FILE_ID_BOTH_DIR_INFORMATION *pfibdi = (FS_FILE_ID_BOTH_DIR_INFORMATION *)FileInfo;

			UNICODE_STRING usDirPath;
			UNICODE_STRING usFullFilePath;
			RtlInitUnicodeString(&usDirPath,RelativePath);
			CombineUnicodeStringPath(&usFullFilePath,&usDirPath,FileName);

			CALLBACK_FILE_INFORMATION cfi;

			cfi.Path = Path;
			cfi.RelativePath = RelativePath;
			cfi.FileName = AllocateSzFromUnicodeString(FileName);
			cfi.UnicodeStringFileName = *FileName;
			cfi.Information = pfibdi;

			hr = pfoctx->pfnCallback(CallbackReason,&cfi,pfoctx->Context);

			FreeMemory( (PVOID)cfi.FileName );

			FreeUnicodeString(&usFullFilePath);

			break;
		}
		case FFCBR_DIRECTORYSTART: // open directory
		case FFCBR_DIRECTORYEND:   // close directory
		{
			CALLBACK_FILE_INFORMATION cfi = {0};
			cfi.Path = DuplicateString( Path );

			if( RelativePath && *RelativePath == 0 )
			{
				//
				// Calling phase of the pre-traverse starting / after traverse ending.
				//
				cfi.RelativePath = NULL;
				cfi.FileName = L"";
				
				RemoveBackslash( (PWSTR)cfi.Path  );

				hr = pfoctx->pfnCallback(CallbackReason,&cfi,pfoctx->Context);
			}
			else
			{
				//
				// Calling phase of under the traversing.
				//
				RemoveBackslash( (PWSTR)cfi.Path  );

				cfi.RelativePath = DuplicateString( RelativePath );

				PCWSTR pName = wcsrchr(cfi.Path,L'\\');
				if( pName )
				{
					cfi.FileName = ++pName;
					RtlInitUnicodeString(&cfi.UnicodeStringFileName,pName);
				}

				hr = pfoctx->pfnCallback(CallbackReason,&cfi,pfoctx->Context);
			}

			FreeMemory( (PVOID)cfi.Path );
			FreeMemory( (PVOID)cfi.RelativePath );

			break;
		}
		case FFCBR_ERROR:
		{
			hr = S_OK; // todo:
			break;
		}
	}

	if( hr == S_FO_DIRECTORY_HIERARCHY_TOO_DEEP )
	{
		Status = STATUS_TD_DIRECTORY_HIERARCHY_TOO_DEEP;
	}
	else if( hr == S_FO_SKIP )
	{
		Status = STATUS_TD_SKIP;
	}
	else if( hr == S_FALSE )
	{
		Status = STATUS_CANCELLED;
	}
	else if( hr != S_OK )
	{
		Status = STATUS_CANCELLED;
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  RecursiveEnumDirectoryFiles()
//
//----------------------------------------------------------------------------
HRESULT
WINAPI
RecursiveEnumDirectoryFiles(
	PCWSTR FullPath,
	PVOID p1, // reserved:
	PVOID p2, // reserved:
	ULONG f1, // reserved:
	DIRECTORYSCANCALLBACK pfnCallback,
	PVOID CallbackContext
	)
{
	NTSTATUS Status;
	UNICODE_STRING Path;
	UNICODE_STRING FileName;

	PWSTR Root;
	PWSTR RelativePath;
	SplitRootPath_W(FullPath,&Root,NULL,&RelativePath,NULL);

	RtlInitUnicodeStringEx(&Path,FullPath);
	RtlInitUnicodeStringEx(&FileName,L"*");

#ifdef _DEBUG
	PWSTR pszPath = AllocateSzFromUnicodeString(&Path);
	PWSTR pszFileName = AllocateSzFromUnicodeString(&FileName);
#endif

	FILE_OPERATION_CALLBACK_CONTEXT context = {0};
	context.pfnCallback = pfnCallback;
	context.Context = CallbackContext;
	Status = TraverseDirectory(Path,FileName,TRUE,DTF_NO_PROCESS_WILDCARD,&CallbackEnumDirectoryFiles,(ULONG_PTR)&context);

#ifdef _DEBUG
	FreeMemory(pszPath);
	FreeMemory(pszFileName);
#endif

	FreeMemory(Root);
	FreeMemory(RelativePath);

	return HRESULT_FROM_NT(Status);
}

//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//  CallbackCountFiles()
//
//  PURPOSE: Callback function that recursive counting files in directory tree.
//
//----------------------------------------------------------------------------
NTSTATUS
CALLBACK
CallbackCountFiles(
	ULONG CallbackReason,
	PCWSTR Path,
	PCWSTR RelativePath,
	UNICODE_STRING *FileName,
	NTSTATUS /*Status*/,
	ULONG FileInfoType,  // Reserved always zero
	PVOID FileInfo,      // FILE_ID_BOTH_DIR_INFORMATION
	ULONG_PTR CallbackContext
	)
{
	FILE_OPERATION_CALLBACK_CONTEXT *pfoctx = (FILE_OPERATION_CALLBACK_CONTEXT *)CallbackContext;
	NTSTATUS Status = 0;

	switch( CallbackReason )
	{
		case FFCBR_FINDFILE:
		{
			FS_FILE_ID_BOTH_DIR_INFORMATION *pfibdi = (FS_FILE_ID_BOTH_DIR_INFORMATION *)FileInfo;

			if( pfibdi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				pfoctx->TotalDirectoryCount++;
			else
				pfoctx->TotalFileCount++;

			pfoctx->TotalBytes.QuadPart += pfibdi->EndOfFile.QuadPart;
			pfoctx->TotalAllocationBytes.QuadPart += pfibdi->AllocationSize.QuadPart;

			// todo: alternate stream ?

			break;
		}
		case FFCBR_DIRECTORYSTART: // open directory
		{
			break;
		}
		case FFCBR_DIRECTORYEND:   // close directory
		{
			break;
		}
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  RecursiveDirectoryFileCount()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
HRESULT
WINAPI
RecursiveDirectoryFileCount(
	PCWSTR DirectoryPath, // Full qualify directory path
	DIR_FILECOUNT *pdfc,
	DIRECTORYSCANCALLBACK pfnCallback,
	PVOID CallbackContext
	)
{
	UNICODE_STRING Path;
	UNICODE_STRING FileName;

	PWSTR Root;
	PWSTR RelativePath;
	SplitRootPath_W(DirectoryPath,&Root,NULL,&RelativePath,NULL);

	RtlInitUnicodeStringEx(&Path,DirectoryPath);
	RtlInitUnicodeStringEx(&FileName,L"*");

#ifdef _DEBUG
	PWSTR pszPath = AllocateSzFromUnicodeString(&Path);
	PWSTR pszFileName = AllocateSzFromUnicodeString(&FileName);
#endif

	FILE_OPERATION_CALLBACK_CONTEXT context = {0};
	context.TotalDirectoryCount = 0;
	context.TotalFileCount = 0;
	context.pfnCallback = pfnCallback;
	context.Context = CallbackContext;
	TraverseDirectory(Path,FileName,TRUE,DTF_NO_PROCESS_WILDCARD,&CallbackCountFiles,(ULONG_PTR)&context);

#ifdef _DEBUG
	FreeMemory(pszPath);
	FreeMemory(pszFileName);
#endif
	pdfc->TotalDirectoryCount = context.TotalDirectoryCount;
	pdfc->TotalFileCount = context.TotalFileCount;
	pdfc->TotalSize = context.TotalBytes;
	pdfc->TotalAllocationBytes = context.TotalAllocationBytes;

	FreeMemory(Root);
	FreeMemory(RelativePath);

	return S_OK;
}
