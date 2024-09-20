//***************************************************************************
//*                                                                         *
//*  runhelp.h                                                              *
//*                                                                         *
//*  PURPOSE:  Open/Run helper class.                                       *
//*                                                                         *
//*  AUTHOR:   YAMASHITA Katsuhiro                                          *
//*                                                                         *
//*  HISTORY:  2024/08/08                                                   *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#pragma once

#include "stringbuffer.h"

// Private environment variables name 
#define EnvStrCurDir            L"CURDIR"
#define EnvStrFilePath          L"_FILEPATH_"
#define EnvStrFileName          L"_FILENAME_"

#define RHCF_MASK_PATH           0x00000001
#define RHCF_MASK_DOSPATH        0x00000002
#define RHCF_MASK_DIRECTORY      0x00000004
#define RHCF_MASK_FILENAME       0x00000008
#define RHCF_MASK_EXECPATH       0x00000010
#define RHCF_MASK_CURDIR         0x00000020
#define RHCF_MASK_FILELIST       0x00000040
#define RHCF_MASK_PARAM          0x00000080

class CRunHelp
{
private:
	VOID InitPrivateEnvironmentVariables(PCWSTR pszExecPath,PCWSTR pszDosPath,PCWSTR pszCurDir)
	{
		CStringBuffer szCurDir(MAX_PATH);
		CStringBuffer szExePath(MAX_PATH);

		// current directory
		GetCurrentDirectory(MAX_PATH,szCurDir);

		ExpandEnvironmentStrings(pszExecPath,szExePath,MAX_PATH);
		PathRemoveArgs(szExePath);
		PathUnquoteSpaces(szExePath);
		PathRemoveFileSpec(szExePath);

		if( pszCurDir )
			SetEnvironmentVariable(EnvStrCurDir,pszCurDir);
		else
			SetEnvironmentVariable(EnvStrCurDir,szExePath);

		// path,filename
		SetEnvironmentVariable(EnvStrFilePath,pszDosPath);
		SetEnvironmentVariable(EnvStrFileName,PathFindFileName(pszDosPath));
	}

	VOID FreeLocalEnvironmentVariables()
	{
		SetEnvironmentVariable(EnvStrCurDir,NULL);
		SetEnvironmentVariable(EnvStrFilePath,NULL);
		SetEnvironmentVariable(EnvStrFileName,NULL);
	}

	DWORD GetAppPathsPath(PCWSTR ExeName,PWSTR ExtendedPath,DWORD cchExtendedPath)
	{
		HKEY hKey;
		TCHAR szKeyPath[MAX_PATH];
		DWORD dwRet;
		DWORD cb;
		DWORD dwType;

		wcscpy_s(szKeyPath,MAX_PATH,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
		wcscat_s(szKeyPath,MAX_PATH,ExeName);

		if( (dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					   szKeyPath,
					   0,
					   KEY_READ,&hKey)) == ERROR_SUCCESS )
		{
			cb = cchExtendedPath * sizeof(WCHAR);

			if( (dwRet = RegQueryValueEx(hKey,L"Path",0,&dwType,(LPBYTE)ExtendedPath,&cb)) == 0 )
			{
				; // succeeded
			}
			else
			{
				if( dwRet != ERROR_MORE_DATA )
					cb = 0;
			}
			RegCloseKey(hKey);
		}
		else
		{
			cb = 0;
		}

		SetLastError(dwRet);

		return (cb/sizeof(WCHAR));
	}

public:
	typedef struct _RUN_COMMAND_PARAM {
		ULONG mask;
		PCWSTR pszPath;
		PCWSTR pszDosPath;
		PCWSTR pszDirectoy;
		PCWSTR pszFileName;
		PCWSTR pszExecPath;
		PCWSTR pszCmdLine;
		PCWSTR pszStartupDirectory;
		PCWSTR pszAppendPath;
		PCWSTR pszCurDir;
		PCWSTR pszCurDirNtDevice; // Reserved
		PCWSTR pszCurDirGuid;     // Reserved
		PCWSTR pszEnvironment;    // Reserved
		ULONG cFileNameList;      // Reserved
		PCWSTR *pFileNameList;    // Reserved
		ULONG_PTR Param;          // Reserved
	} RUN_COMMAND_PARAM;

	LRESULT RunCommand(int cmd,RUN_COMMAND_PARAM *pcf)
	{
		CStringBuffer szStartupDirectory(MAX_PATH);
		CStringBuffer szCmdLine(MAX_PATH);
		CStringBuffer szExecLocationPath(MAX_PATH);
		CStringBuffer szExePath(MAX_PATH);
		HRESULT hr = E_FAIL;

		InitPrivateEnvironmentVariables(pcf->pszExecPath,pcf->pszDosPath,pcf->pszCurDir);

		// Block:
		{
			if( pcf->pszExecPath == NULL || pcf->pszCmdLine == NULL)
			{
				ExpandEnvironmentStrings(pcf->pszExecPath,szExePath,MAX_PATH);
							PathRemoveArgs(szExePath);
				PathUnquoteSpaces(szExePath);
							StringCchCopy(szExecLocationPath,MAX_PATH,szExePath);
				PathRemoveFileSpec(szExecLocationPath);
							if( pcf->pszStartupDirectory )
				ExpandEnvironmentStrings(pcf->pszStartupDirectory,szStartupDirectory,MAX_PATH);
				if( ExpandEnvironmentStrings(
						pcf->pszExecPath,
						szCmdLine,MAX_PATH) > 0 )
				{
					PWSTR pszCompleteCmdLine = NULL;
					DWORD_PTR arg[1];
					arg[0] = (DWORD_PTR)pcf->pszDosPath;
					FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
								szCmdLine,0,0,(LPWSTR)&pszCompleteCmdLine,0,(va_list *)arg);
					if( pszCompleteCmdLine != NULL )
					{
						CStringBuffer sbNewEnvPath(32768);
						CStringBuffer sbOrgEnvPath(32768);
						GetEnvironmentVariable(L"PATH",sbOrgEnvPath,32767);

						// Get path from registry "App Paths" and append to %PATH% environment variable.
						WCHAR szExtendedPath[MAX_PATH];
						if( GetAppPathsPath(PathFindFileName(szExePath),szExtendedPath,_countof(szExtendedPath)) > 0 )
						{
							StringCchCopy(sbNewEnvPath,32767,sbOrgEnvPath);
							wcscat_s(sbNewEnvPath,32767,L";");
							wcscat_s(sbNewEnvPath,32767,szExtendedPath);
							SetEnvironmentVariable(L"PATH",sbNewEnvPath);
						}

						// Set path from XML parameter and  and append to %PATH% environment variable.
						if( pcf->pszAppendPath != NULL && *pcf->pszAppendPath != L'\0' )
						{
							StringCchCopy(sbNewEnvPath,32767,sbOrgEnvPath);
							if( *pcf->pszAppendPath != L';' )
								wcscat_s(sbNewEnvPath,32767,L";");
							wcscat_s(sbNewEnvPath,32767,pcf->pszAppendPath);
							SetEnvironmentVariable(L"PATH",sbNewEnvPath);
						}

						hr = Execute(
								NULL,
								pszCompleteCmdLine,
								szStartupDirectory.IsEmpty() ? NULL : szStartupDirectory.c_str()
								);

						if( !sbOrgEnvPath.IsEmpty() )
						{
							// Revert environment %PATH% string
							SetEnvironmentVariable(L"PATH",sbOrgEnvPath);
						}

						LocalFree(pszCompleteCmdLine);
					}
				}
			}
			else if( pcf->pszExecPath != NULL )
			{
				hr = Execute(pcf->pszExecPath,pcf->pszDosPath,
							szStartupDirectory.IsEmpty() ? NULL : szStartupDirectory.c_str());
			}
			else if( pcf->pszCmdLine != NULL )
			{
				hr = Execute(NULL,pcf->pszCmdLine,
							szStartupDirectory.IsEmpty() ? NULL : szStartupDirectory.c_str());
			}
		}

		FreeLocalEnvironmentVariables();

		return 0;
	}

	HRESULT
	Execute(
		PCWSTR pszApplication,
		PCWSTR pszCommandLine,
		PCWSTR pszCurrentDir,
		PCWSTR pEnvironment = NULL,
		DWORD dwCreationFlags = 0
		)
	{
		STARTUPINFO si = {0};
		PROCESS_INFORMATION pi = {0};
		HRESULT hr;
		BOOL bSuccess;

		bSuccess = CreateProcess(pszApplication,(LPWSTR)pszCommandLine,
						NULL,NULL,FALSE,dwCreationFlags,(PVOID)pEnvironment,pszCurrentDir,&si,&pi);

		hr = bSuccess ? S_OK : HRESULT_FROM_WIN32(GetLastError());

		if( bSuccess )
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		return hr;
	}

	HRESULT
	ShellExecute(
		PCWSTR pszExePath,
		PCWSTR pszCommandLine,
		PCWSTR pszCurrentDir
		)
	{
		SHELLEXECUTEINFO sei = {0};

		LPITEMIDLIST pidl = ILCreateFromPath(pszExePath);

		sei.cbSize = sizeof(sei);
		sei.fMask = SEE_MASK_IDLIST;
		sei.lpIDList = pidl;
		sei.lpDirectory = pszCurrentDir;
		sei.lpParameters = pszCommandLine;
		sei.lpVerb = L"open";
		sei.nShow  = SW_SHOWNORMAL;

		BOOL bSuccess;
		bSuccess = ShellExecuteEx( &sei );

		ILFree(pidl);

		return bSuccess ? S_OK : HRESULT_FROM_WIN32( GetLastError() );
	}

	HRESULT
	RunAdmin(
		PCWSTR pszExePath,
		PCWSTR pszCommandLine,
		PCWSTR pszCurrentDir
		)
	{
		SHELLEXECUTEINFO sei = {0};

		sei.cbSize = sizeof(sei);
		sei.fMask = SEE_MASK_IDLIST;
		sei.lpFile = pszExePath;
		sei.lpDirectory = pszCurrentDir;
		sei.lpParameters = pszCommandLine;
		sei.lpVerb = L"runas";
		sei.nShow  = SW_SHOWNORMAL;

		BOOL bSuccess;
		bSuccess = ShellExecuteEx( &sei );

		return bSuccess ? S_OK : HRESULT_FROM_WIN32( GetLastError() );
	}

	//----------------------------------------------------------------------------
	//
	//  RunCommandPrompt()
	//
	//----------------------------------------------------------------------------
	HRESULT RunCommandPrompt(PCWSTR pszDirPath,BOOL bAdmin)
	{
		HRESULT hr = E_FAIL;
		//cmd.exe /s /k pushd "<dirpath>"
		WCHAR szExePath[MAX_PATH];
		WCHAR szCmdLine[MAX_PATH];

		PWSTR pFilePart = NULL;
		if( SearchPath(NULL,L"cmd.exe",NULL,MAX_PATH,szExePath,&pFilePart) != 0 )
		{
			StringCchPrintf(szCmdLine,MAX_PATH,L"/s /k pushd \"%s\"",pszDirPath);

			if( bAdmin )
				hr = RunAdmin(szExePath,szCmdLine,NULL);
			else
				hr = Execute(szExePath,szCmdLine,NULL,NULL,0);
		}

		return hr;
	}

	//----------------------------------------------------------------------------
	//
	//  RunPowerShell()
	//
	//----------------------------------------------------------------------------

	#define POWERSHELLPARAMFORMAT TEXT(" -noexit -command \"cd \\\"%s\\\"\"")

	HRESULT RunPowerShell(PCWSTR pszDirPath,BOOL bAdmin)
	{
		HRESULT hr = E_FAIL;

		WCHAR szPSPath[MAX_PATH];
		GetPowershellExePath(szPSPath);

		WCHAR szCmdLine[MAX_PATH];
		StringCchPrintf(szCmdLine,MAX_PATH,POWERSHELLPARAMFORMAT,pszDirPath);

		if( bAdmin )
			hr = RunAdmin(szPSPath,szCmdLine,NULL);
		else
			hr = Execute(szPSPath,szCmdLine,NULL,NULL,0);

		return hr;
	}

	//----------------------------------------------------------------------------
	//
	//  RunTerminal()
	//
	//----------------------------------------------------------------------------

	#define TERMINALPARAMFORMAT   TEXT(" -d \"%s\"")
	#define TERMINALPARAMFORMATBS TEXT(" -d \"%s\\\"")

	HRESULT RunTerminal(PCWSTR pszDirPath,BOOL bAdmin)
	{
		HRESULT hr = E_FAIL;

		WCHAR szAppDataLocal[MAX_PATH];
		SHGetFolderPath(NULL,CSIDL_LOCAL_APPDATA,NULL,SHGFP_TYPE_CURRENT,szAppDataLocal);

		// wt.exe - Windows Terminal location
		// ex) "C:\Users\MyName\AppData\Local\Microsoft\WindowsApps\wt.exe"
		WCHAR szWinTerminalPath[MAX_PATH];
		PathCombine(szWinTerminalPath,szAppDataLocal,L"Microsoft\\WindowsApps\\wt.exe");

		WCHAR szCmdLine[MAX_PATH];
		if( IsLastCharacterBackslash(pszDirPath) )
		{
			// if tail character backslash, must need to last double backslash.
			// (If not specified, it is parsed as an escape character on wt.exe)
			//
			// If you want unconditionally remove the backslash, must consider
			// to root directory.
			StringCchPrintf(szCmdLine,MAX_PATH,TERMINALPARAMFORMATBS,pszDirPath);
		}
		else
		{
			StringCchPrintf(szCmdLine,MAX_PATH,TERMINALPARAMFORMAT,pszDirPath);
		}

		if( bAdmin )
			hr = RunAdmin(szWinTerminalPath,szCmdLine,NULL);
		else
			hr = Execute(szWinTerminalPath,szCmdLine,NULL,NULL,0);

		return hr;
	}
};
