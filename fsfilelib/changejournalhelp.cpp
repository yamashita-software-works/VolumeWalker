//***************************************************************************
//*                                                                         *
//*  changejournalhelp.cpp                                                  *
//*                                                                         *
//*  Change Journal Helper                                                  *
//*                                                                         *
//*  Create: 2012.10.26,2024.04.15                                          *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "changejournalhelp.h"   
#include "simplevalarray.h"

// items = (size + block_size - 1) / block_size;
// last data length = size % block_size;
// align = items * block_size;
#define _ULONG_ALIGN(cb)      ((((cb)+sizeof(ULONG)-1)/sizeof(ULONG))*sizeof(ULONG))
#define _ULONGLONG_ALIGN(cb)  ((((cb)+sizeof(ULONGLONG)-1) /sizeof(ULONGLONG))*sizeof(ULONGLONG))
#define _16BYTE_ALIGN(cb)     ((((cb)+(16-1))/16)*16)

#define _BUFFER_SIZE          (_16BYTE_ALIGN(65536 * 16))

class CFSJournalBuffer : public IFSJournalBuffer
{
public:
	CValArray<PVOID> *m_pBufferList;
	LPWSTR m_pszVolumeName;

	USN_JOURNAL_DATA *m_pUsnJournalData;
	ULONG m_cbUsnJournalData;

	SIZE_T m_TotalRecordCount;

public:
	CFSJournalBuffer()
	{
		m_pBufferList = NULL;
		m_pszVolumeName = NULL;
		m_pUsnJournalData = NULL;
		m_cbUsnJournalData = 0;
	}

	~CFSJournalBuffer()
	{
		Free();
		_SafeMemFree(m_pszVolumeName);
	}

	VOID Free()
	{
		if( m_pBufferList != NULL )
		{
			int i;
			for(i = 0; i < m_pBufferList->GetSize(); i++)
			{
				PVOID p = (*m_pBufferList)[i];
				HeapFree(GetProcessHeap(),0,p);
			}

			m_pBufferList->RemoveAll();

			delete m_pBufferList;
			m_pBufferList = NULL;
		}

		if( m_pszVolumeName != NULL )
		{
			_MemFree(m_pszVolumeName);
			m_pszVolumeName = NULL;
		}

		if( m_pUsnJournalData != NULL )
		{
			_MemFree(m_pUsnJournalData);
			m_pUsnJournalData = NULL;
		}
	}

	VOID Release()
	{
		delete this;
	}

	int GetCount()
	{
		return (int)m_pBufferList->GetSize();
	}

	PVOID GetBuffer(int iBufferIndex)
	{
		return (*m_pBufferList)[iBufferIndex];
	}

	SIZE_T GetBufferSize(int iBufferIndex)
	{
		return HeapSize(GetProcessHeap(),0,(*m_pBufferList)[iBufferIndex]);
	}

	LPTSTR GetVolumeName(LPWSTR VolumeName,int cchVolumeName)
	{
		return lstrcpyn(VolumeName,m_pszVolumeName,cchVolumeName);
	}

	void SetVolumeName(LPCWSTR pszVolumeName)
	{
		if( m_pszVolumeName )
			_MemFree(m_pszVolumeName);
		m_pszVolumeName = _MemAllocString(pszVolumeName);
	}

	SIZE_T GetTotalRecordCount() const
	{
		return m_TotalRecordCount;
	}

	BOOL SetJournalData(PVOID ptr, int  cbSize)
	{
		ASSERT( cbSize == sizeof(USN_JOURNAL_DATA) || 
				cbSize == sizeof(USN_JOURNAL_DATA_V1) ||
				cbSize == sizeof(USN_JOURNAL_DATA_V2) ); 

		_SafeMemFree(m_pUsnJournalData);
		m_cbUsnJournalData = 0;

		m_pUsnJournalData = (USN_JOURNAL_DATA *)_MemAllocZero(cbSize);
		if( m_pUsnJournalData )
		{
			memcpy(m_pUsnJournalData,ptr,cbSize);
			m_cbUsnJournalData = cbSize;
			return TRUE;
		}

		return FALSE;
	}

	PVOID GetJournalData() const 
	{
		return m_pUsnJournalData;
	}

	ULONG GetJournalDataLength() const 
	{
		return m_cbUsnJournalData;
	}
};

//----------------------------------------------------------------------------
//
//  CreateJournalBuffer()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C BOOL WINAPI CreateJournalBuffer(PCWSTR pszVolumeName,PVOID *ppData)
{
	HANDLE hVolume;

	USN_JOURNAL_DATA_V1 JournalData;
	READ_USN_JOURNAL_DATA ReadData = {0, 0xFFFFFFFF, FALSE, 0, 0};
	PUSN_RECORD UsnRecord;

	DWORD dwBytes;
	SIZE_T cbRetBytes;

	ULONG cbBufferLength = _BUFFER_SIZE;
	UCHAR *Buffer;
	BOOL bRet;
	DWORD dwResult = 0;
	WCHAR szVolName[MAX_PATH];

	CFSJournalBuffer *pJournalBuffer = NULL;

	// NOTE:
	// This is a NOT root directory.
	if( pszVolumeName[2] == TEXT('?') )
		// "\\?\Volume{xxxx-xxxx-xxxx}"
		lstrcpy(szVolName,pszVolumeName);
	else if( pszVolumeName[1] == L':' )
		// "C:" -> "\\.\C:"
		wsprintf(szVolName,TEXT("\\\\.\\%s"),pszVolumeName);
	else
		lstrcpy(szVolName,pszVolumeName);

	try
	{
		Buffer = (UCHAR*)_MemAlloc( cbBufferLength );
		if( Buffer == NULL )
		{
			throw  ERROR_NOT_ENOUGH_MEMORY;
		}

		pJournalBuffer = new CFSJournalBuffer;
		if( pJournalBuffer == NULL )
		{
			throw  ERROR_NOT_ENOUGH_MEMORY;
		}

		pJournalBuffer->m_pBufferList = new CValArray<PVOID>;
		if( pJournalBuffer->m_pBufferList == NULL )
		{
			throw  ERROR_NOT_ENOUGH_MEMORY;
		}

		pJournalBuffer->SetVolumeName( szVolName );

		hVolume = CreateFile(szVolName, 
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

		if( hVolume == INVALID_HANDLE_VALUE )
		{	
			throw GetLastError();
		}

		if( !DeviceIoControl( hVolume, 
				FSCTL_QUERY_USN_JOURNAL, 
				NULL,
				0,
				&JournalData,
				sizeof(JournalData),
				&dwBytes,
				NULL) )
		{
			throw GetLastError();
		}

		pJournalBuffer->SetJournalData((USN_JOURNAL_DATA*)&JournalData,sizeof(JournalData));

		ReadData.UsnJournalID = JournalData.UsnJournalID;
		ReadData.StartUsn = JournalData.FirstUsn;
#if 0
		ReadData.MaxMajorVersion = JournalData.MaxSupportedMajorVersion;
		ReadData.MinMajorVersion = JournalData.MinSupportedMajorVersion;
#endif
		ULONG cRecords = 0;
		SIZE_T cbTotalRecordSize = 0;

		for(;;)
		{
			memset( Buffer, 0, cbBufferLength );

			if( !DeviceIoControl( hVolume, 
					FSCTL_READ_USN_JOURNAL, 
					&ReadData,sizeof(ReadData),
					Buffer,cbBufferLength,
					&dwBytes,
					NULL) )
			{
				throw GetLastError();
			}

			PVOID p = HeapAlloc( GetProcessHeap(), 0, dwBytes );
			if( p == NULL )
			{
				throw GetLastError();
			}

			RtlCopyMemory(p,Buffer,dwBytes);

			pJournalBuffer->m_pBufferList->Add( p );

			cbRetBytes = dwBytes - sizeof(USN);

			if( cbRetBytes == 0 )
			{
				// Read byte 0, end of data.
				break;
			}

			//
			// Find the first record, point to first element.
			//
			//  +-----+
			//  | USN |
			//  +-------------+ <-- UsnRecord
			//  | USN_RECORD  |
			//  +-------------+
			//  | USN_RECORD  |
			//  ~             ~
			//  +-------------+
			//  | USN_RECORD  |
			//  +-------------+
			//
			UsnRecord = (PUSN_RECORD)(((PUCHAR)Buffer) + sizeof(USN));

			// calc size and count
			while( cbRetBytes > 0 )
			{
				cRecords++;

				cbTotalRecordSize += UsnRecord->RecordLength;

				cbRetBytes -= UsnRecord->RecordLength;
		 
				// Find the next record
				UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord) + UsnRecord->RecordLength);
			}

			// Update starting USN for next call
			ReadData.StartUsn = *(USN *)Buffer; 
		}

		pJournalBuffer->m_TotalRecordCount = (SIZE_T)cRecords;

		bRet = TRUE;
	}
	catch( DWORD err )
	{
		if( pJournalBuffer != NULL )
		{
			DestroyJournalBuffer( pJournalBuffer );
			pJournalBuffer = NULL;
		}

		dwResult = err;

		bRet = FALSE;
	}

	*ppData = pJournalBuffer;

	_MemFree(Buffer);

	if( hVolume != INVALID_HANDLE_VALUE )
		CloseHandle(hVolume);

	SetLastError( dwResult );

	return bRet;
}

//----------------------------------------------------------------------------
//
//  DestroyJournalBuffer()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C BOOL WINAPI DestroyJournalBuffer(PVOID pData)
{
	if( pData == NULL )
		return FALSE;

	IFSJournalBuffer *pJournalBuffer = (IFSJournalBuffer *)pData;

	pJournalBuffer->Release();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////

#pragma pack(1)
typedef struct _SHIM
{
	UCHAR signature[4];
	DWORD length;
	DWORD block_length;
	DWORD actual_length;
} DATA_SHIM;
#pragma pack()

#pragma pack(1)
typedef struct _HEADER
{
	union {
		UCHAR buffer[128];
		struct {
			UCHAR signature[4];
			LARGE_INTEGER creationtime;
		};
	};
} HEADER;
#pragma pack()

BOOL _WriteHeaderData(HANDLE hFile)
{
	HEADER header;

	DWORD cb;
	memset(&header,0,sizeof(header));
	memcpy(header.signature,"CJDF",4);

	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	header.creationtime.HighPart = ft.dwHighDateTime;
	header.creationtime.LowPart = ft.dwLowDateTime;

	return WriteFile(hFile,&header,sizeof(header),&cb,NULL);
}

BOOL _WriteShimData(HANDLE hFile,CHAR *Signature,DWORD cbLength)
{
	DWORD cb;

	DATA_SHIM shim;

	memset(&shim,0,sizeof(shim));
	memcpy(shim.signature,Signature,4);
	shim.length = cbLength;
	shim.block_length = _16BYTE_ALIGN(cbLength);

	return WriteFile(hFile,&shim,sizeof(shim),&cb,NULL);
}

BOOL _WriteUsnJournalData(HANDLE hFile,USN_JOURNAL_DATA *UsnReadData,DWORD cbUsnReadData)
{
	DWORD cb;
	DWORD length = _16BYTE_ALIGN(cbUsnReadData);

	PBYTE pb = (PBYTE)_alloca( length );

	memset(pb,0,length);
	memcpy(pb,UsnReadData,cbUsnReadData);

	return WriteFile(hFile,pb,length,&cb,NULL);
}

BOOL _WriteBlockData(HANDLE hFile,PVOID ptr,DWORD cbLength)
{
	DWORD cb;
	DWORD cbBlockLength = _16BYTE_ALIGN(cbLength);
	return WriteFile(hFile,ptr,cbBlockLength,&cb,NULL);
}

//----------------------------------------------------------------------------
//
//  MakeChangeJournalDumpFile()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
#if 0
EXTERN_C BOOL WINAPI MakeChangeJournalDumpFile(PWTSTR pszVolumeName,PCWSTR pszFileName,PVOID *ppData)
{
	HANDLE hVol;

	USN_JOURNAL_DATA JournalData;
	READ_USN_JOURNAL_DATA ReadData = {0, 0xFFFFFFFF, FALSE, 0, 0};
	PUSN_RECORD UsnRecord;

	DWORD dwBytes;
	SIZE_T cbRetBytes;

	ULONG cbBufferLength = _BUFFER_SIZE;
	UCHAR *Buffer;
	BOOL bRet;
	DWORD dwResult = 0;

	WCHAR szVolName[MAX_PATH];

	DWORD dwCreationDisposition = CREATE_ALWAYS;
	HANDLE hDumpFile;
	hDumpFile = CreateFile(pszFileName,GENERIC_WRITE,0,NULL,dwCreationDisposition,FILE_ATTRIBUTE_NORMAL,NULL);
	if( hDumpFile == INVALID_HANDLE_VALUE )
	{
		return FALSE;
	}

	_WriteHeaderData(hDumpFile);

	Buffer = (UCHAR*)_MemAlloc( cbBufferLength );
	if( Buffer == NULL )
	{
		CloseHandle(hDumpFile);
		SetLastError( ERROR_NOT_ENOUGH_MEMORY );
		return FALSE;
	}

	if( pszVolumeName[2] == TEXT('?') )
		// "\\?\Volume{xxxx-xxxx-xxxx}"
		lstrcpy(szVolName,pszVolumeName);
	else if( pszVolumeName[1] == L':' )
		// "C:" -> "\\.\C:"
		wsprintf(szVolName,TEXT("\\\\.\\%s"),pszVolumeName);
	else
		lstrcpy(szVolName,pszVolumeName);

	try
	{
		hVol = CreateFile(szVolName, 
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

		if( hVol == INVALID_HANDLE_VALUE )
		{	
			throw GetLastError();
		}

		if( !DeviceIoControl( hVol, 
				FSCTL_QUERY_USN_JOURNAL, 
				NULL,
				0,
				&JournalData,
				sizeof(JournalData),
				&dwBytes,
				NULL) )
		{
			throw GetLastError();
		}

		ReadData.UsnJournalID = JournalData.UsnJournalID;
		ReadData.StartUsn = JournalData.FirstUsn;

		_WriteShimData(hDumpFile,"CJUD",dwBytes);

		_WriteUsnJournalData(hDumpFile,&JournalData);

		ULONG cRecords = 0;
		SIZE_T cbTotalRecordSize = 0;

		for(;;)
		{
			memset( Buffer, 0, cbBufferLength );
			dwBytes = 0;

			if( !DeviceIoControl( hVol, 
					FSCTL_READ_USN_JOURNAL, 
					&ReadData,sizeof(ReadData),
					Buffer,cbBufferLength,
					&dwBytes,
					NULL) )
			{
				throw GetLastError();
			}

			if( dwBytes == 0 || dwBytes < sizeof(USN) )
			{
				throw GetLastError();
			}

			//
			// Substruct USN's variable size from read data size.
			//
			if( (dwBytes - sizeof(USN)) == 0 )
			{
				//
				// Only USN, without trailing data-block.
				//
				break;
			}

			// Skip USN value
			cbRetBytes = dwBytes - sizeof(USN);
			UsnRecord = (PUSN_RECORD)(((PUCHAR)Buffer) + sizeof(USN));

			while( cbRetBytes > 0 )
			{
				cRecords++;

				cbTotalRecordSize += UsnRecord->RecordLength;

				cbRetBytes -= UsnRecord->RecordLength;
		 
				UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord) + UsnRecord->RecordLength);
			}

			_WriteShimData(hDumpFile,"CJUR",dwBytes);

			_WriteBlockData(hDumpFile,Buffer,dwBytes);

			//
			// Update starting USN for next call
			//
			ReadData.StartUsn = *(USN *)Buffer; 
		}

		bRet = TRUE;
	}
	catch( DWORD err )
	{
		dwResult = err;

		bRet = FALSE;
	}

	_MemFree(Buffer);

	CloseHandle(hDumpFile);

	SetLastError( dwResult );

	return bRet;
}
#else
EXTERN_C BOOL WINAPI MakeChangeJournalDumpFile(LPCWSTR pszVolumeName,LPCWSTR pszFileName,PVOID *ppData)
{
	return MakeChangeJournalDumpFileEx(pszVolumeName,pszFileName,ppData);
}
#endif

//----------------------------------------------------------------------------
//
//  MakeChangeJournalDumpFileEx()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C BOOL WINAPI MakeChangeJournalDumpFileEx(PCWSTR pszVolumeName,PCWSTR pszFileName,PVOID *ppData)
{
	HANDLE hVol;

	USN_JOURNAL_DATA_V1 JournalData;
	READ_USN_JOURNAL_DATA_V1 ReadData = {0, 0xFFFFFFFF, FALSE, 0, 0};
	PUSN_RECORD_V3 UsnRecord;

	DWORD dwBytes;
	SIZE_T cbRetBytes;

	ULONG cbBufferLength = _BUFFER_SIZE;
	UCHAR *Buffer;
	BOOL bRet;
	DWORD dwResult = 0;

	WCHAR szVolName[MAX_PATH];

	DWORD dwCreationDisposition = CREATE_ALWAYS;
	HANDLE hDumpFile;
	hDumpFile = CreateFile(pszFileName,GENERIC_WRITE,0,NULL,dwCreationDisposition,FILE_ATTRIBUTE_NORMAL,NULL);
	if( hDumpFile == INVALID_HANDLE_VALUE )
	{
		return FALSE;
	}

	_WriteHeaderData(hDumpFile);

	Buffer = (UCHAR*)_MemAlloc( cbBufferLength );
	if( Buffer == NULL )
	{
		CloseHandle(hDumpFile);
		SetLastError( ERROR_NOT_ENOUGH_MEMORY );
		return FALSE;
	}

	if( pszVolumeName[2] == TEXT('?') )
		lstrcpy(szVolName,pszVolumeName);
	else if( pszVolumeName[1] == L':' )
		wsprintf(szVolName,TEXT("\\\\.\\%s"),pszVolumeName);
	else
		lstrcpy(szVolName,pszVolumeName);

	try
	{
		hVol = CreateFile(szVolName, 
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

		if( hVol == INVALID_HANDLE_VALUE )
		{	
			throw GetLastError();
		}

		if( !DeviceIoControl( hVol, 
				FSCTL_QUERY_USN_JOURNAL, 
				NULL,
				0,
				&JournalData,
				sizeof(JournalData),
				&dwBytes,
				NULL) )
		{
			throw GetLastError();
		}

		ReadData.UsnJournalID = JournalData.UsnJournalID;
		ReadData.StartUsn = JournalData.FirstUsn;
		ReadData.MaxMajorVersion = JournalData.MaxSupportedMajorVersion;
		ReadData.MinMajorVersion = JournalData.MinSupportedMajorVersion;

		_WriteShimData(hDumpFile,"CJUD",dwBytes);
		_WriteUsnJournalData(hDumpFile,(USN_JOURNAL_DATA*)&JournalData,sizeof(JournalData));

		ULONG cRecords = 0;
		SIZE_T cbTotalRecordSize = 0;

		for(;;)
		{
			memset( Buffer, 0, cbBufferLength );
			dwBytes = 0;

			if( !DeviceIoControl( hVol, 
					FSCTL_READ_USN_JOURNAL, 
					&ReadData,sizeof(ReadData),
					Buffer,cbBufferLength,
					&dwBytes,
					NULL) )
			{
				throw GetLastError();
			}

			if( dwBytes == 0 || dwBytes < sizeof(USN) )
			{
				throw GetLastError();
			}

			//
			// Substruct USN's variable size from read data size.
			//
			if( (dwBytes - sizeof(USN)) == 0 )
			{
				//
				// Only USN, without trailing data-block.
				//
				break;
			}

			// Skip USN
			cbRetBytes = dwBytes - sizeof(USN);
			UsnRecord = (PUSN_RECORD_V3)(((PUCHAR)Buffer) + sizeof(USN));

			// 
			while( cbRetBytes > 0 )
			{
				cRecords++;

				cbTotalRecordSize += UsnRecord->RecordLength;

				cbRetBytes -= UsnRecord->RecordLength;
		 
				UsnRecord = (PUSN_RECORD_V3)(((PCHAR)UsnRecord) + UsnRecord->RecordLength);
			}

			_WriteShimData(hDumpFile,"CJUR",dwBytes);

			_WriteBlockData(hDumpFile,Buffer,dwBytes);

			//
			// Update starting USN for next call
			//
			ReadData.StartUsn = *(USN *)Buffer; 
		}

		bRet = TRUE;
	}
	catch( DWORD err )
	{
		dwResult = err;

		bRet = FALSE;
	}

	_MemFree(Buffer);

	CloseHandle(hDumpFile);

	SetLastError( dwResult );

	return bRet;
}

//----------------------------------------------------------------------------
//
//  SaveChangeJournalDumpFile()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C BOOL WINAPI SaveChangeJournalDumpFile(IFSJournalBuffer *pJournalBuffer,PCWSTR pszFileName)
{
	DWORD dwBytes;
	SIZE_T cbRetBytes;

	ULONG cbBufferLength = _BUFFER_SIZE;
	UCHAR *Buffer;
	BOOL bRet;
	DWORD dwResult = 0;

	DWORD dwCreationDisposition = CREATE_ALWAYS;
	HANDLE hDumpFile;
	hDumpFile = CreateFile(pszFileName,GENERIC_WRITE,0,NULL,dwCreationDisposition,FILE_ATTRIBUTE_NORMAL,NULL);
	if( hDumpFile == INVALID_HANDLE_VALUE )
	{
		return FALSE;
	}

	try
	{
		//
		// Write Header
		//
		_WriteHeaderData(hDumpFile);

		Buffer = (UCHAR*)_MemAlloc( cbBufferLength ); 
		if( Buffer == NULL )
		{
			throw ERROR_NOT_ENOUGH_MEMORY;
		}

		//
		// Write USN_JOURNAL_DATA
		//
		USN_JOURNAL_DATA_V1 JournalDataBuffer = {0};
		USN_JOURNAL_DATA_V1 *JournalData = &JournalDataBuffer;
		DWORD cbJournalData = sizeof(JournalDataBuffer);

		if( pJournalBuffer->GetJournalData() != NULL )
		{
			JournalData = (USN_JOURNAL_DATA_V1 *)pJournalBuffer->GetJournalData();
			cbJournalData = pJournalBuffer->GetJournalDataLength();
		}

		_WriteShimData(hDumpFile,"CJUD",cbJournalData);
		_WriteUsnJournalData(hDumpFile,(USN_JOURNAL_DATA*)JournalData,cbJournalData);

		//
		// Write USN_RECORDs
		//
		ULONG cRecords = 0;
		SIZE_T cbTotalRecordSize = 0;
		PUSN_RECORD_V3 UsnRecord;

		for(int index = 0; index < pJournalBuffer->GetCount(); index++)
		{
			memset( Buffer, 0, cbBufferLength );

			dwBytes = (DWORD)pJournalBuffer->GetBufferSize(index);

			if( dwBytes == 0 || dwBytes < sizeof(USN) )
			{
				throw GetLastError();
			}

			memcpy(Buffer,pJournalBuffer->GetBuffer(index),dwBytes);

			//
			// Substruct USN's variable size from read data size.
			//
			if( (dwBytes - sizeof(USN)) == 0 )
			{
				//
				// Only USN, without trailing data-block.
				//
				break;
			}

			// Skip USN
			cbRetBytes = dwBytes - sizeof(USN);
			UsnRecord = (PUSN_RECORD_V3)(((PUCHAR)Buffer) + sizeof(USN));

			while( cbRetBytes > 0 )
			{
				cRecords++;

				cbTotalRecordSize += UsnRecord->RecordLength;

				cbRetBytes -= UsnRecord->RecordLength;
		 
				UsnRecord = (PUSN_RECORD_V3)(((PCHAR)UsnRecord) + UsnRecord->RecordLength);
			}

			_WriteShimData(hDumpFile,"CJUR",dwBytes);

			_WriteBlockData(hDumpFile,Buffer,dwBytes);
		}

		bRet = TRUE;
	}
	catch( DWORD err )
	{
		dwResult = err;

		bRet = FALSE;
	}

	_SafeMemFree(Buffer);

	CloseHandle(hDumpFile);

	SetLastError( dwResult );

	return bRet;
}

//----------------------------------------------------------------------------
//
//  ReadChangeJournalDumpFile()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C BOOL WINAPI ReadChangeJournalDumpFile(PCWSTR pszFileName,PVOID *ppData)
{
	HANDLE hVol;

	PUSN_RECORD UsnRecord;

	DWORD dwBytes;
	LONG cbRetBytes;

	ULONG cbBufferLength = _BUFFER_SIZE;
	UCHAR *Buffer;
	BOOL bRet;
	DWORD dwResult = 0;

	CFSJournalBuffer *pJournalBuffer = NULL;

	WCHAR szFileName[MAX_PATH];

	// NOTE:
	// This is a NOT root directory.
	if( pszFileName[2] == TEXT('?') )
		// "\\?\Volume{xxxx-xxxx-xxxx}"
		lstrcpy(szFileName,pszFileName);
	else if( pszFileName[1] == L':' )
		// "C:" -> "\\.\C:"
		wsprintf(szFileName,TEXT("\\\\.\\%s"),pszFileName);
	else
	{
		lstrcpy(szFileName,pszFileName);
	}

	try
	{
		Buffer = (UCHAR*)_MemAlloc( cbBufferLength );
		if( Buffer == NULL )
		{
			throw ERROR_NOT_ENOUGH_MEMORY;
		}

		pJournalBuffer = new CFSJournalBuffer;
		if( pJournalBuffer == NULL )
		{
			throw ERROR_NOT_ENOUGH_MEMORY;
		}

		pJournalBuffer->m_pBufferList = new CValArray<PVOID>;
		if( pJournalBuffer->m_pBufferList == NULL )
		{
			throw  ERROR_NOT_ENOUGH_MEMORY;
		}

		pJournalBuffer->SetVolumeName( szFileName );

		hVol = CreateFile(szFileName, 
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

		if( hVol == INVALID_HANDLE_VALUE )
		{	
			throw (long)GetLastError();
		}

		DWORD cbFileSize;
		cbFileSize = GetFileSize(hVol,NULL);

		CHAR hdr[128];
		DWORD cb;

		// Header
		ReadFile(hVol,hdr,sizeof(hdr),&cb,NULL);
		if( cb == 0 )
			throw ERROR_INVALID_DATA;

		// Read shim data signature "CJUD"
		DATA_SHIM shim;
		ReadFile(hVol,&shim,sizeof(shim),&cb,NULL);
		if( cb < sizeof(DATA_SHIM) || memcmp(shim.signature,"CJUD",4) != 0 )
			throw ERROR_INVALID_DATA;

		// Read data
		DWORD rd = _16BYTE_ALIGN(shim.length);

		if( rd == 0 )
			throw ERROR_INVALID_DATA;

		if( !ReadFile(hVol,Buffer,rd,&dwBytes,NULL) )
			throw ERROR_INVALID_DATA;

		if( dwBytes < rd )
			throw ERROR_INVALID_DATA;

		if( shim.actual_length == 0 )
			shim.actual_length = sizeof(USN_JOURNAL_DATA);
		pJournalBuffer->SetJournalData((USN_JOURNAL_DATA*)Buffer,shim.actual_length);

		ULONG cRecords = 0;
		SIZE_T cbTotalRecordSize = 0;

		pJournalBuffer->m_TotalRecordCount = 0;

		for(;;)
		{
			memset( Buffer, 0, cbBufferLength );

			// Read shim data "CJUR"
			DATA_SHIM shim;
			ReadFile(hVol,&shim,sizeof(shim),&cb,NULL);

			if( cb == 0 )
				break; // End of File

			// signature "CJUR"
			if( cb < sizeof(shim) || memcmp(shim.signature,"CJUR",4) != 0 )
				throw ERROR_INVALID_DATA;

			// Read data
			DWORD rd = _16BYTE_ALIGN(shim.length);

			if( rd == 0 )
				break;

			ReadFile(hVol,Buffer,rd,&dwBytes,NULL);

			if( dwBytes == 0 )
				throw ERROR_INVALID_DATA;

			dwBytes = shim.length;

			PVOID p = HeapAlloc( GetProcessHeap(), 0, dwBytes );
			if( p == NULL )
			{
				throw (long)GetLastError();
			}

			RtlCopyMemory(p,Buffer,dwBytes);

			pJournalBuffer->m_pBufferList->Add( p );

			cbRetBytes = shim.length - sizeof(USN);

			if( cbRetBytes == 0 )
			{
				// Read byte 0, end of data.
				break;
			}

			UsnRecord = (PUSN_RECORD)(((PUCHAR)Buffer) + sizeof(USN));

			// calc size and count
			while( cbRetBytes > 0 )
			{
				cRecords++;

				cbTotalRecordSize += UsnRecord->RecordLength;

				cbRetBytes -= UsnRecord->RecordLength;
		 
				// Find the next record
				UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord) + UsnRecord->RecordLength);
			}
		}

		pJournalBuffer->m_TotalRecordCount = (SIZE_T)cRecords;

		bRet = TRUE;
	}
	catch( long err )
	{
		if( pJournalBuffer != NULL )
		{
			DestroyJournalBuffer( pJournalBuffer );
			pJournalBuffer = NULL;
		}

		dwResult = err;

		bRet = FALSE;
	}

	*ppData = pJournalBuffer;

	_MemFree(Buffer);

	SetLastError( dwResult );

	return bRet;
}

//----------------------------------------------------------------------------
//
//  QueryJournalInformation()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C BOOL WINAPI QueryJournalInformation(PCWSTR pszVolumeName,FS_USN_JOURNAL_DATA *pInfo)
{
	HANDLE hVol;
	USN_JOURNAL_DATA JournalData;
	BOOL bRet;
	DWORD dwResult = 0;
	DWORD dwBytes;
	WCHAR szVolName[MAX_PATH];

	if( StringCchCopy(szVolName,MAX_PATH,pszVolumeName) != S_OK )
		return FALSE;

	try
	{
		hVol = CreateFile(szVolName, 
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

		if( hVol == INVALID_HANDLE_VALUE )
		{	
			throw GetLastError();
		}


		if( !DeviceIoControl( hVol, 
				FSCTL_QUERY_USN_JOURNAL, 
				NULL,
				0,
				&JournalData,
				sizeof(JournalData),
				&dwBytes,
				NULL) )
		{
			throw GetLastError();
		}

		RtlCopyMemory(pInfo,&JournalData,sizeof(FS_USN_JOURNAL_DATA));

		bRet = TRUE;
	}
	catch( DWORD err )
	{
		dwResult = err;

		bRet = FALSE;
	}

	SetLastError( dwResult );

	return bRet;
}

//----------------------------------------------------------------------------
//
//  GetJournalReasonText()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
typedef struct _FS_USN_REASON_INFO {
	ULONG Reason;
	const char *ReasonName;
	const wchar_t *ReasonDescription;
	const wchar_t *Abbreviations;
} FS_USN_REASON_INFO;

#ifndef USN_REASON_INTEGRITY_CHANGE
#define USN_REASON_INTEGRITY_CHANGE      (0x00800000)
#endif
#ifndef USN_REASON_DESIRED_STORAGE_CLASS_CHANGE
#define USN_REASON_DESIRED_STORAGE_CLASS_CHANGE (0x01000000)
#endif

#define _DEF_REASON_INFO(name,desc,abbr) { name, #name, L##desc, L##abbr }

static FS_USN_REASON_INFO _infoTable[] = {
	// 0x00000100 - The file or directory is created for the first time.
	_DEF_REASON_INFO( USN_REASON_FILE_CREATE, "Create" , "CR" ),
	
	// 0x00008000 - A user has either changed one or more file or directory attributes
	// (for example, the read-only, hidden, system, archive, or sparse attribute), 
	// or one or more time stamps.
	_DEF_REASON_INFO( USN_REASON_BASIC_INFO_CHANGE, "Information Change", "IC" ),
	
	// 0x00000002 - The file or directory is extended (added to).
	_DEF_REASON_INFO( USN_REASON_DATA_EXTEND, "Extend", "EX" ),
	
	// 0x00000001 - The data in the file or directory is overwritten.
	_DEF_REASON_INFO( USN_REASON_DATA_OVERWRITE, "Overwrite", "OW" ),
	
	// 0x00000004 - The file or directory is truncated.
	_DEF_REASON_INFO( USN_REASON_DATA_TRUNCATION, "Truncation", "TR" ),
	
	// 0x00000400 - The user made a change to the extended attributes of a file or directory. 
	//              These NTFS file system attributes are not accessible to Windows-based applications.
	_DEF_REASON_INFO( USN_REASON_EA_CHANGE, "EA Change", "EA" ),
	
	// 0x00040000 - The file or directory is encrypted or decrypted.
	_DEF_REASON_INFO( USN_REASON_ENCRYPTION_CHANGE, "Encryption Change", "EC" ),
	
	// 0x00000200 - The file or directory is deleted.
	_DEF_REASON_INFO( USN_REASON_FILE_DELETE, "Delete", "DL" ),
	
	// 0x00020000 - The compression state of the file or directory is changed from or to compressed.
	_DEF_REASON_INFO( USN_REASON_COMPRESSION_CHANGE, "Compression Change", "CC" ),
	
	// 0x00010000 - An NTFS file system hard link is added to or removed from the file or directory. 
	//              An NTFS file system hard link, similar to a POSIX hard link, is one of several
    //              directory entries that see the same file or directory.
	_DEF_REASON_INFO( USN_REASON_HARD_LINK_CHANGE, "HardLink Change", "HC" ),
	
	// 0x00004000 - A user changes the FILE_ATTRIBUTE_NOT_CONTENT_INDEXED attribute. 
	_DEF_REASON_INFO( USN_REASON_INDEXABLE_CHANGE, "Indexable Change", "IC" ),
	
	// 0x00000020 - The one or more named data streams for a file are extended (added to).
	_DEF_REASON_INFO( USN_REASON_NAMED_DATA_EXTEND, "AltStreamData Extend", "AE" ),
	
	// 0x00000010 - The data in one or more named data streams for a file is overwritten.
	_DEF_REASON_INFO( USN_REASON_NAMED_DATA_OVERWRITE, "AltStreamData Overwrite", "AO" ),
	
	// 0x00000040 - The one or more named data streams for a file is truncated.
	_DEF_REASON_INFO( USN_REASON_NAMED_DATA_TRUNCATION, "AltStreamData Truncation", "AT" ),
	
	// 0x00080000 - The object identifier of a file or directory is changed.
	_DEF_REASON_INFO( USN_REASON_OBJECT_ID_CHANGE, "ObjectID Change", "OC" ),
	
	// 0x00002000 - A file or directory is renamed, and the file name in the USN_RECORD 
	//              structure is the new name.
	_DEF_REASON_INFO( USN_REASON_RENAME_NEW_NAME, "Rename NewName", "RN" ),
	
	// 0x00001000 - The file or directory is renamed, and the file name in the USN_RECORD 
	//              structure is the previous name.
	_DEF_REASON_INFO( USN_REASON_RENAME_OLD_NAME, "Rename OldName", "RO" ),
	
	// 0x00100000 - The reparse point that is contained in a file or directory is changed, 
	//              or a reparse point is added to or deleted from a file or directory.
	_DEF_REASON_INFO( USN_REASON_REPARSE_POINT_CHANGE, "Reparse Point Change", "RC" ),
	
	// 0x00000800 - A change is made in the access rights to a file or directory.
	_DEF_REASON_INFO( USN_REASON_SECURITY_CHANGE, "Security Change", "SC" ),
	
	// 0x00200000- A named stream is added to or removed from a file, or a named stream is renamed.
	_DEF_REASON_INFO( USN_REASON_STREAM_CHANGE, "AltStream Change", "AC" ),
	
	// 0x00400000 - 
	_DEF_REASON_INFO( USN_REASON_TRANSACTED_CHANGE, "Transacted Change", "TC" ),
	
	// 0x00800000 - 
	_DEF_REASON_INFO( USN_REASON_INTEGRITY_CHANGE, "Integrity Change", "ITC" ),
	
	// 0x01000000 - 
	_DEF_REASON_INFO( USN_REASON_DESIRED_STORAGE_CLASS_CHANGE, "Desired Storage Class Change", "DSCC" ),
	
	// 0x80000000 - The file or directory is closed.
	_DEF_REASON_INFO( USN_REASON_CLOSE, "Close", "CL" ),
};

/*++

SourceInfo 

USN_SOURCE_AUXILIARY_DATA
0x00000002 The operation adds a private data stream to a file or directory. 

An example might be a virus detector adding checksum information. 
As the virus detector modifies the item, the system generates USN records. 
USN_SOURCE_AUXILIARY_DATA indicates that the modifications did not change
 the application data.
 
USN_SOURCE_DATA_MANAGEMENT
0x00000001 The operation provides information about a change to the file
or directory made by the operating system. 

A typical use is when the Remote Storage system moves data from external to
local storage. Remote Storage is the hierarchical storage management software.
Such a move usually at a minimum adds the USN_REASON_DATA_OVERWRITE flag to
a USN record. However, the data has not changed from the user's point of view. 
By noting USN_SOURCE_DATA_MANAGEMENT in the SourceInfo member, you can determine
that although a write operation is performed on the item, data has not changed.
 
USN_SOURCE_REPLICATION_MANAGEMENT
0x00000004 The operation is modifying a file to match the contents of the same 
file which exists in another member of the replica set.

--*/ 

//----------------------------------------------------------------------------
//
//  GetJournalReasonText()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C ULONG WINAPI GetJournalReasonText(ULONG Reason,LPWSTR Text,ULONG cchText)
{
	int i;
	*Text = L'\0';
	for(i = 0; i < _countof(_infoTable); i++)
	{
		if( _infoTable[i].Reason & Reason )
		{
			StringCchCat(Text,cchText,_infoTable[i].ReasonDescription);
			StringCchCat(Text,cchText,L",");
		}
	}
	int len;
	len = (int)wcslen(Text);
	if( Text[len-1] == L',' )
		Text[len-1] = L'\0';
	return len;
}

//----------------------------------------------------------------------------
//
//  GetJournalReasonAbbreviationsText()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C ULONG WINAPI GetJournalReasonAbbreviationsText(ULONG Reason,LPWSTR Text,ULONG cchText,BOOL bBrackets)
{
	int i;
	*Text = L'\0';
	for(i = 0; i < _countof(_infoTable); i++)
	{
		if( _infoTable[i].Reason & Reason )
		{
			if( bBrackets )
				wcscat(Text,L"(");
			wcscat(Text,_infoTable[i].Abbreviations);
			if( bBrackets )
				wcscat(Text,L")");
			wcscat(Text,L",");
		}
	}
	int len;
	len = (int)wcslen(Text);
	if( Text[len-1] == L',' )
		Text[len-1] = L'\0';
	return len;
}

//----------------------------------------------------------------------------
//
//  GetFileUsn()
//
//  PURPOSE:
//
//----------------------------------------------------------------------------
EXTERN_C ULONG WINAPI GetFileUsn(HANDLE hFileHandle,FS_FILE_USN_INFORMATION *pUsnInfo)
{
	BOOL bSuccess;
	PVOID InputBuffer = NULL;
	DWORD cbInputBuffer = 0;
	LONG ErrorCode;
	DWORD cb;

#if 0
	// 128bit FileId
	READ_FILE_USN_DATA rfud = {0};
	rfud.MinMajorVersion = 2;
	rfud.MaxMajorVersion = 3;
	InputBuffer = &rfud;
	cbInputBuffer = sizeof(READ_FILE_USN_DATA);
#endif

	DWORD cbUsnRecordBuffer = sizeof(USN_RECORD) + (sizeof(WCHAR) * MAX_PATH);

	HANDLE hHeap = GetProcessHeap();
	USN_RECORD *UsnRecordBuffer = (USN_RECORD *)HeapAlloc(hHeap,HEAP_ZERO_MEMORY,cbUsnRecordBuffer);

	if( UsnRecordBuffer == NULL )
	{
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	bSuccess = DeviceIoControl(hFileHandle, 
					FSCTL_READ_FILE_USN_DATA, 
					InputBuffer,cbInputBuffer,
					UsnRecordBuffer,cbUsnRecordBuffer,
					&cb,
					NULL);

	if( bSuccess )
	{
		pUsnInfo->Usn = UsnRecordBuffer->Usn;
		ErrorCode = ERROR_SUCCESS;
	}
	else
	{
		ErrorCode = GetLastError();
	}

	HeapFree(hHeap,0,UsnRecordBuffer);

	return ErrorCode;
}
