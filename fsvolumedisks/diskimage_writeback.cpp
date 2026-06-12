//*****************************************************************************
//
//  diskimage_writeback.cpp
//
//  PURPOSE: Restore raw disk image file.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-01-16 Created
//
//  -------------------------------------------------------------------------
//
//  WARNING:
//  This operation is very dangerous. If you do it easily, may destruct the
//  copy destination. 
//  Please do operation that must be confirmed destination drive/volume.
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "resource.h"
#include "fsvolumedisks.h"
#include "volumehelp.h"
#include "ntnativehelp.h"
#include "ntvolumehelp.h"
#include <diskguid.h>

static WCHAR *g_pszTitle = L"Write Back Image File";

#define SECTOR_SIZE 4096                       // todo:
#define BUFFER_SIZE ((SECTOR_SIZE * 16) * 64)  // todo:

enum {
	PhaseStandby=0,
	PhaseBiginProgress=1,
	PhaseProgress=2,
	PhaseEndProgress=3,
	PhaseShowWaitCompliteProgressBar = 4,
	PhaseShowErrorMessage,
	PhaseExitThread = 9,
};

typedef struct _WRITEBACKDISKIMAGE_THREAD_CONTEXT_BLOCK
{
	HANDLE hThread;
	DWORD dwThreadId;

	CRITICAL_SECTION  cs;

	HWND hwndTaskDialog;
	HWND hwndOwner;
	BOOL UserAbort;

	INT CopyStage;

    double progress;
	int WaitTime;

	HRESULT hr;

	WCHAR szDiskVolumeName[MAX_PATH];
	WCHAR szDiskImageFilePath[MAX_PATH];

} WRITEBACKDISKIMAGE_THREAD_CONTEXT_BLOCK;

typedef struct _WRITEBACKIMAGE_UI_THREAD_PARAM
{
	HWND hWnd;
	WCHAR szDiskVolumeName[MAX_PATH];
	WCHAR szDiskImageFilePath[MAX_PATH];
} WRITEBACKIMAGE_UI_THREAD_PARAM;

typedef struct _WRITEBACKDISKIMAGE_HANDLE_STRUCT
{
	HANDLE hTherad;
	DWORD dwThreadId;
} WRITEBACKDISKIMAGE_HANDLE_STRUCT;

#define DRT_DRIVELETTER      0x0001
#define DRT_VOLUME           0x0002
#define DRT_PHYSICALDRIVE    0x0004
#define DRT_CDROM            0x0008 
#define DRT_MASK             0x00ff

//      pszString                       LOWORD             HIWORD
// L"\\??\\C:"                      DRT_DRIVELETTER        0x43('C')
// L"\\??\\HarddiskVolume10"        DRT_VOLUME             0xA
// L"\\Device\\HarddiskVolume2"     DRT_VOLUME             0x2
// L"\\Device\\CdRom3"              DRT_CDROM              0x3
// L"\\??\\PhysicalDrive1"          DRT_PHYSICALDRIVE      0x1
// L"Z:"                            DRT_DRIVELETTER        0x5A('Z')
static DWORD _GetDriveTypeFromDeviceName( PCWSTR pszString )
{
	USHORT Number;
	WCHAR *p = NULL;

	if( pszString[1] == L':' )
	{
		WORD ch = LOWORD(CharUpper( (LPWSTR)pszString[0] ));
		if( IsCharAlpha( ch ) )
		{
			// "C:"
			return (DWORD)MAKELONG(DRT_DRIVELETTER,ch);
		}
	}

	if( _wcsnicmp(pszString,L"\\??\\",4) == 0 )
	{
		WORD ch = LOWORD(CharUpper( (LPWSTR)pszString[4] ));
		if( IsCharAlpha( ch ) && pszString[5] == L':' )
		{
			// "\??\C:"
			return (DWORD)MAKELONG(DRT_DRIVELETTER,ch);
		}

		p = (PWSTR)pszString + 4;

		if( _wcsnicmp(p,L"PhysicalDrive",13) == 0 )
		{
			p += 13;
			Number = (int)_wtoi(p);
			return (DWORD)MAKELONG(DRT_PHYSICALDRIVE,Number);
		}
	}
	else if( _wcsnicmp(pszString,L"\\Device\\",8) == 0 )
	{	
		p = (PWSTR)pszString + 8;
	}

	if( p )
	{
		if( _wcsnicmp(p,L"HarddiskVolume",14) == 0 )
		{
			p += 14;
			Number = (int)_wtoi(p);
			return (DWORD)MAKELONG(DRT_VOLUME,Number);
		}
		if( _wcsnicmp(p,L"CdRom",5) == 0 )
		{
			p += 5;
			Number = (int)_wtoi(p);
			return (DWORD)MAKELONG(DRT_CDROM,Number);
		}
	}

	return 0;
}

static HRESULT LockVolumesInDrive(HANDLE hDisk,DWORD DriveNumber)
{
    HRESULT hr;

    //
    // Volume Lock is required for restoring a physical drive.
    //
    PDRIVE_LAYOUT_INFORMATION_EX DriveLayoutBuffer = NULL;
    if( !GetDiskDriveLayoutEx(hDisk,&DriveLayoutBuffer) )
    {
  		hr = HRESULT_FROM_WIN32( GetLastError() );
    	return hr;
    }

	if( DriveLayoutBuffer->PartitionStyle != PARTITION_STYLE_RAW )
	{
		DWORD dwIndex;
		for(dwIndex = 0; dwIndex < DriveLayoutBuffer->PartitionCount; dwIndex++)
		{
			PARTITION_INFORMATION_EX *pi;

			pi = &DriveLayoutBuffer->PartitionEntry[ dwIndex ];

			BOOL bValidPartition = FALSE;
			if( pi->PartitionStyle == PARTITION_STYLE_MBR )
			{
				if( DriveLayoutBuffer->PartitionEntry[ dwIndex ].Mbr.PartitionType != PARTITION_ENTRY_UNUSED )
				{
					bValidPartition = TRUE;
				}
			}
			else if( pi->PartitionStyle == PARTITION_STYLE_GPT )
			{
				if( !IsEqualGUID(DriveLayoutBuffer->PartitionEntry[ dwIndex ].Gpt.PartitionType,PARTITION_ENTRY_UNUSED_GUID) )
				{
					bValidPartition = TRUE;
				}
			}
			else
			{
				hr = E_INVALIDARG;
				break;
			}

			if( bValidPartition ) 
			{
				WCHAR szPartitionName[32];
				StringCchPrintf(szPartitionName,ARRAYSIZE(szPartitionName),
                        L"\\??\\Harddisk%uPartition%u",DriveNumber,pi->PartitionNumber);

				HANDLE hVolume = CreateFile(
									szPartitionName,
									GENERIC_READ|GENERIC_WRITE,
									0,
									NULL,
									OPEN_EXISTING,
									FILE_FLAG_BACKUP_SEMANTICS,
									NULL
									);

                if( hVolume != INVALID_HANDLE_VALUE )
                {
					DWORD bytesReturned;

				    if (!DeviceIoControl(hVolume, FSCTL_LOCK_VOLUME, NULL, 0, NULL , 0, &bytesReturned, NULL))
				    {
				    	hr = HRESULT_FROM_WIN32( GetLastError() );
				    	break;
				    }

				    if (!DeviceIoControl(hVolume, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL , 0, &bytesReturned, NULL))
				    {
				    	hr = HRESULT_FROM_WIN32( GetLastError() );
				    	break;
				    }

					CloseHandle(hVolume);

					hr = S_OK;
                }
			}
		}
	}

	StorageMemFree(DriveLayoutBuffer);

    return hr;
}

HRESULT 
WINAPI
RestoreDiskImageFile(
	PCWSTR pszPhysicalDrive, // "\\.\PhysicalDrive0"
	PCWSTR pszImageFilePath, // "C:\foo\bar\disk_image.img"
	ULONG reserved,
	PVOID reserved_ptr,
	CREATEDISKIMAGEFILECALLBACK callback_proc,
	ULONG_PTR reserved_context
	)
{
	HRESULT hr = S_OK;
	HANDLE hDisk = NULL;
	HANDLE hFile = NULL;
	DWORD bytesRead = 0;
	DWORD bytesWritten = 0;
	DWORD bytesReturned;
	BYTE *buffer = NULL;
	GET_LENGTH_INFORMATION fileSize;
	GET_LENGTH_INFORMATION diskSize;
	LARGE_INTEGER processedSize = {0};
    int cVolumeHandleCount = 0;
    DWORD DriveType;
    DWORD DriveNumber;
	DWORD dwError = 0;

	buffer = (BYTE *)_MemAlloc(BUFFER_SIZE);
	if (!buffer)
	{
		return E_OUTOFMEMORY;
	}

	__try
	{
        //
        // Open Source Image File
        //
	    hFile = CreateFile(
					pszImageFilePath,
					GENERIC_READ|SYNCHRONIZE,
					0,
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_SEQUENTIAL_SCAN,
					NULL
					);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			hr = HRESULT_FROM_WIN32( GetLastError() );
			return hr;
		}

		GetFileSizeEx(hFile,&fileSize.Length);

        //
        // Open Destination Drive/Volume
        //
		hDisk = CreateFile(
					pszPhysicalDrive,
					GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
					0,
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_BACKUP_SEMANTICS,
					NULL
					);

		if (hDisk == INVALID_HANDLE_VALUE)
		{
			hr = HRESULT_FROM_WIN32( GetLastError() );
			__leave;
		}

        DWORD dwRet;
        dwRet = _GetDriveTypeFromDeviceName( pszPhysicalDrive );

        DriveType = LOWORD(dwRet);
        DriveNumber = HIWORD(dwRet);

        if( DriveType == DRT_PHYSICALDRIVE ) 
        {
			//
			// for PhysicalDrive
			//
            hr = LockVolumesInDrive(hDisk,DriveNumber);
            if( hr != S_OK )
            {
                __leave;
            }

			if (!DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &diskSize, sizeof(diskSize), &bytesReturned, NULL))
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
				__leave;
			}
		}
		else
		{
			//
			// for Volume/Drive
			//

			// Disk Length
			if (!DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &diskSize, sizeof(diskSize), &bytesReturned, NULL))
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
				__leave;
			}

			// Volume Length
			NTSTATUS Status;
			VOLUME_FS_SIZE_INFORMATION *Size;
			if( (Status = GetVolumeFsInformation(hDisk,VOLFS_SIZE_INFORMATION,(PVOID*)&Size)) == STATUS_SUCCESS )
			{
				diskSize.Length.QuadPart = Size->TotalAllocationUnits.QuadPart * Size->SectorsPerAllocationUnit * Size->BytesPerSector;
	
				FreeMemory(Size);
			}
			else
			{
				hr = HRESULT_FROM_WIN32( NtStatusToDosError(Status) );
				__leave;
			}

			if (!DeviceIoControl(hDisk, FSCTL_LOCK_VOLUME, NULL, 0, NULL , 0, &bytesReturned, NULL))
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
				__leave;
			}

			if (!DeviceIoControl(hDisk, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL , 0, &bytesReturned, NULL))
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
				__leave;
			}
		}

		if( diskSize.Length.QuadPart != fileSize.Length.QuadPart )
		{
			if( callback_proc )
			{
				if( !callback_proc(CDIFCB_Confirm,fileSize.Length,diskSize.Length,0,reserved_context) )
					__leave;
			}
			else
			{
				hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
				__leave;
			}
		}

		LONGLONG loop_count = fileSize.Length.QuadPart / BUFFER_SIZE;      // block copy r/w count
		LONGLONG last_block_size = fileSize.Length.QuadPart % BUFFER_SIZE; // last block r/w bytes
		LONGLONG i;
		DWORD ReadLength = BUFFER_SIZE;

		for(i = 0; i < loop_count; i++)
		{
			if( !ReadFile(hFile, buffer, ReadLength, &bytesRead, NULL) )
			{
 				hr = HRESULT_FROM_WIN32( GetLastError() );
				__leave;
			}

			if( bytesRead != ReadLength )
			{
 				hr = E_FAIL;
				__leave;
			}

			if( !WriteFile(hDisk, buffer, bytesRead, &bytesWritten, NULL ) )
			{
 				hr = HRESULT_FROM_WIN32( GetLastError() );
				__leave;
			}

			processedSize.QuadPart += bytesRead;

			if( callback_proc )
			{
				if( !callback_proc(CDIFCB_Progress,processedSize,fileSize.Length,0,reserved_context) )
					__leave;
			}
		}

		if( last_block_size > 0 )
		{
			//
			// Read remainder block size.
			//
			if( !ReadFile(hFile, buffer, (DWORD)last_block_size, &bytesRead, NULL) )
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
				__leave;
			}

			if( !WriteFile(hDisk, buffer, bytesRead, &bytesReturned, NULL) )
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
				__leave;
			}

			processedSize.QuadPart += bytesRead;
		}
	}
	__finally
	{
		FlushFileBuffers(hDisk);

		if( hFile != INVALID_HANDLE_VALUE )
			CloseHandle(hFile);

		if( hDisk != INVALID_HANDLE_VALUE )
			CloseHandle(hDisk);

		_MemFree(buffer);
	}

	return hr;
}

static
BOOL
CALLBACK
RestoreDiskimageProgressCallback(
	CALLBACKREASON call_reason,
	LARGE_INTEGER processedSize,
	LARGE_INTEGER diskSize,
	DWORD dwError,
	ULONG_PTR param
	)
{
	WRITEBACKDISKIMAGE_THREAD_CONTEXT_BLOCK *ptcb = (WRITEBACKDISKIMAGE_THREAD_CONTEXT_BLOCK *)param;

	if( call_reason == CDIFCB_Error )
	{
		; // Reserved
	}
	else if( call_reason == CDIFCB_Confirm )
	{
		WCHAR szMessage[128];
		WCHAR szFileSize[32],szDiskSize[32];
		_CommaFormatString(processedSize.QuadPart,szFileSize);
		_CommaFormatString(diskSize.QuadPart,szDiskSize);
		StringCchPrintf(szMessage,ARRAYSIZE(szMessage),
				L"Length missmatch.\n\n"
				L"File Size : %s\n"
				L"Disk Size : %s\n"
				L"Do you want to continue file write back?",
				szFileSize,szDiskSize);
		if( MsgBox(ptcb->hwndTaskDialog,szMessage,g_pszTitle,MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) != IDYES )
		{
			SendMessage(ptcb->hwndTaskDialog,TDM_SET_PROGRESS_BAR_STATE,PBST_ERROR,0);
			ptcb->UserAbort = TRUE;
			return FALSE;
		}
		return TRUE;
	}
	else if( call_reason == CDIFCB_Progress )
	{
		double progress;
		progress = (double)processedSize.QuadPart / diskSize.QuadPart * 100.0;

		EnterCriticalSection(&ptcb->cs);
		ptcb->progress = progress;
		LeaveCriticalSection(&ptcb->cs);
	}
	return( !ptcb->UserAbort ) ? TRUE : FALSE;
}

static
DWORD
WINAPI
RestoreDiskImageWorkerThread(
	LPVOID lpParameter
	)
{
	WRITEBACKDISKIMAGE_THREAD_CONTEXT_BLOCK *ptcb = (WRITEBACKDISKIMAGE_THREAD_CONTEXT_BLOCK *)lpParameter;

	ptcb->CopyStage = PhaseBiginProgress;

	ptcb->hr = RestoreDiskImageFile(
					ptcb->szDiskVolumeName,
					ptcb->szDiskImageFilePath,
					0,nullptr,
					&RestoreDiskimageProgressCallback,
					(ULONG_PTR)ptcb);

	ptcb->CopyStage = PhaseEndProgress;

	SendMessage(ptcb->hwndTaskDialog,TDM_CLICK_BUTTON,IDCLOSE,0); // send close task dialog

	return 0;
}

static void SetContentText(HWND hwnd,double progress)
{
	wchar_t buf[100];
	swprintf_s(buf,100,L"Writing... %.2f%%",progress);
	SendMessage(hwnd,TDM_SET_ELEMENT_TEXT,TDE_CONTENT,(LPARAM)buf);
}

static
HRESULT
CALLBACK
TaskDialogCallback(
	HWND hwnd,
    UINT uNotification,
    WPARAM wParam,
    LPARAM lParam,
    LONG_PTR dwRefData
	)
{
	WRITEBACKDISKIMAGE_THREAD_CONTEXT_BLOCK *ptcb = (WRITEBACKDISKIMAGE_THREAD_CONTEXT_BLOCK*)dwRefData;

	switch( uNotification )
	{
		case TDN_CREATED:
		{
			ptcb->hwndTaskDialog = hwnd;
			ResumeThread(ptcb->hThread);
			SendMessage(hwnd,TDM_SET_MARQUEE_PROGRESS_BAR,TRUE,0);
			break;
		}
		case TDN_BUTTON_CLICKED:
		{
			switch( wParam )
			{
				case IDABORT:
				{
					if( ptcb->CopyStage == PhaseShowErrorMessage )
						return S_OK; // close no confirm

					if( MsgBox(hwnd,L"Do you want to abort write back?",g_pszTitle,MB_YESNO|MB_ICONQUESTION) == IDYES )
					{
						ptcb->UserAbort = TRUE;
					}
					return S_FALSE; // do not close  dialog
				}
				case IDCLOSE:
				{
					if(ptcb->CopyStage != PhaseStandby && ptcb->CopyStage != PhaseExitThread)
						return S_FALSE;  // do not close dialog
					break;
				}
			}
			break;
		}
		case TDN_TIMER:
		{
			if( ptcb->CopyStage == PhaseBiginProgress )
			{
				SendMessage(hwnd,TDM_SET_PROGRESS_BAR_MARQUEE,FALSE,0);
				SendMessage(hwnd,TDM_SET_MARQUEE_PROGRESS_BAR,FALSE,0);
				SendMessage(hwnd,TDM_SET_PROGRESS_BAR_RANGE,0,MAKELPARAM(0,1000));
				ptcb->CopyStage = PhaseProgress;
			}
			else if( ptcb->CopyStage == PhaseProgress )
			{
				EnterCriticalSection(&ptcb->cs);
				double progress = ptcb->progress;
				LeaveCriticalSection(&ptcb->cs);

				SetContentText(hwnd,progress);

				int pct = (int)(progress * 10.0);
				SendMessage(hwnd,TDM_SET_PROGRESS_BAR_POS,(WPARAM)(int)pct,0);
			}
			else if( ptcb->CopyStage == PhaseEndProgress )
			{
				if( ptcb->hr == S_OK )
				{
					SetContentText(hwnd,100.0);
					SendMessage(hwnd,TDM_SET_PROGRESS_BAR_POS,(WPARAM)(int)1000,0);
					ptcb->WaitTime = 5;
					ptcb->CopyStage = PhaseShowWaitCompliteProgressBar;
				}
				else
				{
					PWSTR pErrorMessage = new WCHAR[ MAX_PATH ];
					if( pErrorMessage )
					{
						PWSTR pMessage;
						if( WinGetErrorMessage(ptcb->hr,&pMessage) > 0 )
						{
							StringCchPrintf(pErrorMessage,MAX_PATH,L"%s - 0x%08X",pMessage,ptcb->hr);

							WinFreeErrorMessage(pMessage);
						}

						SendMessage(hwnd,TDM_SET_ELEMENT_TEXT,TDE_CONTENT,(LPARAM)pErrorMessage);

						delete[] pErrorMessage;
					}

					SendMessage(hwnd,TDM_SET_PROGRESS_BAR_STATE,(WPARAM)PBST_ERROR,0);
					ptcb->CopyStage = PhaseShowErrorMessage;
				}
			}
			else if( ptcb->CopyStage == PhaseShowWaitCompliteProgressBar )
			{
				if( (ptcb->WaitTime--) == 0 )
					ptcb->CopyStage = PhaseExitThread;
			}
			else if( ptcb->CopyStage == PhaseExitThread )
			{
				SendMessage(hwnd,TDM_CLICK_BUTTON,IDCLOSE,0); // send close task dialog
			}
			break;
		}
	}
	return S_OK;
}

static
DWORD
WINAPI
RestoreDiskImageUIThread(
	LPVOID lpParameter
	)
{
	WRITEBACKIMAGE_UI_THREAD_PARAM *pcuiParam = (WRITEBACKIMAGE_UI_THREAD_PARAM *)lpParameter;

	WRITEBACKDISKIMAGE_THREAD_CONTEXT_BLOCK tcb = {0};
	tcb.CopyStage = PhaseStandby;
	tcb.hwndOwner = pcuiParam->hWnd;
	StringCchCopy(tcb.szDiskVolumeName,MAX_PATH,pcuiParam->szDiskVolumeName);
	StringCchCopy(tcb.szDiskImageFilePath,MAX_PATH,pcuiParam->szDiskImageFilePath);

	delete pcuiParam;

	InitializeCriticalSection( &tcb.cs );

	tcb.hThread = CreateThread(NULL,0,&RestoreDiskImageWorkerThread,&tcb,CREATE_SUSPENDED,&tcb.dwThreadId);

	TASKDIALOGCONFIG tdc = {0};
	const TASKDIALOG_BUTTON tb = { IDABORT , L"Abort" };

	tdc.cbSize = sizeof(tdc);
	tdc.hwndParent         = tcb.hwndOwner;
	tdc.dwFlags            = TDF_SHOW_PROGRESS_BAR | 
                             TDF_CALLBACK_TIMER |
                             TDF_ALLOW_DIALOG_CANCELLATION |
				             TDF_POSITION_RELATIVE_TO_WINDOW;
	tdc.hInstance          = _GetResourceInstance();
    tdc.pfCallback         = &TaskDialogCallback;
    tdc.lpCallbackData     = reinterpret_cast<LONG_PTR>(&tcb);
    tdc.pButtons           = &tb;
    tdc.cButtons           = 1;
	tdc.pszMainInstruction = L"Writing back Disk/Volume Image File";
	tdc.pszContent         = L"Preparing...";
	tdc.pszWindowTitle     = g_pszTitle;

	int SelectedButtonId = 0;
	TaskDialogIndirect(&tdc,&SelectedButtonId,NULL,NULL);

	DeleteCriticalSection(&tcb.cs);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void decorateVolumeName(PWSTR pszVolumeName)
{
	WCHAR *p = pszVolumeName;
	while( *p )
	{
		if( *p == L':' )
		{
			if( p > pszVolumeName )
			{
				*(p - 1) = (WCHAR)CharUpper( (PWSTR)*(p - 1) );
				break;
			}
		}
		p++;
	}
}

static
HRESULT
CALLBACK
ConfirmDialogCallback(
	HWND hwnd,
    UINT uNotification,
    WPARAM wParam,
    LPARAM lParam,
    LONG_PTR dwRefData
	)
{
	switch( uNotification )
	{
		case TDN_CREATED:
		{
			SendMessage(hwnd,TDM_ENABLE_BUTTON,IDYES,0);
			break;
		}
		case TDN_VERIFICATION_CLICKED:
		{
			SendMessage(hwnd,TDM_ENABLE_BUTTON,IDYES,wParam);
			break;
		}
	}
	return S_OK;
}

int ConfirmMessage(HWND hWnd,PWSTR pszVolumeDiskName)
{
	WCHAR *pConfirmMessage = new WCHAR [MAX_PATH];
	if( pConfirmMessage == NULL )
	{
		return E_OUTOFMEMORY;
	}

	WCHAR *pConfirmInstruction = new WCHAR [MAX_PATH];
	if( pConfirmInstruction == NULL )
	{
		delete[] pConfirmMessage;
		return E_OUTOFMEMORY;
	}

	StringCchPrintf(pConfirmInstruction,MAX_PATH,L"'%s' is will be overwritten",pszVolumeDiskName);

	WCHAR *pszMsgFmt = 
		L"The destination '%s' will be overwrite and contained data lost.\n"
		L"\n"
        L"- This operation will not verification and validation.\n"
		L"- This operation may fail.\n"
		L"- For large drive/volume, writes take a long time.\n"
		L"\n"
		L"Do you still want to write back?";

	StringCchPrintf(pConfirmMessage,MAX_PATH,pszMsgFmt,
			pszVolumeDiskName);

	decorateVolumeName(pConfirmInstruction);
	decorateVolumeName(pConfirmMessage);

	TASKDIALOGCONFIG tdc = {0};
	const TASKDIALOG_BUTTON tb[] = 
	{
		{ IDYES ,   L"Yes" },
		{ IDNO ,    L"No" },
		{ IDCANCEL, L"Cancel" },
	};
	BOOL VerificationFlag = FALSE;
	DWORD dw = 0;

	tdc.cbSize              = sizeof(tdc);
	tdc.hwndParent          = hWnd;
	tdc.dwFlags             = TDF_POSITION_RELATIVE_TO_WINDOW;
	tdc.hInstance           = _GetResourceInstance();
    tdc.pButtons            = tb;
    tdc.cButtons            = _countof(tb);
	tdc.nDefaultButton      = IDNO;
	tdc.pszMainInstruction  = pConfirmInstruction;
	tdc.pszContent          = pConfirmMessage;
	tdc.pszWindowTitle      = g_pszTitle;
	tdc.pszMainIcon         = TD_WARNING_ICON;
	tdc.pszVerificationText = L"I got it, accept to be overwritten.";
    tdc.pfCallback          = &ConfirmDialogCallback;
    tdc.lpCallbackData      = reinterpret_cast<LONG_PTR>(&dw);

	int SelectedButtonId = 0;
	TaskDialogIndirect(&tdc,&SelectedButtonId,NULL,&VerificationFlag);

	delete[] pConfirmMessage;
	delete[] pConfirmInstruction;

	return SelectedButtonId;
}

//////////////////////////////////////////////////////////////////////////////

typedef struct _DISKIMAGE_PATH
{
	WCHAR szVolumeDiskName[MAX_PATH];
	WCHAR szImageFilePath[MAX_PATH];
} DISKIMAGE_PATH;

static
INT_PTR
CALLBACK
DialogProc(
	HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
	)
{
	switch( uMsg )
	{
		case WM_INITDIALOG:
		{
			_CenterWindow(hDlg,GetParent(hDlg));

			DISKIMAGE_PATH *pdlgParam = (DISKIMAGE_PATH *)lParam;
			SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

			GetDlgItemText(hDlg,IDC_EDIT1,pdlgParam->szImageFilePath,_countof(pdlgParam->szImageFilePath));
			GetDlgItemText(hDlg,IDC_EDIT2,pdlgParam->szVolumeDiskName,_countof(pdlgParam->szVolumeDiskName));

			SHAutoComplete(GetDlgItem(hDlg,IDC_EDIT1),SHACF_FILESYS_ONLY|SHACF_FILESYSTEM);

			EnableWindow(GetDlgItem(hDlg,IDOK),FALSE);
			return TRUE;
		}
		case WM_COMMAND:
		{
			if( HIWORD(wParam) == EN_UPDATE )
			{
				switch( LOWORD(wParam) )
				{
					case IDC_EDIT1:
					case IDC_EDIT2:
					{
						int cch1 = GetWindowTextLength(GetDlgItem(hDlg,IDC_EDIT1));
						int cch2 = GetWindowTextLength(GetDlgItem(hDlg,IDC_EDIT2));
						EnableWindow( GetDlgItem(hDlg,IDOK), (cch1 && cch2) ? TRUE : FALSE );
						return FALSE;
					}
				}
			}

			switch( LOWORD(wParam) )
			{
				case IDOK:
				{
					DISKIMAGE_PATH *pdlgParam = (DISKIMAGE_PATH *)GetWindowLongPtr(hDlg,DWLP_USER);
					GetDlgItemText(hDlg,IDC_EDIT1,pdlgParam->szImageFilePath,_countof(pdlgParam->szImageFilePath));
					GetDlgItemText(hDlg,IDC_EDIT2,pdlgParam->szVolumeDiskName,_countof(pdlgParam->szVolumeDiskName));

					{
						if( !PathFileExists(pdlgParam->szImageFilePath) )
						{
							MsgBox(hDlg,L"Invalid image file name.",g_pszTitle,MB_OK|MB_ICONEXCLAMATION);
							return 0;
						}

						NTSTATUS Status;

						UNICODE_STRING vdname,path;
						SplitVolumeRelativePath(pdlgParam->szVolumeDiskName,&vdname,&path);

						if( path.Length == 0 )
						{
							HANDLE hFile;
							Status = OpenFileEx_W(&hFile,pdlgParam->szVolumeDiskName,FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);
							if( Status == 0 )
								CloseHandle(hFile);
						}
						else
						{
							Status = STATUS_INVALID_PARAMETER;
						}

						if( Status != 0 )
						{
							MsgBox(hDlg,L"Invalid drive name.",g_pszTitle,MB_OK|MB_ICONEXCLAMATION);
							return 0;
						}
					}

					EndDialog(hDlg,IDOK);
					break;
				}
				case IDCANCEL:
				{
					EndDialog(hDlg,IDCANCEL);
					break;
				}
			}
			break;
		}
	}
	return FALSE;
}

EXTERN_C
HRESULT
WINAPI
WriteBackDiskImageFileDialog(
	HWND hWnd,
	PWSTR Reserved1,
	PWSTR Reserved2,
	PVOID ReservedPtr,
	HANDLE *phHandle
	)
{
	DISKIMAGE_PATH dif = {0};

	INT_PTR ret;
	ret = DialogBoxParam(_GetResourceInstance(),MAKEINTRESOURCE(IDD_DISKIMAGE_WRITEBACK),hWnd,&DialogProc,(LPARAM)&dif);
	if( ret != IDOK )
	{
		return S_FALSE;
	}

	ret = ConfirmMessage(hWnd,dif.szVolumeDiskName);
	if( ret != IDYES )
	{
		return S_FALSE;
	}

	WRITEBACKDISKIMAGE_HANDLE_STRUCT *pchs = NULL;
	if( phHandle )
	{
		pchs = new WRITEBACKDISKIMAGE_HANDLE_STRUCT;
		if( pchs == NULL )
			return E_OUTOFMEMORY;
	}

	HRESULT hr = S_OK;
	HANDLE hThread = NULL;
	DWORD dwThreadId = 0;

	WRITEBACKIMAGE_UI_THREAD_PARAM *pcuiParam = new WRITEBACKIMAGE_UI_THREAD_PARAM;

	StringCchCopy(pcuiParam->szDiskVolumeName,MAX_PATH,dif.szVolumeDiskName);
	StringCchCopy(pcuiParam->szDiskImageFilePath,MAX_PATH,dif.szImageFilePath);
	pcuiParam->hWnd = hWnd;

#if 1
	hThread = CreateThread(NULL,0,&RestoreDiskImageUIThread,pcuiParam,0,&dwThreadId);
#else
	hThread = NULL;
#endif

	if( phHandle && pchs )
	{
		pchs->dwThreadId = dwThreadId;
		pchs->hTherad = hThread;
		*phHandle = (HANDLE)pchs;
	}
	else
	{
		CloseHandle(hThread);
	}

	return hr;
}
