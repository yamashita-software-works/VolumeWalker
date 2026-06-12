#pragma once

//////////////////////////////////////////////////////////////////////////////
// Goto Sector/Cluster Dialog

#define DVL_LOC_OFFSET         0x0
#define DVL_LOC_SECTOR_NUMBER  0x1
#define DVL_LOC_NUMBER_MASK    0x1
#define DVL_LOC_PHYSICAL_DISK  0x4
#define DVL_HEX                0x80

typedef struct _DISK_VOLUME_SECTOR_CLUSTER_LOCATION
{
	LARGE_INTEGER Location;
	ULONG SectorClusterSize;
	ULONG Flags;
} DISK_VOLUME_SECTOR_CLUSTER_LOCATION,*PDISK_VOLUME_SECTOR_CLUSTER_LOCATION;

HRESULT GotoDialog(HWND hWnd,PDISK_VOLUME_SECTOR_CLUSTER_LOCATION pLoc);


//////////////////////////////////////////////////////////////////////////////
//
// Volume Tool Dialogs
//

//
// GUID Editor
//
typedef struct _GUIDEDITPARAM
{
	PWSTR pszWindowTitle;
	PWSTR pszMainInstruction;
} GUIDEDITPARAM;

EXTERN_C
HRESULT
WINAPI
GUIDEditDialog(
	__in HWND hWnd,
	__in DWORD dwFlags,
	__inout GUID *pGuid,
	__inout_opt PWSTR pszReturnGuidStringBuffer,
	__in_opt int cchReturnGuidStringBuffer,
	__in_opt GUIDEDITPARAM *Param
	);

#define GUIDEF_NO_INITIAL_EDITBOX      0x00000001

//
// Volume Label Editor
//
EXTERN_C
HRESULT
WINAPI
VolumeLabelEditDialog(
	__in HWND hWnd,
	__in PCWSTR pszVolumeName,
	__in DWORD dwFlags,
	__in_opt PCWSTR DefaultLabelName,
	__inout_opt PWSTR VolumeLabelName,
	__in_opt int cchVolumeLabelName
	);

//
// Volume Object ID Editor
//
EXTERN_C
HRESULT
WINAPI
VolumeObjectIdEditDialog(
	__in HWND hWnd,
	__in PCWSTR pszVolumeName,
	__in ULONG Flags
	);

//
// Lookup Stream Name
//
EXTERN_C
HRESULT
WINAPI
LookupStreamNameDialog(
	HWND hWnd,
	PWSTR pszVolueName,
	DWORD dwFlags
	);
