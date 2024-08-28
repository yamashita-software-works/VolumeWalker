//****************************************************************************
//
//  find.cpp
//
//  Implements the simple "Find" common dialogs wrapper functions.
//
//  Auther: YAMASHITA Katsuhiro
//
//  Create: 2023.12.14
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "find.h"
#include "common_msg.h"

static UINT g_uFindMessage = 0;
static HWND g_hwndFindDialog = NULL;
static FINDREPLACE g_fr = {0};
static int g_iFindAction = 0;
static WCHAR *g_pszFindText = NULL;

#define _MAX_FINDTEXT MAX_PATH

VOID FindText_Initialize()
{
	g_uFindMessage = RegisterWindowMessage(FINDMSGSTRING);
}

VOID FindText_Uninitialize()
{
	_SafeMemFree(g_pszFindText);
}

UINT_PTR
CALLBACK
FRHookProc(
	HWND hdlg,
	UINT uiMsg,
	WPARAM wParam,
	LPARAM lParam
	)
{
	if( uiMsg == WM_INITDIALOG )
	{
		FINDREPLACE *pfr = (FINDREPLACE *)lParam;
		_CenterWindow(hdlg,(HWND)pfr->lCustData);
		return TRUE;
	}
	return 0;
}

LRESULT OnFindText_CommandHandler(HWND hwndOwner,HWND hwndTarget,FINDACTION CmdId)
{
	if( hwndTarget == NULL )
		hwndTarget = hwndOwner;

	if( Find_Start == CmdId )
	{
		SendMessage(hwndTarget,PM_FINDITEM,FIND_QUERYOPENDIALOG,(LPARAM)0);

		if( g_pszFindText == NULL )
		{
			g_pszFindText = new WCHAR[_MAX_FINDTEXT];
			memset(g_pszFindText,0,sizeof(WCHAR[_MAX_FINDTEXT]));
		}

		memset(&g_fr,0,sizeof(g_fr));

		g_iFindAction = 0;

		g_fr.hInstance     = 0;
		g_fr.hwndOwner     = hwndOwner;
		g_fr.lStructSize   = sizeof(FINDREPLACE);
		g_fr.lpstrFindWhat = g_pszFindText;
		g_fr.wFindWhatLen  = _MAX_FINDTEXT;
		g_fr.lpfnHook      = &FRHookProc;
		g_fr.Flags         = FR_FINDNEXT|FR_DOWN|FR_HIDEWHOLEWORD|FR_HIDEMATCHCASE|FR_ENABLEHOOK;
		g_fr.lCustData     = (LPARAM)hwndTarget;

		g_hwndFindDialog = FindText(&g_fr);
	}
	else if( Find_Next == CmdId || Find_Previous == CmdId )
	{
		if( g_pszFindText && *g_pszFindText != 0 )
		{
			if( g_iFindAction == 0 )
			{
				SendMessage(hwndTarget,PM_FINDITEM,FIND_SEARCH,(LPARAM)&g_fr);
				g_iFindAction = 1;
			}
			else
			{
				g_fr.Flags &= ~FR_DOWN;
				g_fr.Flags |= (Find_Next == CmdId) ? FR_DOWN : 0;
				SendMessage(hwndTarget,PM_FINDITEM,FIND_SEARCH_NEXT,(LPARAM)&g_fr);
			}
		}
	}
	return 0;
}

LRESULT OnFindText_DialogEvent(HWND hwndTarget,LPFINDREPLACE lpfr)
{
	// If the FR_DIALOGTERM flag is set, 
	// invalidate the handle that identifies the dialog box. 
	if( lpfr->Flags & FR_DIALOGTERM )
	{ 
		SendMessage(hwndTarget,PM_FINDITEM,FIND_CLOSEDIALOG,(LPARAM)lpfr);
		g_hwndFindDialog = NULL; 
		return 0;
	} 

	// If the FR_FINDNEXT flag is set, 
	// call the application-defined search routine
	// to search for the requested string. 
	if( lpfr->Flags & FR_FINDNEXT )
	{
		if( g_iFindAction == 0 )
		{
			SendMessage(hwndTarget,PM_FINDITEM,FIND_SEARCH,(LPARAM)&g_fr);
			g_iFindAction = 1;
		}
		else
		{
			SendMessage(hwndTarget,PM_FINDITEM,FIND_SEARCH_NEXT,(LPARAM)&g_fr);
		}
	}
	return 0;
}

BOOL IsFindTextEventMessage(HWND hwndTarget,UINT message,LPARAM lParam)
{
	if( g_uFindMessage == message )
	{
		OnFindText_DialogEvent(hwndTarget,(LPFINDREPLACE)lParam);
		return TRUE;
	}
	return FALSE;
}

BOOL IsFindTextDialogMessage(MSG *pmsg)
{
	if( g_hwndFindDialog && IsDialogMessage(g_hwndFindDialog,pmsg) )
	{
		return TRUE;
	}
	return FALSE;
}

BOOL HasFindText()
{
	return (g_pszFindText != NULL) && (*g_pszFindText != L'\0');
}
