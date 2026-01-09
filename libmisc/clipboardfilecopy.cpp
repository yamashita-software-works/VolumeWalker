//***************************************************************************
//*                                                                         *
//*  clipboardfilecopy.h                                                    *
//*                                                                         *
//*  CLASS:    CLongFilePathClipboard                                       *
//*                                                                         *
//*  PURPOSE:  Implements clipboard manipulation for Long Path.             *
//*                                                                         *
//*  AUTHOR:   YAMASHITA Katsuhiro                                          *
//*                                                                         *
//*  HISTORY:  2015/02/04 - Created                                         *
//*                                                                         *
//***************************************************************************
//   Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//   Licensed under the MIT License.
#include "stdafx.h"
#include "mem.h"
#include "clipboardfilecopy.h"

static UINT g_uFormatLFP = 0;
static UINT g_uFormatDateTime = 0;

ULONG
WINAPI
InitializeLongPathClipboard(
	VOID
	)
{
	g_uFormatLFP = RegisterClipboardFormat( CFWSTR_LONGFILEPATHCLIPBOARDFORMAT );
	return 0;
}

BOOL
WINAPI
IsLongFilePathClipboardAvailable(
	VOID
	)
{
	return IsClipboardFormatAvailable(g_uFormatLFP);
}

UINT
WINAPI
GetLongFilePathClipboardFormat(
	VOID
	)
{
	return g_uFormatLFP;
}

//////////////////////////////////////////////////////////////////////////////

/*++

 EX)
 [Length][Flags][Name...][Length][Flags][Name...][Length][Flags][Name...][0x00000000]

--*/

CLongFilePathClipboard::CLongFilePathClipboard()
{
	m_hGlobal = NULL;
}

CLongFilePathClipboard::~CLongFilePathClipboard()
{
	Clear();
}

VOID CLongFilePathClipboard::Clear()
{
	int i,c;
	c = m_list.GetSize();
	for(i = 0; i < c; i++)
	{
		FS_LONGFILEPATH_CLIPBOARD_NAME *pName = m_list[i];
		_MemFree(pName);
	}
	m_list.RemoveAll();
}

BOOL CLongFilePathClipboard::Start(PCWSTR DirectoryName)
{
	// DirectoryName is NULL, to skip start operation.
	// (In this case, m_list is includes full-path list)
	//
	if( DirectoryName == NULL )
		return TRUE;

	ULONG cb = sizeof(FS_LONGFILEPATH_CLIPBOARD_NAME) + ((ULONG)wcslen(DirectoryName) * sizeof(WCHAR));

	FS_LONGFILEPATH_CLIPBOARD_NAME *pName = (FS_LONGFILEPATH_CLIPBOARD_NAME *)_MemAlloc( cb );
	if( pName == NULL )
		return FALSE;

	pName->Length = cb;
	pName->Flags  = FSLCNF_DIRECTORY_PATH;
	wcscpy(pName->Buffer,DirectoryName);

	return m_list.Add( pName );
}

BOOL CLongFilePathClipboard::AddFile(PCWSTR FileName)
{
	ULONG cb = sizeof(FS_LONGFILEPATH_CLIPBOARD_NAME) + ((ULONG)wcslen(FileName) * sizeof(WCHAR));

	FS_LONGFILEPATH_CLIPBOARD_NAME *pName = (FS_LONGFILEPATH_CLIPBOARD_NAME *)_MemAlloc( cb );
	if( pName == NULL )
		return FALSE;

	pName->Length = cb;
	pName->Flags  = FSLCNF_FILENAME;
	wcscpy(pName->Buffer,FileName);

	return m_list.Add( pName );
}

BOOL CLongFilePathClipboard::AddFullPath(PCWSTR FileFullPath)
{
	ULONG cb = sizeof(FS_LONGFILEPATH_CLIPBOARD_NAME) + ((ULONG)wcslen(FileFullPath) * sizeof(WCHAR));

	FS_LONGFILEPATH_CLIPBOARD_NAME *pName = (FS_LONGFILEPATH_CLIPBOARD_NAME *)_MemAlloc( cb );
	if( pName == NULL )
		return FALSE;

	pName->Length = cb;
	pName->Flags  = FSLCNF_FULLPATH;
	wcscpy(pName->Buffer,FileFullPath);

	return m_list.Add( pName );
}

BOOL CLongFilePathClipboard::Commit(HWND hWnd,BOOL bAllocHandle)
{
	ULONG cb = 0;
	BOOL bSuccess = FALSE;
	
	int i,c;
	c = m_list.GetSize();
	for(i = 0; i < c; i++)
	{
		cb += m_list[i]->Length;
	}

	cb += sizeof(ULONG); // Length for terminates.

	HGLOBAL hgbl;
	hgbl = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,cb);
	if( hgbl == NULL )
	{
		return FALSE;
	}

	PUCHAR pBuffer = (PUCHAR)GlobalLock(hgbl);

	if( pBuffer )
	{
		PUCHAR p = pBuffer;

		for(i = 0; i < c; i++)
		{
			FS_LONGFILEPATH_CLIPBOARD_NAME *pName = m_list[i];
			CopyMemory(p,(PUCHAR)pName,pName->Length);
			p += pName->Length;
		}

		GlobalUnlock(hgbl);

		if( !bAllocHandle )
		{
			// Mode that push data to clipboard.
			//
			if( OpenClipboard(hWnd) )
			{
				// If SetClipboardData succeeds, the system owns the object identified 
				// by the hMem parameter. The application may not write to or free 
				// the data once ownership has been transferred to the system, but it 
				// can lock and read from the data until the CloseClipboard function is
				// called. (The memory must be unlocked before the Clipboard is closed.)
				// If the hMem parameter identifies a memory object, the object must 
				// have been allocated using the function with the GMEM_MOVEABLE flag.
				SetClipboardData(g_uFormatLFP,hgbl);

				CloseClipboard();

				// Rmove internal cache
				Clear();

				bSuccess = TRUE;
			}
		}
		else
		{
			// Mode that returns a global memory handle.
			//
			m_hGlobal = hgbl;

			// Rmove internal cache
			Clear();

			bSuccess = TRUE;
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////////

ULONG
WINAPI
InitializeNtFileDateTimeClipboard(
	VOID
	)
{
	g_uFormatDateTime = RegisterClipboardFormat( CFWSTR_FILEDATETIMECLIPBOARDFORMAT );
	return 0;
}

BOOL
WINAPI
IsNtFileDateTimeClipboardAvailable(
	VOID
	)
{
	return IsClipboardFormatAvailable(g_uFormatDateTime);
}

UINT
WINAPI
GetNtFileDateTimeClipboardFormat(
	VOID
	)
{
	return g_uFormatDateTime;
}
