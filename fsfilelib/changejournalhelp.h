#pragma once

/////////////////////////////////////////////////////////////////////////////
// changejournalhelp.h

typedef struct {
    USN       StartUsn;
    DWORD     ReasonMask;
    DWORD     ReturnOnlyOnClose;
    DWORDLONG Timeout;
    DWORDLONG BytesToWaitFor;
    DWORDLONG UsnJournalID;
    WORD      MinMajorVersion;
    WORD      MaxMajorVersion;
} READ_USN_JOURNAL_DATA_V1, *PREAD_USN_JOURNAL_DATA_V1;

typedef struct {
    DWORDLONG UsnJournalID;
    USN       FirstUsn;
    USN       NextUsn;
    USN       LowestValidUsn;
    USN       MaxUsn;
    DWORDLONG MaximumSize;
    DWORDLONG AllocationDelta;
    WORD      MinSupportedMajorVersion;
    WORD      MaxSupportedMajorVersion;
} USN_JOURNAL_DATA_V1, *PUSN_JOURNAL_DATA_V1;

typedef struct {
    DWORDLONG UsnJournalID;
    USN       FirstUsn;
    USN       NextUsn;
    USN       LowestValidUsn;
    USN       MaxUsn;
    DWORDLONG MaximumSize;
    DWORDLONG AllocationDelta;
    WORD      MinSupportedMajorVersion;
    WORD      MaxSupportedMajorVersion;
    DWORD     Flags;
    DWORDLONG RangeTrackChunkSize;
    LONGLONG  RangeTrackFileSizeThreshold;
} USN_JOURNAL_DATA_V2, *PUSN_JOURNAL_DATA_V2;

#ifndef FLAG_USN_TRACK_MODIFIED_RANGES_ENABLE
#define FLAG_USN_TRACK_MODIFIED_RANGES_ENABLE 0x00000001
#endif

typedef struct {
  DWORD         RecordLength;
  WORD          MajorVersion;
  WORD          MinorVersion;
  BYTE          FileReferenceNumber[16];
  BYTE          ParentFileReferenceNumber[16];
  USN           Usn;
  LARGE_INTEGER TimeStamp;
  DWORD         Reason;
  DWORD         SourceInfo;
  DWORD         SecurityId;
  DWORD         FileAttributes;
  WORD          FileNameLength;
  WORD          FileNameOffset;
  WCHAR         FileName[1];
} USN_RECORD_V3, *PUSN_RECORD_V3;

typedef struct {
  WORD MinMajorVersion;
  WORD MaxMajorVersion;
} READ_FILE_USN_DATA, *PREAD_FILE_USN_DATA;

interface IFSJournalBuffer
{
	virtual VOID Release() = 0;
	virtual int GetCount() = 0;
	virtual PVOID GetBuffer(int iBufferIndex) = 0;
	virtual SIZE_T GetBufferSize(int iBufferIndex) = 0;
	virtual LPTSTR GetVolumeName(LPWSTR VolumeName,int cchVolumeName) = 0;
	virtual SIZE_T GetTotalRecordCount() const = 0;
	virtual BOOL SetJournalData(PVOID ptr, int  cbSize) = 0;
	virtual PVOID GetJournalData() const  = 0;
	virtual ULONG GetJournalDataLength() const = 0; 
};

EXTERN_C BOOL WINAPI CreateJournalBuffer(PCWSTR VolumeName,PVOID *ppData);
EXTERN_C BOOL WINAPI DestroyJournalBuffer(PVOID pData);
EXTERN_C ULONG WINAPI GetJournalReasonAbbreviationsText(ULONG Reason,PWSTR Text,ULONG cchText,BOOL bBrackets);
EXTERN_C ULONG WINAPI GetJournalReasonText(ULONG Reason,PWSTR Text,ULONG cchText);
EXTERN_C BOOL WINAPI MakeChangeJournalDumpFile(PCWSTR pszVolumeName,PCWSTR ,PVOID *ppData);
EXTERN_C BOOL WINAPI MakeChangeJournalDumpFileEx(PCWSTR pszVolumeName,PCWSTR ,PVOID *ppData);
EXTERN_C BOOL WINAPI ReadChangeJournalDumpFile(PCWSTR pszVolumeName,PVOID *ppData);
EXTERN_C BOOL WINAPI SaveChangeJournalDumpFile(IFSJournalBuffer *pJournalBuffer,PCWSTR pszFileName);

typedef struct {
    DWORDLONG UsnJournalID;
    USN FirstUsn;
    USN NextUsn;
    USN LowestValidUsn;
    USN MaxUsn;
    DWORDLONG MaximumSize;
    DWORDLONG AllocationDelta;
} FS_USN_JOURNAL_DATA, *PFS_USN_JOURNAL_DATA;

EXTERN_C BOOL WINAPI QueryJournalInformation(PCWSTR pszVolumeName,FS_USN_JOURNAL_DATA *pInfo);

typedef struct _FS_FILE_USN_INFORMATION
{
	USN Usn;
} FS_FILE_USN_INFORMATION, *PFS_FILE_USN_INFORMATION;

EXTERN_C ULONG WINAPI GetFileUsn(HANDLE hFileHandle,FS_FILE_USN_INFORMATION *pUsnInfo);

#ifdef __cplusplus
#define _INLINE_FUNC __forceinline
#else
#define _INLINE_FUNC
#endif

_INLINE_FUNC PCWSTR _UsnGetItem_FileName(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return (PCWSTR)(((ULONG_PTR)UsnRecord) + ((PUSN_RECORD_V3)UsnRecord)->FileNameOffset);
	return (PCWSTR)(((ULONG_PTR)UsnRecord) + UsnRecord->FileNameOffset);
}

_INLINE_FUNC WORD _UsnGetItem_FileNameOffset(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return ((PUSN_RECORD_V3)UsnRecord)->FileNameOffset;
	return UsnRecord->FileNameOffset;
}

_INLINE_FUNC WORD _UsnGetItem_FileNameLength(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return ((PUSN_RECORD_V3)UsnRecord)->FileNameLength;
	return UsnRecord->FileNameLength;
}

_INLINE_FUNC ULONG _UsnGetItem_FileNameLengthCch(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return (ULONG)((PUSN_RECORD_V3)UsnRecord)->FileNameLength/sizeof(WCHAR);
	return (ULONG)(UsnRecord->FileNameLength/sizeof(WCHAR));
}

_INLINE_FUNC USN _UsnGetItem_Usn(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return ((PUSN_RECORD_V3)UsnRecord)->Usn;
	return UsnRecord->Usn;
}

_INLINE_FUNC DWORDLONG _UsnGetItem_FileReferenceNumber(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return *(DWORDLONG*)&((PUSN_RECORD_V3)UsnRecord)->FileReferenceNumber; // not 128bit ID
	return UsnRecord->FileReferenceNumber;
}

_INLINE_FUNC DWORDLONG _UsnGetItem_ParentFileReferenceNumber(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return *(DWORDLONG*)&((PUSN_RECORD_V3)UsnRecord)->ParentFileReferenceNumber; // not 128bit ID
	return UsnRecord->ParentFileReferenceNumber;
}

_INLINE_FUNC LARGE_INTEGER _UsnGetItem_TimeStamp(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return ((PUSN_RECORD_V3)UsnRecord)->TimeStamp;
	return UsnRecord->TimeStamp;
}

_INLINE_FUNC ULONG64 _UsnGetItem_TimeStamp64(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return (ULONG64)((PUSN_RECORD_V3)UsnRecord)->TimeStamp.QuadPart;
	return (ULONG64)UsnRecord->TimeStamp.QuadPart;
}

_INLINE_FUNC DWORD _UsnGetItem_Reason(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return ((PUSN_RECORD_V3)UsnRecord)->Reason;
	return UsnRecord->Reason;
}

_INLINE_FUNC DWORD _UsnGetItem_SourceInfo(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return ((PUSN_RECORD_V3)UsnRecord)->SourceInfo;
	return UsnRecord->SourceInfo;
}

_INLINE_FUNC DWORD _UsnGetItem_SecurityId(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return ((PUSN_RECORD_V3)UsnRecord)->SecurityId;
	return UsnRecord->SecurityId;
}

_INLINE_FUNC DWORD _UsnGetItem_FileAttributes(PUSN_RECORD UsnRecord)
{
	if( UsnRecord->MajorVersion == 3 )
		return ((PUSN_RECORD_V3)UsnRecord)->FileAttributes;
	return UsnRecord->FileAttributes;
}

_INLINE_FUNC ULONG _UsnGetItem_GetFileName(PUSN_RECORD UsnRecord,PWSTR FileName,ULONG cchFileName)
{
	if( UsnRecord == NULL || FileName == NULL || cchFileName == 0 )
		return 0;
	SIZE_T cch = min(_UsnGetItem_FileNameLengthCch(UsnRecord),(cchFileName-1));
	memcpy(FileName,_UsnGetItem_FileName(UsnRecord),cch*sizeof(WCHAR));
	FileName[cch] = L'\0';
	return (int)cch;
}
