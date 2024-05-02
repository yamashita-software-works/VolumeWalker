//***************************************************************************
//*                                                                         *
//*  ntfsrecord.cpp                                                         *
//*                                                                         *
//*  NTFS Recore Read Helper                                                *
//*                                                                         *
//*  Create: 2015-07-17,2024-04-24(Ported)                                  *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "fsfilelib.h"

//
// NOTE:
// This code also takes intended to be build with WDK environment (only for user mode).
// So, do not use Win32 API.
//
#include <limits.h>

template <class T>
class FsSimpleArray
{
public:
	T* m_aT;
	int m_nSize;
	int m_nAllocSize;
public:
	FsSimpleArray()
	{
		m_aT = NULL;
		m_nSize = 0;
		m_nAllocSize = 0;
	}

	BOOL Add(const T& t)
	{
		if(m_nSize == m_nAllocSize)
		{
			T* aT;
			int nNewAllocSize = (m_nAllocSize == 0) ? 1 : (m_nSize * 2);
	  
			if (nNewAllocSize<0||nNewAllocSize>INT_MAX/sizeof(T))
			{
				return FALSE;
			}

			aT = (T*)realloc(m_aT, nNewAllocSize * sizeof(T));
			if(aT == NULL)
				return FALSE;
			m_nAllocSize = nNewAllocSize;
			m_aT = aT;
		}
		m_aT[m_nSize] = t;

		m_nSize++;
		return TRUE;
	}

	int GetSize()
	{
		return m_nSize;
	}

	const T& operator[] (int nIndex) const
	{
		_ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if(nIndex < 0 || nIndex >= m_nSize)
		{
			RaiseException( EXCEPTION_ARRAY_BOUNDS_EXCEEDED, EXCEPTION_NONCONTINUABLE, 0, NULL );
		}
		return m_aT[nIndex];
	}

	T& operator[] (int nIndex)
	{
		_ASSERT(nIndex >= 0 && nIndex < m_nSize);
		if(nIndex < 0 || nIndex >= m_nSize)
		{
			RaiseException( EXCEPTION_ARRAY_BOUNDS_EXCEEDED, EXCEPTION_NONCONTINUABLE, 0, NULL );
		}
		return m_aT[nIndex];
	}

	T* GetData() const
	{
		return m_aT;
	}

	BOOL Remove(const T& t)
	{
		int nIndex = Find(t);
		if(nIndex == -1)
			return FALSE;
		return RemoveAt(nIndex);
	}

	BOOL RemoveAt(int nIndex)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		if (nIndex < 0 || nIndex >= m_nSize)
			return FALSE;
		m_aT[nIndex].~T();
		if(nIndex != (m_nSize - 1))
			memmove_s((void*)(m_aT + nIndex), (m_nSize - nIndex) * sizeof(T), (void*)(m_aT + nIndex + 1), (m_nSize - (nIndex + 1)) * sizeof(T));
		m_nSize--;
		return TRUE;
	}

	void RemoveAll()
	{
		if(m_aT != NULL)
		{
			for(int i = 0; i < m_nSize; i++)
				m_aT[i].~T();
			free(m_aT);
			m_aT = NULL;
		}
		m_nSize = 0;
		m_nAllocSize = 0;
    }   
};

class CNtfsSpecialFileItemList : public FsSimpleArray<FS_NTFS_SPECIAL_FILE_ITEM *>
{
public:
	int AddItem( FS_NTFS_SPECIAL_FILE_ITEM *pItem )
	{
		FsSimpleArray<FS_NTFS_SPECIAL_FILE_ITEM *>::Add( pItem );
		return 0;
	}

	LONG GetItemCount()
	{
		return GetSize();
	}

	FS_NTFS_SPECIAL_FILE_ITEM *GetItem(LONG nIndex)
	{
		return m_aT[nIndex];
	}

	LONG GetNameLength(LONG nIndex)
	{
		if( m_nSize == 0 || m_aT == NULL )
			return 0;
		return m_aT[nIndex]->NameLength;
	}
};

#define $UNUSED                          (0x0)
#define $STANDARD_INFORMATION            (0x10)
#define $ATTRIBUTE_LIST                  (0x20)
#define $FILE_NAME                       (0x30)
#define $OBJECT_ID                       (0x40)
#define $SECURITY_DESCRIPTOR             (0x50)
#define $VOLUME_NAME                     (0x60)
#define $VOLUME_INFORMATION              (0x70)
#define $DATA                            (0x80)
#define $INDEX_ROOT                      (0x90)
#define $INDEX_ALLOCATION                (0xA0)
#define $BITMAP                          (0xB0)
#define $SYMBOLIC_LINK                   (0xC0)
#define $EA_INFORMATION                  (0xD0)
#define $EA                              (0xE0)
#define $FIRST_USER_DEFINED_ATTRIBUTE    (0x100)
#define $END                             (0xFFFFFFFF)

#define FILE_NAME_NTFS                   (0x01)
#define FILE_NAME_DOS                    (0x02)

#pragma pack(4)

typedef struct _MULTI_SECTOR_HEADER {
    UCHAR  Signature[4];
    USHORT UpdateSequenceArrayOffset;
    USHORT UpdateSequenceArraySize;
} MULTI_SECTOR_HEADER, *PMULTI_SECTOR_HEADER; // sizeof == 0x8

typedef struct _MFT_SEGMENT_REFERENCE {
    ULONG  SegmentNumberLowPart;
    USHORT SegmentNumberHighPart;
    USHORT SequenceNumber;
} MFT_SEGMENT_REFERENCE, *PMFT_SEGMENT_REFERENCE;

typedef MFT_SEGMENT_REFERENCE FILE_REFERENCE, *PFILE_REFERENCE;;

typedef USHORT UPDATE_SEQUENCE_NUMBER, *PUPDATE_SEQUENCE_NUMBER;
typedef LARGE_INTEGER LSN, *PLSN;
typedef LONGLONG VCN;
typedef UPDATE_SEQUENCE_NUMBER UPDATE_SEQUENCE_ARRAY[1];
typedef UPDATE_SEQUENCE_ARRAY *PUPDATE_SEQUENCE_ARRAY;

typedef struct _FILE_RECORD_SEGMENT_HEADER {
  MULTI_SECTOR_HEADER   MultiSectorHeader;     // +0x00  8bytes
  ULONGLONG             Usn;                   // +0x08  8bytes
  USHORT                SequenceNumber;        // +0x0A  2bytes
  USHORT                LinkCount;             // +0x0C  2bytes
  USHORT                FirstAttributeOffset;  // +0x0E  2bytes
  USHORT                Flags;                 // +0x10  2bytes
  ULONG                 ByteInUse;             // +0x12  4bytes
  ULONG                 BytesAllocated;        // +0x14  4bytes
  FILE_REFERENCE        BaseFileRecordSegment; // +0x16  8bytes
  USHORT                NextAttributeNumber;   // +0x1E  2bytes
  UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;   // array USHORT[1] 
} FILE_RECORD_SEGMENT_HEADER, *PFILE_RECORD_SEGMENT_HEADER; // sizeof==48byte

#define FILE_RECORD_SEGMENT_IN_USE   0x0001
#define FILE_FILE_NAME_INDEX_PRESENT 0x0002
// DeletedFile       = 0x0000,
// ExistingFile      = 0x0001,
// DeletedDirectory  = 0x0002,
// ExistingDirectory = 0x0003

typedef ULONG ATTRIBUTE_TYPE_CODE;

typedef struct _ATTRIBUTE_RECORD_HEADER {
    ATTRIBUTE_TYPE_CODE TypeCode;           // +0x00
    ULONG               RecordLength;       // +0x04
    UCHAR               FormCode;           // +0x08
    UCHAR               NameLength;         // +0x09
    USHORT              NameOffset;         // +0x0A
    USHORT              Flags;              // +0x0C
    USHORT              Instance;           // +0x0E
    union {
        struct {
            ULONG    ValueLength;           // +0x10 The size of the attribute value, in bytes.
            USHORT   ValueOffset;           // +0x14 The offset to the value from the start of the attribute record, in bytes.
            USHORT   ResidentFlags;         // +0x16
        } Resident;
        struct {
            VCN      LowestVcn;             // +0x10
            VCN      HighestVcn;            // +0x18
            USHORT   MappingPairsOffset;    // +0x20
            UCHAR    CompressionUnit;       // +0x22
            UCHAR    Reserved[5];           // +0x23
            LONGLONG AllocatedLength;       // +0x28
            LONGLONG FileSize;              // +0x30
            LONGLONG ValidDataLength;       // +0x38
            LONGLONG TotalAllocated;        // +0x40
        } Nonresident;
    } Form;
} ATTRIBUTE_RECORD_HEADER, *PATTRIBUTE_RECORD_HEADER; // sizeof==72bytes

#define RESIDENT_FORM     0x00
#define NONRESIDENT_FORM  0x01

#define ATTRIBUTE_FLAG_COMPRESSION_MASK 0x00FF
#define ATTRIBUTE_FLAG_SPARSE           0x8000
#define ATTRIBUTE_FLAG_ENCRYPTED        0x4000

#define FILE_ATTRIBUTE_FILENAMESDIRECTORIES 0x10000000
#define FILE_ATTRIBUTE_SECUTIRYDESCRIPTORS  0x20000000
/*++
     Name   FileAttribute
    '$I30'  0x10000000    Filenames Directories
    '$SDH'  0x20000000    Security Descriptors  $Secure
    '$SII'                Security Ids          $Secure
    '$O'                  Object Ids            $ObjId
    '$O'                  Owner Ids             $Quota
    '$Q'                  Quotas                $Quota
    '$R'                  Reparse Points        $Reparse
--*/

typedef struct _STANDARD_INFORMATION {
    LONGLONG CreationTime;                  // +0x00
    LONGLONG LastModificationTime;          // +0x08
    LONGLONG LastChangeTime;                // +0x10
    LONGLONG LastAccessTime;                // +0x18
    ULONG FileAttributes;                   // +0x20
    ULONG MaximumVersions;                  // +0x24
    ULONG VersionNumber;                    // +0x28
    ULONG ClassId;                          // +0x2c
    ULONG OwnerId;                          // +0x30
    ULONG SecurityId;                       // +0x34   
    ULONGLONG QuotaCharged;                 // +0x38
    ULONGLONG Usn;                          // +0x40  
} STANDARD_INFORMATION;                     // sizeof==72bytes

typedef struct _BASIC_INFORMATION {
    LONGLONG CreationTime;                  // +0x00
    LONGLONG LastModificationTime;          // +0x08
    LONGLONG LastChangeTime;                // +0x10
    LONGLONG LastAccessTime;                // +0x18
    LONGLONG AllocatedLength;               // +0x20
    LONGLONG FileSize;                      // +0x28
    ULONG FileAttributes;                   // +0x30
    USHORT PackedEaSize;                    // +0x34
    USHORT Reserved;                        // +0x36
} BASIC_INFORMATION, *PBASIC_INFORMATION;   // sizeof==0x038

typedef struct _FILE_NAME {
    ULONGLONG ParentDirectory;              // +0x00
    BASIC_INFORMATION BasicInfo;            // +0x08
    UCHAR FileNameLength;                   // +0x40
    UCHAR Flags;                            // +0x41
    WCHAR FileName[1];                      // +0x42
} FILE_NAME, *PFILE_NAME;

typedef struct _INDEX_ROOT {
    ULONG IndexedAttributeType;             // +0x00
    ULONG CollationRule;                    // +0x04
    ULONG BytesPerIndexBuffer;              // +0x08
    UCHAR BlocksPerIndexBuffer;             // +0x0C
    UCHAR Reserved[3];                      // +0x0D
} INDEX_ROOT,*PINDEX_ROOT;                  // sizeof==16

typedef struct _INDEX_HEADER {
    ULONG FirstIndexEntry;                  // +0x00
    ULONG FirstFreeByte;                    // +0x04
    ULONG BytesAvailable;                   // +0x08
    UCHAR Flags;                            // +0x0C
    UCHAR Reserved[3];                      // +0x0D
} INDEX_HEADER,*PINDEX_HEADER;              // sizeof==16

typedef struct _INDEX_ENTRY {
    union {
        FILE_REFERENCE FileReference;       // +0x00
        struct 
        {
            USHORT DataOffset;              // +0x00  
            USHORT DataLength;              // +0x02
            ULONG  ReservedForZero;         // +0x04
        };
   } Form;
   USHORT Length;                           // +0x08
   USHORT AttributeLength;                  // +0x0A
   USHORT Flags;                            // +0x0C
   USHORT Reserved;                         // +0x0E
   FILE_NAME Name;                          // +0x10
} INDEX_ENTRY, *PINDEX_ENTRY;               // sizeof==16+sizeof(FILE_NAME)

typedef struct _INDEX_ALLOCATION
{    
    MULTI_SECTOR_HEADER MultiSectorHeader;  // +0x00
    LSN Lsn;                                // +0x08
    VCN ThisBlock;                          // +0x10
//  INDEX_HEADER IndexHeader;               // +0x18
//  UPDATE_SEQUENCE_ARRAY UpdateSequenceArray; // +0x028
} INDEX_ALLOCATION,*PINDEX_ALLOCATION;

#pragma pack()

inline ULONG RunLength(PUCHAR run)
{
    return(*run & 0x0f) + ((*run >> 4) & 0x0f) + 1;
}

inline LONGLONG RunLCN(PUCHAR run)
{
   UCHAR n1 = *run & 0x0f;
   UCHAR n2 = (*run >> 4) & 0x0f;
   LONGLONG lcn = (n2 == 0) ? 0 : (CHAR)(run[n1 + n2]);
   LONG i = 0;
   for (i = n1 +n2 - 1; i > n1; i--)
       lcn = (lcn << 8) + run[i];
   return lcn;
}

inline ULONGLONG RunCount(PUCHAR run)
{
    UCHAR n =  *run & 0xf;
    ULONGLONG count = 0;
    ULONG i = 0;
    for (i = n; i > 0; i--)
        count = (count << 8) + run[i];
    return count;
}

#define _ADDOFFSET(p,off)  (((PUCHAR)p)+((ULONG)off))

//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
//
//  _GetClustersInformation()
//
//----------------------------------------------------------------------------
static
BOOL
_GetClustersInformation(
	HANDLE hVolume,
	HANDLE hFile,
	LONGLONG StartVcn,
	LONGLONG EndVcn,
	PUCHAR pBuffer,
	LONGLONG AllocationSize
	)
{
    STARTING_VCN_INPUT_BUFFER inputVcn;
    RETRIEVAL_POINTERS_BUFFER rpBuf; 
    DWORD dwErrorCode = NO_ERROR;
    BOOL bSuccess = FALSE;
	DWORD cbBytesReturned;

    inputVcn.StartingVcn.QuadPart = StartVcn;

	do
	{ 
        bSuccess = DeviceIoControl(hFile,
			            FSCTL_GET_RETRIEVAL_POINTERS,
						&inputVcn,sizeof(STARTING_VCN_INPUT_BUFFER),
						&rpBuf,sizeof(RETRIEVAL_POINTERS_BUFFER),
			            &cbBytesReturned,
						NULL); 

        dwErrorCode = GetLastError(); 

        switch (dwErrorCode)
		{
	        case ERROR_HANDLE_EOF:
	            bSuccess = TRUE;
		        break;
	        case ERROR_MORE_DATA:
	            inputVcn.StartingVcn = rpBuf.Extents[0].NextVcn;
				// through
	        case NO_ERROR:
			{
				if( rpBuf.ExtentCount > 0 )
				{
					NTFS_VOLUME_DATA_BUFFER ntfsVolumeData = {0};

					DeviceIoControl(hVolume,
							FSCTL_GET_NTFS_VOLUME_DATA,
							NULL,0,
							&ntfsVolumeData,sizeof(NTFS_VOLUME_DATA_BUFFER),
							&cbBytesReturned,
							NULL);

					ULONG clusterSize = ntfsVolumeData.BytesPerCluster;

					LONGLONG Vcn; 
					LONGLONG Lcn;
					LONGLONG NextVcn; 
					LONGLONG ExtentClusterCount;
					LONGLONG OffsetBytes;
					LONGLONG AllocationClusterCount = AllocationSize / clusterSize;

					//
					// The purpose is to read INDEX_ALLOCATION.
					//
					Vcn = rpBuf.StartingVcn.QuadPart;
					Lcn = rpBuf.Extents[0].Lcn.QuadPart;
					NextVcn = rpBuf.Extents[0].NextVcn.QuadPart;

					ExtentClusterCount = NextVcn - Vcn;
					OffsetBytes = Lcn * clusterSize;

					LARGE_INTEGER off;
					off.QuadPart = OffsetBytes;
					if( SetFilePointerEx(hVolume,off,NULL,FILE_BEGIN) )
					{
						if(	!ReadFile(hVolume,pBuffer,(DWORD)AllocationSize,&cbBytesReturned,NULL) )
						{
							bSuccess = FALSE;
							break;
						}
					}

					bSuccess = TRUE;
				}
				else
				{
					bSuccess = FALSE;
				}
	            break; 
			}
			default:
				break;
		} 
	}
	while (dwErrorCode == ERROR_MORE_DATA);

    return bSuccess;
}

//----------------------------------------------------------------------------
//
//  _GetAttribute_StandardInformation
//
//----------------------------------------------------------------------------
static void _GetAttribute_StandardInformation(FS_NTFS_SPECIAL_FILE_ITEM *pItem,STANDARD_INFORMATION *pStdInfo,ULONG cb)
{
	pItem->CreationTime.QuadPart         = pStdInfo->CreationTime;
	pItem->LastModificationTime.QuadPart = pStdInfo->LastModificationTime;
	pItem->LastChangeTime.QuadPart       = pStdInfo->LastChangeTime;
	pItem->LastAccessTime.QuadPart       = pStdInfo->LastAccessTime;
	pItem->Usn                           = pStdInfo->Usn;
}

//----------------------------------------------------------------------------
//
//  _GetAttribute_FileName
//
//----------------------------------------------------------------------------
static void _GetAttribute_FileName(FS_NTFS_SPECIAL_FILE_ITEM *pItem,FILE_NAME *pNameInfo,ULONG cb)
{
    pItem->CreationTime.QuadPart         = pNameInfo->BasicInfo.CreationTime;
    pItem->LastModificationTime.QuadPart = pNameInfo->BasicInfo.LastModificationTime;
    pItem->LastChangeTime.QuadPart       = pNameInfo->BasicInfo.LastChangeTime;
    pItem->LastAccessTime.QuadPart       = pNameInfo->BasicInfo.LastAccessTime;
    pItem->AllocatedLength.QuadPart      = pNameInfo->BasicInfo.AllocatedLength;
    pItem->FileSize.QuadPart             = pNameInfo->BasicInfo.FileSize;
    pItem->FileAttributes                = pNameInfo->BasicInfo.FileAttributes;
    pItem->PackedEaSize                  = pNameInfo->BasicInfo.PackedEaSize;
	pItem->ParentDirectory.QuadPart      = pNameInfo->ParentDirectory;

	ULONG cbName = sizeof(WCHAR)*pNameInfo->FileNameLength;
	ZeroMemory(pItem->Name,cbName+sizeof(WCHAR));
	memcpy(pItem->Name,pNameInfo->FileName,cbName);
	pItem->NameLength = (USHORT)cbName;
}

//----------------------------------------------------------------------------
//
//  _GetAttribute_EnumIndexEntry()
//
//----------------------------------------------------------------------------
static int _GetAttribute_EnumIndexEntry(CNtfsSpecialFileItemList *pFileList,INDEX_ENTRY *pEntry)
{
	INDEX_ENTRY *pEntryTop = pEntry;
	UINT nIncludeShortName = 0;

	union {
		FILE_REFERENCE fr;
		ULONGLONG ul;
	} ref;

	while( pEntry->Form.FileReference.SegmentNumberLowPart != 0 )
	{
		if( pEntry->Name.Flags == FILE_NAME_DOS )
		{
			nIncludeShortName++;
		}

		if( pEntry->Name.Flags == 0 || pEntry->Name.Flags & FILE_NAME_NTFS )
		{
			FS_NTFS_SPECIAL_FILE_ITEM *pItem = (FS_NTFS_SPECIAL_FILE_ITEM *)_MemAllocZero( sizeof(FS_NTFS_SPECIAL_FILE_ITEM) );
			if( pItem == NULL )
			{
				return FALSE;
			}

			_GetAttribute_FileName(pItem,&pEntry->Name,sizeof(FILE_NAME));

			if( pItem->FileAttributes & FILE_ATTRIBUTE_FILENAMESDIRECTORIES )
				pItem->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

			ref.fr = pEntry->Form.FileReference;
			pItem->FileId.QuadPart = ref.ul;

			pFileList->AddItem( pItem );
		}

		pEntry = (INDEX_ENTRY *)_ADDOFFSET(pEntry,pEntry->Length);
	}

	//
	// Processing short name 
	//
	if( nIncludeShortName > 0 )
	{
		pEntry = pEntryTop;

		while( pEntry->Form.FileReference.SegmentNumberLowPart != 0 )
		{
			if( pEntry->Name.Flags == FILE_NAME_DOS )
			{
				int i;
				int c = pFileList->GetItemCount();

				for(i = 0; i < c; i++)
				{
					FS_NTFS_SPECIAL_FILE_ITEM *pItem = pFileList->GetItem(i);

					ref.fr = pEntry->Form.FileReference;
					if( pItem->FileId.QuadPart == ref.ul )
					{
						pItem->ShortNameLength = pEntry->Name.FileNameLength * sizeof(WCHAR);
						RtlCopyMemory(
							pItem->ShortName,
							pEntry->Name.FileName,
							pItem->ShortNameLength
							);
						break;
					}
				}
			}

			pEntry = (INDEX_ENTRY *)_ADDOFFSET(pEntry,pEntry->Length);
		}
	}

	return 0;
}

//----------------------------------------------------------------------------
//
//  _GetAttribute_IndexRoot
//
//----------------------------------------------------------------------------
static int _GetAttribute_IndexRoot(CNtfsSpecialFileItemList *pFileList,INDEX_ROOT *pIndexRoot,ULONG cb)
{
	PUCHAR p = (PUCHAR)pIndexRoot;

	p += sizeof(INDEX_ROOT);
	p += sizeof(INDEX_HEADER);

	INDEX_ENTRY *pEntry = (INDEX_ENTRY *)p;

	_GetAttribute_EnumIndexEntry(pFileList,pEntry);

	return 0;
}

//----------------------------------------------------------------------------
//
//  _GetAttribute_IndexAllocation
//
//----------------------------------------------------------------------------
static int _GetAttribute_IndexAllocation(HANDLE hVolume,LONGLONG FileId,CNtfsSpecialFileItemList *pFileList,ATTRIBUTE_RECORD_HEADER *pAttrRecHdr)
{
	int Status = 0;

	if( pAttrRecHdr->FormCode != NONRESIDENT_FORM )
		return 0;

	if( pAttrRecHdr->Form.Nonresident.CompressionUnit != 0 )
		return 0;

	// pAttrRecHdr->Form.Nonresident.LowestVcn
	// pAttrRecHdr->Form.Nonresident.HighestVcn
	// pAttrRecHdr->Form.Nonresident.AllocatedLength
	// pAttrRecHdr->Form.Nonresident.ValidDataLength
	// pAttrRecHdr->Form.Nonresident.FileSize;
	// pAttrRecHdr->Form.Nonresident.TotalAllocated

	PUCHAR pBuffer = NULL;

	HANDLE hFile;
	FS_FILE_ID_DESCRIPTOR FileIdDesc;

	FileIdDesc.dwSize = sizeof(FileIdDesc);
	FileIdDesc.Type = FsFileIdType;
	FileIdDesc.FileId.QuadPart = FileId;

	if( OpenFile_ID(&hFile,hVolume,&FileIdDesc,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,0) == 0 )
	{
		pBuffer = (PUCHAR)_MemAllocZero((ULONG)pAttrRecHdr->Form.Nonresident.AllocatedLength);
		if( pBuffer != NULL )
		{
			if( !_GetClustersInformation( hVolume, hFile, 
					pAttrRecHdr->Form.Nonresident.LowestVcn,
					pAttrRecHdr->Form.Nonresident.HighestVcn,
					pBuffer,pAttrRecHdr->Form.Nonresident.AllocatedLength ) )
			{
				Status = GetLastError();				
			}
		}
		else
		{
			Status = ERROR_NOT_ENOUGH_MEMORY;
		}
		CloseHandle(hFile);
	}
	else
	{
		Status = GetLastError();
	}

	//
	// Parse 'INDX' buffer
	//
	if( Status == NO_ERROR && pBuffer != NULL )
	{
		PUCHAR pIndexAlloc = (PUCHAR)pBuffer;

		if( RtlEqualMemory(((INDEX_ALLOCATION *)pIndexAlloc)->MultiSectorHeader.Signature,"INDX",4) ) // 'INDX' 49 4e 44 58
		{
			pIndexAlloc += sizeof(INDEX_ALLOCATION);
			pIndexAlloc += ((INDEX_HEADER*)pIndexAlloc)->FirstIndexEntry;

			_GetAttribute_EnumIndexEntry(pFileList,(INDEX_ENTRY*)pIndexAlloc);
		}

		_SafeMemFree(pBuffer);
	}

	return Status;
}

//----------------------------------------------------------------------------
//
//  ParseRecord()
//
//----------------------------------------------------------------------------
BOOL ParseRecord( NTFS_FILE_RECORD_OUTPUT_BUFFER *pnfrec, CNtfsSpecialFileItemList *pFileList )
{
	MULTI_SECTOR_HEADER *psechdr = (MULTI_SECTOR_HEADER *)pnfrec->FileRecordBuffer;

	ULONG sign = *((PULONG)(psechdr->Signature));

	if( memcmp(psechdr->Signature,"FILE",4) != 0 ) // 46 49 4c 45 == 0x454C4946
	{
		return FALSE;
	}

	FILE_RECORD_SEGMENT_HEADER *pfrecSeg = (FILE_RECORD_SEGMENT_HEADER *)psechdr;

	ATTRIBUTE_RECORD_HEADER *pAttrRecHdr;
	pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)_ADDOFFSET(pfrecSeg,pfrecSeg->FirstAttributeOffset);

	if( pAttrRecHdr->Flags == NONRESIDENT_FORM )
	{
		// currently, resident only.
		return FALSE;
	}

	FS_NTFS_SPECIAL_FILE_ITEM *pItem = (FS_NTFS_SPECIAL_FILE_ITEM *)_MemAllocZero( sizeof(FS_NTFS_SPECIAL_FILE_ITEM) );
	if( pItem == NULL )
	{
		return FALSE;
	}

	for(;;)
	{
		if( pAttrRecHdr->TypeCode == $END )
			break;

		switch( pAttrRecHdr->TypeCode )
		{
			case $STANDARD_INFORMATION:
				_GetAttribute_StandardInformation(
					pItem,
					(STANDARD_INFORMATION *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Resident.ValueOffset),
					pAttrRecHdr->Form.Resident.ValueLength
					);
				break;
			case $FILE_NAME:
				_GetAttribute_FileName(
					pItem,
					(FILE_NAME *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Resident.ValueOffset),
					pAttrRecHdr->Form.Resident.ValueLength
					);
				break;
			case $INDEX_ROOT:
			{
				PWSTR pAttrName = (PWSTR)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->NameOffset);

				if( memcmp(pAttrName,L"$I30",pAttrRecHdr->NameLength) == 0 )
				{
					pItem->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
				}
				break;
			}
		}

		pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->RecordLength);
	}

	if( pItem->Name[0] != 0 )
	{
		union {
			FILE_REFERENCE fr;
			ULONGLONG ul;
		} ref;

		ref.ul = pnfrec->FileReferenceNumber.QuadPart;
		ref.fr.SequenceNumber = pfrecSeg->SequenceNumber;

		pItem->FileId.QuadPart = ref.ul;

		pFileList->AddItem( pItem );
	}
	else
	{
		_SafeMemFree( pItem );
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//
//  GetIndexRoot()
//
//----------------------------------------------------------------------------
BOOL GetIndexRoot(HANDLE hVolume, LONGLONG FildId, NTFS_FILE_RECORD_OUTPUT_BUFFER *pnfrec, CNtfsSpecialFileItemList *pFileList)
{
	MULTI_SECTOR_HEADER *psechdr = (MULTI_SECTOR_HEADER *)pnfrec->FileRecordBuffer;

	ULONG sign = *((PULONG)(psechdr->Signature));

	if( memcmp(psechdr->Signature,"FILE",4) != 0 ) // 46 49 4c 45 == 0x454C4946
	{
		return FALSE;
	}

	FILE_RECORD_SEGMENT_HEADER *pfrecSeg = (FILE_RECORD_SEGMENT_HEADER *)psechdr;

	ATTRIBUTE_RECORD_HEADER *pAttrRecHdr;
	pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)_ADDOFFSET(pfrecSeg,pfrecSeg->FirstAttributeOffset);

	if( pAttrRecHdr->Flags == NONRESIDENT_FORM )
	{
		// currently, resident only.
		return FALSE;
	}

	for(;;)
	{
		if( pAttrRecHdr->TypeCode == $END )
			break;

		switch( pAttrRecHdr->TypeCode )
		{
			case $STANDARD_INFORMATION:
				break;
			case $FILE_NAME:
				break;
			case $INDEX_ROOT:
			{
				PWSTR pAttrName = (PWSTR)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->NameOffset);

				if( memcmp(pAttrName,L"$I30",pAttrRecHdr->NameLength) == 0 )
				{
					_GetAttribute_IndexRoot(
						pFileList,
						(INDEX_ROOT*)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Resident.ValueOffset),
						pAttrRecHdr->Form.Resident.ValueLength
						);
				}
				break;
			}
			case $INDEX_ALLOCATION:
			{
				_GetAttribute_IndexAllocation(hVolume,FildId,pFileList,pAttrRecHdr);
				break;
			}
		}

		pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->RecordLength);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//
//  GetNtfsSpecialFiles()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
// NOTE:
// - Need set FILE_READ_DATA flag is required when open the volume to read records.
//
EXTERN_C
ULONG
NTAPI
GetNtfsSpecialFiles(
	PCWSTR pszVolumeName,
	LONGLONG FileId,
	BOOL bEnumItem,
	FS_NTFS_SPECIAL_FILE_LIST *NtfsFileList
	)
{
	CNtfsSpecialFileItemList *pspfl = new CNtfsSpecialFileItemList;

	IO_STATUS_BLOCK IoStatusBlock = {0};

	NTFS_FILE_RECORD_INPUT_BUFFER ntfsInRecord = {0};
	NTFS_FILE_RECORD_OUTPUT_BUFFER *pntfsOutRecord = NULL;

	ULONG cbOutputBuffer = 4096 * 10;

	pntfsOutRecord = (NTFS_FILE_RECORD_OUTPUT_BUFFER *)_MemAlloc( cbOutputBuffer );
	if( pntfsOutRecord == NULL )
	{
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	HANDLE hVolume;
	UNICODE_STRING usVolume;
	UNICODE_STRING usPath;
	NTSTATUS Status;
	DWORD cbBytesReturned;

	SplitVolumeRelativePath(pszVolumeName,&usVolume,&usPath);

	Status = OpenFile_U(&hVolume,NULL,&usVolume,
					FILE_READ_DATA|FILE_READ_ATTRIBUTES|SYNCHRONIZE,
					FILE_SHARE_READ|FILE_SHARE_WRITE,
					FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

	if( Status != NO_ERROR )
	{
		return Status;
	}

	// The MFT contains file record segments; 
	// the first 16 of these are reserved for special files, such as the following:
	// ex)
	//    0: MFT ($Mft)
	//    5: root directory (\)
	//    6: volume cluster allocation file ($Bitmap)
	//    8: bad-cluster file ($BadClus)
	//    ...

	if( bEnumItem )
	{
		if( FileId == 0 )
		{
			int i;
			for(i = 0; i < 16; i++)
			{
				ntfsInRecord.FileReferenceNumber.QuadPart = i;

				if( i != 5 ) // except root
				{
					DeviceIoControl(
						hVolume,
						FSCTL_GET_NTFS_FILE_RECORD,
						&ntfsInRecord,sizeof(NTFS_FILE_RECORD_INPUT_BUFFER),
						pntfsOutRecord,cbOutputBuffer,
						&cbBytesReturned,
						NULL);

					Status = GetLastError();

					if( Status == NO_ERROR )
					{
						ParseRecord( pntfsOutRecord, pspfl );
					}
				}
			}
		}
		else
		{
			ntfsInRecord.FileReferenceNumber.QuadPart = FileId;

			DeviceIoControl(
				hVolume,
				FSCTL_GET_NTFS_FILE_RECORD,
				&ntfsInRecord,sizeof(NTFS_FILE_RECORD_INPUT_BUFFER),
				pntfsOutRecord,cbOutputBuffer,
				&cbBytesReturned,
				NULL);

			Status = GetLastError();

			if( Status == NO_ERROR )
			{
				GetIndexRoot( hVolume, FileId, pntfsOutRecord, pspfl );
			}
		}
	}
	else
	{
		ntfsInRecord.FileReferenceNumber.QuadPart = FileId;

		DeviceIoControl(
			hVolume,
			FSCTL_GET_NTFS_FILE_RECORD,
			&ntfsInRecord,sizeof(NTFS_FILE_RECORD_INPUT_BUFFER),
			pntfsOutRecord,cbOutputBuffer,
			&cbBytesReturned,
			NULL);
			Status = GetLastError();

		if( Status == NO_ERROR )
		{
			ParseRecord( pntfsOutRecord, pspfl );
		}
	}

	_MemFree(pntfsOutRecord);

	CloseHandle(hVolume);

	NtfsFileList->cItemListCount = pspfl->GetItemCount();
	NtfsFileList->pItemList = pspfl->GetData();
	NtfsFileList->Handle = (HANDLE)pspfl;

	return 0;
}

//----------------------------------------------------------------------------
//
//  FreeNtfsSpecialFiles()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
FreeNtfsSpecialFiles(
	FS_NTFS_SPECIAL_FILE_LIST *FileList
	)
{
	int i;
	int	cSpecialFiles = FileList->cItemListCount;

	for(i = 0; i < cSpecialFiles; i++)
	{
		FS_NTFS_SPECIAL_FILE_ITEM *pItem = FileList->pItemList[i];
		_MemFree( pItem );
	}

	((CNtfsSpecialFileItemList *)FileList->Handle)->RemoveAll();

	delete ((CNtfsSpecialFileItemList *)FileList->Handle);

	return 0;
}
