#pragma once

interface IDebugText
{
	virtual HRESULT SetText(PCWSTR pszText) = 0;
	virtual HRESULT AddText(PCWSTR pszText) = 0;
};

interface IDebugTextCallback
{
	virtual HRESULT Initialize(IDebugText *) = 0;
};

enum {
	DebugTextInterface=1, // i/f support only
};

enum {
	DebugDumpPhase_Initialize=1, // initialize only
};

typedef HRESULT (WINAPI *PFNDEBUGTEXTCALLBACK)(INT,INT,PVOID,ULONG_PTR);

#define DDDF_USECALLBACKFUNC      (0x00000000)
#define DDDF_USECALLBACKINTERFACE (0x00000001)
#define DDDF_USETEXTINTERFACE     (0x00000002)
#define DDDF_SETDIALOGSIZE        (0x00000004) 

EXTERN_C
HRESULT
WINAPI
DebugDumpDialog(
	HWND hWnd,
	PCWSTR pszWindowTitle,
	PCWSTR pszText,
	DWORD dwFlags,
	PVOID Callback,
	ULONG_PTR Context,
	PVOID Reserved
	);
