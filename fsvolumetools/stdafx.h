#pragma once

#include "targetver.h"

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <limits.h>

#undef WIN32_NO_STATUS        // defines STATUS_XXX in ntddk, now using includes DDK.
#include <ntstatus.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS       // no include STATUS_XXX in winnt.h
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <strsafe.h>
#include <winternl.h> // WinSDK 7.1
#include <winioctl.h>
#include <commoncontrols.h>
#include <uxtheme.h>

#if _MSC_VER <= 1500
#define nullptr NULL
#endif

#pragma warning(disable : 4995)

#include "debug.h"
#include "mem.h"
#include "stringbuffer.h"
#include "libmisc.h"
#include "common.h"
#include "common_control_helper.h"
#include "stringbuffer.h"
#include "libntwdk.h"
#include "ntnativeapi.h"
#include "ntvolumehelp.h"
#include "ntnativehelp.h"
#include "ntwin32helper.h"

extern HINSTANCE _GetResourceInstance();
extern HFONT CreateGlobalFont(HWND hWnd);
