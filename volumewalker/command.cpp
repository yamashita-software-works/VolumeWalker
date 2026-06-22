//****************************************************************************
//
//  command.cpp
//
//  Command Handler
//
//  Author: YAMASHITA Katsuhiro
//
//  History: 2025-03-17 Created
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "command.h"
#include "resource.h"
#include "inifile.h"
#include "shellpathhelp.h"
#include "fsvolumelist.h"

#if _ENABLE_TOOL_TOOLPAD
#include "..\fsvolumetools\toolpad.h"
#define _DLL_VOLUMETOOLS  L"fsvolumetools.dll"
#endif

#define _DLL_VOLUMEDISKS  L"fsvolumedisks.dll"

using namespace CommandHandler;

typedef struct _COMMAND_HANDLER_PARAMETER
{
	HWND hWnd;
} COMMAND_HANDLER_PARAMETER;

typedef int (WINAPI *SHRUNFILEDIALOG)(HWND,HICON,PWSTR,PWSTR,PWSTR,UINT);
#define SHRUNFILEDLG_ORDINAL  61

#define SHRFDF_NO_BROWSE_BUTTON      0x1
#define SHRFDF_NO_INITIAL_FILENAME   0x2

BOOL
WINAPI
_ShellRunFileDlg(
	HWND hWnd,
	LPTSTR pszPath,
	LPTSTR pszCaption,
	LPTSTR pszMessage,
	HICON hIcon,
	ULONG Flags
	)
{
	int ret = -1;

	HINSTANCE hInstShell = (HINSTANCE)LoadLibrary(_T("SHELL32.DLL"));
	if( hInstShell == NULL )
		return FALSE;

	// Usage:
	// https://www.codeproject.com/Articles/2734/Using-the-Windows-RunFile-dialog-The-documented-an
	SHRUNFILEDIALOG SHRunFileDlg = (SHRUNFILEDIALOG)GetProcAddress(hInstShell,(LPSTR)SHRUNFILEDLG_ORDINAL);

	ret = SHRunFileDlg(hWnd,
			hIcon,
			pszPath,
			pszCaption,
			pszMessage,
			Flags
		); 

	FreeLibrary(hInstShell);

	return ret;
}

namespace CommandHandler
{
	HWND hWndToolPad = NULL;

	VOID InitializeVolumeTools()
	{
#if _ENABLE_TOOL_TOOLPAD
		HRESULT (WINAPI *pfnInitializeVolumeTools)(HWND hWnd) = NULL;
		HMODULE hModule;
		hModule = LoadLibrary( _DLL_VOLUMETOOLS );
		if( hModule )
		{
			(FARPROC&)pfnInitializeVolumeTools = GetProcAddress(hModule,"InitializeVolumeTools");
			if( pfnInitializeVolumeTools )
			{
				pfnInitializeVolumeTools(NULL);
			}
		}
#endif
	}
	
	HMENU MakeVolumeCommandMenu(HANDLE /*hCommand*/)
	{
		// hCommand : currently unused.

		HMENU hToolMenu = CreatePopupMenu();
		AppendMenu(hToolMenu, MF_STRING, ID_SHELL_RUNFILE_DIALOG,     L"Run file dialog");
		AppendMenu(hToolMenu, MF_STRING, 0, NULL);
		AppendMenu(hToolMenu, MF_STRING, ID_ATTACH_VIRTUALDISK_IMAGE, L"Attach &Virtual Disk Image...");

#if _ENABLE_TOOL_DISKIMAGE
		HMENU hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, ID_CREATE_DISK_IMAGEFILE,     L"Create Disk &Image File...");
		AppendMenu(hSubMenu, MF_STRING, ID_RESTORE_DISK_IMAGEFILE,    L"&Write Back Disk Image File...");
		AppendMenu(hToolMenu, MF_STRING, 0, NULL);
		AppendMenu(hToolMenu, MF_POPUP, (UINT_PTR)hSubMenu,           L"Deprecated Features");
#endif

#if (_ENABLE_TOOL_TOOLPAD || _ENABLE_TOOL_LAUNCHPAD)
		AppendMenu(hToolMenu, MF_STRING, 0, NULL);
#if _ENABLE_TOOL_LAUNCHPAD
		AppendMenu(hToolMenu, MF_STRING, ID_LAUNCHPAD_WINDOW,         L"&Launch External Tools");
#endif
	    AppendMenu(hToolMenu, MF_STRING, ID_TOOLPAD_WINDOW,           L"&Tool Pad");
#endif
		return hToolMenu;
	}

#if _ENABLE_TOOL_TOOLPAD
	HWND CreateToolPadWindow(HWND hWnd)
	{
		HWND hwnd = NULL;
		HWND (WINAPI *pfnCreateToolPadWindow)(HWND hWnd,UINT,UINT,PVOID) = NULL;
		HMODULE hModule;
		hModule = LoadLibrary( _DLL_VOLUMETOOLS );
		if( hModule )
		{
			(FARPROC&)pfnCreateToolPadWindow = GetProcAddress(hModule,"CreateToolPadWindow");
			if( pfnCreateToolPadWindow )
			{
				hwnd = pfnCreateToolPadWindow(hWnd,TPD_PAGE_RADIX,TPAD_TYPE_MINITOOLS,NULL);
			}
			FreeLibrary(hModule);
		}
		return hwnd;
	}

	VOID OpenToolPadWindow(HWND hWnd,UINT uOpenMode)
	{
		if( uOpenMode == 1 )
		{
			HRESULT (WINAPI *pfnToolPadDialog)(HWND hWnd,UINT,UINT,PVOID) = NULL;
			HMODULE hModule;
			hModule = LoadLibrary( _DLL_VOLUMETOOLS );
			if( hModule )
			{
				(FARPROC&)pfnToolPadDialog = GetProcAddress(hModule,"ToolPadDialog");
				if( pfnToolPadDialog )
				{
					pfnToolPadDialog(hWnd,TPD_PAGE_APPLAUNCH,TPAD_TYPE_LAUNCHPAD,nullptr);
				}
				FreeLibrary(hModule);
			}
		}
		else
		{
			if( hWndToolPad == NULL || !IsWindow(hWndToolPad) )
			{
				hWndToolPad = CreateToolPadWindow(hWnd);
			}
			else
			{
				if( IsWindowVisible(hWndToolPad) )
					SetActiveWindow(hWndToolPad);
				else
					ShowWindow(hWndToolPad,SW_SHOW);
			}
		}
	}
#endif

	//------------------------------------------------------------------------
	//
	//  OnAttachViryualDiskImageFile()
	//
	//------------------------------------------------------------------------
	VOID OnAttachViryualDiskImageFile(HWND hWnd)
	{
		VirtualDiskAttachDialog(hWnd,nullptr,0);
	}

	//------------------------------------------------------------------------
	//
	//  OnCreateDiskImageFile()
	//
	//------------------------------------------------------------------------
	VOID OnCreateDiskImageFile(HWND hWnd)
	{
		HRESULT (WINAPI *CreateDiskImageFileDialog)(
			__in HWND hWnd,
			__in PWSTR Reserved1,
			__in PWSTR Reserved2,
			__in PVOID ReservedPtr,
			__inout_opt	 HANDLE *phHandle
			) = NULL;
		HMODULE hModule;
		hModule = GetModuleHandle( _DLL_VOLUMEDISKS );
		if( hModule == NULL )
			hModule = LoadLibrary( _DLL_VOLUMEDISKS );
		if( hModule )
		{
			(FARPROC&)CreateDiskImageFileDialog = GetProcAddress(hModule,"CreateDiskImageFileDialog");
			if( CreateDiskImageFileDialog )
			{
				CreateDiskImageFileDialog(hWnd,0,0,0,NULL);
			}
		}
	}

	//------------------------------------------------------------------------
	//
	//  OnRestoreDiskImageFile()
	//
	//------------------------------------------------------------------------
	VOID OnRestoreDiskImageFile(HWND hWnd)
	{
		HRESULT (WINAPI *WriteBackDiskImageFileDialog)(
			__in HWND hWnd,
			__in PWSTR Reserved1,
			__in PWSTR Reserved2,
			__in PVOID ReservedPtr,
			__inout_opt	 HANDLE *phHandle
			) = NULL;
		HMODULE hModule;
		hModule = GetModuleHandle( _DLL_VOLUMEDISKS );
		if( hModule == NULL )
			hModule = LoadLibrary( _DLL_VOLUMEDISKS );
		if( hModule )
		{
			(FARPROC&)WriteBackDiskImageFileDialog = GetProcAddress(hModule,"WriteBackDiskImageFileDialog");
			if( WriteBackDiskImageFileDialog )
			{
				WriteBackDiskImageFileDialog(hWnd,0,0,0,NULL);
			}
		}
	}
#if _ENABLE_TOOL_TOOLPAD
	//------------------------------------------------------------------------
	//
	//  OnToolPadWindow()
	//
	//------------------------------------------------------------------------
	VOID OnToolPadWindow(HWND hWnd)
	{
		OpenToolPadWindow(hWnd,0);
	}
#endif
#if _ENABLE_TOOL_LAUNCHPAD
	//------------------------------------------------------------------------
	//
	//  OnLaunchPadWindow()
	//
	//------------------------------------------------------------------------
	VOID OnLaunchPadWindow(HWND hWnd)
	{
		OpenToolPadWindow(hWnd,1);
	}
#endif
	//------------------------------------------------------------------------
	//
	//  OnShellRunFileDialog()
	//
	//------------------------------------------------------------------------
	VOID OnShellRunFileDialog(HWND hWnd)
	{
		HICON hIcon = LoadIcon(_GetInstanceHandle(),MAKEINTRESOURCE(IDI_MAIN));
		 _ShellRunFileDlg(hWnd,NULL,NULL,NULL,hIcon,SHRFDF_NO_INITIAL_FILENAME);
		DestroyIcon(hIcon);
	}

	BOOL NotifyClose(HWND hwnd)
	{
		if( hWndToolPad == hwnd )
		{
			hWndToolPad = NULL;
			return TRUE;
		}
		return FALSE;
	}

	//
	// Message Hook Handler
	//
	BOOL PreTranslateMessage(MSG *pmsg)
	{
		return FALSE;
	}

	//
	// Command State
	//
	BOOL QueryCmdState(UINT CmdId,INT& State)
	{
		switch( CmdId )
		{
			case ID_SHELL_RUNFILE_DIALOG:
			case ID_ATTACH_VIRTUALDISK_IMAGE:
#if _ENABLE_TOOL_DISKIMAGE
			case ID_CREATE_DISK_IMAGEFILE:
			case ID_RESTORE_DISK_IMAGEFILE:
#endif
#if _ENABLE_TOOL_LAUNCHPAD
			case ID_LAUNCHPAD_WINDOW:
#endif
#if _ENABLE_TOOL_TOOLPAD
			case ID_TOOLPAD_WINDOW:
#endif
				State = UPDUI_ENABLED;
				return TRUE;
		}
		return FALSE;
	}

	// 
	// Forward Command
	//
	BOOL ForwardCommand(HANDLE hCommand,UINT idCmd)
	{
		if( hCommand == NULL ) {
			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}
	
		COMMAND_HANDLER_PARAMETER *pchp = (COMMAND_HANDLER_PARAMETER *)hCommand;
		HWND hWnd = pchp->hWnd;
		switch( idCmd )
		{
			case ID_ATTACH_VIRTUALDISK_IMAGE:
				CommandHandler::OnAttachViryualDiskImageFile(hWnd);
				break;
#if _ENABLE_TOOL_DISKIMAGE
			case ID_CREATE_DISK_IMAGEFILE:
				CommandHandler::OnCreateDiskImageFile(hWnd);
				break;
			case ID_RESTORE_DISK_IMAGEFILE:
				CommandHandler::OnRestoreDiskImageFile(hWnd);
				break;
#endif
#if _ENABLE_TOOL_LAUNCHPAD
			case ID_LAUNCHPAD_WINDOW:
				CommandHandler::OnLaunchPadWindow(hWnd);
				break;
#endif
#if _ENABLE_TOOL_TOOLPAD
			case ID_TOOLPAD_WINDOW:
				CommandHandler::OnToolPadWindow(hWnd);
				break;
#endif
			case ID_SHELL_RUNFILE_DIALOG:
				CommandHandler::OnShellRunFileDialog(hWnd);
				break;
			default:
				return SetLastError(ERROR_SUCCESS),FALSE;
		}
	
		return SetLastError(ERROR_SUCCESS),TRUE;
	}
};

//----------------------------------------------------------------------------
//
//  CreateCommandHandler()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
HANDLE CreateCommandHandler(HWND hWnd)
{
	CommandHandler::InitializeVolumeTools();

	COMMAND_HANDLER_PARAMETER *pchp = new COMMAND_HANDLER_PARAMETER;
	if( pchp == NULL )
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return NULL;
	}

	pchp->hWnd = hWnd;

	return SetLastError(ERROR_SUCCESS),pchp;
}

//----------------------------------------------------------------------------
//
//  CloseCommandHandler()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
BOOL CloseCommandHandler(HANDLE hCommand)
{
	if( hCommand == NULL )
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if( CommandHandler::hWndToolPad )
	{
		DWORD dwThreadId = GetWindowThreadProcessId(CommandHandler::hWndToolPad,NULL);
		PostMessage( CommandHandler::hWndToolPad, WM_CONTROL_MESSAGE, MAKEWPARAM(UI_CLOSE,0), 0 );
		HANDLE hThread = OpenThread(THREAD_ALL_ACCESS,FALSE,dwThreadId);
		if( hThread ) {
			WaitForSingleObject(hThread,INFINITE);
			CloseHandle(hThread);
		}
	}

	delete ((COMMAND_HANDLER_PARAMETER *)hCommand);

	return SetLastError(ERROR_SUCCESS),TRUE;
}
