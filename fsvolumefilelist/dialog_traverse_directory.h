#pragma once

HRESULT
WINAPI
TraverseDirectoryDialog(
	HWND hWnd,
	PCWSTR pszCurDir,
	PWSTR *ppszNewPath,
	DWORD dwFlags
	);
