//*****************************************************************************
//
//  bindump.h
//
//  PURPOSE: Simple Paged Binary Dump
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-01-27 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#pragma once

#include "toolbar.h"

#define BDF_BYTEWIDTH_8          0x01 
#define BDF_BYTEWIDTH_16         0x02
#define BDF_BYTEWIDTH_32         0x04

#define BDF_ADDR_QWORD           0x10
#define BDF_ADDR_DWORD           0x20
#define BDF_ADDR_WORD_OFFSET     0x40

#define BDF_ADDR_WIDTH_MASK      0xf0

#define BDF_OFFSETSTR_WORD      0x100
#define BDF_OFFSETSTR_DWORD     0x200
#define BDF_OFFSETSTR_QWORD     0x400
#define BDF_OFFSETSTR_TITLE     0x800
#define BDF_OFFSETSTR_MASK      0xF00

#define BDF_ADDRCENTER         0x1000

struct CBinDumpItem
{
public:
	LPBYTE BufferPos;
	LONGLONG Offset;

	CBinDumpItem()
	{
		BufferPos = NULL;
		Offset = 0;
	}
};

typedef struct _NMBINDUMP
{
	NMHDR hdr;
	PBYTE Buffer;
	DWORD BufferLength;
	DWORD ClusterSize;
	LARGE_INTEGER Offset;
	LARGE_INTEGER ReadOffset;
	LARGE_INTEGER TargetOffset;
	LARGE_INTEGER Size;
	LARGE_INTEGER CursorPos;
	DWORD CurPage;
	DWORD MaxPage;
} NMBINDUMP,*PNMBINDUMP;

#define BDN_FIRST              (0U-6600U)
#define BDN_LAST               (0U-6610U)
#define BDN_UPDATE             (BDN_FIRST-1)
#define BDN_CURSORMOVED        (BDN_FIRST-2)
#define BDN_SETERROR           (BDN_FIRST-3)

#define LVSF_CURSOR_CHANGED    (0x01)
#define LVSF_MOUSE_LBUTTONDOWN (0x10)

class CQuickBinDump : public CBaseWindow
{
	HWND  m_hWndList;
	HFONT m_hFont;
	PWSTR m_pszFilePath;
	PWSTR m_pszErrorMessage;
	BOOL  m_bAuto;
	PWSTR m_pszAddrTitle;
	DWORD m_dwFlags;

	typedef struct FILE_READ_BLOCK_INFO
	{
		ULONG Status;
		HANDLE Handle;
		LARGE_INTEGER ReadOffset;
		LARGE_INTEGER TargetOffset;
		LARGE_INTEGER Size;
		LARGE_INTEGER CursorPos;

		int HexByteWidth;
		BYTE *Buffer;
		ULONG ReadLength;
		ULONG BufferLength;
		ULONG ClusterSize;
		ULONG LineCount;

		FILE_READ_BLOCK_INFO()
		{
			memset(this,0,sizeof(FILE_READ_BLOCK_INFO));
		}

		~FILE_READ_BLOCK_INFO()
		{
		}
	} FILE_READ_BLOCK_INFO;

	FILE_READ_BLOCK_INFO m_rd;

	class _make_dummy_text
	{
		WCHAR sz[ 3 * 32 + 1 ];
	public:
		_make_dummy_text( int width, bool ascii = false )
		{
			sz[0] = 0;
			for(int i = 0; i < (width-1); i++)
			{
				StringCchCat(sz,ARRAYSIZE(sz), ascii ? L"0" : L"00 ");
			}
			StringCchCat(sz,ARRAYSIZE(sz), ascii ? L"0" : L"00");
		}
		operator LPWSTR () const { return (LPWSTR)sz; }
		operator LPCWSTR () const { return sz; }
	};
	
	class _make_header_text
	{
		WCHAR szText[ 3 * 32 + 1 ];
	public:
		_make_header_text( int width, bool ascii = false )
		{
			WCHAR sz[4];
			szText[0] = 0;
	
			for(int i = 0; i < width; i++)
			{
				StringCchPrintf(sz,ARRAYSIZE(sz),
					ascii ? L"%X" : ((i == (width-1)) ? L"+%X" : L"+%X "),
					(i % width));
	
				StringCchCat(szText,ARRAYSIZE(szText),sz);
			}
		}
		operator LPWSTR () const { return (LPWSTR)szText; }
		operator LPCWSTR () const {	return szText; }
	};

public:
	HWND GetListView() const { return m_hWndList; }

	CQuickBinDump() : CBaseWindow(FALSE)
	{
		m_hWndList = NULL;
		m_hFont = NULL;
		m_pszFilePath = NULL;
		m_pszErrorMessage = NULL;
		m_bAuto = FALSE;
		m_bAutoDelete = FALSE;
		ZeroMemory(&m_rd,sizeof(m_rd));
		m_rd.HexByteWidth = 0;
		m_pszAddrTitle = L"Offset";
		m_dwFlags = 0;
	}

	~CQuickBinDump()
	{
	}

	LONGLONG GetSize() const
	{
		return m_rd.Size.QuadPart;
	}

	LONGLONG GetReadOffset() const
	{
		return m_rd.ReadOffset.QuadPart;
	}

	DWORD GetClusterSize() const
	{
		return m_rd.ClusterSize;
	}

	DWORD GetMaxPageCount() const
	{
		if( m_rd.ClusterSize == 0 )
			return 0;
		DWORD dwPage = (DWORD)(m_rd.Size.QuadPart / m_rd.ClusterSize);
		if( m_rd.Size.QuadPart % m_rd.ClusterSize )
			dwPage++;
		return dwPage;
	}

	DWORD GetCurrentPage() const
	{
		if( m_rd.ClusterSize == 0 )
			return 0;
		DWORD dwPage = (DWORD)(m_rd.ReadOffset.QuadPart / m_rd.ClusterSize);
		return dwPage+1;
	}

	int GetHexDumpWidth()
	{
		int cx = 0;
		for(int i = 0; i < 3; i++)
			cx += ListView_GetColumnWidth(m_hWndList,i);
		return cx;
	}

	int GetCharWidth() const
	{
		HDC hdc = GetDC(m_hWnd);
		HFONT fontPrev = (HFONT)SelectObject(hdc,m_hFont);
		TEXTMETRIC tm;
		GetTextMetrics(hdc,&tm);
		SelectObject(hdc,fontPrev);
		ReleaseDC(m_hWnd,hdc);
		return tm.tmAveCharWidth;
	}

	ULONG GetListViewState()
	{
		return (ULONG)GetWindowLongPtr(m_hWndList,GWLP_USERDATA);
	}

    void SetListViewState(LONG State)
    {
		SetWindowLongPtr(m_hWndList,GWLP_USERDATA,State);
    }

	HRESULT Create(HWND hWnd,HWND *phwndBD,DWORD dwFlags,UINT id=0,
				DWORD dwStyle=0,DWORD dwExStyle=0,PCWSTR pszTitle=NULL)
	{
		if( dwStyle == 0 )
			dwStyle = WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
		if( dwExStyle == 0 )
			dwExStyle = WS_EX_CONTROLPARENT;

		m_dwFlags = dwFlags;

		if( m_dwFlags & BDF_BYTEWIDTH_8 )
			m_rd.HexByteWidth = 8;
		else if( m_dwFlags & BDF_BYTEWIDTH_16 )
			m_rd.HexByteWidth = 16;
		else if( m_dwFlags & BDF_BYTEWIDTH_32 )
			m_rd.HexByteWidth = 32;
		else
			m_rd.HexByteWidth = 16;

		HWND hwnd = CBaseWindow::Create(hWnd,id,pszTitle,dwStyle,dwExStyle);
		if( hwnd == NULL )
			return E_FAIL;

		if( phwndBD )
			*phwndBD = hwnd;

		return S_OK;
	}

	HRESULT OnInitPage(PVOID,DWORD,PVOID)
	{
		m_hWndList = CreateWindow(WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS,
                              0,0,0,0,
                              m_hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)m_hFont,0);

		_EnableVisualThemeStyle(m_hWndList);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

#if _ENABLE_DARK_MODE_TEST
		if( _IsDarkModeEnabled() )
			InitDarkModeListView(m_hWndList);
#endif
		InitColumns(m_hWndList);

		SetWindowSubclass(m_hWndList,&ListView_HookProc,1,0);

		SetListViewState(0);

		return S_OK;
	}

	HRESULT Clear()
	{
		ListView_DeleteAllItems(m_hWndList);
		Close();
		return S_OK;
	}

	HRESULT Open(PCWSTR pszFileOrStreamName,DWORD dwFlags=0)
	{
		_SafeMemFree(m_pszFilePath);
		m_pszFilePath = _MemAllocString( pszFileOrStreamName ); 

		m_rd.ReadOffset.QuadPart = 0x0;

		LARGE_INTEGER liStartOffset = {0};

		if( !InitFileBuffer( liStartOffset ) )
		{
			ListView_DeleteAllItems(m_hWndList);
			SetErrorState();
			return HRESULT_FROM_WIN32(m_rd.Status);
		}

		return FillItems();
	}

	HRESULT Close()
	{
		_SafeMemFree(m_pszFilePath);
		_SafeMemFree(m_rd.Buffer);

		if( m_rd.Handle )
		{
			CloseHandle(m_rd.Handle);
			m_rd.Handle = NULL;
		}

		m_rd.BufferLength = 0;
		m_rd.ReadLength = 0;
		m_rd.ReadOffset.QuadPart = 0;
		m_rd.Size.QuadPart = 0;
		m_rd.Status = 0;
 
		return S_OK;
	}

	HRESULT Reload()
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.BufferLength != 0);

		// reload current location sector
		return FillItems();
	}

	HRESULT GotoOffset(LONGLONG offset=0)
	{
		if( offset >= m_rd.Size.QuadPart )
		{
			m_rd.TargetOffset.QuadPart = m_rd.Size.QuadPart;
		}
		else
		{
			m_rd.TargetOffset.QuadPart = offset;
		}

		m_rd.CursorPos.QuadPart = m_rd.TargetOffset.QuadPart;

		return FillItems();
	}

private:
	void UpdateLayout(int cx,int cy)
	{
		int cyToolBar = 0;

		HDWP hdwp;
		hdwp = BeginDeferWindowPos( 1 );

		if( m_hWndList )
		{
			DeferWindowPos(hdwp,m_hWndList,NULL,0,cyToolBar,cx,cy-cyToolBar,SWP_NOMOVE|SWP_NOZORDER|SWP_DRAWFRAME|SWP_NOCOPYBITS);
		}

		EndDeferWindowPos(hdwp);

		if( m_pszErrorMessage )
			ListViewEx_ResetEmptyText(m_hWndList); // Necessary to display in the correct position when resize.
	}

	void InitColumns(HWND hWndList)
	{
		LVCOLUMN lvc = {0};
		int col = 0;

		lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;

		lvc.fmt     = LVCFMT_LEFT;
		lvc.cx      = 0;
		lvc.pszText = L"";
		ListView_InsertColumn(hWndList,col++,&lvc); // Insert dummy

		lvc.fmt     = (m_dwFlags & BDF_ADDRCENTER) ? LVCFMT_CENTER : LVCFMT_LEFT;
		lvc.cx      = 0;
		lvc.pszText = L"Offset";
		ListView_InsertColumn(hWndList,col++,&lvc);

		lvc.fmt     = LVCFMT_LEFT;
		lvc.cx      = 0;
		lvc.pszText = _make_header_text( m_rd.HexByteWidth );
		ListView_InsertColumn(hWndList,col++,&lvc);

		lvc.fmt     = LVCFMT_LEFT;
		lvc.cx      = 0;
		lvc.pszText = _make_header_text( m_rd.HexByteWidth, true );
		ListView_InsertColumn(hWndList,col++,&lvc);

		ListView_DeleteColumn(hWndList,0);

		SetAppropriateColumnWidth();

		SetFixedColumn(0);
		SetFixedColumn(1);
		SetFixedColumn(2);
	}

	int InsertLine(int iItem,PBYTE pb)
	{
		CBinDumpItem *pItem = new CBinDumpItem;
		pItem->BufferPos = pb;
		pItem->Offset    = m_rd.ReadOffset.QuadPart + (iItem * m_rd.HexByteWidth);

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
		int iLine = 0;

		SetRedraw(m_hWndList,FALSE);

		ListView_DeleteAllItems(m_hWndList);

		if( m_rd.TargetOffset.QuadPart != 0 )
		{
			if( m_rd.TargetOffset.QuadPart >= m_rd.Size.QuadPart )
				m_rd.TargetOffset.QuadPart = m_rd.Size.QuadPart;

			m_rd.ReadOffset.QuadPart = (m_rd.TargetOffset.QuadPart / m_rd.ClusterSize) * m_rd.ClusterSize;

			iLine = (int)(m_rd.TargetOffset.QuadPart - m_rd.ReadOffset.QuadPart) / m_rd.HexByteWidth;

			m_rd.TargetOffset.QuadPart = 0;
		}

		ReadFileBlock();

		if( m_rd.Status != 0 )
		{
			SetErrorState();
			SetRedraw(m_hWndList,TRUE);
			return HRESULT_FROM_WIN32(m_rd.Status);
		}

		ULONG i,loop;
		LPBYTE pb = m_rd.Buffer;

		loop = (m_rd.ReadLength / m_rd.HexByteWidth);
		if( (m_rd.ReadLength % m_rd.HexByteWidth) != 0 )
			loop++;

		for(i = 0; i < loop; i++)
		{
			InsertLine(i,pb);

			pb += m_rd.HexByteWidth;
		}

		if( m_rd.ReadLength != 0 )
			ListViewEx_AdjustWidthAllColumns(m_hWndList,LVSCW_AUTOSIZE);
		else
			SetAppropriateColumnWidth();

		m_rd.LineCount = ListView_GetItemCount(m_hWndList);

		ListView_SetItemState(m_hWndList,iLine,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
		ListView_EnsureVisible(m_hWndList,iLine,FALSE);

		SetRedraw(m_hWndList,TRUE);

		NotifyUpdate();

		return S_OK;
	}

	BOOL InitFileBuffer(LARGE_INTEGER& li)
	{
		HANDLE hFile;
		m_rd.Status = OpenFileEx_W(&hFile,m_pszFilePath,GENERIC_READ|SYNCHRONIZE,FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_NON_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);

		if( m_rd.Status != STATUS_SUCCESS )
		{
			m_rd.Status = NtStatusToDosError(m_rd.Status);
			return FALSE;
		}

		m_rd.Handle = hFile;

		DWORD dwClusterSize;
		NT_VOLUME_SIZE_INFORMATION Size;
		if( NtDosGetVolumeSizeInformation(hFile,&Size,sizeof(Size)) == STATUS_SUCCESS )
			dwClusterSize = Size.ClusterSize;
		else
			dwClusterSize = 4096;

		m_rd.ClusterSize  = dwClusterSize;
		m_rd.BufferLength = dwClusterSize;
		m_rd.ReadOffset.QuadPart = 0;

		m_rd.Buffer = (LPBYTE)_MemAlloc( m_rd.BufferLength );
		if( m_rd.Buffer == NULL )
		{
			m_rd.Status = ERROR_NOT_ENOUGH_MEMORY;
			CloseHandle(m_rd.Handle);
			m_rd.Handle = NULL;
			return FALSE;
		}

		if( !GetFileSizeEx(hFile,&m_rd.Size) )
		{
			m_rd.Status = GetLastError();
			CloseHandle(m_rd.Handle);
			m_rd.Handle = NULL;
			return FALSE;
		}

		return TRUE;
	}

	BOOL ReadFileBlock()
	{
		SetFilePointerEx(m_rd.Handle,m_rd.ReadOffset,NULL,FILE_BEGIN);

		DWORD cb;
		ReadFile(m_rd.Handle,m_rd.Buffer,m_rd.BufferLength,&cb,NULL);

		m_rd.ReadLength = cb;

		return m_rd.Status == 0 ? TRUE : FALSE;
	}

	void SetAppropriateColumnWidth()
	{
		SetRedraw(m_hWndList,FALSE);

		LVITEM lvi = {0};
		lvi.mask    = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
		lvi.iItem   = 0;
		lvi.iImage  = I_IMAGENONE;
		switch( m_dwFlags & BDF_OFFSETSTR_MASK )
		{
			case BDF_OFFSETSTR_WORD:
				lvi.pszText = (PWSTR)L"0000 ";
				break;
			case BDF_OFFSETSTR_DWORD:
				lvi.pszText = (PWSTR)L"00000000 ";
				break;
			case BDF_OFFSETSTR_QWORD:
				lvi.pszText = (PWSTR)L"00000000`00000000 ";
				break;
			default:
				lvi.pszText = (PWSTR)m_pszAddrTitle;
		}
		ListView_InsertItem(m_hWndList,&lvi);

		ListView_SetItemText(m_hWndList,0,1,_make_dummy_text( m_rd.HexByteWidth ));
		ListView_SetItemText(m_hWndList,0,2,_make_dummy_text( m_rd.HexByteWidth, true ));

        ListView_SetColumnWidth(m_hWndList,0,LVSCW_AUTOSIZE_USEHEADER);
        ListView_SetColumnWidth(m_hWndList,1,LVSCW_AUTOSIZE);
        ListView_SetColumnWidth(m_hWndList,2,LVSCW_AUTOSIZE);

		ListView_DeleteItem(m_hWndList,0);

		SetRedraw(m_hWndList,TRUE);
	}

	void SetFixedColumn(int iColumn)
	{
		HWND hwndHD = ListView_GetHeader(m_hWndList);
		HDITEM hi = {};
		hi.mask = HDI_FORMAT;
		Header_GetItem(hwndHD,iColumn,&hi);
		hi.fmt |= HDF_FIXEDWIDTH;
		Header_SetItem(hwndHD,iColumn,&hi);
	}

	void NotifyUpdate()
	{
		NMBINDUMP nmbd = {};
		nmbd.hdr.code     = BDN_UPDATE;
		nmbd.hdr.hwndFrom = m_hWnd;
		nmbd.hdr.idFrom   = GetWindowLong(m_hWnd,GWL_ID);
		nmbd.Buffer       = m_rd.Buffer;
		nmbd.BufferLength = m_rd.BufferLength;
		nmbd.ClusterSize  = m_rd.ClusterSize;
		nmbd.ReadOffset   = m_rd.ReadOffset;
		nmbd.TargetOffset = m_rd.TargetOffset;
		nmbd.Size         = m_rd.Size;
		nmbd.CursorPos    = m_rd.CursorPos;
		nmbd.CurPage      = GetCurrentPage();
		nmbd.MaxPage      = GetMaxPageCount();

		CBinDumpItem *pItem = (CBinDumpItem *)ListViewEx_GetCurItemData(m_hWndList);

		if( pItem )
			nmbd.Offset.QuadPart = pItem->Offset;

		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)&nmbd);
	}

	VOID NotifyCursorChanged( CBinDumpItem *pItem )
	{
		NMBINDUMP nmbd = {};
		nmbd.hdr.code     = BDN_CURSORMOVED;
		nmbd.hdr.hwndFrom = m_hWnd;
		nmbd.hdr.idFrom   = GetWindowLong(m_hWnd,GWL_ID);
		nmbd.Buffer       = m_rd.Buffer;
		nmbd.BufferLength = m_rd.BufferLength;
		nmbd.ClusterSize  = m_rd.ClusterSize;
		nmbd.ReadOffset   = m_rd.ReadOffset;
		nmbd.TargetOffset = m_rd.TargetOffset;
		nmbd.CursorPos    = m_rd.CursorPos;
		nmbd.Size         = m_rd.Size;
		nmbd.CurPage      = GetCurrentPage();
		nmbd.MaxPage      = GetMaxPageCount();

		nmbd.Offset.QuadPart = pItem->Offset;

		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)&nmbd);
	}

	void SetErrorState()
	{
		PWSTR pMessage;
		_SafeMemFree(m_pszErrorMessage);
		if( _GetSystemErrorMessage(m_rd.Status,&pMessage) > 0)
		{
			m_pszErrorMessage = _MemAllocString(pMessage);
			_FreeSystemErrorMessage(pMessage);
		}
		else
		{
			WCHAR sz[64];
			StringCchPrintf(sz,ARRAYSIZE(sz),L"Error : 0x%08X",m_rd.Status);
			m_pszErrorMessage = _MemAllocString(sz);
		}

		ListViewEx_ResetEmptyText(m_hWndList);

		NMBINDUMP nmbd = {};
		nmbd.hdr.code     = BDN_SETERROR;
		nmbd.hdr.hwndFrom = m_hWnd;
		nmbd.hdr.idFrom   = GetWindowLong(m_hWnd,GWL_ID);
		nmbd.Buffer       = m_rd.Buffer;
		nmbd.BufferLength = m_rd.BufferLength;
		nmbd.ClusterSize  = m_rd.ClusterSize;
		nmbd.ReadOffset   = m_rd.ReadOffset;
		nmbd.TargetOffset = m_rd.TargetOffset;
		nmbd.CursorPos    = m_rd.CursorPos;
		nmbd.Size         = m_rd.Size;
		nmbd.CurPage      = 0;
		nmbd.MaxPage      = 0;

		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)&nmbd);
	}

	void UpdateEmptyString(BOOL bUpdateOnly=false)
	{
		_SafeMemFree( m_pszErrorMessage );
		m_pszErrorMessage = _MemAllocString( L"(No Data)" );
		ListViewEx_ResetEmptyText(m_hWndList);
	}

	//------------------------------------------------------------------------
	//
	//  Message Hook Procedure for ListView
	//
	//------------------------------------------------------------------------
	static LRESULT CALLBACK
	ListView_HookProc(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
		)
	{
		switch( uMsg )
		{
			case WM_LBUTTONDOWN:
			{
				LONG_PTR State = GetWindowLongPtr(hWnd,GWLP_USERDATA);
				State |= LVSF_MOUSE_LBUTTONDOWN;
				SetWindowLongPtr(hWnd,GWLP_USERDATA,State);
				break;
			}
		}
	
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	//------------------------------------------------------------------------
	//
	//  Message Handler
	//
	//------------------------------------------------------------------------
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
				return OnCommand(LOWORD(wParam));
			case WM_ERASEBKGND:
				return OnEraseBkgnd(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
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

		_SafeMemFree(m_pszErrorMessage);
		_SafeMemFree(m_pszFilePath);
		_SafeMemFree(m_rd.Buffer);

		RemoveWindowSubclass(m_hWndList,&ListView_HookProc,1);

		return 0;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		CBinDumpItem *pItem = (CBinDumpItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		if( pItem )
		{
			HMENU hMenu = CreatePopupMenu();
			AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");

			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,(HWND)wParam,hMenu,pt,0);

			DestroyMenu(hMenu);
		}

		return 0;
	}

	LRESULT OnEraseBkgnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HDC hdc = (HDC)wParam;
		RECT rc;
		GetClientRect(m_hWnd,&rc);

		HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
		FillRect(hdc,&rc,hbr);
		DeleteObject(hbr);

		return (LRESULT)TRUE;
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
			case LVN_ITEMCHANGING:
				return OnItemChanging(pnmhdr);
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
			case LVN_KEYDOWN:
				return OnKeyDown(pnmhdr);
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

	void drawCursor(HWND hwndLV,HDC hdc,int iItem,RECT *pRect,LONGLONG offset,PBYTE pByte)
	{
		HFONT hPrevFont = (HFONT)SelectObject(hdc,m_hFont);

		//
		// Get text metric
		//
		TEXTMETRIC tm;
		GetTextMetrics(hdc,&tm);

		//
		// Get byte column in the current line
		//
		int column = (int)(m_rd.CursorPos.QuadPart - offset);

		ASSERT( column >= 0 );

		RECT rcCursor;
		RECT rcSubItem;
		RECT rcText;

		//
		// Hex dump row
		//
		{
			ListView_GetSubItemRect(hwndLV,iItem,1,LVIR_BOUNDS,&rcSubItem);

			int x = ((tm.tmAveCharWidth * 3) * column);

			rcCursor = rcSubItem;
			rcCursor.left  = rcCursor.left + 6 + x;
			rcCursor.right = rcCursor.left + (tm.tmAveCharWidth * 2);
			rcText = rcCursor;
			rcCursor.top++;
			rcCursor.bottom--;

			HBRUSH hbrCursor;
			hbrCursor = GetSysColorBrush(
					(GetFocus() == m_hWndList) ? COLOR_HIGHLIGHT : COLOR_3DDKSHADOW);
			FillRect(hdc,&rcCursor,hbrCursor);

			SetBkMode(hdc,TRANSPARENT);
			SetTextColor(hdc,GetSysColor(COLOR_HIGHLIGHTTEXT));
					WCHAR sz[4];
					PBYTE pb = pByte;
					StringCchPrintf(sz,4,L"%02X",pb[column]);
			DrawText(hdc,sz,-1,&rcText,DT_SINGLELINE|DT_LEFT|DT_VCENTER);
		}

		//
		// ASCII Character row
		//
		{
			ListView_GetSubItemRect(hwndLV,iItem,2,LVIR_BOUNDS,&rcSubItem);

			int x = (tm.tmAveCharWidth * column);

			rcCursor = rcSubItem;
			rcCursor.left  = rcCursor.left + x + 6;
			rcCursor.right = rcCursor.left + (tm.tmAveCharWidth * 1);
			rcText = rcCursor;
			rcCursor.top++;
			rcCursor.bottom--;

			HBRUSH hbrCursor;
			hbrCursor = GetSysColorBrush(
					(GetFocus() == m_hWndList) ? COLOR_HIGHLIGHT : COLOR_3DDKSHADOW);
			FillRect(hdc,&rcCursor,hbrCursor);

			SetBkMode(hdc,TRANSPARENT);
			SetTextColor(hdc,GetSysColor(COLOR_HIGHLIGHTTEXT));

			WCHAR wsz[4];
			CHAR sz[4];
			PBYTE pb = pByte;
			StringCchPrintfA(sz,4,"%c",isprint(pb[column]) ? pb[column] : '.' );

			MultiByteToWideChar(CP_ACP,0,sz,-1,wsz,ARRAYSIZE(sz));

			DrawText(hdc,wsz,-1,&rcText,DT_SINGLELINE|DT_LEFT|DT_VCENTER);
		}

		SelectObject(hdc,hPrevFont);
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
			//UINT State = ListView_GetItemState(m_hWndList,(int)pnmlvcd->nmcd.dwItemSpec,LVIS_FOCUSED);
			//if( State & LVIS_FOCUSED )
			//{
			//	DrawCursorLine();
			//	return CDRF_SKIPDEFAULT;
			//}
			SelectObject(pnmlvcd->nmcd.hdc,m_hFont);
			return CDRF_NEWFONT|CDRF_NOTIFYPOSTPAINT;
		}

		if( pnmlvcd->nmcd.dwDrawStage == CDDS_ITEMPOSTPAINT )
		{
			UINT State = ListView_GetItemState(m_hWndList,(int)pnmlvcd->nmcd.dwItemSpec,LVIS_FOCUSED);
			if( State & LVIS_FOCUSED )
			{
       			CBinDumpItem *pItem = (CBinDumpItem *)pnmlvcd->nmcd.lItemlParam;
                if( (pItem->Offset <= m_rd.CursorPos.QuadPart) && (m_rd.CursorPos.QuadPart < (pItem->Offset + m_rd.HexByteWidth)) )
                {
                	drawCursor(m_hWndList,
                            pnmlvcd->nmcd.hdc,
                            (int)pnmlvcd->nmcd.dwItemSpec,
                            &pnmlvcd->nmcd.rc,
                            pItem->Offset,
                            pItem->BufferPos
                            );
                }
				return CDRF_SKIPDEFAULT;
			}
		}

		return CDRF_DODEFAULT;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;
		if( pnmlv->lParam )
		{
			CBinDumpItem *pItem = (CBinDumpItem *)pnmlv->lParam;
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
		CBinDumpItem *pItem = (CBinDumpItem *)pdi->item.lParam;

		int id = (int)pdi->item.iSubItem;

		if( pdi->item.mask & LVIF_TEXT )
		{
			if( 0 == id )
			{
				LARGE_INTEGER li;
				switch( m_dwFlags & BDF_ADDR_WIDTH_MASK )
				{
					case BDF_ADDR_QWORD:
						li.QuadPart = (LONGLONG)pItem->Offset;
						StringCchPrintf(pdi->item.pszText,pdi->item.cchTextMax,L"%08X`%08X",li.HighPart,li.LowPart);
						break;
					case BDF_ADDR_DWORD:
						li.QuadPart = (LONGLONG)pItem->Offset;
						StringCchPrintf(pdi->item.pszText,pdi->item.cchTextMax,L"%08X",li.LowPart);
						break;
					case BDF_ADDR_WORD_OFFSET:
						StringCchPrintf(pdi->item.pszText,pdi->item.cchTextMax,L"+%04X",pdi->item.iItem * m_rd.HexByteWidth);
						break;
					default:
						pdi->item.pszText = L"";
						break;
				}
			}
			else if( 1 == id )
			{
				LONGLONG pos = pItem->Offset;

				ZeroMemory(pdi->item.pszText,pdi->item.cchTextMax*sizeof(WCHAR));
				PBYTE pb   = pItem->BufferPos;
				PWSTR psz  = pdi->item.pszText;
				SIZE_T cch = pdi->item.cchTextMax;
				for(int i = 0; (i < m_rd.HexByteWidth) && ((pos + i) < m_rd.Size.QuadPart); i++, psz+=3, cch-=3, pb++)
				{
					if( i < (m_rd.HexByteWidth-1) )
 						StringCchPrintf(psz,cch,L"%02X ",*pb);
					else
 						StringCchPrintf(psz,cch,L"%02X",*pb);
				}
			}
			else if( 2 == id )
			{
				LONGLONG pos = pItem->Offset;

				CHAR buf[64+1];
				CHAR *psz = buf;
				SIZE_T cch = _countof(buf);
				PBYTE pb = pItem->BufferPos;

				for(int i = 0; i < m_rd.HexByteWidth; i++,psz++,cch--,pb++)
				{
					if( (pos + i) >= m_rd.Size.QuadPart )
						break;
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

	LRESULT OnItemChanging(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		if( pnmlv->uNewState & LVIS_FOCUSED )
		{
			LONG State = GetListViewState();

			if( State & LVSF_MOUSE_LBUTTONDOWN )
			{
				UINT Flags = LVHT_ONITEM;
				int iItem;
				POINT pt;
				GetCursorPos(&pt);
				MapWindowPoints(NULL,m_hWndList,&pt,1);
				iItem = ListViewEx_HitTest(m_hWndList,pt,&Flags);
				if( iItem != -1 )
				{
					CBinDumpItem *pItem = (CBinDumpItem *)ListViewEx_GetItemData(m_hWndList,iItem);

					RECT rcSubItem;

					ListView_GetSubItemRect(m_hWndList,iItem,1,LVIR_BOUNDS,&rcSubItem);

					if( PtInRect(&rcSubItem,pt) )
					{
						int column = (pt.x - rcSubItem.left);
						column = column / (GetCharWidth() * 3);

						if( column >= m_rd.HexByteWidth )
							column = m_rd.HexByteWidth - 1;

						if( (pItem->Offset + column) >= m_rd.Size.QuadPart )
						{
							column = (int)((m_rd.Size.QuadPart - pItem->Offset) - 1);
						}

						m_rd.CursorPos.QuadPart = pItem->Offset + column;

						RedrawWindow(m_hWndList,NULL,NULL,RDW_INVALIDATE);

						State |= LVSF_CURSOR_CHANGED;
					}
					else
					{
						ListView_GetSubItemRect(m_hWndList,iItem,2,LVIR_BOUNDS,&rcSubItem);

						rcSubItem.left += 6;
						rcSubItem.right -= 6;

						if( PtInRect(&rcSubItem,pt) )
						{
							int column = (pt.x - rcSubItem.left);
							column = column / GetCharWidth();

							if( column >= m_rd.HexByteWidth )
								column = m_rd.HexByteWidth - 1;

							if( (pItem->Offset + column) >= m_rd.Size.QuadPart )
							{
								column = (int)((m_rd.Size.QuadPart - pItem->Offset) - 1);
							}

							m_rd.CursorPos.QuadPart = pItem->Offset + column;

							RedrawWindow(m_hWndList,NULL,NULL,RDW_INVALIDATE);

							State |= LVSF_CURSOR_CHANGED;
						}
					}					
				}

				State &= ~LVSF_MOUSE_LBUTTONDOWN;
				SetListViewState(State);
			}
		}
		return 0;
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		CBinDumpItem *pItem = (CBinDumpItem *)pnmlv->lParam;

		LONG State = GetListViewState();

		if( (pnmlv->uNewState & LVIS_FOCUSED) == LVIS_FOCUSED )
		{
			if( State &  LVSF_CURSOR_CHANGED )
			{
				State &= ~LVSF_CURSOR_CHANGED;
			}
			else
			{
				int column = (m_rd.CursorPos.QuadPart % m_rd.HexByteWidth);
				m_rd.CursorPos.QuadPart = pItem->Offset + column;

				if( m_rd.CursorPos.QuadPart >= m_rd.Size.QuadPart )
					m_rd.CursorPos.QuadPart = m_rd.Size.QuadPart - 1;
			}

			SetListViewState(State);

			RedrawWindow(m_hWndList,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
		}

		if( ((pnmlv->uOldState == 0) || ((pnmlv->uNewState & (LVIS_SELECTED)) == (LVIS_SELECTED))) 
			 && ((pnmlv->uNewState & (LVIS_SELECTED|LVIS_FOCUSED)) == (LVIS_SELECTED|LVIS_FOCUSED)) )
		{
			NotifyCursorChanged( pItem );
		}

		return 0;
	}

	LRESULT OnKeyDown(NMHDR *pnmhdr)
	{
		NMLVKEYDOWN *pnmkd = (NMLVKEYDOWN *)pnmhdr;

		if( GetKeyState(VK_MENU) < 0 )
			SendMessage(m_hWndList,WM_UPDATEUISTATE,MAKELPARAM(UIS_SET,UISF_HIDEFOCUS),0);

		int iItem = ListViewEx_GetCurSel(m_hWndList);

		switch( pnmkd->wVKey )
		{
			case VK_LEFT:
			{
				if( GetKeyState(VK_MENU) < 0 )
				{
					PostMessage(GetParent(m_hWnd),WM_COMMAND,ID_BACK,0);
				}
				else
				{
					if( m_rd.CursorPos.QuadPart > 0 )
					{
						int column = (int)(m_rd.CursorPos.QuadPart % m_rd.HexByteWidth);
						if( column > 0 )
							m_rd.CursorPos.QuadPart--;

						CBinDumpItem *pItem = (CBinDumpItem *)ListViewEx_GetItemData(m_hWndList,iItem);
						if( pItem )
							NotifyCursorChanged( (CBinDumpItem*)pItem );
					}
				}
				break;
			}
			case VK_RIGHT:
			{
				if( GetKeyState(VK_MENU) < 0 )
				{
					PostMessage(GetParent(m_hWnd),WM_COMMAND,ID_NEXT,0);
				}
				else
				{
					if( (m_rd.CursorPos.QuadPart + 1) < m_rd.Size.QuadPart )
					{
						int column = (int)(m_rd.CursorPos.QuadPart % m_rd.HexByteWidth);
						if( (column + 1) < m_rd.HexByteWidth )
							m_rd.CursorPos.QuadPart++;

						CBinDumpItem *pItem = (CBinDumpItem *)ListViewEx_GetItemData(m_hWndList,iItem);
						if( pItem )
							NotifyCursorChanged( (CBinDumpItem*)pItem );
					}
				}
				break;
			}
			case VK_HOME:
			{
				if( GetKeyState(VK_MENU) < 0 )
				{
					PostMessage(GetParent(m_hWnd),WM_COMMAND,ID_FIRST,0);
				}
				break;
			}
			case VK_END:
			{
				if( GetKeyState(VK_MENU) < 0 )
				{
					PostMessage(GetParent(m_hWnd),WM_COMMAND,ID_LAST,0);
				}
				break;
			}
			case VK_NEXT:
			case VK_PRIOR:
			case VK_UP:
			case VK_DOWN:
				// todo:
				break;
		}

		if( iItem == -1 )
			RedrawWindow(m_hWndList,NULL,NULL,RDW_INVALIDATE);
		else
			ListView_RedrawItems(m_hWndList,iItem,iItem);

		return 0;
	}

	//------------------------------------------------------------------------
	//
	//  Command Handler
	//
	//------------------------------------------------------------------------
	LRESULT OnCommand(UINT CmdId)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				OnCmdEditCopy();
				break;
			case ID_VIEW_REFRESH:
				OnCmdRefresh();
				break;
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
			case ID_GOTO:
				OnCmdGotoOffset();
				break;
		}
		return 0;
	}

	void gotoNext()
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.BufferLength != 0);

		if( m_rd.ReadOffset.QuadPart < m_rd.Size.QuadPart )
		{
			m_rd.ReadOffset.QuadPart += m_rd.BufferLength;
			m_rd.CursorPos.QuadPart += m_rd.BufferLength;
			m_rd.TargetOffset.QuadPart = m_rd.CursorPos.QuadPart;
		}

		FillItems();

		if( GetKeyState(VK_SHIFT) < 0 )
		{
			m_bAuto = true;
		}
	
		if( m_bAuto )	
		{
			PBYTE pb = m_rd.Buffer;
			ULONG i;

			for(i = 0; i < m_rd.BufferLength; i++)
			{
				if( *pb != 0 )
				{
					m_bAuto = false;
					break;
				}
			}
			if( i == m_rd.BufferLength )
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
		ASSERT(m_rd.BufferLength != 0);

		if( m_rd.ReadOffset.QuadPart > 0 )
		{
			m_rd.ReadOffset.QuadPart -= m_rd.BufferLength;
			m_rd.CursorPos.QuadPart -= m_rd.BufferLength;
			m_rd.TargetOffset.QuadPart = m_rd.CursorPos.QuadPart;
		}

		FillItems();

		if( GetKeyState(VK_SHIFT) < 0 )
		{
			m_bAuto = true;
		}
	
		if( m_bAuto )	
		{
			PBYTE pb = m_rd.Buffer;
			ULONG i;

			for(i = 0; i < m_rd.BufferLength; i++)
			{
				if( *pb != 0 )
				{
					m_bAuto = false;
					break;
				}
			}
			if( i == m_rd.BufferLength )
			{
				RedrawWindow(m_hWndList,0,0,RDW_INVALIDATE|RDW_UPDATENOW);
				DoMessage();
				PostMessage(m_hWnd,WM_COMMAND,ID_AUTO_BACK,0);
			}
		}
	}

	void gotoLast(BOOL bLastByte=false)
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.BufferLength != 0);

		LONGLONG off;

		if( !bLastByte )
			off = m_rd.CursorPos.QuadPart - m_rd.ReadOffset.QuadPart; // save current offset in page relative

		if( m_rd.Size.QuadPart % m_rd.ClusterSize )
			m_rd.ReadOffset.QuadPart = m_rd.ClusterSize * (m_rd.Size.QuadPart / m_rd.ClusterSize);
		else
			m_rd.ReadOffset.QuadPart = m_rd.Size.QuadPart - m_rd.ClusterSize;

		if( bLastByte )
		{
			m_rd.TargetOffset.QuadPart = m_rd.CursorPos.QuadPart = m_rd.Size.QuadPart - 1;
		}
		else
		{
			m_rd.TargetOffset.QuadPart = m_rd.ReadOffset.QuadPart + off;
			m_rd.CursorPos.QuadPart = m_rd.TargetOffset.QuadPart;
		}

		FillItems();
	}

	void gotoFirst(BOOL bFirstByte=false)
	{
		ASSERT(m_rd.Buffer != NULL);
		ASSERT(m_rd.BufferLength != 0);

		LONGLONG off;

		if( !bFirstByte )
			off = m_rd.CursorPos.QuadPart - m_rd.ReadOffset.QuadPart; // save current offset in page relative

		m_rd.ReadOffset.QuadPart = 0;

		if( bFirstByte )
		{
			m_rd.TargetOffset.QuadPart = m_rd.CursorPos.QuadPart = 0;
		}
		else
		{
			m_rd.TargetOffset.QuadPart = m_rd.ReadOffset.QuadPart + off;
			m_rd.CursorPos.QuadPart = m_rd.TargetOffset.QuadPart;
		}

		FillItems();
	}

	void copyText()
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

	void OnCmdEditCopy()
	{
		copyText();
	}

	void OnCmdRefresh()
	{
		Reload();
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

	void OnCmdGotoOffset()
	{
		GotoOffset();
	}
};
