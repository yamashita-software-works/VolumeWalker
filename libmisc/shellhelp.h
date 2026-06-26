#pragma once
//****************************************************************************
//
//  shellhelp.h
//
//  Shell helper functions.
//
//  Author:  YAMASHITA Katsuhiro
//
//  History: 2026-06-16 Created.
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
HRESULT
_GetRecycleBinFolder(
	IShellFolder **RecycleBinFolder
	);

HRESULT
_ExecRecycleBinItemCommand(
	HWND hWnd,
	LPITEMIDLIST pidl,
	PCSTR pcaszCommand
	);

HRESULT
_ExecShellFolderMenuCommand(
	HWND hWnd,
	IShellFolder *pFolder,
	LPITEMIDLIST pidl,
	LPCSTR pszRequestCommand
	);

BOOL
_GetShellItemNameW(
	LPSHELLFOLDER lpsf,
	LPITEMIDLIST lpi,
	DWORD dwFlags,
	LPTSTR lpFriendlyName,DWORD cch
	);

LONGLONG
_ConvertVariantDateTime(
	VARIANT& vt
	);
