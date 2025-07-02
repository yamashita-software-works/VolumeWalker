#pragma once

#include "fileitem.h"

EXTERN_C
HRESULT
WINAPI
GetFileItemDetailInformation(
	FILEITEMEX *pFI,
	PCWSTR pszCurDir
	);

EXTERN_C
FILEITEMEX *
WINAPI
AllocateFileItemEx(
	PVOID pReserved,
	ULONG cbReserved
	);

EXTERN_C
BOOL
WINAPI
DeleteFileItemEx(
	FILEITEMEX *pFI
	);
