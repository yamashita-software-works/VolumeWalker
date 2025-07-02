//****************************************************************************
//
//  fileitemdata.cpp
//
//  @comment
//
//  Author:  YAMASHITA Katsuhiro
//
//  History: 2025-05-16 Created
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "stdafx.h"
#include "fsfilelib.h"
#include "fileitemdata.h"

#define _IS_CURDIR_NAME( fname ) (fname[0] == L'.' && fname[1] == L'\0')
#define _IS_PARENT_DIR_NAME( fname ) (fname[0] == L'.' && fname[1] == L'.' && fname[2] == L'\0')

#define CLUSTER_BASIC 0x1
#define CLUSTER_QUICK 0x2
#define CLUSTER_FULL  0x4
#define STANDARD_INFORMATION 0x800

inline void InitFileItemDetailInformation(FILEITEMEX *pFI)
{
    pFI->NumberOfLinks = -1;
	pFI->DeletePending = false;
	pFI->Directory = false;
	pFI->Wof = false;
	pFI->FirstLCN.QuadPart = 0;
	pFI->PhysicalDriveOffset.QuadPart = -1;
	pFI->PhysicalDriveNumber = (ULONG)-1;
}

static
void
GetFileAllocationAndLocationInformation(
	HANDLE hVolume,
	HANDLE hFile,
	PCWSTR pszCurDir,
	ULONG Flags,
	FILEITEMEX *pFI
	)
{
	if( Flags & STANDARD_INFORMATION )
	{
		FILE_STANDARD_INFO fsi = {0};
		if( GetFileInformationByHandleEx(hFile,FileStandardInfo,&fsi,sizeof(fsi)) )
		{
			if( pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				if( (pFI->EndOfFile.QuadPart != fsi.EndOfFile.QuadPart) || 
					(pFI->AllocationSize.QuadPart != fsi.AllocationSize.QuadPart) )
				{
					pFI->EndOfFile      = fsi.EndOfFile;
					pFI->AllocationSize = fsi.AllocationSize;
				}
			}
	    	pFI->NumberOfLinks = fsi.NumberOfLinks;
			pFI->DeletePending = fsi.DeletePending;
			pFI->Directory     = fsi.Directory;
		}
	}

	PWSTR RootDirectory=NULL;
	if( hVolume != NULL )
	{
		if( pszCurDir == NULL || *pszCurDir == L'\0' )
			SplitRootPath_W(pFI->hdr.Path,&RootDirectory,NULL,NULL,NULL);
		else
			SplitRootPath_W(pszCurDir,&RootDirectory,NULL,NULL,NULL);
	}
	else
	{
		RootDirectory = DuplicateString(pszCurDir);
	}

	if( pFI->AllocationSize.QuadPart != 0 )
	{
		if( Flags & CLUSTER_QUICK )
		{
			FS_CLUSTER_INFORMATION_BASIC_EX clusterex = {0};
			if( ReadFileClusterInformaion(NULL,hFile,RootDirectory,ClusterInformationBasicWithPhysicalLocation,&clusterex,sizeof(clusterex)) == 0 )
			{
				pFI->FirstLCN = clusterex.FirstLcn;
				pFI->PhysicalDriveOffset = clusterex.PhysicalLocation;
				pFI->PhysicalDriveNumber = clusterex.DiskNumber;
			}
		}
		else if( Flags & CLUSTER_FULL )
		{
			FS_CLUSTER_INFORMATION *pci;
			if( ReadFileClusterInformaion(NULL,hFile,RootDirectory,ClusterInformationAll,&pci,sizeof(pci)) == 0 )
			{
				pFI->FirstLCN = pci->Extents[0].Lcn;
				pFI->PhysicalDriveOffset.QuadPart = pci->Extents[0].PhysicalOffsets[0].PhysicalOffset->Offset;
				pFI->PhysicalDriveNumber = pci->Extents[0].PhysicalOffsets[0].PhysicalOffset->DiskNumber;
				FreeClusterInformation(pci);
			}
		}
		else if( Flags & CLUSTER_BASIC )
		{
			FS_CLUSTER_INFORMATION_BASIC cluster = {0};
			if( ReadFileClusterInformaion(NULL,hFile,RootDirectory,ClusterInformationBasic,&cluster,sizeof(cluster)) == 0 )
			{
				pFI->FirstLCN = cluster.FirstLcn;
			}
		}
	}
	else
	{
		if( (pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
		{
			pFI->Wof = (GetWofInformation(hFile,NULL,NULL) == S_OK);
		}
	}

	FreeMemory(RootDirectory);
}

//----------------------------------------------------------------------------
//
//  GetFileItemDetailInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
GetFileItemDetailInformation(
	FILEITEMEX *pFI,
	PCWSTR pszCurDir
	)
{
	NTSTATUS Status;

	PWSTR RootDirectory=NULL,RootRelativePath=NULL;
	ULONG cchRootDirectory=0,cchRootRelativePath=0;

	if( pszCurDir == NULL || *pszCurDir == L'\0' )
		SplitRootPath_W(pFI->hdr.Path,&RootDirectory,&cchRootDirectory,&RootRelativePath,&cchRootRelativePath);
	else if( pFI->hdr.Path )
		SplitRootPath_W(pszCurDir,&RootDirectory,&cchRootDirectory,&RootRelativePath,&cchRootRelativePath);
	else
		return E_INVALIDARG;

	InitFileItemDetailInformation(pFI);

	HANDLE hRootDirectory = NULL;
	if( OpenRootDirectory(RootDirectory,0,&hRootDirectory) != STATUS_SUCCESS )
	{
		//
		// The Root directory open failed. If so try open directory using
		// full path string without splitting the volume and root relative path.
		//
		FreeMemory(RootRelativePath);
		RootRelativePath = DuplicateString(pszCurDir); // duplicate full-path string
		hRootDirectory = NULL;
	}

	HANDLE hCurDir;
	if( (Status = OpenFile_W(&hCurDir,hRootDirectory,RootRelativePath,FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_DIRECTORY_FILE)) == 0 )
	{
		if( !_IS_CURDIR_NAME( pFI->hdr.FileName ) && !_IS_PARENT_DIR_NAME( pFI->hdr.FileName ) )
		{
			HANDLE hFile;
			ULONG DesiredAccess = FILE_READ_ATTRIBUTES|SYNCHRONIZE;
			ULONG Option = FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_SYNCHRONOUS_IO_NONALERT;
			if( OpenFile_W(&hFile,hCurDir,pFI->hdr.FileName,DesiredAccess,FILE_SHARE_READ|FILE_SHARE_WRITE,Option) == 0 )
			{
				GetFileAllocationAndLocationInformation(hRootDirectory,hFile,pszCurDir,CLUSTER_QUICK|STANDARD_INFORMATION,pFI);

				CloseHandle(hFile);
			}
		}
		CloseHandle(hCurDir);
	}

	FreeMemory(RootRelativePath);
	FreeMemory(RootDirectory);

	if( hRootDirectory )
		CloseHandle(hRootDirectory);

	return S_OK;
}

//----------------------------------------------------------------------------
//
//  AllocateFileItemEx()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
FILEITEMEX *
WINAPI
AllocateFileItemEx(
	PVOID pReserved,
	ULONG cbReserved
	)
{
	FILEITEMEX *pFI = (FILEITEMEX *)_MemAllocZero( sizeof(FILEITEMEX) );
	if( pFI == NULL )
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	else
		SetLastError(ERROR_SUCCESS);
	return pFI;
}

//----------------------------------------------------------------------------
//
//  DeleteFileItemEx()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
BOOL
WINAPI
DeleteFileItemEx(
	FILEITEMEX *pFI
	)
{
	if( pFI == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	_MemFree( pFI );

	SetLastError(ERROR_SUCCESS);
	return TRUE;
}
