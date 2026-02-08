//***************************************************************************
//*                                                                         *
//*  dllmain.cpp                                                            *
//*                                                                         *
//*  DLL entry point                                                        *
//*                                                                         *
//*  Create: 2023-07-11                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "fsvolumefilelist.h"
#include "ntobjecthelp.h"
#if _ENABLE_DARK_MODE_TEST
#include "darkmode.h"
#pragma comment(lib, "uxtheme.lib")
#endif

#define UILAYOUT_IMPL
#include "UILayout.h"

#include "interface.h"
#include "pagewbdbase.h"
#include "page_volumefilelist.h"
#include "page_volumefilelist_search_result.h"
#include "filelistview.h"

HINSTANCE hInstance = NULL;

HINSTANCE _GetResourceInstance()
{
	return hInstance;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			hInstance = hModule;
			_MemInit();
			InitializeLibMisc(hInstance,GetUserDefaultUILanguage());
			InitLongPathBox(hInstance);
#if _ENABLE_FILE_MANAGER || _ENABLE_FILELIST_DRAGFILE || _ENABLE_FILELIST_DROPFILE
			InitializeLongPathClipboard();
#endif
			break;
		case DLL_PROCESS_DETACH:
			_MemEnd();
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

EXTERN_C
HRESULT
WINAPI
InitializeVolumeFilesConsole(
	DWORD dwFlags
	)
{
#if _ENABLE_DARK_MODE_TEST
	if( dwFlags & VOLUME_DLL_FLAG_ENABLE_DARK_MODE )
	{
		InitDarkMode();
		EnableDarkMode(TRUE);
		return S_OK;
	}
	return E_FAIL;
#else
	return E_NOTIMPL;
#endif
}

HICON SetWindowIcon(HWND hWnd,SHSTOCKICONID ssii)
{
    SHSTOCKICONINFO sii = {0};
	sii.cbSize = sizeof(sii);
	SHGetStockIconInfo(ssii,SHGSI_ICON|SHGSI_SMALLICON|SHGSI_SHELLICONSIZE,&sii);
	DestroyIcon((HICON)SendMessage(hWnd,WM_GETICON,ICON_SMALL,0));
	SendMessage(hWnd,WM_SETICON,ICON_SMALL,(LPARAM)sii.hIcon);
	return sii.hIcon;
}

HFONT GetGlobalFont(HWND hWnd)
{
	HFONT hFont = NULL;
	HDC hdc = GetWindowDC(hWnd);
	LOGFONT lf = {0};
	lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	lf.lfCharSet = ANSI_CHARSET;
	StringCchCopy(lf.lfFaceName,_countof(lf.lfFaceName),L"Consolas");
	hFont = CreateFontIndirect( &lf );
	ReleaseDC(hWnd,hdc);
	return hFont;
}

HFONT GetIconFont()
{
	HFONT hFontIcon;
	LOGFONT lf;
	SystemParametersInfo(SPI_GETICONTITLELOGFONT,sizeof(LOGFONT),&lf,0);
	hFontIcon = CreateFontIndirect(&lf);
	return hFontIcon;
}

FS_CLUSTER_INFORMATION *_CreateClusterInformationBuffer(PCWSTR pszFilePath)
{
	NTSTATUS Status;
	UNICODE_STRING usFileName;
	DWORD dwError;

	if( IsRootDirectory_W(pszFilePath) )
	{
		usFileName.Buffer =  L"";
	}
	else
	{
		SplitPathFileName_W(pszFilePath,NULL,&usFileName);
	}

	PWSTR Path;
	Path = DuplicateString(pszFilePath);
	if( Path == NULL )
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return NULL;
	}

	FS_CLUSTER_INFORMATION *pci = NULL;
	PWSTR RootDirectory=NULL,RootRelativePath=NULL;
	ULONG cchRootDirectory=0,cchRootRelativePath=0;
	PWSTR FileName = NULL;
	RemoveFileSpec(Path);
	SplitRootPath_W(Path,&RootDirectory,&cchRootDirectory,&RootRelativePath,&cchRootRelativePath);
	FreeMemory(Path);

	if( RootDirectory && RootRelativePath )
	{
		HANDLE hRootDirectory = NULL;
		if( OpenRootDirectory(RootDirectory,0,&hRootDirectory) != STATUS_SUCCESS )
		{
			//
			// The Root directory open failed. If so try open directory using
			// full path string without splitting the volume and root relative path.
			//
			FreeMemory(RootRelativePath);
			RootRelativePath = NULL;
			hRootDirectory = NULL;
		}
	
		HANDLE hCurDir;
		if( (Status = OpenFile_W(&hCurDir,hRootDirectory,RootRelativePath,FILE_READ_ATTRIBUTES|SYNCHRONIZE,
							FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_DIRECTORY_FILE)) == STATUS_SUCCESS )
		{
			HANDLE hFile;
			ULONG DesiredAccess = FILE_READ_ATTRIBUTES|SYNCHRONIZE;
			ULONG Option = FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_SYNCHRONOUS_IO_NONALERT;
			if( (Status = OpenFile_W(&hFile,hCurDir,usFileName.Buffer,DesiredAccess,FILE_SHARE_READ|FILE_SHARE_WRITE,Option)) == STATUS_SUCCESS )
			{
				dwError = ReadFileClusterInformaion(NULL,hFile,RootDirectory,ClusterInformationAll,&pci,sizeof(pci));
	
				CloseHandle(hFile);
			}
			else
			{
				dwError = NtStatusToDosError(Status);
			}
	
			CloseHandle(hCurDir);
		}
		else
		{
			dwError = NtStatusToDosError(Status);
		}

		if( hRootDirectory )
			CloseHandle(hRootDirectory);
	}
	else
	{
		dwError = ERROR_OUTOFMEMORY;
	}

	FreeMemory(RootRelativePath);
	FreeMemory(RootDirectory);

	SetLastError(dwError);

	return pci;
}


HRESULT
GetAlternateStream(
	PCWSTR pszFilePath,
	VFS_FILE_STREAM_INFORMATION **pAltStmNames,
	INT *pAltStmNameCount
	)
{
	NTSTATUS Status;
	HRESULT hr;
	DWORD dwMatched = 0;
	HANDLE hFile;
	ULONG DesiredAccess = FILE_READ_ATTRIBUTES|SYNCHRONIZE;
	ULONG Option = FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_SYNCHRONOUS_IO_NONALERT;

	if( (Status = OpenFileEx_W(&hFile,pszFilePath,DesiredAccess,FILE_SHARE_READ|FILE_SHARE_WRITE,Option)) == STATUS_SUCCESS )
	{
		INT AltStreamCount = 0;
		FILE_STREAM_INFORMATION *StreamInformation;

		Status = GetAlternateStreamInformation(hFile,&AltStreamCount,&StreamInformation);

		if( Status == STATUS_SUCCESS )
		{
			if( AltStreamCount > 0 )
			{
				ULONG cbNameBuffer = GetAlternateStreamNameTotalLength(StreamInformation);

				ULONG cbBuffer = (sizeof(VFS_FILE_STREAM_INFORMATION) * AltStreamCount) + cbNameBuffer + sizeof(WCHAR);
				PBYTE pb;
				pb = (PBYTE)CoTaskMemAlloc( cbBuffer );
				ZeroMemory(pb,cbBuffer);

				FILE_STREAM_INFORMATION *p = StreamInformation;
				int iIndex = 0;
				VFS_FILE_STREAM_INFORMATION *pAltName = (VFS_FILE_STREAM_INFORMATION *)pb;
				WCHAR *pNameStorePos = (WCHAR *)(pb + (sizeof(VFS_FILE_STREAM_INFORMATION) * AltStreamCount));
				do
				{
					pAltName[iIndex].StreamSize = p->StreamSize;
					pAltName[iIndex].StreamAllocationSize = p->StreamAllocationSize;
					memcpy(pNameStorePos,p->StreamName,p->StreamNameLength);
					pAltName[iIndex].StreamName = pNameStorePos;
					pAltName[iIndex].Order = iIndex;
					iIndex++;

					pNameStorePos += (WCHAR_LENGTH( p->StreamNameLength) + 1);

					if( p->NextEntryOffset == 0 )
						break;

					p = (FILE_STREAM_INFORMATION *)((ULONG_PTR)p + p->NextEntryOffset);
				}
				while( p != NULL );

				*pAltStmNames = pAltName;
				*pAltStmNameCount = iIndex;
			}

			FreeAlternateStreamInformation(StreamInformation);

			hr = S_OK;
		}
		else
		{
			hr = HRESULT_FROM_WIN32( NtStatusToDosError(Status) );
		}

		CloseHandle(hFile);
	}
	else
	{
		hr = HRESULT_FROM_WIN32( NtStatusToDosError(Status) );
	}

	return hr;
}
