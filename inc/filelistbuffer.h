#pragma once

//
//  File List Buffer Structure
//
typedef struct _FS_FLITEM
{
	DWORD Flags;
	DWORD Attributes;
	union {
		PWSTR Name;
		ULONG_PTR NameOffset;
	};
} FS_FLITEM;

#define FLI_FLG_NAME_OFFSET 0x80000000
#define FLI_FLG_PATH        0x00000000
#define FLI_FLG_DIRECTORY   0x00000001 // Reserved
#define FLI_FLG_FILENAME    0x00000002 // Reserved

typedef struct _FS_FILELISTBUFFER
{
	SIZE_T Size;
	DWORD Flags;
	DWORD Count;
	FS_FLITEM File[1];
} FS_FILELISTBUFFER;

#define FOPFL_SET_NAME_FIELD_OFFSET 0x1

#ifdef __cplusplus

inline PWSTR FLB_GetStrPtr( FS_FILELISTBUFFER *ptr, ULONG_PTR offset )
{
	return (PWSTR)(((ULONG_PTR)ptr)+offset);
}

inline PWSTR FLB_GetFileNamePtr( FS_FILELISTBUFFER *ptr, FS_FLITEM *item )
{
	if( item->Flags & FLI_FLG_NAME_OFFSET )
		return FLB_GetStrPtr(ptr,item->NameOffset);
	return item->Name;
}

inline PWSTR FLB_GetFileName( FS_FILELISTBUFFER *ptr, int iIndex )
{
	return FLB_GetFileNamePtr( ptr, &ptr->File[iIndex] );
}

#endif

#define FLB_GETSTRPTR(p,off)  (PWSTR)(((ULONG_PTR)p)+(off))
