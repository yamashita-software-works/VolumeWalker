//***************************************************************************
//*                                                                         *
//*  ntfilehelp.cpp                                                         *
//*                                                                         *
//*  Create: 2022-03-29                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include <ntifs.h>
#include <stdio.h>
#include <malloc.h>
#include <strsafe.h>
#include <ntstrsafe.h>
#include <locale.h>
#include <conio.h>
#include <winerror.h>
#include "ntnativeapi.h"
#include "ntnativehelp.h"
#include "ntobjecthelp.h"
#include "ntfileid.h"
#include "libntwdk.h"

HRESULT GetNtPath(PCWSTR DosPathName,PWSTR *NtPath,PCWSTR *NtFileNamePart)
{
    UNICODE_STRING NtPathName = {0};
    if( RtlDosPathNameToNtPathName_U(DosPathName,&NtPathName,(PWSTR*)NtFileNamePart,NULL) )
    {
        *NtPath = NtPathName.Buffer;
        return S_OK;
    }
    return E_FAIL;
}

HRESULT GetNtPath_U(PCWSTR DosPathName,UNICODE_STRING *NtPath,PCWSTR *NtFileNamePart)
{
    if( RtlDosPathNameToNtPathName_U(DosPathName,NtPath,(PWSTR*)NtFileNamePart,NULL) )
    {
        return S_OK;
    }
    return E_FAIL;
}

ULONG
GetPathType(
    PCWSTR pszPath
    )
{
    if( HasPrefix(L"\\Device\\",pszPath) )
    {
        // NT Device Path
        // "\Device\Xxxx"
        // "\Device\HarddiskVolumeX"
        // "\Device\CdRomX"
        // "\Device\LanmanRedirector;\..."
        return PATHTYPE_NT_DEVICE;
    }

    if( HasPrefix(L"\\??\\",pszPath) )
    {
        // DOS Device (NT Object Namespace)
        // "\??\C:"
        // "\??\C:\"
        // "\??\C:\foo"
        // "\??\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
        // "\??\HarddiskVolumeX"
        // "\??\unc\"
        // "\??\unc\Server\Share"
        return PATHTYPE_NT_DOS_DEVICE;
    }

    if( HasPrefix(L"\\\\?\\",pszPath) )
    {
        // Win32 Path
        // "\\?\C:"
        // "\\?\C:\"
        // "\\?\C:\foo"
        // "\\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
        return PATHTYPE_WIN32;
    }

    if( HasPrefix(L"\\\\.\\",pszPath) )
    {
        // Win32 Device
        // "\\.\d:
        // "\\.\PhysicalDriveN
        return 0; // not support
    }

    if( HasPrefix(L"\\\\",pszPath) )
    {
        // Win32 UNC 
        // "\\Server\Share"
        // "\\Server"
        return 0; // not support
    }

    if( _DOS_DRIVE_CHAR(pszPath[0]) && pszPath[1] == L':' )
    {
        // DOS Drive
        // "d:\<Path>"
        // "C:\Windows"
        return PATHTYPE_DOS_DRIVE;
    }

    return 0;
}

ULONG
GetPathType_U(
    UNICODE_STRING *pusPath
    )
{
    ULONG Type = 0;
    PWSTR psz = AllocateSzFromUnicodeString(pusPath);
    if( psz ) {
        Type = GetPathType(psz);
        FreeMemory(psz);
    }
    return Type;
}

HRESULT
GetVolumeGuidName(
    PCWSTR NtDeviceName,
    PWSTR GuidName,
    int cchGuidName,
    ULONG PathType
    )
{
    HRESULT hr;
    NTSTATUS Status;
    WCHAR szVolumeGuid[64];

    Status = LookupVolumeGuidName(NtDeviceName,szVolumeGuid,ARRAYSIZE(szVolumeGuid));
    if( Status != STATUS_SUCCESS )
        return HRESULT_FROM_WIN32( RtlNtStatusToDosError(Status) );

    if( PATHTYPE_WIN32 == PathType )
        hr = StringCchPrintf(GuidName,cchGuidName,L"\\\\?\\%s",szVolumeGuid);
    else if( PATHTYPE_NT_DEVICE ==  PathType )
        hr = StringCchPrintf(GuidName,cchGuidName,L"\\??\\%s",szVolumeGuid);
    else
        hr = StringCchCopy(GuidName,cchGuidName,szVolumeGuid);

    return hr;
}

EXTERN_C
HRESULT
NTAPI
SplitRootPath_W(
    PCWSTR pszFullyQualifiedPath,
    PWSTR *RootDirectory,
    PULONG RootDirectoryLength,
    PWSTR *RelativePath,
    PULONG RelativePathLength
    )
{
    PCWSTR pszRoot;

    // "\Device\HarddiskVolumeX\Foo\Bar"
    // "\??\C:\Foo\Bar"
    // "\\?\C:\Foo\Bar"
    // "\??\HarddiskVolume1\Foo\Bar"
    // "\\?\HarddiskVolume1\Foo\Bar"
    // "\??\unc\server\share\"
    //
    int cchFullPath = (int)wcslen(pszFullyQualifiedPath);
    int cchRoot = 0;
    int cchRelativePath = 0;

    if( cchFullPath >= 8 && wcsnicmp(pszFullyQualifiedPath,L"\\Device\\",8) == 0 )
    {
        pszRoot = wcschr(&pszFullyQualifiedPath[8],L'\\');
    }
    else if( cchFullPath >= 4 && (wcsncmp(pszFullyQualifiedPath,L"\\??\\",4) == 0 || wcsncmp(pszFullyQualifiedPath,L"\\\\?\\",4) == 0) )
    {
        if( cchFullPath >= 8 && wcsnicmp(&pszFullyQualifiedPath[4],L"unc",3) == 0 )
        {
            pszRoot = wcschr(&pszFullyQualifiedPath[8],L'\\'); // find servername
            if( pszRoot )
                pszRoot = wcschr(++pszRoot,L'\\'); // find share name
        }
        else
        {
            pszRoot = wcschr(&pszFullyQualifiedPath[4],L'\\');
        }
    }
    else
    {
        return E_INVALIDARG;
    }

    if( pszRoot == NULL )
        return E_INVALIDARG;

    ++pszRoot;
    cchRoot = (int)(pszRoot - pszFullyQualifiedPath);
    cchRelativePath = (int)wcslen(pszRoot);

    if( RootDirectory )
    {
        *RootDirectory = AllocStringBuffer(cchRoot+1);
        memcpy(*RootDirectory,pszFullyQualifiedPath,cchRoot*sizeof(WCHAR));
        (*RootDirectory)[cchRoot] = L'\0';
    }

    if( RootDirectoryLength )
    {
        *RootDirectoryLength = cchRoot;
    }

    if( RelativePath )
    {
        *RelativePath = DuplicateString(pszRoot);
    }

    if( RelativePathLength )
    {
        *RelativePathLength = cchRelativePath;
    }

    if( (RootDirectory && *RootDirectory == NULL) || (RelativePath && *RelativePath == NULL) )
    {
        FreeMemory(*RootDirectory);
        FreeMemory(*RelativePath);
        return E_OUTOFMEMORY;		
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  OpenFileEx_W()
//
//  Open the file with splitting the volume root and relative file path.
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
OpenFileEx_W(
    PHANDLE phFile,
    PCWSTR PathName,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    ULONG OpenOptions
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus = {0};
    NTSTATUS Status;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    UNICODE_STRING usPath;
    HANDLE hRoot;
    PWSTR Root = NULL;
    PWSTR RelativePath = NULL;

    if( phFile == NULL )
        return STATUS_INVALID_PARAMETER;

    *phFile = INVALID_HANDLE_VALUE;

    if( SplitRootPath_W(PathName,&Root,NULL,&RelativePath,NULL) != S_OK )
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlInitUnicodeString(&usPath,Root);

    InitializeObjectAttributes(&ObjectAttributes,&usPath,0,NULL,NULL);
    
    Status = NtOpenFile(&hRoot,
                    STANDARD_RIGHTS_READ|FILE_LIST_DIRECTORY|FILE_READ_ATTRIBUTES|FILE_READ_EA|SYNCHRONIZE,
                    &ObjectAttributes,
                    &IoStatus,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

    if( Status == STATUS_SUCCESS )
    {
        RtlInitUnicodeString(&usPath,RelativePath);

        InitializeObjectAttributes(&ObjectAttributes,&usPath,0,hRoot,NULL);

        Status = NtOpenFile(&hFile,
                        DesiredAccess,
                        &ObjectAttributes,
                        &IoStatus,
                        ShareAccess,
                        OpenOptions);

        if( Status == STATUS_SUCCESS )
        {
            *phFile = hFile;
        }

        NtClose(hRoot);
    }

    FreeMemory(RelativePath);
    FreeMemory(Root);

    return Status;
}

//----------------------------------------------------------------------------
//
//  OpenFile_W()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
OpenFile_W(
    PHANDLE phFile,
    HANDLE hRoot,
    PCWSTR PathName,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    ULONG OpenOptions
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus = {0};
    NTSTATUS Status;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    UNICODE_STRING usPath;

    RtlInitUnicodeString(&usPath,PathName);

    InitializeObjectAttributes(&ObjectAttributes,&usPath,0,hRoot,NULL);

    Status = NtOpenFile(&hFile,
                    DesiredAccess,
                    &ObjectAttributes,
                    &IoStatus,
                    ShareAccess,
                    OpenOptions);

    if( Status != STATUS_SUCCESS )
    {
        ;//todo:
    }

    *phFile = hFile;

    return Status;
}

//----------------------------------------------------------------------------
//
//  OpenFile_U()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
OpenFile_U(
    PHANDLE phFile,
    HANDLE hRoot,
    UNICODE_STRING *PathName,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    ULONG OpenOptions
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus = {0};
    NTSTATUS Status;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    InitializeObjectAttributes(&ObjectAttributes,PathName,0,hRoot,NULL);

    Status = NtOpenFile(&hFile,
                    DesiredAccess,
                    &ObjectAttributes,
                    &IoStatus,
                    ShareAccess,
                    OpenOptions);

    if( Status != STATUS_SUCCESS )
    {
        ;//todo:
    }

    *phFile = hFile;

    return Status;
}

//----------------------------------------------------------------------------
//
//  OpenFile_ID()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
OpenFile_ID(
    PHANDLE phFile,
    HANDLE hVolume,
    PFS_FILE_ID_DESCRIPTOR FileIdDesc,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    ULONG OpenOptions
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus = {0};
    NTSTATUS Status;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    UNICODE_STRING NtPathName;

    switch( FileIdDesc->Type )
    {
        case FsFileIdType:
            NtPathName.Length = NtPathName.MaximumLength = (USHORT)sizeof(FileIdDesc->FileId);
            NtPathName.Buffer = (PWCH)&FileIdDesc->FileId;
            break;
        case FsObjectIdType:
            NtPathName.Length = NtPathName.MaximumLength = (USHORT)sizeof(FileIdDesc->ObjectId);
            NtPathName.Buffer = (PWCH)&FileIdDesc->ObjectId;
            break;
        case FsExtendedFileIdType:
            NtPathName.Length = NtPathName.MaximumLength = (USHORT)sizeof(FileIdDesc->ExtendedFileId);
            NtPathName.Buffer = (PWCH)&FileIdDesc->ExtendedFileId;
            break;
        default:
            Status = STATUS_INVALID_PARAMETER;
            RtlSetLastWin32Error( RtlNtStatusToDosError(Status) );
            return Status;
    }

    InitializeObjectAttributes(&ObjectAttributes,&NtPathName,0,hVolume,NULL);

    OpenOptions |= FILE_OPEN_BY_FILE_ID;

    Status = NtOpenFile(&hFile,
                    DesiredAccess,
                    &ObjectAttributes,
                    &IoStatus,
                    ShareAccess,
                    OpenOptions);

    *phFile = hFile;

    RtlSetLastWin32Error( RtlNtStatusToDosError(Status) );

    return Status;
}

//----------------------------------------------------------------------------
//
//  CreateFile_U()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
CreateFile_U(
    PHANDLE FileHandle,
    HANDLE hRoot,
    UNICODE_STRING *NtFilePath,
    PVOID SecurityDescriptor,
    PLARGE_INTEGER AllocationSize,
    ULONG DesiredAccess,
    ULONG FileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    PVOID EaBuffer,
    ULONG EaLength
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;

    InitializeObjectAttributes(&ObjectAttributes,NtFilePath,0,hRoot,SecurityDescriptor);

    Status = NtCreateFile(FileHandle,
                        DesiredAccess,
                        &ObjectAttributes,
                        &IoStatus,
                        AllocationSize,
                        FileAttributes,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        EaBuffer,
                        EaLength);

    if( Status != STATUS_SUCCESS )
    {
        ;//todo:
    }
    return Status;
}

//----------------------------------------------------------------------------
//
//  CreateDirectory_W()
//
//----------------------------------------------------------------------------
EXTERN_C
LONG
NTAPI
CreateDirectory_W(
    HANDLE hRoot,
    LPCWSTR NewDirectory,
    SECURITY_ATTRIBUTES *SecurityAttributes
    )
{
    HANDLE hDir = NULL;
    IO_STATUS_BLOCK IoStatus = {0};
    UNICODE_STRING NtPathName = {0,0,NULL};
    NTSTATUS Status;

    RtlInitUnicodeString( &NtPathName, NewDirectory );

    Status = CreateFile_U(
                &hDir,
                hRoot,
                &NtPathName,
                SecurityAttributes != NULL ? SecurityAttributes->lpSecurityDescriptor : NULL,
                NULL,
                SYNCHRONIZE|FILE_LIST_DIRECTORY,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                FILE_CREATE,
                FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT|
                FILE_SYNCHRONOUS_IO_NONALERT|FILE_DIRECTORY_FILE,
                NULL,0);

    if( hDir != NULL && hDir != INVALID_HANDLE_VALUE )
        NtClose(hDir);

    return Status;
}

//----------------------------------------------------------------------------
//
//  GetFileAttributes_U()
//
//----------------------------------------------------------------------------
NTSTATUS GetFileAttributes_U( HANDLE RootHandle, UNICODE_STRING *FilePath, ULONG *pulFileAttributes )
{
    HANDLE Handle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus = {0};
    OBJECT_ATTRIBUTES ObjectAttributes;

    if( pulFileAttributes == NULL || FilePath == NULL )
        return STATUS_INVALID_PARAMETER;

    if( RootHandle != NULL && FilePath->Length == 0 )
    {
        Handle = RootHandle;
        Status = STATUS_SUCCESS;
    }
    else
    {
        InitializeObjectAttributes(&ObjectAttributes,FilePath,0,RootHandle,NULL);

        Status = NtOpenFile(&Handle,FILE_READ_ATTRIBUTES,
                        &ObjectAttributes,&IoStatus,FILE_SHARE_READ|FILE_SHARE_WRITE,
                        FILE_OPEN_REPARSE_POINT);
    }

    if( Status == STATUS_SUCCESS )
    {
        FILE_BASIC_INFORMATION fbi = {0};

        Status = NtQueryInformationFile(Handle,&IoStatus,&fbi,sizeof(fbi),FileBasicInformation);

        if( Status == STATUS_SUCCESS && pulFileAttributes != NULL )
        {
            *pulFileAttributes = fbi.FileAttributes;
        }

        if( !(RootHandle != NULL && FilePath->Length == 0) )
            NtClose(Handle);
    }

    return Status;
}

//----------------------------------------------------------------------------
//
//  GetFileAttributes_W()
//
//----------------------------------------------------------------------------
NTSTATUS GetFileAttributes_W( HANDLE RootHandle, PCWSTR FilePath,  ULONG *pulFileAttributes )
{
    UNICODE_STRING usFilePath;
    RtlInitUnicodeString(&usFilePath,FilePath);
    return GetFileAttributes_U(RootHandle, &usFilePath, pulFileAttributes);
}

//----------------------------------------------------------------------------
//
//  GetFileDateTime_U()
//
//----------------------------------------------------------------------------
NTSTATUS GetFileDateTime_U( HANDLE RootHandle, UNICODE_STRING *FilePath, FILE_BASIC_INFORMATION *pfbi )
{
    if( FilePath == NULL )
        return STATUS_INVALID_PARAMETER;

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes,FilePath,0,RootHandle,NULL);

    HANDLE Handle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus = {0};

    Status = NtOpenFile(&Handle,FILE_READ_ATTRIBUTES,
                    &ObjectAttributes,&IoStatus,FILE_SHARE_READ|FILE_SHARE_WRITE,0);

    if( Status == STATUS_SUCCESS )
    {
        FILE_BASIC_INFORMATION fbi = {0};

        Status = NtQueryInformationFile(Handle,&IoStatus,&fbi,sizeof(fbi),FileBasicInformation);

        if( Status == STATUS_SUCCESS && pfbi != NULL)
        {
            *pfbi = fbi;
        }

        NtClose(Handle);
    }

    return Status;
}

//----------------------------------------------------------------------------
//
//  GetFileDateTime()
//
//----------------------------------------------------------------------------
NTSTATUS GetFileDateTime( HANDLE RootHandle, PCWSTR FilePath, FILE_BASIC_INFORMATION *pfbi )
{
    if( FilePath == NULL || pfbi == NULL )
        return STATUS_INVALID_PARAMETER;

    UNICODE_STRING usPath;
    RtlInitUnicodeString(&usPath,FilePath);
    return GetFileDateTime_U(RootHandle, &usPath, pfbi);
}

//----------------------------------------------------------------------------
//
//  GetFileDateTime_W()
//
//----------------------------------------------------------------------------
NTSTATUS NTAPI GetFileDateTime_W(HANDLE RootHandle, PCWSTR FilePath, FS_FILE_DATE_TIME *pfdt)
{
    NTSTATUS Status;
    FILE_BASIC_INFORMATION fbi;
    UNICODE_STRING usPath;
    RtlInitUnicodeString(&usPath,FilePath);
    Status = GetFileDateTime_U(RootHandle, &usPath, &fbi);
    if( Status == STATUS_SUCCESS )
    {
        pfdt->CreationTime   = fbi.CreationTime;
        pfdt->LastAccessTime = fbi.LastAccessTime;
        pfdt->LastWriteTime  = fbi.LastWriteTime;
        pfdt->ChangeTime     = fbi.ChangeTime;
    }
    return Status;
}

//----------------------------------------------------------------------------
//
//  SetFileDateTime_U()
//
//----------------------------------------------------------------------------
NTSTATUS SetFileDateTime_U( HANDLE RootHandle, UNICODE_STRING *FilePath, FILE_BASIC_INFORMATION *pBasicInfo )
{
    OBJECT_ATTRIBUTES ObjectAttributes;

    InitializeObjectAttributes(&ObjectAttributes,FilePath,0,RootHandle,NULL);

    HANDLE Handle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus = {0};

    Status = NtOpenFile(&Handle,FILE_WRITE_ATTRIBUTES,
                    &ObjectAttributes,&IoStatus,FILE_SHARE_READ|FILE_SHARE_WRITE,0);

    if( Status == STATUS_SUCCESS )
    {
        FILE_BASIC_INFORMATION fbi = {0};

        fbi = *pBasicInfo;
//		fbi.FileAttributes = (ULONG)-1; // no change file attributes
        fbi.FileAttributes = (ULONG)0;  // no change file attributes

        Status = NtSetInformationFile(Handle,&IoStatus,&fbi,sizeof(fbi),FileBasicInformation);

        NtClose(Handle);
    }

    return Status;
}

//----------------------------------------------------------------------------
//
//  SetFileDateTime()
//
//----------------------------------------------------------------------------
NTSTATUS SetFileDateTime( HANDLE RootHandle, PCWSTR FilePath, FILE_BASIC_INFORMATION *pfbi )
{
    if( FilePath == NULL || pfbi == NULL )
        return STATUS_INVALID_PARAMETER;

    UNICODE_STRING usPath;
    RtlInitUnicodeString(&usPath,FilePath);
    return SetFileDateTime_U(RootHandle, &usPath, pfbi);
}

//----------------------------------------------------------------------------
//
//  SetFileDateTime_W()
//
//----------------------------------------------------------------------------
NTSTATUS NTAPI SetFileDateTime_W(HANDLE RootHandle, PCWSTR FilePath, FS_FILE_DATE_TIME *pfdt)
{
    FILE_BASIC_INFORMATION fbi = {0};
    fbi.LastWriteTime = pfdt->LastWriteTime;
    fbi.CreationTime = pfdt->CreationTime;
    fbi.LastAccessTime = pfdt->LastAccessTime;
    fbi.ChangeTime = pfdt->ChangeTime;
    return SetFileDateTime( RootHandle, FilePath, &fbi );
}

//----------------------------------------------------------------------------
//
//  GetFileNameInformation_U()
//
//----------------------------------------------------------------------------
NTSTATUS GetFileNameInformation_U(HANDLE hFile,UNICODE_STRING *pusFileName)
{
    NTSTATUS Status;
    ULONG cbNameBuffer = sizeof(FILE_NAME_INFORMATION)+(sizeof(WCHAR)*UNICODE_STRING_MAX_CHARS);

    FILE_NAME_INFORMATION *pName = (FILE_NAME_INFORMATION *)AllocMemory(cbNameBuffer);
    if( pName == NULL )
    {
        return STATUS_NO_MEMORY;
    }

    IO_STATUS_BLOCK IoStatus;
    Status = NtQueryInformationFile(hFile,&IoStatus,pName,cbNameBuffer,FileNameInformation);
    if( Status == STATUS_SUCCESS )
    {
        AllocateUnicodeStringCbBuffer(pusFileName,pName->FileNameLength+sizeof(WCHAR));
        memcpy(pusFileName->Buffer,pName->FileName,pName->FileNameLength);
        pusFileName->Length = (USHORT)pName->FileNameLength;
        pusFileName->MaximumLength = (USHORT)pusFileName->Length + sizeof(WCHAR);
    }
    else
    {
        _SetLastStatusDos(Status);
    }
    return Status;
}

//----------------------------------------------------------------------------
//
//  GetFileStandardInformation_U()
//
//----------------------------------------------------------------------------
NTSTATUS GetFileStandardInformation_U( HANDLE RootHandle, UNICODE_STRING *FilePath, FILE_STANDARD_INFORMATION *pfsi )
{
    if( FilePath == NULL || pfsi == NULL )
        return STATUS_INVALID_PARAMETER;

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes(&ObjectAttributes,FilePath,0,RootHandle,NULL);

    HANDLE Handle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus = {0};

    Status = NtOpenFile(&Handle,FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES,
                    &ObjectAttributes,&IoStatus,FILE_SHARE_READ|FILE_SHARE_WRITE,0);

    if( Status == STATUS_SUCCESS )
    {
        FILE_STANDARD_INFORMATION fsi = {0};

        Status = NtQueryInformationFile(Handle,&IoStatus,&fsi,sizeof(fsi),FileStandardInformation);

        if( Status == STATUS_SUCCESS )
        {
            *pfsi = fsi;
        }

        NtClose(Handle);
    }

    return Status;
}

//----------------------------------------------------------------------------
//
//  GetFileStandardInformation()
//
//----------------------------------------------------------------------------
NTSTATUS GetFileStandardInformation( HANDLE RootHandle, PCWSTR FilePath, FILE_STANDARD_INFORMATION *pfsi )
{
    if( FilePath == NULL || pfsi == NULL )
        return STATUS_INVALID_PARAMETER;

    UNICODE_STRING usPath;
    RtlInitUnicodeString(&usPath,FilePath);
    return GetFileStandardInformation_U(RootHandle, &usPath, pfsi);
}

//----------------------------------------------------------------------------
//
//  GetDirectoryFileInformation_U()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
GetDirectoryFileInformation_U(
    HANDLE hDirectory,
    UNICODE_STRING *pusFileName,
    FS_FILE_DIRECTORY_INFORMATION *pInfoBuffer,
    UNICODE_STRING *pusFileNameInfo
    )
{
    FILE_ID_BOTH_DIR_INFORMATION *pBuffer;
    ULONG cbBuffer;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;

    cbBuffer = sizeof(FILE_ID_BOTH_DIR_INFORMATION) + DOS_MAX_COMPONENT_BYTES;
    pBuffer = (FILE_ID_BOTH_DIR_INFORMATION *)AllocMemory(cbBuffer);
    if( pBuffer == NULL )
        return STATUS_NO_MEMORY;

    Status = NtQueryDirectoryFile(hDirectory,NULL,NULL,NULL,&IoStatus,
                    pBuffer,cbBuffer,
                    FileIdBothDirectoryInformation,
                    TRUE,
                    pusFileName,
                    TRUE);

    if( Status == STATUS_SUCCESS && pInfoBuffer )
    {
        pInfoBuffer->EaSize          = sizeof(FS_FILE_DIRECTORY_INFORMATION);
        pInfoBuffer->CreationTime    = pBuffer->CreationTime;
        pInfoBuffer->LastAccessTime  = pBuffer->LastAccessTime;
        pInfoBuffer->LastWriteTime   = pBuffer->LastWriteTime;
        pInfoBuffer->ChangeTime      = pBuffer->ChangeTime;
        pInfoBuffer->EndOfFile       = pBuffer->EndOfFile;
        pInfoBuffer->AllocationSize  = pBuffer->AllocationSize;
        pInfoBuffer->FileAttributes  = pBuffer->FileAttributes;
        pInfoBuffer->EaSize          = pBuffer->EaSize;
        pInfoBuffer->ShortNameLength = pBuffer->ShortNameLength;
        pInfoBuffer->FileId          = pBuffer->FileId;
        memcpy(pInfoBuffer->ShortName,pBuffer->ShortName,pBuffer->ShortNameLength);
        pInfoBuffer->ShortName[WCHAR_LENGTH(pBuffer->ShortNameLength)] = UNICODE_NULL;

        if( pusFileNameInfo )
        {
            Status = AllocateUnicodeStringCb(pusFileNameInfo,pBuffer->FileName,pBuffer->FileNameLength,TRUE);
        }
    }

    FreeMemory(pBuffer);

    return Status;
}

//----------------------------------------------------------------------------
//
//  GetDirectoryFileInformation()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
GetDirectoryFileInformation(
    HANDLE hDirectory,
    PCWSTR pszFileName,
    FS_FILE_DIRECTORY_INFORMATION *pInfoBuffer
    )
{
    UNICODE_STRING usFileName;

    RtlInitUnicodeString(&usFileName,pszFileName);

    return GetDirectoryFileInformation_U(hDirectory,&usFileName,pInfoBuffer,NULL);
}

//----------------------------------------------------------------------------
//
//  MakeSureDirectoryPathExists_W()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
MakeSureDirectoryPathExists_W(
    PCWSTR DirPath
    )
{
    UNICODE_STRING RootDirectory;
    UNICODE_STRING RootRelativePath;
    NTSTATUS Status;
    PWSTR pszFullPath;

    pszFullPath = DuplicateString(DirPath);

    if( pszFullPath == NULL )
    {
        return STATUS_NO_MEMORY;
    }

    if( !SplitRootRelativePath(pszFullPath,&RootDirectory,&RootRelativePath) )
    {
        FreeMemory(pszFullPath);
        return STATUS_INVALID_PARAMETER;
    }

    HANDLE hParentDir;
    Status = OpenFile_U(&hParentDir,NULL,&RootDirectory,FILE_GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_DIRECTORY_FILE);

    if( Status == STATUS_SUCCESS )
    {
        WCHAR seps[] = L"\\";
        WCHAR *token = NULL;
        WCHAR *next_token = NULL;
        HANDLE hCreatedDir;

        token = wcstok_s((PWSTR)RootRelativePath.Buffer,seps,&next_token);

        while( token != NULL )
        {
            UNICODE_STRING token_u;
            RtlInitUnicodeString(&token_u,token);

            if( !PathFileExists_UEx(hParentDir,&token_u,NULL) )
            {
                Status = CreateDirectory_W(hParentDir,token,NULL);
                if( Status != STATUS_SUCCESS && Status != STATUS_OBJECT_NAME_COLLISION )
                    break;
            }

            Status = OpenFile_U(&hCreatedDir,hParentDir,&token_u,
                        FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE);

            if( Status != STATUS_SUCCESS )
                break;

            NtClose(hParentDir);
            hParentDir = hCreatedDir;

            token = wcstok_s(NULL,seps,&next_token);
        }

        NtClose(hParentDir);
    }

    FreeMemory(pszFullPath);

    return Status;
}

//----------------------------------------------------------------------------
//
//  RenameDirectoryEntry_U()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
RenameDirectoryEntry_U(
    HANDLE hExistingDirectory,
    UNICODE_STRING *pusExistingFilePath,
    HANDLE hDestinationDirectory,
    UNICODE_STRING *pusNewFileName,
    BOOLEAN ReplaceIfExists
    )
{
    NTSTATUS Status;
    HANDLE hFile;

    if( pusExistingFilePath == NULL || pusNewFileName == NULL )
    {
        _SetLastStatusDos( STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    ULONG cbMoveBufferLength = sizeof(FILE_RENAME_INFORMATION) + pusNewFileName->MaximumLength;
    FILE_RENAME_INFORMATION *pMoveBuffer = (FILE_RENAME_INFORMATION *)AllocMemory( cbMoveBufferLength );
    if( pMoveBuffer == NULL )
    {
        _SetLastStatusDos( STATUS_NO_MEMORY );
        return STATUS_NO_MEMORY;
    }

    //
    // Target source file handle is must be source directory relative open.
    //
    Status = OpenFile_U(&hFile,hExistingDirectory,
                    pusExistingFilePath,
                    FILE_GENERIC_READ|FILE_GENERIC_WRITE|DELETE,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    FILE_OPEN_FOR_BACKUP_INTENT);

    if( Status == STATUS_SUCCESS )
    {
        IO_STATUS_BLOCK IoStatus;

        //
        // A handle that the target directory.
        //
        // If the file is not being moved to a different directory, or if the FileName member
        // contains the full pathname, this member is NULL. Otherwise, it is a handle 
        // for the root directory under which the file will reside after it is renamed.
        //
        pMoveBuffer->RootDirectory   = hDestinationDirectory;

        //
        //If the RootDirectory member is NULL, and the file is being moved to a different directory,
        // this member specifies the full pathname to be assigned to the file. Otherwise, it specifies
        // only the file name or a relative pathname.
        //
        pMoveBuffer->FileNameLength  = pusNewFileName->Length;
        RtlCopyMemory(pMoveBuffer->FileName,pusNewFileName->Buffer,pusNewFileName->Length);

        //
        // Set to TRUE to specify that if a file with the given name already exists, 
        // it should be replaced with the given file. Set to FALSE if the rename operation 
        // should fail if a file with the given name already exists.
        //
        pMoveBuffer->ReplaceIfExists = ReplaceIfExists;

        Status = NtSetInformationFile(hFile,&IoStatus,pMoveBuffer,cbMoveBufferLength,FileRenameInformation);

        NtClose(hFile);
    }

    FreeMemory( pMoveBuffer );

    _SetLastStatusDos( Status );

    return Status;
}

//----------------------------------------------------------------------------
//
//  RenameDirectoryEntry()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
RenameDirectoryEntry(
    HANDLE hExistingDirectory,
    PCWSTR pszExistingFileName,
    HANDLE hDestinationDirectory,
    PCWSTR pszNewFileName,
    BOOLEAN ReplaceIfExists
    )
{
    UNICODE_STRING usExistingFileName;
    UNICODE_STRING usNewFileName;

    if( hDestinationDirectory == NULL || pszExistingFileName == NULL || pszNewFileName == NULL )
    {
        _SetLastStatusDos( STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    RtlInitUnicodeString(&usExistingFileName,pszExistingFileName);
    RtlInitUnicodeString(&usNewFileName,pszNewFileName);

    return RenameDirectoryEntry_U(hExistingDirectory,&usExistingFileName,
                hDestinationDirectory,&usNewFileName,ReplaceIfExists);
}

//----------------------------------------------------------------------------
//
//  MoveDirectoryEntry()
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
MoveDirectoryEntry(
    PCWSTR pszSourceFilePath,
    PCWSTR pszDestinationFilePath,
    BOOLEAN ReplaceIfExists
    )
{
    LONG Status = 0;
    HANDLE hSrcDir = NULL;
    HANDLE hDstDir = NULL;

    UNICODE_STRING SourceDirectory;
    UNICODE_STRING DestinationDirectory;
    UNICODE_STRING FileNameFrom;
    UNICODE_STRING FileNameTo;

    RtlInitUnicodeString(&SourceDirectory,pszSourceFilePath);
    SplitPathFileName_U(&SourceDirectory,&FileNameFrom);

    RtlInitUnicodeString(&DestinationDirectory,pszDestinationFilePath);
    SplitPathFileName_U(&DestinationDirectory,&FileNameTo);

#ifdef _DEBUG
    UNICODE_STRING us1,us2;
    RtlDuplicateUnicodeString(0x3,&SourceDirectory,&us1);
    RtlDuplicateUnicodeString(0x3,&DestinationDirectory,&us2);
    RtlFreeUnicodeString(&us1);
    RtlFreeUnicodeString(&us2);
#endif

    __try
    {
        Status = OpenFile_U(&hSrcDir,NULL,&SourceDirectory,
                            FILE_GENERIC_READ,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_DIRECTORY_FILE|FILE_OPEN_FOR_BACKUP_INTENT);

        if( Status != STATUS_SUCCESS )
        {
            __leave;
        }

        Status = OpenFile_U(&hDstDir,NULL,&DestinationDirectory,
                            FILE_GENERIC_READ,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_DIRECTORY_FILE|FILE_OPEN_FOR_BACKUP_INTENT);

        if( Status != STATUS_SUCCESS )
        {
            __leave;
        }

        Status = RenameDirectoryEntry(hSrcDir,FileNameFrom.Buffer,hDstDir,FileNameTo.Buffer,ReplaceIfExists);

    }
    __finally
    {
        if( hSrcDir )
            NtClose(hSrcDir);
        if( hDstDir )
            NtClose(hDstDir);
    }

    return Status;
}

//----------------------------------------------------------------------------
//
//  GetShortPath_W()
//
//----------------------------------------------------------------------------
//
//  - This parameter necessary to be NT object namespace path.
//
//  - Uses this function instead of GetShortPathName() Win32 function.
//
EXTERN_C
NTSTATUS
NTAPI
GetShortPath_W(
    PCWSTR pszFullPath,
    PWSTR pszShortPathBuffer,
    ULONG cchShortPathBuffer
    )
{
    UNICODE_STRING usRoot;
    UNICODE_STRING usRootRelativePath;
    PWSTR path,sep,part;
    NTSTATUS Status;

    if( pszFullPath == NULL || pszShortPathBuffer == NULL )
        return STATUS_INVALID_PARAMETER;

    SplitRootRelativePath(pszFullPath,&usRoot,&usRootRelativePath);

    if( usRootRelativePath.Length == 0 )
        return STATUS_INVALID_PARAMETER; // no path part

    //
    // Copy short name root prefix
    //
    if( cchShortPathBuffer < WCHAR_LENGTH(usRoot.Length) )
        return STATUS_BUFFER_TOO_SMALL;

    memcpy(pszShortPathBuffer,usRoot.Buffer,usRoot.Length);
    
    //
    // Open root directory
    //
    HANDLE hRoot;
    Status = OpenFile_U(&hRoot,NULL,&usRoot,
                FILE_LIST_DIRECTORY|SYNCHRONIZE,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

    if( Status != STATUS_SUCCESS )
        return Status;

    path = part = AllocateSzFromUnicodeString( &usRootRelativePath );

    if( path == NULL )
        Status = STATUS_NO_MEMORY;

    while( path != NULL )
    {
        // Find separator. if found, replace to null character to terminate a path.
        sep = wcschr(part,L'\\');
        if( sep )
            *sep = L'\0';
        else
            sep = part;

        UNICODE_STRING usPath;
        UNICODE_STRING usFileName;
    
        RtlInitUnicodeString(&usPath,path);
        SplitPathFileName_U(&usPath,&usFileName);

        HANDLE hDir;
        Status = OpenFile_U(&hDir,hRoot,&usPath,
                        FILE_LIST_DIRECTORY|SYNCHRONIZE,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

        if( Status == STATUS_SUCCESS )
        {
            FS_FILE_DIRECTORY_INFORMATION di = {0};

            Status = GetDirectoryFileInformation_U(hDir,&usFileName,&di,NULL);

            NtClose(hDir);

            if( Status == STATUS_SUCCESS )
            {
                if( di.ShortNameLength > 0 )
                    Status = RtlStringCchCatW(pszShortPathBuffer,cchShortPathBuffer,di.ShortName);
                else
                    Status = RtlStringCchCatW(pszShortPathBuffer,cchShortPathBuffer,usFileName.Buffer);

                if( sep != part )
                    Status = RtlStringCchCatW(pszShortPathBuffer,cchShortPathBuffer,L"\\");

                if( Status != STATUS_SUCCESS )
                    break;
            }
        }
        else
        {
            break;
        }

        if( sep == part )
            break; // last part, loop out.

        *sep = L'\\';

        part = sep + 1; // to the next part.
    }

    FreeMemory(path);

    NtClose(hRoot);

    return Status;
}

//----------------------------------------------------------------------------
//
//  GetLongPath_W()
//
//----------------------------------------------------------------------------
//
//  - This parameter necessary to be NT object namespace path.
//
//  - Uses this function instead of GetLongPathName() Win32 function.
//
EXTERN_C
NTSTATUS
NTAPI
GetLongPath_W(
    PCWSTR pszFullPath,
    PWSTR pszLongPathBuffer,
    ULONG cchLongPathBuffer
    )
{
    UNICODE_STRING usRoot;
    UNICODE_STRING usRootRelativePath;
    PWSTR path,sep,part;
    NTSTATUS Status;

    if( pszFullPath == NULL || pszLongPathBuffer == NULL )
        return STATUS_INVALID_PARAMETER;

    SplitRootRelativePath(pszFullPath,&usRoot,&usRootRelativePath);

    if( usRootRelativePath.Length == 0 )
        return STATUS_INVALID_PARAMETER; // no path part

    //
    // Copy short name root prefix
    //
    if( cchLongPathBuffer < WCHAR_LENGTH(usRoot.Length) )
        return STATUS_BUFFER_TOO_SMALL;

    memcpy(pszLongPathBuffer,usRoot.Buffer,usRoot.Length);
    
    //
    // Open root directory
    //
    HANDLE hRoot;
    Status = OpenFile_U(&hRoot,NULL,&usRoot,
                FILE_LIST_DIRECTORY|SYNCHRONIZE,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

    if( Status != STATUS_SUCCESS )
        return Status;

    path = part = AllocateSzFromUnicodeString( &usRootRelativePath );

    if( path == NULL )
        Status = STATUS_NO_MEMORY;

    while( path != NULL )
    {
        // Find separator. if found, replace to null character to terminate a path.
        sep = wcschr(part,L'\\');
        if( sep )
            *sep = L'\0';
        else
            sep = part;

        UNICODE_STRING usPath;
        UNICODE_STRING usFileName;
    
        RtlInitUnicodeString(&usPath,path);
        SplitPathFileName_U(&usPath,&usFileName);

        HANDLE hDir;
        Status = OpenFile_U(&hDir,hRoot,&usPath,
                        FILE_LIST_DIRECTORY|SYNCHRONIZE,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

        if( Status == STATUS_SUCCESS )
        {
            FS_FILE_DIRECTORY_INFORMATION di = {0};
            UNICODE_STRING usOrgFileName = {0};

            Status = GetDirectoryFileInformation_U(hDir,&usFileName,&di,&usOrgFileName);

            NtClose(hDir);

            if( Status == STATUS_SUCCESS )
            {
                Status = RtlStringCchCatW(pszLongPathBuffer,cchLongPathBuffer,usOrgFileName.Buffer);

                if( sep != part )
                    Status = RtlStringCchCatW(pszLongPathBuffer,cchLongPathBuffer,L"\\");

                if( Status != STATUS_SUCCESS )
                    break;
            }
        }
        else
        {
            break;
        }

        if( sep == part )
            break; // last part, loop out.

        *sep = L'\\';

        part = sep + 1; // to the next part.
    }

    FreeMemory(path);

    NtClose(hRoot);

    return Status;
}

EXTERN_C
NTSTATUS
NTAPI
GetLongPathNameFromHandle(
    HANDLE hFile,
    PWSTR *LongPathName,
    PULONG pcbLongPathName
    )
{
    NTSTATUS Status;

    if( LongPathName == NULL )
        return STATUS_INVALID_PARAMETER;

    *LongPathName = NULL;

    // NOTE:
    //
    // If phi->Name.Buffer is not NULL, store the name in the buffer at that pointer address.
    // However, if the volume name + path exceeds 32767 characters, the name up to the upper
    // limit will be stored in UNICODE_STRING, and 0x80000005 will be returned as the status.
    //
    // If phi->Name.Buffer is NULL, it will be interpreted and returned as a continuous buffer
    // starting in UNCODE_STRING.
    //
    // With this buffer specification, even if the volume name + path exceeds 32,767 characters, 
    // all will be copied and the length (number of bytes) will be returned in cb. 
    // In this case UNICODE_STRING.Length is the length of the 'Volume'.
    //
    // ex)
    // struct {
    //   USHORT Length;
    //   USHORT MaximumLength
    //   PWSTR  Buffer; // point to name[]
    //   WCHAR  name[1];
    // };
    //
    // The pointer to the memory that stored the returned path must be freed using FreeMemroy().
    //
    ULONG cb;
    UNICODE_STRING *Path;
    cb = sizeof(UNICODE_STRING) + _NT_PATH_FULL_LENGTH_BYTES;
    Path = (UNICODE_STRING *)AllocMemory( cb );
    if( Path != NULL )
    {
        Path->Length = 0;
        Path->MaximumLength = 0;
        Path->Buffer = NULL;

//      const int ObjectNameInformation = 1;

        Status = NtQueryObject(hFile,(OBJECT_INFORMATION_CLASS)ObjectNameInformation,Path,cb,&cb);

        if( Status == STATUS_SUCCESS )
        {
            cb -= sizeof(UNICODE_STRING);

            *LongPathName = (PWSTR)AllocMemory( cb ); // including terminate NULL for C string.

            if( *LongPathName )
            {
                memcpy(*LongPathName,Path->Buffer,cb);

                if( pcbLongPathName != NULL )
                    *pcbLongPathName = (cb - sizeof(WCHAR)); // without terminate NULL

                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }
        }

        FreeMemory(Path);
    }
    else
    {
        Status = STATUS_NO_MEMORY;
    }

    return Status;
}



EXTERN_C
NTSTATUS
NTAPI
SetFileBasicInformation(
    HANDLE hFile,
    NT_FILE_BASIC_INFORMATION *pfbi
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus={0};
    Status = NtSetInformationFile(hFile,&IoStatus,pfbi,sizeof(FILE_BASIC_INFORMATION),FileBasicInformation);
    return Status;
}

EXTERN_C
NTSTATUS
NTAPI
QueryAttributesFile(
    HANDLE hRoot,
    PCWSTR pszFileName,
    NT_FILE_BASIC_INFORMATION *FileBasicInfo
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_BASIC_INFORMATION fbi = {0};
    UNICODE_STRING usFileName;

    RtlInitUnicodeString(&usFileName,pszFileName);

    InitializeObjectAttributes(&ObjectAttributes,&usFileName,0,hRoot,NULL);

    Status = NtQueryAttributesFile(&ObjectAttributes,&fbi);
    if( Status == STATUS_SUCCESS )
    {
        if( FileBasicInfo )
            memcpy(FileBasicInfo,&fbi,sizeof(NT_FILE_BASIC_INFORMATION));
    }
    return Status;
}

//----------------------------------------------------------------------------
//
//  GetFileSizeByHandle()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
GetFileSizeByHandle(
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

//----------------------------------------------------------------------------
//
//  GetFileId()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
NTSTATUS
NTAPI
GetFileId(
    HANDLE hFile,
    LARGE_INTEGER *pFildId
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;

    LARGE_INTEGER li;
    Status = NtQueryInformationFile(hFile,&IoStatus,&li,sizeof(li),FileInternalInformation);

    if( Status == STATUS_SUCCESS )
    {
        *pFildId = li;
    }

    return Status;
}
