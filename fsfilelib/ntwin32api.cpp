//*****************************************************************************
//
//  ntwin32api.cpp
//
//  PURPOSE: NT object namespace file operation with using Win32 API.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2024-06-14 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "stdafx.h"

#include "..\libntwdk\libntwdk.h"
#include "..\libntwdk\ntnativeapi.h"
#include "..\libntwdk\ntobjecthelp.h"

#include "..\libntwdk\ntnativeapi.h"

EXTERN_C
BOOL
WINAPI
NtCopyFileEx(
    __in     LPCWSTR lpExistingFileName,
    __in     LPCWSTR lpNewFileName,
    __in_opt LPPROGRESS_ROUTINE lpProgressRoutine,
    __in_opt LPVOID lpData,
    __in_opt LPBOOL pbCancel,
    __in     DWORD dwCopyFlags
    )
{
    BOOL bSuccess = FALSE;
    DWORD dwWin32Error = 0;
    PWSTR ExistingFileName = NULL;
    PWSTR NewFileName = NULL;

    // Check source file path
    if( HasPrefix( L"\\Device\\", lpExistingFileName ) )
    {
        SIZE_T cch = wcslen(lpExistingFileName) + 1;
        ExistingFileName = AllocStringBuffer( cch );
        if( ExistingFileName )
        {
			// Skip prefix "\Device\" and replace to "\??\".
            if( !(StringCchCopy(ExistingFileName,cch,L"\\??\\") == S_OK &&
                  StringCchCat(ExistingFileName,cch,&lpExistingFileName[8]) == S_OK) )
            {
                dwWin32Error = ERROR_INVALID_FUNCTION;
                goto _cleanup;
            }
        }
    }
    else
    {
        ExistingFileName = DuplicateString(lpExistingFileName);
    }

    if( ExistingFileName == NULL )
    {
        dwWin32Error = ERROR_NOT_ENOUGH_MEMORY;
        goto _cleanup;
    }

    // Check destination file path
    if( HasPrefix( L"\\Device\\", lpNewFileName ) )
    {
        SIZE_T cch = wcslen(lpNewFileName) + 1;
        NewFileName = AllocStringBuffer( cch );
        if( NewFileName )
        {
			// Skip prefix "\Device\" and replace to "\??\".
            if( !(StringCchCopy(NewFileName,cch,L"\\??\\") == S_OK &&
                  StringCchCat(NewFileName,cch,&lpNewFileName[8]) == S_OK) )
            {
                dwWin32Error = ERROR_INVALID_FUNCTION;
                goto _cleanup;
            }
        }
    }
    else
    {
        NewFileName = DuplicateString(lpNewFileName);
    }

    if( NewFileName == NULL )
    {
        dwWin32Error = ERROR_NOT_ENOUGH_MEMORY;
        goto _cleanup;
    }

    bSuccess = CopyFileEx(
                    ExistingFileName,
                    NewFileName,
                    lpProgressRoutine,
                    lpData,
                    pbCancel,
                    dwCopyFlags
                    );

    dwWin32Error = GetLastError();

_cleanup:

    FreeMemory(ExistingFileName);
    FreeMemory(NewFileName);

    SetLastError( dwWin32Error );

    return bSuccess;
}

EXTERN_C
BOOL
WINAPI
NtRenameFileW(
    __in LPCWSTR lpExistingFileName,
    __in LPCWSTR lpNewFileName,
	__in DWORD dwFlags
    )
{
    BOOL bSuccess = FALSE;
    DWORD dwWin32Error = ERROR_SUCCESS;
    PWSTR ExistingFileName = NULL;
    PWSTR NewFileName = NULL;

    // Check source file path
    if( HasPrefix( L"\\Device\\", lpExistingFileName ) )
    {
        SIZE_T cch = wcslen(lpExistingFileName) + 1;
        ExistingFileName = AllocStringBuffer( cch );
        if( ExistingFileName )
        {
			// Skip prefix "\Device\" and replace to "\??\".
            if( !(StringCchCopy(ExistingFileName,cch,L"\\??\\") == S_OK &&
                  StringCchCat(ExistingFileName,cch,&lpExistingFileName[8]) == S_OK) )
            {
                dwWin32Error = ERROR_INVALID_FUNCTION;
                goto _cleanup;
            }
        }
    }
    else
    {
        ExistingFileName = DuplicateString(lpExistingFileName);
    }

    if( ExistingFileName == NULL )
    {
        dwWin32Error = ERROR_NOT_ENOUGH_MEMORY;
        goto _cleanup;
    }

    // Check destination file path
    if( HasPrefix( L"\\Device\\", lpNewFileName ) )
    {
        SIZE_T cch = wcslen(lpNewFileName) + 1;
        NewFileName = AllocStringBuffer( cch );
        if( NewFileName )
        {
			// Skip prefix "\Device\" and replace to "\??\".
            if( !(StringCchCopy(NewFileName,cch,L"\\??\\") == S_OK &&
                  StringCchCat(NewFileName,cch,&lpNewFileName[8]) == S_OK) )
            {
                dwWin32Error = ERROR_INVALID_FUNCTION;
                goto _cleanup;
            }
        }
    }
    else
    {
        NewFileName = DuplicateString(lpNewFileName);
    }

    if( NewFileName == NULL )
    {
        dwWin32Error = ERROR_NOT_ENOUGH_MEMORY;
        goto _cleanup;
    }

    bSuccess = MoveFileEx(
                    ExistingFileName,
                    NewFileName,
                    dwFlags
                    );

	if( !bSuccess )
		dwWin32Error = GetLastError();

_cleanup:

    FreeMemory(ExistingFileName);
    FreeMemory(NewFileName);

    SetLastError( dwWin32Error );

    return bSuccess;
}

EXTERN_C
BOOL
WINAPI
NtDeleteFileW(
    __in LPCWSTR lpFileName
    )
{
    BOOL bSuccess = FALSE;
    DWORD dwWin32Error = ERROR_SUCCESS;
    PWSTR ExistingFileName = NULL;
    PWSTR NewFileName = NULL;

    // Check target file path
    if( HasPrefix( L"\\Device\\", lpFileName ) )
    {
        SIZE_T cch = wcslen(lpFileName) + 1;
        ExistingFileName = AllocStringBuffer( cch );
        if( ExistingFileName )
        {
			// Skip prefix "\Device\" and replace to "\??\".
            if( !(StringCchCopy(ExistingFileName,cch,L"\\??\\") == S_OK &&
                  StringCchCat(ExistingFileName,cch,&lpFileName[8]) == S_OK) )
            {
                dwWin32Error = ERROR_INVALID_FUNCTION;
                goto _cleanup;
            }
        }
    }
    else
    {
        ExistingFileName = DuplicateString(lpFileName);
    }

    if( ExistingFileName == NULL )
    {
        dwWin32Error = ERROR_NOT_ENOUGH_MEMORY;
        goto _cleanup;
    }

	bSuccess = DeleteFile( ExistingFileName );

	if( !bSuccess )
		dwWin32Error = GetLastError();

_cleanup:

    FreeMemory(ExistingFileName);

    SetLastError( dwWin32Error );

	return bSuccess;
}

EXTERN_C
BOOL
WINAPI
NtRemoveDirectoryW(
    __in LPCWSTR lpPathName
    )
{
    BOOL bSuccess = FALSE;
    DWORD dwWin32Error = ERROR_SUCCESS;
    PWSTR ExistingFileName = NULL;
    PWSTR NewFileName = NULL;

    // Check target directory path
    if( HasPrefix( L"\\Device\\", lpPathName ) )
    {
        SIZE_T cch = wcslen(lpPathName) + 1;
        ExistingFileName = AllocStringBuffer( cch );
        if( ExistingFileName )
        {
			// Skip prefix "\Device\" and replace to "\??\".
            if( !(StringCchCopy(ExistingFileName,cch,L"\\??\\") == S_OK &&
                  StringCchCat(ExistingFileName,cch,&lpPathName[8]) == S_OK) )
            {
                dwWin32Error = ERROR_INVALID_FUNCTION;
                goto _cleanup;
            }
        }
    }
    else
    {
        ExistingFileName = DuplicateString(lpPathName);
    }

    if( ExistingFileName == NULL )
    {
        dwWin32Error = ERROR_NOT_ENOUGH_MEMORY;
        goto _cleanup;
    }

	bSuccess = RemoveDirectory( ExistingFileName );

	if( !bSuccess )
		dwWin32Error = GetLastError();

_cleanup:

    FreeMemory(ExistingFileName);

    SetLastError( dwWin32Error );

	return bSuccess;
}

EXTERN_C
BOOL
WINAPI
NtSetFileAttributesW(
    __in LPCWSTR lpFileName,
	__in DWORD FileAttributes
    )
{
    BOOL bSuccess = FALSE;
    DWORD dwWin32Error = ERROR_SUCCESS;
    PWSTR ExistingFileName = NULL;
    PWSTR NewFileName = NULL;

    // Check target file path
    if( HasPrefix( L"\\Device\\", lpFileName ) )
    {
        SIZE_T cch = wcslen(lpFileName) + 1;
        ExistingFileName = AllocStringBuffer( cch );
        if( ExistingFileName )
        {
			// Skip prefix "\Device\" and replace to "\??\".
            if( !(StringCchCopy(ExistingFileName,cch,L"\\??\\") == S_OK &&
                  StringCchCat(ExistingFileName,cch,&lpFileName[8]) == S_OK) )
            {
                dwWin32Error = ERROR_INVALID_FUNCTION;
                goto _cleanup;
            }
        }
    }
    else
    {
        ExistingFileName = DuplicateString(lpFileName);
    }

    if( ExistingFileName == NULL )
    {
        dwWin32Error = ERROR_NOT_ENOUGH_MEMORY;
        goto _cleanup;
    }

	bSuccess = SetFileAttributesW( ExistingFileName, FileAttributes );

	if( !bSuccess )
		dwWin32Error = GetLastError();

_cleanup:

    FreeMemory(ExistingFileName);

    SetLastError( dwWin32Error );

	return bSuccess;
}
