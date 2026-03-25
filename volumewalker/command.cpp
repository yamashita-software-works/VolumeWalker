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

typedef struct _COMMAND_HANDLER_PARAMETER
{
	HWND hWnd;
} COMMAND_HANDLER_PARAMETER;

HANDLE CreateCommandHandler(HWND hWnd)
{
	InitializeVolumeTools();

	COMMAND_HANDLER_PARAMETER *pchp = new COMMAND_HANDLER_PARAMETER;
	if( pchp == NULL ) {
		SetLastError(ERROR_OUTOFMEMORY);
		return NULL;
	}
	pchp->hWnd = hWnd;
	return SetLastError(ERROR_SUCCESS),pchp;
}

BOOL CloseCommandHandler(HANDLE hCommand)
{
	if( hCommand == NULL ) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	delete ((COMMAND_HANDLER_PARAMETER *)hCommand);

	return SetLastError(ERROR_SUCCESS),TRUE;
}

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
#if _ENABLE_TOOL_DISKIMAGE
		AppendMenu(hToolMenu, MF_STRING, 0, NULL);
		AppendMenu(hToolMenu, MF_STRING, ID_CREATE_DISK_IMAGEFILE,    L"Create Disk &Image File...");
		AppendMenu(hToolMenu, MF_STRING, ID_RESTORE_DISK_IMAGEFILE,   L"&Restore Disk Image File...");
#endif
#if _ENABLE_TOOL_DISKIMAGE && (_ENABLE_TOOL_DISKIMAGE || _ENABLE_TOOL_LAUNCHPAD)
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

namespace CommandHandler
{
	VOID OnAttachViryualDiskImageFile(HWND hWnd)
	{
		VirtualDiskAttachDialog(hWnd,nullptr,0);
	}

	//
	// Message Hook Handler
	//
	BOOL Message(MSG *pmsg)
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
			case ID_ATTACH_VIRTUALDISK_IMAGE:
				State = UPDUI_ENABLED;
				return TRUE;
		}
		return FALSE;
	}
};

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
	}

	return SetLastError(ERROR_SUCCESS),TRUE;
}
