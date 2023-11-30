#pragma once
//***************************************************************************
//*                                                                         *
//*  page_volumebasicinfo.h                                                 *
//*                                                                         *
//*  Volume Basic Information Page                                          *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2022-04-04                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "string_def.h"
#include "ntobjecthelp.h"

#define _STR_NA  L"---"

enum {
	ID_GROUP_GENERIC = 1,
	ID_GROUP_SIZE,
	ID_GROUP_MEDIATYPES,
	ID_GROUP_FS_NTFS,
	ID_GROUP_FS_FAT,
	ID_GROUP_FS_UDF,
	ID_GROUP_PHYSICALDRIVE,
	ID_GROUP_FILESYSTEM_ATTRIBUTES,
	ID_GROUP_NAME,
};

typedef struct _VOLBASICINFOITEM
{
	UINT Type;
} VOLBASICINFOITEM, *PVOLBASICINFOITEM;

struct CVolumeInfoItem : public VOLBASICINFOITEM
{
	CVolumeInfoItem()
	{
		memset(this,0,sizeof(VOLBASICINFOITEM));
	}
};

typedef struct _VOLUMEINFOWNDEXTRA
{
	PVOID Reserved;
	WNDPROC pfnWndProcListView;
} VOLUMEINFOWNDEXTRA;

class CVolumeBasicInfoView : public CPageWndBase
{
	HWND m_hWndList;

	VOLUME_DEVICE_INFORMATION *m_pvdi;
	PWSTR m_pszNtDeviceName;
	PWSTR m_pszVolumeGuid;
	PWSTR m_pszDrive;

	HFONT m_hFont;

public:
	CVolumeBasicInfoView()
	{
		m_hWndList = NULL;
		m_pvdi = NULL;
		m_pszNtDeviceName = NULL;
		m_pszVolumeGuid = NULL;
		m_pszDrive = NULL;
	}

	~CVolumeBasicInfoView()
	{
		if( m_pvdi != NULL )
			FreeVolumeInformation(m_pvdi);
		_SafeMemFree(m_pszNtDeviceName);
		_SafeMemFree(m_pszVolumeGuid);
		_SafeMemFree(m_pszDrive);
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_hFont = GetGlobalFont(hWnd,FALSE);

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
		delete (CVolumeInfoItem *)pnmlv->lParam;
		return 0;
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;

		CVolumeInfoItem *pItem = (CVolumeInfoItem *)pdi->item.lParam;

		switch( pdi->item.iSubItem )
		{
			case 0:
				GetTitleText(pItem->Type,&pdi->item.pszText,pdi->item.cchTextMax);
				break;
			case 1:
				if( pItem->Type < 100 )
					GetInfoText(pItem->Type,&pdi->item.pszText,pdi->item.cchTextMax);
				else if( pItem->Type < 200 )
					GetNtfsInfoText(pItem->Type,&pdi->item.pszText,pdi->item.cchTextMax);
				else if( 200 <= pItem->Type &&  pItem->Type <= 299 )
					GetUdfInfoText(pItem->Type,&pdi->item.pszText,pdi->item.cchTextMax);
				break;
		}
		return 0;	
	}

	VOID GetTitleText(int iItemType,PWSTR *pszText,int cchText)
	{
		PWSTR ref = 0;
		int cch;
		cch = LoadString(_GetResourceInstance(),iItemType,(LPWSTR)&ref,0);

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

	VOID makeSizeText(ULONGLONG cb,PWSTR *pszText,int cchText)
	{
		_CommaFormatString(cb,*pszText);

		{
			WCHAR sz[MAX_PATH];
			StrFormatByteSizeW(cb,sz,MAX_PATH);
			StringCchCat(*pszText,cchText,L" (");
			StringCchCat(*pszText,cchText,sz);
			StringCchCat(*pszText,cchText,L")");
		}
	}

	VOID GetInfoText(int iItemType,PWSTR *pszText,int cchText)
	{
		**pszText = L'\0';

		switch( iItemType )
		{
			case diNtDeviceName:
				if( m_pszNtDeviceName )
					*pszText = m_pszNtDeviceName;
				break;
			case diVolumeGuidName:
				if( m_pszVolumeGuid )
					*pszText = m_pszVolumeGuid;
				break;
			case diDriveName:
				if( m_pszDrive )
					*pszText = m_pszDrive;
				break;
			case diSize:
				if( m_pvdi->State.SizeInformation )
				{
					makeSizeText(
						m_pvdi->TotalAllocationUnits.QuadPart * m_pvdi->SectorsPerAllocationUnit * m_pvdi->BytesPerSector,
						pszText,cchText
						);
				}
				break;
			case diFree:
				if( m_pvdi->State.SizeInformation )
				{
					makeSizeText(
						m_pvdi->AvailableAllocationUnits.QuadPart * m_pvdi->SectorsPerAllocationUnit * m_pvdi->BytesPerSector,
						pszText,cchText
						);
				}
				break;
			case diUsage:
				if( m_pvdi->State.SizeInformation )
				{
					makeSizeText(
						(m_pvdi->TotalAllocationUnits.QuadPart * m_pvdi->SectorsPerAllocationUnit * m_pvdi->BytesPerSector)
						- (m_pvdi->AvailableAllocationUnits.QuadPart * m_pvdi->SectorsPerAllocationUnit * m_pvdi->BytesPerSector),
						pszText,cchText
						);
				}
				break;
			case diTotalAllocationUnits:
				if( m_pvdi->State.SizeInformation )
					_CommaFormatString(m_pvdi->TotalAllocationUnits.QuadPart,*pszText);
				break;
			case diAvailableAllocationUnits:
				if( m_pvdi->State.SizeInformation )
					_CommaFormatString(m_pvdi->AvailableAllocationUnits.QuadPart,*pszText);
				break;
			case diBytesPerSector:
				if( m_pvdi->State.SizeInformation )
					_CommaFormatString(m_pvdi->BytesPerSector,*pszText);
				break;
			case diSectorsPerAllocationUnit:
				if( m_pvdi->State.SizeInformation )
					_CommaFormatString(m_pvdi->SectorsPerAllocationUnit,*pszText);
				break;
			case diVolumeLabel:
				if( m_pvdi->State.VolumeInformaion )
					StringCchCopy(*pszText,cchText,m_pvdi->VolumeLabel);
				break;
			case diCreationTime:
				if( m_pvdi->State.VolumeInformaion ) {
					if( m_pvdi->VolumeCreationTime.QuadPart > 0 )
						_GetDateTimeStringEx(m_pvdi->VolumeCreationTime.QuadPart,*pszText,cchText,NULL,NULL,FALSE);
				}
				break;
			case diSerialNumber:
				if( m_pvdi->State.VolumeInformaion ) {
					if( m_pvdi->VolumeSerialNumber != (ULONG)0 )
					{
						StringCchPrintf(*pszText,cchText,L"%04X-%04X",
								HIWORD(m_pvdi->VolumeSerialNumber),LOWORD(m_pvdi->VolumeSerialNumber));
					}
				}
				break;
			case diFileSystem:
				if( m_pvdi->RecognitionFileSystem[0] != 0 )
					StringCchPrintf(*pszText,cchText,L"%S",m_pvdi->RecognitionFileSystem);
				else
					*pszText = m_pvdi->FileSystemName;
				break;
			case diSupportsObjects:
				if( m_pvdi->State.VolumeInformaion )
				{
					StringCchPrintf(*pszText,cchText,L"%s",m_pvdi->SupportsObjects ? L"true" :  L"false" );
				}
				break;
			case diObjectId:
				if( !IsEqualGUID(*((GUID*)m_pvdi->ObjectId.ObjectId),GUID_NULL) )
				{
					// GUID Style
					LPOLESTR pszGuid;
					StringFromIID( *((GUID*)m_pvdi->ObjectId.ObjectId),&pszGuid);
				    StringCchPrintf(*pszText,cchText,L"%s",pszGuid);
					CoTaskMemFree(pszGuid);
				}
				break;
			case diUniqueId:
			{
				PMOUNTDEV_UNIQUE_ID pUniqueId = m_pvdi->pUniqueId;
				if( pUniqueId )
				{
					if( pUniqueId->UniqueIdLength == 0xc )
					{
						StringCchPrintf(*pszText,cchText,L"0x%08X 0x%016I64X",
							*(ULONG *)&pUniqueId->UniqueId[0],
							*(LONG64 *)&pUniqueId->UniqueId[4]);
					}
					else
					{
						if( pUniqueId->UniqueId[0] == 'D' &&
							pUniqueId->UniqueId[1] == 'M' &&
							pUniqueId->UniqueId[2] == 'I' &&
							pUniqueId->UniqueId[3] == 'O' )
						{
							//  01234567
							// "DMIO:ID:"
							GUID guid;
							memcpy(&guid,&pUniqueId->UniqueId[8],sizeof(GUID));
							LPOLESTR pszGuid;
							StringFromIID(guid,&pszGuid);
							StringCchPrintf(*pszText,cchText,L"DMIO:ID:%s",pszGuid);
							CoTaskMemFree(pszGuid);
						}
						else
						{
							UNICODE_STRING us;
							us.Buffer = (PWSTR)pUniqueId->UniqueId;
							us.Length = pUniqueId->UniqueIdLength;
							us.MaximumLength = us.Length;
							StringCchPrintf(*pszText,cchText,L"%wZ",&us);
						}
					}
				}
				break;
			}
			case diDeviceNumber:
			{
				// NOTE:
				// The STORAGE_DEVICE_NUMBER cannot be obtained when the volume is divided into multiple partitions.
				//
				STORAGE_DEVICE_NUMBER *pDeviceNumber = &m_pvdi->DeviceNumber;
				if( pDeviceNumber->DeviceType != 0 )
				{
					if( pDeviceNumber->DeviceNumber != (ULONG)-1 && pDeviceNumber->PartitionNumber != (ULONG)-1 )
					{
						StringCchPrintf(*pszText,cchText,
								L"Harddisk%u Partition%u",
								pDeviceNumber->DeviceNumber,
								pDeviceNumber->PartitionNumber);
					}
				}
				break;
			}
			case diDirtyBit:
			{
				if( m_pvdi->DirtyBit != -1 )
					*pszText = m_pvdi->DirtyBit ? L"true" : L"false";
				else
					*pszText = _STR_NA;
				break;
			}
			case diRetrievalPointerBase:
			{
				if( m_pvdi->RetrievalPointerBase.FileAreaOffset.QuadPart != -1 )
				{
					LONG64 clusters = m_pvdi->RetrievalPointerBase.FileAreaOffset.QuadPart;
					LONG64 offset = m_pvdi->RetrievalPointerBase.FileAreaOffset.QuadPart * m_pvdi->BytesPerSector;

					WCHAR buf1[32];
					WCHAR buf2[32];
					StringCchPrintf(*pszText,cchText,L"%s sectors (offset : %s)",
						_CommaFormatString(clusters,buf1),
						_CommaFormatString(offset,buf2));
				}
				else
					*pszText = _STR_NA;
				break;
			}
		}

		if( *(*pszText) == L'\0' )
			StringCchCopy(*pszText,cchText,_STR_NA);

	}

	VOID GetNtfsInfoText(int iItemType,PWSTR *pszText,int cchText)
	{
		**pszText = L'\0';

		NTFS_VOLUME_DATA_BUFFER *pNtfsVolumeData = &m_pvdi->ntfs.data;

		switch( iItemType )
		{
			case diNtfsVolumeSerialNumber:
				StringCchPrintf(*pszText,cchText,L"0x%016I64X", pNtfsVolumeData->VolumeSerialNumber.QuadPart);
				break;
			case diNtfsNumberSectors:
				_CommaFormatString(pNtfsVolumeData->NumberSectors.QuadPart,*pszText);
				break;
			case diNtfsTotalClusters:
				_CommaFormatString(pNtfsVolumeData->TotalClusters.QuadPart,*pszText);
				break;
			case diNtfsFreeClusters:
				_CommaFormatString(pNtfsVolumeData->FreeClusters.QuadPart,*pszText);
				break;
			case diNtfsTotalReserved:
				_CommaFormatString(pNtfsVolumeData->TotalReserved.QuadPart,*pszText);
				break;
			case diNtfsBytesPerSector:
				_CommaFormatString(pNtfsVolumeData->BytesPerSector,*pszText);
				break;
			case diNtfsBytesPerCluster:
				_CommaFormatString(pNtfsVolumeData->BytesPerCluster,*pszText);
				break;
			case diNtfsBytesPerFileRecordSegment:
				_CommaFormatString(pNtfsVolumeData->BytesPerFileRecordSegment,*pszText);
				break;
			case diNtfsClustersPerFileRecordSegment:
				_CommaFormatString(pNtfsVolumeData->ClustersPerFileRecordSegment,*pszText);
				break;
			case diNtfsMftValidDataLength:
				_CommaFormatString(pNtfsVolumeData->MftValidDataLength.QuadPart,*pszText);
				break;
			case diNtfsMftStartLcn:
				StringCchPrintf(*pszText,cchText,L"0x%I64X",pNtfsVolumeData->MftStartLcn.QuadPart);
				break;
			case diNtfsMft2StartLcn:
				StringCchPrintf(*pszText,cchText,L"0x%I64X",pNtfsVolumeData->Mft2StartLcn.QuadPart);
				break;
			case diNtfsMftZoneStart:
				StringCchPrintf(*pszText,cchText,L"0x%I64X",pNtfsVolumeData->MftZoneStart.QuadPart);
				break;
			case diNtfsMftZoneEnd:
				StringCchPrintf(*pszText,cchText,L"0x%I64X",pNtfsVolumeData->MftZoneEnd.QuadPart);
				break;
		}
	}

	VOID GetUdfInfoText(int iItemType,PWSTR *pszText,int cchText)
	{
		**pszText = L'\0';

		FILE_QUERY_ON_DISK_VOL_INFO_BUFFER *pUdfInfo = &m_pvdi->udf.DiskVolumeInfo;

		switch( iItemType )
		{
			case diUdfDirectoryCount:
				_CommaFormatString(pUdfInfo->DirectoryCount.QuadPart,*pszText);
				break;
			case diUdfFileCount:
				_CommaFormatString(pUdfInfo->FileCount.QuadPart,*pszText);
				break;
			case diUdfFsFormatVersion:
				StringCchPrintf(*pszText,cchText,L"%u.%u",pUdfInfo->FsFormatMajVersion,pUdfInfo->FsFormatMinVersion);
				break;
			case diUdfFsFormatName:
				StringCchCopy(*pszText,cchText,pUdfInfo->FsFormatName);
				break;
			case diUdfFormatTime:
				_GetDateTimeStringEx(pUdfInfo->FormatTime.QuadPart,*pszText,cchText,NULL,NULL,FALSE);
				break;
			case diUdfLastUpdateTime:
				_GetDateTimeStringEx(pUdfInfo->LastUpdateTime.QuadPart,*pszText,cchText,NULL,NULL,FALSE);
				break;
			case diUdfCopyrightInfo:
				StringCchCopy(*pszText,cchText,pUdfInfo->CopyrightInfo);
				break;
			case diUdfAbstractInfo:
				StringCchCopy(*pszText,cchText,pUdfInfo->AbstractInfo);
				break;
			case diUdfFormattingImplementationInfo:
				StringCchCopy(*pszText,cchText,pUdfInfo->FormattingImplementationInfo);
				break;
    		case diUdfLastModifyingImplementationInfo:
				StringCchCopy(*pszText,cchText,pUdfInfo->LastModifyingImplementationInfo);
				break;
		}
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
			{ ID_GROUP_NAME,                  L"Name" },
			{ ID_GROUP_SIZE,                  L"Size"  },
			{ ID_GROUP_GENERIC,               L"Generic" },
			{ ID_GROUP_PHYSICALDRIVE,         L"Physcial Drives" },
			{ ID_GROUP_FS_NTFS,               L"NTFS"  },
			{ ID_GROUP_FS_FAT,                L"FAT"  },
			{ ID_GROUP_FS_UDF,                L"UDF"  },
			{ ID_GROUP_FILESYSTEM_ATTRIBUTES, L"File System Attributes" },
			{ ID_GROUP_MEDIATYPES,            L"Media Types" },
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

		ListView_SetExtendedListViewStyle(hWndList,
			LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_AUTOSIZECOLUMNS|
			LVS_EX_HEADERINALLVIEWS|LVS_EX_JUSTIFYCOLUMNS|LVS_EX_LABELTIP);

		HIMAGELIST himl = ImageList_Create(8,16,ILC_COLOR32,1,1);
		ListView_SetImageList(hWndList,himl,LVSIL_SMALL);

		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = 0;
		lvc.pszText = L"Item";
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

		CVolumeInfoItem *pItem = new CVolumeInfoItem;
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

	INT Insert_NameInfo(int iItem,VOLUME_DEVICE_INFORMATION *pvdi)
	{
		int iGroupId = ID_GROUP_NAME;

		UINT uInfoId[] = {
			diNtDeviceName,
			diVolumeGuidName,
			diDriveName,
		};
		for(int i = 0; i < ARRAYSIZE(uInfoId); i++)
		{
			iItem = Insert(m_hWndList,iGroupId,iItem,uInfoId[i]);
		}
		return iItem;
	}

	INT Insert_SizeInfo(int iItem,VOLUME_DEVICE_INFORMATION *pvdi)
	{
		int iGroupId = ID_GROUP_SIZE;

		UINT uInfoId[] = {
			diSize,
			diUsage,
			diFree,
			diTotalAllocationUnits,
			diAvailableAllocationUnits,
			diSectorsPerAllocationUnit,
			diBytesPerSector,
		};
		for(int i = 0; i < ARRAYSIZE(uInfoId); i++)
		{
			iItem = Insert(m_hWndList,iGroupId,iItem,uInfoId[i]);
		}
		return iItem;
	}

	INT Insert_BasicInfo(int iItem,VOLUME_DEVICE_INFORMATION *pvdi)
	{
		int iGroupId = ID_GROUP_GENERIC;

		UINT uInfoId[] = {
			diFileSystem,
			diVolumeLabel,
			diSerialNumber,
			diCreationTime,
			diSupportsObjects,
			diUniqueId,
			diObjectId,
			diDeviceNumber,
			diDirtyBit,
			diRetrievalPointerBase,
		};
		for(int i = 0; i < ARRAYSIZE(uInfoId); i++)
		{
			iItem = Insert(m_hWndList,iGroupId,iItem,uInfoId[i]);
		}
		return iItem;
	}

	INT Insert_NTFSInfo(int iItem,VOLUME_DEVICE_INFORMATION *pvdi)
	{
		int iGroupId = ID_GROUP_FS_NTFS;

		UINT uInfoId[] = {
			diNtfsVolumeSerialNumber,
			diNtfsNumberSectors,
			diNtfsTotalClusters,
			diNtfsFreeClusters,
			diNtfsTotalReserved,
			diNtfsBytesPerSector,
			diNtfsBytesPerCluster,
			diNtfsBytesPerFileRecordSegment,
			diNtfsClustersPerFileRecordSegment,
			diNtfsMftValidDataLength,
			diNtfsMftStartLcn,
			diNtfsMft2StartLcn,
			diNtfsMftZoneStart,
			diNtfsMftZoneEnd,
		};
		for(int i = 0; i < ARRAYSIZE(uInfoId); i++)
		{
			iItem = Insert(m_hWndList,iGroupId,iItem,uInfoId[i]);
		}
		return iItem;
	}

	INT Insert_UDFInfo(int iItem,VOLUME_DEVICE_INFORMATION *pvdi)
	{
		int iGroupId = ID_GROUP_FS_UDF;

		UINT uInfoId[] = {
		    diUdfDirectoryCount,
		    diUdfFileCount,
		    diUdfFsFormatVersion,
		    diUdfFsFormatName,
		    diUdfFormatTime,
		    diUdfLastUpdateTime,
		    diUdfCopyrightInfo,
		    diUdfAbstractInfo,
			diUdfFormattingImplementationInfo,
			diUdfLastModifyingImplementationInfo,
		};
		for(int i = 0; i < ARRAYSIZE(uInfoId); i++)
		{
			iItem = Insert(m_hWndList,iGroupId,iItem,uInfoId[i]);
		}
		return iItem;
	}

	HRESULT FillItems(VOLUME_DEVICE_INFORMATION *pvdi)
	{
		SetRedraw(m_hWndList,FALSE);

		//
		// Update m_pvdi buffer pointer
		//
		if( m_pvdi != NULL )
			FreeVolumeInformation(m_pvdi);
		m_pvdi = pvdi;

		BOOL bFirst = (ListView_GetItemCount(m_hWndList) == 0 ? TRUE : FALSE);

		//
		// Delete all information items
		//
		ListView_DeleteAllItems(m_hWndList);

		//
		// Start fill information items.
		//
		int iItem = 0;

		iItem = Insert_NameInfo(iItem,pvdi);
		iItem = Insert_SizeInfo(iItem,pvdi);
		iItem = Insert_BasicInfo(iItem,pvdi);

		if( _wcsicmp(pvdi->FileSystemName,L"NTFS") == 0 )
			iItem = Insert_NTFSInfo(iItem,pvdi);

		if( _wcsicmp(pvdi->FileSystemName,L"UDF") == 0 && pvdi->State.UdfData )
			iItem = Insert_UDFInfo(iItem,pvdi);

		FillDeviceCharacteristics(pvdi->Characteristics);

		if( pvdi->State.AttributeInformation )
			FillFileSystemeAttributes(pvdi->FileSystemAttributes);

		if( pvdi->pVolumeDiskExtents )
			FillExtentInformation(pvdi->pVolumeDiskExtents);

		if( pvdi->pMediaTypes )
			FillMediaTypes(pvdi->pMediaTypes);

		//
		// Adjust column width.
		//
		ListView_SetColumnWidth(m_hWndList,0,_DPI_Adjust_X(280));
		ListView_SetColumnWidth(m_hWndList,1,_DPI_Adjust_X(380));

		SetRedraw(m_hWndList,TRUE);

		return S_OK;
	}

	virtual HRESULT UpdateData(PVOID pData)
	{
		SELECT_ITEM *pSel = (SELECT_ITEM *)pData;

		if( pSel == NULL || pSel->pszPath == NULL )
		{
			return E_INVALIDARG;
		}

		PWSTR pszVolumeName = _MemAllocString(pSel->pszPath);
		if( pszVolumeName != NULL )
		{
			ULONG Type;
			Type = GetPathType(pszVolumeName);
			//PATHTYPE_NT_DEVICE

			_SafeMemFree(m_pszNtDeviceName);
			m_pszNtDeviceName = _MemAllocString(pszVolumeName);

			_SafeMemFree(pszVolumeName);
		}
		else
		{
			return E_OUTOFMEMORY;
		}

		ULONG OpenFlags = 0;
		if( IsUserAnAdmin() )
			OpenFlags = OPEN_VOLUME_READ_DATA;

		PVOID InformaionBuffer = NULL;
		GetherVolumeInformation(m_pszNtDeviceName,0,OpenFlags,(void **)&InformaionBuffer);
		if( InformaionBuffer == NULL )
		{
			return E_FAIL;
		}

		WCHAR szVolumeGuid[64];
		WCHAR szDrives[MAX_PATH];
		ZeroMemory(szVolumeGuid,sizeof(szVolumeGuid));
		ZeroMemory(szDrives,sizeof(szDrives));

		_SafeMemFree(m_pszVolumeGuid);
		_SafeMemFree(m_pszDrive);

		if( GetVolumeGuidName(m_pszNtDeviceName,szVolumeGuid,ARRAYSIZE(szVolumeGuid),0) == S_OK )
		{
			WCHAR szWin32GuidRoot[64];
			StringCchPrintf(szWin32GuidRoot,ARRAYSIZE(szWin32GuidRoot),L"\\\\?\\%s\\",szVolumeGuid);
			GetVolumeDrivePathsString(szWin32GuidRoot,szDrives,ARRAYSIZE(szDrives));
		}
		else
		{
			HANDLE h;
			if( EnumDosDeviceTargetNames(&h,m_pszNtDeviceName,EDDTNF_LOCAL) == STATUS_SUCCESS )
			{
				int c = GetDosDeviceTargetNamesCount(h);
				if( c > 0 )
				{
					StringCchCopy(szDrives,ARRAYSIZE(szDrives),GetDosDeviceTargetNamesItem(h,0,NULL));
				}
				FreeDosDeviceTargetNames(h);
			}
		}


	if( szVolumeGuid[0] != 0 )
		m_pszVolumeGuid = _MemAllocString(szVolumeGuid);

	if( szDrives[0] != 0 )
		m_pszDrive = _MemAllocString(szDrives);

		return FillItems((VOLUME_DEVICE_INFORMATION*)InformaionBuffer);
	}

	//
	// String item insert directly.
	//

	VOID InsertItemString(int iIndent,PCWSTR pszName,PCWSTR pszValue,int iGroupId)
	{
		int iItem = ListView_GetItemCount(m_hWndList);

		LVITEM lvi = {0};
		lvi.mask     = LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE|LVIF_GROUPID|LVIF_INDENT;
		lvi.iItem    = iItem;
		lvi.pszText  = (LPWSTR)pszName;
		lvi.iImage   = 0;
		lvi.lParam   = (LPARAM)0;
		lvi.iGroupId = iGroupId;
		lvi.iIndent  = iIndent;
		iItem = ListView_InsertItem(m_hWndList,&lvi);

		if( pszValue )
			ListView_SetItemText(m_hWndList,iItem,1,(LPWSTR)pszValue);
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
			InsertItemString(iIndent,pszName,szBuffer,ID_GROUP_MEDIATYPES);
		}

		va_end(args);
	}

	VOID FillMediaInfo(DEVICE_MEDIA_INFO *pMediaInfo,DWORD MediaInfoCount)
	{
		DWORD i;
		WCHAR szText[MAX_PATH];

		const int iIndent = 1;

		InsertItemString(iIndent,L"",L"",ID_GROUP_MEDIATYPES);

		for(i = 0; i < MediaInfoCount; i++)
		{
			InsertItemFormat(iIndent,L"Media Type",L"%s", 
					GetVolumeTypeString(CSTR_MEDIATYPE,pMediaInfo[i].DeviceSpecific.DiskInfo.MediaType,szText,_countof(szText)),ID_GROUP_MEDIATYPES);
			InsertItemFormat(iIndent,L"Cylinders",L"%s", 
					_CommaFormatString(pMediaInfo[i].DeviceSpecific.DiskInfo.Cylinders.QuadPart,szText),ID_GROUP_MEDIATYPES);
			InsertItemFormat(iIndent,L"Bytes/Sector",L"%s", 
					_CommaFormatString(pMediaInfo[i].DeviceSpecific.DiskInfo.BytesPerSector,szText),ID_GROUP_MEDIATYPES);
			InsertItemFormat(iIndent,L"Sectors/Track",L"%s", 
					_CommaFormatString(pMediaInfo[i].DeviceSpecific.DiskInfo.SectorsPerTrack,szText),ID_GROUP_MEDIATYPES);
			InsertItemFormat(iIndent,L"Tracks/Cylinder",L"%s", 
					_CommaFormatString(pMediaInfo[i].DeviceSpecific.DiskInfo.TracksPerCylinder,szText),ID_GROUP_MEDIATYPES);
			InsertItemFormat(iIndent,L"Number of Media Sides",L"%s", 
					_CommaFormatString(pMediaInfo[i].DeviceSpecific.DiskInfo.NumberMediaSides,szText),ID_GROUP_MEDIATYPES);
			InsertItemFormat(iIndent,L"Media Characteristics",L"0x%08X", 
					pMediaInfo[i].DeviceSpecific.DiskInfo.MediaCharacteristics,ID_GROUP_MEDIATYPES);

			WCHAR szLabel[32];
			DWORD dw;
			DWORD dwMask = 0x80000000;
			for(dw = 0; dw < 32; dw++)
			{
				if( pMediaInfo[i].DeviceSpecific.DiskInfo.MediaCharacteristics & dwMask )
				{
					StringCchPrintf(szLabel,ARRAYSIZE(szLabel),L"0x%08X",dwMask);
					InsertItemFormat(iIndent+1,szLabel,L"%s",
							GetMediaCharacteristicsString((pMediaInfo[i].DeviceSpecific.DiskInfo.MediaCharacteristics & dwMask),szText,_countof(szText)),ID_GROUP_MEDIATYPES);
				}
				dwMask >>= 1;
			}

			if( i < (MediaInfoCount - 1) )
				InsertItemString(iIndent,L"",L"",ID_GROUP_MEDIATYPES);
		}
	}

	VOID FillMediaTypes(GET_MEDIA_TYPES *pMediaTypes)
	{
		WCHAR szText[MAX_PATH];
		const int iIndent = 1;

		GetVolumeTypeString(CSTR_DEVICETYPE,pMediaTypes->DeviceType,szText,_countof(szText));

		InsertItemFormat(iIndent,L"Device Type",L"%s (%u)",szText,pMediaTypes->DeviceType);

		switch( pMediaTypes->DeviceType )
		{
			case FILE_DEVICE_DISK:
				FillMediaInfo(pMediaTypes->MediaInfo,pMediaTypes->MediaInfoCount);
				break;

			case FILE_DEVICE_CD_ROM:
			case FILE_DEVICE_DVD:
				FillMediaInfo(pMediaTypes->MediaInfo,pMediaTypes->MediaInfoCount);  // same as disk
				break;

			case FILE_DEVICE_TAPE:
				// todo:
				break;
		}
	}

	VOID FillExtentInformation(PVOLUME_DISK_EXTENTS pVolumeDiskExtents)
	{
		if( pVolumeDiskExtents == NULL )
			return;

		int iItem = ListView_GetItemCount(m_hWndList);

		WCHAR szBuffer[MAX_PATH];
		WCHAR szText[64];
		LVITEM lvi = {0};

		DWORD i;
		LONGLONG cb;

		for(i = 0; i < pVolumeDiskExtents->NumberOfDiskExtents; i++)
		{
			lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM|LVIF_INDENT|LVIF_GROUPID;
			lvi.pszText = szBuffer;
			lvi.lParam = 0;
			lvi.iImage = 0;

			//
			// Physical Device
			//
			StringCchPrintf(szBuffer,MAX_PATH,L"Extent #%u",i + 1);
			lvi.iItem = iItem++;
			lvi.iIndent = 1;
			lvi.iGroupId = ID_GROUP_PHYSICALDRIVE;
			lvi.pszText = szBuffer;
			iItem = ListView_InsertItem(m_hWndList,&lvi);

			StringCchPrintf(szBuffer,MAX_PATH,L"PhysicalDrive%u",pVolumeDiskExtents->Extents[i].DiskNumber);
			ListView_SetItemText(m_hWndList,iItem,1,szBuffer);

			iItem++;

			//
			// Starting Offset
			//
			StringCchPrintf(szBuffer,MAX_PATH,L"Starting Offset");
			lvi.iItem = iItem;
			lvi.iIndent = 2;
			lvi.iGroupId = ID_GROUP_PHYSICALDRIVE;
			iItem = ListView_InsertItem(m_hWndList,&lvi);

			cb = pVolumeDiskExtents->Extents[i].StartingOffset.QuadPart;
			StrFormatByteSizeW(cb,szText,_countof(szText));
			StringCchPrintf(szBuffer,MAX_PATH,L"0x%I64X (%s)",cb,szText);
			ListView_SetItemText(m_hWndList,iItem,1,szBuffer);

			iItem++;

			//
			// Extent Length
			//
			StringCchPrintf(szBuffer,MAX_PATH,L"Extent Length");
			lvi.iItem = iItem;
			lvi.iIndent = 2;
			lvi.iGroupId = ID_GROUP_PHYSICALDRIVE;
			iItem = ListView_InsertItem(m_hWndList,&lvi);

			cb = pVolumeDiskExtents->Extents[i].ExtentLength.QuadPart;
			StrFormatByteSizeW(cb,szText,_countof(szText));
			StringCchPrintf(szBuffer,MAX_PATH,L"0x%I64X (%s)",cb,szText);
			ListView_SetItemText(m_hWndList,iItem,1,szBuffer);

			iItem++;
		}
	}

	VOID FillDeviceCharacteristics(DWORD dwCharacteristics)
	{
		WCHAR sz[MAX_PATH];
		LVITEM lvi = {0};

		int iItem = ListView_GetItemCount(m_hWndList);

		// Attribute Bit
		lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM|LVIF_INDENT|LVIF_GROUPID;
		lvi.pszText = L"Device Characteristics";
		lvi.lParam = 0;
		lvi.iImage = 0;
		lvi.iItem = iItem;
		lvi.iIndent = 1;
		lvi.iGroupId = ID_GROUP_GENERIC;
		ListView_InsertItem(m_hWndList,&lvi);

		if( dwCharacteristics != 0 )
		{
			StringCchPrintf(sz,MAX_PATH,L"0x%08X",dwCharacteristics);
			ListView_SetItemText(m_hWndList,iItem,1,sz);
		}
		else
		{
			ListView_SetItemText(m_hWndList,iItem,1,_STR_NA);
		}

		iItem++;

		//
		// Characteristics Flag String
		//
		if( dwCharacteristics != 0 )
		{
			int i;
			WCHAR szLabel[32];
			DWORD dwFlag = 0;
			for( i = 0; GetDeviceCharacteristicsFlag(i,&dwFlag); i++ )
			{
				if( dwCharacteristics & dwFlag )
				{
					GetDeviceCharacteristicsString(i,sz,MAX_PATH);

					StringCchPrintf(szLabel,ARRAYSIZE(szLabel),L"0x%08X",dwFlag);

					lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM|LVIF_INDENT|LVIF_GROUPID;
					lvi.pszText = szLabel;
					lvi.lParam = 0;
					lvi.iImage = 0;
					lvi.iItem = iItem;
					lvi.iIndent = 2;
					lvi.iGroupId = ID_GROUP_GENERIC;
					ListView_InsertItem(m_hWndList,&lvi);

					ListView_SetItemText(m_hWndList,iItem,1,sz);

					iItem++;
				}
			}
		}
	}

	VOID FillFileSystemeAttributes(DWORD dwFileSystemAttributes)
	{
		WCHAR sz[MAX_PATH];
		LVITEM lvi = {0};

		int iItem = ListView_GetItemCount(m_hWndList);

		// Attribute Bit
		lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM|LVIF_INDENT|LVIF_GROUPID;
		lvi.pszText = L"FileSystem Attributes";
		lvi.lParam = 0;
		lvi.iImage = 0;
		lvi.iItem = iItem;
		lvi.iIndent = 1;
		lvi.iGroupId = ID_GROUP_FILESYSTEM_ATTRIBUTES;
		ListView_InsertItem(m_hWndList,&lvi);

		StringCchPrintf(sz,MAX_PATH,L"0x%08X",dwFileSystemAttributes);
		ListView_SetItemText(m_hWndList,iItem,1,sz);

		iItem++;

		if( dwFileSystemAttributes != 0 )
		{
			// Flags
			int i = 0;
			DWORD Flag = 0;
			DWORD dwMask = 0x1;
			while( GetVolumeAttributeFlag(i,&Flag) )
			{
				if( dwFileSystemAttributes & Flag )
				{
					StringCchPrintf(sz,MAX_PATH,L"0x%08X",Flag);
					lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM|LVIF_INDENT|LVIF_GROUPID;
					lvi.pszText = sz;
					lvi.lParam = 0;
					lvi.iImage = 0;
					lvi.iItem = iItem;
					lvi.iIndent = 2;
					lvi.iGroupId = ID_GROUP_FILESYSTEM_ATTRIBUTES;
					ListView_InsertItem(m_hWndList,&lvi);

					GetVolumeAttributeString(i,sz,MAX_PATH);
					ListView_SetItemText(m_hWndList,iItem,1,sz);

					iItem++;
				}
				else
				{
#if 0 // todo: optional
					lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM|LVIF_INDENT|LVIF_GROUPID;
					lvi.pszText = L"--";
					lvi.lParam = 0;
					lvi.iImage = 0;
					lvi.iItem = iItem;
					lvi.iIndent = 2;
					lvi.iGroupId = ID_GROUP_FILESYSTEM_ATTRIBUTES;
					ListView_InsertItem(m_hWndList,&lvi);

					ListView_SetItemText(m_hWndList,iItem,1,L"--");

					iItem++;
#endif
				}

				dwMask <<= 1;
				i++;
			}
		}
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
		ASSERT(m_pszNtDeviceName != NULL);
		if( m_pszNtDeviceName )
		{
			SELECT_ITEM sel = {0};
			sel.pszPath = m_pszNtDeviceName;
			sel.pszName = m_pszNtDeviceName;
			UpdateData(&sel);
		}
	}
};
