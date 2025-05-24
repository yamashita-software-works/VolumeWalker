#pragma once

#include "fileitem.h"

EXTERN_C
HANDLE
WINAPI
FILCreate(
	UINT Reserved
	);

EXTERN_C
BOOL
WINAPI
FILDestroy(
	HANDLE hfl
	);

EXTERN_C
INT
WINAPI
FILAddItem(
	HANDLE hfl,
	FILEITEM *Item
	);

EXTERN_C
BOOL
WINAPI
FILRemoveAllItems(
	HANDLE hfl
	);

EXTERN_C
INT
WINAPI
FILAddFileName(
	HANDLE hfl,
	PCWSTR FileName
	);

EXTERN_C
INT
WINAPI
FILGetItemCount(
	HANDLE hfl
	);

EXTERN_C
INT
WINAPI
FILGetItem(
	HANDLE hfl,
	INT Index,
	FILEITEM *Item
	);

EXTERN_C
FILEITEM *
WINAPI
FILGetItemPtr(
	HANDLE hfl,
	INT Index
	);

EXTERN_C
BOOL
WINAPI
FILDebugDump(
	UINT Reserved
	);
