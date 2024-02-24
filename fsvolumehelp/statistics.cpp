//***************************************************************************
//*                                                                         *
//*  statistics.cpp                                                         *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2024-01-23                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "volumehelp.h"

#ifdef _DEBUG
#ifndef _DEBUG_CHECK_STRUCTURE_MEMBER
#define _DEBUG_CHECK_STRUCTURE_MEMBER   0 
#endif
#endif

#ifdef _DEBUG
extern void MakeTestDataNtfs(PNTFS_STATISTICS pa);
extern void MakeTestDataNtfsEx(PNTFS_STATISTICS_EX pa);
#endif

void MakeStatisticsEx(
	PFILESYSTEM_STATISTICS_EX pex,
	PFILESYSTEM_STATISTICS *pa,
	int cpa
	);

//----------------------------------------------------------------------------
//
//  FreeStatisticsData()
//
//----------------------------------------------------------------------------
HRESULT
WINAPI
FreeStatisticsData(
	PBYTE Buffer,
	PFILESYSTEM_STATISTICS *StatisticsPtrArray,
	PFILESYSTEM_STATISTICS_EX StatisticsEx
	)
{
	if( Buffer )
		_MemFree(Buffer);
	if( StatisticsPtrArray )
		_MemFree(StatisticsPtrArray);
	if( StatisticsEx )
		_MemFree(StatisticsEx);
	return S_OK;
}

//----------------------------------------------------------------------------
//
//  GetStatisticsData()
//
//----------------------------------------------------------------------------
HRESULT
WINAPI
GetStatisticsData(
	PCWSTR pszVolumeRoot,
	PBYTE *Buffer,
	SIZE_T *BufferSize,
	PFILESYSTEM_STATISTICS **StatisticsPtrArray,
	SIZE_T *StatisticsPtrArraySize,
	PULONG StatisticsPtrArrayCount,
	PFILESYSTEM_STATISTICS_EX *StatisticsEx,
	SIZE_T *StatisticsExSize
	)
{
	HANDLE hVolumeRoot;
	WCHAR szVolumeRoot[MAX_PATH];
	WCHAR FileSystemName[MAX_PATH+1];
	DWORD FileSystemFlags;

	PBYTE pBuffer = NULL; // pointer to FILESYSTEM_STATISTICS buffer
	SIZE_T cbBufferSize = 0;
	PFILESYSTEM_STATISTICS *PtrArray = NULL;
	DWORD cbPtrArray = 0;

	FILESYSTEM_STATISTICS_EX *pBufferEx = NULL;
	SIZE_T cbBufferSizeEx = 0;

	HRESULT hr;

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	__try
	{
		BOOL bSuccess;
		DWORD cbBytesReturned;

		StringCchPrintf(szVolumeRoot,MAX_PATH,L"\\\\?\\GlobalRoot%s\\",pszVolumeRoot);

		hVolumeRoot = CreateFile(szVolumeRoot,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);

		if( hVolumeRoot == INVALID_HANDLE_VALUE )
		{
			__leave;
		}

		GetVolumeInformationByHandleW(hVolumeRoot,NULL,0,NULL,NULL,&FileSystemFlags,FileSystemName,ARRAYSIZE(FileSystemName));

		cbBufferSize   = sizeof(FILESYSTEM_STATISTICS);
		cbBufferSizeEx = sizeof(FILESYSTEM_STATISTICS_EX);

		if( wcsicmp(FileSystemName,L"NTFS") == 0 )
		{
			cbBufferSize   += sizeof(NTFS_STATISTICS);
			cbBufferSizeEx += sizeof(NTFS_STATISTICS_EX);
		}
		else if( wcsicmp(FileSystemName,L"FAT") == 0 )
		{
			cbBufferSize   += sizeof(FAT_STATISTICS);
			cbBufferSizeEx += sizeof(FAT_STATISTICS);   // _EX structure not present.
		}
		else if( wcsicmp(FileSystemName,L"FAT32") == 0 )
		{
			cbBufferSize   += sizeof(FAT_STATISTICS);
			cbBufferSizeEx += sizeof(FAT_STATISTICS);   // _EX structure not present.
		}
		else if( wcsicmp(FileSystemName,L"exFAT") == 0 )
		{
			cbBufferSize   += sizeof(EXFAT_STATISTICS);
			cbBufferSizeEx += sizeof(EXFAT_STATISTICS); // _EX structure not present.
		}
		else if( wcsicmp(FileSystemName,L"Refs") == 0 )
		{
			cbBufferSize   += sizeof(NTFS_STATISTICS);    // Uses same as NTFS.
			cbBufferSizeEx += sizeof(NTFS_STATISTICS_EX); // Uses same as NTFS.
		}

		//
		// Calc a FILESYSTEM_STATISTICS/FILESYSTEM_STATISTICS_EX total buffer size
		//
		cbBufferSize = ((cbBufferSize+64)/64)*64;     // 64 byte align buffer size
		cbBufferSizeEx = ((cbBufferSizeEx+64)/64)*64; // 64 byte align buffer size

		//
		// Allocate FILESYSTEM_STATISTICS, each CPUs
		//
		cbBufferSize *= si.dwNumberOfProcessors;

		pBuffer = (PBYTE)_MemAllocZero(cbBufferSize);
		if( pBuffer == NULL )
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			__leave;
		}

		bSuccess = DeviceIoControl(hVolumeRoot,
				FSCTL_FILESYSTEM_GET_STATISTICS,
				NULL,0,pBuffer,(DWORD)cbBufferSize,&cbBytesReturned,NULL);

		if( bSuccess == FALSE )
		{
			__leave;
		}

		//
		// Alloc pointer array
		//
		if( StatisticsPtrArray )
		{
			cbPtrArray = si.dwNumberOfProcessors * sizeof(PBYTE);
			PtrArray = (PFILESYSTEM_STATISTICS*)_MemAllocZero( cbPtrArray );
			if( PtrArray == NULL )
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				__leave;
			}

			//
			// Set pointer to beginning of data that each CPU.
			//
			DWORD i;
			FILESYSTEM_STATISTICS *pf = (FILESYSTEM_STATISTICS *)pBuffer;
			for(i = 0; i < si.dwNumberOfProcessors; i++)
			{
#if _DEBUG_CHECK_STRUCTURE_MEMBER
				MakeTestDataNtfs( GetNTFSStatistics(pf) );
#endif
				PtrArray[i] = pf;

				pf = (FILESYSTEM_STATISTICS *)((PBYTE)pf + pf->SizeOfCompleteStructure);
			}
		}

		//
		// Get FSCTL_FILESYSTEM_GET_STATISTICS_EX data
		//
		if( StatisticsEx )
		{
			pBufferEx = (FILESYSTEM_STATISTICS_EX *)_MemAllocZero(cbBufferSizeEx);
			if( pBufferEx == NULL )
			{
				SetLastError(ERROR_NOT_ENOUGH_MEMORY);
				__leave;
			}

			bSuccess = DeviceIoControl(hVolumeRoot,
							FSCTL_FILESYSTEM_GET_STATISTICS_EX,
							NULL,0,pBufferEx,(DWORD)cbBufferSizeEx,&cbBytesReturned,NULL);
#if 0
			if( bSuccess == FALSE )
			{
				__leave;
			}
#else
			if( bSuccess == FALSE )
			{
				// Simulate FILESYSTEM_STATISTICS_EX
				MakeStatisticsEx(pBufferEx,PtrArray,si.dwNumberOfProcessors);
				SetLastError(ERROR_SUCCESS);
			}
#endif

#if _DEBUG_CHECK_STRUCTURE_MEMBER
			MakeTestDataNtfsEx( GetNTFSStatisticsEx(pBufferEx) );
#endif
		}
	}
	__finally
	{
		DWORD dwError = GetLastError();

		if( dwError != ERROR_SUCCESS )
		{
			if( pBuffer )		
			{
				_SafeMemFree(pBuffer);
				cbBufferSize = 0;
			}
	
			if( PtrArray )
			{
				_SafeMemFree(PtrArray);
				cbPtrArray = 0;
			}

			if( pBufferEx )
			{
				_SafeMemFree(pBufferEx);
				cbBufferSizeEx = 0;
			}
		}

		*Buffer = pBuffer;
		if( BufferSize )
			*BufferSize = cbBufferSize;

		if( StatisticsPtrArray )
			*StatisticsPtrArray = (FILESYSTEM_STATISTICS**)PtrArray;
		if( StatisticsPtrArrayCount )
			*StatisticsPtrArrayCount = si.dwNumberOfProcessors;
		if( StatisticsPtrArraySize )
			*StatisticsPtrArraySize = cbPtrArray;

		if( StatisticsEx )
			*StatisticsEx = pBufferEx;
		if( StatisticsExSize )
			*StatisticsExSize = cbBufferSizeEx;

		if( hVolumeRoot != INVALID_HANDLE_VALUE )
			CloseHandle(hVolumeRoot);

		hr = HRESULT_FROM_WIN32( dwError );
	}

	return hr;
}

//----------------------------------------------------------------------------
//
//  CalcStatisticsDiffEx()
//
//----------------------------------------------------------------------------
#define _CALC_DIFF(m,pa,p1,p2)   (pa->m)=(p2->m)-(p1->m)

HRESULT
WINAPI
CalcStatisticsDiffEx(
	PVOID pDiff, 
	PVOID pData1, 
	PVOID pData2
	)
{
	PFILESYSTEM_STATISTICS_EX pa = (PFILESYSTEM_STATISTICS_EX)pDiff;
	PFILESYSTEM_STATISTICS_EX p1 = (PFILESYSTEM_STATISTICS_EX)pData1;
	PFILESYSTEM_STATISTICS_EX p2 = (PFILESYSTEM_STATISTICS_EX)pData2;

	_CALC_DIFF(UserFileReads,       pa, p1, p2);
    _CALC_DIFF(UserFileReadBytes,   pa, p1, p2);
    _CALC_DIFF(UserDiskReads,       pa, p1, p2);
    _CALC_DIFF(UserFileWrites,      pa, p1, p2);
    _CALC_DIFF(UserFileWriteBytes,  pa, p1, p2);
    _CALC_DIFF(UserDiskWrites,      pa, p1, p2);
    _CALC_DIFF(MetaDataReads,       pa, p1, p2);
    _CALC_DIFF(MetaDataReadBytes,   pa, p1, p2);
    _CALC_DIFF(MetaDataDiskReads,   pa, p1, p2);
    _CALC_DIFF(MetaDataWrites,      pa, p1, p2);
    _CALC_DIFF(MetaDataWriteBytes,  pa, p1, p2);
    _CALC_DIFF(MetaDataDiskWrites,  pa, p1, p2);

	return S_OK;
}

//----------------------------------------------------------------------------
//
//  CalcStatisticsDiffNtfsEx()
//
//----------------------------------------------------------------------------
HRESULT
WINAPI
CalcStatisticsDiffNtfsEx(
	PVOID pDiff, 
	PVOID pData1, 
	PVOID pData2
	)
{
	PNTFS_STATISTICS_EX pa = (PNTFS_STATISTICS_EX)pDiff;
	PNTFS_STATISTICS_EX p1 = (PNTFS_STATISTICS_EX)pData1;
	PNTFS_STATISTICS_EX p2 = (PNTFS_STATISTICS_EX)pData2;

	_CALC_DIFF(LogFileFullExceptions,                   pa, p1, p2);
	_CALC_DIFF(OtherExceptions,                         pa, p1, p2);
	_CALC_DIFF(MftReads,                                pa, p1, p2);
	_CALC_DIFF(MftReadBytes,                            pa, p1, p2);
	_CALC_DIFF(MftWrites,                               pa, p1, p2);
	_CALC_DIFF(MftWriteBytes,                           pa, p1, p2);
	_CALC_DIFF(MftWritesUserLevel.Write,                pa, p1, p2);
	_CALC_DIFF(MftWritesUserLevel.Create,               pa, p1, p2);
	_CALC_DIFF(MftWritesUserLevel.SetInfo,              pa, p1, p2);
	_CALC_DIFF(MftWritesUserLevel.Flush,                pa, p1, p2);
	_CALC_DIFF(MftWritesFlushForLogFileFull,            pa, p1, p2);
	_CALC_DIFF(MftWritesLazyWriter,                     pa, p1, p2);
	_CALC_DIFF(MftWritesUserRequest,                    pa, p1, p2);
	_CALC_DIFF(Mft2Writes,                              pa, p1, p2);
	_CALC_DIFF(Mft2WriteBytes,                          pa, p1, p2);
	_CALC_DIFF(Mft2WritesUserLevel.Write,               pa, p1, p2);
	_CALC_DIFF(Mft2WritesUserLevel.Create,              pa, p1, p2);
	_CALC_DIFF(Mft2WritesUserLevel.SetInfo,             pa, p1, p2);
	_CALC_DIFF(Mft2WritesUserLevel.Flush,               pa, p1, p2);
	_CALC_DIFF(Mft2WritesFlushForLogFileFull,           pa, p1, p2);
	_CALC_DIFF(Mft2WritesLazyWriter,                    pa, p1, p2);
	_CALC_DIFF(Mft2WritesUserRequest,                   pa, p1, p2);
	_CALC_DIFF(RootIndexReads,                          pa, p1, p2);
	_CALC_DIFF(RootIndexReadBytes,                      pa, p1, p2);
	_CALC_DIFF(RootIndexWrites,                         pa, p1, p2);
	_CALC_DIFF(RootIndexWriteBytes,                     pa, p1, p2);
	_CALC_DIFF(BitmapReads,                             pa, p1, p2);
	_CALC_DIFF(BitmapReadBytes,                         pa, p1, p2);
	_CALC_DIFF(BitmapWrites,                            pa, p1, p2);
	_CALC_DIFF(BitmapWriteBytes,                        pa, p1, p2);
	_CALC_DIFF(BitmapWritesFlushForLogFileFull,         pa, p1, p2);
	_CALC_DIFF(BitmapWritesLazyWriter,                  pa, p1, p2);
	_CALC_DIFF(BitmapWritesUserRequest,                 pa, p1, p2);
	_CALC_DIFF(BitmapWritesUserLevel.Write,             pa, p1, p2);
	_CALC_DIFF(BitmapWritesUserLevel.Create,            pa, p1, p2);
	_CALC_DIFF(BitmapWritesUserLevel.SetInfo,           pa, p1, p2);
	_CALC_DIFF(BitmapWritesUserLevel.Flush,             pa, p1, p2);
	_CALC_DIFF(MftBitmapReads,                          pa, p1, p2);
	_CALC_DIFF(MftBitmapReadBytes,                      pa, p1, p2);
	_CALC_DIFF(MftBitmapWrites,                         pa, p1, p2);
	_CALC_DIFF(MftBitmapWriteBytes,                     pa, p1, p2);
	_CALC_DIFF(MftBitmapWritesFlushForLogFileFull,      pa, p1, p2);
	_CALC_DIFF(MftBitmapWritesLazyWriter,               pa, p1, p2);
	_CALC_DIFF(MftBitmapWritesUserRequest,              pa, p1, p2);
	_CALC_DIFF(MftBitmapWritesUserLevel.Write,          pa, p1, p2);
	_CALC_DIFF(MftBitmapWritesUserLevel.Create,         pa, p1, p2);
	_CALC_DIFF(MftBitmapWritesUserLevel.SetInfo,        pa, p1, p2);
	_CALC_DIFF(MftBitmapWritesUserLevel.Flush,          pa, p1, p2);
	_CALC_DIFF(UserIndexReads,                          pa, p1, p2);
	_CALC_DIFF(UserIndexReadBytes,                      pa, p1, p2);
	_CALC_DIFF(UserIndexWrites,                         pa, p1, p2);
	_CALC_DIFF(UserIndexWriteBytes,                     pa, p1, p2);
	_CALC_DIFF(LogFileReads,                            pa, p1, p2);
	_CALC_DIFF(LogFileReadBytes,                        pa, p1, p2);
	_CALC_DIFF(LogFileWrites,                           pa, p1, p2);
	_CALC_DIFF(LogFileWriteBytes,                       pa, p1, p2);
	_CALC_DIFF(Allocate.Calls,                          pa, p1, p2);
	_CALC_DIFF(Allocate.RunsReturned,                   pa, p1, p2);
	_CALC_DIFF(Allocate.Hints,                          pa, p1, p2);
	_CALC_DIFF(Allocate.HintsHonored,                   pa, p1, p2);
	_CALC_DIFF(Allocate.Cache,                          pa, p1, p2);
	_CALC_DIFF(Allocate.CacheMiss,                      pa, p1, p2);
	_CALC_DIFF(Allocate.Clusters,                       pa, p1, p2);
	_CALC_DIFF(Allocate.HintsClusters,                  pa, p1, p2);
	_CALC_DIFF(Allocate.CacheClusters,                  pa, p1, p2);
	_CALC_DIFF(Allocate.CacheMissClusters,              pa, p1, p2);
	_CALC_DIFF(DiskResourcesExhausted,                  pa, p1, p2);
	_CALC_DIFF(VolumeTrimCount,                         pa, p1, p2);
	_CALC_DIFF(VolumeTrimTime,                          pa, p1, p2);
	_CALC_DIFF(VolumeTrimByteCount,                     pa, p1, p2);
	_CALC_DIFF(FileLevelTrimCount,                      pa, p1, p2);
	_CALC_DIFF(FileLevelTrimTime,                       pa, p1, p2);
	_CALC_DIFF(FileLevelTrimByteCount,                  pa, p1, p2);
	_CALC_DIFF(VolumeTrimSkippedCount,                  pa, p1, p2);
	_CALC_DIFF(VolumeTrimSkippedByteCount,              pa, p1, p2);
	_CALC_DIFF(NtfsFillStatInfoFromMftRecordCalledCount,                           pa, p1, p2);
	_CALC_DIFF(NtfsFillStatInfoFromMftRecordBailedBecauseOfAttributeListCount,     pa, p1, p2);
	_CALC_DIFF(NtfsFillStatInfoFromMftRecordBailedBecauseOfNonResReparsePointCount,pa, p1, p2);

	return S_OK;
}

#define _CALC_ADD_NTFS(m,pa,p1)   (((PNTFS_STATISTICS_EX)pa)->m)+=(((PNTFS_STATISTICS)p1)->m)
#define _CALC_ADD_NTFS_WIN8(m,pa,p1)   (((PNTFS_STATISTICS_EX)pa)->m)+=(((PNTFS_STATISTICS_WIN8)p1)->m)

HRESULT
WINAPI
AddDataValueNtfsToNtfsEx(
	PNTFS_STATISTICS_EX pex,
	PNTFS_STATISTICS ps
	)
{
	_CALC_ADD_NTFS(LogFileFullExceptions,                   pex, ps);
	_CALC_ADD_NTFS(OtherExceptions,                         pex, ps);
	_CALC_ADD_NTFS(MftReads,                                pex, ps);
	_CALC_ADD_NTFS(MftReadBytes,                            pex, ps);
	_CALC_ADD_NTFS(MftWrites,                               pex, ps);
	_CALC_ADD_NTFS(MftWriteBytes,                           pex, ps);
	_CALC_ADD_NTFS(MftWritesUserLevel.Write,                pex, ps);
	_CALC_ADD_NTFS(MftWritesUserLevel.Create,               pex, ps);
	_CALC_ADD_NTFS(MftWritesUserLevel.SetInfo,              pex, ps);
	_CALC_ADD_NTFS(MftWritesUserLevel.Flush,                pex, ps);
	_CALC_ADD_NTFS(MftWritesFlushForLogFileFull,            pex, ps);
	_CALC_ADD_NTFS(MftWritesLazyWriter,                     pex, ps);
	_CALC_ADD_NTFS(MftWritesUserRequest,                    pex, ps);
	_CALC_ADD_NTFS(Mft2Writes,                              pex, ps);
	_CALC_ADD_NTFS(Mft2WriteBytes,                          pex, ps);
	_CALC_ADD_NTFS(Mft2WritesUserLevel.Write,               pex, ps);
	_CALC_ADD_NTFS(Mft2WritesUserLevel.Create,              pex, ps);
	_CALC_ADD_NTFS(Mft2WritesUserLevel.SetInfo,             pex, ps);
	_CALC_ADD_NTFS(Mft2WritesUserLevel.Flush,               pex, ps);
	_CALC_ADD_NTFS(Mft2WritesFlushForLogFileFull,           pex, ps);
	_CALC_ADD_NTFS(Mft2WritesLazyWriter,                    pex, ps);
	_CALC_ADD_NTFS(Mft2WritesUserRequest,                   pex, ps);
	_CALC_ADD_NTFS(RootIndexReads,                          pex, ps);
	_CALC_ADD_NTFS(RootIndexReadBytes,                      pex, ps);
	_CALC_ADD_NTFS(RootIndexWrites,                         pex, ps);
	_CALC_ADD_NTFS(RootIndexWriteBytes,                     pex, ps);
	_CALC_ADD_NTFS(BitmapReads,                             pex, ps);
	_CALC_ADD_NTFS(BitmapReadBytes,                         pex, ps);
	_CALC_ADD_NTFS(BitmapWrites,                            pex, ps);
	_CALC_ADD_NTFS(BitmapWriteBytes,                        pex, ps);
	_CALC_ADD_NTFS(BitmapWritesFlushForLogFileFull,         pex, ps);
	_CALC_ADD_NTFS(BitmapWritesLazyWriter,                  pex, ps);
	_CALC_ADD_NTFS(BitmapWritesUserRequest,                 pex, ps);
	_CALC_ADD_NTFS(BitmapWritesUserLevel.Write,             pex, ps);
	_CALC_ADD_NTFS(BitmapWritesUserLevel.Create,            pex, ps);
	_CALC_ADD_NTFS(BitmapWritesUserLevel.SetInfo,           pex, ps);
//	_CALC_ADD_NTFS(BitmapWritesUserLevel.Flush,             pex, ps);  _EX only
	_CALC_ADD_NTFS(MftBitmapReads,                          pex, ps);
	_CALC_ADD_NTFS(MftBitmapReadBytes,                      pex, ps);
	_CALC_ADD_NTFS(MftBitmapWrites,                         pex, ps);
	_CALC_ADD_NTFS(MftBitmapWriteBytes,                     pex, ps);
	_CALC_ADD_NTFS(MftBitmapWritesFlushForLogFileFull,      pex, ps);
	_CALC_ADD_NTFS(MftBitmapWritesLazyWriter,               pex, ps);
	_CALC_ADD_NTFS(MftBitmapWritesUserRequest,              pex, ps);
	_CALC_ADD_NTFS(MftBitmapWritesUserLevel.Write,          pex, ps);
	_CALC_ADD_NTFS(MftBitmapWritesUserLevel.Create,         pex, ps);
	_CALC_ADD_NTFS(MftBitmapWritesUserLevel.SetInfo,        pex, ps);
	_CALC_ADD_NTFS(MftBitmapWritesUserLevel.Flush,          pex, ps);
	_CALC_ADD_NTFS(UserIndexReads,                          pex, ps);
	_CALC_ADD_NTFS(UserIndexReadBytes,                      pex, ps);
	_CALC_ADD_NTFS(UserIndexWrites,                         pex, ps);
	_CALC_ADD_NTFS(UserIndexWriteBytes,                     pex, ps);
	_CALC_ADD_NTFS(LogFileReads,                            pex, ps);
	_CALC_ADD_NTFS(LogFileReadBytes,                        pex, ps);
	_CALC_ADD_NTFS(LogFileWrites,                           pex, ps);
	_CALC_ADD_NTFS(LogFileWriteBytes,                       pex, ps);
	_CALC_ADD_NTFS(Allocate.Calls,                          pex, ps);
	_CALC_ADD_NTFS(Allocate.RunsReturned,                   pex, ps);
	_CALC_ADD_NTFS(Allocate.Hints,                          pex, ps);
	_CALC_ADD_NTFS(Allocate.HintsHonored,                   pex, ps);
	_CALC_ADD_NTFS(Allocate.Cache,                          pex, ps);
	_CALC_ADD_NTFS(Allocate.CacheMiss,                      pex, ps);
	_CALC_ADD_NTFS(Allocate.Clusters,                       pex, ps);
	_CALC_ADD_NTFS(Allocate.HintsClusters,                  pex, ps);
	_CALC_ADD_NTFS(Allocate.CacheClusters,                  pex, ps);
	_CALC_ADD_NTFS(Allocate.CacheMissClusters,              pex, ps);
	_CALC_ADD_NTFS_WIN8(DiskResourcesExhausted,             pex, ps);  // Additions for Win8.1
//	_CALC_ADD_NTFS(VolumeTrimCount,                         pex, ps);  // _EX only
//	_CALC_ADD_NTFS(VolumeTrimTime,                          pex, ps);  // _EX only
//	_CALC_ADD_NTFS(VolumeTrimByteCount,                     pex, ps);  // _EX only
//	_CALC_ADD_NTFS(FileLevelTrimCount,                      pex, ps);  // _EX only
//	_CALC_ADD_NTFS(FileLevelTrimTime,                       pex, ps);  // _EX only
//	_CALC_ADD_NTFS(FileLevelTrimByteCount,                  pex, ps);  // _EX only
//	_CALC_ADD_NTFS(VolumeTrimSkippedCount,                  pex, ps);  // _EX only
//	_CALC_ADD_NTFS(VolumeTrimSkippedByteCount,              pex, ps);  // _EX only
//	_CALC_ADD_NTFS(NtfsFillStatInfoFromMftRecordCalledCount,                           pex, ps);  // _EX only
//	_CALC_ADD_NTFS(NtfsFillStatInfoFromMftRecordBailedBecauseOfAttributeListCount,     pex, ps);  // _EX only
//	_CALC_ADD_NTFS(NtfsFillStatInfoFromMftRecordBailedBecauseOfNonResReparsePointCount,pex, ps);  // _EX only

	return S_OK;
}

#define _CALC_ADD_STATISTICS(m,pa,ps)   (((PFILESYSTEM_STATISTICS_EX)pa)->m)+=(((PFILESYSTEM_STATISTICS)ps)->m)

HRESULT
WINAPI
AddDataValueStatisticsToStatisticsEx(
	PFILESYSTEM_STATISTICS_EX pex,
	PFILESYSTEM_STATISTICS ps
	)
{
    _CALC_ADD_STATISTICS(UserFileReads,      pex,ps);
    _CALC_ADD_STATISTICS(UserFileReadBytes,  pex,ps);
    _CALC_ADD_STATISTICS(UserDiskReads,      pex,ps);
    _CALC_ADD_STATISTICS(UserFileWrites,     pex,ps);
    _CALC_ADD_STATISTICS(UserFileWriteBytes, pex,ps);
    _CALC_ADD_STATISTICS(UserDiskWrites,     pex,ps);
    _CALC_ADD_STATISTICS(MetaDataReads,      pex,ps);
    _CALC_ADD_STATISTICS(MetaDataReadBytes,  pex,ps);
    _CALC_ADD_STATISTICS(MetaDataDiskReads,  pex,ps);
    _CALC_ADD_STATISTICS(MetaDataWrites,     pex,ps);
    _CALC_ADD_STATISTICS(MetaDataWriteBytes, pex,ps);
    _CALC_ADD_STATISTICS(MetaDataDiskWrites, pex,ps);
	return S_OK;
}

#define _CALC_ADD_FAT(m,pd,ps)   (((PFAT_STATISTICS)pd)->m)+=(((PFAT_STATISTICS)ps)->m)

HRESULT
WINAPI
AddDataValueFatToFat(
	PFAT_STATISTICS pd,
	PFAT_STATISTICS ps
	)
{
    _CALC_ADD_FAT(CreateHits,          pd,ps);
    _CALC_ADD_FAT(SuccessfulCreates,   pd,ps);
    _CALC_ADD_FAT(FailedCreates,       pd,ps);
    _CALC_ADD_FAT(NonCachedReads,      pd,ps);
    _CALC_ADD_FAT(NonCachedReadBytes,  pd,ps);
    _CALC_ADD_FAT(NonCachedWrites,     pd,ps);
    _CALC_ADD_FAT(NonCachedWriteBytes, pd,ps);
    _CALC_ADD_FAT(NonCachedDiskReads,  pd,ps);
    _CALC_ADD_FAT(NonCachedDiskWrites, pd,ps);
	return S_OK;
}

void MakeStatisticsEx(
	PFILESYSTEM_STATISTICS_EX pex,
	PFILESYSTEM_STATISTICS *pa,
	int cpa
	)
{
	int i;
	for(i = 0; i < cpa; i++)
	{
		AddDataValueStatisticsToStatisticsEx(pex,pa[i]);
	}

	if( pa[0]->FileSystemType == FILESYSTEM_STATISTICS_TYPE_NTFS ||
		pa[0]->FileSystemType == FILESYSTEM_STATISTICS_TYPE_REFS )
	{
		;
	}
	else if( pa[0]->FileSystemType == FILESYSTEM_STATISTICS_TYPE_EXFAT ||
			 pa[0]->FileSystemType == FILESYSTEM_STATISTICS_TYPE_FAT )
	{
		//
		// FAT/ExFAT
		//
		FAT_STATISTICS *pFatDst = GetFATStatisticsEx(pex);
		for(i = 0; i < cpa; i++)
		{
			FAT_STATISTICS *pFatSrc = GetFATStatistics(pa[i]);
			AddDataValueFatToFat(pFatDst,pFatSrc);
		}
	}
}

#ifdef _DEBUG

//----------------------------------------------------------------------------
//
//  MakeTestDataNtfs()
//
//----------------------------------------------------------------------------
#define _SET_TEST_DATA(m,pa,d) (pa->m)=(d);

static void MakeTestDataNtfs(PNTFS_STATISTICS pa)
{
	int val = 0;
	_SET_TEST_DATA(LogFileFullExceptions,               pa, val++);
	_SET_TEST_DATA(OtherExceptions,                     pa, val++);
	_SET_TEST_DATA(MftReads,                            pa, val++);
	_SET_TEST_DATA(MftReadBytes,                        pa, val++);
	_SET_TEST_DATA(MftWrites,                           pa, val++);
	_SET_TEST_DATA(MftWriteBytes,                       pa, val++);
	_SET_TEST_DATA(MftWritesUserLevel.Write,            pa, val++);
	_SET_TEST_DATA(MftWritesUserLevel.Create,           pa, val++);
	_SET_TEST_DATA(MftWritesUserLevel.SetInfo,          pa, val++);
	_SET_TEST_DATA(MftWritesUserLevel.Flush,            pa, val++);
	_SET_TEST_DATA(MftWritesFlushForLogFileFull,        pa, val++);
	_SET_TEST_DATA(MftWritesLazyWriter,                 pa, val++);
	_SET_TEST_DATA(MftWritesUserRequest,                pa, val++);
	_SET_TEST_DATA(Mft2Writes,                          pa, val++);
	_SET_TEST_DATA(Mft2WriteBytes,                      pa, val++);
	_SET_TEST_DATA(Mft2WritesUserLevel.Write,           pa, val++);
	_SET_TEST_DATA(Mft2WritesUserLevel.Create,          pa, val++);
	_SET_TEST_DATA(Mft2WritesUserLevel.SetInfo,         pa, val++);
	_SET_TEST_DATA(Mft2WritesUserLevel.Flush,           pa, val++);
	_SET_TEST_DATA(Mft2WritesFlushForLogFileFull,       pa, val++);
	_SET_TEST_DATA(Mft2WritesLazyWriter,                pa, val++);
	_SET_TEST_DATA(Mft2WritesUserRequest,               pa, val++);
	_SET_TEST_DATA(RootIndexReads,                      pa, val++);
	_SET_TEST_DATA(RootIndexReadBytes,                  pa, val++);
	_SET_TEST_DATA(RootIndexWrites,                     pa, val++);
	_SET_TEST_DATA(RootIndexWriteBytes,                 pa, val++);
	_SET_TEST_DATA(BitmapReads,                         pa, val++);
	_SET_TEST_DATA(BitmapReadBytes,                     pa, val++);
	_SET_TEST_DATA(BitmapWrites,                        pa, val++);
	_SET_TEST_DATA(BitmapWriteBytes,                    pa, val++);
	_SET_TEST_DATA(BitmapWritesFlushForLogFileFull,     pa, val++);
	_SET_TEST_DATA(BitmapWritesLazyWriter,              pa, val++);
	_SET_TEST_DATA(BitmapWritesUserRequest,             pa, val++);
	_SET_TEST_DATA(BitmapWritesUserLevel.Write,         pa, val++);
	_SET_TEST_DATA(BitmapWritesUserLevel.Create,        pa, val++);
	_SET_TEST_DATA(BitmapWritesUserLevel.SetInfo,       pa, val++);
	_SET_TEST_DATA(MftBitmapReads,                      pa, val++);
	_SET_TEST_DATA(MftBitmapReadBytes,                  pa, val++);
	_SET_TEST_DATA(MftBitmapWrites,                     pa, val++);
	_SET_TEST_DATA(MftBitmapWriteBytes,                 pa, val++);
	_SET_TEST_DATA(MftBitmapWritesFlushForLogFileFull,  pa, val++);
	_SET_TEST_DATA(MftBitmapWritesLazyWriter,           pa, val++);
	_SET_TEST_DATA(MftBitmapWritesUserRequest,          pa, val++);
	_SET_TEST_DATA(MftBitmapWritesUserLevel.Write,      pa, val++);
	_SET_TEST_DATA(MftBitmapWritesUserLevel.Create,     pa, val++);
	_SET_TEST_DATA(MftBitmapWritesUserLevel.SetInfo,    pa, val++);
	_SET_TEST_DATA(MftBitmapWritesUserLevel.Flush,      pa, val++);
	_SET_TEST_DATA(UserIndexReads,                      pa, val++);
	_SET_TEST_DATA(UserIndexReadBytes,                  pa, val++);
	_SET_TEST_DATA(UserIndexWrites,                     pa, val++);
	_SET_TEST_DATA(UserIndexWriteBytes,                 pa, val++);
	_SET_TEST_DATA(LogFileReads,                        pa, val++);
	_SET_TEST_DATA(LogFileReadBytes,                    pa, val++);
	_SET_TEST_DATA(LogFileWrites,                       pa, val++);
	_SET_TEST_DATA(LogFileWriteBytes,                   pa, val++);
	_SET_TEST_DATA(Allocate.Calls,                      pa, val++);
	_SET_TEST_DATA(Allocate.RunsReturned,               pa, val++);
	_SET_TEST_DATA(Allocate.Hints,                      pa, val++);
	_SET_TEST_DATA(Allocate.HintsHonored,               pa, val++);
	_SET_TEST_DATA(Allocate.Cache,                      pa, val++);
	_SET_TEST_DATA(Allocate.CacheMiss,                  pa, val++);
	_SET_TEST_DATA(Allocate.Clusters,                   pa, val++);
	_SET_TEST_DATA(Allocate.HintsClusters,              pa, val++);
	_SET_TEST_DATA(Allocate.CacheClusters,              pa, val++);
	_SET_TEST_DATA(Allocate.CacheMissClusters,          pa, val++);
}

//----------------------------------------------------------------------------
//
//  MakeTestDataNtfsEx()
//
//----------------------------------------------------------------------------
static void MakeTestDataNtfsEx(PNTFS_STATISTICS_EX pa)
{
	int val = 0;
	_SET_TEST_DATA(LogFileFullExceptions,               pa, val++);
	_SET_TEST_DATA(OtherExceptions,                     pa, val++);
	_SET_TEST_DATA(MftReads,                            pa, val++);
	_SET_TEST_DATA(MftReadBytes,                        pa, val++);
	_SET_TEST_DATA(MftWrites,                           pa, val++);
	_SET_TEST_DATA(MftWriteBytes,                       pa, val++);
	_SET_TEST_DATA(MftWritesUserLevel.Write,            pa, val++);
	_SET_TEST_DATA(MftWritesUserLevel.Create,           pa, val++);
	_SET_TEST_DATA(MftWritesUserLevel.SetInfo,          pa, val++);
	_SET_TEST_DATA(MftWritesUserLevel.Flush,            pa, val++);
	_SET_TEST_DATA(MftWritesFlushForLogFileFull,        pa, val++);
	_SET_TEST_DATA(MftWritesLazyWriter,                 pa, val++);
	_SET_TEST_DATA(MftWritesUserRequest,                pa, val++);
	_SET_TEST_DATA(Mft2Writes,                          pa, val++);
	_SET_TEST_DATA(Mft2WriteBytes,                      pa, val++);
	_SET_TEST_DATA(Mft2WritesUserLevel.Write,           pa, val++);
	_SET_TEST_DATA(Mft2WritesUserLevel.Create,          pa, val++);
	_SET_TEST_DATA(Mft2WritesUserLevel.SetInfo,         pa, val++);
	_SET_TEST_DATA(Mft2WritesUserLevel.Flush,           pa, val++);
	_SET_TEST_DATA(Mft2WritesFlushForLogFileFull,       pa, val++);
	_SET_TEST_DATA(Mft2WritesLazyWriter,                pa, val++);
	_SET_TEST_DATA(Mft2WritesUserRequest,               pa, val++);
	_SET_TEST_DATA(RootIndexReads,                      pa, val++);
	_SET_TEST_DATA(RootIndexReadBytes,                  pa, val++);
	_SET_TEST_DATA(RootIndexWrites,                     pa, val++);
	_SET_TEST_DATA(RootIndexWriteBytes,                 pa, val++);
	_SET_TEST_DATA(BitmapReads,                         pa, val++);
	_SET_TEST_DATA(BitmapReadBytes,                     pa, val++);
	_SET_TEST_DATA(BitmapWrites,                        pa, val++);
	_SET_TEST_DATA(BitmapWriteBytes,                    pa, val++);
	_SET_TEST_DATA(BitmapWritesFlushForLogFileFull,     pa, val++);
	_SET_TEST_DATA(BitmapWritesLazyWriter,              pa, val++);
	_SET_TEST_DATA(BitmapWritesUserRequest,             pa, val++);
	_SET_TEST_DATA(BitmapWritesUserLevel.Write,         pa, val++);
	_SET_TEST_DATA(BitmapWritesUserLevel.Create,        pa, val++);
	_SET_TEST_DATA(BitmapWritesUserLevel.SetInfo,       pa, val++);
	_SET_TEST_DATA(BitmapWritesUserLevel.Flush,         pa, val++);
	_SET_TEST_DATA(MftBitmapReads,                      pa, val++);
	_SET_TEST_DATA(MftBitmapReadBytes,                  pa, val++);
	_SET_TEST_DATA(MftBitmapWrites,                     pa, val++);
	_SET_TEST_DATA(MftBitmapWriteBytes,                 pa, val++);
	_SET_TEST_DATA(MftBitmapWritesFlushForLogFileFull,  pa, val++);
	_SET_TEST_DATA(MftBitmapWritesLazyWriter,           pa, val++);
	_SET_TEST_DATA(MftBitmapWritesUserRequest,          pa, val++);
	_SET_TEST_DATA(MftBitmapWritesUserLevel.Write,      pa, val++);
	_SET_TEST_DATA(MftBitmapWritesUserLevel.Create,     pa, val++);
	_SET_TEST_DATA(MftBitmapWritesUserLevel.SetInfo,    pa, val++);
	_SET_TEST_DATA(MftBitmapWritesUserLevel.Flush,      pa, val++);
	_SET_TEST_DATA(UserIndexReads,                      pa, val++);
	_SET_TEST_DATA(UserIndexReadBytes,                  pa, val++);
	_SET_TEST_DATA(UserIndexWrites,                     pa, val++);
	_SET_TEST_DATA(UserIndexWriteBytes,                 pa, val++);
	_SET_TEST_DATA(LogFileReads,                        pa, val++);
	_SET_TEST_DATA(LogFileReadBytes,                    pa, val++);
	_SET_TEST_DATA(LogFileWrites,                       pa, val++);
	_SET_TEST_DATA(LogFileWriteBytes,                   pa, val++);
	_SET_TEST_DATA(Allocate.Calls,                      pa, val++);
	_SET_TEST_DATA(Allocate.RunsReturned,               pa, val++);
	_SET_TEST_DATA(Allocate.Hints,                      pa, val++);
	_SET_TEST_DATA(Allocate.HintsHonored,               pa, val++);
	_SET_TEST_DATA(Allocate.Cache,                      pa, val++);
	_SET_TEST_DATA(Allocate.CacheMiss,                  pa, val++);
	_SET_TEST_DATA(Allocate.Clusters,                   pa, val++);
	_SET_TEST_DATA(Allocate.HintsClusters,              pa, val++);
	_SET_TEST_DATA(Allocate.CacheClusters,              pa, val++);
	_SET_TEST_DATA(Allocate.CacheMissClusters,          pa, val++);
	_SET_TEST_DATA(DiskResourcesExhausted,              pa, val++);
	_SET_TEST_DATA(VolumeTrimCount,                     pa, val++);
	_SET_TEST_DATA(VolumeTrimTime,                      pa, val++);
	_SET_TEST_DATA(VolumeTrimByteCount,                 pa, val++);
	_SET_TEST_DATA(FileLevelTrimCount,                  pa, val++);
	_SET_TEST_DATA(FileLevelTrimTime,                   pa, val++);
	_SET_TEST_DATA(FileLevelTrimByteCount,              pa, val++);
	_SET_TEST_DATA(VolumeTrimSkippedCount,              pa, val++);
	_SET_TEST_DATA(VolumeTrimSkippedByteCount,          pa, val++);
	_SET_TEST_DATA(NtfsFillStatInfoFromMftRecordCalledCount,                           pa, val++);
	_SET_TEST_DATA(NtfsFillStatInfoFromMftRecordBailedBecauseOfAttributeListCount,     pa, val++);
	_SET_TEST_DATA(NtfsFillStatInfoFromMftRecordBailedBecauseOfNonResReparsePointCount,pa, val++);
}

#endif
