//*****************************************************************************
//
//  diskimage_create.cpp
//
//  PURPOSE: Create raw disk image file.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-01-22 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "resource.h"
#include "fsvolumedisks.h"
#include "ntvolumehelp.h"

static WCHAR *g_pszTitle = L"Create RAW Disk Image File";

#define SECTOR_SIZE 4096
#define BUFFER_SIZE ((SECTOR_SIZE * 16) * 64)

enum {
	PhaseStandby=0,
	PhaseBiginProgress=1,
	PhaseProgress=2,
	PhaseEndProgress=3,
	PhaseShowWaitCompliteProgressBar = 4,
	PhaseShowErrorMessage = 5,
	PhaseExitThread = 9,
};

#define CDIF_SPARSEFILE              0x1
#define CDIF_COMPRESS                0x2
#define CDIF_SPARSEFILE_CONVERT      0x4

typedef struct _CREATEDISKIMAGE_THREAD_CONTEXT_BLOCK
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

	DWORD dwFlags;

	HRESULT hr;

	WCHAR szDiskVolumeName[MAX_PATH];
	WCHAR szDiskImageFilePath[MAX_PATH];

} CREATEDISKIMAGE_THREAD_CONTEXT_BLOCK;

typedef struct _CREATEDISKIMAGE_UI_THREAD_PARAM
{
	HWND hWnd;
	DWORD dwFlags;
	WCHAR szDiskVolumeName[MAX_PATH];
	WCHAR szDiskImageFilePath[MAX_PATH];
} CREATEDISKIMAGE_UI_THREAD_PARAM;

typedef struct _CREATEDISKIMAGE_HANDLE_STRUCT
{
	HANDLE hTherad;
	DWORD dwThreadId;
} CREATEDISKIMAGE_HANDLE_STRUCT;

typedef struct _CREATEDISKIMAGE_READWRITE
{
	LARGE_INTEGER Offset;
	LARGE_INTEGER ZeroLength;
} CREATEDISKIMAGE_READWRITE;

__inline BOOL checkZeroBuffer(BYTE *p,DWORD cb)
{
	for(DWORD i = 0; i < cb; i++)
	{
		if( *p != 0 )
			return FALSE;
		p++;
	}
	return TRUE;
}

static BOOL _WriteBlockData(HANDLE hFile, BYTE *buffer, DWORD bytesRead, CREATEDISKIMAGE_READWRITE *pcdReadWrite)
{
	DWORD bytesWritten;
	if( !WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL ) )
	{
		return FALSE;
	}
	return TRUE;
}

//
// Rescan the existing file to makes sparse.
//
HRESULT
WINAPI
ConvertToSparseFile(
	HANDLE hFile,
	GET_LENGTH_INFORMATION& diskSize,
	CREATEDISKIMAGEFILECALLBACK callback_proc,
	ULONG_PTR callback_context
	)
{
	HRESULT hr = E_FAIL;

	DWORD cb = 4096; // todo: process in cluster units.
	UCHAR *buffer = new UCHAR[cb];
	DWORD bytesRead;

	LONGLONG loop_count = diskSize.Length.QuadPart / cb;
	LONGLONG i;

	CREATEDISKIMAGE_READWRITE z = {{-1,-1},0};
	LARGE_INTEGER liOffset = {0,0};

	SetFilePointer(hFile,0,0,FILE_BEGIN);

	for(i = 0; i < loop_count; i++)
	{
		if( !ReadFile(hFile, buffer, cb, &bytesRead, NULL) )
		{
			hr = HRESULT_FROM_WIN32( GetLastError() );
			break;
		}

		if( bytesRead != cb )
		{
			hr = E_FAIL;
			break;
		}

		// split by cluster/sector
		if( checkZeroBuffer(buffer,cb) )
		{
			if( z.Offset.QuadPart == -1 )
			{
				z.Offset.QuadPart = liOffset.QuadPart;
				z.ZeroLength.QuadPart = cb;
			}
			else
			{
				z.ZeroLength.QuadPart += cb;
			}
		}
		else
		{
			if( z.Offset.QuadPart != -1 )
			{
				FILE_ZERO_DATA_INFORMATION fzdi = {0};
				fzdi.FileOffset.QuadPart      = z.Offset.QuadPart;
				fzdi.BeyondFinalZero.QuadPart = fzdi.FileOffset.QuadPart + z.ZeroLength.QuadPart;
				DeviceIoControl(hFile,FSCTL_SET_ZERO_DATA,&fzdi,sizeof(fzdi),nullptr,0,nullptr,nullptr);
			}
			z.Offset.QuadPart     = -1;
			z.ZeroLength.QuadPart = 0;
		}

		liOffset.QuadPart += cb;

		if( callback_proc )
		{
			if( !callback_proc(CDIFCB_Progress,liOffset,diskSize.Length,0,callback_context) )
				break;
		}
	}

	if( z.Offset.QuadPart != -1 )
	{
		// if remain
		FILE_ZERO_DATA_INFORMATION fzdi = {0};
		fzdi.FileOffset.QuadPart      = z.Offset.QuadPart;
		fzdi.BeyondFinalZero.QuadPart = fzdi.FileOffset.QuadPart + z.ZeroLength.QuadPart;
		DeviceIoControl(hFile,FSCTL_SET_ZERO_DATA,&fzdi,sizeof(fzdi),nullptr,0,nullptr,nullptr);
	}

	return hr;
}

HRESULT 
WINAPI
CreateDiskImageFile(
	PCWSTR pszPhysicalDrive,
	PCWSTR pszImageFilePath,
	DWORD dwFlags,
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
	GET_LENGTH_INFORMATION diskSize;
	CREATEDISKIMAGE_READWRITE cdReadWrite={ {-1,-1},0 };
	LARGE_INTEGER processedSize = {0};
	LARGE_INTEGER offZeroStartOffset = {-1,-1};
	LARGE_INTEGER offZeroBeyondFinalZero = {-1,-1};
	DWORD dwError = 0;

	buffer = (BYTE *)_MemAlloc(BUFFER_SIZE);
	if (!buffer)
	{
		return E_OUTOFMEMORY;
	}

	__try
	{
		hDisk = CreateFile(
					pszPhysicalDrive,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_SEQUENTIAL_SCAN,
					NULL
					);

		if (hDisk == INVALID_HANDLE_VALUE)
		{
			hr = HRESULT_FROM_WIN32( GetLastError() );
			return hr;
		}

		if( StrStrI(pszPhysicalDrive,L"PhysicalDrive") != 0 )
		{
			// for PhysicalDrive
			if (!DeviceIoControl(hDisk, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &diskSize, sizeof(diskSize), &bytesReturned, NULL))
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
				return hr;
			}
		}
		else
		{
			// for Volume/Drive
			VOLUME_FS_SIZE_INFORMATION *Size;
			GetVolumeFsInformation(hDisk,VOLFS_SIZE_INFORMATION,(PVOID*)&Size);
			diskSize.Length.QuadPart = Size->TotalAllocationUnits.QuadPart * Size->SectorsPerAllocationUnit * Size->BytesPerSector;
			FreeMemory(Size);
		}

		hFile = CreateFile(
					pszImageFilePath,
					GENERIC_READ|GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,
					NULL
					);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			hr = HRESULT_FROM_WIN32( GetLastError() );
			return hr;
		}

		if( dwFlags & CDIF_SPARSEFILE )  // Convert to the sparse file.
		{
			FILE_SET_SPARSE_BUFFER flag = { true };
			DeviceIoControl(hFile, FSCTL_SET_SPARSE,&flag,sizeof(flag),nullptr,0,nullptr,nullptr);

			SetFilePointerEx(hFile,diskSize.Length,NULL,FILE_BEGIN);
			SetEndOfFile(hFile);
		
			SetFilePointer(hFile,0,NULL,FILE_BEGIN);
		}

		// NOTE:
		// When reading a file from a physical drive, if the function attempts to
		// read beyond the end of the allocated space, it fails. Unlike a regular file read,
		// no data is read, and bytesRead returns 0. 
		// In this case, GetLastError() returns ERROR_INVALID_FUNCTION.

		LONGLONG loop_count = diskSize.Length.QuadPart / BUFFER_SIZE; // block copy r/w count
		LONGLONG last_block_size = diskSize.Length.QuadPart % BUFFER_SIZE; // last block r/w bytes
		LONGLONG i;

		for(i = 0; i < loop_count; i++)
		{
			if( !ReadFile(hDisk, buffer, BUFFER_SIZE, &bytesRead, NULL) )
			{
 				hr = HRESULT_FROM_WIN32( GetLastError() );
				__leave;
			}

			if( bytesRead != BUFFER_SIZE )
			{
 				hr = E_FAIL;
				__leave;
			}

			if( dwFlags & CDIF_SPARSEFILE )
			{
				if( checkZeroBuffer(buffer, bytesRead) )
				{
					if( offZeroStartOffset.QuadPart == -1 )
					{
						offZeroStartOffset.QuadPart = processedSize.QuadPart;
					}

					offZeroBeyondFinalZero.QuadPart = processedSize.QuadPart + bytesRead;
				}
				else
				{
					if( offZeroStartOffset.QuadPart != -1 )
					{
						FILE_ZERO_DATA_INFORMATION fzdi = {0};
						fzdi.FileOffset.QuadPart = offZeroStartOffset.QuadPart;
						fzdi.BeyondFinalZero.QuadPart = offZeroBeyondFinalZero.QuadPart;
						DeviceIoControl(hFile, FSCTL_SET_ZERO_DATA, &fzdi, sizeof(fzdi), nullptr, 0, nullptr, nullptr);
						SetFilePointerEx(hFile,offZeroBeyondFinalZero,nullptr,FILE_BEGIN);
						offZeroBeyondFinalZero.QuadPart = offZeroStartOffset.QuadPart = -1;
					}

					if( !_WriteBlockData(hFile, buffer, bytesRead, &cdReadWrite) )
					{
						hr = HRESULT_FROM_WIN32( GetLastError() );
						__leave;
					}
				}
			}
			else
			{
				if( !_WriteBlockData(hFile, buffer, bytesRead, &cdReadWrite) )
				{
					hr = HRESULT_FROM_WIN32( GetLastError() );
					__leave;
				}
			}

			processedSize.QuadPart += bytesRead;

			if( callback_proc )
			{
				if( !callback_proc(CDIFCB_Progress,processedSize,diskSize.Length,0,reserved_context) )
					__leave;
			}
		}

		//
		// Read remainder block size.
		//
		if( !ReadFile(hDisk, buffer, (DWORD)last_block_size, &bytesRead, NULL) )
		{
			hr = HRESULT_FROM_WIN32( GetLastError() );
			__leave;
		}

		if( dwFlags & CDIF_SPARSEFILE )
		{
			if( checkZeroBuffer(buffer, bytesRead) )
			{
				FILE_ZERO_DATA_INFORMATION fzdi = {0};

				if( offZeroStartOffset.QuadPart != -1 )
				{
					fzdi.FileOffset.QuadPart      = offZeroStartOffset.QuadPart;
					fzdi.BeyondFinalZero.QuadPart = offZeroBeyondFinalZero.QuadPart + bytesRead;
				}
				else
				{
					fzdi.FileOffset.QuadPart      = processedSize.QuadPart;
					fzdi.BeyondFinalZero.QuadPart = processedSize.QuadPart + bytesRead;
				}

				DeviceIoControl(hFile, FSCTL_SET_ZERO_DATA, &fzdi, sizeof(fzdi), nullptr, 0, nullptr, nullptr);
				SetFilePointerEx(hFile,offZeroBeyondFinalZero,nullptr,FILE_BEGIN);

				offZeroBeyondFinalZero.QuadPart = offZeroStartOffset.QuadPart = -1;
			}
			else		
			{
				if( offZeroStartOffset.QuadPart != -1 )
				{
					FILE_ZERO_DATA_INFORMATION fzdi = {0};

					fzdi.FileOffset.QuadPart      = offZeroStartOffset.QuadPart;
					fzdi.BeyondFinalZero.QuadPart = offZeroBeyondFinalZero.QuadPart;

					DeviceIoControl(hFile, FSCTL_SET_ZERO_DATA, &fzdi, sizeof(fzdi), nullptr, 0, nullptr, nullptr);
					SetFilePointerEx(hFile,offZeroBeyondFinalZero,nullptr,FILE_BEGIN);

					offZeroBeyondFinalZero.QuadPart = offZeroStartOffset.QuadPart = -1;
				}

				if( !WriteFile(hFile, buffer, bytesRead, &bytesReturned, NULL) )
				{
					hr = HRESULT_FROM_WIN32( GetLastError() );
					__leave;
				}
			}
		}
		else
		{
			if( !WriteFile(hFile, buffer, bytesRead, &bytesReturned, NULL) )
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
				__leave;
			}
		}

		processedSize.QuadPart += bytesRead;

		if( dwFlags & CDIF_SPARSEFILE_CONVERT )
		{
			//
			// Convert the file to a sparse file and rescan it.
			//
			if( (dwFlags & CDIF_SPARSEFILE) == 0 )
			{
				// If specified CDIF_SPARSEFILE_CONVERT and not specifies CDIF SPARSE FILE,
				// convert to sparse file.
				FILE_SET_SPARSE_BUFFER flag = { true };
				if( !DeviceIoControl(hFile, FSCTL_SET_SPARSE,&flag,sizeof(flag),nullptr,0,nullptr,nullptr) )
				{
					hr = HRESULT_FROM_WIN32( GetLastError() );
					__leave;
				}
			}

			hr = ConvertToSparseFile(hFile,diskSize,callback_proc,reserved_context);
			if( hr != S_OK )
			{
				__leave;
			}
		}

		if( dwFlags & CDIF_COMPRESS )
		{
			; // todo: reserved:
		}
	}
	__finally
	{
		FlushFileBuffers(hFile);

		if( hFile )
			CloseHandle(hFile);
		if( hDisk )
			CloseHandle(hDisk);

		_MemFree(buffer);
	}

	return hr;
}

static
BOOL
CALLBACK
create_diskimage_progress_callback(
	CALLBACKREASON call_reason,
	LARGE_INTEGER processedSize,
	LARGE_INTEGER diskSize,
	DWORD dwError,
	ULONG_PTR param
	)
{
	CREATEDISKIMAGE_THREAD_CONTEXT_BLOCK *ptcb = (CREATEDISKIMAGE_THREAD_CONTEXT_BLOCK *)param;

	if( call_reason == CDIFCB_Error )
	{
		; // Reserved
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
CreateDiskImageWorkerThread(
	LPVOID lpParameter
	)
{
	CREATEDISKIMAGE_THREAD_CONTEXT_BLOCK *ptcb = (CREATEDISKIMAGE_THREAD_CONTEXT_BLOCK *)lpParameter;

	ptcb->CopyStage = PhaseBiginProgress;

	ptcb->hr = CreateDiskImageFile(
					ptcb->szDiskVolumeName,
					ptcb->szDiskImageFilePath,
					ptcb->dwFlags,
					nullptr,
					&create_diskimage_progress_callback,
					(ULONG_PTR)ptcb);

	ptcb->CopyStage = PhaseEndProgress;

	return 0;
}

static void SetContentText(HWND hwnd,double progress)
{
	wchar_t buf[100];
	swprintf_s(buf,100,L"Creating... %.2f%%",progress);
	SendMessage(hwnd,TDM_SET_ELEMENT_TEXT,TDE_CONTENT,(LPARAM)buf);
}

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
	CREATEDISKIMAGE_THREAD_CONTEXT_BLOCK *ptcb = (CREATEDISKIMAGE_THREAD_CONTEXT_BLOCK*)dwRefData;

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
					if( ptcb->CopyStage != PhaseShowErrorMessage )
					{
						SuspendThread( ptcb->hThread );

						if( MsgBox(hwnd,L"Do you want to abort the operation?",g_pszTitle,MB_YESNO|MB_ICONQUESTION) == IDYES )
						{
							ptcb->UserAbort = TRUE;
							ptcb->CopyStage = PhaseShowWaitCompliteProgressBar;
						}

						ResumeThread( ptcb->hThread );
					}
					else	
					{
						ptcb->UserAbort = TRUE;
						ptcb->CopyStage = PhaseShowWaitCompliteProgressBar;
					}
					return S_FALSE; // do not close  dialog
				}
				case IDCLOSE:
					if(ptcb->CopyStage != PhaseStandby && ptcb->CopyStage != PhaseExitThread)
						return S_FALSE;  // do not close dialog
					break;
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
					if( !ptcb->UserAbort )
						SetContentText(hwnd,100.0);
					else
						SendMessage(hwnd,TDM_SET_ELEMENT_TEXT,TDE_CONTENT,(LPARAM)L"User Abort...");
					SendMessage(hwnd,TDM_SET_PROGRESS_BAR_POS,(WPARAM)(int)1000,0);
					ptcb->WaitTime = 5;
					ptcb->CopyStage = PhaseShowWaitCompliteProgressBar;
				}
				else
				{
					WCHAR szMsg[64];
					StringCchPrintf(szMsg,ARRAYSIZE(szMsg),L"Error occurred - 0x%08X",ptcb->hr);
					SendMessage(hwnd,TDM_SET_ELEMENT_TEXT,TDE_CONTENT,(LPARAM)szMsg);
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
CreateDiskImageUIThread(
	LPVOID lpParameter
	)
{
	CREATEDISKIMAGE_UI_THREAD_PARAM *pcuiParam = (CREATEDISKIMAGE_UI_THREAD_PARAM *)lpParameter;

	CREATEDISKIMAGE_THREAD_CONTEXT_BLOCK tcb = {0};
	tcb.CopyStage = PhaseStandby;
	tcb.hwndOwner = pcuiParam->hWnd;
	tcb.dwFlags   = pcuiParam->dwFlags;
	StringCchCopy(tcb.szDiskVolumeName,MAX_PATH,pcuiParam->szDiskVolumeName);
	StringCchCopy(tcb.szDiskImageFilePath,MAX_PATH,pcuiParam->szDiskImageFilePath);

	delete pcuiParam;

	InitializeCriticalSection( &tcb.cs );

	tcb.hThread = CreateThread(NULL,0,&CreateDiskImageWorkerThread,&tcb,CREATE_SUSPENDED,&tcb.dwThreadId);

	TASKDIALOGCONFIG tdc = {0};
	const TASKDIALOG_BUTTON tb = { IDABORT , L"Abort" };

	tdc.cbSize = sizeof(tdc);
	tdc.hwndParent         = tcb.hwndOwner;//hwndOwner;
	tdc.dwFlags            = TDF_SHOW_PROGRESS_BAR | 
                             TDF_CALLBACK_TIMER |
                             TDF_ALLOW_DIALOG_CANCELLATION |
				             TDF_POSITION_RELATIVE_TO_WINDOW;
	tdc.hInstance          = _GetResourceInstance();
    tdc.pfCallback         = &TaskDialogCallback;
    tdc.lpCallbackData     = reinterpret_cast<LONG_PTR>(&tcb);
    tdc.pButtons           = &tb;
    tdc.cButtons           = 1;
	tdc.pszMainInstruction = L"Create RAW Disk/Volume Image File";
	tdc.pszContent         = L"Preparing...";
	tdc.pszWindowTitle     = g_pszTitle;

	int SelectedButtonId = 0;
	TaskDialogIndirect(&tdc,&SelectedButtonId,NULL,NULL);

	DeleteCriticalSection(&tcb.cs);

	WaitForSingleObject(tcb.hThread,30*1000);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

typedef struct _DISKIMAGE_PATH
{
	DWORD dwFlags;
	WCHAR szVolumeDiskName[MAX_PATH];
	WCHAR szImageFilePath[MAX_PATH];
} DISKIMAGE_PATH;

namespace ImageFilePath
{
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
	
				GetDlgItemText(hDlg,IDC_EDIT1,pdlgParam->szVolumeDiskName,_countof(pdlgParam->szVolumeDiskName));
				GetDlgItemText(hDlg,IDC_EDIT2,pdlgParam->szImageFilePath,_countof(pdlgParam->szImageFilePath));
	
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
						GetDlgItemText(hDlg,IDC_EDIT1,pdlgParam->szVolumeDiskName,_countof(pdlgParam->szVolumeDiskName));
						GetDlgItemText(hDlg,IDC_EDIT2,pdlgParam->szImageFilePath,_countof(pdlgParam->szImageFilePath));
	
						NTSTATUS Status;
						HANDLE hFile;
						Status = OpenFileEx_W(&hFile,pdlgParam->szVolumeDiskName,FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);
						if( Status == 0 )
							CloseHandle(hFile);
						if( Status != 0 )
						{
							MsgBox(hDlg,L"Invalid drive name.",g_pszTitle,MB_OK|MB_ICONEXCLAMATION);
							return 0;
						}

						if( PathFileExists(pdlgParam->szImageFilePath) )
						{
							if( MsgBox(hDlg,L"Do you want to overwrite existing file?",g_pszTitle,MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION) != IDYES )
								return 0;
						}
	
						pdlgParam->dwFlags = 0;

						WCHAR *szRootDirectory = _MemAllocString(pdlgParam->szImageFilePath);
						PathStripToRoot(szRootDirectory);
						DWORD dwFileSystemFlags = 0;
						GetVolumeInformation(szRootDirectory,NULL,0,NULL,NULL,&dwFileSystemFlags,NULL,0);
						_SafeMemFree(szRootDirectory);

						BOOL bSparse = (IsDlgButtonChecked(hDlg,IDC_CHECK1) == BST_CHECKED);

						if( bSparse )
							pdlgParam->dwFlags = CDIF_SPARSEFILE;

						if( bSparse && ((dwFileSystemFlags & FILE_SUPPORTS_SPARSE_FILES) == 0) )
						{
							if( MsgBox(hDlg,L"The disk image file creation destination does not support sparse files. Do you want to create a regular file?",g_pszTitle,MB_YESNO|MB_ICONQUESTION) != IDYES )
								return 0;
							pdlgParam->dwFlags &= ~CDIF_SPARSEFILE;
						}

						EndDialog(hDlg,IDOK);
						break;
					}
					case IDCANCEL:
						EndDialog(hDlg,IDCANCEL);
						break;
				}
				break;
			}
		}
		return FALSE;
	}
};

//-----------------------------------------------------------------------------
//
//  CreateDiskImageFileDialog
//
//  PURPOSE: Create and show create disk image file dialog.
//
//-----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
CreateDiskImageFileDialog(
	__in HWND hWnd,
	__in PWSTR Reserved1,
	__in PWSTR Reserved2,
	__in PVOID ReservedPtr,
	__inout_opt HANDLE * // Reserved
	)
{
	DISKIMAGE_PATH dif = {0};

	INT_PTR ret;
	ret = DialogBoxParam(_GetResourceInstance(),MAKEINTRESOURCE(IDD_DISKIMAGE_CREATE),hWnd,&ImageFilePath::DialogProc,(LPARAM)&dif);
	if( ret != IDOK )
	{
		return S_FALSE;
	}

	HRESULT hr = S_OK;
	HANDLE hThread;
	DWORD dwThreadId;

	CREATEDISKIMAGE_UI_THREAD_PARAM *pcuiParam = new CREATEDISKIMAGE_UI_THREAD_PARAM;

	StringCchCopy(pcuiParam->szDiskVolumeName,MAX_PATH,dif.szVolumeDiskName);
	StringCchCopy(pcuiParam->szDiskImageFilePath,MAX_PATH,dif.szImageFilePath);
	pcuiParam->dwFlags = dif.dwFlags;
	pcuiParam->hWnd = hWnd;

	hThread = CreateThread(NULL,0,&CreateDiskImageUIThread,pcuiParam,0,&dwThreadId);

	// todo: Currently, modal process mode only.
    DWORD dwResult;
	DWORD dwCount = 1;
    for(;;)
    {
        dwResult = MsgWaitForMultipleObjects(dwCount,&hThread,FALSE,INFINITE,QS_ALLINPUT);

		if( dwResult == (WAIT_OBJECT_0 + dwCount) )
        {
            MSG msg;
            while( PeekMessage(&msg,hWnd,0,0,PM_REMOVE) )
            {
                DispatchMessage(&msg);
            }
        }
		else if( dwResult <= (WAIT_OBJECT_0 + dwCount - 1) )
		{
			break;
		}
        else if( WAIT_ABANDONED_0 == dwResult )
        {
            break;			
        }
    }

	CloseHandle(hThread);

	return hr;
}
