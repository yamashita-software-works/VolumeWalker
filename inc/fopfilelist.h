#pragma once
//***************************************************************************
//*                                                                         *
//*  fopfilelist.h                                                          *
//*                                                                         *
//*  Create: 2024-01-17                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************

#include "filelistbuffer.h"

#ifdef _DEBUG
#define _DEBUG_MEMLIB 1
#endif

struct AllocMem
{
	__inline PVOID _Alloc(SIZE_T cb)
	{
#if _DEBUG_MEMLIB
		return _MemAllocZero( cb );
#else
		return CoTaskMemAlloc( cb );
#endif
	}

	__inline PVOID _ReAlloc(PVOID p,SIZE_T cb)
	{
#if _DEBUG_MEMLIB
		return _MemReAlloc(p, cb);
#else
		return CoTaskMemRealloc(p, cb);
#endif
	}

	__inline VOID _Free(PVOID p)
	{
#if _DEBUG_MEMLIB
		_MemFree(p);
#else
		CoTaskMemFree( p );
#endif
	}

	#define _SafeFree(p) if(p) { _Free(p); p=NULL; }

	__inline PWSTR _DupString(PCWSTR psz)
	{
#if _DEBUG_MEMLIB
		return _MemAllocString(psz);
#else
		SIZE_T cch = wcslen(psz) + 1;
		PWSTR pszBuffer = (PWSTR)CoTaskMemAlloc( cch * sizeof(WCHAR) );
		if( pszBuffer )
			StringCchCopy(pszBuffer,cch,psz);
		return pszBuffer;
#endif
	}

	__inline PWSTR _AllocStringBuffer(SIZE_T cch)
	{
#if _DEBUG_MEMLIB
		return _MemAllocStringBuffer(cch);
#else
		return (PWSTR)CoTaskMemAlloc( (cch + 1) * sizeof(WCHAR) );
#endif
	}
};

//
//  File List for File Operations
//

typedef enum {
	FoItemNone=-1,
	FoItemPath=0,
	FoItemFile=1,
	FoItemDirectory=2,
} FO_ITEM_TYPE;

typedef struct _FO_FILELISTITEM {
	PWSTR  Name;
	ULONG  Type;
	ULONG  FileAttributes;
} FO_FILELISTITEM;

interface IFileOperationList
{
	virtual const INT GetItemCount() const = 0;
	virtual FO_FILELISTITEM& GetItem(int iIndex) const  = 0;
	virtual const FO_FILELISTITEM *GetList() const = 0;
	virtual PCWSTR GetSourcePath() const = 0;
	virtual PCWSTR GetDestinationPath() const = 0;
};

struct CFileOperationList : 
	public IFileOperationList,
	protected AllocMem
{
	//
	// Interface Implementation
	//
	virtual FO_FILELISTITEM& GetItem(int iIndex) const 
	{
		return Files[iIndex];
	}

	virtual const INT GetItemCount() const
	{
		return (INT)this->Index;
	}

	virtual const FO_FILELISTITEM *GetList() const
	{
		return Files;
	}

	virtual PCWSTR GetDestinationPath() const
	{
		return DestinationPath;
	}

	virtual PCWSTR GetSourcePath() const
	{
		return SourcePath;
	}

	//
	// Class Implementation
	//
	CFileOperationList(int cFileCount,int cGrow=8)
	{
		this->Files = (FO_FILELISTITEM *)_Alloc( sizeof(FO_FILELISTITEM) * cFileCount );
		this->Count = cFileCount;
		this->Grow = cGrow;
		this->Index = 0;
		this->SourcePath = NULL;
		this->DestinationPath = NULL;
	}

	~CFileOperationList()
	{
		for(int i = 0; i < this->Index; i++)
		{
			_SafeFree(Files[i].Name);
		}

		_SafeFree(Files);
		_SafeFree(DestinationPath);
		_SafeFree(SourcePath);

		Count = 0;
		Grow = 0;
		Index = 0;
	}

	BOOL Add(PCWSTR FileName,ULONG Attributes=0,ULONG ItemType=FoItemPath)
	{
		if( this->Index == this->Count )
		{
			this->Count += this->Grow;
			FO_FILELISTITEM *tmp = this->Files;
			tmp = (FO_FILELISTITEM *)_ReAlloc(tmp, sizeof(FO_FILELISTITEM) * this->Count);
			if( tmp == NULL )
			{
				_SafeFree(this->Files);
				return FALSE;
			}
			this->Files = tmp;
		}
		PWSTR psz = _DupString( FileName );
		if( psz == NULL )
			return FALSE;
		this->Files[this->Index].Name = psz;
		this->Files[this->Index].Type = ItemType;
		this->Files[this->Index].FileAttributes = Attributes;
		this->Index++;
		return TRUE;
	}

	BOOL SetDestinationDirectory(PCWSTR pszCurPath,PCWSTR pszDirectoryName)
	{
		size_t cch = 0;
		cch = wcslen(pszCurPath);
		if( pszDirectoryName )
		{
			cch ++;
			cch += wcslen(pszDirectoryName);
		}

		DestinationPath = _AllocStringBuffer( cch );
		if( DestinationPath == NULL )
			return FALSE;

		StringCchCopy(DestinationPath,cch+1,pszCurPath);

		if( pszDirectoryName )
		{
			if( !IsLastCharacterBackslash(DestinationPath) )
				StringCchCat(DestinationPath,cch+1,L"\\");
			StringCchCat(DestinationPath,cch+1,pszDirectoryName);
		}

		return TRUE;
	}

	HRESULT CreateBuffer(FS_FILELISTBUFFER **FileListBuffer,DWORD dwFlags=0)
	{
		HRESULT hr;

		CFileOperationList *pfop = this;

		ULONG Count = pfop->GetItemCount();
	
		ULONG cbSize;
		cbSize = (sizeof(FS_FILELISTBUFFER) + (Count * sizeof(FS_FLITEM))) - sizeof(FS_FLITEM);
	
		ULONG cbNameBufferSize = 0;
		ULONG i;
		for(i = 0; i < Count; i++)
		{
			cbNameBufferSize += ((ULONG)wcslen(pfop->Item(i).Name) + 1);
		}
	
		cbNameBufferSize++; // Reserved
	
		cbNameBufferSize *= sizeof(WCHAR);
	
		// Allocate using task allocator.
		FS_FILELISTBUFFER *pfl;
		pfl = (FS_FILELISTBUFFER *)CoTaskMemAlloc( cbNameBufferSize + cbSize );

		if( pfl )
		{
			ZeroMemory( pfl, cbNameBufferSize + cbSize );

			PWSTR pNameStore = (PWSTR)(((ULONG_PTR)pfl) + cbSize);

			for(i = 0; i < Count; i++)
			{
				pfl->File[i].Flags      = pfop->Item(i).Type;
				pfl->File[i].Attributes = pfop->Item(i).FileAttributes;
				pfl->File[i].Name       = pNameStore;

				SIZE_T cch = wcslen(pfop->Item(i).Name);
				StringCchCopy(pNameStore,cch+1,pfop->Item(i).Name);

				pNameStore += (cch + 1);
			}
	
			if( dwFlags & FOPFL_SET_NAME_FIELD_OFFSET )
			{  
				for(i = 0; i < Count; i++)
				{
					pfl->File[i].NameOffset = (ULONG_PTR)pfl->File[i].Name - (ULONG_PTR)pfl;
					pfl->File[i].Flags |= FLI_FLG_NAME_OFFSET;
				}
			}

			pfl->Count = Count;
			pfl->Size = cbNameBufferSize + cbSize;
	
			*FileListBuffer = pfl;

			hr = S_OK;
		}
		else
		{
			*FileListBuffer = NULL;
			hr = E_OUTOFMEMORY;
		}
		return hr;
	}

	FO_FILELISTITEM& operator [] (int iIndex) const 
	{
		return Files[iIndex];
	}

	FO_FILELISTITEM& Item(int iIndex) const 
	{
		return Files[iIndex];
	}

	PCWSTR SetSourcePath(PCWSTR pszPath)
	{
		_SafeFree(SourcePath);
		SourcePath = _DupString(pszPath);
		return SourcePath;
	}

private:
	FO_FILELISTITEM *Files;
	PWSTR DestinationPath;
	PWSTR SourcePath;
	INT Count;
	INT Grow;
	INT Index;
};

struct CFileOperationHelp
{
	static BOOL AreListItemsSameSource(CFileOperationList *pFiles,PWSTR *ppSourcePath)
	{
		ULONG i,cFiles;
	
		cFiles = pFiles->GetItemCount();
	
		const FO_FILELISTITEM *pFL = pFiles->GetList();
	
		PWSTR pszCheckPath = NULL;
		BOOL bResult = FALSE;
	
		for(i = 0; i < cFiles; i++)
		{
			if( pFL[i].Type == FoItemPath )
			{
				UNICODE_STRING Path = {0};
				UNICODE_STRING FileName = {0};
	
				SplitPathFileName_W(pFL[i].Name,&Path,&FileName);
				
#ifdef _DEBUG
				PWSTR p1 = AllocateSzFromUnicodeString(&Path);
				PWSTR p2 = AllocateSzFromUnicodeString(&FileName);
				FreeMemory(p1);
				FreeMemory(p2);
#endif
	
				if( pszCheckPath )
				{
					PWSTR p = AllocateSzFromUnicodeString(&Path);
					if( _wcsicmp(pszCheckPath,p) != 0 )
					{
						FreeMemory(p);
						break;
					}
					FreeMemory(p);
				}
				else
				{
					pszCheckPath = AllocateSzFromUnicodeString(&Path);
				}
			}
		}
	
		bResult = (cFiles == i);
	
		if( pszCheckPath && ppSourcePath )
			*ppSourcePath = _MemAllocString(pszCheckPath);
	
		FreeMemory(pszCheckPath);
	
		return bResult;
	}
};
