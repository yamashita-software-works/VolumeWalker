//
// XpTheme.cpp - Windows XP Theme helper 
// Written by YAMASHITA Katsuhiro 2002/9/28
//
#include "stdafx.h"
#include "themehelp.h"
#include <vssym32.h>

#ifndef _THEMELIBLINK_      // Win10
#define _THEMELIBLINK_ 1    // Win10
#endif                      // Win10

#if defined( _THEMELIBLINK_ )

#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")


#else

typedef HANDLE HTHEME;

HRESULT (WINAPI *DrawThemeParentBackground)(
	HWND hwnd,
    HDC hdc,
    RECT* prc
	) = NULL;

HRESULT (WINAPI *DrawThemeBackground)(
    HTHEME hTheme,
    HDC hdc,
    int iPartId,
    int iStateId,
    const RECT* pRect,
    const RECT* pClipRect
	) = NULL;

HTHEME (WINAPI *OpenThemeData)(
	HWND hwnd,
	LPCWSTR pszClassList
	) = NULL;

HRESULT (WINAPI *CloseThemeData)(
    HTHEME hTheme
	) = NULL;

BOOL (WINAPI *IsThemeActive)(
	VOID
	) = NULL;

HRESULT (WINAPI *EnableThemeDialogTexture)(
	HWND hwnd,
    DWORD dwFlags
	) = NULL;

HRESULT (WINAPI *GetThemeRect)(
    HTHEME hTheme,
    int iPartId,
    int iStateId,
    int iPropId,
    RECT *pRect
    ) = NULL;

int (WINAPI *GetThemeSysSize)(
    HTHEME hTheme,
    int iSizeID
    ) = NULL;

COLORREF (WINAPI *GetThemeSysColor)(
	HTHEME hTheme,
    int iColorID
	) = NULL;

HBRUSH (WINAPI *GetThemeSysColorBrush)(
	HTHEME hTheme,
    int iColorID
	) = NULL;

HRESULT (WINAPI *SetWindowTheme)(
    HWND hwnd,
    LPCWSTR pszSubAppName,
	LPCWSTR	pszSubIdList
    ) = NULL;

#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

static BOOL g_bXpThemeLib = FALSE;

#endif

BOOL WINAPI XpInitThemeLibrary()
{
#if !defined( _THEMELIBLINK_ )
	if( g_bXpThemeLib )
		return TRUE;

	HMODULE hmod = LoadLibrary( TEXT("UxTheme.dll") );

	if( hmod == NULL )
	{
		g_bXpThemeLib = FALSE;
		return FALSE;	
	}

	(FARPROC&)DrawThemeParentBackground = GetProcAddress(hmod,"DrawThemeParentBackground");
	(FARPROC&)DrawThemeBackground = GetProcAddress(hmod,"DrawThemeBackground");
	(FARPROC&)OpenThemeData = GetProcAddress(hmod,"OpenThemeData");
	(FARPROC&)CloseThemeData= GetProcAddress(hmod,"CloseThemeData");
	(FARPROC&)IsThemeActive = GetProcAddress(hmod,"IsThemeActive");
	(FARPROC&)EnableThemeDialogTexture = GetProcAddress(hmod,"EnableThemeDialogTexture");
	(FARPROC&)GetThemeSysSize =  GetProcAddress(hmod,"GetThemeSysSize");
	(FARPROC&)GetThemeSysColor =  GetProcAddress(hmod,"GetThemeSysColor");
	(FARPROC&)GetThemeSysColorBrush =  GetProcAddress(hmod,"GetThemeSysColorBrush");
	(FARPROC&)SetWindowTheme =  GetProcAddress(hmod,"SetWindowTheme");

	if(	(DrawThemeParentBackground == NULL)
		|| (DrawThemeBackground == NULL)
		|| (OpenThemeData == NULL)
		|| (CloseThemeData == NULL)
		|| (IsThemeActive == NULL)
		|| (EnableThemeDialogTexture == NULL)
		|| (GetThemeSysSize == NULL)
		|| (GetThemeSysColor == NULL)
		|| (GetThemeSysColorBrush == NULL)
		|| (SetWindowTheme ==  NULL)
		)
	{
		return FALSE;	
	}
	g_bXpThemeLib = TRUE;
#endif
	return TRUE;
}

BOOL WINAPI IsXpThemeEnabled()
{
#if !defined( _THEMELIBLINK_ )
	if( !g_bXpThemeLib )
		return FALSE;
#endif
	return IsThemeActive();
}

BOOL WINAPI XpFindManifest(HMODULE hModule)
{
	HRSRC hrsc;
	hrsc = FindResource(hModule,MAKEINTRESOURCE(CREATEPROCESS_MANIFEST_RESOURCE_ID),RT_MANIFEST);
	if( hrsc != NULL )
		return TRUE;

	TCHAR szPath[MAX_PATH];
	if( GetModuleFileName(hModule,szPath,MAX_PATH) != 0 )
	{
		DWORD d;

		_tcscat_s(szPath,MAX_PATH,TEXT(".manifest"));

		d = GetFileAttributes(szPath);
		if( d != (DWORD)-1 && !(d & FILE_ATTRIBUTE_DIRECTORY) )
		{
			return TRUE;
		}
	}

	return FALSE;
}

VOID WINAPI XpEnableThemeEffrect(BOOL bEnable)
{
#if !defined( _THEMELIBLINK_ )
	g_bXpThemeLib = bEnable;
#endif
}

BOOL WINAPI XpIsThemeEffrectEnabled()
{
#if !defined( _THEMELIBLINK_ )
	return g_bXpThemeLib;
#else
	return TRUE;
#endif
}

HRESULT WINAPI XpDrawCtrlColorBackground(HWND hwndOwner,HWND hwndCtrl,HDC hdcCtrl,HBRUSH *phbrBack)
{
	HRESULT hr;
#if !defined( _THEMELIBLINK_ )
	if( !g_bXpThemeLib )
		return E_FAIL;
#endif

	hr = DrawThemeParentBackground(hwndCtrl,hdcCtrl,NULL);

	if( phbrBack )
	{
		SetBkMode( hdcCtrl,TRANSPARENT );
		*phbrBack = (HBRUSH)GetStockObject(NULL_BRUSH);
	}

	return SUCCEEDED(hr) ? S_OK : hr;
}

HRESULT WINAPI XpEraseTabPageBackground(HWND hWnd,HDC hDC)
{
	HRESULT hr = E_FAIL;
#if !defined( _THEMELIBLINK_ )
	if( !g_bXpThemeLib )
		return E_FAIL;
#endif

	RECT rc;
	GetClientRect(hWnd,&rc);

	HTHEME hTheme;
	hTheme = OpenThemeData(hWnd,L"TAB");

	if( hTheme != NULL )
	{
		DrawThemeBackground(hTheme,hDC,10,0,&rc,NULL);

		CloseThemeData(hTheme);

		hr = S_OK;
	}

	return hr;
}

HRESULT WINAPI XpEnableThemeDialogTexture(HWND hwnd)
{
	return EnableThemeDialogTexture(hwnd,ETDT_ENABLETAB|ETDT_USETABTEXTURE);
}

HRESULT WINAPI XpDrawWindowBackground(HWND hWnd,HDC hDC,RECT& rc,int Part,int State)
{
	HRESULT hr = E_FAIL;
#if !defined( _THEMELIBLINK_ )
	if( !g_bXpThemeLib )
		return E_FAIL;
#endif
	HTHEME hTheme;
	hTheme = OpenThemeData(hWnd,L"WINDOW");

	if( hTheme != NULL )
	{
		DrawThemeBackground(hTheme,hDC,Part,State,&rc,NULL);

		CloseThemeData(hTheme);

		hr = S_OK;
	}

	return hr;
}

HRESULT WINAPI XpGetThemeSysSize(HWND hWnd,PWSTR ClassName,int iSizeId,int *pnVal)
{
	HRESULT hr = E_FAIL;
#if !defined( _THEMELIBLINK_ )
	if( !g_bXpThemeLib )
		return E_FAIL;
#endif
	HTHEME hTheme;
	hTheme = OpenThemeData(hWnd,ClassName);

	*pnVal = 0;

	if( hTheme != NULL )
	{
		int n;
		if( (n = GetThemeSysSize(hTheme,iSizeId)) != 0 )
		{
			*pnVal = n;
		}
		CloseThemeData(hTheme);

		hr = S_OK;
	}

	return hr;
}

HRESULT WINAPI XpGetThemeSysColorBrush(HWND hWnd,PWSTR ClassName,int iColorId,HBRUSH *phBrush)
{
	HRESULT hr = E_FAIL;
#if !defined( _THEMELIBLINK_ )
	if( !g_bXpThemeLib )
		return E_FAIL;
#endif
	HTHEME hTheme = NULL;

	if( ClassName != NULL )
		hTheme = OpenThemeData(hWnd,ClassName);

	*phBrush = GetThemeSysColorBrush(hTheme,iColorId);

	if( hTheme )
		CloseThemeData(hTheme);

	hr = S_OK;

	return hr;
}

HRESULT WINAPI XpGetThemeSysColor(HWND hWnd,PWSTR ClassName,int iColorId,COLORREF *pColor)
{
	HRESULT hr = E_FAIL;
#if !defined( _THEMELIBLINK_ )
	if( !g_bXpThemeLib )
		return E_FAIL;
#endif
	HTHEME hTheme = NULL;

	if( ClassName != NULL )
		hTheme = OpenThemeData(hWnd,ClassName);

	*pColor = GetThemeSysColor(hTheme,iColorId);

	if( hTheme )
		CloseThemeData(hTheme);

	hr = S_OK;

	return hr;
}

HRESULT WINAPI XpSetWindowTheme(HWND hWnd,LPCWSTR pszSubAppName,LPCWSTR	pszSubIdList)
{
#if !defined( _THEMELIBLINK_ )
	if( !g_bXpThemeLib )
		return E_FAIL;
#endif
	return SetWindowTheme(hWnd,pszSubAppName,pszSubIdList);
}
