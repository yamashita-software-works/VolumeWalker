// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef _WIN32_IE
#define _WIN32_IE _WIN32_IE_IE70
#endif

#pragma warning(disable: 4995)
#pragma warning(disable: 4996)

#define ATLASSERT
#define TRACE

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <shlobj.h>

#include "mem.h"
#include "debug.h"
#include "simplevalarray.h"

namespace LIBMISC {
extern HMODULE g_ResourceInstance;
extern DWORD g_dwLanguageId;
extern VOID InitMessageBox(HINSTANCE hInst,PCWSTR pszCaption);
};
