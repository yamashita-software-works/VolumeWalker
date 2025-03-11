//****************************************************************************
//
//  fileitemarray.cpp
//
//  FILEITEM array management functions.
//
//  Author: YAMASHITA Katsuhiro
//
//  Create: 2024-07-24
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "stdafx.h"
#include "fileitemlist.h"

#ifdef _DEBUG
#include "simplevalarray.h"
CValArray<HANDLE> debug_array;
#endif

// internal structure
typedef struct _FILELISTITEM
{
	FILEITEM Item; // Item.hdr.FileName -> StrDup/LocalFree
} FILELISTITEM;

typedef struct _FILELISTSTRUCT
{
	HDPA    hdpa;
	HANDLE  hHeap; // Reserved
} FILELISTSTRUCT;

//
// Because this is to allow memory allocation/freeing 
// even if different inter modules use the library.
//
static PVOID _Alloc(SIZE_T cb)
{
	return (PVOID)LocalAlloc(LPTR,cb);
}

static VOID _Free(PVOID p)
{
	if( p != NULL )
		LocalFree(p);
}

static VOID _FreeItem(FILELISTITEM *pfli)
{
	_Free(pfli->Item.hdr.FileName);
	_Free(pfli->Item.hdr.Path);
	_Free(pfli);
}

static PWSTR _StrDup(PCWSTR psz)
{
	return StrDup(psz);
}

EXTERN_C
BOOL
WINAPI
FILDebugDump(
	UINT Reserved
	)
{
#ifdef _DEBUG
	WCHAR szBuf[MAX_PATH];
	int i,c = debug_array.GetCount();
	for(i = 0; i < c; i++)
	{
		StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"0x%08X\n",debug_array[i]);
		OutputDebugString(szBuf);
		break;
	}
	return TRUE;
#else
	return FALSE;
#endif
}

EXTERN_C
HANDLE
WINAPI
FILCreate(
	UINT Reserved
	)
{
	FILELISTSTRUCT *ps = (FILELISTSTRUCT*)_Alloc( sizeof(FILELISTSTRUCT) );
	if( ps == NULL )
		return NULL;

	ps->hHeap = GetProcessHeap();
	ps->hdpa = DPA_CreateEx(Reserved,ps->hHeap);

	if( ps->hHeap == NULL || ps->hdpa == NULL )
	{
		_Free(ps);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}

#ifdef _DEBUG
	debug_array.Add(ps);
#endif

	return (HANDLE)ps;
}

EXTERN_C
BOOL
WINAPI
FILDestroy(
	HANDLE hfl
	)
{
	FILELISTSTRUCT *ps = (FILELISTSTRUCT*)hfl;
	if( ps == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// todo: FILRemoveAllItems()
#ifdef _DEBUG
	ASSERT( DPA_GetPtrCount(ps->hdpa) == 0 );
#endif

	// ps->hHeap: currentaly process heap. no need free.
	DPA_Destroy(ps->hdpa);

	_Free(ps);

#ifdef _DEBUG
	int i,c = debug_array.GetCount();
	for(i = 0; i < c; i++)
	{
		if( debug_array[i] == (HANDLE)hfl )
		{
			debug_array.Delete(i);
			break;
		}
	}
#endif

	return TRUE;
}

EXTERN_C
BOOL
WINAPI
FILRemoveAllItems(
	HANDLE hfl
	)
{
	FILELISTSTRUCT *ps = (FILELISTSTRUCT*)hfl;
	if( ps == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	INT cItems = FILGetItemCount(hfl);

	for(INT i = 0; i < cItems; i++)
	{
		FILELISTITEM *pfli = (FILELISTITEM *)DPA_GetPtr(ps->hdpa,i);
		if( pfli )
		{
			_FreeItem( pfli );
		}
	}

	DPA_DeleteAllPtrs(ps->hdpa);

	return TRUE;
}

EXTERN_C
INT
WINAPI
FILAddItem(
	HANDLE hfl,
	FILEITEM *Item
	)
{
	FILELISTSTRUCT *ps = (FILELISTSTRUCT*)hfl;
	if( ps == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}

	if( Item == NULL || Item->hdr.FileName == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return -1;
	}

	FILELISTITEM *pFileListItem = (FILELISTITEM *)_Alloc( sizeof(FILELISTITEM) );
	if( pFileListItem == NULL )
	{
		return -1;
	}

	memcpy(&pFileListItem->Item,Item,sizeof(FILEITEM));

	pFileListItem->Item.hdr.FileName = _StrDup( Item->hdr.FileName );

	if( Item->hdr.Path )
		pFileListItem->Item.hdr.Path = _StrDup(Item->hdr.Path);

	if( pFileListItem->Item.hdr.FileName == NULL )
	{
		_Free(pFileListItem);
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return -1;
	}

	return DPA_AppendPtr(ps->hdpa,pFileListItem);
}

EXTERN_C
INT
WINAPI
FILAddFileName(
	HANDLE hfl,
	PCWSTR FileName
	)
{
	FILEITEM fi = {0};
	fi.hdr.FileName = (PWSTR)FileName;
	return FILAddItem(hfl,&fi);
}

EXTERN_C
INT
WINAPI
FILGetItemCount(
	HANDLE hfl
	)
{
	FILELISTSTRUCT *ps = (FILELISTSTRUCT*)hfl;
	if( ps == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0; // return zero
	}
	return DPA_GetPtrCount(ps->hdpa);
}

EXTERN_C
INT
WINAPI
FILGetItem(
	HANDLE hfl,
	INT Index,
	FILEITEM *Item
	)
{
	FILELISTSTRUCT *ps = (FILELISTSTRUCT*)hfl;
	if( ps == NULL || Item == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	FILELISTITEM *pfli = (FILELISTITEM *)DPA_GetPtr(ps->hdpa,Index);
	if( pfli == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	memcpy(Item,&pfli->Item,sizeof(FILEITEM));

	// note: must do not touch Item->hdr.FileName in returned buffer. it's refer only.

	return TRUE;
}

EXTERN_C
FILEITEM *
WINAPI
FILGetItemPtr(
	HANDLE hfl,
	INT Index
	)
{
	FILELISTSTRUCT *ps = (FILELISTSTRUCT*)hfl;
	if( ps == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	FILELISTITEM *pfli = (FILELISTITEM *)DPA_GetPtr(ps->hdpa,Index);
	if( pfli == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	return &pfli->Item;
}
