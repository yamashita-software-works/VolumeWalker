//***************************************************************************
//*                                                                         *
//*  fsfilelib.cpp                                                          *
//*                                                                         *
//*  NT native API functions                                                *
//*                                                                         *
//*  Create: 2023-04-26                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include <ntifs.h>
#include <windef.h>
#include <ntstatus.h>
#include <winerror.h>
#include <strsafe.h>
#include <malloc.h>
#include "libntwdk.h"
#include "ntnativeapi.h"
#include "ntobjecthelp.h"
#include "wfswof.h"
#include "fsfilelib.h"
#include "..\libntwdk\ntnativeapi.h"

//
// Utility
//
EXTERN_C
ULONG
APIENTRY
_StringFromGUID(
	const GUID *pGuid,
	PWSTR pszGuid,
	ULONG cchMax
	)
{
	return RtlNtStatusToDosError( StringFromGUID(pGuid,pszGuid,cchMax) );
}

EXTERN_C
BOOL
APIENTRY
NtPathIsRootDirectory(
	PCWSTR pszPath
	)
{
	UNICODE_STRING us;
	RtlInitUnicodeString(&us,pszPath);
	return IsRootDirectory_U(&us) ? TRUE : FALSE;
}

EXTERN_C
BOOL
APIENTRY
_HasPrefix(
	PCWSTR pszPrefix,
	PCWSTR pszPath
	)
{
	return HasPrefix(pszPrefix,pszPath);
}

//
// NtPath
//
EXTERN_C
UINT
APIENTRY
NtPathIsNtDevicePath(
	PCWSTR pszPath
	)
{
	return (UINT)IsNtDevicePath(pszPath);
}

EXTERN_C
BOOL
APIENTRY
NtPathGetRootDirectory(
	PCWSTR pszPath,
	PWSTR pszRootDir,
	ULONG cchRootDir
	)
{
	if( pszPath == NULL || pszRootDir == NULL || cchRootDir == 0)
		return FALSE;

	UNICODE_STRING usPath;
	RtlInitUnicodeString(&usPath,pszPath);
	if( GetRootDirectory_U(&usPath) == false )
	{
		return FALSE;
	}

	if( WCHAR_LENGTH(usPath.Length) >= cchRootDir )
	{
		// Copy as much as possible.
		memcpy(pszRootDir,usPath.Buffer,WCHAR_BYTES(cchRootDir));
		pszRootDir[ cchRootDir-1 ] = UNICODE_NULL;
		return FALSE;
	}

	memcpy(pszRootDir,usPath.Buffer,usPath.Length);
	pszRootDir[ WCHAR_LENGTH(usPath.Length) ] = UNICODE_NULL;

	return TRUE;
}

EXTERN_C
BOOL
APIENTRY
NtPathGetVolumeName(
	PCWSTR pszPath,
	PWSTR pszVolumeName,
	ULONG cchVolumeName
	)
{
	if( pszPath == NULL || pszVolumeName == NULL || cchVolumeName == 0)
		return FALSE;

	UNICODE_STRING usPath;
	RtlInitUnicodeString(&usPath,pszPath);
	if( GetVolumeName_U(&usPath) == false )
	{
		return FALSE;
	}

	if( WCHAR_LENGTH(usPath.Length) >= cchVolumeName )
	{
		return FALSE;
	}

	memcpy(pszVolumeName,usPath.Buffer,usPath.Length);

	pszVolumeName[ WCHAR_LENGTH(usPath.Length) ] = UNICODE_NULL;

	return TRUE;
}

EXTERN_C
BOOL
APIENTRY
NtPathFileExists(
	PCWSTR pszPath
	)
{
	return PathFileExists_W(pszPath,NULL);
}

EXTERN_C
BOOL
APIENTRY
NtPathQueryVolumeDeviceName(
	PCWSTR pszGrobalRootPrefixedPath,
	PWSTR pszBuffer,
	ULONG cchBuffer
	)
{
	UNICODE_STRING us;
	RtlInitUnicodeString(&us,pszGrobalRootPrefixedPath);
	if( PathIsPrefixDosDevice_U(&us) )
	{
		GetVolumeName_U(&us);
		QuerySymbolicLinkObject(NULL,NULL,&us,pszBuffer,cchBuffer);
	}
	return TRUE;
}

static BOOLEAN _compare_target_name(HANDLE hObjDir,WCHAR *SymName,UNICODE_STRING *NtVolumeDeviceName)
{
	WCHAR buf[32];
    UNICODE_STRING usLinkTarget;
	usLinkTarget.Length = 0;
	usLinkTarget.MaximumLength = sizeof(buf);
	usLinkTarget.Buffer = buf;
	if( QuerySymbolicLinkObject_U(hObjDir,SymName,&usLinkTarget,FALSE) == 0 )
	{
		if( RtlCompareUnicodeString(&usLinkTarget,NtVolumeDeviceName,TRUE) == 0 )
		{
			return TRUE;
		}
	}
	return FALSE;
}

static bool __stdcall _lookup_guid(const WCHAR *SymName,const UNICODE_STRING*)
{
	return ((_wcsnicmp(SymName,L"Volume{",7) == 0 && (SymName[43] == L'}' && SymName[44] == '\0')));
}

static bool __stdcall _lookup_dos_drive(const WCHAR *SymName,const UNICODE_STRING*)
{
	return (iswalpha(SymName[0]) && SymName[1] == L':');
}

static bool __stdcall _lookup_from_nt_devicename(const WCHAR *SymName,const UNICODE_STRING *NtPath)
{
	// &NtPath->Buffer[8] skip "\Device\" and compare trailing string.
	// e.g.
	//  "\Device\HarddiskVolume1"
	//  "\Device\Cdrom1"
	//
	return (_wcsnicmp(SymName,&NtPath->Buffer[8],(NtPath->Length/sizeof(WCHAR)-8)) == 0);
}

static bool __stdcall _lookup_from_dos_namespace(const WCHAR *SymName,const UNICODE_STRING *NtPath)
{
	// &NtPath->Buffer[4] skip "\??\" and compare trailing string.
	// e.g.
	//  "\??\C:
	//  "\??\HarddiskVolume1"
	//
	return (_wcsnicmp(SymName,&NtPath->Buffer[4],(NtPath->Length/sizeof(WCHAR)-4)) == 0);
}

typedef bool(__stdcall *COMP_FUNC_PTR)(const WCHAR *,const UNICODE_STRING *);

EXTERN_C
BOOL
APIENTRY
NtPathTranslatePath(
	PCWSTR pszNtPath,
	ULONG Flags,
	PWSTR pszPath,
	ULONG cchPath
	)
{
    HANDLE hObjDir;
    NTSTATUS Status;
	DWORD dwError;
	BOOL bResult = FALSE;
    ULONG Index = 0;
    WCHAR SymName[260];
    UNICODE_STRING usNtPathVolume;
	UNICODE_STRING usVolumeRelativePath;
	UINT PathType = 0;

	if( !SplitVolumeRelativePath(pszNtPath,&usNtPathVolume,&usVolumeRelativePath) )
	{
		_SetLastStatusDos( STATUS_OBJECT_PATH_INVALID );
        return FALSE;
	}

	if( HasPrefix(L"\\??\\",pszNtPath) )
	{
		PathType = 3;
	}
	else
	{
		switch( PTF_TYPE_MASK & Flags )
		{
		case PTF_GUID:
			PathType = 0;
			break;
		case PTF_DOSDRIVE:
			PathType = 1;
			break;
		case PTF_DOSNAMESPACE:
			PathType = 2;
			break;
		default:
			_SetLastStatusDos( STATUS_INVALID_PARAMETER );
			return FALSE;
		}
	}

	static COMP_FUNC_PTR cmp_func_ptr[] = {
		&_lookup_guid,
		&_lookup_dos_drive,
		&_lookup_from_nt_devicename,
		&_lookup_from_dos_namespace,
	};
	COMP_FUNC_PTR cmp_fnc = cmp_func_ptr[PathType];

#ifdef _DEBUG
	PWSTR p1 = AllocateSzFromUnicodeString(&usNtPathVolume);
	PWSTR p2 = AllocateSzFromUnicodeString(&usVolumeRelativePath);
	FreeMemory(p2);
	FreeMemory(p1);
#endif

    Status = OpenObjectDirectory( L"\\GLOBAL??", &hObjDir );

    if( Status == STATUS_SUCCESS )
    {
        while( (dwError = QueryObjectDirectory(hObjDir,&Index,SymName,ARRAYSIZE(SymName),NULL,0)) == ERROR_SUCCESS )
        {
			if( cmp_fnc(SymName,&usNtPathVolume) )
            {
				if( (PathType == 3) || _compare_target_name(hObjDir,SymName,&usNtPathVolume) )
				{
					*pszPath = UNICODE_NULL;
					if( Flags & PTF_NTDOSNAMESPACEPREFIX )
						StringCchCat(pszPath,cchPath,L"\\??\\");  // NT namespace win32 global root
					else if( Flags & PTF_WIN32PATHPREFIX )
						StringCchCat(pszPath,cchPath,L"\\\\?\\"); // win32 file namespace
					StringCchCat(pszPath,cchPath,SymName);
					if( usVolumeRelativePath.Buffer )
						StringCchCat(pszPath,cchPath,usVolumeRelativePath.Buffer);
					bResult = TRUE;
					break;
				}
			}
        }

        CloseObjectDirectory( hObjDir );
    }

	_SetLastStatusDos( bResult ? STATUS_SUCCESS : STATUS_OBJECT_NAME_NOT_FOUND );

    return bResult;
}

EXTERN_C
BOOL
APIENTRY
NtPathToDosPath(
	PCWSTR pszNtPath,
	PWSTR pszPath,
	ULONG cchPath
	)
{
	return NtPathTranslatePath(pszNtPath,PTF_DOSDRIVE,pszPath,cchPath);
}

EXTERN_C
BOOL
APIENTRY
NtPathToDosPathEx(
	PCWSTR pszNtPath,
	PWSTR pszPath,
	ULONG cchPath,
	ULONG Flags
	)
{
	if( (Flags & PTF_TYPE_MASK) == 0 )
		Flags |= PTF_DOSDRIVE;

	return NtPathTranslatePath(pszNtPath,
				Flags | (Flags & PTF_PREFIX_MASK),
				pszPath,cchPath);
}

EXTERN_C
BOOL
APIENTRY
NtPathToGuidPath(
	PCWSTR pszNtPath,
	PWSTR pszPath,
	ULONG cchPath,
	ULONG Flags
	)
{
	return NtPathTranslatePath(pszNtPath,PTF_GUID|(Flags & PTF_PREFIX_MASK),
			pszPath,cchPath);
}


EXTERN_C
DWORD
APIENTRY
NtPathGetLongPathNameFromHandle(
	HANDLE hFile,
	PWSTR *LongPathName,
	PULONG pcbLongPathName
	)
{
	NTSTATUS Status;
	Status = GetLongPathNameFromHandle(hFile,LongPathName,pcbLongPathName);
	RtlSetLastWin32Error( Status );
	return RtlNtStatusToDosError(Status);
}

//---------------------------------------------------------------------------
//
//  DosPathToNtDevicePath()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
BOOL
APIENTRY
DosPathToNtDevicePath(
	PCWSTR pszDosPath,
	PWSTR pszNtPathBuffer,
	ULONG cchNtPathBuffer,
	ULONG Flags
	)
{
	UNICODE_STRING VolumeName;
	UNICODE_STRING RootPath;
	BOOL bSuccess = FALSE;

	if( !SplitVolumeRelativePath(pszDosPath,&VolumeName,&RootPath) )
	{
		_SetLastStatusDos(STATUS_INVALID_PARAMETER);
		return FALSE;
	}

	PWSTR pVolume = AllocateSzFromUnicodeString(&VolumeName);
	PWSTR pRootPath = AllocateSzFromUnicodeString(&RootPath);

	if( pVolume && pRootPath )
	{
		WCHAR szNtVolume[64];
		RtlZeroMemory(szNtVolume,sizeof(szNtVolume));

		if( PathIsPrefixDosDevice(pVolume) )
		{
			if( QuerySymbolicLinkObjectName(NULL,pVolume,szNtVolume,ARRAYSIZE(szNtVolume),NULL) != 0 )
			{
				;
			}
		}
		else
		{
			if( QuerySymbolicLinkObjectName(L"\\Global??",pVolume,szNtVolume,ARRAYSIZE(szNtVolume),NULL) != 0 )
			{
				if( QuerySymbolicLinkObjectName(L"\\??",pVolume,szNtVolume,ARRAYSIZE(szNtVolume),NULL) != 0 )
				{
					;
				}
			}
		}

		if( szNtVolume[0] != L'\0' )
		{
			PWSTR pszNtDevicePath;
			pszNtDevicePath = CombinePath(szNtVolume,pRootPath);

			if( pszNtDevicePath )
			{
				HRESULT hr;
				hr = StringCchCopy(pszNtPathBuffer,cchNtPathBuffer,pszNtDevicePath);
				if( hr == S_OK )
				{
					_SetLastWin32Error( ERROR_SUCCESS );
					bSuccess = TRUE;
				}
				else
				{
					_SetLastWin32Error( HRESULT_CODE(hr) );
				}
			}
			else
			{
				_SetLastStatusDos( STATUS_NO_MEMORY );
			}
		}
	}
	else
	{
		_SetLastStatusDos( STATUS_NO_MEMORY );
	}

	FreeMemory(pVolume);
	FreeMemory(pRootPath);

	return bSuccess;
}

//----------------------------------------------------------------------------
//
//  GetWofInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
GetWofInformation(
	HANDLE FileHandle,
	PVOID *ExternalInfo,
	PVOID *ProviderInfo
	)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus = {0};

	ULONG cbBuffer = 4096;
	UCHAR *pBuffer = (UCHAR *)AllocMemory(cbBuffer);
	if( pBuffer == NULL )
	{
		return E_OUTOFMEMORY;
	}

	Status = NtFsControlFile(FileHandle,NULL,NULL,NULL,
					&IoStatus,
					FSCTL_GET_EXTERNAL_BACKING,
					NULL,0,
					pBuffer,cbBuffer);

	if( Status == STATUS_PENDING )
	{
		LARGE_INTEGER li;
		li.QuadPart = -(10000000 * 10); // maximum wait 10sec
		Status = NtWaitForSingleObject(FileHandle,FALSE,&li);
		if( Status != STATUS_SUCCESS )
		{
			FreeMemory(pBuffer);
			*ExternalInfo = NULL;
			*ProviderInfo = NULL;
			return Status;
		}
		Status = IoStatus.Status;
	}

	if( Status == STATUS_SUCCESS && IoStatus.Information != 0 )
	{
		if( ExternalInfo != NULL && ProviderInfo != NULL )
		{
			PVOID ptr;
			ptr = ReAllocateHeap(pBuffer,(ULONG)IoStatus.Information);

			if( ptr != NULL )
			{
				pBuffer = (UCHAR*)ptr;

				WOF_EXTERNAL_INFO *pwei = (WOF_EXTERNAL_INFO *)pBuffer;

				*ExternalInfo = pwei;

				if( pwei->Provider == WOF_PROVIDER_FILE )
				{
					FILE_PROVIDER_EXTERNAL_INFO_V1 *pFile = (FILE_PROVIDER_EXTERNAL_INFO_V1 *)&pBuffer[8];
					// pFile->Version;
					// pFile->Algorithm
					// pFile->Flags
					*ProviderInfo = pFile;
				}
				else if( pwei->Provider == WOF_PROVIDER_WIM )
				{
					WIM_PROVIDER_EXTERNAL_INFO *pWIM = (WIM_PROVIDER_EXTERNAL_INFO *)&pBuffer[8];
					// pWIM->Version;
					// pWIM->Flags;
					// pWIM->DataSourceId;
					// pWIM->ResourceHash[WIM_PROVIDER_HASH_SIZE];
					*ProviderInfo = pWIM;
				}
				else
				{
					FreeMemory(pwei);
					*ExternalInfo = NULL;
					*ProviderInfo = NULL;
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				FreeMemory(pBuffer);
				Status = STATUS_NO_MEMORY;
			}
		}
		else
		{
			FreeMemory(pBuffer);
		}
	}
	else
	{
		FreeMemory(pBuffer);
		if( ExternalInfo )
			*ExternalInfo = NULL;
		if( ProviderInfo )
			*ProviderInfo = NULL;
	}

	RtlSetLastWin32Error( RtlNtStatusToDosError(Status) );

	return HRESULT_FROM_WIN32( RtlGetLastWin32Error() );
}

//----------------------------------------------------------------------------
//
//  FreeWofFileInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT 
APIENTRY
FreeWofInformation(
	PVOID ExternalInfo,
	PVOID ProviderInfo
	)
{
	// 'ProviderInfo' is do not free memory. 
	// Because pointed same memory region of 'ExternalInfo'.
	// This behavior is a reserved for the future.
	FreeMemory(ExternalInfo);
	return S_OK;
}

///////////////////////////////// FileInfo //////////////////////////////////

#define PRIVATE static

//---------------------------------------------------------------------------
//
//  NTFile_OpenFile()
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
NTFile_OpenFile(
	HANDLE *phFile,
	PCWSTR FilePath,
	ULONG DesiredAccess,
	ULONG ShareAccess,
	ULONG OpenOptions
	)
{
	HRESULT hr;

	if( OpenFile_W(phFile,NULL,FilePath,DesiredAccess,ShareAccess,OpenOptions) == STATUS_SUCCESS )
	{
		hr = S_OK;
	}
	else
	{
		hr = E_FAIL;
	}
	return hr;
}

//---------------------------------------------------------------------------
//
//  NTFile_CloseFile()
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
NTFile_CloseFile(
	HANDLE hFile
	)
{
	return HRESULT_FROM_NT( NtClose(hFile) );
}

//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//  _DeviceIoControl()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
PRIVATE
BOOLEAN
NTAPI
_DeviceIoControl(
	HANDLE FileHandle,
	ULONG FsControlCode,
	PVOID InputBuffer,
    ULONG InputBufferLength,
	PVOID OutputBuffer,
    ULONG OutputBufferLength,
	PULONG BytesReturned,
	NTSTATUS *pNtStatus
	)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus = {0};

	Status = NtFsControlFile(FileHandle,NULL,NULL,NULL,&IoStatus,
						FsControlCode,
						InputBuffer,InputBufferLength,
						OutputBuffer,OutputBufferLength);

	if( BytesReturned )
		*BytesReturned = (ULONG)IoStatus.Information;

	if( pNtStatus )
		*pNtStatus = Status;

	SetLastErrorNtStatusToWin32( Status );

	return (Status == STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
//
//  _GetFileSize()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
PRIVATE
NTSTATUS
NTAPI
_GetFileSize(
	HANDLE hFile,
	LARGE_INTEGER *pSize,
	LARGE_INTEGER *pAllocationSize
	)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;

	FILE_STANDARD_INFORMATION fsi;
	Status = NtQueryInformationFile(hFile,&IoStatus,&fsi,sizeof(fsi),FileStandardInformation);

	if( Status == STATUS_SUCCESS )
	{
		if( pSize )
			*pSize = fsi.EndOfFile;

		if( pAllocationSize )
			*pAllocationSize = fsi.AllocationSize;
	}

	return Status;
}

//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//  GetAlternateStreamNames()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
PRIVATE
NTSTATUS
APIENTRY
GetAlternateStreamNames(
	HANDLE hFile,
	INT *pAltStreamCount,
	FILE_STREAM_INFORMATION **StreamInformation
	)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;
	INT cAltStreams = 0;
	FILE_STREAM_INFORMATION s = {0};
	FILE_STREAM_INFORMATION *pfsi = &s;
	ULONG cb = sizeof(FILE_STREAM_INFORMATION);

	if( pAltStreamCount )
		*pAltStreamCount = 0;

	Status = NtQueryInformationFile(hFile,&IoStatus,pfsi,cb,FileStreamInformation);

	if( Status == STATUS_SUCCESS )
		return Status;

	// STATUS_BUFFER_OVERFLOW
	// The output buffer was filled before all of the stream information could be returned. 
	// Only complete FILE_STREAM_INFORMATION structures are returned.
	// STATUS_INFO_LENGTH_MISMATCH
	// The specified information record length does not match the length that is required
	// for the specified information class.

	if( Status != STATUS_BUFFER_OVERFLOW )
		return Status;

	for(;;)
	{
		cb += 4096;

		pfsi = (FILE_STREAM_INFORMATION *)AllocMemory( cb );

		if( pfsi == NULL )
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		Status = NtQueryInformationFile(hFile,&IoStatus,pfsi,cb,FileStreamInformation);

		if( Status == STATUS_SUCCESS )
		{
			*StreamInformation = pfsi;

			if( pAltStreamCount )
			{
				FILE_STREAM_INFORMATION *psn = pfsi;

				for(;;)
				{
					cAltStreams++;
					if( psn->NextEntryOffset == 0 )
						break;
					psn = (FILE_STREAM_INFORMATION *)((ULONG_PTR)psn + psn->NextEntryOffset);
				}
				*pAltStreamCount = cAltStreams;
			}
			break;
		}

		FreeMemory(pfsi);

		if( Status != STATUS_BUFFER_OVERFLOW )
		{
			break;
		}
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  FreeEAInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
PRIVATE
NTSTATUS
APIENTRY
FreeEAInformation(
	FILE_INFORMATON_EA_BUFFER *EABuffer
	)
{
	if( EABuffer == NULL )
		return STATUS_INVALID_PARAMETER;

	ULONG i;
	for(i = 0; i < EABuffer->EaCount; i++)
	{
		if( EABuffer->Ea[i].Name )
			FreeMemory( EABuffer->Ea[i].Name );
		if( EABuffer->Ea[i].Value )
			FreeMemory( EABuffer->Ea[i].Value );
	}

	FreeMemory(EABuffer);

	return 0;
}

//----------------------------------------------------------------------------
//
//  GetEAInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
PRIVATE
NTSTATUS
APIENTRY
GetEAInformation(
	HANDLE hFile,
	INT *pAltStreamCount,
	FILE_INFORMATON_EA_BUFFER **EABuffer
	)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus = {0};
	FILE_FULL_EA_INFORMATION *pEA = NULL;
	FILE_INFORMATON_EA_BUFFER *pEaBuffer = NULL;

	__try
	{
		ULONG EaIndex = 1; // Index is 1-based.
		BOOLEAN bStartScan = TRUE;

		ULONG cbEA =  sizeof(FILE_FULL_EA_INFORMATION) + 32768;
		pEA = (FILE_FULL_EA_INFORMATION *)AllocMemory( cbEA );
		if( pEA == NULL )
			__leave;

		for(;;)
		{
			Status = NtQueryEaFile(
						hFile,&IoStatus,
						pEA,cbEA,
						TRUE,// Single entry return
						NULL,0,
						&EaIndex,bStartScan);

			if( Status == STATUS_NONEXISTENT_EA_ENTRY )
			{
				break; // hadn't  ea data
			}

			if( Status == STATUS_NO_MORE_EAS )
			{
				Status = STATUS_SUCCESS;
				break; // End of EA
			}

			if( Status != STATUS_SUCCESS )
			{
				break; // some error
			}

			//
			// Allocatiom/Reallocation EA return buffer
			//
			if( pEaBuffer == NULL )
			{
				pEaBuffer = (FILE_INFORMATON_EA_BUFFER*)AllocMemory( sizeof(FILE_INFORMATON_EA_BUFFER) );
				if( pEaBuffer == NULL )
					break;
				pEaBuffer->EaCount = 0;
			}
			else
			{
				pEaBuffer = (FILE_INFORMATON_EA_BUFFER*)ReallocMemory( pEaBuffer, 
								sizeof(FILE_INFORMATON_EA_BUFFER) + (sizeof(FILE_INFORMATON_EA_DATA) * pEaBuffer->EaCount) );
				if( pEaBuffer == NULL )
					break;
			}

			//
			// Copy EA information
			//
			FILE_INFORMATON_EA_DATA *Item;
			Item = &pEaBuffer->Ea[pEaBuffer->EaCount];

			// EA basic information
			Item->Flags = pEA->Flags;
			Item->NameLength = pEA->EaNameLength;
			Item->ValueLength = pEA->EaValueLength;

			// EA name
			Item->Name = (CHAR *)AllocMemory(pEA->EaNameLength+1);
			if( Item->Name == NULL )
			{
				Status = STATUS_NO_MEMORY;
				break;
			}
			RtlCopyMemory((VOID*)Item->Name,pEA->EaName,pEA->EaNameLength);

			// EA data
			Item->Value = (UCHAR *)AllocMemory(pEA->EaValueLength);
			if( Item->Value == NULL )
			{
				Status = STATUS_NO_MEMORY;
				break;
			}
			RtlCopyMemory((VOID*)Item->Value,pEA->EaName+(pEA->EaNameLength+1),pEA->EaValueLength);

			pEaBuffer->EaCount++;
			bStartScan = FALSE;
			EaIndex++;
		}
	}
	__finally
	{
		if( pEaBuffer == NULL || pEA == NULL )
			Status = STATUS_NO_MEMORY;

		if( Status != STATUS_SUCCESS )
		{
			FreeEAInformation( pEaBuffer );
			pEaBuffer = NULL;
		}

		if( pEA != NULL )
			FreeMemory( pEA );

		*EABuffer = pEaBuffer;
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  GetReparsePointInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
PRIVATE
BOOLEAN
APIENTRY
FreeReparsePointInformation(
	PVOID InformationBuffer
	)
{
	if( InformationBuffer == NULL )
		return false;

	FS_REPARSE_POINT_INFORMATION_EX *pInfo = (FS_REPARSE_POINT_INFORMATION_EX *)InformationBuffer;

	switch( pInfo->ReparseTag )
	{
		case IO_REPARSE_TAG_MOUNT_POINT:
			FreeMemory(pInfo->MountPoint.PrintPath);
			FreeMemory(pInfo->MountPoint.TargetPath);
			pInfo->MountPoint.PrintPath = NULL;
			pInfo->MountPoint.TargetPath = NULL;
			break;
		case IO_REPARSE_TAG_SYMLINK:
			FreeMemory(pInfo->SymLink.PrintPath);
			FreeMemory(pInfo->SymLink.TargetPath);
			pInfo->SymLink.PrintPath = NULL;
			pInfo->SymLink.TargetPath = NULL;
			break;
		case IO_REPARSE_TAG_APPEXECLINK:
			FreeMemory(pInfo->AppExecLink.Buffer);
			memset(&pInfo->AppExecLink,0,sizeof(pInfo->AppExecLink));
			break;
		default:
			FreeMemory(pInfo->GenericReparse.Buffer);
			pInfo->GenericReparse.Buffer = NULL;
			break;		
	}

	pInfo->ReparseTag = 0;
	pInfo->ReparseDataLength = 0;
	pInfo->Flags = 0;

	return true;
}

//----------------------------------------------------------------------------
//
//  GetReparsePointInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
PRIVATE
BOOLEAN
APIENTRY
GetReparsePointInformation(
	HANDLE hRoot,
	PCWSTR FilePath,
	PVOID InformationStruct,
	ULONG InformationStructLength,
	ULONG Flags
	)
{
	BOOLEAN bSuccess = FALSE;
	HANDLE hFile;

	if( FilePath != NULL )
	{
		NTSTATUS Status;
		UNICODE_STRING ustrPath;
		RtlInitUnicodeString(&ustrPath,FilePath);
		Status = OpenFile_U(&hFile,hRoot,&ustrPath,
					FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,
					FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT);
		if( Status != STATUS_SUCCESS )
		{
			SetLastErrorNtStatusToWin32( STATUS_INVALID_PARAMETER );
			return FALSE;
		}
	}
	else if( hRoot != NULL && FilePath == NULL )
	{
		hFile = hRoot;
	}
	else
	{
		SetLastErrorNtStatusToWin32( STATUS_INVALID_PARAMETER );
		return FALSE;
	}

	ULONG cbDataLength = sizeof(REPARSE_DATA_BUFFER) + _NT_PATH_FULL_LENGTH_BYTES;
	REPARSE_DATA_BUFFER *pBuffer = (REPARSE_DATA_BUFFER *)AllocMemory( cbDataLength );

	if( pBuffer != NULL && hFile != INVALID_HANDLE_VALUE )
	{
		DWORD cb = 0;
		for(;;)
		{
	        bSuccess = _DeviceIoControl(hFile,
							FSCTL_GET_REPARSE_POINT,
							NULL,0,
							pBuffer,cbDataLength,
							&cb,NULL);
			if( bSuccess )
			{
				if( cb != 0 && (cb < cbDataLength) )
				{
					pBuffer = (REPARSE_DATA_BUFFER *)ReallocMemory(pBuffer,cb); // buffer shrink
					cbDataLength = cb;
				}
				break;
			}

			if( RtlGetLastWin32Error() == STATUS_BUFFER_TOO_SMALL )
			{
				cbDataLength += _NT_PATH_FULL_LENGTH_BYTES;
				pBuffer = (REPARSE_DATA_BUFFER *)ReallocMemory(pBuffer,cbDataLength);
				if( pBuffer == NULL )
					break;
			}
			else
			{
				break; // fatal error
			}
		}
	}

	if( FilePath != NULL && hFile != INVALID_HANDLE_VALUE )
		NtClose(hFile);

	if( pBuffer == NULL )
	{
		_SetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
		bSuccess = FALSE;
	}

	//
	// Parse ReparsePoint Type
	//
	if( bSuccess )
	{
		FS_REPARSE_POINT_INFORMATION_EX *pInfo = (FS_REPARSE_POINT_INFORMATION_EX *)InformationStruct;
		WCHAR *pName;
		ULONG Result = 0;
		SIZE_T cbNameSize;

		pInfo->ReparseTag = pBuffer->ReparseTag;
		pInfo->ReparseDataLength = pBuffer->ReparseDataLength;
		pInfo->Reserved = pBuffer->Reserved;

		switch( pBuffer->ReparseTag )
		{
			case IO_REPARSE_TAG_MOUNT_POINT:
			{
				// Get target path
				if( pInfo->MountPoint.TargetPath == NULL )
				{
					cbNameSize = pBuffer->MountPointReparseBuffer.SubstituteNameLength;
					pName = (pBuffer->MountPointReparseBuffer.PathBuffer + (pBuffer->MountPointReparseBuffer.SubstituteNameOffset/sizeof(WCHAR)));
#if 1
					pInfo->MountPoint.TargetPath = AllocStringBufferCb(cbNameSize + sizeof(WCHAR));
					memcpy(pInfo->MountPoint.TargetPath,pName,cbNameSize);
					pInfo->MountPoint.TargetPath[WCHAR_CHARS(cbNameSize)] = UNICODE_NULL;
#else
					pInfo->MountPoint.TargetPath = pName;
#endif
					pInfo->MountPoint.TargetPathLength = (ULONG)cbNameSize;
				}

				// Get print path
				if( pInfo->MountPoint.PrintPath == NULL )
				{
					cbNameSize = pBuffer->SymbolicLinkReparseBuffer.PrintNameLength;
					pName = (pBuffer->MountPointReparseBuffer.PathBuffer + (pBuffer->SymbolicLinkReparseBuffer.PrintNameOffset/sizeof(WCHAR)));
#if 1	
					pInfo->MountPoint.PrintPath = AllocStringBufferCb(cbNameSize + sizeof(WCHAR));
					memcpy(pInfo->MountPoint.PrintPath,pName,cbNameSize);
					pInfo->MountPoint.PrintPath[WCHAR_CHARS(cbNameSize)] = UNICODE_NULL;
#else
					pInfo->MountPoint.PrintPath = pName;
#endif
					pInfo->MountPoint.PrintPathLength = (ULONG)cbNameSize;
				}

				FreeMemory(pBuffer);

				_SetLastWin32Error( ERROR_SUCCESS );
				break;
			}
			case IO_REPARSE_TAG_SYMLINK:
			{
				pInfo->SymLink.Flags = pBuffer->SymbolicLinkReparseBuffer.Flags;

				// Get target path
				if( pInfo->SymLink.TargetPath == NULL )
				{
					cbNameSize = pBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength;

					pName = (pBuffer->SymbolicLinkReparseBuffer.PathBuffer + (pBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset/sizeof(WCHAR)));
#if 1
					pInfo->SymLink.TargetPath = AllocStringBufferCb(cbNameSize + sizeof(WCHAR));
					memcpy(pInfo->SymLink.TargetPath,pName,cbNameSize);
					pInfo->SymLink.TargetPath[WCHAR_CHARS(cbNameSize)] = UNICODE_NULL;
#else
					pInfo->SymLink.TargetPath = pName;
#endif
					pInfo->SymLink.TargetPathLength = (ULONG)cbNameSize;
				}

				// Get print path
				if( pInfo->SymLink.PrintPath == NULL )
				{
					cbNameSize = pBuffer->SymbolicLinkReparseBuffer.PrintNameLength;

					pName = (pBuffer->SymbolicLinkReparseBuffer.PathBuffer + (pBuffer->SymbolicLinkReparseBuffer.PrintNameOffset/sizeof(WCHAR)));
#if 1
					pInfo->SymLink.PrintPath = AllocStringBufferCb(cbNameSize + sizeof(WCHAR));
					memcpy(pInfo->SymLink.PrintPath,pName,cbNameSize);
					pInfo->SymLink.PrintPath[WCHAR_CHARS(cbNameSize)] = UNICODE_NULL;
#else
					pInfo->SymLink.PrintPath = pName;
#endif
					pInfo->SymLink.PrintPathLength = (ULONG)cbNameSize;
				}

				FreeMemory(pBuffer);

				_SetLastWin32Error( ERROR_SUCCESS );
				break;
			}
			case IO_REPARSE_TAG_APPEXECLINK:
			{
				APPEXECLINK_READ_BUFFER *pal = (APPEXECLINK_READ_BUFFER *)pBuffer;
				pInfo->AppExecLink.Version = pal->Version;
				{
					pInfo->AppExecLink.Buffer = (PUCHAR)pBuffer;

					WCHAR *pStr = ((APPEXECLINK_READ_BUFFER *)pInfo->AppExecLink.Buffer)->StringList;

					// 0:"Package ID"
					// 1:"Entry Point"
					// 2:"Executable"
					// 3:"Application Type" Integer as ASCII. 
					for(int iIndex = 0; iIndex < 4; iIndex++)
					{
						switch( iIndex )
						{
							case 0:
								pInfo->AppExecLink.PackageID = pStr;
								break;
							case 1:
								pInfo->AppExecLink.EntryPoint = pStr;
								break;
							case 2:
								pInfo->AppExecLink.Executable = pStr;
								break;
							case 3:
								pInfo->AppExecLink.ApplicType = pStr;
								break;
						}
						pStr += (wcslen(pStr)+1);
					}
				}
				break;
			}
			case IO_REPARSE_TAG_HSM:
			case IO_REPARSE_TAG_HSM2:
			case IO_REPARSE_TAG_SIS:
			case IO_REPARSE_TAG_WIM:
			case IO_REPARSE_TAG_CSV:
			case IO_REPARSE_TAG_DFS:
			case IO_REPARSE_TAG_DFSR:
			case IO_REPARSE_TAG_DEDUP:
			case IO_REPARSE_TAG_NFS:
			case IO_REPARSE_TAG_FILE_PLACEHOLDER:
			case IO_REPARSE_TAG_WOF:
			case IO_REPARSE_TAG_WCI:
			case IO_REPARSE_TAG_WCI_1:
			case IO_REPARSE_TAG_GLOBAL_REPARSE:
			case IO_REPARSE_TAG_CLOUD:
			case IO_REPARSE_TAG_CLOUD_1:
			case IO_REPARSE_TAG_CLOUD_2:
			case IO_REPARSE_TAG_CLOUD_3:
			case IO_REPARSE_TAG_CLOUD_4:
			case IO_REPARSE_TAG_CLOUD_5:
			case IO_REPARSE_TAG_CLOUD_6:
			case IO_REPARSE_TAG_CLOUD_7:
			case IO_REPARSE_TAG_CLOUD_8:
			case IO_REPARSE_TAG_CLOUD_9:
			case IO_REPARSE_TAG_CLOUD_A:
			case IO_REPARSE_TAG_CLOUD_B:
			case IO_REPARSE_TAG_CLOUD_C:
			case IO_REPARSE_TAG_CLOUD_D:
			case IO_REPARSE_TAG_CLOUD_E:
			case IO_REPARSE_TAG_CLOUD_F:
			case IO_REPARSE_TAG_CLOUD_MASK:
			case IO_REPARSE_TAG_PROJFS:
			case IO_REPARSE_TAG_STORAGE_SYNC:
			case IO_REPARSE_TAG_WCI_TOMBSTONE:
			case IO_REPARSE_TAG_UNHANDLED:
			case IO_REPARSE_TAG_ONEDRIVE:
			case IO_REPARSE_TAG_PROJFS_TOMBSTONE:
			case IO_REPARSE_TAG_AF_UNIX:
			case IO_REPARSE_TAG_WCI_LINK:
			case IO_REPARSE_TAG_WCI_LINK_1:
				pInfo->ReparseTag = pBuffer->ReparseTag;
				pInfo->GenericReparse.Buffer = (PUCHAR)pBuffer;
				_SetLastWin32Error( ERROR_SUCCESS );
				bSuccess = TRUE;
				break;

			default:
				_SetLastWin32Error( ERROR_INVALID_REPARSE_DATA );
				bSuccess = FALSE;
				break;
		}
	}

	return bSuccess;
}

//----------------------------------------------------------------------------
//
//  GetObjectIdInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
PRIVATE
NTSTATUS
APIENTRY
GetObjectIdInformation(
	HANDLE hFile,
	FILE_OBJECTID_BUFFER *pfob
	)
{
	FILE_OBJECTID_BUFFER fob = {0};
	BOOLEAN bSuccess;
	ULONG cb;

	bSuccess = _DeviceIoControl(hFile,FSCTL_GET_OBJECT_ID,
						NULL,0,&fob,sizeof(fob),&cb,NULL);

	if( bSuccess )
		*pfob = fob;

	return RtlGetLastWin32Error();
}

#if 0
//----------------------------------------------------------------------------
//
//  GetWofInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
GetWofInformation(
	HANDLE FileHandle,
	PVOID *ExternalInfo,
	PVOID *ProviderInfo
	)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus = {0};

	ULONG cbBuffer = 4096;
	UCHAR *pBuffer = (UCHAR *)AllocMemory(cbBuffer);
	if( pBuffer == NULL )
	{
		return E_OUTOFMEMORY;
	}

	Status = NtFsControlFile(FileHandle,NULL,NULL,NULL,
					&IoStatus,
					FSCTL_GET_EXTERNAL_BACKING,
					NULL,0,
					pBuffer,cbBuffer);

	if( Status == STATUS_PENDING )
	{
		LARGE_INTEGER li;
		li.QuadPart = -(10000000 * 10); // maximum wait 10sec
		Status = NtWaitForSingleObject(FileHandle,FALSE,&li);
		if( Status != STATUS_SUCCESS )
		{
			FreeMemory(pBuffer);
			*ExternalInfo = NULL;
			*ProviderInfo = NULL;
			return Status;
		}
		Status = IoStatus.Status;
	}

	if( Status == STATUS_SUCCESS && IoStatus.Information != 0 )
	{
		if( ExternalInfo != NULL && ProviderInfo != NULL )
		{
			PVOID ptr;
			ptr = ReAllocateHeap(pBuffer,(ULONG)IoStatus.Information);

			if( ptr != NULL )
			{
				pBuffer = (UCHAR*)ptr;

				WOF_EXTERNAL_INFO *pwei = (WOF_EXTERNAL_INFO *)pBuffer;

				*ExternalInfo = pwei;

				if( pwei->Provider == WOF_PROVIDER_FILE )
				{
					FILE_PROVIDER_EXTERNAL_INFO_V1 *pFile = (FILE_PROVIDER_EXTERNAL_INFO_V1 *)&pBuffer[8];
					// pFile->Version;
					// pFile->Algorithm
					// pFile->Flags
					*ProviderInfo = pFile;
				}
				else if( pwei->Provider == WOF_PROVIDER_WIM )
				{
					WIM_PROVIDER_EXTERNAL_INFO *pWIM = (WIM_PROVIDER_EXTERNAL_INFO *)&pBuffer[8];
					// pWIM->Version;
					// pWIM->Flags;
					// pWIM->DataSourceId;
					// pWIM->ResourceHash[WIM_PROVIDER_HASH_SIZE];
					*ProviderInfo = pWIM;
				}
				else
				{
					FreeMemory(pwei);
					*ExternalInfo = NULL;
					*ProviderInfo = NULL;
					Status = STATUS_UNSUCCESSFUL;
				}
			}
			else
			{
				FreeMemory(pBuffer);
				Status = STATUS_NO_MEMORY;
			}
		}
		else
		{
			FreeMemory(pBuffer);
		}
	}
	else
	{
		FreeMemory(pBuffer);
		if( ExternalInfo )
			*ExternalInfo = NULL;
		if( ProviderInfo )
			*ProviderInfo = NULL;
	}

	SetLastErrorNtStatusToWin32( Status );

	return HRESULT_FROM_WIN32( RtlGetLastWin32Error() );
}

//----------------------------------------------------------------------------
//
//  FreeWofFileInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------

#define FreeWofFileInformation(ei,pi) FreeMemory(ei) // pi is do not free
#endif

//----------------------------------------------------------------------------
//
//  GetSparseAllocatedRange()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
#pragma pack(8)
typedef struct _FS_SPARSE_ALLOCATED_RANGE {
    LARGE_INTEGER FileOffset;
    LARGE_INTEGER Length;
} FS_SPARSE_ALLOCATED_RANGE, *PFS_SPARSE_ALLOCATED_RANGE;

typedef struct _FS_SPARSE_ALLOCATED_RANGE_BUFFER {
    ULONG ItemCount;
	ULONG BufferSize;
    FS_SPARSE_ALLOCATED_RANGE AllocatedRange[1];
} FS_SPARSE_ALLOCATED_RANGE_BUFFER, *PFS_SPARSE_ALLOCATED_RANGE_BUFFER;
#pragma pack()

PRIVATE
NTSTATUS
APIENTRY
GetSparseAllocatedRange(
	HANDLE hRoot,
	LPCWSTR FilePath,
	FS_SPARSE_ALLOCATED_RANGE_BUFFER **pBuffer
	)
{
	HANDLE hFile;
	NTSTATUS Status;

	if( FilePath != NULL )
	{
		Status = OpenFile_W(&hFile,hRoot, FilePath, FILE_GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,0);

		if( Status != STATUS_SUCCESS )
		{
			return Status;
		}
	}
	else
	{
		hFile = hRoot;
	}

	FILE_ALLOCATED_RANGE_BUFFER ScanRange;
	FS_SPARSE_ALLOCATED_RANGE_BUFFER *AllocatedBuffer;
	DWORD cbBytesReturned = 0;
	DWORD cb = (sizeof(FILE_ALLOCATED_RANGE_BUFFER) * 255) + sizeof(FS_SPARSE_ALLOCATED_RANGE_BUFFER);
	DWORD dwError = 0;

_retry_alloc:
	AllocatedBuffer = (FS_SPARSE_ALLOCATED_RANGE_BUFFER *)AllocMemory( cb );
	if( AllocatedBuffer == NULL )
	{
		NtClose(hFile);
		return STATUS_NO_MEMORY;
	}

	ScanRange.FileOffset.QuadPart = 0;
	_GetFileSize(hFile,&ScanRange.Length,NULL);

	if( !_DeviceIoControl(hFile,
			FSCTL_QUERY_ALLOCATED_RANGES,
			&ScanRange,sizeof(ScanRange),
			AllocatedBuffer->AllocatedRange,cb,
			&cbBytesReturned,&Status) )
	{
		dwError = RtlGetLastWin32Error();
		if(dwError == ERROR_MORE_DATA )
		{
			FreeMemory( AllocatedBuffer );
			cb += (sizeof(FILE_ALLOCATED_RANGE_BUFFER) + 256);
			goto _retry_alloc;
		}
	}
	else
	{
		ULONG cCount = cbBytesReturned / sizeof(FS_SPARSE_ALLOCATED_RANGE);

		cbBytesReturned = sizeof(FS_SPARSE_ALLOCATED_RANGE_BUFFER) + ((cCount - 1) * sizeof(FS_SPARSE_ALLOCATED_RANGE));

		FS_SPARSE_ALLOCATED_RANGE_BUFFER *pRanges = (FS_SPARSE_ALLOCATED_RANGE_BUFFER *)ReallocMemory(AllocatedBuffer,cbBytesReturned); // for shrink
		if( pRanges )
		{
			pRanges->ItemCount = cCount;
			pRanges->BufferSize = cbBytesReturned;
			*pBuffer = pRanges;
		}
		else
		{
			Status = STATUS_NO_MEMORY;
		}
	}

	if( FilePath != NULL )
		NtClose(hFile);

	return Status;
}

/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//
//  NTFile_GatherFileInformation()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
NTFile_GatherFileInformation(
	HANDLE hFile,
	FILE_INFORMATION_STRUCT **ppfi
	)
{
	IO_STATUS_BLOCK IoStatus;

	FILE_INFORMATION_STRUCT *pfi = (FILE_INFORMATION_STRUCT *)AllocMemory( sizeof(FILE_INFORMATION_STRUCT) );
	if( pfi == NULL )
		return E_OUTOFMEMORY;

	//
	// Volume information
	//
	ULONG cbfsai = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + (64);
	FILE_FS_ATTRIBUTE_INFORMATION *fsai = (FILE_FS_ATTRIBUTE_INFORMATION *)AllocMemory(cbfsai);
	NtQueryVolumeInformationFile(hFile,&IoStatus,fsai,cbfsai,FileFsAttributeInformation);
	memcpy(pfi->FileSystemName,fsai->FileSystemName,fsai->FileSystemNameLength);
	pfi->MaximumComponentNameLength = fsai->MaximumComponentNameLength;
	pfi->FileSystemAttributes = fsai->FileSystemAttributes;
	FreeMemory(fsai);

	//
	// File Basic Information
	//
	FILE_BASIC_INFORMATION fbi;
	NtQueryInformationFile(hFile,&IoStatus,&fbi,sizeof(fbi),FileBasicInformation);

	pfi->FileAttributes = fbi.FileAttributes;
	pfi->LastWriteTime = fbi.LastWriteTime;
	pfi->CreationTime = fbi.CreationTime;
	pfi->LastAccessTime = fbi.LastAccessTime;
	pfi->ChangeTime = fbi.ChangeTime;

	//
	// File Standard Information
	//
	FILE_STANDARD_INFORMATION fsi;
	NtQueryInformationFile(hFile,&IoStatus,&fsi,sizeof(fsi),FileStandardInformation);

	pfi->AllocationSize = fsi.AllocationSize;
	pfi->EndOfFile = fsi.EndOfFile;
	pfi->NumberOfLinks = fsi.NumberOfLinks;
	pfi->Directory = fsi.Directory;
	pfi->DeletePending = fsi.DeletePending;

	//
	// File Internal Information (Fild Id/File Reference Number)
	//
	FILE_INTERNAL_INFORMATION FileId;
	if( NtQueryInformationFile(hFile,&IoStatus,&FileId,sizeof(FileId),FileInternalInformation) == STATUS_SUCCESS )
	{
		pfi->FileReferenceNumber = FileId.IndexNumber;
	}

	//
	// File Name / Alternate (Short) Name Information
	//
	ULONG cb = sizeof(FILE_NAME_INFORMATION) + WIN32_MAX_PATH_BYTES;
	FILE_NAME_INFORMATION *pfni = (FILE_NAME_INFORMATION *)AllocMemory(cb);
	if( pfni )
	{
		NtQueryInformationFile(hFile,&IoStatus,pfni,cb,FileNameInformation);
		pfni->FileName[ pfni->FileNameLength / sizeof(WCHAR) ] = UNICODE_NULL;
		pfi->Name = DuplicateString(pfni->FileName);

		if( NtQueryInformationFile(hFile,&IoStatus,pfni,cb,FileAlternateNameInformation) == STATUS_SUCCESS )
		{
			pfni->FileName[ pfni->FileNameLength / sizeof(WCHAR) ] = UNICODE_NULL;
			pfi->ShortName = DuplicateString(pfni->FileName);
		}

		FreeMemory(pfni);
	}

	//
	// Alternate Stream Information
	//
	{
		FILE_STREAM_INFORMATION *StreamInformation = NULL;
		if( GetAlternateStreamNames(hFile,&pfi->AltStream.cAltStreamName,&StreamInformation) == STATUS_SUCCESS )
		{
			if( pfi->AltStream.cAltStreamName > 0 )
			{
				pfi->AltStream.AltStreamName = (FILE_INFORMATION_ALTSTREAM*)AllocMemory( pfi->AltStream.cAltStreamName * sizeof(FILE_INFORMATION_ALTSTREAM) );

				FILE_STREAM_INFORMATION *ps = StreamInformation;
				for(int i = 0;;i++)
				{
					pfi->AltStream.AltStreamName[i].Name = AllocStringLengthCb(ps->StreamName,ps->StreamNameLength);
					pfi->AltStream.AltStreamName[i].Size = ps->StreamSize;
					pfi->AltStream.AltStreamName[i].AllocSize = ps->StreamAllocationSize;
					if( ps->NextEntryOffset == 0 )
						break;
					ps = (FILE_STREAM_INFORMATION *)((ULONG_PTR)ps + ps->NextEntryOffset);
				}
			}

			FreeMemory(StreamInformation);
		}
	}

	//
	// EA(Extended Attributes)
	//
	{
		FILE_EA_INFORMATION fei = {0};
		NtQueryInformationFile(hFile,&IoStatus,&fei,sizeof(FILE_EA_INFORMATION),FileEaInformation);
		if( fei.EaSize != 0 )
		{
			pfi->EaSize = fei.EaSize;
			GetEAInformation(hFile,NULL,&pfi->EaBuffer);
		}
	}

	//
	// Object IDs Information
	//
	FILE_OBJECTID_BUFFER fob = {0};
	if( GetObjectIdInformation(hFile,&fob) == STATUS_SUCCESS )
	{
		memcpy(pfi->ObjectId.ObjectId,fob.ObjectId,16);
		memcpy(pfi->ObjectId.BirthVolumeId,fob.BirthVolumeId,16);
		memcpy(pfi->ObjectId.BirthObjectId,fob.BirthObjectId,16);
		memcpy(pfi->ObjectId.DomainId,fob.DomainId,16);

		pfi->State.ObjectId = 1;
	}

	//
	// Reparse Point Information
	//
	if( pfi->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT )
	{
		if( GetReparsePointInformation(hFile,NULL,&pfi->ReparsePointInfo,sizeof(FS_REPARSE_POINT_INFORMATION_EX),0) )
		{
			pfi->State.ReparsePoint = 1;
		}
	}

	//
	// File Compress Information
	//
	if( pfi->FileAttributes & FILE_ATTRIBUTE_COMPRESSED )
	{
		FILE_COMPRESSION_INFORMATION fci = {0};
		if( NT_SUCCESS(NtQueryInformationFile(hFile,&IoStatus,&fci,sizeof(fci),FileCompressionInformation)) )
		{
			;
		}
	}

	//
	// Sparse File Information
	//
	FS_SPARSE_ALLOCATED_RANGE_BUFFER *pBuffer = NULL;
	ULONG cbBufferSize = 0;
	if( pfi->FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE )
	{
		GetSparseAllocatedRange(hFile,NULL,&pBuffer);
	}
	
	//
	// Wof Information
	//
	if( (pfi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
	{
		if( GetWofInformation(hFile,(PVOID*)&pfi->Wof.ExternalInfo,&pfi->Wof.GenericPtr) == STATUS_SUCCESS )
		{
			pfi->State.Wof = true;
		}
	}

	*ppfi = pfi;

	return S_OK;
}

//---------------------------------------------------------------------------
//
//  NTFile_FreeFileInformation()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
NTFile_FreeFileInformation(
	FILE_INFORMATION_STRUCT *pfi
	)
{
	if( pfi )
	{
		FreeReparsePointInformation(&pfi->ReparsePointInfo);

		FreeEAInformation(pfi->EaBuffer);

		FreeMemory(pfi->Name);
		FreeMemory(pfi->ShortName);

		int i;
		for(i = 0; i < pfi->AltStream.cAltStreamName; i++)
			FreeMemory(pfi->AltStream.AltStreamName[i].Name);
		FreeMemory(pfi->AltStream.AltStreamName);

		FreeWofInformation(pfi->Wof.ExternalInfo,pfi->Wof.GenericPtr);

		FreeMemory(pfi);
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//  NTFile_GetAttributeString()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
static struct {
	DWORD AttributeFlag;
	TCHAR AttributeChar;
} attributes_char[] = {
	{FILE_ATTRIBUTE_DIRECTORY,            L'd'},
	{FILE_ATTRIBUTE_READONLY,             L'r'},
	{FILE_ATTRIBUTE_HIDDEN,               L'h'},
	{FILE_ATTRIBUTE_SYSTEM,               L's'},
	{FILE_ATTRIBUTE_ARCHIVE,              L'a'},
	{FILE_ATTRIBUTE_ENCRYPTED,            L'e'},
	{FILE_ATTRIBUTE_COMPRESSED,           L'c'},
	{FILE_ATTRIBUTE_REPARSE_POINT,        L'l'},
	{FILE_ATTRIBUTE_SPARSE_FILE,          L'p'},
	{FILE_ATTRIBUTE_TEMPORARY,            L't'},
	{FILE_ATTRIBUTE_VIRTUAL,              L'v'},
	{FILE_ATTRIBUTE_OFFLINE,              L'o'},
	{FILE_ATTRIBUTE_DEVICE,               L'D'}, // 'x'
	{FILE_ATTRIBUTE_NORMAL,               L'n'},
	{FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,  L'i'},
// Win8/Windows Server 2012
	{FILE_ATTRIBUTE_NO_SCRUB_DATA,        L'X'}, // 'u'->'X'
	{FILE_ATTRIBUTE_INTEGRITY_STREAM,     L'V'}, // 'g'->'V'
// Win10
	{FILE_ATTRIBUTE_EA,                   L'E'},
	{FILE_ATTRIBUTE_PINNED,               L'P'},
	{FILE_ATTRIBUTE_UNPINNED,             L'U'},
	{FILE_ATTRIBUTE_RECALL_ON_OPEN,       L'R'},
	{FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS,L'D'},
	{FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL,  L'Q'},
};

EXTERN_C
BOOL
APIENTRY
NTFile_GetAttributeString(
	DWORD Attributes,
	LPWSTR String,
	int cchString
	)
{
	int i,c;

	c = ARRAYSIZE(attributes_char);

	for(i = 0; i < c; i++)
	{
		if( Attributes & attributes_char[i].AttributeFlag )
		{
			*String++ = attributes_char[i].AttributeChar;
		}
		else
		{
			//*String++ = _T('-');
		}
	}

	*String = L'\0';

	return TRUE;
}

EXTERN_C
BOOL
APIPRIVATE
GetAttributeString(
	DWORD Attributes,
	LPWSTR String,
	int cchString
	)
{
	return NTFile_GetAttributeString(Attributes,String,cchString);
}
