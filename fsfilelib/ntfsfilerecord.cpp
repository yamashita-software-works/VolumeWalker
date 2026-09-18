//*****************************************************************************
//*                                                                           *
//*  ntfsfilerecord.cpp                                                       *
//*                                                                           *
//*  NTFS file record information helper                                      *
//*                                                                           *
//*  Author:  YAMASHITA Katsuhiro                                             *
//*                                                                           *
//*  History: 2026-06-14 Created.                                             *
//*                                                                           *
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "fsfilelib.h"
#include <limits.h>

inline PVOID _AllocMemory( SIZE_T cb )
{
	return AllocMemory(cb);
}

inline PVOID _ReallocMemory(PVOID pbuf,SIZE_T cbReturnDataLength)
{
	return ReallocMemory(pbuf,cbReturnDataLength);
}

inline VOID _FreeMemory(PVOID pbuf)
{
	FreeMemory(pbuf);
}

//----------------------------------------------------------------------------
//
//  FreeNtfsMemory()
//
//----------------------------------------------------------------------------
EXTERN_C
void 
WINAPI
FreeNtfsMemory(
	PVOID pv
	)
{
	_FreeMemory(pv);
}

//----------------------------------------------------------------------------
//
//  GetNtfsRecordType()
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
GetNtfsRecordType(
	PVOID Header
	)
{
	if( Header == NULL )
		return (ULONG)0;
	ATTRIBUTE_RECORD_HEADER *pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)Header;
	return pAttrRecHdr->TypeCode;
}

//----------------------------------------------------------------------------
//
//  GetNtfsRecordLength()
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
GetNtfsRecordLength(
	PVOID Header
	)
{
	if( Header == NULL )
		return (ULONG)0;
	ATTRIBUTE_RECORD_HEADER *pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)Header;
	return pAttrRecHdr->RecordLength;
}

//----------------------------------------------------------------------------
//
//  GetNtfsRecordAttributeList()
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
GetNtfsRecordAttributeList(
	PVOID Header,
	PVOID *pAttributeList,
	ULONG *pcb
	)
{
	if( Header == NULL )
		return (ULONG)0;
	ATTRIBUTE_RECORD_HEADER *pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)Header;

	PWSTR pAttrName = (PWSTR)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->NameOffset);
	if( pAttrRecHdr->Flags == RESIDENT_FORM )
	{
		PBYTE pb = (PBYTE)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Resident.ValueOffset);
		ULONG cb = pAttrRecHdr->Form.Resident.ValueLength;
		ATTRIBUTE_LIST_ENTRY *pa = (ATTRIBUTE_LIST_ENTRY *)pb;
		*pAttributeList = pa;
		*pcb = cb;
		return cb;
	}
	return 0;
}

//----------------------------------------------------------------------------
//
//  GetNtfsRecordAttributeEntryLength()
//
//----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
GetNtfsRecordAttributeEntryLength(
	PVOID Entry
	)
{
	if( Entry == NULL )
		return (ULONG)0;
	ATTRIBUTE_LIST_ENTRY *pAttrEntry = (ATTRIBUTE_LIST_ENTRY *)Entry;
	return pAttrEntry->RecordLength;
}


//----------------------------------------------------------------------------
//
//  GetNtfsRecordNameString()
//
//----------------------------------------------------------------------------
#define _DEF_NTFS_RECORD_STRING(id,desc)  { id, L#id, L##desc }

typedef struct _NT_NTFS_RECORD_STRING
{
	ULONG Type;
	PWSTR Name;
	PWSTR Desc;
} NT_NTFS_RECORD_STRING;

NT_NTFS_RECORD_STRING name[] = {
	_DEF_NTFS_RECORD_STRING($STANDARD_INFORMATION, ""),
	_DEF_NTFS_RECORD_STRING($ATTRIBUTE_LIST,       ""),
	_DEF_NTFS_RECORD_STRING($FILE_NAME,            ""),
	_DEF_NTFS_RECORD_STRING($OBJECT_ID,            ""),
	_DEF_NTFS_RECORD_STRING($SECURITY_DESCRIPTOR,  ""),
	_DEF_NTFS_RECORD_STRING($VOLUME_NAME,          ""),
	_DEF_NTFS_RECORD_STRING($VOLUME_INFORMATION,   ""),
	_DEF_NTFS_RECORD_STRING($DATA,                 ""),
	_DEF_NTFS_RECORD_STRING($INDEX_ROOT,           ""),
	_DEF_NTFS_RECORD_STRING($INDEX_ALLOCATION,     ""),
	_DEF_NTFS_RECORD_STRING($BITMAP,               ""),
	_DEF_NTFS_RECORD_STRING($SYMBOLIC_LINK,        ""),
	_DEF_NTFS_RECORD_STRING($EA_INFORMATION,       ""),
	_DEF_NTFS_RECORD_STRING($EA,                   ""),
};

EXTERN_C
ULONG
WINAPI
GetNtfsRecordNameString(
	UINT id,
	PWSTR szBuf,
	int cchBuf,
	ULONG dwFlags
	)
{
	for(int i = 0; i < _countof(name); i++)
	{
		if( id == name[i].Type )
		{
			StringCchCopy(szBuf,cchBuf,name[i].Name);
			return (ULONG)wcslen(szBuf);
		}
	}
	StringCchPrintf(szBuf,cchBuf,L"$0x%X",id);
	return (ULONG)wcslen(szBuf);
}

//-----------------------------------------------------------------------------
//
//  NtDosGetNtfsFileRecordInformation()
//
//  PURPOSE:
//
//-----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
NtDosGetNtfsFileRecordInformation(
	PCWSTR pszVolumeName,
	HANDLE VolumeHandle,
	LONGLONG FileId,
	PVOID Buffer,
	ULONG BufferLength,
	PULONG ReturnDataLength
	)
{
	IO_STATUS_BLOCK IoStatusBlock = {0};

	ULONG cbOutputBuffer = 4096 * 10;

	HANDLE hVolume = NULL;
	DWORD dwWin32Error = 0;

	if( VolumeHandle == NULL )
	{
		NTSTATUS Status;
		UNICODE_STRING usVolume;
		UNICODE_STRING usPath;

		SplitVolumeRelativePath(pszVolumeName,&usVolume,&usPath);

		Status = OpenFile_U(&hVolume,NULL,&usVolume,
						FILE_READ_DATA|FILE_READ_ATTRIBUTES|SYNCHRONIZE,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

		if( Status != NO_ERROR )
		{
			return NtStatusToDosError(Status);
		}
	}
	else
	{
		hVolume = VolumeHandle;
	}

	NTFS_FILE_RECORD_INPUT_BUFFER ntfsInRecord = {0};
	NTFS_FILE_RECORD_OUTPUT_BUFFER *pntfsOutRecord = NULL;

	pntfsOutRecord = (NTFS_FILE_RECORD_OUTPUT_BUFFER *)_MemAlloc( cbOutputBuffer );
	if( pntfsOutRecord != NULL )
	{
		DWORD cbBytesReturned = 0;

		ntfsInRecord.FileReferenceNumber.QuadPart = FileId;

		if( DeviceIoControl(
				hVolume,
				FSCTL_GET_NTFS_FILE_RECORD,
				&ntfsInRecord,sizeof(NTFS_FILE_RECORD_INPUT_BUFFER),
				pntfsOutRecord,cbOutputBuffer,
				&cbBytesReturned,
				NULL) )
		{
			if( cbBytesReturned <= BufferLength )
			{
				RtlCopyMemory(Buffer,pntfsOutRecord,cbBytesReturned);
				dwWin32Error = NO_ERROR;
			}
			else
			{
				dwWin32Error = ERROR_INSUFFICIENT_BUFFER;
			}
		}
		else
		{
			dwWin32Error = GetLastError();
		}

		if( ReturnDataLength )
			*ReturnDataLength = cbBytesReturned;

		_MemFree(pntfsOutRecord);
	}
	else
	{
		dwWin32Error = ERROR_NOT_ENOUGH_MEMORY;
	}

	if( VolumeHandle == NULL )
	{
		CloseHandle(hVolume);
	}

	return dwWin32Error;
}

//-----------------------------------------------------------------------------
//
//  GetNtfsFileRecordInformation()
//
//  PURPOSE:
//
//-----------------------------------------------------------------------------
EXTERN_C
ULONG
WINAPI
GetNtfsFileRecordInformation(
	PCWSTR pszVolumeName,
	HANDLE VolumeHandle,
	LONGLONG FileId,
	PVOID *Buffer,
	LPDWORD pcbBuffer
	)
{
	ULONG Status;
	ULONG cbBufferLength = 4096;
	ULONG cbReturnDataLength = 0;
	PBYTE pbuf;

	FileId &= ~0xFFFF000000000000;

	for(;;)
	{
		pbuf = (PBYTE)_AllocMemory(cbBufferLength);

		if( pbuf == NULL )
			return ERROR_OUTOFMEMORY;

		Status = NtDosGetNtfsFileRecordInformation(pszVolumeName,VolumeHandle,FileId,pbuf,cbBufferLength,&cbReturnDataLength);
		if( Status != ERROR_INSUFFICIENT_BUFFER )
			break;

		_FreeMemory(pbuf);

		cbBufferLength *= 2;
	}

	if( Status == NO_ERROR )
	{
		PBYTE pb = (PBYTE)_ReallocMemory(pbuf,cbReturnDataLength);
		if( pb == NULL )
		{
			_FreeMemory(pbuf);
			return ERROR_OUTOFMEMORY;
		}

		*Buffer = pb;

		if( pcbBuffer )
			*pcbBuffer = cbReturnDataLength;
	}

	return Status;
}

//-----------------------------------------------------------------------------
//
//  GetNtfsClusterMappingPair()
//
//  PURPOSE:
//
//-----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
GetNtfsClusterMappingPair(
	ATTRIBUTE_RECORD_HEADER *pAttrRecHdr,
	CLUSTER_MAPPING_PAIR **pClusters,
	LONGLONG *pn
	)
{
	LONGLONG CurrentVcn = 0;
	LONGLONG CurrentLcn = 0;
	LONGLONG cnt;
	LONGLONG lcn;
	LONGLONG total_cnt = 0;

	if( pAttrRecHdr == NULL )
		return E_INVALIDARG;

	// Initialize NextVcn to Attribute->LowestVcn and CurrentLcn to 0.
	LONGLONG NextVcn = pAttrRecHdr->Form.Nonresident.LowestVcn;

	// Initialize the byte stream pointer to (PCHAR)Attribute + Attribute->AttributeForm->Nonresident->MappingPairsOffset.
	PUCHAR RunPair = _ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Nonresident.MappingPairsOffset);

	// Count all elements
	while( *RunPair != 0 )
	{
		total_cnt++;
		RunPair += RunLength(RunPair);
	}

	if( pn )
	{
		*pn = total_cnt;
	}

	CLUSTER_MAPPING_PAIR *pcmp = NULL;
	if( pClusters )
	{
		pcmp = (CLUSTER_MAPPING_PAIR *)_AllocMemory( (SIZE_T)(total_cnt * sizeof(CLUSTER_MAPPING_PAIR)) );
		if( pcmp == NULL )
		{
			return E_OUTOFMEMORY;
		}
	}

	RunPair = _ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Nonresident.MappingPairsOffset);

	// If it is 0, then break; else extract V and L as previously described.
	int i = 0;
	while( *RunPair != 0 )
	{
		// Set CurrentVcn to NextVcn.
		CurrentVcn = NextVcn;

		cnt = RunCount(RunPair);
		lcn = RunLCN(RunPair);

		// Interpret the next V bytes as a signed quantity,	
		// with the low-order byte first. Unpack it sign-extended into 64 bits and add it to NextVcn.
		NextVcn += cnt;

		// Interpret the next L bytes as a signed quantity, 
		// with the low-order byte first. Unpack it sign-extended into 64 bits and add it to CurrentLcn. 
		// If this produces a CurrentLcn of 0, then the VCNs from CurrentVcn to NextVcn-1 are unallocated.
		CurrentLcn += lcn;

		if( pcmp )
		{
			pcmp[i].CurrentVcn = CurrentVcn;
			pcmp[i].NextVcn = NextVcn;
			pcmp[i].CurrentLcn = lcn ? CurrentLcn : -1;
			pcmp[i].LcnCount = cnt;
		}

		i++;

		// Read the next byte from the stream.
		RunPair += RunLength(RunPair);
	}

	if( pClusters )
		*pClusters = pcmp;

	return S_OK;
}

//-----------------------------------------------------------------------------
//
//  FindNtfsRecordAttribueFromBuffer()
//
//  PURPOSE:
//
//-----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
FindNtfsRecordAttribueFromBuffer(
	NTFS_FILE_RECORD_OUTPUT_BUFFER *pBuf,
	ULONG TypeCode,
	ULONG Index,
	PVOID pBuffer,
	ULONG cbBuffer
	)
{
	HRESULT hr = E_FAIL;

	if( pBuf == NULL )
		return E_INVALIDARG;

	if( pBuffer == NULL )
		return E_INVALIDARG;

	MULTI_SECTOR_HEADER *psechdr = (MULTI_SECTOR_HEADER *)pBuf->FileRecordBuffer;

	ULONG sign = *((PULONG)(psechdr->Signature));

	if( *(INT*)&psechdr->Signature != 'ELIF' ) // 46 49 4c 45 == 0x454C4946 "FILE"
	{
		goto _exit_;
	}

	hr = S_FALSE;

	FILE_RECORD_SEGMENT_HEADER *pfrecSeg = (FILE_RECORD_SEGMENT_HEADER *)psechdr;
	ATTRIBUTE_RECORD_HEADER *pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)_ADDOFFSET(pfrecSeg,pfrecSeg->FirstAttributeOffset);

	while( pAttrRecHdr->TypeCode != $END )
	{
		if( pAttrRecHdr->TypeCode == TypeCode && pAttrRecHdr->Instance == (USHORT)Index )
		{
			if( cbBuffer == 0 )
			{
				*((ATTRIBUTE_RECORD_HEADER **)pBuffer) = pAttrRecHdr;
			}
			else
			{
				if( cbBuffer >= pAttrRecHdr->RecordLength )
				{
					memcpy(pBuffer,pAttrRecHdr,pAttrRecHdr->RecordLength);
					hr = S_OK;
				}
				else
				{
					hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
				}
			}
			break;
		}

		pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->RecordLength);
	}
_exit_:
	return hr;
}

//-----------------------------------------------------------------------------
//
//  FindNtfsRacordAttribue()
//
//  PURPOSE:
//
//-----------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
FindNtfsRacordAttribue(
	HANDLE hVolume,
	LONGLONG FileId,
	ULONG TypeCode,
	ULONG Index,
	PVOID pBuffer,
	ULONG cbBuffer
	)
{
	HRESULT hr = E_FAIL;

	NTFS_FILE_RECORD_OUTPUT_BUFFER *pBuf;
	ULONG Status;
	if( (Status = GetNtfsFileRecordInformation( NULL, hVolume, FileId, (PVOID*)&pBuf, NULL )) == 0 )
	{
		hr = FindNtfsRecordAttribueFromBuffer(pBuf,TypeCode,Index,pBuffer,cbBuffer);
		_FreeMemory(pBuf);
	}
	else
	{
		hr = HRESULT_FROM_WIN32( Status );
	}
	return hr;
}
