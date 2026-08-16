//****************************************************************************
//*                                                                          *
//*  subclassproc.cpp                                                        *
//*                                                                          *
//*  PURPOSE:                                                                *
//*                                                                          *
//*  AUTHOR:  Yamashita Katsuhiro                                            *
//*                                                                          *
//*  HISTORY: 2026-03-28 Createc                                             *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
// 
#include "stdafx.h"
#include "subclassproc.h"

/*++

  NOTE:
  These input sub-procs, no parse and validation on the string being pasted 
  from clipboard.

--*/

inline BOOL IsControlKey(INT ch,DWORD_PTR dwRefData)
{
	switch( ch )
	{
		case 0x03: // Ctrl+C
		case 0x16: // Ctrl+V
		case 0x08: // VK_BACK
			return TRUE;
	}

	if( (dwRefData & EBSPF_ACCEPT_SPACE) && ch == ' ' )
		return TRUE;

	return FALSE;
}

LRESULT
CALLBACK
EditBox_DecInputSubclassProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
	)
{
	switch( uMsg )
	{
		case WM_CHAR:
		{
			INT ch = (INT)wParam;

			if( (ch & ~0xff) != 0 )
				return 0;

			if( isdigit(ch) )
				break;

			if( IsControlKey(ch,dwRefData) )
				break;

			return 0;
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT
CALLBACK
EditBox_HexInputSubclassProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
	)
{
	switch( uMsg )
	{
		case WM_CHAR:
		{
			INT ch = (INT)wParam;

			if( (ch & ~0xff) != 0 )
				return 0;

			if( isxdigit(ch) )
				break;

			if( IsControlKey(ch,dwRefData) )
				break;

			if( (dwRefData & EBSPF_ACCEPT_HEX_PREFIX_CHAR) && (ch == 'x' || ch == 'X') )
			{
				int cch = GetWindowTextLength(hWnd);
				PWSTR pszTestText = _MemAllocStringBuffer(cch+1);
				if( pszTestText )
				{
					GetWindowText(hWnd,pszTestText,cch+1);

					if( pszTestText[cch-1] == L'0' )
					{
						_MemFree(pszTestText);
						break;
					}
				
					DWORD dw = Edit_GetSel(hWnd);
					if( dw != (INT)-1 )
					{
						WORD pos = LOWORD(dw);
						if( pos > 0 )
						{
							if( pszTestText[pos-1] == L'0' )
							{
								_MemFree(pszTestText);
								break;
							}
						}
					}
	
					_MemFree(pszTestText);
				}
			}

			return 0;
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT
CALLBACK
EditBox_BinInputSubclassProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
	)
{
	switch( uMsg )
	{
		case WM_CHAR:
		{
			INT ch = (INT)wParam;

			if( (ch & ~0xff) != 0 )
				return 0;

			if( ch == '0' || ch == '1' )
				break;

			if( IsControlKey(ch,dwRefData) )
				break;

			return 0;
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// Acceptable input formats.
// "123456"
// "0x123456"
// "+123456"
// "-123456"

static BOOL _checkText(PCWSTR psz,int radix=-1)
{
	if( radix == -1 ) {
		if( wcsnicmp(psz,L"0x",2) == 0 )
			radix = 16,psz+=2;
		else if( psz[0] == L'+' ||  psz[0] == L'-' )
			radix = 10,psz++;
		else
			radix = 10;
	}

	if( *psz == L'\0' )
		return false;

	while( *psz )
	{
		if( radix == 16 ) {
			if( !iswxdigit( *psz ) )
				return false;
		}
		else if( radix == 10 ) {
			if( !iswdigit( *psz ) )
				return false;
		}
		else
		{
			return false;
		}
		psz++;
	}

	return true;
}

LRESULT
CALLBACK
EditBox_NumberInputSubclassProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
	)
{
	switch( uMsg )
	{
		case WM_PASTE:
		{
			PWSTR pszPasteText = NULL;
	        if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) 
		        return 0; 
			if (!OpenClipboard(hWnd)) 
				return 0; 
 
			HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT); 
			if (hglb != NULL) 
			{ 
				LPWSTR lptstr = (LPWSTR)GlobalLock(hglb); 
				if (lptstr != NULL) 
				{ 
					pszPasteText = _MemAllocString(lptstr);
					GlobalUnlock(hglb); 
		        } 
			} 
			CloseClipboard(); 

			if( pszPasteText == NULL )
				return 0;

			BOOL bAccept = FALSE;
			int cch = GetWindowTextLength(hWnd);
			if( cch == 0 )
			{
				if( !_checkText(pszPasteText) )
				{
					_SafeMemFree(pszPasteText);
					return 0;
				}
			}
			else
			{
				PWSTR pszTestText = _MemAllocStringBuffer(cch+1);
				if( pszTestText )
				{
					int radix = -1;
					GetWindowText(hWnd,pszTestText,cch+1);

					if( pszTestText[0] == L'+' || pszTestText[0] == L'-' )
					{
						radix = 10;
					}
					else if( wcsnicmp(pszTestText,L"0x",2) == 0 )
					{
						radix = 16;
					}
					else
					{
						radix = 10; // default
					}
					_MemFree(pszTestText);

					if( !_checkText(pszPasteText,radix) )
					{
						_SafeMemFree(pszPasteText);
						return 0;
					}
				}
				else
				{
					_SafeMemFree(pszPasteText);
					return 0;
				}
			}
			_SafeMemFree(pszPasteText);
			break;
		}
		case WM_CHAR:
		{
			INT ch = (INT)wParam;

			if( (ch & ~0xff) != 0 )
				return 0;

			if( isdigit(ch) )
				break;

			if( IsControlKey(ch,dwRefData) )
				break;

			{
				BOOL bAccept = FALSE;
				int cch = GetWindowTextLength(hWnd);
				PWSTR pszTestText = _MemAllocStringBuffer(cch+1);
				if( pszTestText )
				{
					GetWindowText(hWnd,pszTestText,cch+1);

					if( pszTestText[0] == L'\0' && (ch == L'+' || ch == L'-') )
					{
						bAccept = TRUE;
					}
					else if( wcscmp(pszTestText,L"0") == 0 && ch == L'x' )
					{
						bAccept = TRUE;
					}
					else if( memcmp(pszTestText,L"0x",4) == 0 )
					{
						if( isxdigit(ch) )
							bAccept = TRUE;
					}

					_MemFree(pszTestText);

					if( bAccept )
						break;
				}
			}
			return 0;
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
