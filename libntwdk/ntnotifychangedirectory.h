#ifndef _NTNOTIFYCHANGEDIRECTORY_
#define _NTNOTIFYCHANGEDIRECTORY_

//
// ReadDirectoryChangesExW/NtNotifyChangeDirectoryFileEx
//

typedef enum _DIRECTORY_NOTIFY_INFORMATION_CLASS
{
    DirectoryNotifyInformation = 1,     // FILE_NOTIFY_INFORMATION
    DirectoryNotifyExtendedInformation, // FILE_NOTIFY_EXTENDED_INFORMATION
    DirectoryNotifyFullInformation,     // FILE_NOTIFY_FULL_INFORMATION // since 22H2
    DirectoryNotifyMaximumInformation
} DIRECTORY_NOTIFY_INFORMATION_CLASS, *PDIRECTORY_NOTIFY_INFORMATION_CLASS;

typedef struct _FILE_NOTIFY_EXTENDED_INFORMATION {
  ULONG         NextEntryOffset;
  ULONG         Action;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastModificationTime;
  LARGE_INTEGER LastChangeTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER AllocatedLength;
  LARGE_INTEGER FileSize;
  ULONG         FileAttributes;
  union {
    ULONG ReparsePointTag;
    ULONG EaSize;
  } DUMMYUNIONNAME;
  LARGE_INTEGER FileId;
  LARGE_INTEGER ParentFileId;
  ULONG         FileNameLength;
  WCHAR         FileName[1];
} FILE_NOTIFY_EXTENDED_INFORMATION, *PFILE_NOTIFY_EXTENDED_INFORMATION;

typedef struct _FILE_NOTIFY_FULL_INFORMATION {
  ULONG         NextEntryOffset;
  ULONG         Action;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastModificationTime;
  LARGE_INTEGER LastChangeTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER AllocatedLength;
  LARGE_INTEGER FileSize;
  ULONG         FileAttributes;
  union {
    ULONG ReparsePointTag;
    ULONG EaSize;
  } DUMMYUNIONNAME;
  LARGE_INTEGER FileId;
  LARGE_INTEGER ParentFileId;
  USHORT        FileNameLength;
  UCHAR         FileNameFlags;
  UCHAR         Reserved;
  WCHAR         FileName[1];
} FILE_NOTIFY_FULL_INFORMATION, *PFILE_NOTIFY_FULL_INFORMATION;

#define FILE_NAME_NTFS 0x01
#define FILE_NAME_DOS  0x02

#endif
