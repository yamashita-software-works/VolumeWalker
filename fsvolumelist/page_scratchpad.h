#pragma once
//*****************************************************************************
//*                                                                           *
//*  page_scratchpad.h                                                        *
//*                                                                           *
//*  Scratch Pad Page                                                         *
//*                                                                           *
//*  Author:  YAMASHITA Katsuhiro                                             *
//*                                                                           *
//*  History: 2025-10-18 Created.                                             *
//*                                                                           *
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "common.h"
#include "pagewbdbase.h"
#include <Richedit.h>

///////////////////////////////////////////////////////////////////////////////

inline 
HWND CreateRichEdit(
    HWND hwndOwner,        // Dialog box handle.
    int x, int y,          // Location.
    int width, int height, // Dimensions.
    HINSTANCE hinst)       // Application or DLL instance.
{
    LoadLibrary(TEXT("Msftedit.dll"));
    HWND hwndEdit= CreateWindowEx(0, MSFTEDIT_CLASS, TEXT(""),
        ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
		WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP, 
        x, y, width, height, 
        hwndOwner, NULL, hinst, NULL);
    return hwndEdit;
}

inline
DWORD GetOption(PCWSTR pszSectionName,PCWSTR pwszKeyName,PWSTR wszBuffer,DWORD cchBuffer)
{
	WCHAR szIniFile[MAX_PATH];
	GetModuleFileName(0,szIniFile,MAX_PATH);

	PathRemoveExtension (szIniFile);
	PathAddExtension(szIniFile,L".ini");

	DWORD cch;
	cch = GetPrivateProfileString(pszSectionName,pwszKeyName,L"",wszBuffer,cchBuffer,szIniFile);

	return cch;
}

inline
BOOL GetScratchPadDocumentPath(PWSTR pszPath,DWORD cchPath)
{
	if( GetTempPath(cchPath,pszPath) == 0 )
		return FALSE;
	return (BOOL)PathCombine(pszPath,pszPath,L"volumewalker.{73e4bcf2-c313-45e1-b3eb-67bbb5afa0d1}.tmp");
}

inline
BOOL LoadScratchPadDocument(PWSTR *pszText)
{
	BOOL bSuccess = FALSE;
	DWORD dwError = 0;

	WCHAR szTemp[MAX_PATH];
	if( !GetScratchPadDocumentPath(szTemp,MAX_PATH) )
		return FALSE;

	HANDLE hFile;
	hFile = CreateFile(szTemp,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if( hFile == INVALID_HANDLE_VALUE )
	{
		return FALSE;
	}

	DWORD cb;
	cb = GetFileSize(hFile,NULL);

	if( cb > 0 )
	{
		PWSTR pBuffer;
		pBuffer = (PWSTR)CoTaskMemAlloc(cb+sizeof(WCHAR));

		if( pBuffer )
		{
			ZeroMemory(pBuffer,cb+sizeof(WCHAR));

			DWORD cbRead;
			if( ReadFile(hFile,pBuffer,cb,&cbRead,NULL) )
			{
				*pszText = pBuffer;
				bSuccess = TRUE;
			}
			dwError = GetLastError();
		}
		else
		{
			dwError = ERROR_OUTOFMEMORY;
		}
	}
	else
	{
		dwError = ERROR_NO_DATA;
	}

	if( !CloseHandle(hFile) )
		dwError = GetLastError();

	SetLastError(dwError);

	return bSuccess;
}

inline
BOOL SaveScratchPadDocument(PWSTR pszText,DWORD cbText)
{
	BOOL bSuccess = FALSE;
	DWORD dwError = 0;

	WCHAR szTemp[MAX_PATH];
	if( !GetScratchPadDocumentPath(szTemp,MAX_PATH) )
		return FALSE;

	HANDLE hFile;
	hFile = CreateFile(szTemp,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if( hFile == INVALID_HANDLE_VALUE )
	{
		return FALSE;
	}

	DWORD cbWriten;
	if( WriteFile(hFile,pszText,cbText,&cbWriten,NULL) )
	{
		bSuccess = TRUE;
	}
	else
	{
		dwError = GetLastError();
	}

	if( !CloseHandle(hFile) )
		dwError = GetLastError();

	SetLastError(dwError);

	return bSuccess;
}

inline
int parse_get_string_token(wchar_t *string,wchar_t *seps,wchar_t **token_ptr_array[],int initial_count)
{
	wchar_t *buffer;
	wchar_t *token = nullptr;
	wchar_t *next_token = nullptr;
	wchar_t **ptr_array;
	int i = 0;
	int count = 0;

	buffer = wcsdup(string);
	if( buffer == nullptr )
		return -1;

    token = wcstok_s(buffer, seps, &next_token);
    while( token != nullptr )
    {
		count++;
		token = wcstok_s(nullptr, seps, &next_token);
    }

	if( count > 0 ) 
	{
		count = max(initial_count,count);
		ptr_array = (wchar_t **)malloc(count * sizeof(wchar_t *));
		if( ptr_array != nullptr )
		{
			memset(ptr_array,0,count * sizeof(wchar_t *));
			wcscpy(buffer,string);
			token = wcstok_s(buffer, seps, &next_token);
			while( token != nullptr )
			{
				ptr_array[i++] = wcsdup(token);
				token = wcstok_s(nullptr, seps, &next_token);
			}
			*token_ptr_array = ptr_array;
		}
		else
		{
			count = -1;
		}
	}

	free(buffer);

	return count;
}

inline
int parse_free_string_token(wchar_t *token_ptr_array[],int token_ptr_array_count)
{
	int i;
	for(i = 0; i < token_ptr_array_count; i++)
	{
		if( token_ptr_array[i] )
			free(token_ptr_array[i]);
	}
	free(token_ptr_array);
	return 0;
}

inline
BOOL
GetScratchPadFont(
	LOGFONT& lf
	)
{
	WCHAR wszBuffer[MAX_PATH];

	if( GetOption(L"ScratchPad",L"Font",wszBuffer,ARRAYSIZE(wszBuffer)) == 0 )
	{
		return FALSE;
	}

	PWSTR *TokenArray;
	int cTokenArray;
	cTokenArray = parse_get_string_token(wszBuffer,L",",&TokenArray,3);

	if( cTokenArray > 0 )
	{
		int iPointSize = 0;

		if( TokenArray[0] )
		{
			StringCchCopy(lf.lfFaceName,LF_FACESIZE,TokenArray[0]);
		}

		if( TokenArray[1] )
		{
			iPointSize = _wtoi(TokenArray[1]);
			HDC hdc = GetWindowDC(NULL);
			lf.lfHeight = -MulDiv(iPointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			ReleaseDC(NULL,hdc);
		}

		if( TokenArray[2] )
		{
			lf.lfCharSet = (BYTE)_wtoi(TokenArray[2]);
		}

#if 0
		WCHAR sz[256];
		StringCchPrintf(sz,ARRAYSIZE(sz),L"%d : facename=%s,point=%d,height=%d,charset=%d",cPtrArray,lf.lfFaceName,iPointSize,lf.lfHeight,lf.lfCharSet);
		MessageBox(NULL,sz,L"",MB_OK);
#endif

		parse_free_string_token(TokenArray,cTokenArray);
	}


	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

class CScratchPadPage :	public CPageWndBase
{
	HWND m_hWndEdit;
	HFONT m_hFont;

public:
	CScratchPadPage()
	{
	}

	~CScratchPadPage()
	{
	}

	virtual HRESULT OnInitPage(PVOID ptr,DWORD,PVOID)
	{
		SELECT_ITEM *SelectItem = (SELECT_ITEM *)ptr;

		m_hWndEdit = CreateRichEdit(m_hWnd,0,0,0,0,GetModuleHandle(NULL));

		LOGFONT lf = {0};
		if( !GetScratchPadFont(lf) )
		{
			HDC hdc = GetWindowDC(m_hWnd);
			lf.lfHeight = -MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			ReleaseDC(m_hWnd,hdc);
			lf.lfCharSet = DEFAULT_CHARSET;
			StringCchCopy(lf.lfFaceName,_countof(lf.lfFaceName),L"Consolas");
		}
		m_hFont = CreateFontIndirect( &lf );

		SendMessage(m_hWndEdit,WM_SETFONT,(WPARAM)m_hFont,0);

		PWSTR pszDocText=NULL;
		if( LoadScratchPadDocument(&pszDocText) )
		{
			PWSTR pText = pszDocText;

			// Assumed UNICODE16 only with BOM.
			// Otherwise the behavior is undefined.
			WORD bom = *(WORD*)pText;
			if( bom == 0xfeff || bom == 0xfffe )
				pText += 1;

			SendMessage(m_hWndEdit,WM_SETTEXT,0,(LPARAM)pText);

			CoTaskMemFree(pszDocText);
		}
	
		return S_OK;
	}

	virtual HRESULT OnInitLayout(const RECT *prc)
	{
		UpdateLayout(_RECT_WIDTH((*prc)),_RECT_HIGHT((*prc)));
		return E_NOTIMPL;
	}

	virtual HRESULT OnDestroyPage(PVOID)
	{
		return E_NOTIMPL;
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		SaveDocument();

		if( m_hFont ) {
			DeleteObject(m_hFont);
			m_hFont = NULL;
		}

		return 0;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}

	LRESULT OnQueryMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case UI_QUERY_SELECTEDITEM:
			{
				break;
			}
		}
		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;

		switch( pnmhdr->code )
		{
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
		}
		return 0;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		pnmhdr->hwndFrom = m_hWnd;
		pnmhdr->idFrom = GetWindowLong(m_hWnd,GWL_ID);
		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)pnmhdr);
		return 0;
	}

	VOID UpdateLayout(int cx,int cy)
	{
		if( m_hWndEdit )
		{
			SetWindowPos(m_hWndEdit,NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOMOVE);
		}
	}

	HRESULT FillItems(SELECT_ITEM *)
	{
		CWaitCursor wait;
		return S_OK;
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		return FillItems((SELECT_ITEM*)pFile);
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				SetFocus(m_hWndEdit);
				return 0;	
			case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	//
	// Commaand Handling
	//
	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				*State = UPDUI_ENABLED;
				return S_OK;
		}
		return S_FALSE;
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				OnCmdEditCopy();
				break;
			case ID_VIEW_REFRESH:
				OnCmdRefresh();
				break;
		}
		return S_OK;
	}

	void OnCmdEditCopy()
	{
		SendMessage(m_hWndEdit,WM_COPY,0,0);
	}

	void OnCmdRefresh()
	{
		SELECT_ITEM sel = {0};
		FillItems(&sel);
	}

	VOID SaveDocument()
	{
		int cbBuffer;

		cbBuffer = GetWindowTextLength(m_hWndEdit);
		if( cbBuffer > 0 )
		{
			cbBuffer *= sizeof(WCHAR);

			PWSTR pszBuffer;
			pszBuffer = (PWSTR)_MemAllocZero( cbBuffer + sizeof(WCHAR) + sizeof(WCHAR) );

			pszBuffer[0] = 0xfeff; // unicode BOM-LE
			GetWindowText(m_hWndEdit,&pszBuffer[1],cbBuffer);

			SaveScratchPadDocument(pszBuffer,cbBuffer+sizeof(WCHAR)); // data+bom

			_MemFree(pszBuffer);
		}
	}
};
