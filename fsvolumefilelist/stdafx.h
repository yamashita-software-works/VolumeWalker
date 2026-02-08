#pragma once

#include "targetver.h"
#include "..\build_switch.inc"

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

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
#include <commdlg.h>
#include <vssym32.h>

#include <gdiplus.h>
using namespace Gdiplus;

#if _MSC_VER <= 1500
#define nullptr NULL
#endif

#pragma warning(disable : 4995)
#pragma warning(disable : 4996)

#include "libntwdk.h"
#include "debug.h"
#include "mem.h"
#include "libmisc.h"
#include "dparray.h"
#include "simplestack.h"
#include "common_control_helper.h"
#include "simplevalarray.h"
#include "common.h"
#include "common_resid.h"
#include "common_resid_appdef.h"
#include "common_msg.h"
#include "listhelp.h"
#include "volumeconsoledef.h"
#include "volumeconsoleid.h"
#include "stringbuffer.h"
#include "globals.h"
#include "fsfilelib.h"
#include "fileitem.h"
#include "basewindow.h"
#include "themehelp.h"
#include "column.h"

#if _ENABLE_DARK_MODE_TEST
#include "darkmode.h"
#include "listviewutil.h"
#else
inline BOOL _IsDarkModeEnabled() { return FALSE; }
#endif

enum {
    COLUMN_None=0,
    COLUMN_Name,                /* 1 */
    COLUMN_LastWriteTime,       /* 2 */
    COLUMN_CreationTime,        /* 3 */
    COLUMN_LastAccessTime,      /* 4 */
    COLUMN_ChangeTime,          /* 5 */
    COLUMN_EndOfFile,           /* 6 */
    COLUMN_AllocationSize,      /* 7 */
    COLUMN_FileAttributes,      /* 8 */
    COLUMN_FileNameLength,      /* 9 */
    COLUMN_EaSize,              /* 10 */
    COLUMN_FileIndex,           /* 11 */
    COLUMN_FileId,              /* 12 */
    COLUMN_ShortName,           /* 13 */
    COLUMN_Extension,           /* 14 */
    COLUMN_Path,                /* 15 */
    COLUMN_Lcn,                 /* 16 */
    COLUMN_Date,                /* 17 */
    COLUMN_Reason,              /* 18 */
    COLUMN_Usn,                 /* 19 */
    COLUMN_Frn,                 /* 20 */
    COLUMN_ParentFrn,           /* 21 */
    COLUMN_PhysicalDriveNumber, /* 22 */
    COLUMN_PhysicalDriveOffset, /* 23 */
    COLUMN_UsnJournalID,        /* 24 */
    COLUMN_FirstUsn,            /* 25 */
    COLUMN_NextUsn,             /* 26 */
    COLUMN_LowestValidUsn,      /* 27 */
    COLUMN_MaxUsn,              /* 28 */
    COLUMN_MaximumSize,         /* 29 */
    COLUMN_AllocationDelta,     /* 30 */
    COLUMN_VolumeName,          /* 31 */
    COLUMN_ReparseTag,          /* 32 */
    COLUMN_ObjectId,            /* 33 */ 
    COLUMN_BirthVolumeId,       /* 34 */ 
    COLUMN_BirthObjectId,       /* 35 */ 
    COLUMN_DomainId,            /* 36 */
	COLUMN_Drive,               /* 37 */
	COLUMN_RelativePath,        /* 39 */
	COLUMN_VolumeRelativePath,  /* 40 */
    COLUMN_MaxItem,             /* 41 */
    COLUMN_MaxCount=COLUMN_MaxItem,
};


///////////////////////////////////////////////////////////////////////////////

#if _ENABLE_FILE_MANAGER || _ENABLE_FILELIST_DROPFILE
#include "clipboardfilecopy.h"
#endif

#if _ENABLE_FILE_MANAGER || _ENABLE_FILELIST_DRAGFILE || _ENABLE_FILELIST_DROPFILE

#include "fsfiletools.h" // for FO_PARAM,FO_SEARCH

#else

//
// FTM_FILEOPERATION
//
//
// wParam - Pointer to a FO_PARAM structure.
//
// lParam - A command dependent parameter.
//
// Return - Returns HRESULT code.
//
#define FTM_FILEOPERATION   (WM_USER+_TFM_FIRST+10)  

typedef struct _FO_PARAM {
	HWND hwnd;
	UINT cmd;
	UINT Flags;
	HRESULT hr;
} FO_PARAM;

enum {
	FO_SEARCH,
};

#define FOF_FILEOPERATIONLIST  0x0
#define FOF_SELECTEDFILELIST   0x1

#endif

#define VNM_SELECTVOLUME     (WM_APP+1210)

#define PPM_SETPATH          (WM_APP+125) // todo:
#define PPM_NOTIFY           (WM_APP+606) // todo:

FS_CLUSTER_INFORMATION *_CreateClusterInformationBuffer(PCWSTR pszFilePath);

typedef struct _VFS_FILE_STREAM_INFORMATION
{
    LARGE_INTEGER StreamSize;
    LARGE_INTEGER StreamAllocationSize;
    WCHAR *StreamName;
	INT Order;
} VFS_FILE_STREAM_INFORMATION;

HRESULT
GetAlternateStream(
	PCWSTR pszFilePath,
	VFS_FILE_STREAM_INFORMATION **pAltStmNames,
	INT *pAltStmNameCount
	);

inline INT GetAlternateStreamNameCount(FILE_STREAM_INFORMATION *StreamInformation)
{
	INT cNames = 0;
	FILE_STREAM_INFORMATION *p = StreamInformation;
	do
	{
		cNames++;
		if( p->NextEntryOffset == 0 )
			break;
		p = (FILE_STREAM_INFORMATION *)((ULONG_PTR)p + p->NextEntryOffset);
	}
	while( p != NULL );
	return cNames;
}

inline ULONG GetAlternateStreamNameTotalLength(FILE_STREAM_INFORMATION *StreamInformation)
{
	ULONG cb = 0;
	FILE_STREAM_INFORMATION *p = StreamInformation;
	do
	{
		cb += p->StreamNameLength;
		cb += sizeof(WCHAR); // C terminate null
		if( p->NextEntryOffset == 0 )
			break;
		p = (FILE_STREAM_INFORMATION *)((ULONG_PTR)p + p->NextEntryOffset);
	}
	while( p != NULL );
	return cb;
}
