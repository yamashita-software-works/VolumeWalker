//*****************************************************************************
//
//  filelist_filesearch.cpp
//
//  PURPOSE: Search files on the volume.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-04-18 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "stdafx.h"
#include "..\libntwdk\libntwdk.h"
#include "..\libntwdk\ntnativeapi.h"
#include "..\libntwdk\ntobjecthelp.h"
#include "..\libntwdk\ntwin32helper.h"
#include "..\fsfilelib\fileitemlist.h"
#include "filelist_directorytraverse.h"
#include "filelist_filesearch.h"
#include "resource.h"

#define _DEBUG_SEARCH_RESULT_OUTPUT 0

#define UAID_CONTINUE   (0)
#define UAID_OK         (IDOK)
#define UAID_SKIP       (IDIGNORE)
#define UAID_APPLY_ALL  (1001)
#define UAID_RETRY      (IDRETRY)
#define UAID_CANCEL     (IDCANCEL)

typedef struct _SEARCH_CALLBACK_PARAM
{
	PWSTR pszSearchPath;
	ULONG SearchDirCount;
	ULONG SearchFileCount;	
	ULONG FindFileCount;
	ULONG DirectoryLevel;
	HWND hWndDialog;
	BOOL bAbort;
	BOOL bLimitReached;
	SEARCH_PARAMETER *SearchParam;
	HANDLE hMatchedFiles;
	DWORD FileSystemFlags;
	WCHAR FileSystemName[32];
} SEARCH_CALLBACK_PARAM;

typedef struct _SEARCH_THREAD_CONTEXT
{
	HWND hwndDialog;
	FS_SELECTED_FILELIST *FileList;
	HANDLE hThread;
	DWORD dwThreadId;
	SEARCH_CALLBACK_PARAM Search;
} SEARCH_THREAD_CONTEXT;

typedef struct _SEARCH_DIALOG
{
	FS_SELECTED_FILELIST *FileList;
	SEARCH_PARAMETER *SearchParam;
	SEARCH_THREAD_CONTEXT *ThreadContext;
} SEARCH_DIALOG;

#ifdef _DEBUG
#define _DEBUG_STAGE_TRACE(s,pcfi)\
				OutputDebugString( s );\
					OutputDebugString( pcfi->Path );\
					OutputDebugString( L"\n" );\
					OutputDebugString( pcfi->FileName );\
					OutputDebugString( L"\n" );\
					OutputDebugString( pcfi->RelativePath );\
					OutputDebugString( L"\n" );\
				OutputDebugString( L"\n" );
#else
#define _DEBUG_STAGE_TRACE __noop
#endif

//////////////////////////////////////////////////////////////////////////////
// Message UI

static INT DirectoryTooDeepMessage(HWND hwndOwner)
{
	int SelectedButtonId = 0;

	TASKDIALOGCONFIG tdc = {0};
	const TASKDIALOG_BUTTON tb[] = { 
		{UAID_SKIP ,        L"Continue"},
		{UAID_CANCEL ,      L"Abort"},
	}; 

	tdc.cbSize             = sizeof(tdc);
	tdc.hwndParent         = hwndOwner;
	tdc.dwFlags            = TDF_ALLOW_DIALOG_CANCELLATION |
				             TDF_POSITION_RELATIVE_TO_WINDOW;
	tdc.hInstance          = _GetResourceInstance();
    tdc.pButtons           = tb;
    tdc.cButtons           = ARRAYSIZE(tb);
	tdc.cxWidth            = 320; // DLU,todo:
	tdc.pszMainInstruction = L"Directory Hierarchy Too Deep";
	tdc.pszContent         = L"The directory hierarchy is too deep. Can't continue searching items contained in deeply directory.";
	tdc.pszWindowTitle     = L"File Search";

	TaskDialogIndirect(&tdc,&SelectedButtonId,NULL,NULL);

	return SelectedButtonId;
}

//////////////////////////////////////////////////////////////////////////////
// Search Callback

inline DWORD CompareFileAttributes(CALLBACK_FILE_INFORMATION *pcfi,SEARCH_PARAMETER *SearchParam)
{
	BOOL bMatched;
	if( SearchParam->CompareFlag & SEARCH_FLAG_ATTR_MATCH_WHOLE_BITS )
		bMatched = ( SearchParam->FileAttributes == pcfi->Information->FileAttributes );
	else
		bMatched = ( SearchParam->FileAttributes & pcfi->Information->FileAttributes ) ? TRUE : FALSE;

	return bMatched ? SEARCH_FLAG_ATTRIBUTE : 0;
}

inline DWORD CompareLargeIntegerRange(LARGE_INTEGER li,SEARCH_RANGE_VALUE *Range,int ValueType)
{
	return( ((Range->From.QuadPart <= li.QuadPart) && (li.QuadPart <= Range->To.QuadPart)) ? ValueType : 0 );
}

static 
SEARCH_FLAGS
CompareFileDetailInformation(
	SEARCH_FLAGS CompareFlag,
	DWORD FileAttributes,
	PCWSTR pszFileName,
	PCWSTR pszCurDir
	)
{
	NTSTATUS Status;
	DWORD dwMatched = 0;

	PWSTR RootDirectory=NULL,RootRelativePath=NULL;
	ULONG cchRootDirectory=0,cchRootRelativePath=0;

	if( pszCurDir == NULL || *pszCurDir == L'\0' )
		SplitRootPath_W(pszFileName,&RootDirectory,&cchRootDirectory,&RootRelativePath,&cchRootRelativePath);
	else if( pszCurDir )
		SplitRootPath_W(pszCurDir,&RootDirectory,&cchRootDirectory,&RootRelativePath,&cchRootRelativePath);
	else
		return 0;

	HANDLE hRootDirectory = NULL;
	if( OpenRootDirectory(RootDirectory,0,&hRootDirectory) != STATUS_SUCCESS )
	{
		//
		// The Root directory open failed. If so try open directory using
		// full path string without splitting the volume and root relative path.
		//
		FreeMemory(RootRelativePath);
		RootRelativePath = DuplicateString(pszCurDir); // duplicate full-path string
		hRootDirectory = NULL;
	}

	HANDLE hCurDir;
	if( (Status = OpenFile_W(&hCurDir,hRootDirectory,RootRelativePath,FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_DIRECTORY_FILE)) == 0 )
	{
		HANDLE hFile;
		ULONG DesiredAccess = FILE_READ_ATTRIBUTES|SYNCHRONIZE;
		ULONG Option = FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_SYNCHRONOUS_IO_NONALERT;
		if( OpenFile_W(&hFile,hCurDir,pszFileName,DesiredAccess,FILE_SHARE_READ|FILE_SHARE_WRITE,Option) == 0 )
		{
			// Alternate Strem
			if( CompareFlag & SEARCH_FLAG_ALT_STREAM )
			{
				INT AltStreamCount = 0;
				FILE_STREAM_INFORMATION *StreamInformation;
				if( GetAlternateStreamInformation(hFile,&AltStreamCount,&StreamInformation) == S_OK )
				{
					if( AltStreamCount > 0 )
					{
						FILE_STREAM_INFORMATION *p = StreamInformation;

						do
						{
							if( memcmp(L"::$DATA",p->StreamName,p->StreamNameLength) != 0 ) // no name (default) stream
							{
								dwMatched |= SEARCH_FLAG_ALT_STREAM;
								break;
							}

							if( p->NextEntryOffset == 0 )
								break;

							p = (FILE_STREAM_INFORMATION *)((ULONG_PTR)p + p->NextEntryOffset);
						}
						while( p != NULL );
					}

					FreeAlternateStreamInformation(StreamInformation);
				}
			}

			// todo: ...

			CloseHandle(hFile);
		}
		CloseHandle(hCurDir);
	}

	FreeMemory(RootRelativePath);
	FreeMemory(RootDirectory);

	if( hRootDirectory )
		CloseHandle(hRootDirectory);

	return dwMatched;
}

static HRESULT CALLBACK SearchFileCallbackProc(ULONG CallbackReason,PVOID CallbackData,PVOID Context)
{
	CALLBACK_FILE_INFORMATION *pcfi = (CALLBACK_FILE_INFORMATION *)CallbackData;
	SEARCH_CALLBACK_PARAM *pscp = (SEARCH_CALLBACK_PARAM *)Context;
	HRESULT hr = S_OK;

	if( pscp->bAbort )
		return S_FALSE;

	PWSTR pszDestinationFullQualifyFile = NULL;

	switch( CallbackReason )
	{
		case FFCBR_DIRECTORYSTART:
		{
			pscp->DirectoryLevel++;
			if( pscp->DirectoryLevel == pscp->SearchParam->MaxDirectoryLevel )
			{
				if( DirectoryTooDeepMessage(pscp->hWndDialog) == UAID_CANCEL )
					return S_FALSE;
				return S_FO_DIRECTORY_HIERARCHY_TOO_DEEP;
			}
			break;
		}
		case FFCBR_DIRECTORYEND:
		{
			pscp->DirectoryLevel--;
			break;
		}
		case FFCBR_FINDFILE:
		{
			if( pcfi->Information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				pscp->SearchDirCount++;
			else
				pscp->SearchFileCount++;

			pszDestinationFullQualifyFile = CombinePath(pcfi->Path,pcfi->FileName);
			if( pszDestinationFullQualifyFile )
			{
				SEARCH_FLAGS dwMatched = 0;

				if( PathMatchSpecEx(pcfi->FileName,pscp->SearchParam->Name,PMSF_MULTIPLE|PMSF_DONT_STRIP_SPACES) == S_OK )
				{
					//
					// If CompareFlag is zero,
					//
					if( pscp->SearchParam->CompareFlag == 0 )
						dwMatched |= 0x1; // file name is matched

					//
					// Comparing file attributes, if flag on.
					//
					dwMatched |= ((pscp->SearchParam->CompareFlag & SEARCH_FLAG_ATTRIBUTE) 
								? CompareFileAttributes(pcfi,pscp->SearchParam) : 0);

					//
					// Comparing file date time, if flag on.
					//
					dwMatched |= ((pscp->SearchParam->CompareFlag & SEARCH_FLAG_DATETIME_LASTWRITE) 
								? CompareLargeIntegerRange(pcfi->Information->LastWriteTime,&pscp->SearchParam->DateTime.LastWrite,SEARCH_FLAG_DATETIME_LASTWRITE) : 0);

					dwMatched |= ((pscp->SearchParam->CompareFlag & SEARCH_FLAG_DATETIME_CREATION) 
								? CompareLargeIntegerRange(pcfi->Information->CreationTime,&pscp->SearchParam->DateTime.Creation,SEARCH_FLAG_DATETIME_CREATION) : 0);

					dwMatched |= ((pscp->SearchParam->CompareFlag & SEARCH_FLAG_DATETIME_LASTACCESS) 
								? CompareLargeIntegerRange(pcfi->Information->ChangeTime,&pscp->SearchParam->DateTime.LastAccess,SEARCH_FLAG_DATETIME_LASTACCESS) : 0);

					//
					// Comparing file size, if flag on.
					//
					dwMatched |= ((pscp->SearchParam->CompareFlag & SEARCH_FLAG_ENDOFFILE) 
								? CompareLargeIntegerRange(pcfi->Information->EndOfFile,&pscp->SearchParam->EndOfFile,SEARCH_FLAG_ENDOFFILE) : 0);

					dwMatched |= ((pscp->SearchParam->CompareFlag & SEARCH_FLAG_ALLOCATIONSIZE) 
								? CompareLargeIntegerRange(pcfi->Information->AllocationSize,&pscp->SearchParam->AllocateSize,SEARCH_FLAG_ALLOCATIONSIZE) : 0);

					//
					// Comparing other file information, if flag on.
					//
					if( pscp->SearchParam->CompareFlag & SEARCH_FLAG_ALT_STREAM )
					{
						dwMatched |= CompareFileDetailInformation(pscp->SearchParam->CompareFlag,pcfi->Information->FileAttributes,pcfi->FileName,pcfi->Path);
					}

					//
					// File matched
					//
					if( dwMatched != 0 )
					{
						FILEITEMEX *pItemEx = AllocateFileItemEx(NULL,0);

						pItemEx->hdr.FileName   = (PWSTR)StrDup(pcfi->FileName);
						pItemEx->hdr.Path       = (PWSTR)StrDup(pcfi->Path);
						pItemEx->FileAttributes = pcfi->Information->FileAttributes;
						pItemEx->LastWriteTime  = pcfi->Information->LastWriteTime;
						pItemEx->CreationTime   = pcfi->Information->CreationTime;
						pItemEx->LastAccessTime = pcfi->Information->LastAccessTime;
						pItemEx->ChangeTime     = pcfi->Information->ChangeTime;
						pItemEx->EndOfFile      = pcfi->Information->EndOfFile;
						pItemEx->AllocationSize = pcfi->Information->AllocationSize;
						pItemEx->EaSize         = pcfi->Information->EaSize;
						pItemEx->FileId         = pcfi->Information->FileId;
						pItemEx->FileIndex      = pcfi->Information->FileIndex;
						if( pcfi->Information->ShortNameLength != 0 )
						{
							memcpy(pItemEx->ShortName,pcfi->Information->ShortName,pcfi->Information->ShortNameLength);
						}
						
						GetFileItemDetailInformation(pItemEx,pcfi->Path);

						int iIndex = FILAddItemExPtr( pscp->hMatchedFiles, pItemEx );

						if( iIndex == -1 )
						{
							// fatal error
							pscp->bAbort = TRUE;
						}

						pscp->FindFileCount++;
					}
				}
				FreeMemory(pszDestinationFullQualifyFile);
			}

			if( pscp->SearchParam->MaxFoundItemCount != 0 )
			{
				if( pscp->FindFileCount >= pscp->SearchParam->MaxFoundItemCount )
				{
					pscp->bLimitReached = TRUE;
					pscp->bAbort = TRUE;
				}
			}

			break;
		}
	}

	return pscp->bAbort ? S_FALSE : S_OK;
}

//----------------------------------------------------------------------------
//
//  Search_ThreadProc()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
DWORD WINAPI Search_ThreadProc(LPVOID lpParameter)
{
	SEARCH_THREAD_CONTEXT *pThreadContext = (SEARCH_THREAD_CONTEXT *)lpParameter;

	FS_SELECTED_FILELIST *pFiles = pThreadContext->FileList;
	SEARCH_PARAMETER *SearchParam = pThreadContext->Search.SearchParam;

	FS_FILELISTBUFFER *pFileList = pFiles->FileListBuffer;

	HWND hDialog  = pThreadContext->hwndDialog;

	PCWSTR TargetFullQualifyPath = NULL;
	DWORD dwError = 0;
	ULONG MaxDirectoryLevel = 1024; // todo:

	int i;
	for(i = 0; i < (int)pFileList->Count; i++)
	{
		FS_FLITEM& fi = pFileList->File[i];

		if( fi.Flags & FLI_FLG_NAME_OFFSET )
		{
			TargetFullQualifyPath = GetFileNamePtr(pFiles->FileListBuffer,&fi);
		}
		else
		{
			TargetFullQualifyPath = fi.Name;
		}

		//
		// Get volume information
		//
		DWORD FileSystemFlags = 0;
		HANDLE hDir;
		NTSTATUS Status;
		if( (Status = OpenFileEx_W(&hDir,TargetFullQualifyPath,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_DIRECTORY_FILE|FILE_OPEN_FOR_BACKUP_INTENT)) == 0 )
		{
			GetVolumeInformationByHandleW(hDir,
					nullptr,0,
					nullptr,
					nullptr,&pThreadContext->Search.FileSystemFlags,
					pThreadContext->Search.FileSystemName,
					ARRAYSIZE(pThreadContext->Search.FileSystemName));

			CloseHandle(hDir);
		}

		pThreadContext->Search.pszSearchPath = (PWSTR)TargetFullQualifyPath;
		pThreadContext->Search.SearchParam->MaxDirectoryLevel = MaxDirectoryLevel;

		BOOL bDirectory = IsDirectory( TargetFullQualifyPath );
		if( bDirectory )
		{
			HRESULT hr;
			//
			// Directory with Recursive Enumeration
			//
			hr = RecursiveEnumDirectoryFiles(TargetFullQualifyPath,NULL,NULL,0,&SearchFileCallbackProc,&pThreadContext->Search);
			if( hr != S_OK )
			{
				NTSTATUS Status = hr & ~FACILITY_NT_BIT;
				if( Status == STATUS_CANCELLED )
					dwError = ERROR_CANCELLED;
				else
					dwError = NtStatusToDosError( Status );
				break;
			}
			else
			{
				dwError = ERROR_SUCCESS;
			}
		}
		else
		{
			//
			// Found a File 
			//
			dwError = ERROR_SUCCESS;
		}

		if( dwError == ERROR_CANCELLED )
		{
			break;
		}
	}

	//
	// Notify thread end to primary thread.
	// Because this is an asynchronous call, you must use PostMessage.
	//
	PostMessage(hDialog,PM_FODLG_ENDOPERATION,dwError,0);

	return dwError;
}

SEARCH_THREAD_CONTEXT *CreateSearchThread(HWND hwndDialog,SEARCH_DIALOG *psd)
{
	SEARCH_THREAD_CONTEXT *pThreaParam = new SEARCH_THREAD_CONTEXT;

	ZeroMemory(pThreaParam,sizeof(SEARCH_THREAD_CONTEXT));

	pThreaParam->hwndDialog = hwndDialog;
	pThreaParam->Search.hWndDialog = hwndDialog;
	pThreaParam->FileList = psd->FileList;
	pThreaParam->Search.SearchParam = psd->SearchParam;

	pThreaParam->Search.hMatchedFiles = FILCreate( 256 );

	pThreaParam->hThread = CreateThread(NULL,0,&Search_ThreadProc,pThreaParam,CREATE_SUSPENDED,&pThreaParam->dwThreadId);

	return pThreaParam;
}

//////////////////////////////////////////////////////////////////////////////
// Search Progress Dialog

namespace SearchProgressDialog
{
	INT_PTR OnInitDialog(HWND hDlg,UINT,WPARAM,LPARAM lParam)
	{
		_CenterWindow(hDlg,GetActiveWindow());

		SEARCH_DIALOG *psd = (SEARCH_DIALOG *)lParam;

		psd->ThreadContext = CreateSearchThread(hDlg,psd);

		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)psd);

		return TRUE;
	}

	INT_PTR OnDestroy(HWND hDlg,UINT,WPARAM,LPARAM)
	{
		SEARCH_DIALOG *psd = (SEARCH_DIALOG *)GetWindowLongPtr(hDlg,DWLP_USER);
		return 0;
	}

	INT_PTR OnShowWindow(HWND hDlg,UINT,WPARAM wParam,LPARAM lParam)
	{
		if( wParam )
		{
			SEARCH_DIALOG *psd = (SEARCH_DIALOG *)GetWindowLongPtr(hDlg,DWLP_USER);
			ResumeThread( psd->ThreadContext->hThread );
			SetTimer(hDlg,1,500,NULL);
		}
		return 0;
	}

	INT_PTR OnTimer(HWND hDlg,UINT,WPARAM wParam,LPARAM lParam)
	{
		SEARCH_DIALOG *psd = (SEARCH_DIALOG *)GetWindowLongPtr(hDlg,DWLP_USER);
		WCHAR sz[MAX_PATH];
		StringCchPrintf(sz,MAX_PATH,L"%u directories, %u files searched, %u files found.",
			psd->ThreadContext->Search.SearchDirCount,
			psd->ThreadContext->Search.SearchFileCount,
			psd->ThreadContext->Search.FindFileCount);
		SetDlgItemText(hDlg,IDC_TEXT,sz);
		return 0;
	}

	INT_PTR OnCommand(HWND hDlg,UINT,WPARAM wParam,LPARAM lParam)
	{
		SEARCH_DIALOG *psd = (SEARCH_DIALOG *)GetWindowLongPtr(hDlg,DWLP_USER);
		if( LOWORD(wParam) == IDCANCEL )
		{
			psd->ThreadContext->Search.bAbort = TRUE;
		}
		return 0;
	}

	INT_PTR OnEndOperation(HWND hDlg,UINT,WPARAM wParam,LPARAM lParam)
	{
		SEARCH_DIALOG *psd = (SEARCH_DIALOG *)GetWindowLongPtr(hDlg,DWLP_USER);

		WaitForSingleObject(psd->ThreadContext->hThread,INFINITE);

		DWORD dwThreadExitCode;
		GetExitCodeThread(psd->ThreadContext->hThread,&dwThreadExitCode);

		CloseHandle( psd->ThreadContext->hThread );
		psd->ThreadContext->hThread = NULL;


		if( psd->ThreadContext->Search.bLimitReached )
		{
			MsgBox(hDlg,L"Found files are reached to limit on search result.",L"Simple Search",MB_OK|MB_ICONINFORMATION);
		}

		EndDialog(hDlg,IDOK);

		return 0;
	}

	INT_PTR
	CALLBACK
	DlgProc(
		HWND hwndDlg,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam
		)
	{
		switch( uMsg )
		{
			case WM_INITDIALOG:
				return OnInitDialog(hwndDlg,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hwndDlg,uMsg,wParam,lParam);
			case WM_SHOWWINDOW:
				return OnShowWindow(hwndDlg,uMsg,wParam,lParam);
			case WM_TIMER:
				return OnTimer(hwndDlg,uMsg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hwndDlg,uMsg,wParam,lParam);
			case PM_FODLG_ENDOPERATION:
				return OnEndOperation(hwndDlg,uMsg,wParam,lParam);
			case WM_CLOSE:
				EndDialog(hwndDlg,IDCLOSE);
				break;
		}
		return 0;
	}
};

#if _DEBUG_SEARCH_RESULT_OUTPUT
void DebugSearchResultOutput(HANDLE Handle)
{
	int cItems = FILGetItemCount(Handle);
	for(int i = 0; i < cItems; i++)
	{
#if 0
		FILEITEM fi;
		
		FILGetItem(Handle,i,&fi);

		OutputDebugString( fi.hdr.FileName );
		OutputDebugString( L"\r\n" );
#else
		FILEITEMEX *pItemEx = FILGetItemExPtr(Handle,i);

		OutputDebugString( pItemEx->hdr.FileName );
		OutputDebugString( L"\r\n" );
#endif
	}
}
#endif

static VOID InitSearchParameter(SEARCH_PARAMETER *psp)
{
	SYSTEMTIME stFrom,stTo;

	ZeroMemory(&stFrom,sizeof(SYSTEMTIME));
	stFrom.wYear  = 1980; // todo: MS-DOS start date
	stFrom.wDay   = 1;
	stFrom.wMonth = 1;

	GetSystemTime(&stTo);

	InitDateTimeRange(&psp->DateTime.LastWrite,&stFrom,&stTo);
	InitDateTimeRange(&psp->DateTime.Creation,&stFrom,&stTo);
	InitDateTimeRange(&psp->DateTime.LastAccess,&stFrom,&stTo);
	InitDateTimeRange(&psp->DateTime.Change,&stFrom,&stTo);
}

//----------------------------------------------------------------------------
//
//  Search()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------

SEARCH_PARAMETER g_sp = {0}; // todo:

HRESULT
Search(
	HWND hWnd,
	FS_SELECTED_FILELIST *FileList,
	HANDLE *pHandleMatchedFiles
	)
{
	HRESULT hr;
	SEARCH_PARAMETER sp = {0};
	SEARCH_DIALOG sd = {0};

	if( g_sp.Name[0] == 0 )
		InitSearchParameter(&sp);
	else
		sp = g_sp;

	sd.FileList = FileList; 
	sd.SearchParam = &sp;

	if( (hr = SearchParamDialog(hWnd,FileList,&sp)) == S_OK )
	{
		INT_PTR lResult;
		lResult = DialogBoxParam(_GetResourceInstance(),MAKEINTRESOURCE(IDD_SEARCH),hWnd,SearchProgressDialog::DlgProc,(LPARAM)&sd);

		if( lResult == IDOK )
		{
			HANDLE Handle = sd.ThreadContext->Search.hMatchedFiles;
			delete sd.ThreadContext;

			if( Handle != NULL )
			{
#if _DEBUG_SEARCH_RESULT_OUTPUT
				DebugSearchResultOutput(Handle);
#endif
				g_sp = sp;

				*pHandleMatchedFiles = Handle;
				hr = S_OK;
			}
			else
			{
				*pHandleMatchedFiles = NULL;
				hr = E_FAIL;
			}
		}
		else
		{
			*pHandleMatchedFiles = NULL;
			hr = S_FALSE;
		}
	}
	return hr;
}
