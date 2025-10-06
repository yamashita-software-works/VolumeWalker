#pragma once

//
// 2024-06-14
//

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
    );

EXTERN_C
BOOL
WINAPI
NtRenameFileW(
    __in LPCWSTR lpExistingFileName,
    __in LPCWSTR lpNewFileName,
	__in DWORD dwFlags
    );

EXTERN_C
BOOL
WINAPI
NtDeleteFileW(
    __in LPCWSTR lpFileName
    );

EXTERN_C
BOOL
WINAPI
NtRemoveDirectoryW(
    __in LPCWSTR lpPathName
    );

EXTERN_C
BOOL
WINAPI
NtSetFileAttributesW(
    __in LPCWSTR lpFileName,
	__in DWORD FileAttributes
    );


EXTERN_C
NTSTATUS
WINAPI
NtWin32BackupCopy(
	__in VOID* pfsop,
	__in HANDLE hDstFile,
	__in PCWSTR pDstFilePath,
	__in HANDLE hSrcFile,
	__in PCWSTR pSrcFilePath,
	__in BOOLEAN bDirectory,
	__in LPPROGRESS_ROUTINE lpProgressRoutine,
	__in LPVOID lpData
	);
