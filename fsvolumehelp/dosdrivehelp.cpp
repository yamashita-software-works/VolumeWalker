//***************************************************************************
//*                                                                         *
//*  dosdrivehelp.cpp                                                       *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "volumehelp.h"
#include "ntnativehelp.h"
#include "..\inc\simplevalarray.h"

EXTERN_C
HRESULT
WINAPI
GetVolumeDrivePathsString(
	PWCHAR VolumeName,
	PWSTR Paths,
	DWORD cchPaths
	)
{
	DWORD CharCount = MAX_PATH + 1;
	PWCHAR Names = NULL;
	PWCHAR NameIdx = NULL;
	BOOL Success = FALSE;
	HRESULT hr = E_FAIL;

	if( Paths == NULL )
		return E_INVALIDARG;

	Paths[0] = 0;

	for(;;) 
	{
		Names = (PWCHAR) new WCHAR [CharCount];

		if( !Names ) 
		{
			return E_OUTOFMEMORY;
		}

		Success = GetVolumePathNamesForVolumeNameW(VolumeName, Names, CharCount, &CharCount);

		if( Success ) 
		{
			break;
		}

		if( GetLastError() != ERROR_MORE_DATA ) 
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			break;
		}

		delete [] Names;
		Names = NULL;
	}

	if( Success )
	{
		Paths[0] = 0;

		for( NameIdx = Names; NameIdx[0] != L'\0';  NameIdx += wcslen(NameIdx) + 1 ) 
		{
			if( Paths[0] != 0 )
				wcscat_s(Paths,cchPaths,L";");

			if( NameIdx[1] == L':' && NameIdx[2] == L'\\' && NameIdx[3] == L'\0' )
			{
				WCHAR szDrive[4];
				szDrive[0] = NameIdx[0];
				szDrive[1] = L':';
				szDrive[2] = L'\0';
				hr = StringCchCat(Paths,cchPaths,szDrive);
			}
			else
			{
				hr = StringCchCat(Paths,cchPaths,NameIdx);
			}

			if( hr != S_OK )
				break;
		}
	}

	if( Names != NULL ) 
	{
		delete[] Names;
	}

	return hr;
}

static HRESULT GetVolumeDriveName(PWCHAR VolumeName,PWSTR szDosName,int cchDosName)
{
    DWORD  CharCount = MAX_PATH + 1;
    PWCHAR NamesBuffer = NULL;
    PWCHAR NamePtr = NULL;
	DWORD  dwError = ERROR_SUCCESS;

    for(;;) 
    {
        NamesBuffer = _MemAllocStringBuffer( CharCount );

        if( !NamesBuffer ) 
        {
            return E_OUTOFMEMORY;
        }

        if( GetVolumePathNamesForVolumeNameW(VolumeName, NamesBuffer, CharCount, &CharCount) )
        {
            break;
        }

	    dwError = GetLastError();
        if( dwError != ERROR_MORE_DATA ) 
        {
            break;
        }

        //  Try again with the new suggested size.
	    _SafeMemFree( NamesBuffer );
    }

    if( dwError == ERROR_SUCCESS )
    {
		szDosName[0] = L'\0';
        for( NamePtr = NamesBuffer; *NamePtr != L'\0'; NamePtr += wcslen(NamePtr) + 1 ) 
        {
            if( NamePtr[1] == L':' && NamePtr[2] == L'\\' && NamePtr[3] == L'\0' )
			{
				StringCchCopy(szDosName,cchDosName,NamePtr);
				break;
			}
        }
    }

	_SafeMemFree( NamesBuffer );

	return HRESULT_FROM_WIN32(dwError);
}

static VOID GetVolumeNames(CValArray<PWSTR> *pvolnames)
{
	HANDLE hFind;
	WCHAR  VolumeName[MAX_PATH];

    hFind = FindFirstVolumeW(VolumeName, ARRAYSIZE(VolumeName));

    if( hFind != INVALID_HANDLE_VALUE )
    {
		do
		{
			pvolnames->Add( _MemAllocString(VolumeName) );
		}
		while( FindNextVolumeW(hFind, VolumeName, ARRAYSIZE(VolumeName)) );
	}
    FindVolumeClose(hFind);
}

EXTERN_C
HRESULT
WINAPI
EnumDosDriveItems(
	DOS_DRIVE_INFORMATION_ARRAY **DosDrivesTable
	)
{
	HRESULT hr = E_FAIL;

	WCHAR szDrives[ 26 * 4 + 1 ];
	GetLogicalDriveStrings(ARRAYSIZE(szDrives),szDrives);

	ULONG i,cDrives = 0;;
	WCHAR *p;

	if( DosDrivesTable == NULL )
		return E_INVALIDARG;

	// count valid drive letters.
	p = szDrives;
	while( *p )
	{
		cDrives++;
		p += (wcslen(p) + 1);
	}

	// allocate structure buffer.
	DOS_DRIVE_INFORMATION_ARRAY *pBuffer = (DOS_DRIVE_INFORMATION_ARRAY *)_MemAllocZero( sizeof(DOS_DRIVE_INFORMATION_ARRAY) + ((cDrives -1 ) * sizeof(DOS_DRIVE_INFORMATION)) );
	if( pBuffer == NULL )
		return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

	// get information each drive.
	WCHAR szBuffer[MAX_PATH];
	WCHAR FileSystemNameBuffer[MAX_PATH+1];
	WCHAR VolumeLabel[MAX_PATH+1];

	CValArray<PWSTR> volnames;
	GetVolumeNames(&volnames);

	int vn,cVolNames = volnames.GetCount();

	for( i = 0, p = szDrives; *p != L'\0'; p += (wcslen(p) + 1), i++ )
	{
		DOS_DRIVE_INFORMATION& drv = pBuffer->Drive[i];

		StringCchCopy(drv.szDrive,ARRAYSIZE(drv.szDrive),p);
		StringCchCopy(drv.szDriveRoot,ARRAYSIZE(drv.szDriveRoot),p);

		drv.DriveType = GetDriveType(p);

		if( GetDiskFreeSpaceEx(p,&drv.FreeBytesAvailable,&drv.TotalNumberOfBytes,&drv.TotalNumberOfFreeBytes) )
		{
			drv.State.Size = TRUE;
		}

		ZeroMemory(FileSystemNameBuffer,sizeof(FileSystemNameBuffer));
		ZeroMemory(VolumeLabel,sizeof(VolumeLabel));

		if( GetVolumeInformation(p,
					VolumeLabel,ARRAYSIZE(VolumeLabel),
					&drv.VolumeSerialNumber,
					&drv.MaximumComponentLength,
					&drv.FileSystemFlags,
					FileSystemNameBuffer,ARRAYSIZE(FileSystemNameBuffer)) )
		{
			drv.State.Information = TRUE;
		}
		drv.FileSystemName = _MemAllocString(FileSystemNameBuffer);
		drv.VolumeLabel = _MemAllocString(VolumeLabel);

		if( QueryDosDevice(drv.szDrive,szBuffer,ARRAYSIZE(szBuffer)) )
		{
			drv.Device = _MemAllocString(szBuffer);
		}

		for(vn = 0; vn < cVolNames; vn++)
		{
			WCHAR szDosDrive[4];
			szDosDrive[0] = 0;
			if( GetVolumeDriveName(volnames[vn],szDosDrive,ARRAYSIZE(szDosDrive) ) == S_OK &&
				szDosDrive[0] != 0 && wcsicmp(szDosDrive,drv.szDriveRoot) == 0 )
			{
				StringCchCopy(drv.szVolumeName,sizeof(drv.szVolumeName),volnames[vn]);
				break;
			}
		}
	}

	pBuffer->Count = i;

	for(i = 0; i < (ULONG)volnames.GetCount(); i++)
	{
		_MemFree( volnames[i] );
	}

	*DosDrivesTable = pBuffer;

	return S_OK;
}

EXTERN_C
HRESULT
WINAPI
FreeDosDriveItems(
	DOS_DRIVE_INFORMATION_ARRAY *DosDrivesTable
	)
{
	if( DosDrivesTable == NULL )
		return E_INVALIDARG;

	ULONG i;
	for(i = 0; i < DosDrivesTable->Count; i++)
	{
		DOS_DRIVE_INFORMATION& drv = DosDrivesTable->Drive[i];
		_SafeMemFree(drv.FileSystemName);
		_SafeMemFree(drv.Device);
		_SafeMemFree(drv.VolumeLabel);
	}

	_MemFree(DosDrivesTable);

	return S_OK;
}
