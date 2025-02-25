//
// XpTheme.h - XP Theme helper 
// Written by YAMASHITA Katsuhiro 2002/9/28
//
#pragma once

BOOL
WINAPI
IsXpThemeEnabled(
	void
	);

BOOL
WINAPI 
XpFindManifest(
	HMODULE hModule
	);

VOID
WINAPI
XpEnableThemeEffrect(
	BOOL bEnable
	);

BOOL
WINAPI
XpIsThemeEffrectEnabled(
	VOID
	);

HRESULT
WINAPI
XpSetTabPageBackground(
	HWND hwndOwner,
	HWND hwndCtrl,
	HDC hdcCtrl,
	int nCtlColor,
	HBRUSH *phbrBack
	);

HRESULT
WINAPI
XpDrawCtrlColorBackground(
	HWND hwndOwner,
	HWND hwndCtrl,
	HDC hdcCtrl,
	HBRUSH *phbrBack
	);

HRESULT
WINAPI
XpEraseTabPageBackground(
	HWND hWnd,
	HDC hDC
	);

BOOL
WINAPI
XpInitThemeLibrary(
	void
	);

HRESULT
WINAPI
XpEnableThemeDialogTexture(
	HWND hwnd
	);

HRESULT
WINAPI
XpDrawWindowBackground(
	HWND hWnd,
	HDC hDC,
	RECT& rc,
	int Part,
	int States
	);

HRESULT
WINAPI
XpGetThemeSysSize(
	HWND hWnd,
	PWSTR ClassName,
	int iSizeId,
	int *pnVal
	);

HRESULT
WINAPI
XpGetThemeSysColorBrush(
	HWND hWnd,
	PWSTR ClassName,
	int iColorId,
	HBRUSH *phBrush
	);

HRESULT
WINAPI
XpGetThemeSysColor(
	HWND hWnd,
	PWSTR ClassName,
	int iColorId,
	COLORREF *pColor
	);

HRESULT
WINAPI
XpSetWindowTheme(
	HWND hWnd,
	LPCWSTR pszSubAppName,
	LPCWSTR	pszSubIdList
	);
