#pragma once

VOID FindText_Initialize();
VOID FindText_SetActiveView(HWND hwndTarget);
BOOL IsFindTextEventMessage(HWND hwndTarget,UINT message,LPARAM lParam);
BOOL IsFindTextDialogMessage(MSG *pmsg);

LRESULT OnFindText_CommandHandler(HWND hwndOwner,HWND hwndTarget,UINT CmdId);
LRESULT OnFindText_DialogEvent(HWND hwndTarget,LPFINDREPLACE lpfr);

BOOL HasFindText();
