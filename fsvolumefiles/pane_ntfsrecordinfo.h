#pragma once
//*****************************************************************************
//
//  pane_ntfsrecordinfo.h
//
//  PURPOSE: NTFS File Record Information Pane.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-07-11 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "dialog_debug_dump.h"
#include <wincrypt.h>

class CDebugText : public IDebugText
{
public:
	CDebugText() {}
	~CDebugText() {}

	void appendDec(ULONGLONG n)
	{
		WCHAR szBuffer[64];
		WCHAR szNumber[64];
		StringCchPrintf(szBuffer,ARRAYSIZE(szBuffer),L" (%s)\r\n",_CommaFormatString(n,szNumber));
		this->AddText(szBuffer);
	}

	void dumpBYTE(int cWidth,PWSTR pszLabel,BYTE b,BOOL bDec=FALSE)
	{
		WCHAR szBuf[MAX_PATH];
		StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"%*s : 0x%02X",cWidth,pszLabel,b);
		this->AddText(szBuf);
		bDec ? appendDec(b) : this->AddText(L"\r\n");
	}

	void dumpWORD(int cWidth,PWSTR pszLabel,WORD w,BOOL bDec=FALSE)
	{
		WCHAR szBuf[MAX_PATH];
		StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"%*s : 0x%04X",cWidth,pszLabel,w);
		this->AddText(szBuf);
		bDec ? appendDec(w) : this->AddText(L"\r\n");
	}

	void dumpDWORD(int cWidth,PWSTR pszLabel,DWORD dw,BOOL bDec=FALSE)
	{
		WCHAR szBuf[MAX_PATH];
		StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"%*s : 0x%08X",cWidth,pszLabel,dw);
		this->AddText(szBuf);
		bDec ? appendDec(dw) : this->AddText(L"\r\n");
	}

	void dumpQWORD(int cWidth,PWSTR pszLabel,DWORD64 dw64,BOOL bDec=FALSE)
	{
		WCHAR szBuf[MAX_PATH];
		StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"%*s : 0x%016I64X",cWidth,pszLabel,dw64);
		this->AddText(szBuf);
		bDec ? appendDec(dw64) : this->AddText(L"\r\n");
	}

	void dumpUCHAR(int cWidth,PWSTR pszLabel,BYTE b,BOOL bDec=FALSE) { dumpBYTE(cWidth,pszLabel,b,bDec); };
	void dumpUSHORT(int cWidth,PWSTR pszLabel,USHORT us,BOOL bDec=FALSE) { dumpWORD(cWidth,pszLabel,us,bDec); };
	void dumpULONG(int cWidth,PWSTR pszLabel,ULONG l,BOOL bDec=FALSE) { dumpDWORD(cWidth,pszLabel,l,bDec); };
	void dumpULONGLONG(int cWidth,PWSTR pszLabel,ULONGLONG ll,BOOL bDec=FALSE) { dumpQWORD(cWidth,pszLabel,ll,bDec); };

	void dumpName(int cWidth,PWSTR pszLabel,ULONG FileNameLength,PWSTR FileName)
	{
		WCHAR szBuf[MAX_PATH * 2];
#if 0
		StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"%*s : %*s\r\n",cWidth,pszLabel,FileNameLength,FileName);
#else
		UNICODE_STRING usFileName;
		usFileName.Length = (USHORT)(FileNameLength * sizeof(WCHAR));
		usFileName.MaximumLength = usFileName.Length;
		usFileName.Buffer = FileName;
		StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"%*s : %wZ\r\n",cWidth,pszLabel,&usFileName);
#endif
		this->AddText(szBuf);
	}

	void dumpDateTime(int cWidth,PWSTR pszLabel,LONGLONG dt)
	{
		WCHAR sz[MAX_PATH];
		PWSTR pszDateFormat = NULL;
		PWSTR pszTimeFormat = NULL;
		_GetDateTimeStringEx2(dt,sz,ARRAYSIZE(sz),pszDateFormat,NULL,FALSE,TRUE);
	
		WCHAR szBuf[MAX_PATH];
		StringCchPrintf(szBuf,ARRAYSIZE(szBuf),L"%*s : %s (0x%016I64X)\r\n",cWidth,pszLabel,sz,dt);
		this->AddText(szBuf);
	}
};

enum {
	ITEM_RECORD_ATTRIBUTE=0,
	ITEM_RECORD_ATTRIBUTE_ENTRY=1,
};

typedef struct _NTFSRECORD_LIST_ITEM
{
	USHORT itemType;
	LONGLONG FileId;
	PWSTR pszDisplayName;
	int RecordIndex;
	ATTRIBUTE_RECORD_HEADER *Attribute;
	int SegmentIndex;
	ATTRIBUTE_LIST_ENTRY *AttrListEntry;
	ATTRIBUTE_RECORD_HEADER *AttributeList;
	LONGLONG VcnCount;
} NTFSRECORD_LIST_ITEM;

class CNTFSRecordInformationWindow : public CBaseWindow
{
	typedef struct {
		NTFSRECORD_LIST_ITEM *pItem;
		CNTFSRecordInformationWindow *pThis;
	} CALLBACKPARAM;

	HWND m_hWndList;
	HWND m_hWndCtrlFocus;

	HFONT m_hFont;
	HFONT m_hFontHeader;

	PWSTR m_pszFilePath;
	PWSTR m_pszFileName;
	DWORD m_dwFlags;

	struct {
		int CurrentSubItem;
		int Direction;
	} m_Sort;

	typedef struct _SORT_PARAM {
		UINT id;
		INT direction; // must 1 or -1, do not use 0
	} SORT_PARAM;

	PWSTR m_pszErrorMessage;

	HDPA m_hdpa;

public:
	CNTFSRecordInformationWindow()
	{
		m_hWndList = NULL;
		m_hWndCtrlFocus = NULL;
		m_hFont = NULL;
		m_hFontHeader = NULL;
		m_pszErrorMessage = NULL;
		m_pszFilePath = NULL;
		m_pszFileName = NULL;
		m_dwFlags = 0;
		m_hdpa = NULL;
	}

	~CNTFSRecordInformationWindow()
	{
	}

	HRESULT SetFile(PCWSTR pszFilePath,DWORD dwFileAttributes)
	{
		FILEITEMEX fiex = {};
		fiex.FileAttributes = dwFileAttributes;
		return (HRESULT)OnSetPath(pszFilePath,&fiex);
	}

protected:
	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		m_hdpa = DPA_Create( 16 );
		InitializeListView();
		return 0;
	}

	LRESULT OnDestroy(HWND,UINT,WPARAM,LPARAM)
	{
		if( m_hdpa )
		{
			FreeRecordData();
			DPA_Destroy(m_hdpa);
			m_hdpa = NULL;
		}

		if( m_hFont )
			DeleteObject(m_hFont);
		if( m_hFontHeader )
			DeleteObject(m_hFont);
		_SafeMemFree(m_pszErrorMessage);
		_SafeMemFree(m_pszFilePath);
		_SafeMemFree(m_pszFileName);
		return 0;
	}

	LRESULT OnSize(HWND,UINT,WPARAM,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		if( m_hWndCtrlFocus )
			SetFocus(m_hWndCtrlFocus);
		return 0;
	}

	LRESULT OnEraseBkGnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HDC hdc = (HDC)wParam;
		RECT rc;
		GetClientRect(hWnd,&rc);
		FillRect(hdc,&rc,GetSysColorBrush(COLOR_3DFACE));
		return (LRESULT)1;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		NTFSRECORD_LIST_ITEM *pItem = (NTFSRECORD_LIST_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

		if( pItem )
		{
			HMENU hMenu = CreatePopupMenu();
			AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");

			HMENU hSubMenu = CreatePopupMenu();
			{
				PWSTR *pColumns;
				int cColumns;
				ListViewEx_GetTextColumns(m_hWndList,&pColumns,&cColumns,NULL);

				for(int i = 0; i < cColumns; i++)
				{
					AppendMenu(hSubMenu,MF_STRING,ID_EDIT_COPY_COUMN_FIRST+i,pColumns[i]);
				}
				CoTaskMemFree(pColumns);
			}
			AppendMenu(hMenu,MF_POPUP,(UINT_PTR)hSubMenu,L"Copy Co&lumn Text");

			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ListViewEx_SimpleContextMenuHandler(hWnd,m_hWndList,(HWND)wParam,hMenu,pt,0);

			DestroyMenu(hMenu);
		}

		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;
		switch( pnmhdr->code )
		{
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
			case LVN_GETDISPINFO:
				return OnGetDispInfo(pnmhdr);
			case LVN_DELETEITEM:
				return OnDeleteItem(pnmhdr);
			case LVN_ITEMCHANGED:
				return OnItemChanged(pnmhdr);
#if 0
			case LVN_COLUMNCLICK:
				return OnColumnClick(pnmhdr);
#endif
			case LVN_GETEMPTYMARKUP:
				return OnGetEmptyMarkup(pnmhdr);
			case LVN_ITEMACTIVATE:
				return OnItemActivate(pnmhdr);
		}
		return 0;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		m_hWndCtrlFocus = pnmhdr->hwndFrom;

		// Message forward to parent window
		return ForwardNotificationToParent(m_hWnd,pnmhdr);
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

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;
		NTFSRECORD_LIST_ITEM *pItem = (NTFSRECORD_LIST_ITEM *)ListViewEx_GetItemData(m_hWndList,pnmia->iItem);

		WCHAR szTitle[64];
		WCHAR szRecordName[64];
		if( GetNtfsRecordNameString( pItem->Attribute->TypeCode, szRecordName, ARRAYSIZE(szRecordName), 0) == 0 )
			StringCchCopy(szRecordName,ARRAYSIZE(szRecordName),L"(Unknown)");

		StringCchPrintf(szTitle,ARRAYSIZE(szTitle),L"%s-%s",m_pszFileName,szRecordName);

		CALLBACKPARAM cp;
		cp.pItem = pItem;
		cp.pThis = this;
		DebugDumpDialog(GetParent(m_hWnd),szTitle,L"",
				DDDF_USETEXTINTERFACE|DDDF_SETDIALOGSIZE,
				&CallbackProc,
				(ULONG_PTR)&cp,NULL);

		return 0;
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		NMLISTVIEW nmlv = *(NMLISTVIEW *)pnmhdr;
		nmlv.hdr.hwndFrom = m_hWnd;
		nmlv.hdr.idFrom = GetWindowLong(m_hWnd,GWL_ID);
		return SendMessage(GetParent(m_hWnd),WM_NOTIFY,(WPARAM)nmlv.hdr.idFrom,(LPARAM)&nmlv);
	}

#if 0
	LRESULT OnColumnClick(NMHDR *pnmhdr)
	{
		NMLISTVIEW* pnmlv = (NMLISTVIEW*)pnmhdr;

		DoSort(pnmlv->iSubItem,TRUE);

		return 0;
	}
#endif

	LRESULT OnGetDispInfo(NMHDR* pnmhdr)
	{
		NMLVDISPINFO* pnmlvdi = (NMLVDISPINFO*)pnmhdr;

		if( pnmlvdi->item.mask & LVIF_IMAGE )
		{
			; // reserved
		}
		if( pnmlvdi->item.mask & LVIF_TEXT )
		{
			NTFSRECORD_LIST_ITEM *pItem = (NTFSRECORD_LIST_ITEM *)pnmlvdi->item.lParam;
	
			*pnmlvdi->item.pszText = L'\0';

			pnmlvdi->item.mask |= LVIF_DI_SETITEM;

			if( pItem->itemType == ITEM_RECORD_ATTRIBUTE )
			{
				switch( pnmlvdi->item.iSubItem )
				{
					case 0:
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s",pItem->pszDisplayName);
						break;
					case 1:
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%02X",pItem->Attribute->TypeCode);
						break;
					case 2:
						_CommaFormatString(pItem->Attribute->RecordLength,pnmlvdi->item.pszText);
						break;
					case 3:
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%04X",pItem->Attribute->Instance);
						break;
					case 4:
					{
						if( pItem->Attribute->FormCode == NONRESIDENT_FORM )
						{
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%016I64X",pItem->FileId);
						}
						break;
					}
					case 5:
					{
						if( pItem->Attribute->FormCode == NONRESIDENT_FORM )
						{
							LONGLONG n;
							GetNtfsClusterMappingPair(pItem->Attribute,NULL,&n);
							_CommaFormatString(n,pnmlvdi->item.pszText);
						}
						break;
					}
				}
			}
			else
			{
				switch( pnmlvdi->item.iSubItem )
				{
					case 0:
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"%s",pItem->pszDisplayName);
						break;
					case 1:
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%02X",pItem->AttrListEntry->AttributeTypeCode);
						break;
					case 2:
						_CommaFormatString(pItem->Attribute->RecordLength,pnmlvdi->item.pszText);
						break;
					case 3:
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%04X",pItem->AttrListEntry->Reserved);
						break;
					case 4:
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%016I64X",pItem->AttrListEntry->SegmentReference.QuadPart);
						break;
					case 5:
						if( pItem->VcnCount != 0 )
							_CommaFormatString(pItem->VcnCount,pnmlvdi->item.pszText);
						else
							pnmlvdi->item.pszText = L"-";
						break;
				}
			}
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW* pnmlv = (NMLISTVIEW*)pnmhdr;

		NTFSRECORD_LIST_ITEM *pItem = (NTFSRECORD_LIST_ITEM*)pnmlv->lParam;

		_SafeMemFree(pItem->pszDisplayName);

		delete pItem;

		return 0;
	}

	LRESULT OnCommand(HWND,UINT,WPARAM wParam,LPARAM)
	{
		UINT CmdId = LOWORD(wParam);

		switch( CmdId )
		{
			case ID_EDIT_COPY:
				if( GetKeyState(VK_SHIFT) < 0 )
				{
					SetClipboardTextFromListViewColumn(m_hWndList,SCTEXT_FORMAT_SELECTONLY,1);
				}
				else
				{
					SetClipboardTextFromListView(m_hWndList,SCTEXT_UNICODE);
				}
				break;
			default:
				if( ID_EDIT_COPY_COUMN_FIRST <= CmdId && CmdId <= ID_EDIT_COPY_COUMN_LAST )
					SetClipboardTextFromListViewColumn(m_hWndList,SCTEXT_FORMAT_SELECTONLY,(CmdId-ID_EDIT_COPY_COUMN_FIRST));
				break;
		}

		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_ERASEBKGND:
				return OnEraseBkGnd(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				return OnSetFocus(hWnd,uMsg,wParam,lParam);
		    case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
		    case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case PPM_SETPATH:
				return OnSetPath((PCWSTR)lParam,(FILEITEMEX *)wParam);
			case PPM_CHDIR:
				return OnChDir((PCWSTR)lParam,(FILEITEMEX *)wParam);
			case PPM_SELECTSTREAM:
				return OnSelItem(wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

#if 0 // todo: reserved
	static int CALLBACK CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		NTFSRECORD_LIST_ITEM *pItem1 = (NTFSRECORD_LIST_ITEM *)lParam1;
		NTFSRECORD_LIST_ITEM *pItem2 = (NTFSRECORD_LIST_ITEM *)lParam2;

		SORT_PARAM *psp = (SORT_PARAM *)lParamSort;

		int iRet = 0;

		switch( psp->id )
		{
			case 0:
				iRet = wcscmp(pItem1->pszDisplayName,pItem2->pszDisplayName);
				break;
			case 2:
			{
				LONGLONG s1,s2;
				s1 = ( pItem1->itemType == 0 ) ? pItem1->Attribute->RecordLength : pItem1->AttrListEntry->RecordLength;
				s2 = ( pItem2->itemType == 0 ) ? pItem2->Attribute->RecordLength : pItem2->AttrListEntry->RecordLength;
				iRet = _COMP(s1,s2);
				break;
			}
		}

		iRet *= psp->direction; 

		return iRet;
	}

	void DoSort(int iSubItem=-1,BOOL bToggle=FALSE)
	{
		if( m_Sort.CurrentSubItem != -1 )  //  previous sort column
			ListViewEx_SetHeaderArrow(m_hWndList,m_Sort.CurrentSubItem,0); // clear previous column header mark

		if( bToggle )
		{
			if( m_Sort.CurrentSubItem != iSubItem )
				m_Sort.Direction = 1;     // current column changed: new current column always ascend sort.
			else
				m_Sort.Direction *= -1;   // toggle.
		}

		SORT_PARAM op = {0};
		op.id              = iSubItem;
		op.direction       = m_Sort.Direction; // must 1 or -1, do not use 0
		ListView_SortItems(m_hWndList,&CNTFSRecordInformationWindow::CompareProc,&op);

		ListViewEx_SetHeaderArrow(m_hWndList,iSubItem,m_Sort.Direction);

		m_Sort.CurrentSubItem = iSubItem;
	}
#endif
	VOID UpdateLayout(int cx=-1,int cy=-1,BOOL absSplitPos=FALSE)
	{
		if( cx == -1 && cy == -1 )
		{
			RECT rc;
			GetClientRect(m_hWnd,&rc);
			cx = _RECT_WIDTH(rc);
			cy = _RECT_HIGHT(rc);
		}

		HDWP hdwp = BeginDeferWindowPos(1);

		DeferWindowPos(hdwp,m_hWndList,NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOCOPYBITS);

		EndDeferWindowPos(hdwp);
	}

	void InitializeListView()
	{
		m_hWndList = CreateWindowEx(0,
							  WC_LISTVIEW, 
                              L"", 
#if 0
							  WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_TABSTOP|LVS_REPORT|LVS_SHOWSELALWAYS,
#else
							  WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_TABSTOP|LVS_REPORT|LVS_SHOWSELALWAYS|LVS_NOSORTHEADER,
#endif
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		m_hFont = GetGlobalFont(m_hWnd);
		m_hFontHeader = GetIconFont();

		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)m_hFont,0);
		SendMessage(ListView_GetHeader(m_hWndList),WM_SETFONT,(WPARAM)m_hFontHeader,0);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_INFOTIP|LVS_EX_LABELTIP);
	
		HIMAGELIST himl;
		himl = ImageList_Create(
					6,
					_DPI_Adjust_Y( GetSystemMetrics(SM_CYSMICON) + 8 ),
					ILC_COLOR32|ILC_MASK,1,1);
		ListView_SetImageList(m_hWndList,himl,LVSIL_SMALL);

		_EnableVisualThemeStyle(m_hWndList);
	
		RECT rc;
		GetClientRect(m_hWndList,&rc);
		int cx = _RECT_WIDTH(rc);

		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH;

		struct {
			PWSTR Text;
			int fmt;
		} Columns[] = {
			{L"Name",          LVCFMT_LEFT},
			{L"Type Id",       LVCFMT_RIGHT},
			{L"Size",          LVCFMT_RIGHT},
			{L"Index",         LVCFMT_RIGHT},
			{L"Segment FRN",   LVCFMT_RIGHT},
			{L"VCN Count",     LVCFMT_RIGHT},
		};

		{
			lvc.fmt = LVCFMT_LEFT;
			lvc.pszText = L"";
			lvc.cx = 0;
			lvc.iSubItem = 0;
			lvc.iOrder = 0;
			ListView_InsertColumn(m_hWndList,0,&lvc);
		}

		for(int i = 0; i < ARRAYSIZE(Columns); i++)
		{
			lvc.fmt = Columns[i].fmt;
			lvc.pszText = Columns[i].Text;
			lvc.cx = (int)((double)cx * 0.3);
			lvc.iSubItem = i+1;
			ListView_InsertColumn(m_hWndList,i+1,&lvc);
		}

		// If a column is added to a list-view control with index 0 (the leftmost column),
		// it is always LVCFMT_LEFT. Setting other flags on column 0 does not override that alignment. 
		// Therefore if you keep inserting columns with index 0, the text in all columns are left-aligned. 
		// If you want the first column to be right-aligned or centered you can make a dummy column, 
		// then insert one or more columns with index 1 or higher and specify the alignment you require.
		// Finally delete the dummy column
		ListView_DeleteColumn(m_hWndList,0);

		ListView_SetColumnWidth(m_hWndList,0,DPI_SIZE_CX(180));
		ListView_SetColumnWidth(m_hWndList,1,DPI_SIZE_CX(80));
		ListView_SetColumnWidth(m_hWndList,2,DPI_SIZE_CX(80));
		ListView_SetColumnWidth(m_hWndList,3,DPI_SIZE_CX(80));
		ListView_SetColumnWidth(m_hWndList,4,DPI_SIZE_CX(80));
		ListView_SetColumnWidth(m_hWndList,5,DPI_SIZE_CX(80));

		m_Sort.CurrentSubItem = -1;
		m_Sort.Direction = -1;

		setErrorMessage( L"No record data." );
	}

	int InsertItem(
			HWND hWndList,
			int iItem,
			int ItemType,
			LONGLONG FileId,
			int iIndex,
			ATTRIBUTE_RECORD_HEADER *attribute,
			int ialIndex,
			ATTRIBUTE_LIST_ENTRY *alentry,
			ATTRIBUTE_RECORD_HEADER *segment_attribute
			)
	{
		NTFSRECORD_LIST_ITEM *pItem = new NTFSRECORD_LIST_ITEM;
		if( pItem == NULL )
			return -1;

		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);
	
		ZeroMemory(pItem,sizeof(NTFSRECORD_LIST_ITEM));

		if( ItemType == ITEM_RECORD_ATTRIBUTE )
		{
			pItem->itemType         = ITEM_RECORD_ATTRIBUTE;
			pItem->Attribute        = attribute;
			pItem->AttrListEntry    = nullptr;
			pItem->RecordIndex      = iIndex;
			pItem->VcnCount         = 0;
			pItem->SegmentIndex     = -1;
			pItem->AttributeList    = nullptr;
		}
		else
		{
			pItem->itemType         = ITEM_RECORD_ATTRIBUTE_ENTRY;
			pItem->Attribute        = segment_attribute;
			pItem->AttrListEntry    = alentry;
			pItem->RecordIndex      = iIndex;
			pItem->SegmentIndex     = ialIndex;
			pItem->AttributeList    = attribute;

			if( segment_attribute->FormCode == NONRESIDENT_FORM )
				GetNtfsClusterMappingPair(segment_attribute,NULL,&pItem->VcnCount);
		}

		pItem->FileId = FileId;

		WCHAR szType[64];
		ULONG TypeCode;
		if( pItem->itemType == 0 )
			TypeCode = attribute->TypeCode;
		else
			TypeCode = alentry->AttributeTypeCode;
		if( GetNtfsRecordNameString( TypeCode, szType, ARRAYSIZE(szType), 0) == 0 )
			StringCchCopy(szType,ARRAYSIZE(szType),L"(Unknown)");
		pItem->pszDisplayName = _MemAllocString(szType);

		LVITEM lvi = {0};
		lvi.mask    = LVIF_PARAM|LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT;
		lvi.pszText = (PWSTR)LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;
		lvi.iItem   = iItem;
		lvi.iIndent = pItem->itemType == ITEM_RECORD_ATTRIBUTE ? 0 : 2;

		iItem = ListView_InsertItem(hWndList,&lvi);
	
		return iItem;
	}

	void makeErrorMessage( PCWSTR psz )
	{
		_SafeMemFree( m_pszErrorMessage );
		m_pszErrorMessage = _MemAllocString(psz);
	}

	void setErrorMessage( PCWSTR psz )
	{
		makeErrorMessage( psz );
	}

	void setErrorMessage( HRESULT hr )
	{
		WCHAR sz[MAX_PATH];
		StringCchPrintf(sz,MAX_PATH,L"Error Code: 0x%08X",hr);
	}

	LRESULT OnSelItem(WPARAM,LPARAM)
	{
		if( ListView_GetItemCount(m_hWndList) > 0 )
		{
			ListViewEx_SetCurSel(m_hWndList,0);
			return 0;
		}
		return (LRESULT)-1;
	}

	LRESULT OnChDir(PCWSTR pszFilePath,FILEITEMEX *pFIEx)
	{
		return 0;
	}

	LRESULT OnSetPath(PCWSTR pszFilePath,FILEITEMEX *pFIEx)
	{
		LRESULT lResult = 0;

		if( pszFilePath == NULL && pFIEx == NULL )
		{
			// Clear list
			SetRedraw(m_hWndList,FALSE);
			ListView_DeleteAllItems(m_hWndList);
			FreeRecordData();
			SetRedraw(m_hWndList,TRUE);
			ListViewEx_ResetEmptyText(m_hWndList);
			return 0;
		}

		_SafeMemFree(m_pszFilePath);
		m_pszFilePath = _MemAllocString( pszFilePath );
		m_pszFileName = _MemAllocString( pFIEx->hdr.FileName );

		if( m_pszFilePath == NULL || m_pszFileName == NULL )
		{
			_SafeMemFree(m_pszFilePath);
			_SafeMemFree(m_pszFileName);
			return 0;
		}

		ULONG Status;

		// Open volume
		HANDLE hVolume = NULL;
		Status = NtDosOpenVolume(&hVolume,pszFilePath,FILE_READ_DATA|FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_SYNCHRONOUS_IO_NONALERT);
		if( Status != NO_ERROR )
		{
			// Status
			return 0;
		}

		// Opne file
		LARGE_INTEGER FileId = {0}; // FRN

		if( pFIEx->FileId.QuadPart != 0 )
		{
			FileId.QuadPart = pFIEx->FileId.QuadPart;
		}
		else
		{
			HANDLE hFile;
			NTSTATUS Status;
			Status = OpenFileEx_W(&hFile,pszFilePath,FILE_READ_ATTRIBUTES|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_SYNCHRONOUS_IO_NONALERT);
			if( Status == STATUS_SUCCESS )
			{
				GetFileId(hFile,&FileId);
				CloseHandle(hFile);
			}
		}

		FillItems(hVolume,FileId.QuadPart);

		CloseHandle(hVolume);

		return lResult;
	}

	HRESULT FreeRecordData()
	{
		int i,cPtrs = DPA_GetPtrCount( m_hdpa );

		for(i = 0; i < cPtrs; i++)
		{
			NTFS_FILE_RECORD_OUTPUT_BUFFER *pBuf = (NTFS_FILE_RECORD_OUTPUT_BUFFER *)DPA_GetPtr(m_hdpa,i);
			FreeNtfsMemory(pBuf);
		}

		DPA_DeleteAllPtrs(m_hdpa);

		return S_OK;
	}

	HRESULT FillItems(HANDLE hVolume,LONGLONG FileId)
	{
		HRESULT hr;

		SetRedraw(m_hWndList,FALSE);
		ListView_DeleteAllItems(m_hWndList);

		FreeRecordData();

		hr = EnumItems(hVolume,FileId,0);

		ListView_SetColumnWidth(m_hWndList,1,LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hWndList,4,LVSCW_AUTOSIZE_USEHEADER);

	    if( ListView_GetItemCount(m_hWndList) == 0 )
		{
			ListViewEx_ResetEmptyText(m_hWndList);
		}
		else
		{
#if 0
			DoSort(m_Sort.CurrentSubItem,FALSE);
#endif
		}

		SetRedraw(m_hWndList,TRUE);

		return hr;
	}

	HRESULT EnumItems(HANDLE hVolume,LONGLONG FileId,int Type)
	{
		HRESULT hr = E_FAIL;

		ULONG Result;
		NTFS_FILE_RECORD_OUTPUT_BUFFER *pBuf;
		if( (Result = GetNtfsFileRecordInformation( NULL, hVolume, FileId, (PVOID*)&pBuf, NULL )) == NO_ERROR )
		{
			EnumRecordData(hVolume,FileId,Type,pBuf);
		}
		else
		{
			hr = E_FAIL;
		}

		return hr;
	}

	HRESULT EnumRecordData(HANDLE hVolume,LONGLONG FileId,int Type,NTFS_FILE_RECORD_OUTPUT_BUFFER *pnfrec)
	{
		MULTI_SECTOR_HEADER *psechdr = (MULTI_SECTOR_HEADER *)pnfrec->FileRecordBuffer;
	
		ULONG sign = *((PULONG)(psechdr->Signature));
	
		if( *(INT*)&psechdr->Signature != 'ELIF' ) // 46 49 4c 45 == 0x454C4946 "FILE"
		{
			return E_FAIL;
		}

		int iSegmentArrayIndex = DPA_AppendPtr(m_hdpa,pnfrec);
	
		FILE_RECORD_SEGMENT_HEADER *pfrecSeg = (FILE_RECORD_SEGMENT_HEADER *)psechdr;
		ATTRIBUTE_RECORD_HEADER *pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)_ADDOFFSET(pfrecSeg,pfrecSeg->FirstAttributeOffset);
	
		while( pAttrRecHdr->TypeCode != $END )
		{
			InsertItem(m_hWndList,-1,0,FileId,iSegmentArrayIndex,pAttrRecHdr,-1,nullptr,nullptr);

			switch( pAttrRecHdr->TypeCode )
			{
				case $ATTRIBUTE_LIST:
				{
					PVOID pa = NULL;
					ULONG cb;
					ULONG i,c;
					GetNtfsRecordAttributeList(pAttrRecHdr,&pa,&cb);
					i = 0;
					c = 0;
					ATTRIBUTE_LIST_ENTRY *entry = (ATTRIBUTE_LIST_ENTRY *)pa;
					while( ((ULONG_PTR)entry - (ULONG_PTR)pa) < cb )
					{
						ATTRIBUTE_RECORD_HEADER *pattrhdr = NULL;

						i++;
						c += entry->RecordLength;

						NTFS_FILE_RECORD_OUTPUT_BUFFER *pBuf;
						ULONG Status;
						if( (Status = GetNtfsFileRecordInformation( NULL, hVolume, entry->SegmentReference.QuadPart, (PVOID*)&pBuf, NULL )) == NO_ERROR )
						{
							int iBufferIndex = DPA_AppendPtr(m_hdpa,pBuf);

							FindNtfsRecordAttribueFromBuffer(pBuf,
									entry->AttributeTypeCode,
									entry->Reserved,
									&pattrhdr,0);

							InsertItem(m_hWndList,-1,1,FileId,iSegmentArrayIndex,pAttrRecHdr,iBufferIndex,entry,pattrhdr);
						}

						entry = (ATTRIBUTE_LIST_ENTRY *)(((CHAR *)entry) + entry->RecordLength);
					}
					break;
				}
			}
	
			pAttrRecHdr = (ATTRIBUTE_RECORD_HEADER *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->RecordLength);
		}
	
		return S_OK;
	}

	//////////////////////////////////////////////////////////////////////////////
	// Text dump engine

	static HRESULT WINAPI CallbackProc(INT Reason,INT IfType,PVOID pi,ULONG_PTR ctx)
	{
		CALLBACKPARAM *pcp = (CALLBACKPARAM *)ctx;
		return pcp->pThis->_CallbackProc(Reason,IfType,pi,(ULONG_PTR)pcp->pItem);
	}

	HRESULT _CallbackProc(INT Reason,INT IfType,PVOID pi,ULONG_PTR ctx)
	{
		CDebugText *pc = static_cast<CDebugText *>(pi);
		CDebugText *pDebugText = pc;
		const int cwidth = 20;
	
		WCHAR sz[4096];
		DWORD cch = ARRAYSIZE(sz);

		NTFSRECORD_LIST_ITEM *pItem = (NTFSRECORD_LIST_ITEM *)ctx;

		if( pItem->itemType == ITEM_RECORD_ATTRIBUTE_ENTRY )
		{
			pc->AddText( L"\r\nAttribute List Entry:\r\n\r\n" );

			pDebugText->dumpDWORD(cwidth, L"Type Code",   pItem->AttrListEntry->AttributeTypeCode);
			pDebugText->dumpWORD(cwidth,  L"Index",       pItem->AttrListEntry->Reserved);
			pDebugText->dumpWORD(cwidth,  L"Length",      pItem->AttrListEntry->RecordLength,TRUE);
			pDebugText->dumpQWORD(cwidth, L"Lowest Vcn",  pItem->AttrListEntry->LowestVcn);
			pDebugText->dumpQWORD(cwidth, L"Segamnt FRN", pItem->AttrListEntry->SegmentReference.QuadPart);
			pDebugText->dumpName(cwidth,  L"Name",        pItem->AttrListEntry->AttributeNameLength,pItem->AttrListEntry->AttributeName);
		}

		ATTRIBUTE_RECORD_HEADER *pAttrRecHdr = pItem->Attribute;
	
		pc->AddText( L"\r\nAttribute Record:\r\n\r\n" );
	
		pDebugText->dumpDWORD(cwidth, L"Record Length", pAttrRecHdr->RecordLength);
		pDebugText->dumpDWORD(cwidth, L"Form Code",     pAttrRecHdr->FormCode);
		pDebugText->dumpDWORD(cwidth, L"Flags",         pAttrRecHdr->Flags);
		pDebugText->dumpDWORD(cwidth, L"Name Length",   pAttrRecHdr->NameLength);
		pDebugText->dumpDWORD(cwidth, L"Name Offset",   pAttrRecHdr->NameOffset);

		PWSTR pName = (PWSTR)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->NameOffset);
		ASSERT( pName != NULL );
		pDebugText->dumpName(cwidth,L"Name",pAttrRecHdr->NameLength,pName);
	
		if( pAttrRecHdr->FormCode == RESIDENT_FORM )
		{
			pDebugText->AddText( L"\r\nResident Data:\r\n\r\n" );
	
			pDebugText->dumpDWORD(cwidth, L"Resident Flags", pAttrRecHdr->Form.Resident.ResidentFlags);
			pDebugText->dumpDWORD(cwidth, L"Value Length",   pAttrRecHdr->Form.Resident.ValueLength);
			pDebugText->dumpDWORD(cwidth, L"Value Offset",   pAttrRecHdr->Form.Resident.ValueOffset);
		}
		else
		{
			pDebugText->AddText( L"\r\nNonresident Data:\r\n\r\n" );

			pDebugText->dumpQWORD(cwidth, L"Lowest Vcn",        pAttrRecHdr->Form.Nonresident.LowestVcn);
			pDebugText->dumpQWORD(cwidth, L"Highest Vcn",       pAttrRecHdr->Form.Nonresident.HighestVcn);
			pDebugText->dumpBYTE(cwidth,  L"Compression Unit",  pAttrRecHdr->Form.Nonresident.CompressionUnit);
			pDebugText->dumpQWORD(cwidth, L"Allocated Length",  pAttrRecHdr->Form.Nonresident.AllocatedLength);
			pDebugText->dumpQWORD(cwidth, L"File Size",         pAttrRecHdr->Form.Nonresident.FileSize);
			pDebugText->dumpQWORD(cwidth, L"Valid Data Length", pAttrRecHdr->Form.Nonresident.ValidDataLength);
			if( pAttrRecHdr->Form.Nonresident.CompressionUnit != 0 )
				pDebugText->dumpQWORD(cwidth, L"Total Allocated", pAttrRecHdr->Form.Nonresident.TotalAllocated);
		}
	
		switch( pAttrRecHdr->TypeCode )
		{
			case $STANDARD_INFORMATION:
				DumpAttribute_StandardInformation(
					pc,
					(STANDARD_INFORMATION *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Resident.ValueOffset),
					pAttrRecHdr->Form.Resident.ValueLength);
				break;
			case $FILE_NAME:
				DumpAttribute_FileNameInformation(
					pc,
					(FILE_NAME *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Resident.ValueOffset),
					pAttrRecHdr->Form.Resident.ValueLength);
				break;
			case $INDEX_ROOT:
				DumpAttribute_IndexRoot(
					pc,
					(INDEX_ROOT *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Resident.ValueOffset),
					pAttrRecHdr->Form.Resident.ValueLength);
				break;
		}
	
		if( pItem->Attribute->FormCode == NONRESIDENT_FORM )
		{
			pc->AddText( L"\r\n\r\nNonregident Data Location:\r\n\r\n" );
			pc->AddText( L"      VCN            Next VCN       Current LCN            LCN Count\r\n" );
			pc->AddText( L"----------------+----------------+----------------+-------------------------\r\n" );
	
			CLUSTER_MAPPING_PAIR *pair = NULL;
			LONGLONG i,n;
			GetNtfsClusterMappingPair(pAttrRecHdr,&pair,&n);
			for(i = 0; i < n; i++)
			{
				WCHAR sz[1024];
				WCHAR szCnt[32];
				WCHAR szCluster[32];
				_CommaFormatString(pair[i].LcnCount,szCnt);
				if( pair[i].CurrentLcn != -1 )
					StringCchPrintf(szCluster,ARRAYSIZE(szCluster),L"%16I64X",pair[i].CurrentLcn);
				else
					StringCchCopy(szCluster,ARRAYSIZE(szCluster),L"-");
				StringCchPrintf(sz,1024,L"%16I64X %16I64X %16s %25s\r\n",pair[i].CurrentVcn,pair[i].NextVcn,szCluster,szCnt);
				pc->AddText(sz);
			}
			FreeNtfsMemory(pair);
		}
	
		pc->AddText( L"\r\nRecord Dump:\r\n\r\n" );
		CryptBinaryToString( 
			(BYTE*)pAttrRecHdr,
			pAttrRecHdr->RecordLength,
			CRYPT_STRING_HEXASCIIADDR,
			sz,&cch);
		pc->AddText( sz );
	
		if( pItem->Attribute->FormCode == RESIDENT_FORM )
		{
			if( pAttrRecHdr->Form.Resident.ValueLength > 0 )
			{
				pc->AddText( L"\r\nData in the Nonresident Record:\r\n\r\n" );
	
				BYTE *pb = (BYTE *)_ADDOFFSET(pAttrRecHdr,pAttrRecHdr->Form.Resident.ValueOffset);
	
				CryptBinaryToString( 
						(BYTE*)pb,
						pAttrRecHdr->Form.Resident.ValueLength,
						CRYPT_STRING_HEXASCIIADDR,
						sz,&cch);

				pc->AddText( sz );
			}
		}
	
		return S_OK;
	}
	
	void DumpAttribute_StandardInformation(CDebugText *pDebugText,STANDARD_INFORMATION *pStdInfo,ULONG cb)
	{
		const int cwidth = 20;
	
		pDebugText->AddText( L"\r\nStandard Information:\r\n\r\n" );
	
		pDebugText->dumpDateTime(cwidth,L"Creation",pStdInfo->CreationTime);
		pDebugText->dumpDateTime(cwidth,L"Last Modification",pStdInfo->LastModificationTime);
		pDebugText->dumpDateTime(cwidth,L"Last Change",pStdInfo->LastChangeTime);
		pDebugText->dumpDateTime(cwidth,L"Last Access",pStdInfo->LastAccessTime);
		pDebugText->dumpDWORD(cwidth,L"File Attributes",pStdInfo->FileAttributes);
		pDebugText->dumpDWORD(cwidth,L"Owner Id",pStdInfo->OwnerId);
		pDebugText->dumpDWORD(cwidth,L"Security Id",pStdInfo->SecurityId);
		pDebugText->dumpDWORD(cwidth,L"Class Id",pStdInfo->ClassId);
		pDebugText->dumpQWORD(cwidth,L"Usn",	pStdInfo->Usn);
		pDebugText->dumpQWORD(cwidth,L"Quota Charged",pStdInfo->QuotaCharged);
	}
	
	void DumpAttribute_FileNameInformation(CDebugText *pDebugText,FILE_NAME *pFileName,ULONG cb)
	{
		pDebugText->AddText( L"\r\nFile Name:\r\n\r\n" );
	
		const int cwidth = 20;
		DumpAttribute_FileNameStructure(pDebugText,pFileName,cb,cwidth);
	}
	
	void DumpAttribute_IndexRoot(CDebugText *pDebugText,INDEX_ROOT *IndexRoot,ULONG Length)
	{
		pDebugText->AddText( L"\r\nIndex Root\r\n\r\n" );

		const int cwidth = 25;

		pDebugText->dumpDWORD(cwidth,L"Indexed Attribute Type",IndexRoot->IndexedAttributeType);
		pDebugText->dumpDWORD(cwidth,L"Collation Rule",IndexRoot->CollationRule);
		pDebugText->dumpDWORD(cwidth,L"Bytes Per IndexBuffer",IndexRoot->BytesPerIndexBuffer,TRUE);
		pDebugText->dumpBYTE(cwidth,L"Blocks Per IndexBuffer",IndexRoot->BlocksPerIndexBuffer);

		// DIRECTORY_INDEX array
		pDebugText->dumpDWORD(cwidth,L"First Index Entry",IndexRoot->IndexHeader.FirstIndexEntry);
		pDebugText->dumpDWORD(cwidth,L"First Free Byte",IndexRoot->IndexHeader.FirstFreeByte);
		pDebugText->dumpDWORD(cwidth,L"Bytes Available",IndexRoot->IndexHeader.BytesAvailable);
		pDebugText->dumpBYTE(cwidth,L"Flags",IndexRoot->IndexHeader.Flags);

		pDebugText->AddText( L"\r\n" );

		INDEX_ENTRY *entry = (INDEX_ENTRY *)_ADDOFFSET( &IndexRoot->IndexHeader, IndexRoot->IndexHeader.FirstIndexEntry );

		WCHAR szTitle[32];
		int iIndex = 0;

		for(;;)
		{
			StringCchPrintf(szTitle,ARRAYSIZE(szTitle),L"Entery #%d\r\n",iIndex++);
			pDebugText->AddText(szTitle);

			pDebugText->dumpWORD(cwidth,L"Length",entry->Length);
			pDebugText->dumpWORD(cwidth,L"Attribute Length",entry->AttributeLength);
			pDebugText->dumpWORD(cwidth,L"Entry Flags",entry->Flags);

			if( entry->AttributeLength )
			{
				pDebugText->dumpQWORD(cwidth,L"File Reference",entry->Form.FileReference.QuadPart);
				DumpAttribute_FileNameStructure(pDebugText,&entry->Name,entry->AttributeLength,cwidth);
			}

			if( entry->Flags & INDEX_ENTRY_NODE )
			{
				VCN *vcn = (VCN *)_ADDOFFSET( entry, entry->Length - sizeof(VCN) );
	
				if( vcn )
					pDebugText->dumpQWORD(cwidth,L"VCN",*vcn);
			}

			if( entry->Flags & INDEX_ENTRY_END )
				break;

			entry = (INDEX_ENTRY *)_ADDOFFSET(entry,entry->Length);

			pDebugText->AddText( L"\r\n" );
		}
	}

	void DumpAttribute_FileNameStructure(CDebugText *pDebugText,FILE_NAME *pFileName,ULONG cb,const int cwidth)
	{
		pDebugText->dumpName(cwidth,L"Name", (ULONG)pFileName->FileNameLength,pFileName->FileName);
		pDebugText->dumpBYTE(cwidth,L"Name Length", (ULONG)pFileName->FileNameLength);
		pDebugText->dumpBYTE(cwidth,L"Flags", (ULONG)pFileName->Flags);

		pDebugText->dumpQWORD(cwidth,L"Parent Directory",pFileName->ParentDirectory);

		pDebugText->dumpDateTime(cwidth,L"Createion",pFileName->BasicInfo.CreationTime);
		pDebugText->dumpDateTime(cwidth,L"Last Modification",pFileName->BasicInfo.LastModificationTime);
		pDebugText->dumpDateTime(cwidth,L"Last Access",pFileName->BasicInfo.LastAccessTime);
		pDebugText->dumpDateTime(cwidth,L"Last Change",pFileName->BasicInfo.LastChangeTime);

		pDebugText->dumpQWORD(cwidth,L"Allocated Length",pFileName->BasicInfo.AllocatedLength);
		pDebugText->dumpQWORD(cwidth,L"File Size",pFileName->BasicInfo.FileSize);
		pDebugText->dumpDWORD(cwidth,L"File Attributes",pFileName->BasicInfo.FileAttributes);
		pDebugText->dumpWORD(cwidth,L"Packed EA Size",pFileName->BasicInfo.PackedEaSize);
	}
};
