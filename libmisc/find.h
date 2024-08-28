#pragma once

VOID FindText_Initialize();
VOID FindText_Uninitialize();
VOID FindText_SetActiveView(HWND hwndTarget);
BOOL IsFindTextEventMessage(HWND hwndTarget,UINT message,LPARAM lParam);
BOOL IsFindTextDialogMessage(MSG *pmsg);

typedef enum {
	Find_Start,
	Find_Next,
	Find_Previous,
} FINDACTION;

LRESULT OnFindText_CommandHandler(HWND hwndOwner,HWND hwndTarget,FINDACTION Action);
LRESULT OnFindText_DialogEvent(HWND hwndTarget,LPFINDREPLACE lpfr);

BOOL HasFindText();
