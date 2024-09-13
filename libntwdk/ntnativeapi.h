//
// ntnativeapi.h
//
// NT native API definitions.
//
#ifndef _NTNATIVEAPI_
#define _NTNATIVEAPI_

EXTERN_C
VOID
NTAPI
RtlSetLastWin32Error(
    ULONG ErrorCode
    );

EXTERN_C
ULONG
NTAPI
RtlGetLastWin32Error(
    VOID
    );

#ifndef _NTIFS_

EXTERN_C
NTSTATUS
NTAPI 
RtlInitUnicodeStringEx (
    UNICODE_STRING *DestinationString, 
    PCWSTR SourceString
    );

EXTERN_C
NTSTATUS 
NTAPI
RtlAppendUnicodeToString(
    IN OUT PUNICODE_STRING  Destination,
    IN PCWSTR  Source
    );

#endif

//////////////////////////////////////////////////////////////////////////////

//
// Heap Runtime Routine
//

EXTERN_C
ULONG
NTAPI
RtlGetProcessHeaps(
    ULONG MaxNumberOfHeaps,
    PVOID *HeapArray
    );

EXTERN_C
PVOID
NTAPI
RtlReAllocateHeap(
    HANDLE hHeap,
    ULONG dwFlags,
    PVOID lpMem,
    SIZE_T dwBytes
    );

//////////////////////////////////////////////////////////////////////////////

//
// Path Runtime Routine
//

#include "ntpathcomponent.h"

typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR;

EXTERN_C
NTSYSAPI
ULONG
NTAPI
RtlGetCurrentDirectory_U(
    IN ULONG nBufferLength,
    OUT PWSTR lpBuffer
    );

EXTERN_C
ULONG
NTAPI
RtlGetFullPathName_U(
    PCWSTR lpFileName,
    ULONG nBufferLength,
    PWSTR lpBuffer,
    PWSTR *lpFilePart
    );

EXTERN_C
BOOLEAN
NTAPI
RtlDoesFileExists_U(
    PCWSTR FileName
    );

EXTERN_C
NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosOrNtPathElement(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    );

EXTERN_C
NTSTATUS
NTAPI
RtlGetLengthWithoutTrailingPathSeperators(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    );

typedef enum _RTL_PATH_TYPE {
    RtlPathTypeUnknown,         // 0
    RtlPathTypeUncAbsolute,     // 1
    RtlPathTypeDriveAbsolute,   // 2
    RtlPathTypeDriveRelative,   // 3
    RtlPathTypeRooted,          // 4
    RtlPathTypeRelative,        // 5
    RtlPathTypeLocalDevice,     // 6
    RtlPathTypeRootLocalDevice  // 7
} RTL_PATH_TYPE;

EXTERN_C
NTSYSAPI
RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_U(
    PCWSTR DosFileName
    );

typedef struct _RTLP_CURDIR_REF *PRTLP_CURDIR_REF;

typedef struct _RTL_RELATIVE_NAME_U {
    UNICODE_STRING RelativeName;
    HANDLE ContainingDirectory;
    PRTLP_CURDIR_REF CurDirRef;
} RTL_RELATIVE_NAME_U, *PRTL_RELATIVE_NAME_U;

EXTERN_C
NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToRelativeNtPathName_U(
    PCWSTR DosFileName,
    PUNICODE_STRING NtFileName,
    PWSTR *FilePart,
    PRTL_RELATIVE_NAME_U RelativeName
    );

EXTERN_C
VOID
NTAPI
RtlReleaseRelativeName(
    PRTL_RELATIVE_NAME_U RelativeName
    );

EXTERN_C
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U(
     __in_opt PVOID Environment,
     __in PUNICODE_STRING Name,
     __out PUNICODE_STRING Value
     );

EXTERN_C
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(
    IN PCWSTR DosPathName,
    OUT PUNICODE_STRING NtPathName,
    OUT PWSTR *NtFileNamePart,
    OUT PRTL_RELATIVE_NAME_U DirectoryInfo
    );

struct _RTL_BUFFER;

#if !defined(RTL_BUFFER)

#define RTL_BUFFER RTL_BUFFER

typedef struct _RTL_BUFFER {
    PUCHAR    Buffer;
    PUCHAR    StaticBuffer;
    SIZE_T    Size;
    SIZE_T    StaticSize;
    SIZE_T    ReservedForAllocatedSize; // for future doubling
    PVOID     ReservedForIMalloc; // for future pluggable growth
} RTL_BUFFER, *PRTL_BUFFER;

#endif

#define RTLP_BUFFER_IS_HEAP_ALLOCATED(b) ((b)->Buffer != (b)->StaticBuffer)

struct _RTL_UNICODE_STRING_BUFFER;

typedef struct _RTL_UNICODE_STRING_BUFFER {
    UNICODE_STRING String;
    RTL_BUFFER     ByteBuffer;
    UCHAR          MinimumStaticBufferForTerminalNul[sizeof(WCHAR)];
} RTL_UNICODE_STRING_BUFFER, *PRTL_UNICODE_STRING_BUFFER;

//
// These are OUT Disposition values.
//
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_AMBIGUOUS   (0x00000001)
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_UNC         (0x00000002)
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_DRIVE       (0x00000003)
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_ALREADY_DOS (0x00000004)

NTSYSAPI
NTSTATUS
NTAPI
RtlNtPathNameToDosPathName(
    __in ULONG Flags,
    __inout PRTL_UNICODE_STRING_BUFFER Path,
    __out_opt PULONG Disposition,
    __inout_opt PWSTR* FilePart
    );

#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE (0x00000001)
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING (0x00000002)
#define RTL_DUPSTR_ADD_NULL                          RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE
#define RTL_DUPSTR_ALLOC_NULL                        RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING


// VOID
// RtlInitBuffer(
//     OUT PRTL_BUFFER Buffer,
//     IN  PUCHAR      StaticBuffer,
//     IN  SIZE_T      StaticSize
//     );
#define RtlInitBuffer(Buff, StatBuff, StatSize) \
    do {                                        \
        (Buff)->Buffer       = (StatBuff);      \
        (Buff)->Size         = (StatSize);      \
        (Buff)->StaticBuffer = (StatBuff);      \
        (Buff)->StaticSize   = (StatSize);      \
    } while (0)

#define RTL_ENSURE_BUFFER_SIZE_NO_COPY (0x00000001)

// VOID
// RtlFreeBuffer(
//     IN  PRTL_BUFFER Buffer,
//     );
#define RtlFreeBuffer(Buff)                              \
    do {                                                 \
        if ((Buff) != NULL && (Buff)->Buffer != NULL) {  \
            if (RTLP_BUFFER_IS_HEAP_ALLOCATED(Buff)) {   \
                UNICODE_STRING UnicodeString;            \
                UnicodeString.Buffer = (PWSTR)(PVOID)(Buff)->Buffer; \
                RtlFreeUnicodeString(&UnicodeString);    \
            }                                            \
            (Buff)->Buffer = (Buff)->StaticBuffer;       \
            (Buff)->Size = (Buff)->StaticSize;           \
        }                                                \
    } while (0)

EXTERN_C
BOOLEAN
NTAPI
RtlSetCurrentTransaction(
	HANDLE hTransaction
	);

//////////////////////////////////////////////////////////////////////////////

//
// Process Runtime Routine
//
typedef struct _NT_PEB_FRAGMENT
{
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    union {
        BOOLEAN BitField;
        struct {
            BOOLEAN ImageUsesLargePages : 1;
            BOOLEAN SpareBits : 7;
         };
    };
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PVOID Ldr;               // PEB_LDR_DATA*
    PVOID ProcessParameters; // RTL_USER_PROCESS_PARAMETERS*
    PVOID SubSystemData;
    PVOID ProcessHeap;       // Process Heap
} NT_PEB,NT_PEB_FRAGMENT;

#if 0
#define RtlProcessHeap() (NtCurrentPeb()->ProcessHeap)
#endif

EXTERN_C
PVOID
NTAPI
RtlGetCurrentPeb(
    VOID
    );

typedef struct _SECTION_IMAGE_INFORMATION
{
     PVOID TransferAddress;
     ULONG ZeroBits;
     ULONG MaximumStackSize;
     ULONG CommittedStackSize;
     ULONG SubSystemType;
     union
     {
          struct
          {
               USHORT SubSystemMinorVersion;
               USHORT SubSystemMajorVersion;
          };
          ULONG SubSystemVersion;
     };
     ULONG GpValue;
     USHORT ImageCharacteristics;
     USHORT DllCharacteristics;
     USHORT Machine;
     UCHAR ImageContainsCode;
     UCHAR ImageFlags;
     ULONG ComPlusNativeReady: 1;
     ULONG ComPlusILOnly: 1;
     ULONG ImageDynamicallyRelocated: 1;
     ULONG Reserved: 5;
     ULONG LoaderFlags;
     ULONG ImageFileSize;
     ULONG CheckSum;
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

typedef struct RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

#ifndef _WINTERNL_
typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG MaximumLength;
    ULONG Length;
    ULONG Flags;
    ULONG DebugFlags;
    HANDLE ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;
    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PWSTR Environment;
    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;
    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR CurrentDirectories[32];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;
#endif

#ifndef _NTIFS_
typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;
#endif

typedef struct _RTL_USER_PROCESS_INFORMATION
{
    ULONG Size;
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
    SECTION_IMAGE_INFORMATION ImageInformation;
} RTL_USER_PROCESS_INFORMATION, *PRTL_USER_PROCESS_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserProcess(
    __in PUNICODE_STRING NtImagePathName,
    __in ULONG AttributesDeprecated,
    __in PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    __in_opt PSECURITY_DESCRIPTOR ProcessSecurityDescriptor,
    __in_opt PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    __in_opt HANDLE ParentProcess,
    __in BOOLEAN InheritHandles,
    __in_opt HANDLE DebugPort,
    __in_opt HANDLE TokenHandle, // used to be ExceptionPort
    __out PRTL_USER_PROCESS_INFORMATION ProcessInformation
    );

EXTERN_C
NTSTATUS
NTAPI
RtlCreateProcessParameters(
    OUT PRTL_USER_PROCESS_PARAMETERS *pProcessParameters,
    IN PUNICODE_STRING ImagePathName,
    IN PUNICODE_STRING DllPath OPTIONAL,
    IN PUNICODE_STRING CurrentDirectory OPTIONAL,
    IN PUNICODE_STRING CommandLine OPTIONAL,
    IN PVOID Environment OPTIONAL,
    IN PUNICODE_STRING WindowTitle OPTIONAL,
    IN PUNICODE_STRING DesktopInfo OPTIONAL,
    IN PUNICODE_STRING ShellInfo OPTIONAL,
    IN PUNICODE_STRING RuntimeData OPTIONAL
    );

EXTERN_C
PRTL_USER_PROCESS_PARAMETERS
NTAPI
RtlNormalizeProcessParams(
    IN OUT PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    );

NTSYSAPI
PRTL_USER_PROCESS_PARAMETERS
NTAPI
RtlDeNormalizeProcessParams(
    __inout PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyProcessParameters(
    __in __post_invalid PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    );

typedef NTSTATUS (NTAPI *PUSER_THREAD_START_ROUTINE)(
    __in PVOID ThreadParameter
    );

EXTERN_C
NTSTATUS
NTAPI
RtlCreateUserThread(
    IN HANDLE Process,
    IN PSECURITY_DESCRIPTOR ThreadSecurityDescriptor OPTIONAL,
    IN BOOLEAN CreateSuspended,
    IN ULONG ZeroBits OPTIONAL,
    IN SIZE_T MaximumStackSize OPTIONAL,
    IN SIZE_T CommittedStackSize OPTIONAL,
    IN PUSER_THREAD_START_ROUTINE StartAddress,
    IN PVOID Parameter OPTIONAL,
    OUT PHANDLE Thread OPTIONAL,
    OUT PCLIENT_ID ClientId OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateEnvironment(
    __in BOOLEAN CloneCurrentEnvironment,
    __out PVOID *Environment
    );

//////////////////////////////////////////////////////////////////////////////

//
// NT Native API
//

#ifndef _WDMDDK_
typedef struct _FILE_NETWORK_OPEN_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;
#endif

EXTERN_C
NTSTATUS
NTAPI
NtQueryFullAttributesFile(
    IN POBJECT_ATTRIBUTES  ObjectAttributes,
    OUT PFILE_NETWORK_OPEN_INFORMATION  FileInformation
    );

EXTERN_C
NTSTATUS
NTAPI
NtNotifyChangeDirectoryFile(
    IN HANDLE  FileHandle,
    IN HANDLE  Event OPTIONAL,
    IN PIO_APC_ROUTINE  ApcRoutine OPTIONAL,
    IN PVOID  ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK  IoStatusBlock,
    OUT PVOID  Buffer,
    IN ULONG  BufferLength,
    IN ULONG  NotifyFilter,
    IN BOOLEAN  WatchSubTree
    );

EXTERN_C
NTSTATUS
NTAPI
NtWaitForSingleObject(
    IN HANDLE  Handle,
    IN BOOLEAN  Alertable,
    IN PLARGE_INTEGER  Timeout OPTIONAL
    );

EXTERN_C
NTSTATUS
NTAPI
NtSuspendThread (
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

EXTERN_C
NTSTATUS
NTAPI
NtResumeThread (
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

EXTERN_C
NTSTATUS
NTAPI
NtSuspendProcess (
    __in HANDLE ProcessHandle
    );

EXTERN_C
NTSTATUS
NTAPI
NtResumeProcess (
    __in HANDLE ProcessHandle
    );

EXTERN_C
NTSTATUS
NTAPI
NtQuerySystemTime(
    __in PLARGE_INTEGER SystemTime
    );

EXTERN_C
NTSTATUS
NTAPI
NtDuplicateObject(
    __in HANDLE hSourceProcessHandle,
    __in HANDLE hSourceHandle,
    __in_opt HANDLE hTargetProcessHandle,
    __out_opt PHANDLE pTargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG  Attributes,
    __in ULONG  Options
    );

//////////////////////////////////////////////////////////////////////////////

EXTERN_C
NTSTATUS
NTAPI
RtlLocalTimeToSystemTime(
     __in  PLARGE_INTEGER LocalTime,
     __out PLARGE_INTEGER SystemTime
    );

EXTERN_C
NTSTATUS
NTAPI
RtlSystemTimeToLocalTime(
    __in PLARGE_INTEGER SystemTime,
    __out PLARGE_INTEGER LocalTime
   );


//////////////////////////////////////////////////////////////////////////////

//
// ntifs.h
//

//
// Define the flags for NtSet(Query)EaFile service structure entries
//

#define FILE_NEED_EA                    0x00000080

//
// Define EA type values
//

#define FILE_EA_TYPE_BINARY             0xfffe
#define FILE_EA_TYPE_ASCII              0xfffd
#define FILE_EA_TYPE_BITMAP             0xfffb
#define FILE_EA_TYPE_METAFILE           0xfffa
#define FILE_EA_TYPE_ICON               0xfff9
#define FILE_EA_TYPE_EA                 0xffee
#define FILE_EA_TYPE_MVMT               0xffdf
#define FILE_EA_TYPE_MVST               0xffde
#define FILE_EA_TYPE_ASN1               0xffdd
#define FILE_EA_TYPE_FAMILY_IDS         0xff01

#if (NTDDI_VERSION <= NTDDI_WIN7)

typedef enum _FILE_INFORMATION_CLASS_EX {

        //
        //  These are special versions of these operations (defined earlier)
        //  which can be used by kernel mode drivers only to bypass security
        //  access checks for Rename and HardLink operations.  These operations
        //  are only recognized by the IOManager, a file system should never
        //  receive these.
        //

    FileRenameInformationBypassAccessCheck = 56,  // 56
    FileLinkInformationBypassAccessCheck,    // 57

        //
        // End of special information classes reserved for IOManager.
        //

    FileVolumeNameInformation,               // 58
    FileIdInformation,                       // 59
    FileIdExtdDirectoryInformation,          // 60
    FileReplaceCompletionInformation,        // 61
    FileHardLinkFullIdInformation,           // 62
    FileIdExtdBothDirectoryInformation,      // 63
    FileDispositionInformationEx,            // 64
    FileRenameInformationEx,                 // 65
    FileRenameInformationExBypassAccessCheck, // 66
    FileDesiredStorageClassInformation,      // 67
    FileStatInformation,                     // 68 Windows 10, version 1709.
    FileMemoryPartitionInformation,          // 69
    FileStatLxInformation,                   // 70
    FileCaseSensitiveInformation,            // 71
    FileLinkInformationEx,                   // 72 Windows 10, version 1809.
    FileLinkInformationExBypassAccessCheck,  // 73 Windows 10, version 1809.
    FileStorageReserveIdInformation,         // 74
    FileCaseSensitiveInformationForceAccessCheck, // 75
    FileKnownFolderInformation,              // 76
} FILE_INFORMATION_CLASS_EX, *PFILE_INFORMATION_CLASS_EX;

#endif

//
// Windows 8.1
//

#include "ntfileid.h"

typedef struct _FILE_ID_EXTD_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    ULONG ReparsePointTag;
    FILE_ID_128 FileId;
    WCHAR FileName[1];
} FILE_ID_EXTD_DIR_INFORMATION, *PFILE_ID_EXTD_DIR_INFORMATION;

typedef struct _FILE_ID_EXTD_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    ULONG ReparsePointTag;
    FILE_ID_128 FileId;
    CCHAR ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FILE_ID_EXTD_BOTH_DIR_INFORMATION, *PFILE_ID_EXTD_BOTH_DIR_INFORMATION;

//
// Windows 10
//
#include "ntnotifychangedirectory.h"

//////////////////////////////////////////////////////////////////////////////

#ifdef _NTIFS_

EXTERN_C
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryEaFile(
    __in   HANDLE FileHandle,
    __out  PIO_STATUS_BLOCK IoStatusBlock,
    __out  PVOID Buffer,
    __in   ULONG Length,
    __in   BOOLEAN ReturnSingleEntry,
    __in_opt PVOID EaList,
    __in   ULONG EaListLength,
    __in_opt PULONG EaIndex,
    __in   BOOLEAN RestartScan
    );

EXTERN_C
NTSTATUS
NTAPI
NtQueryAttributesFile(
    __in  POBJECT_ATTRIBUTES ObjectAttributes,
    __out PFILE_BASIC_INFORMATION FileInformation
    );

#endif

//
// for Symboloc Link
//
#define SYMLINK_FLAG_RELATIVE   1

#ifndef _NTIFS_

typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#endif

//
// for App Exec Link
//
typedef struct _REPARSE_APPEXECLINK_READ_BUFFER { // For tag IO_REPARSE_TAG_APPEXECLINK
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    ULONG  Version;	        // Currently version 3
    WCHAR  StringList[1];	// Multistring (Consecutive strings each ending with a NUL)
  /* There are normally 4 strings here. Ex:
    Package ID:	    L"Microsoft.WindowsTerminal_8wekyb3d8bbwe"
    Entry Point:	L"Microsoft.WindowsTerminal_8wekyb3d8bbwe!App"
    Executable:	    L"C:\Program Files\WindowsApps\Microsoft.WindowsTerminal_1.4.3243.0_x64__8wekyb3d8bbwe\wt.exe"
    Applic. Type:	l"0" Integer as ASCII. "0" = Desktop bridge application; Else sandboxed UWP application
  */     
} APPEXECLINK_READ_BUFFER, *PAPPEXECLINK_READ_BUFFER;

#include "ntreparsepointtag.h"

//////////////////////////////////////////////////////////////////////////////

//
// System Information Native API
//

typedef struct _SYSTEM_TIME_OF_DAY_INFORMATION
{
	LARGE_INTEGER BootTime;
	LARGE_INTEGER CurrentTime;
	LARGE_INTEGER TimeZoneBias;
	ULONG CurrentTimeZoneId;	
} SYSTEM_TIME_OF_DAY_INFORMATION;

#ifndef _WINTERNL_
typedef enum {
	SystemTimeOfDayInformation = 3,
} SYSTEM_INFORMATION_CLASS;

EXTERN_C
NTSTATUS
NTAPI
NtQuerySystemInformation (
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength,
    __out_opt PULONG ReturnLength
    );

EXTERN_C
NTSTATUS
NTAPI
NtSetSystemInformation (
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __in_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength
    );
#endif

//////////////////////////////////////////////////////////////////////////////

//
// Object Directory Native API
//

EXTERN_C
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenDirectoryObject (
    __out PHANDLE DirectoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

EXTERN_C
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDirectoryObject (
    __in HANDLE DirectoryHandle,
    __out_bcount_opt(Length) PVOID Buffer,
    __in ULONG Length,
    __in BOOLEAN ReturnSingleEntry,
    __in BOOLEAN RestartScan,
    __inout PULONG Context,
    __out_opt PULONG ReturnLength
    );

EXTERN_C
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSymbolicLinkObject (
    __out PHANDLE LinkHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

EXTERN_C
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySymbolicLinkObject (
    __in HANDLE LinkHandle,
    __inout PUNICODE_STRING LinkTarget,
    __out_opt PULONG ReturnedLength
    );

//////////////////////////////////////////////////////////////////////////////

//
// Volume Information Native API
//

typedef struct _FILE_FS_SECTOR_SIZE_INFORMATION
{
    ULONG LogicalBytesPerSector;
    ULONG PhysicalBytesPerSectorForAtomicity;
    ULONG PhysicalBytesPerSectorForPerformance;
    ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
    ULONG Flags;
    ULONG ByteOffsetForSectorAlignment;
    ULONG ByteOffsetForPartitionAlignment;
} FILE_FS_SECTOR_SIZE_INFORMATION, *PFILE_FS_SECTOR_SIZE_INFORMATION;

typedef struct _FILE_FS_DATA_COPY_INFORMATION
{
    ULONG NumberOfCopies;
} FILE_FS_DATA_COPY_INFORMATION,*PFILE_FS_DATA_COPY_INFORMATION;

typedef struct _FILE_FS_METADATA_SIZE_INFORMATION
{
    LARGE_INTEGER TotalMetadataAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_METADATA_SIZE_INFORMATION,*PFILE_FS_METADATA_SIZE_INFORMATION;

typedef struct _FILE_FS_FULL_SIZE_INFORMATION_EX {
    ULONGLONG ActualTotalAllocationUnits;
    ULONGLONG ActualAvailableAllocationUnits;
    ULONGLONG ActualPoolUnavailableAllocationUnits;
    ULONGLONG CallerTotalAllocationUnits;
    ULONGLONG CallerAvailableAllocationUnits;
    ULONGLONG CallerPoolUnavailableAllocationUnits;
    ULONGLONG UsedAllocationUnits;
    ULONGLONG TotalReservedAllocationUnits;
    ULONGLONG VolumeStorageReserveAllocationUnits;
    ULONGLONG AvailableCommittedAllocationUnits;
    ULONGLONG PoolAvailableAllocationUnits;
    ULONG     SectorsPerAllocationUnit;
    ULONG     BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION_EX, *PFILE_FS_FULL_SIZE_INFORMATION_EX;

//
//  Flag definitions for FILE_FS_SECTOR_SIZE_INFORMATION structure
//
#define SSINFO_FLAGS_ALIGNED_DEVICE                 0x00000001
#define SSINFO_FLAGS_PARTITION_ALIGNED_ON_DEVICE    0x00000002
#define SSINFO_FLAGS_NO_SEEK_PENALTY                0x00000004
#define SSINFO_FLAGS_TRIM_ENABLED                   0x00000008
#define SSINFO_FLAGS_BYTE_ADDRESSABLE               0x00000010
#define SSINFO_OFFSET_UNKNOWN (0xffffffff)

//
// FS_INFORMATION_CLASS Class
//
/*++
typedef enum _FSINFOCLASS {
    FileFsVolumeInformation          = 1,
    FileFsLabelInformation,         // 2
    FileFsSizeInformation,          // 3
    FileFsDeviceInformation,        // 4
    FileFsAttributeInformation,     // 5
    FileFsControlInformation,       // 6
    FileFsFullSizeInformation,      // 7
    FileFsObjectIdInformation,      // 8
    FileFsDriverPathInformation,    // 9
    FileFsVolumeFlagsInformation,   // 10 <-- Win7 WDK defined
    FileFsSectorSizeInformation,    // 11
    FileFsDataCopyInformation,      // 12
    FileFsMetadataSizeInformation,  // 13
    FileFsFullSizeInformationEx,    // 14
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;
--*/
#define FileFsSectorSizeInformation   ((FS_INFORMATION_CLASS)11)
#define FileFsDataCopyInformation     ((FS_INFORMATION_CLASS)12)
#define FileFsMetadataSizeInformation ((FS_INFORMATION_CLASS)13)
#define FileFsFullSizeInformationEx   ((FS_INFORMATION_CLASS)14)

//////////////////////////////////////////////////////////////////////////////

//
// Win32 compatible
//

#ifndef CREATE_NEW
#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5
#endif

#endif
