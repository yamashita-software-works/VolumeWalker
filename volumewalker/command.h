// 2025-03-17 Created
#pragma once

HANDLE CreateCommandHandler(HWND hWnd);
BOOL CloseCommandHandler(HANDLE hCommand);
BOOL ForwardCommand(HANDLE hCommand,UINT idCmd);
HMENU MakeVolumeCommandMenu(HANDLE hCommand);
VOID InitializeVolumeTools();

namespace CommandHandler
{
	BOOL Message(MSG *pmsg);
	INT QueryCmdState(UINT CmdId,INT& State);
};
