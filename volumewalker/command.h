// 2025-03-17 Created
#pragma once

HANDLE CreateCommandHandler(HWND hWnd);
BOOL CloseCommandHandler(HANDLE hCommand);

namespace CommandHandler
{
	BOOL PreTranslateMessage(MSG *pmsg);
	INT QueryCmdState(UINT CmdId,INT& State);
	BOOL NotifyClose(HWND);
	BOOL ForwardCommand(HANDLE hCommand,UINT idCmd);
	HMENU MakeVolumeCommandMenu(HANDLE /*hCommand*/);
};
