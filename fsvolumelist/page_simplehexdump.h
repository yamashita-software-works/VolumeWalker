#pragma once
//***************************************************************************
//*                                                                         *
//*  page_simplehexdump.h                                                   *
//*                                                                         *
//*  Simple Hex Dump Page                                                   *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2024-02-22                                                     *
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
#include <devguid.h>
#include "simpletoolbar.h"
#include "dialogs.h"

#define MAX_VOLME_NAME_LENGTH  (MAX_PATH)

struct CSimpleHexDumpItem
{
public:
	LPBYTE BufferPos;
	LONGLONG Offset;

	CSimpleHexDumpItem()
	{
		BufferPos = NULL;
		Offset = 0;
	}
};

//////////////////////////////////////////////////////////////////////////////

class CSimpleHexDumpPage :
	public CPageWndBase,
	public CFindHandler<CSimpleHexDumpPage>
{
	HWND m_hWndList;

	HFONT m_hFont;

	CColumnList m_columns;

	typedef struct READ_POINT_INFO
	{
		ULONG Status;
		LARGE_INTEGER BaseOffset;
		LARGE_INTEGER ReadOffset;
		LARGE_INTEGER Size;
		ULONG Length;
		ULONG ReadLength;
		
		ULONG ClusterLength;
		ULONG SectorLength;

		int ByteWidth;
		BYTE *Buffer;
		WCHAR Name[MAX_VOLME_NAME_LENGTH];
		READ_POINT_INFO()
		{
			memset(this,0,sizeof(READ_POINT_INFO));
		}

		~READ_POINT_INFO()
		{
		}
	} READ_POINT_INFO;

	READ_POINT_INFO m_rd;

	CSimpleToolbar m_Toolbar;
	HWND m_hWndToolBar;

	PWSTR m_pszErrorMessage;

	// The sector offset to the first logical cluster number (LCN) 
	// of the file system relative to the start of the volume.
	RETRIEVAL_POINTER_BASE m_rpb;

	LARGE_INTEGER m_liFileAreaOffsetByte;

	BOOL m_bAuto;

public:
	HWND GetListView() const { return m_hWndList; }

public:
	CSimpleHexDumpPage()
	{
		m_hWndList = NULL;
		m_hWndToolBar = NULL;
		m_hFont = NULL;
		m_pszErrorMessage = NULL;
		ZeroMemory(&m_rd,sizeof(m_rd));
		m_rd.ByteWidth = 16;
		m_liFileAreaOffsetByte.QuadPart = 0;
		ZeroMemory(&m_rpb,sizeof(m_rpb));
		m_bAuto = FALSE;
	}

	~CSimpleHexDumpPage()
	{
		_SafeMemFree(m_pszErrorMessage);
	}

	virtual HRESULT OnInitPage(PVOID,DWORD,PVOID)
	{
		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		_EnableVisualThemeStyle(m_hWndList);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);

		if( !LoadColumns(m_hWndList) )
		{
			InitColumns(m_hWndList);
		}

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hWndList);
#endif

	    TBBUTTON tbButtons[] = 
		{
			{ MAKELONG(0, 0), ID_BACK, TBSTATE_ENABLED, BTNS_AUTOSIZE|BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"Back" },
	        { MAKELONG(0, 0), ID_NEXT, TBSTATE_ENABLED, BTNS_AUTOSIZE|BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"Next" },
//	        { MAKELONG(0, 0), 0,       0,               0,                           {0}, 0, (INT_PTR)NULL },    // Insert space
	        { MAKELONG(0, 0), 0,       0,               BTNS_SEP,                    {0}, 0, (INT_PTR)NULL },    // Draw vertical line for separate
	        { MAKELONG(0, 0), ID_FIRST,TBSTATE_ENABLED, BTNS_AUTOSIZE|BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"First" },
	        { MAKELONG(0, 0), ID_LAST, TBSTATE_ENABLED, BTNS_AUTOSIZE|BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"Last" },
	        { MAKELONG(0, 0), ID_HOME, TBSTATE_ENABLED, BTNS_AUTOSIZE|BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"Home" },
	        { MAKELONG(0, 0), 0,       0,               BTNS_SEP,                    {0}, 0, (INT_PTR)NULL },
	        { MAKELONG(0, 0), ID_GOTO, TBSTATE_ENABLED, BTNS_AUTOSIZE|BTNS_SHOWTEXT, {0}, 0, (INT_PTR)L"Go To" },
		};

		m_hWndToolBar = m_Toolbar.CreateSimpleToolbar(m_hWnd,tbButtons,ARRAYSIZE(tbButtons));

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

		UpdateToolbarButtons();

		return S_OK;
	}

	virtual HRESULT OnInitLayout(const RECT *prc)
	{
		return E_NOTIMPL;
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
		if( m_hFont )
			DeleteObject(m_hFont);

		_SafeMemFree(m_rd.Buffer);

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
			case LVN_GETEMPTYMARKUP:
				return OnGetEmptyMarkup(pnmhdr);
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
			case TBN_GETINFOTIP:
				return OnGetToolbarInfoTip(pnmhdr);
		}
		return 0;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		if( IsXpThemeEnabled() )
			SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);
		pnmhdr->hwndFrom = m_hWnd;
		pnmhdr->idFrom = GetWindowLong(m_hWnd,GWL_ID);
		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)pnmhdr);
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

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;
		if( pnmlv->lParam )
		{
			CSimpleHexDumpItem *pItem = (CSimpleHexDumpItem *)pnmlv->lParam;
			delete pItem;
		}
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

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		CSimpleHexDumpItem *pItem = (CSimpleHexDumpItem *)pdi->item.lParam;

		int id = (int)ListViewEx_GetHeaderItemData(pnmhdr->hwndFrom,pdi->item.iSubItem);

		if( pdi->item.mask & LVIF_TEXT )
		{
			if( COLUMN_Address == id )
			{
				CSimpleHexDumpItem *pItem = (CSimpleHexDumpItem *)pdi->item.lParam;
#if 0
				StringCchPrintf(pdi->item.pszText,pdi->item.cchTextMax,L"%016I64X",pdi->item.lParam);
#else
				LARGE_INTEGER li;
				li.QuadPart = (LONGLONG)pItem->Offset + (m_rd.ReadOffset.QuadPart - (m_rd.ReadOffset.QuadPart % m_rd.Length));
				StringCchPrintf(pdi->item.pszText,pdi->item.cchTextMax,L"%08X`%08X",li.HighPart,li.LowPart);
#endif
			}
			else if( COLUMN_DumpHex == id )
			{
				ZeroMemory(pdi->item.pszText,pdi->item.cchTextMax*sizeof(WCHAR));
				PBYTE pb = ((CSimpleHexDumpItem *)pdi->item.lParam)->BufferPos;
				PWSTR psz = pdi->item.pszText;
				SIZE_T cch = pdi->item.cchTextMax;
				for(int i = 0; i < m_rd.ByteWidth; i++,psz += 3,cch -= 3,pb++)
				{
 					StringCchPrintf(psz,cch,L"%02X ",*pb);
				}

			}
			else if( COLUMN_DumpChar == id )
			{
				CHAR buf[64+1];
				CHAR *psz = buf;
				SIZE_T cch = _countof(buf);
				PBYTE pb = ((CSimpleHexDumpItem *)pdi->item.lParam)->BufferPos;
				for(int i = 0; i < m_rd.ByteWidth; i++,psz++,cch--,pb++)
				{
#if 0
					StringCchPrintfA(psz,cch,"%c",isprint(*pb) ? *pb : '.');
#else
					StringCchPrintfA(psz,cch,"%c",(0x20 <= *pb && *pb <= 0x7E) ? *pb : '.');
#endif
				}
				MultiByteToWideChar(CP_ACP,0,buf,-1,pdi->item.pszText,pdi->item.cchTextMax);
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

	LRESULT OnGetToolbarInfoTip(NMHDR *pnmhdr)
	{
		NMTBGETINFOTIP *pnmtbit = (NMTBGETINFOTIP *)pnmhdr;

		PWSTR pszText = NULL;
		switch( pnmtbit->iItem )
		{
			case ID_BACK:
				pszText = L"Ctrl+K";
				break;
			case ID_NEXT:
				pszText = L"Ctrl+L";
				break;
			case ID_GOTO:
				pszText = L"Ctrl+G";
				break;
		}

		if( pszText )
			StringCchCopy(pnmtbit->pszText,pnmtbit->cchTextMax,pszText);

		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		CSimpleHexDumpItem *pItem = (CSimpleHexDumpItem *)ListViewEx_GetItemData(m_hWndList,iItem);

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
			case WM_COMMAND:
				InvokeCommand(LOWORD(wParam));
				return 0;
			case WM_ERASEBKGND:
			{
				HDC hdc = (HDC)wParam;
				RECT rc;
				GetClientRect(m_hWnd,&rc);

				DWORD dw;
				dw = (DWORD)SendMessage(m_hWndToolBar,TB_GETBUTTONSIZE,0,0);
				rc.bottom = HIWORD(dw);
#if 0
				HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
#else
				HBRUSH hbr = CreateSolidBrush(RGB(243,243,243));
#endif
				FillRect(hdc,&rc,hbr);
				DeleteObject(hbr);
				return 1;
			}
			case WM_QUERY_MESSAGE:
			{
				if( lParam != 0 )
				{
					QM_PARAM *pParam = (QM_PARAM *)lParam;

					if( wParam == QMT_VOLUMEPATH && lParam != 0 )
					{
						StringCchCopy(pParam->VolumePath,pParam->dwLength,m_rd.Name);
						return (LRESULT)TRUE;
					}
					else if( wParam == QMT_STARTOFFSET && lParam != 0 )
					{
						pParam->liValue.QuadPart = m_rd.BaseOffset.QuadPart;
						return (LRESULT)TRUE;
					}
				}
				break;
			}
			case PM_FINDITEM:
				return CFindHandler<CSimpleHexDumpPage>::OnFindItem(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	void UpdateLayout(int cx,int cy)
	{
		int cyToolBar = 0;

		SendMessage(m_hWndToolBar,WM_SIZE,0,MAKELPARAM(cx,cy));

		DWORD dw;
		dw = (DWORD)SendMessage(m_hWndToolBar,TB_GETBUTTONSIZE,0,0);
		cyToolBar = HIWORD(dw);

		HDWP hdwp;
		hdwp = BeginDeferWindowPos( 1 );

		if( m_hWndList )
		{
			DeferWindowPos(hdwp,m_hWndList,NULL,0,cyToolBar,cx,cy-cyToolBar,SWP_NOZORDER);
		}

		EndDeferWindowPos(hdwp);
	}

	void InitColumns(HWND hWndList)
	{
		LVCOLUMN lvc = {0};

		static COLUMN columns_filelist[] = {
			{ COLUMN_Address,   L"Address",    0, 0, LVCFMT_LEFT },
			{ COLUMN_DumpHex,   L"Hex",        1, 0, LVCFMT_LEFT },
			{ COLUMN_DumpChar,  L"Character",  2, 0, LVCFMT_LEFT },
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

	int _InsertLine(int iItem,PBYTE pb)
	{
		CSimpleHexDumpItem *pItem = new CSimpleHexDumpItem;
		pItem->BufferPos = pb;
		pItem->Offset = (LONGLONG)(pb - m_rd.Buffer);

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = iItem;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = (PWSTR)LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;

		iItem   = ListView_InsertItem(m_hWndList,&lvi);

		return iItem;
	}

	HRESULT FillItems()
	{
		CWaitCursor wait;

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		ReadSector();

		if( m_rd.Status != 0 )
		{
			SetErrorState();

			SetRedraw(m_hWndList,TRUE);
			return HRESULT_FROM_WIN32(m_rd.Status);
		}

		ULONG i;
		LPBYTE pb = m_rd.Buffer;

		for(i = 0; i < (m_rd.ReadLength / m_rd.ByteWidth); i++)
		{
			_InsertLine(i,pb);

			pb += m_rd.ByteWidth;
		}

		if( m_rd.ReadLength != 0 )
			ListViewEx_AdjustWidthAllColumns(m_hWndList,LVSCW_AUTOSIZE);
		else
			SetAppropriateColumnWidth();

		int iLine = 0;
		if( (m_rd.ReadOffset.QuadPart % m_rd.Length) != 0 )
		{
			iLine = (int)(m_rd.ReadOffset.QuadPart - CalcSectorTopOffset(m_rd.ReadOffset,m_rd.Length)) / m_rd.ByteWidth;
		}

		ListView_SetItemState(m_hWndList,iLine,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
		ListView_EnsureVisible(m_hWndList,iLine,FALSE);

		SetRedraw(m_hWndList,TRUE);

		return S_OK;
	}

	BOOL InitSectorBuffer()
	{
		HANDLE Handle;
		Handle = OpenDisk(m_rd.Name,0,GENERIC_READ);

		if( Handle != INVALID_HANDLE_VALUE )
		{
			if( wcsnicmp(m_rd.Name,L"PhysicalDrive",13) == 0 )
			{
				PDISK_GEOMETRY_EX Geometry = NULL;
				ULONG cbGeometry = 0;
				m_rd.Status = GetDiskDriveGeometryEx(Handle,&Geometry,&cbGeometry);

				if( m_rd.Status == 0 )
				{
					m_rd.Length = Geometry->Geometry.BytesPerSector;
					m_rd.SectorLength = m_rd.Length;
					m_rd.ClusterLength = 0;

					m_rd.Size.QuadPart = Geometry->DiskSize.QuadPart;
					m_rd.Buffer = (LPBYTE)_MemAlloc( m_rd.Length );

					ASSERT(m_rd.Buffer != NULL);
					if( m_rd.Buffer == NULL )
					{
						m_rd.Status = ERROR_NOT_ENOUGH_MEMORY;
					}

					VolumeMemFree(Geometry);
				}

				m_rpb.FileAreaOffset.QuadPart = 0;
			}
			else
			{
				// Volume / Drive
				{
					VOLUME_FS_SIZE_INFORMATION *Size;
					NTSTATUS Status;
					Status = GetVolumeFsInformation(Handle,VOLFS_SIZE_INFORMATION,(void**)&Size);
					if( Status == 0 )
					{
						m_rd.Length = (Size->BytesPerSector * Size->SectorsPerAllocationUnit);
						m_rd.ClusterLength = m_rd.Length;
						m_rd.SectorLength = Size->BytesPerSector;

						m_rd.Size.QuadPart = Size->TotalAllocationUnits.QuadPart * m_rd.Length;
						m_rd.Buffer = (LPBYTE)_MemAlloc( m_rd.Length );

						ASSERT(m_rd.Buffer != NULL);
						if( m_rd.Buffer == NULL )
						{
							m_rd.Status = ERROR_NOT_ENOUGH_MEMORY;
						}

						FreeMemory(Size);
					}
					else
					{
						m_rd.Status = RtlNtStatusToDosError(Status); 
					}
				}

				// Get the Retrieval Pointer Base, if we available.
				{
					DWORD dwBytesReturned;
					if( DeviceIoControl(Handle,
							FSCTL_GET_RETRIEVAL_POINTER_BASE,
							NULL,0,
							&m_rpb,sizeof(m_rpb),
							&dwBytesReturned,
							NULL) )
					{
						m_liFileAreaOffsetByte.QuadPart = m_rpb.FileAreaOffset.QuadPart * m_rd.SectorLength;
					}
					else
					{
						m_liFileAreaOffsetByte.QuadPart = 0;
					}
				}
			}

			CloseHandle(Handle);
		}
		else
		{
			// disk/volume/drive open error
			m_rd.Status = GetLastError();
		}
		
		return m_rd.Status == 0 ? TRUE : FALSE;
	}

	LONGLONG CalcSectorTopOffset(LARGE_INTEGER& li,LONG SectorSize)
	{
		return (m_rd.Length * (li.QuadPart / SectorSize));
	}

	BOOL ReadSector()
	{
		HANDLE Handle;

		Handle = OpenDisk(m_rd.Name,0,GENERIC_READ);

		if( Handle != INVALID_HANDLE_VALUE )
		{
			AllowExtendedDASDIo( Handle );

			LARGE_INTEGER li;
			li.QuadPart = CalcSectorTopOffset(m_rd.ReadOffset,m_rd.Length);

			SetFilePointerEx(Handle,li,NULL,FILE_BEGIN);

			DWORD cb = 0;
			if( ReadFile(Handle,m_rd.Buffer,m_rd.Length,&cb,NULL) && cb > 0 )
			{
				m_rd.ReadLength = cb;
				m_rd.Status = ERROR_SUCCESS;
			}
			else
			{
				m_rd.ReadLength = 0;
				m_rd.Status = GetLastError();
			}

			CloseHandle(Handle);
		}
		else
		{
			m_rd.Status = GetLastError();
		}

		return m_rd.Status == 0 ? TRUE : FALSE;
	}

	virtual HRESULT UpdateData(PVOID pSelItem)
	{
		ASSERT(pSelItem != NULL);
		if( pSelItem == NULL )
			return 0;

		SELECT_OFFSET_ITEM *pSelOffset = (SELECT_OFFSET_ITEM*)pSelItem;

		//
		// Set up drive/volume name and read start offset.
		//
		StringCchCopy(m_rd.Name,ARRAYSIZE(m_rd.Name),pSelOffset->hdr.pszVolume);
		m_rd.ReadOffset.QuadPart = pSelOffset->liStartOffset.QuadPart;
		m_rd.BaseOffset.QuadPart = pSelOffset->liStartOffset.QuadPart;
		_SafeMemFree(m_rd.Buffer);

		//
		// Allocation sector buffer and initialize sector size, extent size.
		//
		if( !InitSectorBuffer() )
		{
			SetErrorState();
			return HRESULT_FROM_WIN32(m_rd.Status);
		}

		//
		// Update toolbar buttons
		//
		UpdateToolbarButtons();

		//
		// Reads sector from the currently set read position and fill the listview.
		//
		return FillItems();
	}

	void doReload()
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.Length != 0);

		// reload current location sector
		FillItems();
	}

	void gotoNext()
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.Length != 0);

		if( m_rd.ReadOffset.QuadPart < m_rd.Size.QuadPart )
		{
			m_rd.ReadOffset.QuadPart += m_rd.Length;
		}

		UpdateToolbarButtons();

		FillItems();

		if( GetKeyState(VK_SHIFT) < 0 )
		{
			m_bAuto = true;
		}
	
		if( m_bAuto )	
		{
			PBYTE pb = m_rd.Buffer;
			ULONG i;

			for(i = 0; i < m_rd.Length; i++)
			{
				if( *pb != 0 )
				{
					m_bAuto = false;
					break;
				}
			}
			if( i == m_rd.Length )
			{
				RedrawWindow(m_hWndList,0,0,RDW_INVALIDATE|RDW_UPDATENOW);
				DoMessage();
				PostMessage(m_hWnd,WM_COMMAND,ID_AUTO_NEXT,0);
			}
		}
	}

	void gotoBack()
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.Length != 0);

		if( m_rd.ReadOffset.QuadPart > 0 )
		{
			m_rd.ReadOffset.QuadPart -= m_rd.Length;
		}

		UpdateToolbarButtons();

		FillItems();

		if( GetKeyState(VK_SHIFT) < 0 )
		{
			m_bAuto = true;
		}
	
		if( m_bAuto )	
		{
			PBYTE pb = m_rd.Buffer;
			ULONG i;

			for(i = 0; i < m_rd.Length; i++)
			{
				if( *pb != 0 )
				{
					m_bAuto = false;
					break;
				}
			}
			if( i == m_rd.Length )
			{
				RedrawWindow(m_hWndList,0,0,RDW_INVALIDATE|RDW_UPDATENOW);
				DoMessage();
				PostMessage(m_hWnd,WM_COMMAND,ID_AUTO_BACK,0);
			}
		}
	}

	void gotoLast()
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.Length != 0);

		m_rd.ReadOffset.QuadPart = m_rd.Size.QuadPart - m_rd.Length;

		UpdateToolbarButtons();

		FillItems();
	}

	void gotoFirst()
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.Length != 0);

		m_rd.ReadOffset.QuadPart = 0;

		UpdateToolbarButtons();

		FillItems();
	}

	void gotoHome()
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.Length != 0);

		m_rd.ReadOffset.QuadPart = m_rd.BaseOffset.QuadPart;

		UpdateToolbarButtons();

		FillItems();
	}

	void gotoSector()
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.Length != 0);

		static DISK_VOLUME_SECTOR_CLUSTER_LOCATION loc = {{0,0},0,DVL_HEX|DVL_LOC_SECTOR_NUMBER};

		loc.SectorClusterSize = m_rd.Length;

		if( loc.Flags & DVL_LOC_SECTOR_NUMBER )
		{
			// Specifies Sector Number / Cluster Number
			if( m_rd.ClusterLength != 0 )
			{
				// Cluster Number
				if( (m_rd.ReadOffset.QuadPart - m_liFileAreaOffsetByte.QuadPart) >= 0 )
					loc.Location.QuadPart = (m_rd.ReadOffset.QuadPart - m_liFileAreaOffsetByte.QuadPart) / m_rd.Length;
				else
					loc.Location.QuadPart = 0;
			}
			else
			{
				// Sector Number
				loc.Location.QuadPart = m_rd.ReadOffset.QuadPart / m_rd.Length;
			}
		}
		else
		{
			// Specifies byte offset
			loc.Location.QuadPart = m_rd.ReadOffset.QuadPart;
		}

		if( m_rd.ClusterLength == 0 )
			loc.Flags |= DVL_LOC_PHYSICAL_DISK;
		else
			loc.Flags &= ~DVL_LOC_PHYSICAL_DISK;

		if( GotoDialog(m_hWnd,&loc) == S_OK )
		{
			if( loc.Flags & DVL_LOC_SECTOR_NUMBER )
			{
				m_rd.ReadOffset.QuadPart = loc.Location.QuadPart * m_rd.Length + m_liFileAreaOffsetByte.QuadPart;
			}
			else
			{
				m_rd.ReadOffset.QuadPart = loc.Location.QuadPart;
			}

			UpdateToolbarButtons();

			FillItems();
		}
	}

	//
	// Command Handler
	//
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

		// Toolbar / Hex dump local command
		if( !IsEnableCommand(CmdId) )
			return S_FALSE;

		switch( CmdId )
		{
			case ID_BACK:
				OnCmdBack();
				break;
			case ID_NEXT:
				OnCmdNext();
				break;
			case ID_AUTO_BACK:
				OnCmdAutoBack();
				break;
			case ID_AUTO_NEXT:
				OnCmdAutoNext();
				break;
			case ID_FIRST:
				OnCmdFirst();
				break;
			case ID_LAST:
				OnCmdLast();
				break;
			case ID_HOME:
				OnCmdHome();
				break;
			case ID_GOTO:
				OnCmdGotoSector();
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
			case ID_EDIT_FIND:
			case ID_EDIT_FIND_NEXT:
			case ID_EDIT_FIND_PREVIOUS:
				*State = UPDUI_ENABLED;
				return S_OK;
			case ID_GOTO:
				*State = ((m_rd.Size.QuadPart > 0) && (m_rd.Buffer != NULL)) ?  UPDUI_ENABLED : UPDUI_DISABLED;
				return S_OK;
			case ID_NEXT:
			case ID_BACK:
			case ID_AUTO_NEXT:
			case ID_AUTO_BACK:
				*State = ((m_rd.Size.QuadPart > 0) && (m_rd.Buffer != NULL)) ?  UPDUI_ENABLED : UPDUI_DISABLED;
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
		doReload();
	}

	BOOL IsEnableCommand(UINT id)
	{
		return ( (GetToolbarButtonState(id) & TBSTATE_ENABLED) == TBSTATE_ENABLED );
	}

	void OnCmdNext()
	{
		m_bAuto = false;
		gotoNext();
	}

	void OnCmdBack()
	{
		m_bAuto = false;
		gotoBack();
	}

	void OnCmdAutoNext()
	{
		gotoNext();
	}

	void OnCmdAutoBack()
	{
		gotoBack();
	}

	void OnCmdFirst()
	{
		gotoFirst();
	}

	void OnCmdLast()
	{
		gotoLast();
	}

	void OnCmdHome()
	{
		gotoHome();
	}

	void OnCmdGotoSector()
	{
		gotoSector();
	}

private:
	void SetAppropriateColumnWidth()
	{
		SetRedraw(m_hWndList,FALSE);
		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = 0;
		lvi.iImage  = I_IMAGENONE;
		lvi.pszText = (PWSTR)L"00000000`00000000";
		ListView_InsertItem(m_hWndList,&lvi);
		ListView_SetItemText(m_hWndList,0,1,L"00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
		ListView_SetItemText(m_hWndList,0,2,L"0123456789ABCDEF");

		ListViewEx_AdjustWidthAllColumns(m_hWndList,LVSCW_AUTOSIZE);

		ListView_DeleteItem(m_hWndList,0);
		SetRedraw(m_hWndList,TRUE);
	}

	void SetToolbarButtonState(UINT uCmdId,BOOL Enable)
	{
		SendMessage(m_hWndToolBar,TB_SETSTATE,(WPARAM)uCmdId,(LPARAM)(Enable ? TBSTATE_ENABLED : 0));
	}

	int GetToolbarButtonState(UINT uCmdId)
	{
		return (int)SendMessage(m_hWndToolBar,TB_GETSTATE,(WPARAM)uCmdId,0);
	}

	void UpdateToolbarButtons()
	{
		SetToolbarButtonState(ID_BACK,(m_rd.ReadOffset.QuadPart > 0));
		SetToolbarButtonState(ID_NEXT,(m_rd.ReadOffset.QuadPart < (m_rd.Size.QuadPart - m_rd.Length)));

		SetToolbarButtonState(ID_HOME,(m_rd.Size.QuadPart > 0) && (m_rd.BaseOffset.QuadPart != m_rd.ReadOffset.QuadPart));

		SetToolbarButtonState(ID_FIRST,(m_rd.Size.QuadPart > 0) && (m_rd.ReadOffset.QuadPart > 0));
		SetToolbarButtonState(ID_LAST,(m_rd.Size.QuadPart > 0) && (m_rd.ReadOffset.QuadPart < (m_rd.Size.QuadPart - m_rd.Length)));
		SetToolbarButtonState(ID_GOTO,(m_rd.Size.QuadPart > 0) && (m_rd.Buffer != NULL) );
	}

	void SetErrorState()
	{
		PWSTR pMessage;
		if( WinGetErrorMessage(m_rd.Status,&pMessage) > 0 )
		{
			_SafeMemFree(m_pszErrorMessage);
			m_pszErrorMessage = _MemAllocString(pMessage);
			WinFreeErrorMessage(pMessage);
		}
		SetAppropriateColumnWidth();
		SetToolbarButtonState(ID_BACK,FALSE);
		SetToolbarButtonState(ID_NEXT,FALSE);
		SetToolbarButtonState(ID_FIRST,FALSE);
		SetToolbarButtonState(ID_LAST,FALSE);
		SetToolbarButtonState(ID_HOME,FALSE);
		SetToolbarButtonState(ID_GOTO, m_rd.Length ? TRUE : FALSE);
	}
};
