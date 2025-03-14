#pragma once

typedef struct _MENUCOMMANDSTATE
{
	UINT IdFirst;
	UINT IdLast;
} MENUCOMMANDSTATE;

typedef INT (CALLBACK *PFNQUERYCOMMANDSTATE)(UINT CmdId,UINT MenuState,PVOID,LPARAM);

VOID UpdateUI_MenuItem(HMENU hMenu,PFNQUERYCOMMANDSTATE pfnQueryCmdStateCallback,LPARAM lParam);
