#pragma once
//***************************************************************************
//*                                                                         *
//*  page_physicaldriveinfo.h                                               *
//*                                                                         *
//*  Physical Drive Information Page                                        *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2023-08-17                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "string_def.h"
#include "diskfindvolume.h"

#define _STR_NA  L"---"

enum {
	ID_GROUP_PD_BASIC = 1,
	ID_GROUP_PD_GEOMETRY,
	ID_GROUP_PD_ACCESS_ALIGNMENT_DESCRIPTOR,
	ID_GROUP_PD_MBR,
	ID_GROUP_PD_GPT,
	ID_GROUP_PD_PARTITION,
};

typedef struct _PHYSICALDISKINFOITEM
{
	UINT Type;
} PHYSICALDISKINFOITEM, *PPHYSICALDISKINFOITEM;

struct CPhysicalDiskInfoItem : public PHYSICALDISKINFOITEM
{
	CPhysicalDiskInfoItem()
	{
		memset(this,0,sizeof(PHYSICALDISKINFOITEM));
	}
};

typedef struct _PHYSICALDISKINFOWNDEXTRA
{
	PVOID Reserved;
	WNDPROC pfnWndProcListView;
} PHYSICALDISKINFOWNDEXTRA;

class CPhysicalDiskInfoView : public CPageWndBase
{
	HWND m_hWndList;

	CPhysicalDriveInformation *m_pdi;

	DWORD m_dwDriveNumber;

	PWSTR m_pszPhysicalDrive;

	HFONT m_hFont;

public:
	CPhysicalDiskInfoView()
	{
		m_hWndList = NULL;
		m_pszPhysicalDrive = NULL;
		m_pdi = NULL;
		m_dwDriveNumber = (DWORD)-1;
		m_hFont = NULL;
	}

	~CPhysicalDiskInfoView()
	{
		_SafeMemFree(m_pszPhysicalDrive);

		if( m_pdi )
			delete m_pdi;
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_hFont = GetGlobalFont(hWnd);

		m_hWndList = CreateWindowEx(0,WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER, 
                              0,0,0,0,
                              hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		InitList(m_hWndList);
		InitGroup();

		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( m_hFont )
			DeleteObject(m_hFont);
		return 0;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;

		switch( pnmhdr->code )
		{
			case LVN_GETDISPINFO:
				return OnGetDispInfo(pnmhdr);
			case LVN_DELETEITEM:
				return OnDeleteItem(pnmhdr);
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
			case NM_CUSTOMDRAW:
				return OnCustomDraw(pnmhdr);
		}
		return 0;
	}

	LRESULT OnCustomDraw(NMHDR *pnmhdr)
	{
		NMLVCUSTOMDRAW *pnmlvcd = (NMLVCUSTOMDRAW *)pnmhdr;

		if( pnmlvcd->nmcd.hdr.hwndFrom != m_hWndList )
			return CDRF_DODEFAULT;

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			SelectObject(pnmlvcd->nmcd.hdc,m_hFont);
			return CDRF_NEWFONT;
		}

		return CDRF_DODEFAULT;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)pnmhdr);
		return 0;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;
		delete (CPhysicalDiskInfoItem *)pnmlv->lParam;
		return 0;
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;

		CPhysicalDiskInfoItem *pItem = (CPhysicalDiskInfoItem *)pdi->item.lParam;

		switch( pdi->item.iSubItem )
		{
			case 0:
				GetTitleText(pItem->Type,&pdi->item.pszText,pdi->item.cchTextMax);
				break;
			case 1:
				GetInfoText(pItem->Type,&pdi->item.pszText,pdi->item.cchTextMax);
				break;
		}
		return 0;	
	}

	VOID GetTitleText(int iItemType,PWSTR *pszText,int cchText)
	{
		PWSTR ref = 0;
		int cch;
		cch = LoadString(_GetResourceInstance(),iItemType,
			(LPWSTR)&ref,0);

		if( ref )
		{
			memcpy(*pszText,ref,cch*sizeof(WCHAR));
			(*pszText)[cch] = 0;
		}
		else
		{
			*pszText = L"???";
		}
	}

	VOID GetInfoText(int iItemType,PWSTR *pszText,int cchText)
	{
		WCHAR szBuffer[MAX_PATH]; // work buffer

		**pszText = L'\0';

		switch( iItemType )
		{
			case diPhysicalDriveName:
				*pszText = m_pszPhysicalDrive;
				break;
			case diPhysicalDiskSize:
				if( m_pdi->pGeometry )
					StrFormatByteSizeW(m_pdi->pGeometry->DiskSize.QuadPart,*pszText,cchText);
				break;
			case diPartitionStyle:
				if( m_pdi->pDriveLayout )
					*pszText = (PWSTR)GetPartitionStyleText(m_pdi->pDriveLayout->PartitionStyle);
				break;
			case diMBRSignature:
				if( m_pdi->pDriveLayout )
					StringCchPrintf(*pszText,cchText,L"0x%08X",m_pdi->pDriveLayout->Mbr.Signature);
				break;
			case diGPTDiskId:
				if( m_pdi->pDriveLayout )
					_MakeGUIDString(m_pdi->pDriveLayout->Gpt.DiskId,*pszText,cchText);
				break;
			case diGPTStartingUsableOffset:
				if( m_pdi->pDriveLayout )
					StringCchPrintf(*pszText,cchText,L"0x%I64X",m_pdi->pDriveLayout->Gpt.StartingUsableOffset.QuadPart);
				break;
			case diGPTUsableLength:
				if( m_pdi->pDriveLayout )
					StringCchPrintf(*pszText,cchText,L"0x%I64X",m_pdi->pDriveLayout->Gpt.UsableLength.QuadPart);
				break;
			case diGPTMaxPartitionCount:
				if( m_pdi->pDriveLayout )
					StringCchPrintf(*pszText,cchText,L"0x%X (%s)",m_pdi->pDriveLayout->Gpt.MaxPartitionCount,_CommaFormatString(m_pdi->pDriveLayout->Gpt.MaxPartitionCount,szBuffer));
				break;
			case diDiskSize:
				if( m_pdi->pGeometry ) {
					StrFormatByteSizeW(m_pdi->pGeometry->DiskSize.QuadPart,szBuffer,_countof(szBuffer));
					StringCchPrintf(*pszText,cchText,L"0x%016I64X (%s)",m_pdi->pGeometry->DiskSize.QuadPart,szBuffer);
				}
				break;
			case diCylinders:
				if( m_pdi->pGeometry )
					StringCchPrintf(*pszText,cchText,L"0x%016I64X (%s)",m_pdi->pGeometry->Geometry.Cylinders.QuadPart,_CommaFormatString(m_pdi->pGeometry->Geometry.Cylinders.QuadPart,szBuffer));
				break;
			case diTracksPerCylinder:
				if( m_pdi->pGeometry )
					StringCchPrintf(*pszText,cchText,L"0x%08X (%s)",m_pdi->pGeometry->Geometry.TracksPerCylinder,_CommaFormatString(m_pdi->pGeometry->Geometry.TracksPerCylinder,szBuffer));
				break;
			case diSectorsPerTrack:
				if( m_pdi->pGeometry )
					StringCchPrintf(*pszText,cchText,L"0x%08X (%s)",m_pdi->pGeometry->Geometry.SectorsPerTrack,_CommaFormatString(m_pdi->pGeometry->Geometry.SectorsPerTrack,szBuffer));
				break;
			case diBytesPerSector:
				if( m_pdi->pGeometry )
					StringCchPrintf(*pszText,cchText,L"0x%08X (%s)",m_pdi->pGeometry->Geometry.BytesPerSector,_CommaFormatString(m_pdi->pGeometry->Geometry.BytesPerSector,szBuffer));
				break;
			case diAlignmentBytesPerPhysicalSector:
				StringCchPrintf(*pszText,cchText,L"%lu bytes", m_pdi->Alignment.BytesPerPhysicalSector);
				break;
			case diAlignmentBytesPerLogicalSector:
				StringCchPrintf(*pszText,cchText,L"%lu bytes", m_pdi->Alignment.BytesPerLogicalSector);
				break;
			case diAlignmentBytesOffsetForSectorAlignment:
				StringCchPrintf(*pszText,cchText,L"%lu bytes", m_pdi->Alignment.BytesOffsetForSectorAlignment);
				break;
			case diAlignmentBytesPerCacheLine:
				StringCchPrintf(*pszText,cchText,L"%lu bytes", m_pdi->Alignment.BytesPerCacheLine);
				break;
			case diAlignmentBytesOffsetForCacheAlignment:
				StringCchPrintf(*pszText,cchText, L"%lu bytes", m_pdi->Alignment.BytesOffsetForCacheAlignment);
				break;
			case diVendorId:
			case diProductId:
			case diProductRevision:
			case diSerialNumber:
				if( m_pdi->pDeviceDescriptor ) {
					StringCchPrintf(*pszText,cchText,L"%S",DeviceDescriptor(iItemType,m_pdi->pDeviceDescriptor));
					StrTrim(*pszText,L" ");
				}
				break;
		}

		if( *(*pszText) == L'\0' )
			StringCchCopy(*pszText,cchText,_STR_NA);
	}

	PCHAR DeviceDescriptor(int iItemType,PSTORAGE_DEVICE_DESCRIPTOR pDeviceDescriptor)
	{
		CHAR __based(pDeviceDescriptor) *pstr = 0;

		CHAR szBuffer[256];
		PSTR pszText = szBuffer;

		if( iItemType == diVendorId )
		{
			PCHAR ven = pstr + pDeviceDescriptor->VendorIdOffset;
			if( pDeviceDescriptor->VendorIdOffset != 0 )
			{
				return ven;
			}
			return "";
		}

		if( iItemType == diProductId )
		{
			PCHAR pro = pstr + pDeviceDescriptor->ProductIdOffset;
			if( pDeviceDescriptor->ProductIdOffset != 0 )
			{
				return pro;
			}
			return "";
		}

		if( iItemType == diProductRevision )
		{
			PCHAR rev = pstr + pDeviceDescriptor->ProductRevisionOffset;
			if( pDeviceDescriptor->ProductRevisionOffset != 0 )
			{
				return rev;
			}
			return "";
		}

		if( iItemType == diSerialNumber )
		{
			PCHAR num = pstr + pDeviceDescriptor->SerialNumberOffset;
			if( pDeviceDescriptor->SerialNumberOffset != 0 )
			{
				return num;
			}
			return "";
		}
		return "";
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		HMENU hMenu = CreatePopupMenu();
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,(HWND)wParam,hMenu,pt,0);

		DestroyMenu(hMenu);

		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_SETFOCUS:
				SetFocus(m_hWndList);
				break;
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	VOID UpdateLayout(int cx,int cy,BOOL bColumnAdjust=FALSE)
	{
		if( m_hWndList )
		{
			SetRedraw(m_hWndList,FALSE);

			int c1 = ListView_GetColumnWidth(m_hWndList,0);
			int c2 = ListView_GetColumnWidth(m_hWndList,1);
			int cxList = cx;
			int cyList = cy;

			SetWindowPos(m_hWndList,NULL,
					0,0,
					cxList,cyList,
					SWP_NOZORDER);

			SetRedraw(m_hWndList,TRUE);
		}
	}

	int InsertGroup(HWND hWndList,int iGroupId,LPCWSTR pszHeaderText,int iImage=I_IMAGENONE,BOOL fCollapsed=FALSE,LPCWSTR pszSubTitle=NULL)
	{
		LVGROUP group = {0};

		group.cbSize    = sizeof(LVGROUP);
		group.mask = LVGF_GROUPID|LVGF_TITLEIMAGE|LVGF_HEADER|LVGF_STATE;
		group.iTitleImage = iImage;
		group.pszHeader = (LPWSTR)pszHeaderText;
		group.uAlign = LVGA_HEADER_LEFT;
		group.iGroupId  = iGroupId;
		group.state = LVGS_COLLAPSIBLE | (fCollapsed ? LVGS_COLLAPSED : 0);

		if( pszSubTitle )
		{
			group.mask |= LVGF_SUBTITLE;
			group.pszSubtitle = (LPWSTR)pszSubTitle;
		}

		return (int)ListView_InsertGroup(hWndList,-1,(PLVGROUP)&group);
	}

	typedef struct _GROUP_ITEM
	{
		int idGroup;
		PCWSTR Text;
	} GROUP_ITEM;

	void InitGroup()
	{
		GROUP_ITEM Group[] = {
			{ ID_GROUP_PD_BASIC,      L"Basic"  },
			{ ID_GROUP_PD_MBR,        L"MBR"  },
			{ ID_GROUP_PD_GPT,        L"GPT"  },
			{ ID_GROUP_PD_GEOMETRY,   L"Disk Geometry" },
			{ ID_GROUP_PD_ACCESS_ALIGNMENT_DESCRIPTOR, L"Access Alignment Descriptor" },
			{ ID_GROUP_PD_PARTITION,  L"Partition" },

		};
		int cGroupItem = ARRAYSIZE(Group);

		for(int i = 0; i < cGroupItem; i++)
		{
			InsertGroup(m_hWndList,Group[i].idGroup,Group[i].Text);
		}
	}

	HRESULT InitList(HWND hWndList)
	{
		_EnableVisualThemeStyle(hWndList);

		ListView_SetExtendedListViewStyle(hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_AUTOSIZECOLUMNS|LVS_EX_HEADERINALLVIEWS|LVS_EX_JUSTIFYCOLUMNS);

		HIMAGELIST himl = ImageList_Create(8,16,ILC_COLOR32,1,1);
		ListView_SetImageList(hWndList,himl,LVSIL_SMALL);

		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = 0;
		lvc.pszText = L"Item";
		lvc.cxMin = 240;
		lvc.cxIdeal = 240;
		lvc.cxDefault = 240;
		ListView_InsertColumn(hWndList,0,&lvc);

		lvc.cx = 0;
		lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		lvc.pszText = L"Value";
		ListView_InsertColumn(hWndList,1,&lvc);

		ListView_EnableGroupView(hWndList,TRUE);

		return S_OK;
	}

	int Insert(HWND hWndList,int iGroupId,int iItem, int iItemType)
	{
		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);

		CPhysicalDiskInfoItem *pItem = new CPhysicalDiskInfoItem;
		pItem->Type = iItemType;

		LVITEM lvi = {0};
		lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT|LVIF_PARAM|LVIF_GROUPID;
		lvi.iItem = iItem;
		lvi.iImage = I_IMAGENONE;
		lvi.iIndent = 1;
		lvi.lParam = (LPARAM)pItem;
		lvi.iGroupId = iGroupId;
		lvi.pszText = LPSTR_TEXTCALLBACK;

		iItem = ListView_InsertItem(hWndList,&lvi);

		ListView_SetItemText(hWndList,iItem,1,LPSTR_TEXTCALLBACK);

		return iItem;
	}

	INT Insert_BasicInfo(int iItem)
	{
		int iGroupId = ID_GROUP_PD_BASIC;

		UINT uInfoId[] = {
			diPhysicalDriveName,
			diPhysicalDiskSize,
			diPartitionStyle,

			diVendorId,
			diProductId,
			diProductRevision,
			diSerialNumber,
		};
		for(int i = 0; i < ARRAYSIZE(uInfoId); i++)
		{
			iItem = Insert(m_hWndList,iGroupId,iItem,uInfoId[i]);
		}

		if( m_pdi->m_dwDriveLayoutStatus == ERROR_SUCCESS )
		{
			if( m_pdi->pDriveLayout->PartitionStyle == PARTITION_STYLE_MBR )
			{
				iItem = Insert(m_hWndList,ID_GROUP_PD_MBR,iItem,diMBRSignature);
			}
			else if( m_pdi->pDriveLayout->PartitionStyle == PARTITION_STYLE_GPT ) 
			{
				iItem = Insert(m_hWndList,ID_GROUP_PD_GPT,iItem,diGPTDiskId);
				iItem = Insert(m_hWndList,ID_GROUP_PD_GPT,iItem,diGPTStartingUsableOffset);
				iItem = Insert(m_hWndList,ID_GROUP_PD_GPT,iItem,diGPTUsableLength);
				iItem = Insert(m_hWndList,ID_GROUP_PD_GPT,iItem,diGPTMaxPartitionCount);
			}
		}

		return iItem;
	}

	INT Insert_DiskGeometry(int iItem)
	{
		UINT uId[] = {
			diDiskSize,
			diCylinders,
			diTracksPerCylinder,
			diSectorsPerTrack,
			diBytesPerSector,
		};
		for(int i = 0; i < ARRAYSIZE(uId); i++)
		{
			iItem = Insert(m_hWndList,ID_GROUP_PD_GEOMETRY,iItem,uId[i]);
		}
		return iItem;
	}

	INT Insert_AccessAlignmentDescriptor(int iItem,CPhysicalDriveInformation *)
	{
		UINT uId[] = {
			diAlignmentBytesPerPhysicalSector,
			diAlignmentBytesPerLogicalSector,
			diAlignmentBytesOffsetForSectorAlignment,
			diAlignmentBytesPerCacheLine,
			diAlignmentBytesOffsetForCacheAlignment,
			diAlignmentBytesPerLogicalSector,
		};
		for(int i = 0; i < ARRAYSIZE(uId); i++)
		{
			iItem = Insert(m_hWndList,ID_GROUP_PD_ACCESS_ALIGNMENT_DESCRIPTOR,iItem,uId[i]);
		}
		return iItem;
	}

	int InsertItemString(int iItem,int iIndent,PCWSTR pszName,PCWSTR pszValume)
	{
		if( iItem == -1 )
			iItem = ListView_GetItemCount(m_hWndList);

		CPhysicalDiskInfoItem *pItem = new CPhysicalDiskInfoItem;
		pItem->Type = 0;

		LVITEM lvi = {0};
		lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT|LVIF_PARAM|LVIF_GROUPID;
		lvi.iItem = iItem;
		lvi.iImage = I_IMAGENONE;
		lvi.iIndent = iIndent;
		lvi.lParam = (LPARAM)pItem;
		lvi.iGroupId = ID_GROUP_PD_PARTITION;
		lvi.pszText = (LPWSTR)pszName;

		iItem = ListView_InsertItem(m_hWndList,&lvi);

		ListView_SetItemText(m_hWndList,iItem,1,(LPWSTR)pszValume);

		return iItem;
	}	

	VOID _cdecl InsertItemFormat(int iIndent,LPCWSTR pszName,LPCWSTR lpszFormat, ...)
	{
		va_list args;
		va_start(args, lpszFormat);

		int nBuf;
		WCHAR szBuffer[1024];
		size_t cch = sizeof(szBuffer) / sizeof(WCHAR);

		nBuf = _vsnwprintf_s(szBuffer, ARRAYSIZE(szBuffer), cch, lpszFormat, args);

		if( nBuf != -1 )
		{
			InsertItemString(-1,iIndent,pszName,szBuffer);
		}

		va_end(args);
	}

	VOID Insert_DriveLayout(DRIVE_LAYOUT_INFORMATION_EX *pdli)
	{
		DWORD i;
		WCHAR szText[512];
		WCHAR szStartOffset[64];
		WCHAR szLength[64];
		WCHAR szHeader[MAX_PATH];
		WCHAR szSubTitle[100];
		PCWSTR pszVolumeName;
		WCHAR szDrive[4];

		// future reserved options
		BOOLEAN bIncludeUnuseEntry = FALSE;
		BOOLEAN bInternalMode = FALSE;
		BOOLEAN bCenterHeaderTitle = FALSE;
		BOOLEAN bHeaderImage = TRUE;

		CDiskFindVolume m_findVolume;

		m_findVolume.Enum();

		for(i = 0; i < pdli->PartitionCount; i++)
		{
			if( pdli->PartitionEntry[i].PartitionNumber == 0 )
			{
				if( pdli->PartitionStyle == PARTITION_STYLE_MBR && 
					pdli->PartitionEntry[i].Mbr.PartitionType == PARTITION_ENTRY_UNUSED )
				{
					continue; // check MBR only
				}
			}

			if( i > 0 )
				InsertItemString(-1,0,L"",L"");

			pszVolumeName = NULL;
			szDrive[0] = L'\0';

			if( pdli->PartitionEntry[i].PartitionNumber &&
				pdli->PartitionEntry[i].PartitionLength.QuadPart != 0 )
			{
				if( m_findVolume.Find( m_dwDriveNumber,
									   pdli->PartitionEntry[i].StartingOffset,
									   pdli->PartitionEntry[i].PartitionLength,
									   pszVolumeName) )
				{
					GetVolumeDosName(pszVolumeName,szDrive,ARRAYSIZE(szDrive));
				}
			}

			szHeader[0] = 0;

		    DWORD PartitionNumber = pdli->PartitionEntry[i].PartitionNumber;

			if( !bCenterHeaderTitle || (bCenterHeaderTitle && !bHeaderImage) )
			{
				if( PartitionNumber > 0 )
					StringCchPrintf(szHeader,MAX_PATH,L"Partition%d",PartitionNumber);
				else
				{
					if( pdli->PartitionStyle == PARTITION_STYLE_MBR && 
					    (pdli->PartitionEntry[i].Mbr.PartitionType == PARTITION_EXTENDED || 
						 pdli->PartitionEntry[i].Mbr.PartitionType == PARTITION_XINT13_EXTENDED) )
					{
						StringCchPrintf(szHeader,MAX_PATH,L"Extended Partition");
					}
					else
					{
						StringCchPrintf(szHeader,MAX_PATH,L"Partition");
					}
				}
			}

			if( pdli->PartitionEntry[i].PartitionNumber != 0 )
			{
				StringCchPrintf(szSubTitle,ARRAYSIZE(szSubTitle),L"Harddisk%dPartition%d",
						m_dwDriveNumber,PartitionNumber);
				InsertItemString(-1,1,szHeader,szSubTitle);
			}
			else
			{
				if( pdli->PartitionStyle == PARTITION_STYLE_MBR )
					StringCchPrintf(szHeader,MAX_PATH,L"Extended");
				else
					StringCchPrintf(szHeader,MAX_PATH,L"Partition Entry %d",i);
				InsertItemString(-1,1,szHeader,L"");
			}

			int iIndent = 2;

			//
			//  Partition Start and Length
			//
			_FormatByteSize(pdli->PartitionEntry[i].StartingOffset.QuadPart,szStartOffset,_countof(szStartOffset));
			_FormatByteSize(pdli->PartitionEntry[i].PartitionLength.QuadPart,szLength,_countof(szLength));

			if( pdli->PartitionEntry[i].PartitionLength.QuadPart != 0 )
			{
				//
				// Volume/Drive
				//
				if( pszVolumeName )
					InsertItemFormat(iIndent,L"Volume",L"%s",pszVolumeName);

				if( szDrive[0] != L'\0' )
					InsertItemFormat(iIndent,L"Drive",L"%s",szDrive);

				InsertItemFormat(iIndent,L"Start",L"0x%I64X (%s)",
						pdli->PartitionEntry[i].StartingOffset,
						szStartOffset);

				InsertItemFormat(iIndent,L"Length",L"0x%I64X (%s)",
						pdli->PartitionEntry[i].PartitionLength,
						szLength);
			}
	
			//
			// Partition Information
			//
			if( pdli->PartitionStyle == PARTITION_STYLE_MBR )
			{
				if( (pdli->PartitionEntry[i].Mbr.PartitionType != PARTITION_ENTRY_UNUSED) || bIncludeUnuseEntry )
				{
					//
					// MBR
					//
					InsertItemFormat(iIndent,L"Type",L"%s (0x%02X)",
							GetPartitionTypeText(pdli->PartitionEntry[i].Mbr.PartitionType),
							pdli->PartitionEntry[i].Mbr.PartitionType);

					if( pdli->PartitionEntry[i].Mbr.PartitionType != PARTITION_ENTRY_UNUSED )
					{
						InsertItemFormat(iIndent,L"Hidden Sectors",L"0x%X (%s)",
								pdli->PartitionEntry[i].Mbr.HiddenSectors,
								_CommaFormatString(pdli->PartitionEntry[i].Mbr.HiddenSectors,szText));

						szText[0] = L'\0';
						if( pdli->PartitionEntry[i].Mbr.BootIndicator )
							StringCchCat(szText,_countof(szText),L"BOOT");

						if( pdli->PartitionEntry[i].Mbr.RecognizedPartition )
						{
							if( szText[0] )
								StringCchCat(szText,_countof(szText),L",");
							StringCchCat(szText,_countof(szText),L"RECOGNIZED");
						}

						if( szText[0] == L'\0' )
						{
							StringCchCopy(szText,_countof(szText),L"---");
						}

						InsertItemString(-1,iIndent,L"Boolean Flags",szText);

						szText[0] = L'\0';
						if( pdli->PartitionEntry[i].Mbr.BootIndicator )
						{
							InsertItemString(-1,iIndent,L"Boot",L"Yes");
						}
					}
				}
			}
			else if( pdli->PartitionStyle == PARTITION_STYLE_GPT )
			{
				//
				// GPT
				//
				InsertItemFormat(iIndent,L"Partition Type",L"%s",
					GetGPTPartitionTypeString(pdli->PartitionEntry[i].Gpt.PartitionType,szText,_countof(szText)));

				if( !IsEqualGUID(pdli->PartitionEntry[i].Gpt.PartitionType,PARTITION_ENTRY_UNUSED_GUID) )
				{
					InsertItemFormat(iIndent,L"Partition Name",L"%s",
							pdli->PartitionEntry[i].Gpt.Name[0] != L'\0' ? pdli->PartitionEntry[i].Gpt.Name : L"(None)");

					InsertItemFormat(iIndent,L"Partition Id",L"%s",
							_MakeGUIDString(pdli->PartitionEntry[i].Gpt.PartitionId,szText,_countof(szText))); // Unique GUID for this partition.

					// The Extensible Firmware Interface (EFI) attributes of the partition.
					InsertItemFormat(iIndent,L"Attributes",L"0x%I64X",
							pdli->PartitionEntry[i].Gpt.Attributes);

					int iBit;
					DWORD64 dwMask = 0x1;
					for(iBit = 0; iBit < 64; iBit++)
					{
						if( pdli->PartitionEntry[i].Gpt.Attributes & dwMask )
						{
							InsertItemFormat(iIndent,L"",L"%s",
									GetGPTAttribute((pdli->PartitionEntry[i].Gpt.Attributes & dwMask),szText,_countof(szText)));
						}
						dwMask <<= 1;
					}
				}

			}
			if( pdli->PartitionEntry[i].Mbr.PartitionType != PARTITION_ENTRY_UNUSED )
			{
				InsertItemFormat(iIndent,L"Rewrite Partition",L"%s",
						pdli->PartitionEntry[i].RewritePartition ? L"true" : L"false");
			}
		}

		m_findVolume.Free();
	}

	HRESULT FillItems(CPhysicalDriveInformation *pdi)
	{
		SetRedraw(m_hWndList,FALSE);

		//
		// Delete all information items
		//
		ListView_DeleteAllItems(m_hWndList);

		//
		// Update Data hold pointer
		//
		if( m_pdi )
			delete m_pdi;
		m_pdi = pdi;

		//
		// Start fill information items.
		//
		int iItem = 0;

		iItem = Insert_BasicInfo(iItem);

		if( m_pdi->pGeometry )
			iItem = Insert_DiskGeometry(iItem);

		if( m_pdi->IsValidAlignment() )
			iItem = Insert_AccessAlignmentDescriptor(iItem,pdi);

		Insert_DriveLayout(pdi->pDriveLayout);

		//
		// Adjust column width.
		//
		ListView_SetColumnWidth(m_hWndList,0,280);
		ListView_SetColumnWidth(m_hWndList,1,380);

		SetRedraw(m_hWndList,TRUE);

		return S_OK;
	}

	virtual HRESULT UpdateData(PVOID pData)
	{
		SELECT_ITEM *pSel = (SELECT_ITEM *)pData;

		if( pSel == NULL || pSel->pszName == NULL )
			return E_INVALIDARG;

		PWSTR pszPhysicalDisk = _MemAllocString(pSel->pszName);

		int cch = ((sizeof(L"PhysicalDrive")/sizeof(WCHAR))-1);
		DWORD dwDriveNumber = _wtoi( &pszPhysicalDisk[cch] );

		m_dwDriveNumber = dwDriveNumber;

		CPhysicalDriveInformation *pdi = new CPhysicalDriveInformation;

		if( pdi->OpenDisk(pszPhysicalDisk,dwDriveNumber) == S_OK )
		{
			_SafeMemFree(m_pszPhysicalDrive);
			m_pszPhysicalDrive = _MemAllocString(pszPhysicalDisk);

			pdi->GetGeometry();
			pdi->GetDriveLayout();
			pdi->GetDeviceIdDescriptor();
			pdi->GetDetectSectorSize();
		}

		_MemFree(pszPhysicalDisk);

		return FillItems(pdi);
	}

	virtual HRESULT InitLayout(const RECT *prc)
	{
		return S_OK;
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				OnCmdEditCopy();
				break;
			case ID_VIEW_REFRESH:
				OnCmdRefresh();
				break;
		}
		return S_OK;
	}

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				*State = ListView_GetSelectedCount(m_hWndList) ? UPDUI_ENABLED : UPDUI_DISABLED;
				return S_OK;
			case ID_VIEW_REFRESH:
				*State = UPDUI_ENABLED;
				return S_OK;
		}
		return S_FALSE;
	}

	void OnCmdEditCopy()
	{
		if( GetKeyState(VK_SHIFT) < 0 )
		{
			SetClipboardTextFromListViewColumn(m_hWndList,SCTEXT_FORMAT_SELECTONLY,1);
		}
		else
		{
			SetClipboardTextFromListView(m_hWndList,SCTEXT_UNICODE);
		}
	}

	void OnCmdRefresh()
	{
		ASSERT(m_pszPhysicalDrive != NULL);
		if( m_pszPhysicalDrive )
		{
			SELECT_ITEM sel = {0};
			sel.pszPath = m_pszPhysicalDrive;
			sel.pszName = m_pszPhysicalDrive;
			UpdateData(&sel);
		}
	}
};
