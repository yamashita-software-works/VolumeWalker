#pragma once

#include "targetver.h"

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

#if _MSC_VER <= 1500
#define nullptr NULL
#endif

#pragma warning(disable : 4995)

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
#include "appdef_resid.h"
#include "common_msg.h"
#include "listhelp.h"
#include "volumeconsoleid.h"
#include "stringbuffer.h"

#define  _ASSERT ASSERT

HINSTANCE _GetResourceInstance();

#include "filemisc.h"
#include "fsfilelib.h"

typedef struct _FILEITEMHEADER
{
	ULONG Reserved;
	PWSTR Path;
	PWSTR FileName;
} FILEITEMHEADER;

typedef struct _FILEITEM
{
	FILEITEMHEADER hdr;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    ULONG FileIndex;
    LARGE_INTEGER FileId;
    WCHAR ShortName[14];
} FILEITEM,*PFILEITEM;

class CFileItem : public FILEITEM
{
public:
	ULONG ItemTypeFlag;
	LARGE_INTEGER FirstLCN;
	LARGE_INTEGER ParentFileId; // todo: 128bit id not support

	CFileItem()
	{
		memset(this,0,sizeof(CFileItem));
		FirstLCN.QuadPart = 0;
	}

	CFileItem(PCWSTR pszDirPath,PCWSTR pszFile)
	{
		memset(this,0,sizeof(CFileItem));
		if( pszDirPath )
			hdr.Path = _MemAllocString(pszDirPath);
		if( pszFile )
			hdr.FileName = _MemAllocString(pszFile);
	}

	~CFileItem()
	{
		_SafeMemFree(hdr.Path);
		_SafeMemFree(hdr.FileName);
	}
};

typedef struct _FILELIST
{
	PWSTR RootPath;
	ULONG cItemCount;
	CFileItem **pFI;
} FILELIST,*PFILELIST;

HIMAGELIST GetGlobalShareImageList();
INT GetUpDirImageIndex();
HICON SetWindowIcon(HWND hWnd,SHSTOCKICONID ssii);
VOID DrawFocusFrame(HWND hWnd,HDC hdc,RECT *prc,BOOL bDrawFocus=FALSE,COLORREF crBorder=RGB(80,110,190));
PWSTR GetIniFilePath();
HFONT GetGlobalFont(HWND hWnd);
HFONT GetIconFont();

