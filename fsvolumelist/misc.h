#pragma once

enum {
	OpenWithExplorer=0,
	OpenWithCommandPrompt,
	OpenWithPowerShell,
	OpenWithTerminal,
	OpenWithBash,
	OpenWithAdmin=0x8000,
};

HRESULT OpenVolumeLocation(HWND hWnd,UINT Open,PCWSTR pszDosDrive,PCWSTR pszVolumeGuid);
