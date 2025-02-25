#pragma once
//
//  ntwin32helper.h
//
//  PURPOSE: 
//    using in ntddk build for Win32 API.Use when you want to
//    call Win32 API from NTDDK user mode source code.
//
#ifndef WINAPI
#define WINAPI      __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_NOMINUTESORSECONDS   0x00000001  // do not use minutes or seconds
#define TIME_NOSECONDS            0x00000002  // do not use seconds
#define TIME_NOTIMEMARKER         0x00000004  // do not use time marker
#define TIME_FORCE24HOURFORMAT    0x00000008  // always use 24 hour format

#define DATE_SHORTDATE            0x00000001  // use short date picture
#define DATE_LONGDATE             0x00000002  // use long date picture
#define DATE_USE_ALT_CALENDAR     0x00000004  // use alternate calendar (if any)
#define DATE_YEARMONTH            0x00000008  // use year month picture
#define DATE_LTRREADING           0x00000010  // add marks for left to right reading order layout
#define DATE_RTLREADING           0x00000020  // add marks for right to left reading order layout
#define DATE_AUTOLAYOUT           0x00000040  // add appropriate marks for left-to-right or right-to-left reading order layout

void WinGetTimeString(ULONG64 DateTime,LPTSTR pszText,int cchTextMax,LPCTSTR TimeFormat,BOOLEAN bTimeAsUTC,ULONG Flags);
void WinGetDateString(ULONG64 DateTime,LPTSTR pszText,int cchTextMax,LPCTSTR DateFormat,BOOLEAN bTimeAsUTC,ULONG Flags);

BOOLEAN
WINAPI
WinFileTimeToDosDateTime(
    __in   const LARGE_INTEGER *pliFileTime,
    __out  PUSHORT lpFatDate,
    __out  PUSHORT lpFatTime
    );

BOOLEAN
WINAPI
WinDosDateTimeToFileTime(
    __in   USHORT wFatDate,
    __in   USHORT wFatTime,
    __out  LARGE_INTEGER *pliFileTime
    );

int WinGetSystemErrorMessageEx(ULONG ErrorCode,PWSTR *ppMessage,ULONG dwLanguageId);
int WinGetErrorMessage(ULONG ErrorCode,PWSTR *ppMessage);
int WinGetSystemErrorMessage(ULONG ErrorCode,PWSTR *ppMessage);
void WinFreeErrorMessage(PWSTR pMessage);

PVOID WinLocalAlloc(ULONG Flags, SIZE_T uBytes);
PVOID WinLocalReAlloc(PVOID pMem,SIZE_T uBytes,ULONG uFlags);
PVOID WinLocalFree(PVOID pMem);
SIZE_T WinLocalSize(PVOID pMem);

#ifdef __cplusplus
};
#endif
