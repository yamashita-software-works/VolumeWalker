//***************************************************************************
//*                                                                         *
//*  ntfsrecord.cpp                                                         *
//*                                                                         *
//*  NTFS Record Read Helper                                                *
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
// This code is historically reused and outdated. It may be deprecated and 
// replaced in the future.
//

//
// NOTE:
// The FsSimpleArray class is also intended to be build with WDK environment 
// for user mode. So, do not use Win32 API in class method.
//

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
BOOL GetIndexRoot(HANDLE hVolume, LONGLONG FileId, NTFS_FILE_RECORD_OUTPUT_BUFFER *pnfrec, CNtfsSpecialFileItemList *pFileList)
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
				_GetAttribute_IndexAllocation(hVolume,FileId,pFileList,pAttrRecHdr);
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
		_SafeMemFree(pntfsOutRecord);
		return Status;
	}

	CNtfsSpecialFileItemList *pspfl = new CNtfsSpecialFileItemList;

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

	if( Status != 0 )
	{
		FreeNtfsSpecialFiles(NtfsFileList);
	}

	return Status;
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
