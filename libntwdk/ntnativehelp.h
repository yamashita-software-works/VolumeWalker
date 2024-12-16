#pragma once
//***************************************************************************
//*                                                                         *
//*  ntnativehelp.h                                                         *
//*                                                                         *
//*  ntddk/ntifs helper function library                                    *
//*                                                                         *
//*  Create: 2021-04-12                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#ifndef NTAPI
#define NTAPI __stdcall
#endif

#ifndef STDCALL
#define STDCALL __stdcall
#endif

#ifndef CALLBACK
#define CALLBACK __stdcall
#endif

#ifndef _USE_INTERNAL_MEMORY_DEBUG
#define _USE_INTERNAL_MEMORY_DEBUG  0
#endif

#if _USE_INTERNAL_MEMORY_DEBUG
#include "..\libcwh\mem.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if !(_USE_INTERNAL_MEMORY_DEBUG)
PVOID NTAPI AllocMemory(SIZE_T cb);
PVOID NTAPI ReAllocateHeap(PVOID pv,SIZE_T cb);
#define ReallocMemory(pv,cb) ReAllocateHeap(pv,cb)
VOID NTAPI FreeMemory(PVOID ptr);
WCHAR* NTAPI AllocStringBuffer(SIZE_T cch);
WCHAR* NTAPI AllocStringBufferCb(SIZE_T cb);
PWSTR NTAPI DuplicateString(PCWSTR psz);
#endif
PWSTR NTAPI AllocateSzFromUnicodeString(__in UNICODE_STRING *pus);
PWSTR NTAPI AllocStringLengthCb(PCWSTR psz,SIZE_T cb);

NTSTATUS NTAPI AllocateUnicodeString(__out UNICODE_STRING *pus,__in PCWSTR psz);
NTSTATUS NTAPI DuplicateUnicodeString(__out UNICODE_STRING *pusDup,__in UNICODE_STRING *pusSrc);
NTSTATUS NTAPI AllocateUnicodeStringCbBuffer(UNICODE_STRING *pus,ULONG cb);
NTSTATUS NTAPI AllocateUnicodeStringCchBuffer(UNICODE_STRING *pus,ULONG cch);
NTSTATUS NTAPI AllocateUnicodeStringCb(UNICODE_STRING *pus,PCWSTR String,ULONG cb,BOOLEAN NullTerminate);
NTSTATUS CombineUnicodeStringPath(UNICODE_STRING *CombinedPath,UNICODE_STRING *Path,UNICODE_STRING *FileName);
NTSTATUS NTAPI FreeUnicodeString(UNICODE_STRING *pus);

LONG NTAPI CompareUnicodeString(__in PUNICODE_STRING  String1,__in PUNICODE_STRING  String2,__in BOOLEAN  CaseInSensitive);

NTSTATUS FindRootDirectory_U(__in UNICODE_STRING *pusFullyQualifiedPath,__out PWSTR *pRootDirectory);
NTSTATUS FindRootDirectory_W(__in PCWSTR pszFullyQualifiedPath,__out PWSTR *pRootDirectory);
NTSTATUS GetFileNamePart_U(__in UNICODE_STRING *FilePath,__out UNICODE_STRING *FileName);
NTSTATUS SplitPathFileName_U(__inout UNICODE_STRING *Path,__out UNICODE_STRING *FileName);
NTSTATUS SplitPathFileName_W(PCWSTR pszPath,UNICODE_STRING *Path,UNICODE_STRING *FileName);
NTSTATUS GetFileNameInformation_U(HANDLE hFile,UNICODE_STRING *pusFileName);

BOOLEAN SplitRootRelativePath_U(UNICODE_STRING *pusFullPath,UNICODE_STRING *RootDirectory,UNICODE_STRING *RootRelativePath);
BOOLEAN SplitRootRelativePath(__in PCWSTR pszFullPath,__out UNICODE_STRING *RootDirectory,__out UNICODE_STRING *RootRelativePath);
BOOLEAN SplitVolumeRelativePath(__in PCWSTR pszFullPath,__out UNICODE_STRING *VolumeName,__out UNICODE_STRING *VolumeRelativePath);
BOOLEAN SplitVolumeRelativePath_U(__in UNICODE_STRING *FullPath,__out UNICODE_STRING *VolumeName,__out UNICODE_STRING *VolumeRelativePath);
BOOLEAN GetRootDirectory_U(__inout UNICODE_STRING *pusFullyQualifiedPath);
BOOLEAN GetVolumeName_U(__inout UNICODE_STRING *pusFullyQualifiedPath);

BOOLEAN PathFileExists_U(UNICODE_STRING *pusPath,ULONG *FileAttributes);
BOOLEAN PathFileExists_UEx(HANDLE hParentDir,UNICODE_STRING *pusPath,ULONG *FileAttributes);
BOOLEAN PathFileExists_W(PCWSTR pszPath,ULONG *FileAttributes);

BOOLEAN IsNtDevicePath(PCWSTR pszPath);
BOOLEAN IsStringVolumeGuid(PCWSTR pszString);
BOOLEAN IsRelativePath(PCWSTR pszPath);
BOOLEAN IsDirectory(PCWSTR pszPath);
BOOLEAN IsDirectory_U(UNICODE_STRING *pusPath);
BOOLEAN IsRootDirectory_U(UNICODE_STRING *pusFullyPath);
BOOLEAN IsRootDirectory_W(__in PCWSTR pszFullyQualifiedPath);
BOOLEAN IsLastCharacterBackslash(PCWSTR pszPath);
BOOLEAN IsLastCharacterBackslash_U(UNICODE_STRING *pusPath);
BOOLEAN HasPrefix(PCWSTR pszPrefix,PCWSTR pszPath);
BOOLEAN HasPrefix_U(PCWSTR pszPrefix,UNICODE_STRING *String);
BOOLEAN HasWildCardChar_U(UNICODE_STRING *String);

VOID RemoveBackslash(PWSTR pszPath);
VOID RemoveBackslash_U(UNICODE_STRING *pusPath);
VOID RemoveFileSpec(PWSTR pszPath);
VOID RemoveFileSpec_W(PWSTR pszPath);
BOOLEAN RemovePrefix(PCWSTR pszPrefix,PCWSTR Path,PWSTR PathBuffer,ULONG cchPathBuffer);

BOOLEAN AppendBackslash_W(PWSTR pszPath,int cchPath);

PCWSTR FindFileName_W(PCWSTR pszPath);

PWSTR CombinePath(PCWSTR pszPath,PCWSTR pszFileName);
PWSTR CombinePath_U(PCWSTR pszPath,UNICODE_STRING *pusFileName);
PWSTR CombinePathBuffer(PWSTR lpszDest,int cchDest,PCWSTR lpszDir,PCWSTR lpszFile);

PWSTR DosPathNameToNtPathName(PCWSTR pszDosPath);
PWSTR DosPathNameToNtPathName_W(PCWSTR pszDosPath);

NTSTATUS GetFileAttributes_U(HANDLE RootHandle, UNICODE_STRING *FilePath, ULONG *pulFileAttributes);
NTSTATUS GetFileAttributes_W(HANDLE RootHandle, PCWSTR FilePath,  ULONG *pulFileAttributes);
NTSTATUS SetFileAttributes_W(HANDLE RootHandle, PCWSTR FilePath,  ULONG ulFileAttributes);
#ifdef _NTIFS_
NTSTATUS GetFileDateTime_U(HANDLE RootHandle, UNICODE_STRING *FilePath, FILE_BASIC_INFORMATION *pbi);
NTSTATUS GetFileDateTime(HANDLE RootHandle, PCWSTR FilePath, FILE_BASIC_INFORMATION *pfbi);
NTSTATUS SetFileDateTime_U( HANDLE RootHandle, UNICODE_STRING *FilePath, FILE_BASIC_INFORMATION *pbi );
NTSTATUS SetFileDateTime( HANDLE RootHandle, PCWSTR FilePath, FILE_BASIC_INFORMATION *pfbi );
#endif
VOID NTAPI _SetLastStatusDos(NTSTATUS ntStatus);
VOID NTAPI _SetLastWin32Error(ULONG Win32ErrorCode);
VOID NTAPI _SetLastNtStatus(NTSTATUS ntStatus);

ULONG NTAPI SetLastErrorNtStatusToWin32(NTSTATUS ntStatus);
ULONG NTAPI NtStatusToDosError(NTSTATUS ntStatus);

BOOLEAN _UStrMatchI(const WCHAR *ptn,const WCHAR *str,const WCHAR *end);
BOOLEAN _UStrMatch(const WCHAR *ptn,const WCHAR *str,const WCHAR *end);
BOOLEAN _UStrMatch_UStr(const WCHAR *ptn,const WCHAR *str,const WCHAR *end);
BOOLEAN _UStrMatchI_UStr(const WCHAR *ptn,const WCHAR *str,const WCHAR *end);
BOOLEAN _UStrMatch_U(const WCHAR *ptn,const UNICODE_STRING *pus);
BOOLEAN _UStrMatchI_U(const WCHAR *ptn,const UNICODE_STRING *pus);

HRESULT GetNtPath(PCWSTR DosPathName,PWSTR *NtPath,PCWSTR *NtFileNamePart);
HRESULT GetNtPath_U(PCWSTR DosPathName,UNICODE_STRING *NtPath,PCWSTR *NtFileNamePart);

HANDLE SPtrArray_Create(INT InitialCount);
INT SPtrArray_Destroy(HANDLE hspa);
INT SPtrArray_GetCount(HANDLE hspa);
INT SPtrArray_Add(HANDLE hspa,PVOID Insert);
PVOID SPtrArray_Get(HANDLE hspa,int iIndex);
INT SPtrArray_Delete(HANDLE hspa,int iIndex);
BOOLEAN SPtrArray_DeleteAll(HANDLE hspa);
BOOLEAN SPtrArray_Reset(HANDLE hspa);
SIZE_T SPtrArray_GetBufferSize(HANDLE hspa);

#define SPtrArray_GetPWSTR(h,i) ((PWSTR)SPtrArray_Get(h,i))
#define SPtrArray_GetPtr(h,i) SPtrArray_Get(h,i)

typedef
NTSTATUS
(CALLBACK *FINDFILECALLBACK)(
    ULONG CallbackReason,
    PCWSTR Path,
    PCWSTR RelativePath,
    UNICODE_STRING *FileName,
    NTSTATUS Status,
    ULONG FileInfoType,  // Reserved always zero
    PVOID FileInfo,      // FILE_ID_BOTH_DIR_INFORMATION
    ULONG_PTR CallbackContext
    );

// Callback reason
#define FFCBR_FINDFILE         0
#define FFCBR_DIRECTORYSTART   1
#define FFCBR_DIRECTORYEND     2
#define FFCBR_ERROR            3

NTSTATUS
TraverseDirectory(
    UNICODE_STRING& DirectoryFullPath,
    UNICODE_STRING& FileName,
    BOOLEAN bRecursive,
    ULONG Flags,
    FINDFILECALLBACK pfnCallback,
    ULONG_PTR CallbackContext
    );
#define DTF_NO_PROCESS_WILDCARD 0x1

#define FACILITY_LIBWDK 0x80
#define NTSTATUS_FROM_LIBWDK(x) ((NTSTATUS)(x) <= 0 ? ((NTSTATUS)(x)) : ((NTSTATUS) (((x) & 0x0000FFFF) | (FACILITY_LIBWDK << 16) | ERROR_SEVERITY_SUCCESS)))

#define STATUS_TD_SKIP                           NTSTATUS_FROM_LIBWDK(2)
#define STATUS_TD_DIRECTORY_HIERARCHY_TOO_DEEP   NTSTATUS_FROM_LIBWDK(3)

typedef
BOOLEAN
(CALLBACK *ENUMFILESCALLBACK)(
    HANDLE hDirectory,
    PCWSTR DirectoryName,
	ULONG Flags,
    PVOID pFileInfo,
    ULONG_PTR CallbackContext
    );

EXTERN_C
NTSTATUS
NTAPI
EnumFiles(
    HANDLE hRoot,
    PCWSTR pszDirectoryPath,
    PCWSTR pszFileName,
	ULONG Flags,
    ENUMFILESCALLBACK pfnCallback,
    ULONG_PTR CallbackContext
    );

typedef struct _FSDIRENUMCALLBACKINFO
{
    HANDLE DirectoryHandle;
    PCWSTR Path;
} FSDIRENUMCALLBACKINFO,*PFSDIRENUMCALLBACKINFO;

typedef HRESULT (CALLBACK *FSDIRENUMCALLBACKPROC)(
    ULONG InformationType,
    PVOID Information,
    PFSDIRENUMCALLBACKINFO DirEnumCallbackInfo,
    PVOID Context
    );

EXTERN_C
HRESULT
NTAPI
EnumDirectoryFiles_W(
    PCWSTR Path,
    PCWSTR FileNameFilter,
    ULONG Flags,
    FSDIRENUMCALLBACKPROC Callback,
    PVOID Context
    );

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
    );

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
    );

#ifdef _NTIFS_
#define OpenFile OpenFile_W
#endif

EXTERN_C
NTSTATUS
NTAPI
OpenFileEx_W(
    PHANDLE phFile,
    PCWSTR PathName,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    ULONG OpenOptions
    );

#include "ntfileid.h"

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
    );

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
    );

#ifndef BOOL
#define BOOL INT
#endif

typedef unsigned long DWORD;

#ifndef _WINBASE_
typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    PVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
#endif

EXTERN_C
LONG
NTAPI
CreateDirectory_W(
    HANDLE hRoot,
    LPCWSTR NewDirectory,
    SECURITY_ATTRIBUTES *SecurityAttributes
    );

#ifdef _NTIFS_
#define CreateDirectory CreateDirectory_W
#endif

EXTERN_C
NTSTATUS
NTAPI
StringFromGUID(
    __in  const GUID *Guid,
    __out LPWSTR lpszGuid,
    __in  int cchMax
    );

EXTERN_C
NTSTATUS
NTAPI
GUIDFromString(
    __in LPCWSTR lpszGuid,
    __out GUID* Guid
    );

EXTERN_C
BOOL
NTAPI
StringToIntegerW(
    IN PCWSTR String,
    IN ULONG  Base  OPTIONAL,
    OUT PULONG  Value
    );

EXTERN_C
INT
NTAPI
IntegerToStringCchW(
    IN ULONG Value,
    IN ULONG Base  OPTIONAL,
    IN OUT PWSTR StringBuffer,
    IN ULONG StringBufferLength
    );

EXTERN_C
INT
NTAPI
IntegerToStringCbW(
    IN ULONG Value,
    IN ULONG Base  OPTIONAL,
    IN OUT PWSTR StringBuffer,
    IN ULONG StringBufferBytes
    );

#define LPWSTR_GLOBALROOTPREFIX      L"\\??\\GlobalRoot"
#define LPWSTR_GLOBALROOTPREFIX_W32  L"\\\\?\\GlobalRoot"
#define LPWSTR_DOSNAMESPACEPREFIX    L"\\??\\"

#define _DOS_DRIVE_CHAR(ch) ((L'A' <= ch && ch <= L'Z') || (L'a' <= ch && ch <= L'z'))

// A string minimum length of 6 characters need.
#define PathIsPrefixDosDeviceDrive(p) \
        (\
        p != NULL && \
        p[0] == L'\\' && \
        p[1] == L'?' && \
        p[2] == L'?' && \
        p[3] == L'\\' && \
        ((L'A' <= p[4] && p[4] <= L'Z') || (L'a' <= p[4] && p[4] <= L'z')) && \
        p[5] == L':')

// A string minimum length of 4 characters need.
#define PathIsPrefixDosDevice(p) \
        (\
        p != NULL && \
        p[0] == L'\\' && \
        p[1] == L'?' && \
        p[2] == L'?' && \
        p[3] == L'\\' )

#define PathIsPrefixDosDevice_U(pus) \
        ((pus)->Length >= 8 && PathIsPrefixDosDevice((pus)->Buffer))

#define IS_RELATIVE_DIR_NAME(path) \
            ((path[0] == TEXT('.') && path[1] == TEXT('\0')) || \
             (path[0] == TEXT('.') && path[1] == TEXT('.') && path[2] == TEXT('\0')))

#define IS_RELATIVE_CURRENT_DIR_NAME(path) \
            (path[0] == TEXT('.') && path[1] == TEXT('\0'))

#define IS_RELATIVE_PARENT_DIR_NAME(path) \
            (path[0] == TEXT('.') && path[1] == TEXT('.') && path[2] == TEXT('\0'))

#define IS_RELATIVE_DIR_NAME_WITH_UNICODE_SIZE(path,size) \
            ((path[0] == L'.' && size == sizeof(WCHAR)) || \
            (path[0] == L'.' && path[1] == L'.' && (size == (sizeof(WCHAR)*2))))

#define IS_RELATIVE_CURRENT_DIR_NAME_WITH_UNICODE_SIZE(path,size) \
            (path[0] == L'.' && size == sizeof(WCHAR)) 

#define IS_RELATIVE_PARENT_DIR_NAME_WITH_UNICODE_SIZE(path,size) \
            (path[0] == L'.' && path[1] == L'.' && (size == (sizeof(WCHAR)*2))) 

#define PATHTYPE_WIN32_DEVICE     0x10
#define PATHTYPE_WIN32            0x8
#define PATHTYPE_DOS_DRIVE        0x4
#define PATHTYPE_NT_DOS_DEVICE    0x2
#define PATHTYPE_NT_DEVICE        0x1

ULONG
GetPathType(
    __in PCWSTR pszPath
    );

ULONG
GetPathType_U(
    __in UNICODE_STRING *pusPath
    );

HRESULT
GetVolumeGuidName(
    __in PCWSTR NtDeviceName,
    __out PWSTR GuidName,
    __in int cchGuidName,
    __in ULONG PathType
    );

typedef struct _FS_FILE_DIRECTORY_INFORMATION {
    ULONG cbSize;
    ULONG Reserved;
    LARGE_INTEGER CreationTime;  // same layout from FILE_ID_BOTH_DIR_INFORMATION.CreationTime
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;        // to this member.
    ULONG EaSize;
    CCHAR ShortNameLength;
    WCHAR ShortName[12+1];
    union {
        LARGE_INTEGER FileId;
    };
} FS_FILE_DIRECTORY_INFORMATION;

#define FS_DIRINFO_COMMON_COPY_SIZE (sizeof(ULONG) + sizeof(LARGE_INTEGER) * 6)

EXTERN_C
NTSTATUS
NTAPI
GetDirectoryFileInformation_U(
    __in HANDLE hDirectory,
    __in UNICODE_STRING *pusFileName,
    __out FS_FILE_DIRECTORY_INFORMATION *pInfoBuffer,
    __out_opt UNICODE_STRING *pusFileNameInfo
    );

EXTERN_C
NTSTATUS
NTAPI
MakeSureDirectoryPathExists_W(
    HANDLE DirectoryHandle,
    PCWSTR DirPath
    );

EXTERN_C
NTSTATUS
NTAPI
RenameDirectoryEntry_U(
    HANDLE hExistingDirectory,
    UNICODE_STRING *pusExistingFilePath,
    HANDLE hDestinationDirectory,
    UNICODE_STRING *pusNewFileName,
    BOOLEAN ReplaceIfExists
    );

EXTERN_C
NTSTATUS
NTAPI
RenameDirectoryEntry(
    HANDLE hExistingDirectory,
    PCWSTR pszExistingFileName,
    HANDLE hDestinationDirectory,
    PCWSTR pszNewFileName,
    BOOLEAN ReplaceIfExists
    );

EXTERN_C
NTSTATUS
NTAPI
MoveDirectoryEntry(
    PCWSTR pszSourceFilePath,
    PCWSTR pszDestinationFilePath,
    BOOLEAN ReplaceIfExists
    );

EXTERN_C
NTSTATUS
NTAPI
GetShortPath_W(
    __in PCWSTR pszFullPath,
    __out PWSTR pszShortPathBuffer,
    __in ULONG cchShortPathBuffer
    );

EXTERN_C
NTSTATUS
NTAPI
GetLongPath_W(
    __in PCWSTR pszFullPath,
    __out PWSTR pszLongPathBuffer,
    __in ULONG cchLongPathBuffer
    );

EXTERN_C
HRESULT
NTAPI
SplitRootPath_W(
    __in PCWSTR pszFullyQualifiedPath,
    __inout_opt PWSTR *RootDirectory,
    __inout_opt PULONG RootDirectoryLength,
    __inout_opt PWSTR *RelativePath,
    __inout_opt PULONG RelativePathLength
    );

EXTERN_C
NTSTATUS
NTAPI
GetLongPathNameFromHandle(
    __in HANDLE hFile,
    __inout PWSTR *LongPathName,
    __inout_opt PULONG pcbLongPathName
    );

EXTERN_C
NTSTATUS
NTAPI
GetSystemBootTime(
    __inout_opt LARGE_INTEGER *BootTime,
    __inout_opt LARGE_INTEGER *BootLocalTime,
    __inout_opt LARGE_INTEGER *ElapsedTime
    );

typedef struct _FS_FILE_DATE_TIME
{
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
} FS_FILE_DATE_TIME;

EXTERN_C
NTSTATUS
NTAPI
GetFileDateTime_W(
    __in HANDLE RootHandle,
    __in PCWSTR FilePath,
    __inout FS_FILE_DATE_TIME *pfdt
    );

EXTERN_C
NTSTATUS
NTAPI
SetFileDateTime_W(
    __in HANDLE RootHandle,
    __in PCWSTR FilePath,
    __in FS_FILE_DATE_TIME *pfdt
    );

#ifdef __cplusplus
}
#endif
