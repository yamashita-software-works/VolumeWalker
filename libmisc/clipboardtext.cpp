//***************************************************************************
//*                                                                         *
//*  clipboardtext.h                                                        *
//*                                                                         *
//*  PURPOSE:  Implements clipboard for text.                               *
//*                                                                         *
//*  AUTHOR:   YAMASHITA Katsuhiro                                          *
//*                                                                         *
//*  HISTORY:  2023/06/28 - Created (Origin code is ProcessWalker in 2001.) *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "libmisc.h"

class CSetClipboardTextW
{
public:
	CSetClipboardTextW()
	{
		m_hWnd = NULL;
		m_hgbl = NULL;
		m_dwPutOffset = 0;
	}

	~CSetClipboardTextW()
	{
	}

	BOOL Start(HWND hWnd,DWORD dwSize)
	{
		m_hWnd = hWnd;

		if( OpenClipboard(hWnd) )
		{
			EmptyClipboard();

			m_hgbl = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,dwSize);
			m_dwPutOffset = 0;
			return TRUE;
		}

		return FALSE;
	}

	void End()
	{
		// After a memory object is placed on the clipboard, ownership of that memory handle is 
		// transferred to the system. When the clipboard is emptied and the memory object has 
		// one of the following clipboard formats, the system frees the memory object by calling 
		// the specified function: 
		//
		// Function to free object  Clipboard format 
		// ------------------------+-----------------------
		// GlobalFree	             CF_UNICODETEXT
#ifdef _DEBUG
		PWSTR pTest = (PWSTR)GlobalLock(m_hgbl);
		GlobalUnlock(m_hgbl);
#endif

		SetClipboardData(CF_UNICODETEXT,m_hgbl);

		CloseClipboard();

		m_hgbl = NULL;
	}

	DWORD SetText(PCWSTR pszText,BOOL bNewLine = TRUE)
	{
		DWORD  cbTextLength;
		SIZE_T cbBufferSize;
		PTSTR psz;

		cbTextLength = lstrlenW(pszText);
		if(bNewLine)
			cbTextLength += 2;
		cbTextLength *= sizeof(WCHAR);

		cbBufferSize = GlobalSize(m_hgbl);

		m_hgbl = GlobalReAlloc(m_hgbl,
					cbTextLength + cbBufferSize,
					GMEM_MOVEABLE|GMEM_ZEROINIT);

		psz = (LPTSTR)GlobalLock(m_hgbl);

		psz += (m_dwPutOffset/sizeof(WCHAR));

		lstrcatW(psz,pszText);
		if(bNewLine)
			lstrcatW(psz,L"\r\n");

		m_dwPutOffset += (cbTextLength/sizeof(WCHAR));

		GlobalUnlock(m_hgbl);

		return 0;
	}
private:
	HWND m_hWnd;
	HGLOBAL m_hgbl;
	DWORD m_dwPutOffset;
};

LONG
WINAPI
SetClipboardTextFromListViewColumn(
	HWND hwndLV,
	UINT uFormat,
	int iColumn
	)
{
	int   cItem;
	int   cColItem;
	int   i,iCol;
	int   cPutCount;
	HWND  hwndHD;
	PWSTR pszSepFmt;
	PWSTR pszTemp;

	// Get item count form list-view
	cItem = ListView_GetItemCount(hwndLV);

	// Get header column count
	hwndHD = GetDlgItem(hwndLV,0);
	cColItem = Header_GetItemCount(hwndHD);

	CSetClipboardTextW clip;
	clip.Start(hwndLV,sizeof(WCHAR));

	if(	uFormat & SCTEXT_FORMAT_CSV )
		pszSepFmt = L",";
	else if( uFormat & SCTEXT_FORMAT_TSV )
		pszSepFmt = L"\t";
	else
		pszSepFmt = L" ";

	int cchBuffer = 1024;
	pszTemp = (PTSTR)_MemAlloc( cchBuffer * sizeof(WCHAR) );
	if( pszTemp == NULL )
	{
		SetLastError( ERROR_NOT_ENOUGH_MEMORY );
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	for( i = 0, cPutCount = 0; i < cItem; i++ )
	{
		if( uFormat & SCTEXT_FORMAT_SELECTONLY )
		{
			UINT state;
			state = ListView_GetItemState(hwndLV,i,LVIS_SELECTED);
			if( state == 0 )
				continue;
		}			

		if( cPutCount > 0 ) 
			clip.SetText(L"", TRUE);

		cPutCount++;

		if( iColumn != -1 )
		{
			// Specified column	put out
			iCol = iColumn;
			cColItem = iColumn+1;
		}
		else
		{
			// All columns put out
			iCol = 0;
		}

		for(/* no init */; iCol < cColItem; iCol++)
		{
			pszTemp[0] = 0;

			LVITEM lvi = {0};
			BOOL bLoop = TRUE;
			do
			{
				int cch;
				lvi.pszText = pszTemp;
				lvi.cchTextMax = cchBuffer;
				lvi.iSubItem = iCol;
				cch = (int)SendMessage(hwndLV,LVM_GETITEMTEXT,(WPARAM)(int)i,(LPARAM)&lvi);

				// Hack for IE4
				if( (cch != 0) && (pszTemp[0] == 0) && (lvi.pszText != pszTemp) )
					lstrcpynW(pszTemp,lvi.pszText,cchBuffer);

				if( cch >= cchBuffer )
				{
					cchBuffer = cch + 1;
					pszTemp = (PWSTR)_MemReAlloc( pszTemp, cchBuffer * sizeof(WCHAR) );
					if( pszTemp == NULL )
					{
						SetLastError( ERROR_NOT_ENOUGH_MEMORY );
						clip.End();
						return ERROR_NOT_ENOUGH_MEMORY;
					}
				}
				else
				{
					bLoop = FALSE; // loop out
				}
			}
			while( bLoop );

			if( pszTemp[0] != 0 )
			{
				if( uFormat & SCTEXT_FORMAT_DOUBLEQUATE )
				{
					clip.SetText(_T("\""),FALSE);
					clip.SetText(pszTemp,FALSE);
					clip.SetText(_T("\""),FALSE);
				}
				else
					clip.SetText(pszTemp,FALSE);

				if( iColumn == -1 )
				{
					if( iCol != (cColItem-1) )
						clip.SetText(pszSepFmt,FALSE);
				}
			}
		}
	}

	_MemFree( pszTemp );

	if( cPutCount > 1 )
		clip.SetText(L"",TRUE);

	clip.End();

	SetLastError( ERROR_SUCCESS );
	return ERROR_SUCCESS;
}

LONG
WINAPI
SetClipboardTextFromListView(
	HWND hwndLV,
	ULONG Flags
	)
{
	int cItems;

	// Get item count
	//
	cItems = ListView_GetItemCount(hwndLV);
	if( cItems == 0 )
	{
		return 0;
	}

	//
	// Alloc the global memory for string.
	//
	HGLOBAL hGlobal;
	LPTSTR pString;
	int cchString = 0;
	hGlobal = GlobalAlloc(GMEM_MOVEABLE,sizeof(WCHAR));
	if( hGlobal == NULL )
	{
		return 0;
	}

	//
	// Get the row count.
	//
	HWND hwndHD;
	hwndHD = ListView_GetHeader(hwndLV);
	int cColumns;
	cColumns = Header_GetItemCount(hwndHD);

	//
	// Get text each columns
	//
	WCHAR sz[MAX_PATH];
	int i,j;
	for(i = 0; i < cItems; i++)
	{
		UINT State = ListView_GetItemState(hwndLV,i,LVIS_SELECTED);

		if( State )
		{
			int cch = 0;

			for(j = 0 ; j < cColumns; j++)
			{
				LVITEM lvi;
				lvi.iSubItem = j;
				lvi.pszText = sz;
				lvi.cchTextMax = MAX_PATH;
				cch += (int)SendMessage(hwndLV,LVM_GETITEMTEXT,i,(LPARAM)&lvi);
				cch += 1; // for separator character
			}
			cch+=2; // for newline    '\r\n'
			cch++;  // for terminator '\0'

			hGlobal = GlobalReAlloc(hGlobal,((cchString + cch) * sizeof(WCHAR)),GMEM_MOVEABLE|GMEM_ZEROINIT);

			pString = (LPTSTR)GlobalLock(hGlobal);

			pString += cchString;

			for(j = 0 ; j < cColumns; j++)
			{
				LVITEM lvi;
				lvi.iSubItem = j;
				lvi.pszText = sz;
				lvi.cchTextMax = MAX_PATH;
				cch = (int)SendMessage(hwndLV,LVM_GETITEMTEXT,i,(LPARAM)&lvi);

				memcpy(pString,sz,cch*sizeof(WCHAR));
				pString += cch;

				if( j < cColumns - 1 )
				{
					if( Flags & SEPCHAR_TAB )
						*pString++ = L'\t';
					else if( Flags & SEPCHAR_COMMA )
						*pString++ = L',';
					else // SEPCHAR_SPACE
						*pString++ = L' ';
					cch++;
				}

				cchString += cch;
			}

			*pString++ = L'\r';
			cchString++;
			*pString++ = L'\n';
			cchString++;

			GlobalUnlock(hGlobal);
		}
	}

#ifdef _DEBUG
	pString = (LPTSTR)GlobalLock(hGlobal);

	GlobalUnlock(hGlobal);
#endif

	if( OpenClipboard(hwndLV) )
	{
		EmptyClipboard();

		SetClipboardData(CF_UNICODETEXT,hGlobal);

		CloseClipboard();	
	}

	return 0;
}

LONG
WINAPI
SetClipboardText(
	HWND hwndCilpboardOwner,
	PVOID pszCopyString,
	ULONG CodeType
	)
{
	SIZE_T cb;
	if( CodeType == SCTEXT_UNICODE )
		cb = (wcslen((PCWSTR)pszCopyString) + 1) * sizeof(WCHAR);
	else
		cb = strlen((char*)pszCopyString) + 1;

	// Alloc the global memory for string.
	//
	HGLOBAL hGlobal;
	int cchString = 0;
	hGlobal = GlobalAlloc(GMEM_MOVEABLE,cb);
	if( hGlobal == NULL )
	{
		return GetLastError();
	}

	PVOID pString = GlobalLock(hGlobal);

	memcpy(pString,pszCopyString,cb);

	GlobalUnlock(hGlobal);

	if( OpenClipboard(hwndCilpboardOwner) )
	{
		EmptyClipboard();

		if( CodeType == SCTEXT_UNICODE )
			SetClipboardData(CF_UNICODETEXT,hGlobal);
		else
			SetClipboardData(CF_TEXT,hGlobal);

		CloseClipboard();	
	}

	return 0;
}
