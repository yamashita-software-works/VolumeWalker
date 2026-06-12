//*****************************************************************************
//
//  dialog_volumeobjectideditor.cpp
//
//  PURPOSE: Volume ObjectID edit dialog.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-02-09 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "fsvolumelist.h"
#include "resource.h"
#include "libntwdk.h"
#include "ntnativeapi.h"

//---------------------------------------------------------------------------
//
//  VolumeObjectIdEditDialog()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
VolumeObjectIdEditDialog(
	__in HWND hWnd,
	__in PCWSTR pszVolumeName,
	__in ULONG Flags
	)
{
	HRESULT hr = S_OK;
	PCWSTR pszDialogTitle = L"Volume ObjectID Editor";
	WCHAR szVolumeName[MAX_PATH];

    if( HasPrefix(L"\\??\\",pszVolumeName) )
		StringCchCopy(szVolumeName,MAX_PATH,pszVolumeName);
	else	
		StringCchPrintf(szVolumeName,MAX_PATH,L"\\??\\%s",pszVolumeName);

	DWORD dwDesired = FILE_WRITE_ATTRIBUTES|FILE_WRITE_DATA|SYNCHRONIZE;

	HANDLE hVolume;
	NTSTATUS Status;
	Status = OpenFile_W(&hVolume,NULL,szVolumeName,
						dwDesired,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						FILE_SYNCHRONOUS_IO_ALERT|FILE_NON_DIRECTORY_FILE|FILE_OPEN_FOR_BACKUP_INTENT);

	if( Status != STATUS_SUCCESS )
	{
		WCHAR szBuffer[1024];

		if( STATUS_ACCESS_DENIED == Status )
		{
			StringCchCopy(szBuffer,ARRAYSIZE(szBuffer),L"Could not open the volume.");
		}
		else
		{
			PWSTR pMessage = NULL;
			_GetSystemErrorMessage(Status,&pMessage);

			FormatNtStatusErrorMessage(pMessage,szBuffer,ARRAYSIZE(szBuffer),0);

			if( szBuffer[0] == L'\0' )
				StringCchCopy(szBuffer,ARRAYSIZE(szBuffer),L"Unknown error occurred.");

			_FreeSystemErrorMessage(pMessage);
		}

		MsgBox(hWnd,szBuffer,pszDialogTitle,MB_OK|MB_ICONSTOP);

		return 0;
	}

	DWORD FileSystemFlags;
	GetVolumeInformationByHandleW(hVolume,NULL,0,NULL,0,&FileSystemFlags,NULL,0);

	if( FILE_SUPPORTS_OBJECT_IDS & FileSystemFlags )
	{
		DWORD dwFlags = 0;
		FILE_OBJECTID_BUFFER fob = {0};
		IO_STATUS_BLOCK IoStatus = {0};

		Status = NtQueryVolumeInformationFile(hVolume,&IoStatus,&fob,sizeof(fob),FileFsObjectIdInformation);

		if( STATUS_OBJECT_NAME_NOT_FOUND == Status )
		{
			if( MsgBox(hWnd,L"This volume is not assigned object id.\nDo you want to create the volume object id?",pszDialogTitle,MB_OKCANCEL|MB_ICONINFORMATION) != IDOK )
			{
				goto _exit;
			}
		}

		if( STATUS_INVALID_PARAMETER != Status )
		{
			GUIDEDITPARAM param = {0};
			GUID Guid = {0};
			if( Status == STATUS_SUCCESS )
			{
				memcpy(&Guid,fob.ObjectId,sizeof(GUID));
			}
			else
			{
				dwFlags = GUIDEF_NO_INITIAL_EDITBOX;
			}

			param.pszWindowTitle = (PWSTR)pszDialogTitle;
			param.pszMainInstruction = L""; // todo:

			if( GUIDEditDialog(hWnd,
						dwFlags,
						&Guid,
						NULL,0,
						&param) == S_OK )
			{
				memcpy(&fob.ObjectId,&Guid,sizeof(GUID));

#if 1
				Status = NtSetVolumeInformationFile(hVolume,&IoStatus,&fob,sizeof(fob),FileFsObjectIdInformation);
#else
				Status = STATUS_SUCCESS;
#endif

				if( Status != STATUS_SUCCESS )
				{
					_ErrorMessageBoxEx(hWnd,0,pszDialogTitle,L"Object ID setting failed.",Status,MB_OK|MB_ICONSTOP);
				}
			}
		}
		else
		{
			MsgBox(hWnd,L"This volume can not set the Object ID.",pszDialogTitle,MB_OK|MB_ICONSTOP);
		}
	}
	else
	{
		MsgBox(hWnd,L"This volume can not set the Object ID.",pszDialogTitle,MB_OK|MB_ICONINFORMATION);
	}

_exit:
	CloseHandle(hVolume);

	return hr;
}
