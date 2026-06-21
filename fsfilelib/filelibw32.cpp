//***************************************************************************
//*                                                                         *
//*  filelibw32.cpp                                                         *
//*                                                                         *
//*  Win32 API based functions                                              *
//*                                                                         *
//*  Create: 2024-12-07                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "fsfilelib.h"

EXTERN_C
HRESULT
APIENTRY
NtPathParseDeviceName(
	PCWSTR pszPath,
	PWSTR pszDeviceName,
	int cchDeviceName,
	PWSTR pszDosDeviceName,
	int cchDosDeviceName
	)
{
	HRESULT hr;
	WCHAR szVolume[MAX_PATH];
	SIZE_T cch;

	cch = wcslen(pszPath);

	if( pszDeviceName )
		*pszDeviceName = L'\0';

	if( pszDosDeviceName )
		*pszDosDeviceName = L'\0';
	
	if( (cch >= 4 && pszPath[0] == L'\\' && pszPath[1] == L'?' && pszPath[2] == L'?' && pszPath[3] == L'\\') ||
		(cch >= 4 && pszPath[0] == L'\\' && pszPath[1] == L'\\' && (pszPath[2] == L'.' || pszPath[2] == L'?') && pszPath[3] == L'\\') ||
		(cch >= 8 && (wcsnicmp(pszPath,L"\\Device\\",8) == 0)) )
	{
		// NOTE: UNC path not supports

		WCHAR *pSep = NULL;
		WCHAR *pHead = NULL;

		if( iswalpha(pszPath[4]) && pszPath[5] == L':' )
		{
			// "\??\C:"
			// "\\?\C:"
			pHead = (WCHAR*)&pszPath[4];
		}
		else if( cch >= 48 && pszPath[10] == L'{' && pszPath[47] == L'}' )
		{
			// "\??\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
			// "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
			pHead = (WCHAR*)&pszPath[4];
		}
		else if( pszPath[2] == L'?' || pszPath[2] == L'.' )
		{
			// "\??\HardiskVolume1"
			// "\??\Cdrom1"
			// "\\?\C:"
			// "\\.\C:"
			pHead = (WCHAR*)&pszPath[4];
		}
		else if( pszPath[2] != L'?' )
		{
			if( _wcsnicmp(&pszPath[8],L"LanmanRedirector",16) == 0 )
			{
				// "\Device\LanmanRedirector\xxxxxxxx"
				if( pszDeviceName )
				{
					if( FindDeviceNameFromPath(pszPath,pszDeviceName,cchDeviceName,pszDosDeviceName,cchDosDeviceName) != -1 )
					{
						pHead = NULL;
						pSep  = NULL;
						hr = S_OK;
					}
				}
			}
			else
			{
				// "\Device\xxxxxxxx"
				pHead = (WCHAR*)&pszPath[8];
			}
		}
		else
		{
			// invalid/unknown path
			pHead = (WCHAR*)pszPath;
		}

		if( pHead != NULL )
		{
			pSep = wcspbrk(pHead,L"\\/");

			if( pSep )
			{
				SIZE_T cb = (((SIZE_T)(pSep - pHead)) * sizeof(WCHAR));
				memcpy_s(szVolume,sizeof(szVolume),pHead,cb);
				szVolume[ WCHAR_LENGTH(cb) ] = L'\0';
			}
			else
			{
				StringCchCopy(szVolume,MAX_PATH,pHead);
			}

			if( QueryDosDevice(szVolume,pszDeviceName,cchDeviceName) > 0 )
			{
				if( pszDosDeviceName )
				{
					StringCchCopy(pszDosDeviceName,cchDosDeviceName,szVolume);
				}
				hr = S_OK;
			}
			else
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
			}
		}
	}
	else
	{
		hr = E_INVALIDARG;
	}

	return hr;
}

#define _MAX_TARGET_PATH_BUFFER_SIZE 32768

EXTERN_C
HRESULT
APIENTRY
NtPathDosDriveFromNtDevicePath(
	PCWSTR NtDevicePath,
	PWSTR DosDrive,
	ULONG cchDosDrive,
	ULONG Flags,
	PCWSTR *RootDirectoryPart
	)
{
	WCHAR drive_buffer[ (26 * (3 + 1)) + 1 ]; // "'A:\'\0 ... 'Z:\'\0\0"
	WCHAR *drive;
	const DWORD cchTargetPathBuffer = _MAX_TARGET_PATH_BUFFER_SIZE;
	WCHAR szTargetPath[_MAX_TARGET_PATH_BUFFER_SIZE];
	DWORD cchTargetPath;
	HRESULT hr = S_FALSE;

	GetLogicalDriveStrings(ARRAYSIZE(drive_buffer),drive_buffer);

	*DosDrive = L'\0';

	drive = drive_buffer;

	while( *drive )
	{
		// "C:\" -> "C:"
		drive[2] = L'\0';
		cchTargetPath = QueryDosDevice(drive,szTargetPath,cchTargetPathBuffer);
		drive[2] = L'\\';

		int iret;
		if( Flags & DDNTF_DEVICENAME_COMPARE )
		{
			iret = HasPrefix(szTargetPath,NtDevicePath) ? 0 : 1;

			if( iret == 0 && RootDirectoryPart && (Flags && DDNTF_RETURN_ROOTDIRECTORY_POINT) )
			{
				SIZE_T cchTargetPath = wcslen(szTargetPath);
				SIZE_T cchNtDevicePath = wcslen(NtDevicePath);

				if( cchNtDevicePath >= cchTargetPath )
				{
					*RootDirectoryPart = &NtDevicePath[ cchTargetPath ];
				}
			}
		}
		else
		{
			iret = wcsicmp(szTargetPath,NtDevicePath);
		}

		if( iret == 0 )
		{
			if( (Flags & DDNTF_RETURN_DRIVE_ROOT) == 0 )
				drive[2] = L'\0';
			hr = StringCchCopy(DosDrive,cchDosDrive,drive);
			break;
		}

		drive += (wcslen(drive) + 1);
	}

	return hr;
}

EXTERN_C
PWSTR
WINAPI
NtPathMakeFullyQualifiedFileNameWithTypeName(
	PCWSTR pszFileName,
	PCWSTR pszStreamName,
	PCWSTR pszTypeName
	)
{
	SIZE_T cch;

	if( pszFileName == NULL || pszTypeName == NULL )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	cch = wcslen(pszFileName);

	if( pszStreamName )
		cch += wcslen(pszStreamName);

	if( pszTypeName )
		cch += wcslen(pszTypeName);

	cch += 2; // ':' + ':'
	cch += 1; // Terminate Null

	PWSTR psz = (PWSTR)AllocMemory(cch * sizeof(WCHAR));
	if( psz == NULL )
	{
		SetLastError( ERROR_OUTOFMEMORY );
		return NULL;
	}

#if 0
	StringCchCopy(psz,cch,pszFileName);
	StringCchCat(psz,cch,L":");
	if( pszStreamName )
		StringCchCat(psz,cch,pszStreamName);
	StringCchCat(psz,cch,L":");
	StringCchCat(psz,cch,pszTypeName);
#else
	StringCchPrintf(psz,cch,
			L"%s:%s:%s",
			pszFileName,
			pszStreamName ? pszStreamName : L"",
			pszTypeName);
#endif

	SetLastError( ERROR_SUCCESS );

	return psz;
}

EXTERN_C
HRESULT
WINAPI
NtDosGetAlternateStreams(
    PCWSTR pszFilePath,
    NT_FILE_STREAM_INFORMATION_EX **pAltStmNames,
    INT *pAltStmNameCount
    )
{
    NTSTATUS Status;
    HRESULT hr;
    DWORD dwMatched = 0;
    HANDLE hFile;
    ULONG DesiredAccess = FILE_READ_ATTRIBUTES|SYNCHRONIZE;
    ULONG Option = FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_SYNCHRONOUS_IO_NONALERT;

    if( pszFilePath == NULL || pAltStmNames == NULL || pAltStmNameCount == NULL )
        return E_INVALIDARG;

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

                ULONG cbBuffer = (sizeof(NT_FILE_STREAM_INFORMATION_EX) * AltStreamCount) + cbNameBuffer + sizeof(WCHAR);
                PBYTE pb;
				pb = (PBYTE)LocalAlloc( LPTR, cbBuffer );
                if( pb )
                {
                    ZeroMemory(pb,cbBuffer);

                    FILE_STREAM_INFORMATION *p = StreamInformation;
                    int iIndex = 0;
                    NT_FILE_STREAM_INFORMATION_EX *pAltName = (NT_FILE_STREAM_INFORMATION_EX *)pb;
                    WCHAR *pNameStorePos = (WCHAR *)(pb + (sizeof(NT_FILE_STREAM_INFORMATION_EX) * AltStreamCount));
                    do
                    {
                        pAltName[iIndex].StreamSize = p->StreamSize;
                        pAltName[iIndex].StreamAllocationSize = p->StreamAllocationSize;
                        memcpy(pNameStorePos,p->StreamName,p->StreamNameLength);
                        pAltName[iIndex].StreamName = pNameStorePos;
                        pAltName[iIndex].Order = iIndex;

                        iIndex++;

                        pNameStorePos += (WCHAR_LENGTH(p->StreamNameLength) + 1);

                        if( p->NextEntryOffset == 0 )
                            break;

                        p = (FILE_STREAM_INFORMATION *)((ULONG_PTR)p + p->NextEntryOffset);
                    }
                    while( p != NULL );

                    *pAltStmNames = pAltName;
                    *pAltStmNameCount = iIndex;

                    hr = S_OK;
                }
                else
                {
                    *pAltStmNames = NULL;
                    *pAltStmNameCount = 0;
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                *pAltStmNames = NULL;
                *pAltStmNameCount = 0;
                hr = S_FALSE;
            }

            FreeAlternateStreamInformation(StreamInformation);
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

static
HRESULT
_GetStreamInformation(
    PCWSTR pszFile,
    BOOL bDirectory,
    FILE_STANDARD_INFO *pStdInfo,
    FILE_BASIC_INFO *pBasicInfo
    )
{
    NTSTATUS Status;
    HANDLE hFile;

    Status = OpenFileEx_W(&hFile,pszFile,
                    FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_SYNCHRONOUS_IO_NONALERT);

    if( Status == STATUS_SUCCESS )
    {
        if( pStdInfo )
        {
            if( GetFileInformationByHandleEx(hFile,FileStandardInfo,pStdInfo,sizeof(FILE_STANDARD_INFO)) )
            {
                ;
            }
        }

        if( pBasicInfo )
        {
            if( GetFileInformationByHandleEx(hFile,FileBasicInfo,pBasicInfo,sizeof(FILE_BASIC_INFO)) )
            {
                ;
            }
        }

        NtClose(hFile);

        return S_OK;
    }

    return HRESULT_FROM_WIN32( NtStatusToDosError(Status) );
}

EXTERN_C
HRESULT
WINAPI
NtDosGetAttributeTypeCodeStreams(
    PCWSTR pszFilePath,
    BOOL bDirectory,
    NT_FILE_STREAM_INFORMATION_EX **pStmNames,
    INT *pStmNameCount
    )
{
    static PWSTR typeNameDir[] = {
        L"::$INDEX_ALLOCATION",
        L"::$BITMAP",
    };

    static PWSTR typeNameFile[] = {
        L"::$DATA",
        L"::$ATTRIBUTE_LIST",
        L"::$REPARSE_POINT",
        L"::$EA",
        L"::$LOGGED_UTILITY_STREAM",
    };

    PWSTR *aTyeName;
    int TypeNameCount;

    if( bDirectory )
    {
        aTyeName = typeNameDir;
        TypeNameCount = _countof(typeNameDir);
    }
    else
    {
        aTyeName = typeNameFile;
        TypeNameCount = _countof(typeNameFile);
    }

    // calc typename length
    SIZE_T cbNameBuffer = 0;
    for(int i = 0; i < TypeNameCount; i++)
    {
        cbNameBuffer += ((wcslen(aTyeName[i]) + 1) * sizeof(WCHAR));
    }

    // create buffer
    SIZE_T cbBuffer = (sizeof(NT_FILE_STREAM_INFORMATION_EX) * TypeNameCount) + cbNameBuffer + sizeof(WCHAR);
    PBYTE pb;
    pb = (PBYTE)LocalAlloc( LPTR, cbBuffer );
    if( pb == NULL )
    {
        return E_OUTOFMEMORY;
    }

    // e.g. Returns buffer layout
    //
    // +--------------------------------+--------------------------------+-------+-------+--+
    // |NT_FILE_STREAM_INFORMATION_EX[0]|NT_FILE_STREAM_INFORMATION_EX[1]|Name1\0|Name2\0|\0|
    // +--------------------------------+--------------------------------+-------+-------+--+
    //               |                              |                    |       |
    //               +---------------------------------------------------+       |
    //                                              +----------------------------+
    ZeroMemory(pb,cbBuffer);

    NT_FILE_STREAM_INFORMATION_EX *pStruct = (NT_FILE_STREAM_INFORMATION_EX *)pb;
    WCHAR *pNameStorePos = (WCHAR *)(pb + (sizeof(NT_FILE_STREAM_INFORMATION_EX) * TypeNameCount));

    for(int i = 0; i < TypeNameCount; i++)
    {
        PWSTR Name = aTyeName[i];

        PWSTR pszFullyQualifiedPath = NtPathMakeFullyQualifiedFileNameWithTypeName(pszFilePath,NULL,&Name[2]);

        if( pszFullyQualifiedPath )
        {
            FILE_STANDARD_INFO fsi = {};
            _GetStreamInformation(pszFullyQualifiedPath,bDirectory,&fsi,NULL);

            pStruct[i].Order = i;
            pStruct[i].StreamName = pNameStorePos;
            pStruct[i].StreamSize.QuadPart = fsi.EndOfFile.QuadPart;
            pStruct[i].StreamAllocationSize.QuadPart = fsi.AllocationSize.QuadPart;

            SIZE_T cbName = ((wcslen(Name) + 1) * sizeof(WCHAR));

            memcpy(pNameStorePos,Name,cbName);
            pNameStorePos += (cbName/sizeof(WCHAR));

            NtDosFreeMemory( pszFullyQualifiedPath );
        }
    }

    *pStmNames = (NT_FILE_STREAM_INFORMATION_EX*)pb;
    *pStmNameCount = TypeNameCount;

    return S_OK;
}
