//****************************************************************************
//
//  inifile.cpp
//
//  Implements INI file read/write functions.
//
//  Author: YAMASHITA Katsuhiro
//
//  Create: 2023.06.29
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "volumewalker.h"
#include "inifile.h"

#define _LPWSTR_SECTION_LAYOUT    L"Layout"
#define _LPWSTR_SECTION_MDILAYOUT L"MDILayout"
#define _LPWSTR_HEADER            L"; *** do not modify this file ***\r\n[" _LPWSTR_SECTION_LAYOUT L"]\r\n"

static PWSTR g_pszIniFile = NULL;
static PWSTR g_pszConfigFile = NULL;

PCWSTR _GetIniFileName()
{
	return g_pszIniFile;
}

VOID _SetIniFileName(PCWSTR pszIniFile)
{
	_SafeMemFree(g_pszIniFile);
	if( pszIniFile )
		g_pszIniFile = _MemAllocString(pszIniFile);
}

PCWSTR _GetConfigFileName()
{
	return g_pszConfigFile;
}

VOID _SetConfigFileName(PCWSTR pszConfigFile)
{
	_SafeMemFree(g_pszConfigFile);
	if( pszConfigFile )
		g_pszConfigFile = _MemAllocString(pszConfigFile);
}

BOOL HasValidIniFile()
{
	if( !PathFileExists(_GetIniFileName()) )
	{
		SetLastError(ERROR_FILE_NOT_FOUND);
		return FALSE;
	}

	DWORD dw;
	WCHAR buf[100];
	dw = GetPrivateProfileString(L"Layout",L"Window",L"",buf,ARRAYSIZE(buf),_GetIniFileName());

	return (dw != 0);
}

BOOL CreateUnicodeIniFile(PCWSTR pszIniFileName,PCWSTR ContentsString)
{
	if( PathFileExists(pszIniFileName) )
	{
		SetLastError(ERROR_FILE_EXISTS);
		return FALSE;
	}

	BOOL bSuccess = FALSE;
	HANDLE hFile;
	hFile = CreateFile(pszIniFileName,GENERIC_WRITE|SYNCHRONIZE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
	if( hFile != INVALID_HANDLE_VALUE )
	{
		WCHAR buf[] = { 0xfeff }; // UNICODE BOM
		DWORD cb;
		bSuccess = WriteFile(hFile,buf,2,&cb,NULL);
		DWORD dwError = GetLastError();

		if( bSuccess && ContentsString )
		{
			bSuccess = WriteFile(hFile,ContentsString,(DWORD)(wcslen(ContentsString)*sizeof(WCHAR)),&cb,NULL);
			dwError = GetLastError();
		}

		CloseHandle(hFile);
		SetLastError(dwError);
	}
	return bSuccess;
}

static BOOL _IsAllCharsNum(PCWSTR psz)
{
	if( *psz == L'-' )
		psz++;
	while( *psz )
	{
		if( !(L'0' <= *psz && *psz <= L'9') )
			return FALSE;
		psz++;
	}
	return TRUE;
}

int GetWindowSizeFromTextLine(PWSTR pszLine,POINT *pPoint,SIZE *pSize,PWSTR *ppszTrail)
{
	wchar_t *next_token;
	wchar_t *seps = L",";
	wchar_t *ch_x,*ch_y,*ch_cx,*ch_cy,*ch_trail;

	PWSTR pszString = _MemAllocString(pszLine);

	// x
    ch_x = wcstok_s(pszString, seps, &next_token);

	// y
	ch_y = wcstok_s(NULL, seps, &next_token);

	// cx
	ch_cx = wcstok_s(NULL, seps, &next_token);

	// cy
	ch_cy = wcstok_s(NULL, seps, &next_token);

	// trail
	ch_trail = wcstok_s(NULL, seps, &next_token);

	if( ch_x && ch_y && ch_cx && ch_cy )
	{
		WCHAR *pszTrimChars = L" \t\n\r";
		StrTrim(ch_x,pszTrimChars);
		StrTrim(ch_y,pszTrimChars);
		StrTrim(ch_cx,pszTrimChars);
		StrTrim(ch_cy,pszTrimChars);
		
		if( _IsAllCharsNum(ch_x) && _IsAllCharsNum(ch_y) 
			&& _IsAllCharsNum(ch_cx) & _IsAllCharsNum(ch_cy) )
		{
			if( pPoint )
			{
				pPoint->x = _wtoi(ch_x);
				pPoint->y = _wtoi(ch_y);
			}

			if( pSize )
			{
				pSize->cx = _wtoi(ch_cx);
				pSize->cy = _wtoi(ch_cy);
			}
		}

		if( ch_trail && ppszTrail )
		{
			*ppszTrail = &pszLine[ ch_trail - pszString ];
		}
	}

	_MemFree(pszString);

	return 0;
}

int GetSectionInt(PWSTR pszSection,PWSTR pszEntry,int defNum)
{
	return (INT)GetPrivateProfileInt(pszSection,pszEntry,defNum,g_pszIniFile);
}

int WriteSectionInt(PWSTR pszSection,PCWSTR pszEntry,int val)
{
	WCHAR sz[32];
	StringCchPrintf(sz,ARRAYSIZE(sz),L"%d",val);
	return WritePrivateProfileString(pszSection,pszEntry,sz,g_pszIniFile);
}

int WriteSectionString(PCWSTR pszSection,PCWSTR pszEntry,PWSTR psz)
{
	return WritePrivateProfileString(pszSection,pszEntry,psz,g_pszIniFile);
}

int WriteSectionBuffer(PCWSTR pszSection,PCWSTR pszBuffer)
{
	return WritePrivateProfileSection(pszSection,pszBuffer,g_pszIniFile);
}

DWORD GetPrivateProfileString(PCWSTR pszSection,PCWSTR pszEntry,PWSTR psz,DWORD cch)
{
	return GetPrivateProfileString(pszSection,pszEntry,NULL,psz,cch,g_pszIniFile);
}

//////////////////////////////////////////////////////////////////////////////

static BOOL GetSectionInfo(PWSTR pszSection,MDICHILDFRAMEINIT_DIRFILES *pmdidoc)
{
	DWORD dwRet;
	WCHAR buf[MAX_PATH];

	dwRet = GetPrivateProfileString(pszSection,L"Window",NULL,buf,ARRAYSIZE(buf),g_pszIniFile);
	if( dwRet == 0 )
	{
		return FALSE;
	}

	GetWindowSizeFromTextLine(buf,&pmdidoc->hdr.pt,&pmdidoc->hdr.size,NULL);

	pmdidoc->hdr.show = GetSectionInt(pszSection,L"Show",SW_SHOW);

	if( GetPrivateProfileString(pszSection,L"Offset",NULL,buf,ARRAYSIZE(buf),g_pszIniFile) > 0 )
	{
		pmdidoc->liStartOffset.QuadPart = _wcstoui64(buf,NULL,16);
	}

	DWORD cch = 32768 + MAX_PATH;
	PWSTR psz = new WCHAR[cch];

	if( psz != NULL )
	{
		if( GetPrivateProfileString(pszSection,L"Volume",NULL,psz,cch,g_pszIniFile) > 0 )
		{
			pmdidoc->Path = _MemAllocString( psz );
		}

		delete[] psz;
	}

	return TRUE;
}

static BOOL WriteSectionInfo(PWSTR pszSection,HWND hwndMDIChildFrame,HWND hwndView,GUID& wndGuid,MDICHILDFRAMEINIT_DIRFILES *pmdidoc)
{
	int cch = 32768 + MAX_PATH;
	WCHAR *sz = new WCHAR[ cch ];

	StringCchPrintf(sz,cch,L"%d,%d,%d,%d",
		pmdidoc->hdr.pt.x,pmdidoc->hdr.pt.y,
		pmdidoc->hdr.size.cx,pmdidoc->hdr.size.cy);
	WriteSectionString(pszSection,L"Window",sz);

	StringCchPrintf(sz,cch,L"%d",pmdidoc->hdr.show);
	WriteSectionString(pszSection,L"Show",sz);

	if( wndGuid.Data4[0] != 0 || wndGuid.Data2 == 0x1 )
	{
		switch( wndGuid.Data1 )
		{
			case VOLUME_CONSOLE_SIMPLEHEXDUMP:
			case VOLUME_CONSOLE_CONTENT_FILES:
			{
				WQ_PARAM wqp = {0};
				wqp.dwLength = cch;
				wqp.VolumePath = sz;
				SendMessage(hwndView,WM_QUERY_MESSAGE,WQ_GETVOLUMEPATH,(LPARAM)&wqp);
				break;
			}
			default:
			{
				GetWindowText(hwndMDIChildFrame,sz,cch);
			}
		}
		WriteSectionString(pszSection,L"Volume",sz);
	}

	delete[] sz;

	if( wndGuid.Data1 == VOLUME_CONSOLE_SIMPLEHEXDUMP )
	{
		WQ_PARAM wqp = {0};
		wqp.dwLength = sizeof(LARGE_INTEGER);
		wqp.liValue.QuadPart = 0;
		SendMessage(hwndView,WM_QUERY_MESSAGE,WQ_GETSTARTOFFSET,(LPARAM)&wqp);
		StringCchPrintf(sz,cch,L"0x%I64x",wqp.liValue.QuadPart);
		WriteSectionString(pszSection,L"Offset",sz);
	}

	return TRUE;
}

HRESULT LoadLayout(HWND hWnd,HWND hWndMDIClient,MDICHILDFRAMEINIT_DIRFILES **pFrames,DWORD *pdwFrames)
{
	if( !PathFileExists(g_pszIniFile) )
	{
		return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
	}

	DWORD dwRet;
	DWORD cch = 65536;
	WCHAR *psz = new WCHAR[cch];

	// The size of the buffer pointed to by the lpReturnedString parameter, 
	// in characters. The maximum profile section size is 32,767 characters.
	dwRet = GetPrivateProfileSection(_LPWSTR_SECTION_MDILAYOUT,psz,cch,g_pszIniFile);

	if( dwRet == 0 )
	{
		delete[] psz;
		return HRESULT_FROM_WIN32( ERROR_NO_DATA ); // not found and so on.
	}

	if( dwRet == (cch-2) )
	{
		delete[] psz;
		return HRESULT_FROM_WIN32( ERROR_INVALID_DATA ); // buffer overflow
	}

	RECT rcMDIClient;
	GetWindowRect(hWndMDIClient,&rcMDIClient);
	MapWindowPoints(GetDesktopWindow(),hWndMDIClient,(LPPOINT)&rcMDIClient,2);

	int cChildFrameCount = 0;
	WCHAR *pszLine;

	pszLine = psz;
	while( *pszLine )
	{
		cChildFrameCount++;
		pszLine = (pszLine + wcslen(pszLine) + 1);
	}

	MDICHILDFRAMEINIT_DIRFILES *pChildFrames = 
			(MDICHILDFRAMEINIT_DIRFILES *)_MemAllocZero(cChildFrameCount*sizeof(MDICHILDFRAMEINIT_DIRFILES));
	int i = 0;
	WCHAR szSection[100];
	WCHAR szGuid[38+1];
	PWSTR pSep;

	pszLine = psz;
	while( *pszLine )
	{
		pSep = StrChr(pszLine,L'=');

		if( pSep )
		{
			++pSep;
			StringCchPrintf(szGuid,_countof(szGuid),L"{%s}",pSep);
			GUIDFromString(szGuid,&pChildFrames[i].Guid);

			StringCchCopy(szSection,ARRAYSIZE(szSection),pSep);
			StrTrim(szSection,L" \t\r\n");

			if( GetSectionInfo(szSection,&pChildFrames[i]) )
			{
				RECT rcResult;
				RECT rcTest;
				rcTest.left   = pChildFrames[i].hdr.pt.x;
				rcTest.top    = pChildFrames[i].hdr.pt.y;
				rcTest.right  = rcTest.left + pChildFrames[i].hdr.size.cx;
				rcTest.bottom = rcTest.top + pChildFrames[i].hdr.size.cy;

				if( IntersectRect(&rcResult,&rcMDIClient,&rcTest) )
				{
					i++;
				}
			}
		}

		pszLine = (pszLine + wcslen(pszLine) + 1);
	}

	delete[] psz;

	*pdwFrames  = i;
	*pFrames = pChildFrames;

	return S_OK;
}

HRESULT FreeLayout(MDICHILDFRAMEINIT_DIRFILES *mdiFrames,DWORD dwFrames)
{
	for( DWORD i = 0; i < dwFrames; i++ )
	{
		_MemFree( mdiFrames[i].Path );
	}

	_MemFree( mdiFrames );

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////

static HRESULT RemoveLayoutSectionInfo()
{
	if( !PathFileExists(g_pszIniFile) )
	{
		return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
	}

	DWORD dwRet;
	DWORD cch = 65536;
	WCHAR *psz = new WCHAR[cch];

	// The size of the buffer pointed to by the lpReturnedString parameter, 
	// in characters. The maximum profile section size is 32,767 characters.
	dwRet = GetPrivateProfileSection(_LPWSTR_SECTION_MDILAYOUT,psz,cch,g_pszIniFile);

	if( dwRet == 0 )
	{
		delete[] psz;
		return HRESULT_FROM_WIN32( ERROR_NO_DATA ); // not found and so on.
	}

	if( dwRet == (cch-2) )
	{
		delete[] psz;
		return HRESULT_FROM_WIN32( ERROR_INVALID_DATA ); // buffer overflow
	}

	int cChildFrameCount = 0;
	WCHAR *pszLine;

	pszLine = psz;
	while( *pszLine )
	{
		cChildFrameCount++;
		pszLine = (pszLine + wcslen(pszLine) + 1);
	}

	int i = 0;
	WCHAR szSection[100];
	PWSTR pSep;

	pszLine = psz;
	while( *pszLine )
	{
		pSep = StrChr(pszLine,L'=');

		if( pSep )
		{
			// get section name
			StringCchCopy(szSection,ARRAYSIZE(szSection),++pSep);
			StrTrim(szSection,L" \t\r\n");

			// delete mdi page section
			WriteSectionString(szSection,NULL,NULL);

			// delete mdi page column section
			StringCchPrintf(szSection,ARRAYSIZE(szSection),L"%s.columns",szSection);
			WriteSectionString(szSection,NULL,NULL);
		}

		pszLine = (pszLine + wcslen(pszLine) + 1);
	}

	//
	// clear previous layout settings
	//
	WriteSectionBuffer(_LPWSTR_SECTION_MDILAYOUT,L"");

	delete[] psz;

	return S_OK;
}

static HRESULT SaveConfig(HWND hWnd)
{
	DWORD cch = 256;
	WCHAR *psz = new WCHAR[cch];

	WINDOWPLACEMENT wndpl = {0};
	wndpl.length = sizeof(wndpl);
	GetWindowPlacement(hWnd,&wndpl);

	RECT rc;
	rc = wndpl.rcNormalPosition;

	WCHAR sz[MAX_PATH];

	StringCchPrintf(sz,ARRAYSIZE(sz),L"%d,%d,%d,%d",rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top);
	WriteSectionString(_LPWSTR_SECTION_LAYOUT,L"Window",sz);

	StringCchPrintf(sz,ARRAYSIZE(sz),L"%d",wndpl.showCmd);
	WriteSectionString(_LPWSTR_SECTION_LAYOUT,L"Show",sz);

	delete[] psz;

	return S_OK;
}

static VOID SaveLayout(HWND hWnd,HWND hWndMDIClient)
{
	HWND hwnd;
	hwnd = GetWindow(hWndMDIClient,GW_CHILD);
	if( hwnd )
	{
		MDICHILDFRAMEINIT_DIRFILES mdidoc = {0};
		WCHAR szEntry[MAX_PATH];
		WCHAR szSection[MAX_PATH];

		int iIndex = 0;
		do
		{
			MDICHILDWNDDATA *pd = (MDICHILDWNDDATA *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
			if( pd )
			{
				RECT rc;
				WINDOWPLACEMENT wndpl = {0};
				wndpl.length = sizeof(wndpl);
				GetWindowPlacement(hwnd,&wndpl);

				rc = wndpl.rcNormalPosition;

				mdidoc.hdr.pt.x = rc.left;
				mdidoc.hdr.pt.y = rc.top;
				mdidoc.hdr.size.cx = (rc.right - rc.left);
				mdidoc.hdr.size.cy = (rc.bottom - rc.top);
				mdidoc.hdr.show = wndpl.showCmd;

				CONSOLE_VIEW_ID *pcv = _GET_CONSOLE_VIEW_ID(pd->hWndView);
				GUID Guid = pcv->wndGuid;
				StringFromGUID( &Guid, szSection, _countof(szSection) );
				GUIDStringRemoveBrackets(szSection);

				WriteSectionInfo(szSection,hwnd,pd->hWndView,pcv->wndGuid,&mdidoc);

				StringCchPrintf(szEntry,ARRAYSIZE(szEntry),L"%d",iIndex);
				WriteSectionString(_LPWSTR_SECTION_MDILAYOUT,szEntry,szSection);

				iIndex++;
			}
		} while( hwnd = GetWindow(hwnd,GW_HWNDNEXT) );
	}
}

//----------------------------------------------------------------------------
//
//  SaveMainFrameConfig()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
HRESULT SaveMainFrameConfig(HWND hWnd,HWND hWndMDIClient)
{
	PCWSTR pszIniFile = _GetIniFileName();
	if( !PathFileExists(pszIniFile) )
	{
		CreateUnicodeIniFile(pszIniFile,_LPWSTR_HEADER);
		WritePrivateProfileSection(_LPWSTR_SECTION_LAYOUT,L"",pszIniFile);
		WritePrivateProfileSection(_LPWSTR_SECTION_MDILAYOUT,L"",pszIniFile);
	}
	else
	{
		RemoveLayoutSectionInfo();
	}
	SaveLayout(hWnd,hWndMDIClient);
	SaveConfig(hWnd);
	return S_OK;
}

//----------------------------------------------------------------------------
//
//  LoadMainFrameConfig()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
HRESULT LoadMainFrameConfig(HWND hWnd,INT *pnCmdShow)
{
	DWORD dwRet;
	DWORD cch = 256;
	WCHAR *psz = new WCHAR[cch];

	dwRet = GetPrivateProfileString(_LPWSTR_SECTION_LAYOUT,L"Window",NULL,psz,cch,g_pszIniFile);

	if( dwRet == 0 )
	{
		delete psz;
		return S_FALSE; 
	}

	POINT pt;
	SIZE size;
	PWSTR pszTrail;
	GetWindowSizeFromTextLine(psz,&pt,&size,&pszTrail);

	delete psz;

	if( size.cx == 0 || size.cy == 0 )
		return E_FAIL;

	RECT rcMainFrame;
	rcMainFrame.left   = pt.x;
	rcMainFrame.top    = pt.y;
	rcMainFrame.right  = pt.x + size.cx;
	rcMainFrame.bottom = pt.y + size.cy;

	HMONITOR hMonitor;
	hMonitor = MonitorFromRect(&rcMainFrame,MONITOR_DEFAULTTONULL);

	if( hMonitor )
	{
		SetWindowPos(hWnd,NULL,pt.x,pt.y,size.cx,size.cy,SWP_NOZORDER);
	}

	if( pnCmdShow )
	{
		*pnCmdShow = GetPrivateProfileInt(_LPWSTR_SECTION_LAYOUT,L"Show",0,g_pszIniFile);
		if( !(1 <= *pnCmdShow && *pnCmdShow <= SW_MAX) )
		{
			*pnCmdShow = SW_SHOW;
		}
	}

	return S_OK;
}

#if 0
//----------------------------------------------------------------------------
//
//  WriteSectionRect()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
BOOL WriteSectionRect(PCWSTR pszSection,PCWSTR pszEntry,const RECT *prc)
{
	WCHAR sz[128];

	StringCchPrintf(sz,ARRAYSIZE(sz),L"%d,%d,%d,%d",
		prc->left, prc->top,
		prc->right - prc->left,
		prc->bottom - prc->top);

	return WriteSectionString(pszSection,pszEntry,sz);
}

//----------------------------------------------------------------------------
//
//  ReadSectionRect()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
BOOL ReadSectionRect(PCWSTR pszSection,PCWSTR pszEntry,RECT *prc)
{
	WCHAR sz[128];
	DWORD cch;

	cch = GetPrivateProfileString(pszSection,pszEntry,sz,ARRAYSIZE(sz));
	if( cch )
	{
		ZeroMemory(prc,sizeof(RECT));
		swscanf_s(sz,L"%d,%d,%d,%d",&prc->left,&prc->top,&prc->right,&prc->bottom);
		return TRUE;
	}
	return FALSE;
}
#endif

//
// simple determine macro
//
//  0         1         2         3     
//  012345678901234567890123456789012345678
// "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
// "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
//
#define IsNoBlacketIIDSectionName(p) (p[8]==L'-' && p[13]==L'-' && p[18]==L'-' && p[23]==L'-')

#define IsIIDSectionName(p) (p[9]==L'-' && p[14]==L'-' && p[19]==L'-' && p[24]==L'-')

//----------------------------------------------------------------------------
//
//  GetSectionNames()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
PWSTR GetSectionNames()
{
	DWORD cch;
	DWORD cchAlloc;
	PWSTR pszBuffer;

	cchAlloc = 10;

	for(int _tryAlloc = 0; _tryAlloc < 10; _tryAlloc++)
	{
		pszBuffer = _MemAllocStringBuffer( cchAlloc );

		if( pszBuffer == NULL )
			break;

		cch = GetPrivateProfileSectionNames(pszBuffer,cchAlloc,_GetIniFileName());

		if( cch == 0 )
			break;

		// If the buffer is not large enough to contain all the section names associated
		// with the specified initialization file, the return value is equal to the size 
		// specified by cchAlloc minus two.

		if( cch != (cchAlloc - 2) )
		{
			break;
		}

		cch = 0;
		cchAlloc *= 2;

		_MemFree( pszBuffer );
	}

	if( cch == 0 )
	{
		_MemFree(pszBuffer);
		return NULL;
	}

	return pszBuffer;
}

//----------------------------------------------------------------------------
//
//  DeleteMDIChildFrameSections()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
BOOL DeleteMDIChildFrameSections()
{
	PWSTR pszSectionNames = GetSectionNames();
	PWSTR psz;

	if( pszSectionNames == NULL )
		return FALSE;

	psz = pszSectionNames;

	while( *psz )
	{
		if( IsNoBlacketIIDSectionName(psz) )
		{
			WritePrivateProfileSection(psz,NULL,_GetIniFileName());
		}

		psz += (wcslen(psz) + 1);
	}

	_MemFree(pszSectionNames);

	return TRUE;
}
