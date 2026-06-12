#pragma once

HRESULT
WINAPI
GotoDirectoryDialog(
	HWND hWnd,
	PCWSTR pszCurDir,
	PWSTR *ppszNewPath,
	DWORD dwFlags
	);
