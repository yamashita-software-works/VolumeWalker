//***************************************************************************
//*                                                                         *
//*  ntdirtraverse.cpp                                                      *
//*                                                                         *
//*  Create: 2021-04-12                                                     *
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
#include <locale.h>
#include <conio.h>
#include <winerror.h>

#include "ntnativeapi.h"
#include "ntnativehelp.h"

#define _PAGESIZE 4096

FORCEINLINE
VOID
_SetUnicodeString(
    OUT UNICODE_STRING *pus,
    IN PWSTR name,
    IN LONG len
    )
{
    pus->Buffer = name;
    pus->Length = pus->MaximumLength = (USHORT)len;
}

//---------------------------------------------------------------------------
// Directory Traverse
//---------------------------------------------------------------------------
typedef enum {
    None = 0,
    DirFileInfo_IdBoth,
    DirFileInfo_TxD,
    DirFileInfo_IdExtdBoth,
} DIRECTORY_INFO_BUFFER_TYPE;

#define _C_STYLE 0

#if _C_STYLE // C-style
typedef NTSTATUS (NTAPI *PFNGETDIRECTORYFILEINFO)(
    PVOID pBuffer,
    UNICODE_STRING *pFileName,
    ULONG *pFileAttributes
    );

NTSTATUS NTAPI IdBothDirName(PVOID ptr,UNICODE_STRING *pFileName,ULONG *pFileAttributes)
{
    pFileName->Buffer = ((FILE_ID_BOTH_DIR_INFORMATION *)ptr)->FileName;
    pFileName->Length = (USHORT)((FILE_ID_BOTH_DIR_INFORMATION *)ptr)->FileNameLength;
    pFileName->MaximumLength = pFileName->Length;
    *pFileAttributes = ((FILE_ID_BOTH_DIR_INFORMATION *)ptr)->FileAttributes;
    return 0;
}

NTSTATUS NTAPI IdBothDirName(PVOID ptr,UNICODE_STRING *pFileName,ULONG *pFileAttributes)
{
    pFileName->Buffer = ((FILE_ID_GLOBAL_TX_DIR_INFORMATION *)ptr)->FileName;
    pFileName->Length = (USHORT)((FILE_ID_GLOBAL_TX_DIR_INFORMATION *)ptr)->FileNameLength;
    pFileName->MaximumLength = pFileName->Length;
    *pFileAttributes = ((FILE_ID_GLOBAL_TX_DIR_INFORMATION *)ptr)->FileAttributes;
    return 0;
}
#else
class QueryDirectoryFileName
{
public:
    virtual PWCH GetName(void *) = 0; //{ return NULL; }
    virtual USHORT GetNameLength(void *) = 0; //{ return 0; }
    virtual ULONG GetFileAttributes(void *) = 0; // { return 0; }
};

template< class T >
class QueryDirectoryFileNameImpl : public QueryDirectoryFileName
{
public:
    virtual PWCH GetName(void *ptr) { return ((T *)ptr)->FileName; }
    virtual USHORT GetNameLength(void *ptr) { return (USHORT)((T *)ptr)->FileNameLength; }
    virtual ULONG GetFileAttributes(void *ptr) { return ((T *)ptr)->FileAttributes; }
};
#endif

typedef struct _TRAVERSE_DIRECTORY_PARAM
{
    WCHAR *Path;
    // Recursive call
    //
    BOOLEAN bRecursive;
    int RecursiveCallLevel;
    // User Callback
    //
    FINDFILECALLBACK pfnCallback;
    ULONG_PTR CallbackContext;

    UNICODE_STRING FileName;

    ULONG Flags;

    // Information Buffer
    //
    DIRECTORY_INFO_BUFFER_TYPE InfoType;
    FILE_INFORMATION_CLASS FileInformationClass;
    ULONG FileInformationSize;
#if _C_STYLE
    PFNGETDIRECTORYFILEINFO pfnGetFileInfo;
#else
    QueryDirectoryFileName *pcGetFileInfo;
#endif

} TRAVERSE_DIRECTORY_PARAM;

#define DTF_NO_PROCESS_WILDCARD 0x1

class CTraverseDirectoryParam : public TRAVERSE_DIRECTORY_PARAM
{
    PWSTR m_pRootPoint;
    PWSTR m_pConcatenatePoint;
    ULONG m_cbCurDirNameLength;

public:
    CTraverseDirectoryParam(DIRECTORY_INFO_BUFFER_TYPE Type=None)
    {
        Path = NULL;
        RecursiveCallLevel = 0;
        m_pRootPoint = NULL;
        m_pConcatenatePoint = NULL;
        m_cbCurDirNameLength = 0;
        InfoType = Type;
    }

    ~CTraverseDirectoryParam()
    {
        FreeMemory(Path);
    }

    NTSTATUS InitTraverseRoot(PCWSTR pszPath,ULONG cbLength)
    {
        ULONG cb = 32768 * sizeof(WCHAR);
        Path = (WCHAR *)AllocMemory( cb );
        if( Path == NULL )
            return STATUS_NO_MEMORY;
#ifdef _DEBUG
        memset(Path,0xcc,cb);
#else
        memset(Path,0,cb);
#endif
        int cch = cbLength / sizeof(WCHAR);
        RtlCopyMemory(Path,pszPath,cbLength);

        if( *(Path + (cch - 1)) != L'\\' ) // append backslash
        {
            *(Path + cch) = L'\\';
            cch++;
        }

        m_pConcatenatePoint = Path + cch;
        *m_pConcatenatePoint = L'\0';

        // Target is specified directory as current directory .
        //
        m_pRootPoint = m_pConcatenatePoint;

        return STATUS_SUCCESS;
    }

    VOID PushDirectory(PWSTR FileName,ULONG FileNameLength)
    {
        RtlCopyMemory(m_pConcatenatePoint,FileName,FileNameLength);

        m_pConcatenatePoint += (FileNameLength / sizeof(WCHAR));
        *m_pConcatenatePoint++ = L'\\';
        *m_pConcatenatePoint = L'\0';

        // hold current name length
        m_cbCurDirNameLength = FileNameLength;

        RecursiveCallLevel++;
    }

    VOID PopDirectory(ULONG FileNameLength)
    {
        m_pConcatenatePoint -= ((FileNameLength / sizeof(WCHAR)) + 1);
        *m_pConcatenatePoint = L'\0';
        RecursiveCallLevel--;
    }

    PCWSTR RefRelativeRootPtr() const
    {
        return m_pRootPoint; 
    }

    PCWSTR GetFullPath() const
    {
        return Path;
    }

    ULONG GetFullPathLength() const
    {
        return (ULONG)(wcslen(Path) * sizeof(WCHAR));
    }

    PCWSTR GetCurrentDirectoryName() const
    {
        ULONG cb = GetFullPathLength();
        int cch = (int)((cb - m_cbCurDirNameLength)/sizeof(WCHAR)) - 1;
        return &Path[cch];
    }

    int GetRecursiveCallLevel() const 
    {
        return RecursiveCallLevel;
    }
};

static NTSTATUS callbackStartDirectory(UNICODE_STRING *pusFileName,CTraverseDirectoryParam *pTDP)
{
    if( pTDP->pfnCallback )
    {
        if( pTDP->FileName.Buffer != NULL && !(pTDP->Flags & DTF_NO_PROCESS_WILDCARD) )
        {
            NTSTATUS Status = STATUS_SUCCESS;
            PWSTR pszPattern = AllocateSzFromUnicodeString(&pTDP->FileName);
            if( _UStrMatchI_U(pszPattern,pusFileName) )
            {
                Status = pTDP->pfnCallback(FFCBR_DIRECTORYSTART,pTDP->GetFullPath(),pTDP->RefRelativeRootPtr(),pusFileName,0,0,NULL,pTDP->CallbackContext);
            }
            FreeMemory(pszPattern);
            return Status;
        }
        return pTDP->pfnCallback(FFCBR_DIRECTORYSTART,pTDP->GetFullPath(),pTDP->RefRelativeRootPtr(),pusFileName,0,0,NULL,pTDP->CallbackContext);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS callbackEndDirectory(UNICODE_STRING* pusFileName,CTraverseDirectoryParam *pTDP)
{
    if( pTDP->pfnCallback )
    {
        if( pTDP->FileName.Buffer != NULL && !(pTDP->Flags & DTF_NO_PROCESS_WILDCARD) )
        {
            NTSTATUS Status = STATUS_SUCCESS;
            PWSTR pszPattern = AllocateSzFromUnicodeString(&pTDP->FileName);
            if( _UStrMatchI_U(pszPattern,pusFileName) )
            {
                Status = pTDP->pfnCallback(FFCBR_DIRECTORYEND,pTDP->GetFullPath(),pTDP->RefRelativeRootPtr(),pusFileName,0,0,NULL,pTDP->CallbackContext);
            }
            FreeMemory(pszPattern);
            return Status;
        }
        return pTDP->pfnCallback(FFCBR_DIRECTORYEND,pTDP->GetFullPath(),pTDP->RefRelativeRootPtr(),pusFileName,0,0,NULL,pTDP->CallbackContext);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS callbackFile(UNICODE_STRING& usFileName,INT InfoType,PVOID pInfoBuffer,CTraverseDirectoryParam *pTDP)
{
    if( pTDP->pfnCallback )
    {
        if( pTDP->FileName.Buffer != NULL && !(pTDP->Flags & DTF_NO_PROCESS_WILDCARD) )
        {
            NTSTATUS Status = STATUS_SUCCESS;
            PWSTR pszPattern = AllocateSzFromUnicodeString(&pTDP->FileName);
            if( _UStrMatchI_U(pszPattern,&usFileName) )
            {
                Status = pTDP->pfnCallback(FFCBR_FINDFILE,pTDP->GetFullPath(),pTDP->RefRelativeRootPtr(),&usFileName,0,InfoType,pInfoBuffer,pTDP->CallbackContext);
            }
            FreeMemory(pszPattern);
            return Status;
        }

        return pTDP->pfnCallback(FFCBR_FINDFILE,pTDP->GetFullPath(),pTDP->RefRelativeRootPtr(),&usFileName,0,InfoType,pInfoBuffer,pTDP->CallbackContext);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS callbackError(UNICODE_STRING *FileName,CTraverseDirectoryParam *pTDP,NTSTATUS Status)
{
    if( pTDP->pfnCallback )
    {
        return pTDP->pfnCallback(FFCBR_ERROR,pTDP->GetFullPath(),pTDP->RefRelativeRootPtr(),FileName,Status,0,NULL,pTDP->CallbackContext);
    }
    return STATUS_SUCCESS;
}

static
NTSTATUS
_QueryDirectoryFile(
    HANDLE hDirectory,
    FILE_INFORMATION_CLASS FileInfoClass,
    PVOID pBuffer,
    ULONG cbBuffer,
    BOOLEAN bRestartScan
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;

    Status = NtQueryDirectoryFile(hDirectory,
                                NULL,NULL,NULL,
                                &IoStatus,
                                pBuffer,cbBuffer,
                                FileInfoClass,
                                FALSE,
                                NULL,
                                bRestartScan
                                );
    return Status;
}

static
NTSTATUS
_TraverseDirectoryImpl(
    HANDLE hParent,
    UNICODE_STRING *NtPath,
    CTraverseDirectoryParam *pTDP
    )
{
    HANDLE hDirectory;
    NTSTATUS Status;
    BOOLEAN bRestartScan = TRUE;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus = {0};

    if( (Status = callbackStartDirectory(NtPath,pTDP)) != STATUS_SUCCESS )
        return Status;

    // open directory
    //
    InitializeObjectAttributes(&ObjectAttributes,NtPath,0,hParent,NULL);

    Status = NtOpenFile(&hDirectory,
                FILE_LIST_DIRECTORY|FILE_TRAVERSE|SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT
                // FILE_OPEN_REPARSE_POINT bypass reparse point processing for the file. 
                );

    if( Status == STATUS_SUCCESS )
    {
        PVOID pBuffer = NULL;
        ULONG cbBuffer = _PAGESIZE * 8;
        UNICODE_STRING FileName;
        ULONG FileAttributes;

        pBuffer = AllocMemory( cbBuffer );

        if( pBuffer != NULL )
        {
            while( Status == STATUS_SUCCESS )
            {
                Status = _QueryDirectoryFile(hDirectory,pTDP->FileInformationClass,pBuffer,cbBuffer,bRestartScan);
                if( Status != STATUS_SUCCESS )
                {
                    if( Status == STATUS_NO_MORE_FILES )
                        Status = STATUS_SUCCESS; // replace state
                    break;
                }
    
                PVOID ptr = pBuffer;

                while( Status == STATUS_SUCCESS )
                {
#if _C_STYLE
                    pTDP->pfnGetFileInfo(ptr,&FileName,&FileAttributes);
#else
                    FileName.Buffer = pTDP->pcGetFileInfo->GetName(ptr);
                    FileName.Length = FileName.MaximumLength = pTDP->pcGetFileInfo->GetNameLength(ptr);
                    FileAttributes  = pTDP->pcGetFileInfo->GetFileAttributes(ptr);
#endif
                    if( !IS_RELATIVE_DIR_NAME_WITH_UNICODE_SIZE( FileName.Buffer, FileName.Length ) )
                    {
#if 0
                        if( pTDP->bRecursive && (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                        {
                            pTDP->PushDirectory(FileName.Buffer,FileName.Length);

                            Status = _TraverseDirectoryImpl(hDirectory,&FileName,pTDP);

                            pTDP->PopDirectory(FileName.Length);
                        }
                        else
                        {
                            Status = callbackFile(FileName,pTDP->InfoType,ptr,pTDP);
                        }
#else
                        //
                        // Callback the Information both Directory and File.
                        //
                        Status = callbackFile(FileName,pTDP->InfoType,ptr,pTDP);

                        if( pTDP->bRecursive && (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                        {
                            pTDP->PushDirectory(FileName.Buffer,FileName.Length);

                            Status = _TraverseDirectoryImpl(hDirectory,&FileName,pTDP);

                            pTDP->PopDirectory(FileName.Length);
                        }
#endif
                    }

                    if( *((ULONG *)ptr) == 0 ) // (p->NextEntryOffset == 0 )
                    {
                        break;
                    }

                    ((ULONG_PTR&)ptr) += *((ULONG *)ptr); // p += p->NextEntryOffset;
                }

                // NOTE: The RestartScan parameter is currently ignored.
                //
                bRestartScan = FALSE;
            }

            FreeMemory(pBuffer);
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }

        NtClose(hDirectory);
    }
    else
    {
        callbackError(NtPath,pTDP,Status); // todo:
    }

    Status = callbackEndDirectory(NtPath,pTDP);

    return Status;
}

//---------------------------------------------------------------------------
//
//  QueryDirectoryEntryInformation()
//
//---------------------------------------------------------------------------
NTSTATUS
QueryDirectoryEntryInformation(
    HANDLE hRoot,
    UNICODE_STRING* pusPath,
    FILE_INFORMATION_CLASS InfoClass,
    ULONG cbInfoClassSize,
    PVOID *DirectoryEntry
    )
{
    HANDLE hDirectory;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;

    InitializeObjectAttributes(&ObjectAttributes,pusPath,0,hRoot,NULL);

    Status = NtOpenFile(&hDirectory,
                FILE_LIST_DIRECTORY|FILE_TRAVERSE|SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT
                // FILE_OPEN_REPARSE_POINT bypass reparse point processing for the file. 
                );

    if( Status == STATUS_SUCCESS )
    {
        PVOID pBuffer = NULL;
        ULONG cbBuffer = cbInfoClassSize  + (sizeof(WCHAR) * WIN32_MAX_PATH);

        pBuffer = AllocMemory( cbBuffer );

        if( pBuffer != NULL )
        {
            Status = NtQueryDirectoryFile(hDirectory,
                                NULL,NULL,NULL,
                                &IoStatus,
                                pBuffer,cbBuffer,
                                InfoClass,
                                FALSE,
                                NULL,
                                TRUE
                                );

            if( Status == STATUS_SUCCESS )
            {
                *DirectoryEntry = ReAllocateHeap(pBuffer,IoStatus.Information); // shrink buffer
            }
            else
            {
                *DirectoryEntry = NULL;
            }
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }

        NtClose(hDirectory);
    }

    return Status;
}

//---------------------------------------------------------------------------
//
//  TraverseDirectory()
//
//---------------------------------------------------------------------------
NTSTATUS
TraverseDirectory(
    UNICODE_STRING& DirectoryFullPath,
    UNICODE_STRING& FileName,
    BOOLEAN bRecursive,
    ULONG Flags,
    FINDFILECALLBACK pfnCallback,
    ULONG_PTR CallbackContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    CTraverseDirectoryParam *pTDP = new CTraverseDirectoryParam;
    if( pTDP == NULL )
        return STATUS_NO_MEMORY;

    pTDP->pfnCallback = pfnCallback;
    pTDP->CallbackContext = CallbackContext;
    pTDP->bRecursive = bRecursive;
    pTDP->FileName = FileName;
    pTDP->Flags = Flags;

    pTDP->InfoType = DirFileInfo_IdBoth;
    pTDP->FileInformationClass = FileIdBothDirectoryInformation;
    pTDP->FileInformationSize = sizeof(FILE_ID_BOTH_DIR_INFORMATION);
#if _C_STYLE
    pTDP->pfnGetFileInfo = &IdBothDirName;
#else
    pTDP->pcGetFileInfo = new QueryDirectoryFileNameImpl<FILE_ID_BOTH_DIR_INFORMATION>;
#endif
    if( pTDP->InitTraverseRoot(DirectoryFullPath.Buffer,DirectoryFullPath.Length) == STATUS_SUCCESS )
    {
        FILE_ID_BOTH_DIR_INFORMATION *pd;

        if( NT_SUCCESS(QueryDirectoryEntryInformation(NULL,&DirectoryFullPath,pTDP->FileInformationClass,pTDP->FileInformationSize,(PVOID*)&pd)) )
        {
            Status = _TraverseDirectoryImpl(NULL,&DirectoryFullPath,pTDP);

            FreeMemory(pd);
        }
    }
    else
    {
        Status = STATUS_NO_MEMORY;
    }

#if !_C_STYLE
    delete pTDP->pcGetFileInfo;
#endif

    delete pTDP;

    return Status;
}
