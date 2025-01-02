#pragma once

HRESULT
WINAPI
GotoDirectoryOnSameVolumeDialog(
	HWND hWnd,
	PCWSTR pszCurDir,
	PWSTR *ppszNewPath,
	DWORD dwFlags
	);
