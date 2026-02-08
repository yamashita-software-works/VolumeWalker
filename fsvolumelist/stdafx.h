#pragma once

#include "targetver.h"
#include "..\build_switch.inc"

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#undef WIN32_NO_STATUS       // defines STATUS_XXX in ntddk, now using includes DDK.
#include <ntstatus.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS      // no include STATUS_XXX in winnt.h
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <strsafe.h>
#include <winternl.h>        // WinSDK 7.1
#include <winioctl.h>
#include <commoncontrols.h>
#include <uxtheme.h>
#include <diskguid.h>
#include <setupapi.h>
#include <sddl.h>
#include <vssym32.h>

#if _MSC_VER <= 1500
#define nullptr NULL
#endif

#pragma warning(disable : 4995)

#include "debug.h"
#include "mem.h"
#include "multisz.h"
#include "libmisc.h"
#include "common.h"
#include "common_control_helper.h"
#include "common_msg.h"
#include "simplestack.h"
#include "stringbuffer.h"
#include "listhelp.h"
#include "libntwdk.h"
#include "ntwin32helper.h"
#include "winfsctl.h"
#include "common_resid.h"
#include "common_resid_appdef.h"
#include "appwindowdefs.h"
#include "..\fsvolumehelp\volumehelp.h"
#include "..\fsvolumehelp\physicaldriveinformationclass.h"
#include "..\fsvolumehelp\volumedevinfostruct.h"
#include "..\fsvolumehelp\storagedevice.h"
#include "volumeconsoledef.h"
#include "volumeconsoleid.h"
#include "interface.h"
#include "column.h"
#include "themehelp.h"
#if _ENABLE_DARK_MODE_TEST
#include "darkmode.h"
#include "listviewutil.h"
#else
inline BOOL _IsDarkModeEnabled() { return FALSE; }
#endif
#include "dialogs.h"

HWND CreateVolumeInformationWindow(HWND hWnd);
HWND CreateDiskLayoutWindow(HWND hWnd);
HWND CreatePhysicalDriveInformationWindow(HWND hWnd);
HWND CreateStorageDeviceWindow(HWND hWndParent);
HWND CreateMountedDeviceWindow(HWND hWndParent);
HWND CreateVolumeListWindow(HWND hWndParent);
HWND CreatePhysicalDriveListWindow(HWND hWndParent);
HWND CreateShadowCopyListWindow(HWND hWndParent);
HWND CreateDosDriveWindow(HWND hWndParent);
HWND CreateFileSystemStatisticsWindow(HWND hWndParent);
HWND CreateSimpleHexDumpWindow(HWND hWndParent);
HWND CreateFilterDriverWindow(HWND hWndParent);
HWND CreateDiskPerformanceWindow(HWND hWndParent);
HWND CreateVolumeMountPointWindow(HWND hWndParent);
HWND CreateEncryptionVolumeWindow(HWND hWndParent);
HWND CreateMountedDriveWindow(HWND hWndParent);
HWND CreateVDSConsoleWindow(HWND hWndParent);
HWND CreateScratchPadWindow(HWND hWndParent);

inline VOID _DrawFocusFrame(HWND hWnd,HDC hdc,RECT *prc,BOOL bDrawFocus=FALSE)
{
	const COLORREF crActiveFrame = RGB(80,110,190);
//	const COLORREF crDarkModeActiveFrame = RGB(172,172,172);
	const COLORREF crDarkModeActiveFrame = RGB(96,205,255);
	DrawFocusFrame(hWnd,hdc,prc,bDrawFocus,
			_IsDarkModeEnabled() ? crDarkModeActiveFrame : crActiveFrame);
}

HINSTANCE _GetResourceInstance();
HFONT GetGlobalFont(HWND hWnd);
HFONT GetIconFont();

#define _COLOR_BKGD_DIRTY_VOLUME    RGB(255,220,0)
#define _COLOR_TEXT_DIRTY_VOLUME    RGB(180,0,0)
#define _COLOR_TEXT_VIRTUALDISK     RGB(0,32,180)

inline VOID OpenHexDumpConsole(UINT ConsoleId,PCWSTR psz,LONGLONG StartOffset)
{
	OPEN_MDI_CHILDFRAME_PARAM open_mdi = {0};
	open_mdi.Flags    = 0;
	open_mdi.Path     = (PWSTR)psz;
	open_mdi.StartOffset.QuadPart = StartOffset;
	SendMessage(GetActiveWindow(),WM_OPEN_MDI_CHILDFRAME,MAKEWPARAM(ConsoleId,0),(LPARAM)&open_mdi);
}

inline VOID OpenConsole_SendMessage(UINT ConsoleId,PCWSTR psz)
{
	if( ConsoleId == VOLUME_CONSOLE_SIMPLEHEXDUMP )
	{
		OpenHexDumpConsole(ConsoleId,psz,0);
	}
	else
	{	
		SIZE_T cch = (wcslen(psz) + 1);
		OPEN_MDI_CHILDFRAME_PARAM *popen_mdi = (OPEN_MDI_CHILDFRAME_PARAM *)CoTaskMemAlloc( sizeof(OPEN_MDI_CHILDFRAME_PARAM) + (cch * sizeof(WCHAR)) );
		popen_mdi->Flags    = 0;
		popen_mdi->Path     = (PWSTR)(((UINT_PTR)popen_mdi)+sizeof(OPEN_MDI_CHILDFRAME_PARAM));
		StringCchCopy(popen_mdi->Path,cch,psz);
		PostMessage(GetActiveWindow(),WM_OPEN_MDI_CHILDFRAME,MAKEWPARAM(ConsoleId,1),(LPARAM)popen_mdi);
	}
}

enum {
	COLUMN_None=0,
    COLUMN_Name,
	COLUMN_CreationTime,
	COLUMN_Size,
	COLUMN_Free,
	COLUMN_Usage,
	COLUMN_UsageRate,
	COLUMN_Format,
	COLUMN_Guid,
	COLUMN_Drive,
	COLUMN_VendorId,
	COLUMN_ProductId,
	COLUMN_PartitionStyle,
	COLUMN_BusType,
	COLUMN_DeviceId,
	COLUMN_Identifier,
	COLUMN_OriginalDevice,
	COLUMN_OriginalVolume,
	COLUMN_SnapshotId,
	COLUMN_SnapshotSetId,
	COLUMN_Attributes,
	COLUMN_VolumeLabel,
	COLUMN_Path,
	COLUMN_Type,
	COLUMN_FileSystemType,
	COLUMN_Flags,
	COLUMN_Address,
	COLUMN_Offset,
	COLUMN_DumpHex,
	COLUMN_DumpChar,
	COLUMN_FilterName,
	COLUMN_FilterInstanceName,
	COLUMN_FilterAltitude,
	COLUMN_FilterVolumeName,
	COLUMN_FilterFrameId,
	COLUMN_InstallDate,
	COLUMN_FirstInstallDate,
	COLUMN_LastArrivalDate,
	COLUMN_LastRemovalDate,
	COLUMN_Volume,
	COLUMN_Device,
	COLUMN_Status,
	COLUMN_EncryptionMethod,
	COLUMN_ConversionStatus,
	COLUMN_PersistentVolumeID,
	COLUMN_MaxItem,
	COLUMN_MaxCount=COLUMN_MaxItem,
};

enum {
	COLUMN_VDS_None=0,
    COLUMN_VDS_Name,
    COLUMN_VDS_DiskAddress,
    COLUMN_VDS_FriendlyName,
    COLUMN_VDS_AdaptorName,
    COLUMN_VDS_DevicePath,
	COLUMN_VDS_PartitionStyle,
	COLUMN_VDS_Status,
	COLUMN_VDS_Size,
	COLUMN_VDS_BusType,
	COLUMN_VDS_MaxItem,
	COLUMN_VDS_MaxCount=COLUMN_VDS_MaxItem,
};

extern const COLUMN_NAME *GetColumnNameTable();
extern const int GetColumnNameTableItemCount();

EXTERN_C
HIMAGELIST
WINAPI
CreateVolumeImageList(
	int iImageList
	);
