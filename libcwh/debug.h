// Simple Debug Helper+++++++++++++++++++++++++++++
// 1998.05.12
// 2000.01.29 - Unicode support
// 2000.08.28 - BugFix
// 2001.03.02 - Update
// 2001.10.16 - Update

#ifndef _SIMPLE_DEBUG_HELPER_
#define _SIMPLE_DEBUG_HELPER_
#pragma warning(push)
#pragma warning(disable : 4995)

#ifdef _WIN64
#define DebugBreak()  { DebugBreak(); }
#else
#define DebugBreak()  {_asm int 3}
#endif
// DEBUG library No use version.
#ifndef _DEBUG_ASSERT_BUFFER_SIZE
#define _DEBUG_ASSERT_BUFFER_SIZE 512
#endif

#define _ASSERTA(exp) \
{\
  if((exp) == 0)\
  {\
     CHAR sz[_DEBUG_ASSERT_BUFFER_SIZE]; int r;\
     wsprintfA(sz,"Debug Assertion Failed!\n\n%s\nLine : %d",__FILE__,__LINE__);\
     r = MessageBoxA(0,sz,"Assert!",MB_SYSTEMMODAL|MB_ABORTRETRYIGNORE|MB_ICONSTOP);\
     if( r == IDRETRY ) { DebugBreak(); }\
     if( r == IDABORT ) { ExitProcess(0); }\
  }\
}

#define _VERIFYA(exp) \
{\
  if((exp) == 0)\
  {\
     CHAR sz[_DEBUG_ASSERT_BUFFER_SIZE]; int r;\
     sprintf(sz,"Runtime Assertion Failed!\n\nFile:%s  Line:%d",strrchr((const char*)__FILE__,TEXT('\\'))+1,__LINE__);\
     r = MessageBoxA(0,sz,"Assert!",MB_SYSTEMMODAL|MB_ABORTRETRYIGNORE|MB_ICONSTOP);\
     if( r == IDRETRY ) { DebugBreak(); }\
     if( r == IDABORT ) { ExitProcess(0); }\
  }\
}

#ifdef _DEBUG

/*++OBSOLETE
#define TRACEW(s)           OutputDebugStringW( L##s )
#define TRACEA(s)           OutputDebugStringA( s )

#ifdef UNICODE
#define TRACE  TRACEW
#else
#define TRACE  TRACEA
#endif
--*/

#define ASSERT(exp)         _ASSERTA(exp)          // Always mapping ANSI version
#define _RuntimeAssert(exp) _ASSERTA(exp)          // Always mapping ANSI version
#define VERIFY(exp)         _RuntimeAssert(exp)

inline void _cdecl _TraceExA(LPCSTR lpszFormat, ...)
{
    va_list args;
    va_start(args, lpszFormat);

    int nBuf;
    char szBuffer[1024];
    size_t cch = sizeof(szBuffer);

    nBuf = _vsnprintf_s(szBuffer,ARRAYSIZE(szBuffer), cch, lpszFormat, args);
    if( nBuf != -1 )
    {
        OutputDebugStringA(szBuffer);
    }

    va_end(args);
}
#define _TRACEA _TraceExA

inline void _cdecl _TraceExW(LPCWSTR lpszFormat, ...)
{
    va_list args;
    va_start(args, lpszFormat);

    int nBuf;
    WCHAR szBuffer[1024];
    size_t cch = sizeof(szBuffer) / sizeof(WCHAR);

    nBuf = _vsnwprintf_s(szBuffer, ARRAYSIZE(szBuffer), cch, lpszFormat, args);
    if( nBuf != -1 )
    {
        OutputDebugStringW(szBuffer);
    }

    va_end(args);
}

#define _TRACEW _TraceExW

#define _TRACE _TRACEA  // Always mapping ANSI version

#else

// For Release build. 
#if _MSC_VER > 1200
#define _NOP  __noop
#else
#define _NOP  NULL
#endif

#ifndef TRACE
#define TRACE(f)            _NOP
#endif

#define ASSERT              _NOP
#define _RuntimeAssert(exp) _VERIFYA(exp)  // Always mapping ANSI version
#define VERIFY(exp)         _VERIFYA(exp)
#define TRACEA              _NOP
#define TRACEW              _NOP
#define _TRACE              _NOP
#define _TRACEA             _NOP
#define _TRACEW             _NOP

#endif

#pragma warning(pop)
#endif