//***************************************************************************
//*                                                                         *
//*  shellhelp.cpp                                                          *
//*                                                                         *
//*  Create: 2024-08-07                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "libmisc.h"

#define FORCE_DEFINE_KNOWN_FOLDER(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
FORCE_DEFINE_KNOWN_FOLDER(FOLDERID_AppsFolder, 0x1e87508d, 0x89c2, 0x42f0, 0x8a, 0x7e, 0x64, 0x5a, 0x0f, 0x50, 0xca, 0x58);

BOOL
WINAPI
SHFileIconInit(
    BOOL fRestoreCache
    )
{
    BOOL bResult = FALSE;
    static BOOL (WINAPI *pfnFileIconInit)(BOOL fRestoreCache) = NULL;
    HMODULE hModule = LoadLibrary(L"SHELL32");

    if( hModule )
    {
        (FARPROC&)pfnFileIconInit = GetProcAddress(hModule,(LPCSTR)660);
        if( pfnFileIconInit )
            bResult = pfnFileIconInit(fRestoreCache);
        FreeLibrary(hModule);
    }
    return bResult;
}

HRESULT GetShellImageList(HIMAGELIST *phImageList,HIMAGELIST *phImageListSmall)
{
	// Get Desktop folder
	LPITEMIDLIST pidl;
	HRESULT hr = SHGetSpecialFolderLocation(NULL,CSIDL_DESKTOP, &pidl);

	if( SUCCEEDED(hr) )
	{
		// Get system image lists
		SHFILEINFO sfi = { 0 };

		if( phImageList )
		{
			*phImageList = (HIMAGELIST)SHGetFileInfo((LPWSTR)pidl, 0, &sfi, sizeof(sfi),SHGFI_PIDL|SHGFI_SYSICONINDEX);
		}

		if( phImageListSmall )
		{
			memset(&sfi, 0, sizeof(SHFILEINFO));
			*phImageListSmall = (HIMAGELIST)SHGetFileInfo((LPWSTR)pidl, 0, &sfi, sizeof(sfi),SHGFI_PIDL|SHGFI_SYSICONINDEX|SHGFI_SMALLICON);
		}

		CoTaskMemFree(pidl);

		hr = S_OK;

		if( phImageList && *phImageList == NULL )
			hr = E_FAIL;
		else if( phImageListSmall && *phImageListSmall == NULL )
			hr = E_FAIL;
	}

	return hr;
}

int GetShellIconIndexIL(LPITEMIDLIST lpi, UINT uFlags)
{
	SHFILEINFO sfi = { 0 };
	uFlags |= (SHGFI_PIDL|SHGFI_SYSICONINDEX);
	DWORD_PTR dwRet = SHGetFileInfo((LPCTSTR)lpi, 0, &sfi, sizeof(SHFILEINFO), uFlags);
	if( sfi.hIcon )
		DestroyIcon(sfi.hIcon);
	return (dwRet != 0) ? sfi.iIcon : I_IMAGENONE;
}

HICON GetShellIconIL(LPITEMIDLIST lpi, UINT uFlags)
{
	SHFILEINFO sfi = { 0 };
	uFlags |= (SHGFI_PIDL|SHGFI_ICON);
	DWORD_PTR dwRet = SHGetFileInfo((LPCTSTR)lpi, 0, &sfi, sizeof(SHFILEINFO), uFlags);
	return (dwRet != 0) ? sfi.hIcon : NULL;
}

HICON WINAPI GetShellFileIcon(PCWSTR pszPath, UINT uFlags)
{
	SHFILEINFO sfi = { 0 };
	uFlags |= SHGFI_ICON;
	DWORD_PTR dwRet = SHGetFileInfo(pszPath, 0, &sfi, sizeof(SHFILEINFO), uFlags);
	return (dwRet != 0) ? sfi.hIcon : NULL;
}

HICON WINAPI GetShellStockIcon(SHSTOCKICONID StockIconId)
{
	SHSTOCKICONINFO sii = {0};
	sii.cbSize = sizeof(sii);
	SHGetStockIconInfo(StockIconId,SHGSI_ICON|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
	return sii.hIcon;
}

BOOL GetShellItemName(LPSHELLFOLDER lpsf, LPITEMIDLIST lpi, DWORD dwFlags, LPWSTR lpFriendlyName,int cchFriendlyName)
{
	BOOL bSuccess = TRUE;
	STRRET str = { STRRET_CSTR };

	if (lpsf->GetDisplayNameOf(lpi, dwFlags, &str) == NOERROR)
	{
		switch (str.uType)
		{
		case STRRET_WSTR:
			if( str.pOleStr )
				lstrcpy(lpFriendlyName, str.pOleStr);
			else
				*lpFriendlyName = L'\0';
			CoTaskMemFree(str.pOleStr);
			break;
		case STRRET_OFFSET:
			lstrcpy(lpFriendlyName, (LPTSTR)lpi + str.uOffset);
			break;
		case STRRET_CSTR:
			MultiByteToWideChar(CP_ACP,0,str.cStr,-1,lpFriendlyName,cchFriendlyName);
			break;
		default:
			bSuccess = FALSE;
			break;
		}
	}
	else
	{
		bSuccess = FALSE;
	}

	return bSuccess;
}

HRESULT OpenShellItemPath(PCWSTR psz)
{
	HRESULT hr;

	SHELLEXECUTEINFO sei = {0};

	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_DEFAULT;
	sei.lpVerb = L"open";
	sei.lpFile = psz;

	ShellExecuteEx( &sei );

	hr = HRESULT_FROM_WIN32( GetLastError() );

	return hr;
}

HRESULT OpenShellItemIL(LPITEMIDLIST pidl)
{
	HRESULT hr;

	SHELLEXECUTEINFO sei = {0};

	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_INVOKEIDLIST;
	sei.lpVerb = L"open";
	sei.nShow = SW_SHOWNORMAL;
	sei.lpIDList = pidl;
	sei.hwnd = GetActiveWindow();

	ShellExecuteEx( &sei );

	hr = HRESULT_FROM_WIN32( GetLastError() );

	return hr;
}

HRESULT OpenShellItemILEx(HWND hWnd,LPITEMIDLIST pidl,PCWSTR pszParameters,PCWSTR pszCurDir)
{
	HRESULT hr;

	SHELLEXECUTEINFO sei = {0};

	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_INVOKEIDLIST;
	sei.lpVerb = L"open";
	sei.nShow = SW_SHOWNORMAL;
	sei.lpIDList = pidl;
	sei.hwnd = hWnd;
	sei.lpParameters = pszParameters;
	sei.lpDirectory = pszCurDir;
	ShellExecuteEx( &sei );

	hr = HRESULT_FROM_WIN32( GetLastError() );

	return hr;
}

BOOL
WINAPI
_OpenByExplorerEx(
    HWND hWnd,
    LPCTSTR pszPath,
    LPCTSTR pszCurrentDirectory,
    BOOL bAdmin
    )
{
    BOOL bSuccess;
    if( bAdmin )
    {
        // effective for .exe file only.
        SHELLEXECUTEINFO sei = {0};
        sei.cbSize = sizeof(sei);
        sei.fMask = 0;
        sei.lpFile = pszPath;
        sei.lpDirectory = pszCurrentDirectory;
        sei.lpParameters = NULL;
        sei.lpVerb = L"runas";
        sei.nShow  = SW_SHOWNORMAL;
        bSuccess = ShellExecuteEx( &sei );
    }
    else
    {
		LPITEMIDLIST pidl = ILCreateFromPath(pszPath);
        SHELLEXECUTEINFO sei = {0};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_IDLIST|SEE_MASK_ASYNCOK;
		sei.lpIDList = pidl;
        sei.lpDirectory = pszCurrentDirectory;
        sei.lpParameters = NULL;
        sei.lpVerb = L"open";
        sei.nShow  = SW_SHOWNORMAL;
        bSuccess = ShellExecuteEx( &sei );
		ILFree(pidl);
    }
    return bSuccess;
}

BOOL WINAPI GetPowershellExePath(LPTSTR szPSPath)
{
    HKEY hkey;
    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\PowerShell"), &hkey))
    {
        return FALSE;
    }

    szPSPath[0] = TEXT('\0');

    for (int ikey = 0; ikey < 5; ikey++)
    {
        TCHAR szSub[10];    // just the "1" or "3"

        DWORD dwError = RegEnumKey(hkey, ikey, szSub, _countof(szSub));

        if (dwError == ERROR_SUCCESS)
        {
            // if installed, get powershell exe
            DWORD dwInstall;
            DWORD dwType;
            DWORD cbValue = sizeof(dwInstall);
            dwError = RegGetValue(hkey, szSub, TEXT("Install"), RRF_RT_DWORD, &dwType, (PVOID)&dwInstall, &cbValue);

            if (dwError == ERROR_SUCCESS && dwInstall == 1)
            {
                // this install of powershell is active; get path

                HKEY hkeySub;
                dwError = RegOpenKey(hkey, szSub, &hkeySub);

                if (dwError == ERROR_SUCCESS)
                {
                    LPTSTR szPSExe = TEXT("\\Powershell.exe");

                    cbValue = (MAX_PATH - lstrlen(szPSExe)) * sizeof(TCHAR);
					// BUGBUG: RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ is failed under Windows 7
                    dwError = RegGetValue(hkeySub, TEXT("PowerShellEngine"), TEXT("ApplicationBase"), RRF_RT_ANY, &dwType, (PVOID)szPSPath, &cbValue);

                    if (dwError == ERROR_SUCCESS && ((dwType & RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ) != 0) )
                    {
                        lstrcat(szPSPath, szPSExe);
                    }
                    else
                    {
                        // reset to empty string if not successful
                        szPSPath[0] = TEXT('\0');
                    }

                    RegCloseKey(hkeySub);
                }
            }
        }
    }

    RegCloseKey(hkey);

    // return true if we got a valid path
    return szPSPath[0] != TEXT('\0');
}

BOOL WINAPI GetBashExePath(LPTSTR szBashPath, UINT bufSize)
{
	const TCHAR szBashFilename[] = TEXT("bash.exe");
	UINT len;

	len = GetSystemDirectory(szBashPath, bufSize);
	if ((len != 0) && (len + _countof(szBashFilename) + 1 < bufSize) && PathAppend(szBashPath, TEXT("bash.exe")))
	{
		if (PathFileExists(szBashPath))
			return TRUE;
	}

	// If we are running 32 bit Winfile on 64 bit Windows, System32 folder is redirected to SysWow64, which
	// doesn't include bash.exe. So we also need to check Sysnative folder, which always maps to System32 folder.
	len = ExpandEnvironmentStrings(TEXT("%SystemRoot%\\Sysnative\\bash.exe"), szBashPath, bufSize);
	if (len != 0 && len <= bufSize)
	{
		return PathFileExists(szBashPath);
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//  GetPathFromAppPaths()
//
//---------------------------------------------------------------------------
EXTERN_C
LONG
WINAPI
GetPathFromAppPaths(
	PCWSTR pszName,
	PWSTR pszPath,
	DWORD cchPath,
	BOOL bExpand,
	PWSTR pszStartupDir,
	DWORD cchStartupDir,
	BOOL bExpandStartupDir
	)
{
	static PCWSTR pszSubKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths";
	HKEY hKey;
	LONG lResult;
	LONG cchResult = 0;
	HRESULT hr = E_FAIL;

	lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,pszSubKey,0,KEY_READ,&hKey);

	if( lResult == 0 )
	{
		HKEY hSubKey;
		lResult = RegOpenKeyEx(hKey,pszName,0,KEY_READ,&hSubKey);

		if( lResult == 0 )
		{
			DWORD RegType;
			DWORD cch;
			PWSTR pBuffer;

			//
			// Image Path
			//
			cch = 0;
			RegQueryValueEx(hSubKey,L"",NULL,&RegType,NULL,&cch);

			pBuffer = (PWSTR)_MemAlloc( cch  );

			if( pBuffer )
			{
				RegQueryValueEx(hSubKey,L"",NULL,&RegType,(LPBYTE)pBuffer,&cch);

				if( RegType == REG_EXPAND_SZ && bExpand )
				{
					ExpandEnvironmentStrings(pBuffer,pszPath,cchPath);
				}
				else
				{
					StringCchCopy(pszPath,cchPath,pBuffer);
				}

				cchResult = (int)wcslen(pszPath);

				_MemFree(pBuffer);
			}

			//
			// Startup Path
			//
			if( pszStartupDir )
			{
				cch = 0;
				RegQueryValueEx(hSubKey,L"Path",NULL,&RegType,NULL,&cch);

				pBuffer = (PWSTR)_MemAlloc( cch  );

				if( pBuffer )
				{
					RegQueryValueEx(hSubKey,L"Path",NULL,&RegType,(LPBYTE)pBuffer,&cch);

					if( RegType == REG_EXPAND_SZ && bExpandStartupDir )
					{
						ExpandEnvironmentStrings(pBuffer,pszStartupDir,cchStartupDir);
					}
					else
					{
						StringCchCopy(pszStartupDir,cchStartupDir,pBuffer);
					}

					_MemFree(pBuffer);
				}
			}

			RegCloseKey(hSubKey);
		}

		RegCloseKey(hKey);
	}

	return cchResult;
}

EXTERN_C
HRESULT
WINAPI
OpenTerminal(
	HWND hwndOwner,
	PCWSTR pszPath
	)
{
	HRESULT hr;

	LPITEMIDLIST pidlAppsFolder = NULL;
	hr = SHGetKnownFolderIDList(FOLDERID_AppsFolder,0,0,&pidlAppsFolder);
	if( hr != S_OK )
	{
		return hr;
	}

	IShellItem *pAppFolder = NULL;
	if( (hr = SHCreateItemFromIDList(pidlAppsFolder,IID_IShellItem,(void **)&pAppFolder)) == S_OK )
	{
		IShellItem *Terminal;
		if( (hr = SHCreateItemFromRelativeName(pAppFolder,
					L"Microsoft.WindowsTerminal_8wekyb3d8bbwe!App",
					NULL,IID_IShellItem,(void **)&Terminal)) == S_OK )
		{
			IPersistIDList *pPersistIDList;
			if( (hr = Terminal->QueryInterface(IID_IPersistIDList,(void **)&pPersistIDList)) == S_OK )
			{
				LPITEMIDLIST pidlApp = NULL;
				if( (hr = pPersistIDList->GetIDList( &pidlApp )) == S_OK )
				{
					WCHAR szParam[MAX_PATH];
					if( (hr = StringCchPrintf(szParam,MAX_PATH,L"-d %s",pszPath)) == S_OK )
					{
						hr = OpenShellItemILEx(hwndOwner,pidlApp,szParam,NULL);
					}

					ILFree(pidlApp);
				}
				pPersistIDList->Release();
			}
			Terminal->Release();
		}
		pAppFolder->Release();
	}

	ILFree(pidlAppsFolder);

	return hr;
}

DWORD ExecProgram(HWND hwndFrame,LPTSTR lpPath, LPTSTR lpParms, LPTSTR lpDir, BOOL bLoadIt, BOOL bRunAs)
{
    DWORD   ret;
    HCURSOR hCursor;
    LPTSTR  lpszTitle;

    ret = 0;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Set title to file spec only
    // Note: this is set to the app, so
    // drag, drop, execute title shows app, not file

    // Set up title
    for (lpszTitle=lpPath+lstrlen(lpPath); *lpszTitle != L'\\' && *lpszTitle != L':' &&
         lpszTitle != lpPath; lpszTitle --)
         ;

    // If we encountered a \ or : then skip it

    if (lpszTitle != lpPath)
         lpszTitle++;

    ret = (DWORD)ShellExecute(hwndFrame, bRunAs ? L"runas" : NULL, lpPath, lpParms, lpDir, bLoadIt ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL);

    SetCursor(hCursor);

    return ret;
}

//
// Image List Helper
//
static HIMAGELIST m_himl = NULL;
static int m_iImageUpDir = I_IMAGENONE;

INT WINAPI GetUpDirImageIndex()
{
	return m_iImageUpDir;
}

HIMAGELIST WINAPI GetGlobalShareImageList(	int ImageList )
{
	if( m_himl == NULL )
	{
		//
		// The image lists retrieved through this function are 
		// global system image lists;
		// do not call ImageList_Destroy using them.
		//
		Shell_GetImageLists(NULL,&m_himl);

#ifdef _DEBUG
		int cImages;
		cImages = ImageList_GetImageCount(m_himl);
		_TRACE("System image count=%u\n",cImages);
#endif

		int cx,cy;
		ImageList_GetIconSize(m_himl,&cx,&cy);

		HICON hIcon = (HICON)LoadImage(GetModuleHandle(L"shell32"), MAKEINTRESOURCE(46), IMAGE_ICON, cx, cy, 0);
		m_iImageUpDir = ImageList_AddIcon(m_himl,hIcon);
		DestroyIcon(hIcon);
	}
	return m_himl;
}

int
WINAPI 
GetShellFileIconImageListIndexEx(
	PCWSTR pszPath, // Fully Qualified File/Directory Path
	PCWSTR pszFileName,
	DWORD dwFileAttributes,
	DWORD dwFlags, /* Reserved */
	HICON *phIcon  /* Reserved */
	)
{
	SHFILEINFO sfi = {0};
	int iImage = I_IMAGENONE;

	if( pszFileName && wcscmp(pszFileName,L"..") == 0 )
	{
		iImage = GetUpDirImageIndex();
	}
	else if( pszPath )
	{
		PCWSTR pszDosPath = NULL;
		if( iswalpha(pszPath[0]) && pszPath[1] == L':' )
		{
			pszDosPath = pszPath;
		}
		else if( pszPath[0] == L'\\' && pszPath[1] == L'?' && pszPath[2] == L'?' && pszPath[3] == L'\\' )
		{
			if( iswalpha(pszPath[4]) && pszPath[5] == L':' )
				pszDosPath = &pszPath[4];
			else
				; // todo:
		}
			
		if( pszDosPath )
		{
			UINT fOverlay = 0;

			fOverlay |= SHGFI_OVERLAYINDEX;

			if( SHGetFileInfo(pszDosPath,dwFileAttributes,&sfi,sizeof(sfi),SHGFI_ICON|SHGFI_SMALLICON|SHGFI_SYSICONINDEX|fOverlay) != 0 )
			{
				if( sfi.hIcon != NULL )
					DestroyIcon(sfi.hIcon);

				iImage = sfi.iIcon;
			}
		}
	}

	if( iImage == I_IMAGENONE )
	{
		if( *pszFileName == L'\0' )
		{
			SHSTOCKICONINFO sii = {sizeof(sii)};
			SHGetStockIconInfo(SIID_DRIVEFIXED,SHGSI_SYSICONINDEX|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
			iImage = sii.iSysImageIndex;
			if( sii.hIcon )
				DestroyIcon(sii.hIcon);
		}
		else if( pszFileName[0] == L'\\' && pszFileName[1] == L'\0' )
		{
			SHSTOCKICONINFO sii = {sizeof(sii)};
			SHGetStockIconInfo(
					(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? SIID_FOLDER : SIID_DOCNOASSOC,
					SHGSI_SYSICONINDEX|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
			iImage = sii.iSysImageIndex;
			if( sii.hIcon )
				DestroyIcon(sii.hIcon);
		}
		else
		{
			UINT fOverlay = 0;
			if( dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
				fOverlay |= (SHGSI_LINKOVERLAY|SHGFI_OVERLAYINDEX);

			SHGetFileInfo(PathFindFileName(pszFileName),dwFileAttributes,&sfi,sizeof(sfi),
					SHGFI_ICON|SHGFI_SMALLICON|SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES|fOverlay);

			iImage = sfi.iIcon;

			if( sfi.hIcon != NULL )
				DestroyIcon(sfi.hIcon);
		}
	}

	return iImage;
}

int WINAPI GetShellFileImageListIndex(PCWSTR pszPath,PCWSTR pszFileName,DWORD dwFileAttributes)
{
	return GetShellFileIconImageListIndexEx(pszPath,pszFileName,dwFileAttributes,0,NULL);
}
