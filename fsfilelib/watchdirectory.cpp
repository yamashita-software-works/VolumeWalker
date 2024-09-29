//***************************************************************************
//*                                                                         *
//*  watchdirectory.cpp                                                     *
//*                                                                         *
//*  Directory watch functions.                                             *
//*                                                                         *
//*  Create: 2024-01-12                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "libntwdk.h"
#include "ntnativeapi.h"
#include "ntobjecthelp.h"
#include "fsfilelib.h"
#include "ntnotifychangedirectory.h"

#define EVENT_CHANGE_DIRECTORY   (WAIT_OBJECT_0+0)
#define EVENT_EXIT               (WAIT_OBJECT_0+1)
#define EVENT_DIRECTORY          (WAIT_OBJECT_0+2)

#define HANDLE_CHANGE_DIRECTORY  (EVENT_CHANGE_DIRECTORY-WAIT_OBJECT_0)
#define HANDLE_EXIT              (EVENT_EXIT-WAIT_OBJECT_0)
#define HANDLE_DIRECTORY         (EVENT_DIRECTORY-WAIT_OBJECT_0)

#define HANDLE_COUNT             3

typedef NTSTATUS (NTAPI *PFN_NTNOTIFYCHANGEDIRECTORYFILEEX)(
    __in HANDLE FileHandle,
    __in_opt HANDLE Event,
    __in_opt PIO_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcContext,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __in ULONG CompletionFilter,
    __in BOOLEAN WatchTree,
    __in DIRECTORY_NOTIFY_INFORMATION_CLASS DirectoryNotifyInformationClass
    ); 

DWORD WINAPI _DirectoryWatchThread(PVOID pv)
{
	IO_STATUS_BLOCK IoStatusBlock = {0};
	LONG Status;
	ULONG NotifyFilter = 0xFFF;
	HANDLE hWaitHandles[HANDLE_COUNT];
	ULONG cHandles;
	DWORD dwWakeupEvent;
	BOOL bLoop = TRUE;

	DIRECTORY_WATCH_STRUCT *pWatch = (DIRECTORY_WATCH_STRUCT *)pv;

	PFN_NTNOTIFYCHANGEDIRECTORYFILEEX pfnNtNotifyChangeDirectoryFileEx = NULL;

	if( pWatch->InformationClass != DirectoryWatchNotifyInformation )
	{
		pfnNtNotifyChangeDirectoryFileEx = (PFN_NTNOTIFYCHANGEDIRECTORYFILEEX)GetProcAddress(GetModuleHandle(L"ntdll.dll"),"NtNotifyChangeDirectoryFileEx");

		if( pfnNtNotifyChangeDirectoryFileEx == NULL )
		{
			return GetLastError();
		}
	}

	while( bLoop )
	{
		cHandles = 0;

		// Change watch directory event.
		hWaitHandles[cHandles++] = pWatch->hChgDirEvent;

		// Exit watch event.
		hWaitHandles[cHandles++] = pWatch->hExitEvent;

		// Watch directory.
		if( pWatch->hDirectoryHandle != NULL )
		{
			IoStatusBlock.Status      = 0;
			IoStatusBlock.Pointer     = 0;
			IoStatusBlock.Information = 0;

			if( pfnNtNotifyChangeDirectoryFileEx && (pWatch->InformationClass != DirectoryWatchNotifyInformation) )
			{
				Status = pfnNtNotifyChangeDirectoryFileEx(
							pWatch->hDirectoryHandle,
							NULL, // Event Handle
							NULL, // ApcRoutine
							NULL, // ApcContext
							&IoStatusBlock,
							pWatch->pNotifyBuffer,
							pWatch->cbNotifyBufferSize,
							NotifyFilter,
							FALSE,
							(DIRECTORY_NOTIFY_INFORMATION_CLASS)pWatch->InformationClass
							);
			}
			else
			{
				Status = NtNotifyChangeDirectoryFile(
							pWatch->hDirectoryHandle,
							NULL, // Event Handle
							NULL, // ApcRoutine
							NULL, // ApcContext
							&IoStatusBlock,
							pWatch->pNotifyBuffer,
							pWatch->cbNotifyBufferSize,
							NotifyFilter,
							FALSE
							);
			}

			if( Status == STATUS_PENDING || Status == STATUS_SUCCESS )
				hWaitHandles[cHandles++] = pWatch->hDirectoryHandle;
		}

		// Wait event
		dwWakeupEvent = WaitForMultipleObjects(cHandles,hWaitHandles,FALSE,INFINITE);

		switch( dwWakeupEvent )
		{
			case EVENT_CHANGE_DIRECTORY:
				pWatch->EnterChangeDirectorySection = TRUE;
				WaitForSingleObject(pWatch->hChangedUp,INFINITE);
				pWatch->EnterChangeDirectorySection = FALSE;
				break;
			case EVENT_DIRECTORY:
				if( IoStatusBlock.Information > 0 && pWatch->pfnNotifyCallback )
				{
					DIRWATCHNOTIFYEVENT event;
					event.InformationClass = DirectoryNotifyExtendedInformation;
					event.pNotifyBuffer = (FILE_NOTIFY_INFORMATION*)pWatch->pNotifyBuffer;
					event.cbNotifyBufferLength = (ULONG)IoStatusBlock.Information;
					event.Context = pWatch->NotifyCallbackContext;
					pWatch->pfnNotifyCallback(&event);
				}
				break;
			case EVENT_EXIT:
				bLoop = FALSE;
				break;
			default:
				bLoop = FALSE; // Loop out with fatal error.
				break;
		}
	}

	return 0;
}

EXTERN_C
HRESULT
WINAPI
StartDirectoryWatchEx(
	HANDLE *pHandle,
	UINT InformationClass,
	PFNDIRWATCHNOTIFYPROC pfnNotifyCallback,
	PVOID Context
	)
{
	HRESULT hr;
	HANDLE hThread;
	DWORD ThreadId;
	DWORD dwError = 0;

	if( pHandle == NULL || pfnNotifyCallback == NULL )
	{
		return E_INVALIDARG;
	}

	if( !(DirectoryWatchNotifyInformation <= InformationClass && InformationClass < DirectoryWatchNotifyMaxInformation) )
	{
		return E_INVALIDARG;
	}

	if( InformationClass != DirectoryWatchNotifyInformation )
	{
		if( GetProcAddress(GetModuleHandle(L"ntdll.dll"),"NtNotifyChangeDirectoryFileEx") == NULL )
		{
			return E_INVALIDARG;
		}
	}

	DIRECTORY_WATCH_STRUCT *pWatch = new DIRECTORY_WATCH_STRUCT;
	if( pWatch == NULL )
	{
		return E_OUTOFMEMORY;
	}

	ZeroMemory(pWatch,sizeof(DIRECTORY_WATCH_STRUCT));

	__try
	{
		pWatch->hChgDirEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		if( pWatch->hChgDirEvent == NULL )
		{
			dwError = GetLastError();
			__leave;
		}

		pWatch->hExitEvent  = CreateEvent(NULL,FALSE,FALSE,NULL);
		if( pWatch->hExitEvent == NULL )
		{
			dwError = GetLastError();
			__leave;
		}

		pWatch->hChangedUp  = CreateEvent(NULL,FALSE,FALSE,NULL);
		if( pWatch->hChangedUp == NULL )
		{
			dwError = GetLastError();
			__leave;
		}

		hThread = CreateThread(NULL,0,&_DirectoryWatchThread,pWatch,CREATE_SUSPENDED,&ThreadId);

		if( hThread != NULL )
		{
			pWatch->hThread = hThread;
			pWatch->pfnNotifyCallback = pfnNotifyCallback;
			pWatch->NotifyCallbackContext = Context;
			pWatch->InformationClass = InformationClass;

			ResumeThread(pWatch->hThread);
		}
		else
		{
			dwError = GetLastError();
			__leave;
		}
	}
	__finally
	{
		if( dwError != 0 )
		{
			if( pWatch->hChgDirEvent )
				NtClose(pWatch->hChgDirEvent);
			if( pWatch->hExitEvent )
				NtClose(pWatch->hExitEvent);
			if( pWatch->hThread )
				NtClose(pWatch->hThread);
			delete pWatch;
			hr = HRESULT_FROM_WIN32(dwError);
		}
		else
		{
			*pHandle = (HANDLE)pWatch;
			hr = S_OK;
		}
		SetLastError(dwError);
	}

	return hr;
}

EXTERN_C
HRESULT
WINAPI
StartDirectoryWatch(
    HANDLE *pHandle,
    PFNDIRWATCHNOTIFYPROC pfnNotifyCallback,
    PVOID Context
    )
{
	return StartDirectoryWatchEx(pHandle,DirectoryWatchNotifyInformation,pfnNotifyCallback,Context);
}

inline BOOL __CloseHandle(HANDLE& Handle)
{
	if( CloseHandle(Handle) )
	{
		Handle = NULL;
		return TRUE;
	}
	return FALSE;
}	

EXTERN_C
HRESULT
WINAPI
StopDirectoryWatch(
	HANDLE Handle
	)
{
	if( Handle == NULL )
		return E_INVALIDARG;

	DIRECTORY_WATCH_STRUCT *pWatch = (DIRECTORY_WATCH_STRUCT *)Handle;

	SetEvent( pWatch->hExitEvent );

	WaitForSingleObject(pWatch->hThread,INFINITE);

	__CloseHandle(pWatch->hChangedUp);
	__CloseHandle(pWatch->hChgDirEvent);
	__CloseHandle(pWatch->hExitEvent);
	__CloseHandle(pWatch->hThread);

	if( pWatch->hDirectoryHandle )
		__CloseHandle(pWatch->hDirectoryHandle);

	if( pWatch->pNotifyBuffer )
	{
		_MemFree( pWatch->pNotifyBuffer );
		pWatch->pNotifyBuffer = NULL;
		pWatch->cbNotifyBufferSize = 0;
	}

	delete pWatch;

	return S_OK;
}

EXTERN_C
HRESULT
WINAPI
SetWatchDirectory(
	HANDLE Handle,
	PCWSTR pszDirectory
	)
{
	DIRECTORY_WATCH_STRUCT *pWatch = (DIRECTORY_WATCH_STRUCT *)Handle;
	NTSTATUS Status;

	//
	// Request break directory watch thread.
	//
	SetEvent(pWatch->hChgDirEvent);

	//
	// Wait thread accept change directory.
	//
	while( pWatch->EnterChangeDirectorySection == 0 )
		SwitchToThread();

	//
	// Close previous watch directory handle.
	//
	if( pWatch->hDirectoryHandle )
	{
		CloseHandle(pWatch->hDirectoryHandle);
		pWatch->hDirectoryHandle = NULL;
	}

	if( pWatch->pNotifyBuffer )
	{
		_MemFree( pWatch->pNotifyBuffer );
		pWatch->pNotifyBuffer = NULL;
		pWatch->cbNotifyBufferSize = 0;
	}

	//
	// Change/Update watch directory.
	//
	if( pszDirectory != NULL )
	{
		Status = OpenFileEx_W(&pWatch->hDirectoryHandle,
						pszDirectory,
						FILE_LIST_DIRECTORY|SYNCHRONIZE,
						FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
						FILE_DIRECTORY_FILE);

		if( Status == STATUS_SUCCESS )
		{
			pWatch->cbNotifyBufferSize = _NT_PATH_FULL_LENGTH_BYTES * 8;
			pWatch->cbNotifyBufferSize = ((pWatch->cbNotifyBufferSize + 1) / 4) * 4; // DWORD align
#if 0
			ULONG DeviceType;
			ULONG Characteristics;
			GetVolumeDeviceType(pWatch->hDirectoryHandle,&DeviceType,&Characteristics);
#endif
			pWatch->pNotifyBuffer  = _MemAllocZero( pWatch->cbNotifyBufferSize );
		}
	}
	else
	{
		// Stop directory watch, but still running a thread.
		Status = STATUS_SUCCESS;
	}

	//
	// Wakeup watch thread
	//
	SetEvent(pWatch->hChangedUp);

	return HRESULT_FROM_NT( Status );
}
