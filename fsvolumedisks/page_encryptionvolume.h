#pragma once
//***************************************************************************
//*                                                                         *
//*  page_encryptionvolume.h                                                *
//*                                                                         *
//*  Encryption Volume Information Page                                     *
//*                                                                         *
//*  Author:  YAMASHITA Katsuhiro                                           *
//*                                                                         *
//*  History: 2025-03-01 Created.                                           *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "dparray.h"
#include "common.h"
#include "column.h"
#include "findhandler.h"
#include "..\fsfilelib\fsfilelib.h"
#include <comdef.h>
#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

#define _STR_PROPNAME_SORTITEM L"SortItem"

#define MAX_VOLUME_BUFFER_LENGTH (50)

#define PM_INFORMATION_ACQUIRED  (WM_APP+1000)

enum {
	PS_PROTECTION_OFF=0,
	PS_PROTECTION_ON=1,
	PS_PROTECTION_UNKNOWN=2,
};

enum {
	CS_FULLY_DECRYPTED=0,
	CS_FULLY_ENCRYPTED=1,
	CS_ENCRYPTION_IN_PROGRESS=2,
	CS_DECRYPTION_IN_PROGRESS=3,
	CS_ENCRYPTION_PAUSED=4,
	CS_DECRYPTION_PAUSED=5,
};

enum {
	EM_NOT_ENCRYPTED=0,
	EM_AES_128_WITH_DIFFUSER=1,
	EM_AES_256_WITH_DIFFUSER=2,
	EM_AES_128=3,
	EM_AES_256=4,
	EM_HARDWARE_ENCRYPTION=5,
	EM_XTS_AES_128=6,
	EM_XTS_AES_256=7,
};

enum {
	VT_SYSTEM=0,
	VT_FIXED=1,
	VT_REMOVEABLE=2,
};

class EncryptionVolumeItem 
{
public:
	int ProtectionStatus;
	int EncryptionMethod;
	int ConversionStatus;
	int PropValue;

	WCHAR DriveLetter[3];
	WCHAR DeviceID[MAX_VOLUME_BUFFER_LENGTH];
	WCHAR PersistentVolumeID[MAX_VOLUME_BUFFER_LENGTH];
	WCHAR DeviceName[64];

public:
	EncryptionVolumeItem()
	{
		memset(this,0,sizeof(EncryptionVolumeItem));
	}

	~EncryptionVolumeItem()
	{
	}
};

class CEncryptionVolumePage :
	public CPageWndBase,
	public CFindHandler<CEncryptionVolumePage>
{
	HWND m_hWndList;

	COLUMN_HANDLER_DEF<CEncryptionVolumePage> *m_disp_proc;
	COMPARE_HANDLER_PROC_DEF_EX<CEncryptionVolumePage> *m_comp_proc;

	typedef struct _SORT_COLUMN_DIRECTION {
		int CurrentSubItem;
		int Direction;
	} SORT_COLUMN_DIRECTION;

	SORT_COLUMN_DIRECTION m_Sort;

	HFONT m_hFont;

	CColumnList m_columns;

	PWSTR m_pszErrorMessage;

	HIMAGELIST m_himl;

	HANDLE m_hThread;

	BOOL m_bFirstUpdate;

public:
	HWND GetListView() const { return m_hWndList; }

public:
	CEncryptionVolumePage()
	{
		m_hWndList = NULL;
		m_Sort.CurrentSubItem = -1;
		m_Sort.Direction = 1;
		m_disp_proc = NULL;
		m_comp_proc = NULL;
		m_pszErrorMessage = NULL;
		m_himl = NULL;
		m_hThread = NULL;
		m_bFirstUpdate = TRUE;
	}

	~CEncryptionVolumePage()
	{
		_SafeMemFree(m_pszErrorMessage);

		if( m_disp_proc )
			delete[] m_disp_proc;
		if( m_comp_proc )
			delete[] m_comp_proc;

		ASSERT( m_hThread == NULL );
	}

	virtual HRESULT OnInitPage(PVOID,DWORD,PVOID)
	{
		{ // Block: Create List
			HWND hwndList;

			hwndList = CreateWindowEx(0,WC_LISTVIEW, 
                              L"",
                              WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

			_EnableVisualThemeStyle(hwndList);

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

			HIMAGELIST himl = ImageList_Create(1,16,ILC_COLOR32,1,1);
			ListView_SetImageList(hwndList,himl,LVSIL_SMALL);

			if( !LoadColumns(hwndList) )
			{
				InitColumns(hwndList);
			}

			SetProp(hwndList,_STR_PROPNAME_SORTITEM,&m_Sort);

#if _ENABLE_DARK_MODE_TEST
			if( _IsDarkModeEnabled() )
				InitDarkModeListView(hwndList);
#endif
			m_himl = ImageList_Create(1,16,ILC_COLOR32,1,1);
			ListView_SetImageList(hwndList,himl,LVSIL_SMALL);

			// Save current list-view handle
			m_hWndList = hwndList;
		}

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

		return S_OK;
	}

	virtual HRESULT OnInitLayout(const RECT *prc)
	{
		return S_OK;
	}

	virtual HRESULT OnDestroyPage(PVOID)
	{
		return E_NOTIMPL;
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		m_hFont = GetGlobalFont(hWnd);
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( m_himl )
			ImageList_Destroy(m_himl);
		if( m_hFont )
			DeleteObject(m_hFont);

		RemoveProp(m_hWndList,_STR_PROPNAME_SORTITEM);

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
			case NM_CUSTOMDRAW:
				return OnCustomDraw(pnmhdr);
			case LVN_GETDISPINFO:
				return OnGetDispInfo(pnmhdr);
			case LVN_ITEMCHANGED:
				return OnItemChanged(pnmhdr);
			case LVN_DELETEITEM:
				return OnDeleteItem(pnmhdr);
			case LVN_ITEMACTIVATE:
				return OnItemActivate(pnmhdr);
			case LVN_COLUMNCLICK:
				return OnColumnClick(pnmhdr);
			case LVN_GETEMPTYMARKUP:
				return OnGetEmptyMarkup(pnmhdr);
			case LVN_KEYDOWN:
				return OnKeyDown(pnmhdr);
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;
		delete (EncryptionVolumeItem *)pnmlv->lParam;
		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;
		return 0;
	}

	LRESULT OnGetEmptyMarkup(NMHDR *pnmhdr)
	{
		NMLVEMPTYMARKUP *pnmlvem = (NMLVEMPTYMARKUP *)pnmhdr;

		if( m_pszErrorMessage )
		{
			pnmlvem->dwFlags = EMF_CENTERED;
			StringCchCopy(pnmlvem->szMarkup,ARRAYSIZE(pnmlvem->szMarkup),m_pszErrorMessage);
			return TRUE;
		}
		return FALSE;
	}

	LRESULT OnKeyDown(NMHDR *pnmhdr)
	{
		NMLVKEYDOWN *pnmkd = (NMLVKEYDOWN *)pnmhdr;

		if( pnmkd->wVKey == VK_SPACE || pnmkd->wVKey == VK_RETURN)
		{
			int iGroup = (int)ListView_GetFocusedGroup(pnmkd->hdr.hwndFrom);
			if( iGroup != -1 )
			{
				LVGROUP lvg = {0};
				lvg.cbSize = sizeof(lvg);
				lvg.mask = LVGF_GROUPID|LVGF_STATE;
				ListView_GetGroupInfoByIndex(pnmkd->hdr.hwndFrom,iGroup,&lvg);

				lvg.state = ListView_GetGroupState(pnmkd->hdr.hwndFrom,lvg.iGroupId,LVGS_COLLAPSED|LVGS_COLLAPSIBLE);

				lvg.mask = LVGF_STATE;
				lvg.state ^= LVGS_COLLAPSED;
				lvg.stateMask = LVGS_COLLAPSED;
				ListView_SetGroupInfo(pnmkd->hdr.hwndFrom,lvg.iGroupId,&lvg);
			}
		}
		
		return 0;
	}

	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)pnmlv->lParam;

		DoSort(pnmlv->hdr.hwndFrom,pnmlv->iSubItem);

		return 0;
	}

	void DoSort(HWND hwndLV,int iSubItem=-1,int iDirection=-1)
	{
		int id = (int)ListViewEx_GetHeaderItemData(hwndLV,iSubItem);

		SORT_COLUMN_DIRECTION& Sort = *(SORT_COLUMN_DIRECTION*)GetProp(hwndLV,_STR_PROPNAME_SORTITEM);

		if( Sort.CurrentSubItem != -1 )
			ListViewEx_SetHeaderArrow(hwndLV,Sort.CurrentSubItem,0);

		if( iDirection != 0 )
		{
			if( Sort.CurrentSubItem != iSubItem )
				Sort.Direction = 1;
			else
				Sort.Direction *= iDirection;
		}

		SortItemsEx(iSubItem,id,&Sort);

		ListViewEx_SetHeaderArrow(hwndLV,iSubItem,Sort.Direction);

		Sort.CurrentSubItem = iSubItem;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		pnmhdr->hwndFrom = m_hWnd;
		pnmhdr->idFrom = GetWindowLong(m_hWnd,GWL_ID);
		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)pnmhdr);
		return 0;
	}

	LRESULT OnCustomDraw(NMHDR *pnmhdr)
	{
		NMLVCUSTOMDRAW *pnmlvcd = (NMLVCUSTOMDRAW *)pnmhdr;

		if( IsXpThemeEnabled() )
		{
			int iGroup = (int)ListView_GetFocusedGroup(m_hWndList);
			if( iGroup == -1 )
				SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
			else
				SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_CLEAR,UISF_HIDEFOCUS),0);
		}

		if( pnmlvcd->nmcd.hdr.hwndFrom != m_hWndList )
			return CDRF_DODEFAULT;

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			SelectObject(pnmlvcd->nmcd.hdc,m_hFont);
			return CDRF_NEWFONT|CDRF_NOTIFYPOSTPAINT;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT )
		{
			if( IsXpThemeEnabled() )
			{
				UINT State = ListView_GetItemState(m_hWndList,(int)pnmlvcd->nmcd.dwItemSpec,LVIS_FOCUSED);
				if( State & LVIS_FOCUSED )
				{
					_DrawFocusFrame(m_hWndList,pnmlvcd->nmcd.hdc,&pnmlvcd->nmcd.rc);
				}
			}
		}

		return CDRF_DODEFAULT;
	}

	LRESULT OnDisp_Volume(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)pnmlvdi->item.lParam;
#if 0
		pnmlvdi->item.pszText = pItem->DeviceID;
#else
		StringCchCopy(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,&pItem->DeviceID[4]);
		RemoveBackslash(pnmlvdi->item.pszText);
#endif
		return 0;
	}

	LRESULT OnDisp_Device(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)pnmlvdi->item.lParam;
#if 0
		pnmlvdi->item.pszText = pItem->DeviceName;
#else
		if( HasPrefix(L"\\Device\\",pItem->DeviceName) )
		{
			StringCchCopy(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,&pItem->DeviceName[8]);
		}
		else
		{
			pnmlvdi->item.pszText = pItem->DeviceName;
		}
#endif
		return 0;
	}

	LRESULT OnDisp_Drive(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)pnmlvdi->item.lParam;
		if( *pItem->DriveLetter )
			pnmlvdi->item.pszText = pItem->DriveLetter;
		return 0;
	}

	LRESULT OnDisp_Status(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)pnmlvdi->item.lParam;
		static wchar_t *txt[] = {
			L"Enabled",
			L"Bitlocker On",
			L"Locked",
		};
		if( 0 <= pItem->ProtectionStatus && pItem->ProtectionStatus < _countof(txt) ) 
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s",txt[pItem->ProtectionStatus]);
		else
			pnmlvdi->item.pszText = L"-";
		return 0;
	}

	LRESULT OnDisp_EncryptionMethod(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)pnmlvdi->item.lParam;

		static wchar_t *txt[] = {
			L"NONE",                  // EM_NOT_ENCRYPTED
			L"AES 128 with Diffuser", // EM_AES_128_WITH_DIFFUSER
			L"AES 256 with Diffuser", // EM_AES_256_WITH_DIFFUSER
			L"AES 128",               // EM_AES_128
			L"AES 256",               // EM_AES_256
			L"Hardware Encryption",   // EM_HARDWARE_ENCRYPTION
			L"XTS-AES 128",           // EM_XTS_AES_128
			L"XTS-AES 256",           // EM_XTS_AES_256
		};

		if( 0 == pItem->EncryptionMethod )
		{
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s",
					(pItem->ProtectionStatus==PS_PROTECTION_ON) ? txt[pItem->EncryptionMethod] : L"-" );
		}
		else if( 0 < pItem->EncryptionMethod && pItem->EncryptionMethod < _countof(txt) ) 
		{
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s",txt[pItem->EncryptionMethod]);
		}
		else
		{
			if( pItem->EncryptionMethod != -1 )
				StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"Unknown (%d)",pItem->EncryptionMethod);
			else
				StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"Unknown");
		}
		return 0;
	}

	LRESULT OnDisp_ConversionStatus(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)pnmlvdi->item.lParam;

		static wchar_t *txt[] = {
			L"NONE",
			L"Fully Encrypted",        // CS_FULLY_ENCRYPTED
			L"Encryption in Progress", // CS_ENCRYPTION_IN_PROGRESS
			L"Decryption in Progress", // CS_DECRYPTION_IN_PROGRESS
			L"Encryption Paused",      // CS_ENCRYPTION_PAUSED
			L"Decryption Paused",      // CS_DECRYPTION_PAUSED
		};

		if( 0 == pItem->ConversionStatus )
		{
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s",
					(pItem->ProtectionStatus==PS_PROTECTION_ON) ? txt[pItem->ConversionStatus] : L"-" );
		}
		else if( 0 < pItem->ConversionStatus && pItem->ConversionStatus < _countof(txt) ) 
		{
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s",txt[pItem->ConversionStatus]);
		}
		else
		{
			StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"Unknown (%d)",pItem->ConversionStatus);
		}
		return 0;
	}

	LRESULT OnDisp_PersistentVolumeID(UINT id,NMLVDISPINFO *pnmlvdi)
	{
		EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)pnmlvdi->item.lParam;
		if( *pItem->PersistentVolumeID != 0 )
			pnmlvdi->item.pszText = pItem->PersistentVolumeID;
		else
			pnmlvdi->item.pszText = L"-";

		return 0;
	}

	void InitColumnTable()
	{
		static COLUMN_HANDLER_DEF<CEncryptionVolumePage> ch[] =
		{
			COL_HANDLER_MAP_DEF(COLUMN_Volume,             &CEncryptionVolumePage::OnDisp_Volume),
			COL_HANDLER_MAP_DEF(COLUMN_Device,             &CEncryptionVolumePage::OnDisp_Device),
			COL_HANDLER_MAP_DEF(COLUMN_Drive,              &CEncryptionVolumePage::OnDisp_Drive),
			COL_HANDLER_MAP_DEF(COLUMN_Status,             &CEncryptionVolumePage::OnDisp_Status),
			COL_HANDLER_MAP_DEF(COLUMN_EncryptionMethod,   &CEncryptionVolumePage::OnDisp_EncryptionMethod),
			COL_HANDLER_MAP_DEF(COLUMN_ConversionStatus,   &CEncryptionVolumePage::OnDisp_ConversionStatus),
			COL_HANDLER_MAP_DEF(COLUMN_PersistentVolumeID, &CEncryptionVolumePage::OnDisp_PersistentVolumeID),
		};

		m_disp_proc = new COLUMN_HANDLER_DEF<CEncryptionVolumePage>[COLUMN_MaxItem];

		ZeroMemory(m_disp_proc,sizeof(COLUMN_HANDLER_DEF<CEncryptionVolumePage>) * COLUMN_MaxItem);

		for(int i = 0; i < _countof(ch); i++)
		{
			m_disp_proc[ ch[i].colid ].colid = ch[i].colid;
			m_disp_proc[ ch[i].colid ].pfn   = ch[i].pfn;
		}
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)pdi->item.lParam;

		int id = (int)ListViewEx_GetHeaderItemData(pnmhdr->hwndFrom,pdi->item.iSubItem);

		if( m_disp_proc == NULL )
		{
			InitColumnTable();	
		}

		if( pdi->item.mask & LVIF_TEXT )
		{
			if( (id < COLUMN_MaxItem) && m_disp_proc[ id ].pfn )
			{
				pdi->item.mask |= LVIF_DI_SETITEM;
				return (this->*m_disp_proc[ id ].pfn)(id,pdi);
			}
		}
		return 0;	
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		if( ((pnmlv->uOldState == 0)|| ((pnmlv->uNewState & (LVIS_SELECTED)) == (LVIS_SELECTED))) 
			 && ((pnmlv->uNewState & (LVIS_SELECTED|LVIS_FOCUSED)) == (LVIS_SELECTED|LVIS_FOCUSED)) )
		{
			; // Reserved
		}

		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem);

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
				return 0;
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
			case PM_FINDITEM:
				return CFindHandler<CEncryptionVolumePage>::OnFindItem(hWnd,uMsg,wParam,lParam);
			case PM_INFORMATION_ACQUIRED:
				return OnInformationAcquired(hWnd,uMsg,wParam,lParam);
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd,&ps);

				RECT rc;
				GetClientRect(hWnd,&rc);

				HFONT hFont,hFontOld;
				hFont = GetIconFont();
				hFontOld = (HFONT)SelectObject(hdc,hFont);

				PWSTR psz = L"Reading...";
				DrawText(hdc,psz,-1,&rc,DT_CENTER|DT_VCENTER|DT_SINGLELINE);

				SelectObject(hdc,hFontOld);
				DeleteObject(hFont);

				EndPaint(hWnd,&ps);
				return 0;
			}
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	void UpdateLayout(int cx,int cy)
	{
		if( m_hWndList )
			SetWindowPos(m_hWndList,NULL,0,0,cx,cy,SWP_NOMOVE|SWP_NOZORDER);

		if( !IsWindowVisible(m_hWndList) )
		{
			RedrawWindow(m_hWnd,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_UPDATENOW|RDW_ERASENOW);
		}
	}

	void SetErrorState(ULONG Status)
	{
		PWSTR pMessage;
		if( WinGetErrorMessage(Status,&pMessage) > 0 )
		{
			SetEmptyMessage(pMessage);
			WinFreeErrorMessage(pMessage);
		}
	}

	void SetEmptyMessage(PCWSTR psz)
	{
		_SafeMemFree(m_pszErrorMessage);
		m_pszErrorMessage = _MemAllocString(psz);
	}

	void InitColumns(HWND hWndList)
	{
		LVCOLUMN lvc = {0};

		static COLUMN columns_filelist[] = {
			{ COLUMN_Device,             L"Device",              0, 100, LVCFMT_LEFT },
			{ COLUMN_Drive,              L"Drive",               1, 100, LVCFMT_LEFT },
			{ COLUMN_Status,             L"Status",              2, 100, LVCFMT_LEFT },
			{ COLUMN_EncryptionMethod,   L"Encryption Method",   3, 100, LVCFMT_LEFT },
			{ COLUMN_ConversionStatus,   L"Conversion Status",   4, 100, LVCFMT_LEFT },
			{ COLUMN_Volume,             L"Volume ID",           5, 100, LVCFMT_LEFT },
			{ COLUMN_PersistentVolumeID, L"Persistent Volume ID",6, 100, LVCFMT_LEFT },
		};

		m_columns.SetDefaultColumns(columns_filelist,ARRAYSIZE(columns_filelist));

		int i,c;
		c = m_columns.GetDefaultColumnCount();
		for(i = 0; i < c; i++)
		{
			const COLUMN *pcol = m_columns.GetDefaultColumnItem(i);
			lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
			lvc.fmt     = pcol->fmt;
			lvc.cx      = pcol->cx;
			lvc.pszText = pcol->Name;
			lvc.iOrder  = pcol->iOrder;

			int index = ListView_InsertColumn(hWndList,lvc.iOrder,&lvc);

			ListViewEx_SetHeaderItemData( hWndList, index, pcol->id );
		}
	}

	BOOL LoadColumns(HWND hWndList)
	{
		COLUMN_TABLE *pcoltbl;
		if( m_columns.LoadUserDefinitionColumnTable(&pcoltbl,L"ColumnLayout") == 0)
			return FALSE;

		LVCOLUMN lvc = {0};

		ULONG i;
		for(i = 0; i < pcoltbl->cItems; i++)
		{
			const COLUMN *pcol = &pcoltbl->column[i];
			lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
			lvc.fmt     = pcol->fmt;
			lvc.cx      = pcol->cx;
			lvc.pszText = pcol->Name;
			lvc.iOrder  = pcol->iOrder;
			int index = ListView_InsertColumn(hWndList,lvc.iOrder,&lvc);

			ListViewEx_SetHeaderItemData( hWndList, index, pcol->id );
		}

		return TRUE;
	}

	static HRESULT EnumEncryptionVolumeItemByWMI(HDPA *phdpa)
	{
		HRESULT hr = S_OK;

		IWbemLocator* pLoc = NULL;
		hr = CoCreateInstance(
				CLSID_WbemLocator, 
				0, 
				CLSCTX_INPROC_SERVER, 
				IID_IWbemLocator, 
				(LPVOID*)&pLoc
				);

		if( FAILED(hr) )
		{
			// Failed to create IWbemLocator object. 
			return hr;
		}

		BSTR bs = SysAllocString(L"ROOT\\CIMV2\\Security\\MicrosoftVolumeEncryption");
		IWbemServices* pSvc = NULL;
		hr = pLoc->ConnectServer(
					bs, 
					NULL, NULL, NULL, 
					0, NULL, NULL, &pSvc
					);
		SysFreeString(bs);

		if( FAILED(hr) )
		{
			// Could not connect to WMI namespace.
			pLoc->Release();
			return hr;
		}

		hr = CoSetProxyBlanket(
					pSvc, 
					RPC_C_AUTHN_WINNT, 
					RPC_C_AUTHZ_NONE, 
					NULL, 
					RPC_C_AUTHN_LEVEL_CALL, 
					RPC_C_IMP_LEVEL_IMPERSONATE, 
					NULL, 
					EOAC_NONE
					);

		if( FAILED(hr) )
		{
			// Could not set proxy blanket.
			pSvc->Release();
			pLoc->Release();
			return hr;
		}

		IEnumWbemClassObject* pEnumerator = NULL;
		BSTR bsWQL   = SysAllocString(L"WQL");
		BSTR bsQuery = SysAllocString(L"SELECT * FROM Win32_EncryptableVolume");
		hr = pSvc->ExecQuery(
					bsWQL,
					bsQuery,
					WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
					NULL, 
					&pEnumerator
					);
		SysFreeString(bsQuery);
		SysFreeString(bsWQL);

		if( FAILED(hr) )
		{
			// Query for encryptable volumes failed.
			pSvc->Release();
			pLoc->Release();
			return hr;
		}

		ULONG c_hdpa = 0;
		HDPA hdpa = DPA_Create(24);
		if( hdpa == NULL )
		{
			pEnumerator->Release();
			pSvc->Release();
			pLoc->Release();
			return E_OUTOFMEMORY;
		}

		IWbemClassObject* pclsObj = NULL;
		ULONG uReturn = 0;
		EncryptionVolumeItem *pItem;

		while( pEnumerator )
		{
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
			if( 0 == uReturn )
				break;

			pItem = new EncryptionVolumeItem;	
			if( pItem == NULL )
				break;

			VARIANT vtProp;

			hr = pclsObj->Get(L"ProtectionStatus", 0, &vtProp, 0, 0);
			if (SUCCEEDED(hr) && vtProp.vt == VT_I4)
			{
				pItem->ProtectionStatus = vtProp.lVal;
				VariantClear(&vtProp);
			}

			hr = pclsObj->Get(L"DriveLetter", 0, &vtProp, 0, 0);
			if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
			{
				pItem->DriveLetter[0] = vtProp.bstrVal[0];
				pItem->DriveLetter[1] = vtProp.bstrVal[1];
				pItem->DriveLetter[2] = L'\0';
				VariantClear(&vtProp);
			}

			hr = pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0);
			if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
			{
				StringCchCopy(pItem->DeviceID,_countof(pItem->DeviceID),vtProp.bstrVal);
				VariantClear(&vtProp);
			}

			hr = pclsObj->Get(L"PersistentVolumeID", 0, &vtProp, 0, 0);
			if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
			{
				StringCchCopy(pItem->PersistentVolumeID,_countof(pItem->PersistentVolumeID),vtProp.bstrVal);
				VariantClear(&vtProp);
			}

			hr = pclsObj->Get(L"ConversionStatus", 0, &vtProp, 0, 0);
			if (SUCCEEDED(hr) && vtProp.vt == VT_I4)
			{
				pItem->ConversionStatus = vtProp.lVal;
				VariantClear(&vtProp);
			}

			hr = pclsObj->Get(L"EncryptionMethod",0,&vtProp,0,0);
			if (SUCCEEDED(hr) && vtProp.vt == VT_I4)
			{
				pItem->EncryptionMethod = vtProp.lVal;
				VariantClear(&vtProp);
			}

			c_hdpa = DPA_AppendPtr(hdpa,pItem);

			pclsObj->Release();
		}

		pEnumerator->Release();

		pSvc->Release();
		pLoc->Release();

		*phdpa = hdpa;

		return hr;
	}

	static DWORD WINAPI ThreadProc( __in  LPVOID lpParameter)
	{
		HRESULT hr;

		CoInitializeEx(0, COINIT_MULTITHREADED);

		HDPA hdpa = NULL;
		hr = EnumEncryptionVolumeItemByWMI(&hdpa);

		CoUninitialize();

		PostMessage( (HWND)lpParameter, PM_INFORMATION_ACQUIRED, (WPARAM)hdpa, (LPARAM)hr );

		return 0;
	}

	LRESULT OnInformationAcquired(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		if( m_hThread )
		{
			WaitForSingleObject(m_hThread,INFINITE);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}

		HDPA hdpa = (HDPA)wParam;

		if( hdpa != NULL )
		{
			int cItems = DPA_GetPtrCount(hdpa);

			for(int i = 0; i < cItems; i++)
			{
				EncryptionVolumeItem *pItem = (EncryptionVolumeItem *)DPA_GetPtr(hdpa,i);

				WCHAR sz[MAX_PATH];
				StringCchCopy(sz,MAX_PATH,pItem->DeviceID);
				RemoveBackslash(sz);

				QueryDosDevice(&sz[4],pItem->DeviceName,ARRAYSIZE(pItem->DeviceName));

				InsertItem(m_hWndList,-1,-1,I_IMAGENONE,pItem);
			}

			SORT_COLUMN_DIRECTION *Sort = (SORT_COLUMN_DIRECTION*)GetProp(m_hWndList,_STR_PROPNAME_SORTITEM);
			DoSort(m_hWndList,Sort->CurrentSubItem,0);

			DPA_Destroy(hdpa);
		}
		else
		{
			HRESULT hr = (HRESULT)lParam;
			if( WBEM_E_ACCESS_DENIED == hr )
				SetEmptyMessage(L"Access Denied.\nFailed to read by WMI.");
			else
				SetErrorState( (ULONG)hr );
		}

		if( m_bFirstUpdate )
		{
			UpdateColumnWidth();
			ListViewEx_SetLastColumnWidth(m_hWndList,LVEXCHTF_ADJUST_WIDTH_BY_COLUMN_ITEM_TEXT);
			m_bFirstUpdate = FALSE;
		}

		ShowWindow(m_hWndList,SW_SHOW);

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

		return 0;
	}

	int UpdateColumnWidth()
	{
		HWND hwndLV = m_hWndList;
		HWND hwndHD = ListView_GetHeader(hwndLV);
		int i,cx = 0;	
		int cHeaders = Header_GetItemCount(hwndHD);
		for(i = 0; i < (cHeaders-1); i++)
		{
			ListView_SetColumnWidth(hwndLV,i,LVSCW_AUTOSIZE_USEHEADER);
			cx += ListView_GetColumnWidth(hwndLV,i);
		}
		return cx;
	}

	int InsertItem(HWND hWndList,int iGroupId,int iItem,int iImage, EncryptionVolumeItem *pItem)
	{
		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = iImage;
		lvi.lParam  = (LPARAM)pItem;
		lvi.pszText = (PWSTR)LPSTR_TEXTCALLBACK;

		iItem = ListView_InsertItem(hWndList,&lvi);

		ListView_SetItemText(hWndList,iItem,1,LPSTR_TEXTCALLBACK);
		ListView_SetItemText(hWndList,iItem,2,LPSTR_TEXTCALLBACK);
		ListView_SetItemText(hWndList,iItem,3,LPSTR_TEXTCALLBACK);
		ListView_SetItemText(hWndList,iItem,4,LPSTR_TEXTCALLBACK);

		return iItem;
	}

	HRESULT FillItems(SELECT_ITEM *pSel)
	{
		CWaitCursor wait;

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);
		ListView_RemoveAllGroups(m_hWndList);

		if( IsUserAnAdmin() )
		{
			m_hThread = CreateThread(NULL,0,&ThreadProc,(LPVOID)m_hWnd,0,0);
		}
		else
		{
			m_hThread = NULL;
			UpdateColumnWidth();
			ListViewEx_SetLastColumnWidth(m_hWndList,LVEXCHTF_ADJUST_WIDTH_BY_COLUMN_ITEM_TEXT);
			RECT rc;
			GetClientRect(m_hWnd,&rc);
			UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));
			ShowWindow(m_hWndList,SW_SHOW);
			SetEmptyMessage( L"Access denied.\nTo get fully-information, run under administrator mode.");
		}

		SetRedraw(m_hWndList,TRUE);

		return S_OK;
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		return FillItems((SELECT_ITEM*)pFile);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	//  Sort
	//
	int comp_volume_ex(int iItem1,int iItem2, const void *p)
	{
		EncryptionVolumeItem *pItem1 = (EncryptionVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem1);
		EncryptionVolumeItem *pItem2 = (EncryptionVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem2);
		return StrCmp(pItem1->DeviceID,pItem2->DeviceID);
	}

	int comp_device_ex(int iItem1,int iItem2, const void *p)
	{
		EncryptionVolumeItem *pItem1 = (EncryptionVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem1);
		EncryptionVolumeItem *pItem2 = (EncryptionVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem2);
		return StrCmpLogicalW(pItem1->DeviceName,pItem2->DeviceName);
	}

	int comp_drive_ex(int iItem1,int iItem2, const void *p)
	{
		EncryptionVolumeItem *pItem1 = (EncryptionVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem1);
		EncryptionVolumeItem *pItem2 = (EncryptionVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem2);
		if( *pItem1->DriveLetter != NULL && *pItem2->DriveLetter == NULL )
			return -1;
		else if( *pItem1->DriveLetter == NULL && *pItem2->DriveLetter != NULL )
			return 1;
		else if( *pItem1->DriveLetter != NULL && *pItem2->DriveLetter != NULL )
			return StrCmpI(pItem1->DriveLetter,pItem2->DriveLetter);
		return 0;
	}

	int comp_encryption_method_ex(int iItem1,int iItem2, const void *p)
	{
		SORT_PARAM_EX<CEncryptionVolumePage> *opex = (SORT_PARAM_EX<CEncryptionVolumePage> *)p;

		WCHAR sz1[MAX_PATH];
		WCHAR sz2[MAX_PATH];

		ListView_GetItemText(m_hWndList,iItem1,opex->iSubItem,sz1,MAX_PATH);
		ListView_GetItemText(m_hWndList,iItem2,opex->iSubItem,sz2,MAX_PATH);

		if( (*sz1 == L'-' && *sz2 != L'-' ) || (*sz1 == L'\0' && *sz2 != L'\0' ) )
			return 1;
		else if( (*sz1 != L'-' && *sz2 == L'-') || (*sz1 != L'\0' && *sz2 == L'\0') )
			return -1;

		return StrCmp(sz1,sz2);
	}

	int comp_conversion_status_ex(int iItem1,int iItem2, const void *p)
	{
		SORT_PARAM_EX<CEncryptionVolumePage> *opex = (SORT_PARAM_EX<CEncryptionVolumePage> *)p;

		WCHAR sz1[MAX_PATH];
		WCHAR sz2[MAX_PATH];

		ListView_GetItemText(m_hWndList,iItem1,opex->iSubItem,sz1,MAX_PATH);
		ListView_GetItemText(m_hWndList,iItem2,opex->iSubItem,sz2,MAX_PATH);

		if( (*sz1 == L'-' && *sz2 != L'-' ) || (*sz1 == L'\0' && *sz2 != L'\0' ) )
			return 1;
		else if( (*sz1 != L'-' && *sz2 == L'-') || (*sz1 != L'\0' && *sz2 == L'\0') )
			return -1;

		return StrCmp(sz1,sz2);
	}

	int comp_persistent_volumeid_ex(int iItem1,int iItem2, const void *p)
	{
		SORT_PARAM_EX<CEncryptionVolumePage> *opex = (SORT_PARAM_EX<CEncryptionVolumePage> *)p;

		EncryptionVolumeItem *pItem1 = (EncryptionVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem1);
		EncryptionVolumeItem *pItem2 = (EncryptionVolumeItem *)ListViewEx_GetItemData(m_hWndList,iItem2);
#if 1
		if( pItem1->ProtectionStatus == 0 && pItem2->ProtectionStatus != 0 )
			return 1;
		else if( pItem1->ProtectionStatus != 0 && pItem2->ProtectionStatus == 0 )
			return -1;
		else
		{
			if( pItem1->ProtectionStatus == 1 && pItem2->ProtectionStatus == 2 )
				return -1;
			else if( pItem1->ProtectionStatus == 2 && pItem2->ProtectionStatus == 1 )
				return 1;
			else
			{
				GUID Guid1;
				GUID Guid2;
				GUIDFromString(pItem1->PersistentVolumeID,&Guid1);
				GUIDFromString(pItem2->PersistentVolumeID,&Guid2);
				return memcmp(&Guid1,&Guid2,sizeof(GUID));
			}
		}

		return 0;
#else
		return StrCmp(pItem1->PersistentVolumeID,pItem2->PersistentVolumeID);
#endif
	}

	void init_compare_proc_def_table()
	{
		static COMPARE_HANDLER_PROC_DEF_EX<CEncryptionVolumePage> comp_proc[] = 
		{
			{0,NULL},
			{COLUMN_Volume,             &CEncryptionVolumePage::comp_volume_ex},
			{COLUMN_Device,             &CEncryptionVolumePage::comp_device_ex},
			{COLUMN_Drive,              &CEncryptionVolumePage::comp_drive_ex},
			{COLUMN_EncryptionMethod,   &CEncryptionVolumePage::comp_encryption_method_ex},
			{COLUMN_ConversionStatus,   &CEncryptionVolumePage::comp_conversion_status_ex},
			{COLUMN_PersistentVolumeID, &CEncryptionVolumePage::comp_persistent_volumeid_ex},
		};

		m_comp_proc = new COMPARE_HANDLER_PROC_DEF_EX<CEncryptionVolumePage>[COLUMN_MaxItem];

		ZeroMemory(m_comp_proc,sizeof(COMPARE_HANDLER_PROC_DEF_EX<CEncryptionVolumePage>)*COLUMN_MaxItem);

		int i;
		for(i = 0; i < _countof(comp_proc); i++)
		{
			m_comp_proc[ comp_proc[i].colid ].colid = comp_proc[i].colid;
			m_comp_proc[ comp_proc[i].colid ].proc  = comp_proc[i].proc;
		}
	}

	int CompareItemEx(int iItem1,int iItem2,SORT_PARAM_EX<CEncryptionVolumePage> *op)
	{
		if( m_comp_proc == NULL )
		{
			init_compare_proc_def_table();
		}

		int iResult = 0;

		if( iResult == 0 && m_comp_proc[op->id].proc != NULL )
		{
			iResult = (this->*m_comp_proc[op->id].proc)(iItem1,iItem2,op);
		}

		iResult *= op->direction;

		return iResult;
	}

	static int CALLBACK CompareProcEx(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		SORT_PARAM_EX<CEncryptionVolumePage> *op = (SORT_PARAM_EX<CEncryptionVolumePage> *)lParamSort;
		return op->pThis->CompareItemEx((int)lParam1,(int)lParam2,op);
	}

	void SortItemsEx(int iSubItem,UINT id,SORT_COLUMN_DIRECTION *pSort)
	{
		SORT_PARAM_EX<CEncryptionVolumePage> op = {0};
		op.pThis = this;
		op.id = id;
		op.iSubItem = iSubItem;
		op.direction = pSort->Direction; // 1 or -1 do not use 0
		ListView_SortItemsEx(m_hWndList,CompareProcEx,&op);
	}

	//////////////////////////////////////////////////////////////////////////
	//
	//  Command Handling
	//
	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				*State = ListView_GetSelectedCount(m_hWndList) ? UPDUI_ENABLED : UPDUI_DISABLED;
				return S_OK;
			case ID_VIEW_REFRESH:
			case ID_EDIT_FIND:
			case ID_EDIT_FIND_NEXT:
			case ID_EDIT_FIND_PREVIOUS:
				*State = UPDUI_ENABLED;
				return S_OK;
		}
		return S_FALSE;
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
		SELECT_ITEM sel = {0};
		FillItems(&sel);
	}
};
