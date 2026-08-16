// 2026-03-28
#pragma once

LRESULT
CALLBACK
EditBox_DecInputSubclassProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
	);

LRESULT
CALLBACK
EditBox_HexInputSubclassProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
	);

LRESULT
CALLBACK
EditBox_BinInputSubclassProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
	);

#define EBSPF_ACCEPT_SPACE                (0x00000001)
#define EBSPF_ACCEPT_HEX_PREFIX_CHAR      (0x00000002)

LRESULT
CALLBACK
EditBox_NumberInputSubclassProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
	);
