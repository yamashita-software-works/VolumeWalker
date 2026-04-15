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
#include "..\fsvolumelist\fsvolumelist.h"

#define _DLL_VOLUMETOOLS  L"fsvolumetools.dll"

using namespace CommandHandler;

typedef struct _COMMAND_HANDLER_PARAMETER
{
	HWND hWnd;
} COMMAND_HANDLER_PARAMETER;

namespace CommandHandler
{
	HWND hWndToolPad = NULL;

	VOID InitializeVolumeTools()
	{
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
	}
	
	HMENU MakeVolumeCommandMenu(HANDLE /*hCommand*/)
	{
		// hCommand : currently unused.
	
		HMENU hToolMenu = CreatePopupMenu();
		
		AppendMenu(hToolMenu, MF_STRING, ID_ATTACH_VIRTUALDISK_IMAGE, L"Attach &Virtual Disk Image...");
	
		if( PathFileExists( makedllpath(_DLL_VOLUMETOOLS) ) )
		{
	#if (_ENABLE_TOOL_DISKIMAGE || _ENABLE_TOOL_DISKIMAGE || _ENABLE_TOOL_TOOLPAD || _ENABLE_TOOL_LAUNCHPAD)
			AppendMenu(hToolMenu, MF_STRING, 0, NULL);
	#endif
	#if _ENABLE_TOOL_DISKIMAGE
			AppendMenu(hToolMenu, MF_STRING, ID_CREATE_DISK_IMAGEFILE,    L"Create Disk &Image File...");
			AppendMenu(hToolMenu, MF_STRING, ID_RESTORE_DISK_IMAGEFILE,   L"&Write Back Image File to Disk...");
	#endif
	#if _ENABLE_TOOL_DISKIMAGE && (_ENABLE_TOOL_DISKIMAGE || _ENABLE_TOOL_TOOLPAD || _ENABLE_TOOL_LAUNCHPAD)
			AppendMenu(hToolMenu, MF_STRING, 0, NULL);
	#endif
	#if _ENABLE_TOOL_TOOLPAD
		    AppendMenu(hToolMenu, MF_STRING, ID_TOOLPAD_WINDOW,           L"&Tool Pad");
	#endif
	#if _ENABLE_TOOL_LAUNCHPAD
			AppendMenu(hToolMenu, MF_STRING, ID_LAUNCHPAD_WINDOW,         L"&Launch Pad");
	#endif
		}
	
		return hToolMenu;
	}
	
	HWND CreateToolPadWindow(HWND hWnd)
	{
#if 0
		// Modal Dialog
		HRESULT (WINAPI *pfnToolPadWindow)(HWND hWnd) = NULL;
		HMODULE hModule;
		hModule = LoadLibrary( _DLL_VOLUMETOOLS );
		if( hModule )
		{
			(FARPROC&)pfnToolPadWindow = GetProcAddress(hModule,"ToolPadWindow");
			if( pfnToolPadWindow )
			{
				pfnToolPadWindow(hWnd);
			}
			FreeLibrary(hModule);
		}
#else
		// Modeless Dialog
		HWND hwnd = NULL;
		HWND (WINAPI *pfnCreateToolPadWindow)(HWND hWnd) = NULL;
		HMODULE hModule;
		hModule = LoadLibrary( _DLL_VOLUMETOOLS );
		if( hModule )
		{
			(FARPROC&)pfnCreateToolPadWindow = GetProcAddress(hModule,"CreateToolPadWindow");
			if( pfnCreateToolPadWindow )
			{
				hwnd = pfnCreateToolPadWindow(hWnd);
			}
			FreeLibrary(hModule);
		}
#endif
		return hwnd;
	}

	VOID OpenToolWindow(HWND hWnd,HWND *phWndTool)
	{
		if( !IsWindow(*phWndTool) )
		{
			*phWndTool = NULL;
		}
		else
		{
			if( IsWindowVisible(*phWndTool) )
			{
				ShowWindow(*phWndTool,SW_HIDE);
			}
			else
			{
				ShowWindow(*phWndTool,SW_SHOWNA);
			}
			return ;
		}
		
		if( *phWndTool == NULL )
		{
			*phWndTool = CreateWindowEx(WS_EX_TOOLWINDOW,L"VolumeToolsHostWndClass",NULL,WS_OVERLAPPEDWINDOW|WS_VISIBLE,
										 CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, hWnd, NULL, _GetResourceInstance(), NULL);
			{
				HMODULE hLib;
				hLib = LoadLibraryEx( L"imageres.dll", NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE );
				HICON hIcon = (HICON)LoadImage(hLib,MAKEINTRESOURCE(72),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
				SendMessage(*phWndTool,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
				FreeLibrary(hLib);
				SetWindowText(*phWndTool,L"Tool Host Window");
			}
		}
	
		if( *phWndTool )
		{
			CreateToolPadWindow(*phWndTool);

			SIZE size = { 640, 480 };
			RECT rcMainWnd;
			RECT rc;
			int x,y,cx,cy;
			PWSTR pszSectionName = L"ToolPad";
			if( ReadSectionRect(pszSectionName,L"Window",&rc) == 0 )
			{
				GetWindowRect(hWnd,&rcMainWnd);
				rcMainWnd.right -=  (size.cx + 48);
				rcMainWnd.bottom -=  (size.cy + 48);
				x = rcMainWnd.right;
				y = rcMainWnd.bottom;
				cx = size.cx;
				cy = size.cy;
			}
			else
			{
				x = rc.left;
				y = rc.top;
				cx = rc.right;
				cy = rc.bottom;
			}
	
			SetWindowPos(*phWndTool,NULL,x,y,cx,cy,SWP_NOZORDER|SWP_NOACTIVATE);
	
			ShowWindow(*phWndTool,SW_SHOWNOACTIVATE);
		}
	}

	VOID OnAttachViryualDiskImageFile(HWND hWnd)
	{
		VirtualDiskAttachDialog(hWnd,nullptr,0);
	}

	VOID OnCreateDiskImageFile(HWND hWnd)
	{
		HANDLE Handle;
		HRESULT (WINAPI *CreateDiskImageDialog)(
			__in HWND hWnd,
			__in PWSTR Reserved1,
			__in PWSTR Reserved2,
			__in PVOID ReservedPtr,
			__inout_opt	 HANDLE *phHandle
			) = NULL;
		HMODULE hModule;
		hModule = GetModuleHandle( _DLL_VOLUMETOOLS );
		if( hModule == NULL )
			hModule = LoadLibrary( _DLL_VOLUMETOOLS );
		if( hModule )
		{
			(FARPROC&)CreateDiskImageDialog = GetProcAddress(hModule,"CreateDiskImageDialog");
			if( CreateDiskImageDialog )
			{
				CreateDiskImageDialog(hWnd,0,0,0,&Handle);
			}
		}
	}

	VOID OnRestoreDiskImageFile(HWND hWnd)
	{
		HANDLE Handle;
		HRESULT (WINAPI *RestoreDiskImageDialog)(
			__in HWND hWnd,
			__in PWSTR Reserved1,
			__in PWSTR Reserved2,
			__in PVOID ReservedPtr,
			__inout_opt	 HANDLE *phHandle
			) = NULL;
		HMODULE hModule;
		hModule = GetModuleHandle( _DLL_VOLUMETOOLS );
		if( hModule == NULL )
			hModule = LoadLibrary( _DLL_VOLUMETOOLS );
		if( hModule )
		{
			(FARPROC&)RestoreDiskImageDialog = GetProcAddress(hModule,"WriteBackDiskImageDialog");
			if( RestoreDiskImageDialog )
			{
				RestoreDiskImageDialog(hWnd,0,0,0,&Handle);
			}
		}
	}

	VOID OnToolPadWindow(HWND hWnd)
	{
#if 1
#if 0
		HRESULT (WINAPI *pfnToolPadWindow)(HWND hWnd) = NULL;
		HMODULE hModule;
		hModule = LoadLibrary( _DLL_VOLUMETOOLS );
		if( hModule )
		{
			(FARPROC&)pfnToolPadWindow = GetProcAddress(hModule,"ToolPadWindow");
			if( pfnToolPadWindow )
			{
				pfnToolPadWindow(hWnd);
			}
			FreeLibrary(hModule);
		}
#else
		if( hWndToolPad == NULL || !IsWindow(hWndToolPad) )
			hWndToolPad = CreateToolPadWindow(hWnd);
		else
		{
			if( IsWindowVisible(hWndToolPad) )
				SetActiveWindow(hWndToolPad);
			else
				ShowWindow(hWndToolPad,SW_SHOW);
		}
#endif
#else
		OpenToolWindow(hWnd,&hWndToolPad);
#endif
	}

	VOID OnLaunchPadWindow(HWND hWnd)
	{
		HRESULT (WINAPI *pfnLaunchPadDialog)(HWND hWnd) = NULL;
		HMODULE hModule;
		hModule = LoadLibrary( _DLL_VOLUMETOOLS );
		if( hModule )
		{
			(FARPROC&)pfnLaunchPadDialog = GetProcAddress(hModule,"LaunchPadDialog");
			if( pfnLaunchPadDialog )
			{
				pfnLaunchPadDialog(hWnd);
			}
			FreeLibrary(hModule);
		}
	}

	//
	// Message Hook Handler
	//
	BOOL PreTranslateMessage(MSG *pmsg)
	{
		return FALSE;
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
	// Command State
	//
	BOOL QueryCmdState(UINT CmdId,INT& State)
	{
		switch( CmdId )
		{
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
		}
	
		return SetLastError(ERROR_SUCCESS),TRUE;
	}
};


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
