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
