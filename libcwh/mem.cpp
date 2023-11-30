//****************************************************************************
//*                                                                          *
//*  Mem.cpp                                                                 *
//*                                                                          *
//*  PURPOSE:   Memory allocate function                                     *
//*                                                                          *
//*  HISTORY:   1999.06.19 Create                                            *
//*             2000.01.29 BUGFIX: UNICODE string allocation                 *
//*             2010.12.02 code clean up (unuse stdafx.h)                    *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include <windows.h>
#include <malloc.h>
#include <crtdbg.h>
#include <stdio.h>
#include <tchar.h>
#include "mem.h"
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef new

#ifdef   _DEBUG
#define  SET_CRT_DEBUG_FIELD(a)   _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define  CLEAR_CRT_DEBUG_FIELD(a) _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#else
#define  SET_CRT_DEBUG_FIELD(a)   ((void) 0)
#define  CLEAR_CRT_DEBUG_FIELD(a) ((void) 0)
#endif

VOID _MemInit()
{
#ifdef _DEBUG
	_CrtSetReportMode( _CRT_WARN,   _CRTDBG_MODE_DEBUG );
	_CrtSetReportFile( _CRT_WARN,   _CRTDBG_FILE_STDERR );

	_CrtSetReportMode( _CRT_ERROR,  _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_WNDW );
	_CrtSetReportFile( _CRT_ERROR,  _CRTDBG_FILE_STDERR );

	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_WNDW );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );

#ifdef _DEBUG_HEAP_DELAY_FREE_MEM_DF
	// 解放済みのブロックは使われずにデバッグ ヒープの
	// リンク リスト中に保持されて、0xDD で埋められます。
	// 注) 実行速度が遅くなります。
	SET_CRT_DEBUG_FIELD( _CRTDBG_DELAY_FREE_MEM_DF );
#endif

#ifdef _DEBUG_HEAP_CHECK_CRT_DF
	// C ランタイム用の内部ブロックをチェックに含めます。
	//
	SET_CRT_DEBUG_FIELD( _CRTDBG_CHECK_CRT_DF );
#endif

#endif
}

VOID _MemEnd()
{
#ifdef _DEBUG
	SET_CRT_DEBUG_FIELD( _CRTDBG_LEAK_CHECK_DF );
#endif
}

VOID _MemFree(VOID *p)
{
#ifndef _DEBUG
	free(p);
#else
	_free_dbg(p,_NORMAL_BLOCK);
	_CrtCheckMemory( );
#endif
}

#ifdef _DEBUG

VOID* _MemAllocDebug(SIZE_T cb,LPSTR File,int Line)
{
	void *pv = _malloc_dbg(cb,_NORMAL_BLOCK,(LPCSTR)File,Line);
	memset(pv,0xcc,cb); // bug check
	return pv;
}

PTSTR _MemAllocStringDebug(PCTSTR sz,LPSTR File,int Line)
{
	PTSTR p;
	ASSERT( sz != NULL );
	p = (PTSTR)_MemAllocDebug((lstrlen(sz)+1) * sizeof(sz[0]),File,Line);
	if( p )
		lstrcpy(p,sz);
	return p;
}

VOID* _MemReAllocDebug(VOID *pv,SIZE_T cb,LPSTR File,int Line)
{
	return _realloc_dbg(pv,cb,_NORMAL_BLOCK,File,Line);
}

VOID* _MemAllocZeroDebug(SIZE_T cb,LPSTR File,int Line)
{
	VOID *p;
	p = _MemAllocDebug(cb,File,Line);
	memset(p,0,cb);
	return p;
}

PTSTR _MemAllocStringBufferDebug(SIZE_T cch,LPSTR File,int Line)
{
	void *pv = _malloc_dbg((cch + 1) * sizeof(TCHAR),_NORMAL_BLOCK,(LPCSTR)File,Line);
	memset(pv,0xcc,(cch + 1) * sizeof(TCHAR)); // bug check
	return (PTSTR)pv;
}

PTSTR _MemAllocStringCatDebug(PCTSTR psz1,PCTSTR psz2,LPSTR File,int Line)
{
	PTSTR p;
	SIZE_T cch;

	cch = _tcslen(psz1) + _tcslen(psz2) + 1;

	p = _MemAllocStringBufferDebug(cch,File,Line);
	if( p == NULL)
		return NULL;

	_tcscpy(p,psz1);
	_tcscat(p,psz2);
	return p;
}

void* __cdecl operator new(size_t nSize, LPCSTR pszFilename, int nLine)
{
	return _MemAllocDebug(nSize,(LPSTR)pszFilename,nLine);
}

void operator delete(void *pMem, LPCSTR lpszFileName, int nLine)
{
	UNREFERENCED_PARAMETER(lpszFileName);
	UNREFERENCED_PARAMETER(nLine);
	_MemFree(pMem);
}

void _MemDebugDumpMemoryLeaks()
{
	_CrtDumpMemoryLeaks();
}

#else

VOID* _MemAlloc(SIZE_T cb)
{
	void *pv = malloc( cb );
	return pv;
}

VOID* _MemAllocZero(SIZE_T cb)
{
	register PVOID pv = _MemAlloc(cb);
	memset(pv,0,cb);
	return pv;
}

VOID* _MemReAlloc(VOID *pv,SIZE_T cb)
{
	return realloc(pv,cb);
}

PTSTR _MemAllocString(PCTSTR psz)
{
	if( psz == NULL )
		return NULL;
	PTSTR p;
	p = (PTSTR)_MemAlloc((_tcslen(psz)+1) * sizeof(psz[0]));
	if( p )
		_tcscpy(p,psz);
	return p;
}

PTSTR _MemAllocStringBuffer(SIZE_T cch)
{
	return (PTSTR)_MemAlloc( (cch + 1) * sizeof(TCHAR) );
}

PTSTR _MemAllocStringCat(PCTSTR psz1,PCTSTR psz2)
{
	PTSTR p;
	SIZE_T cch;

	cch = _tcslen(psz1) + _tcslen(psz2) + 1;

	p = _MemAllocStringBuffer(cch);
	if( p == NULL)
		return NULL;

	_tcscpy(p,psz1);
	_tcscat(p,psz2);
	return p;
}
#endif

#ifdef __cplusplus
}
#endif
