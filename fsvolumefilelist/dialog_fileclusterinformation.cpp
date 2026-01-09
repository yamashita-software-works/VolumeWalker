//****************************************************************************
//*                                                                          *
//*  dialog_fileclusterinformation.cpp                                       *
//*                                                                          *
//*  File cluster information viewer                                         *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2025-09-24 Created                                             *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "stdafx.h"
#include "resource.h"
#include "uilayout.h"
#include "fsvolumefilelist.h"

static PCWSTR g_pszTitle =  L"Cluster Information";

typedef struct _CI_DIALOG_PARAM
{
	HWND hWndDialog;
	HWND hWndList;
	HWND hWndHost;
	FS_CLUSTER_INFORMATION *pClusterInfo;
	WCHAR szVolumeName[MAX_PATH];
} CI_DIALOG_PARAM;

typedef struct _CI_LIST_ITEM
{
	const CI_DIALOG_PARAM *pdlgParam;
	UINT Type;
	struct {
		FS_CLUSTER_LOCATION *Location;
		FS_VOLUME_PHYSICAL_OFFSET *PhysicalOffset;
	} Cluster;
} CI_LIST_ITEM;

//////////////////////////////////////////////////////////////////////////////
// Dialog Implementation

struct CClusterInformationDialog : public CDialogWindowEx
{
	HWND m_hWndList;
	PCWSTR m_pszFileName;

	CUILayout m_Layout;

	CClusterInformationDialog()
	{
		m_hWndList = NULL;
		m_pszFileName = NULL;
	}
	
	~CClusterInformationDialog()
	{
	}
	
	int InsertItem(HWND hWndList,CI_DIALOG_PARAM *pdlgParam,int iItem,int iType,FS_CLUSTER_LOCATION *pcl,FS_VOLUME_PHYSICAL_OFFSET *pvpo)
	{
		CI_LIST_ITEM *pItem = new CI_LIST_ITEM;
		if( pItem == NULL )
			return -1;

		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);
	
		pItem->pdlgParam = pdlgParam;
		pItem->Type      = iType;
		pItem->Cluster.Location        = (FS_CLUSTER_LOCATION *)pcl;
		pItem->Cluster.PhysicalOffset  = (FS_VOLUME_PHYSICAL_OFFSET *)pvpo;

		LVITEM lvi = {0};
		lvi.mask    = LVIF_PARAM|LVIF_TEXT|LVIF_IMAGE;
		lvi.pszText = (PWSTR)LPSTR_TEXTCALLBACK;
		lvi.lParam  = (LPARAM)pItem;
		lvi.iItem   = iItem;

		iItem = ListView_InsertItem(hWndList,&lvi);
	
		return iItem;
	}

	VOID FillListItems(HWND hWndList,CI_DIALOG_PARAM *pdlgParam,PVOID)
	{
		FS_CLUSTER_INFORMATION *pci = pdlgParam->pClusterInfo;

		ULONG ext;
		for(ext = 0; ext < pci->ExtentCount; ext++)
		{
			InsertItem(hWndList,pdlgParam,ext,0,&pci->Extents[ext],NULL);

			if( pci->Extents[ext].PhysicalOffsets && pci->Extents[ext].PhysicalOffsets->NumberOfPhysicalOffsets > 1 )
			{
				for( UINT index = 1; index < pci->Extents[ext].PhysicalOffsets->NumberOfPhysicalOffsets; index++ )
				{
					InsertItem(hWndList,pdlgParam,-1,1,
							&pci->Extents[ext],
							&pci->Extents[ext].PhysicalOffsets->PhysicalOffset[index]);
				}
			}
		}
	}

	VOID OpenDump(int iItem)
	{
		CI_LIST_ITEM *pItem = (CI_LIST_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

		if( pItem->Type == 0 )
		{
			if( pItem->Cluster.Location->Lcn.QuadPart == -1 )
				return;
		}
		else
		{
			if( pItem->Cluster.PhysicalOffset == NULL )
				return;
		}

		OPEN_MDI_CHILDFRAME_PARAM open_mdi = {0};
		open_mdi.Flags = 0;
		open_mdi.Path = (PWSTR)pItem->pdlgParam->szVolumeName;
		open_mdi.StartOffset.QuadPart = 
					(pItem->pdlgParam->pClusterInfo->BytesPerCluster * pItem->Cluster.Location->Lcn.QuadPart);
		SendMessage(GetWindow(m_hWnd,GW_OWNER),WM_OPEN_MDI_CHILDFRAME,MAKEWPARAM(VOLUME_CONSOLE_SIMPLEHEXDUMP,0),(LPARAM)&open_mdi);
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		CI_DIALOG_PARAM *pdlgParam = (CI_DIALOG_PARAM *)lParam;
	
		pdlgParam->hWndDialog = hDlg;

		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		//
		// Initialize Layout
		//
		m_Layout.Initialize(hDlg);			
		m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_BOTTOMRIGHT,IDC_LIST,FALSE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMRIGHT,CUILayout::AP_BOTTOMRIGHT,IDCLOSE,FALSE);

		DWORD dw = GetWindowLong(hDlg,GWL_EXSTYLE);
		dw |= WS_EX_DLGMODALFRAME;
		SetWindowLong(hDlg,GWL_EXSTYLE,dw);
		if( (HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0) == NULL )
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)NULL);

		_CenterWindow( hDlg, pdlgParam->hWndHost );

		//
		// Initialize Target File ListView
		//
		m_hWndList = GetDlgItem(hDlg,IDC_LIST);

		ASSERT( (GetWindowLong(m_hWndList, GWL_STYLE) & LVS_SHAREIMAGELISTS) );

		pdlgParam->hWndList = m_hWndList;
	
		SendMessage(m_hWndList,WM_SETFONT,(WPARAM)GetGlobalFont(hDlg),0);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_INFOTIP|LVS_EX_LABELTIP);
	
		_EnableVisualThemeStyle(m_hWndList);
	
		RECT rc;
		GetClientRect(m_hWndList,&rc);
		int cx = _RECT_WIDTH(rc);
	
		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_FMT|LVCF_TEXT|LVCF_WIDTH;//|LVCF_ORDER;

		struct {
			PWSTR Text;
			int fmt;
		} Columns[] = {
			{L"VCN",               LVCFMT_RIGHT},
			{L"LCN Start",         LVCFMT_RIGHT},
			{L"LCN End",           LVCFMT_RIGHT},
			{L"Cluster Count",     LVCFMT_RIGHT},
			{L"Logical Offset",    LVCFMT_RIGHT},
			{L"Physical Drive",    LVCFMT_LEFT},
			{L"Physical Offset",   LVCFMT_RIGHT},
			{L"Extent Size",       LVCFMT_RIGHT},
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

		FillListItems(m_hWndList,pdlgParam,NULL);

		int cxColumn = 0;
		{
			int cColumns = ListViewEx_GetColumnCount(m_hWndList);
			for(int i = 0; i < cColumns; i++)
			{
				ListView_SetColumnWidth(m_hWndList,i,LVSCW_AUTOSIZE_USEHEADER);
			}
		}
		ListView_SetColumnWidth(m_hWndList,0,
				ListView_GetColumnWidth(m_hWndList,0) + DPI_SIZE_CX(20));

		ListViewEx_SetLastColumnWidth(m_hWndList,LVEXCHTF_ADJUST_WIDTH_BY_COLUMN_ITEM_TEXT);

		SetDlgItemText(hDlg,IDC_TEXT,m_pszFileName);

		RECT rcList;
		GetWindowRect(m_hWndList,&rcList);
		MapWindowPoints(NULL,hDlg,(LPPOINT)&rcList,2);
		CDialogWindowEx::m_cyHeaderHight = rcList.top;

		CDialogWindowEx::OnInitDialog(hDlg);

		return FALSE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		CDialogWindowEx::OnDestory(hDlg);

		CI_DIALOG_PARAM *pdlgParam = (CI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		HFONT hFont = (HFONT)SendMessage(m_hWndList,WM_GETFONT,0,0);
		DeleteObject(hFont);

		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		CI_DIALOG_PARAM *pdlgParam = (CI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		return 0;
	}
	
	LRESULT OnSize(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		m_Layout.AdjustControls(cx,cy);
		return 0;
	}

	LRESULT OnGetDispInfo(NMLVDISPINFO* pnmlvdi)
	{
		if( pnmlvdi->item.mask & LVIF_IMAGE )
		{
			; // reserved
		}
		if( pnmlvdi->item.mask & LVIF_TEXT )
		{
			CI_LIST_ITEM *pItem = (CI_LIST_ITEM *)pnmlvdi->item.lParam;
	
			*pnmlvdi->item.pszText = L'\0';

			StringCchCopy(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax, L"-");

			switch( pnmlvdi->item.iSubItem )
			{
				case 0: // VCN
					if( pItem->Type == 0 )
					{
						StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->Cluster.Location->Vcn.QuadPart);
					}
					break;
				case 1: // LCN Start
					if( pItem->Type == 0 )
					{
						if( pItem->Cluster.Location->Lcn.QuadPart != -1 )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->Cluster.Location->Lcn.QuadPart);
					}
					break;
				case 2: // LCN End
					if( pItem->Type == 0 )
					{
						if( pItem->Cluster.Location->Lcn.QuadPart != -1 )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->Cluster.Location->Lcn.QuadPart
								+ pItem->Cluster.Location->Count.QuadPart - 1);
					}
					break;
				case 3: // Cluster Count
					if( pItem->Type == 0 )
					{
						_CommaFormatString(pItem->Cluster.Location->Count.QuadPart,pnmlvdi->item.pszText);
					}
					break;
				case 4: // Logical Offset
					if( pItem->Type == 0 )
					{
						if( pItem->Cluster.Location->Lcn.QuadPart != -1 )
						{
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->pdlgParam->pClusterInfo->BytesPerCluster
								* pItem->Cluster.Location->Lcn.QuadPart);
						}
					}
					break;
				case 5: // Physical Drive
					if( pItem->Type == 0 )
					{
						if( pItem->Cluster.Location->PhysicalOffsets )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"PhysicalDrive%d",pItem->Cluster.Location->PhysicalOffsets->PhysicalOffset[0].DiskNumber);
					}
					else
					{
						if( pItem->Cluster.PhysicalOffset )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"PhysicalDrive%d",pItem->Cluster.PhysicalOffset->DiskNumber);
					}
					break;
				case 6: // Physical Offset
					if( pItem->Type == 0 )
					{
						if( pItem->Cluster.Location->PhysicalOffsets )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->Cluster.Location->PhysicalOffsets->PhysicalOffset[0].Offset);
					}
					else
					{
						if( pItem->Cluster.PhysicalOffset )
							StringCchPrintf(pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax,L"0x%I64X",pItem->Cluster.PhysicalOffset->Offset);
					}
					break;
				case 7: // Extent Size
					StrFormatByteSizeW((pItem->pdlgParam->pClusterInfo->BytesPerCluster * pItem->Cluster.Location->Count.QuadPart),pnmlvdi->item.pszText,pnmlvdi->item.cchTextMax);
					break;
			}
		}
		return 0;
	}

	LRESULT OnDeleteItem(NMLISTVIEW* pnmlv)
	{
		CI_LIST_ITEM *p = (CI_LIST_ITEM*)pnmlv->lParam;
		delete p;
		return 0;
	}

	LRESULT OnItemActivate(NMHDR *pnmhdr)
	{
		NMITEMACTIVATE *pnmia = (NMITEMACTIVATE *)pnmhdr;
		if( pnmia->iItem != -1 )
		{
			OpenDump(pnmia->iItem);
		}
		return 0;
	}

	LRESULT OnNotify(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		switch( ((NMHDR *)lParam)->code )
		{
			case LVN_GETDISPINFO:
				OnGetDispInfo((NMLVDISPINFO*)lParam);
				return TRUE;
			case LVN_DELETEITEM:
				OnDeleteItem((NMLISTVIEW*)lParam);
				return TRUE;
			case LVN_ITEMACTIVATE:
				return OnItemActivate((NMHDR*)lParam);
		}
		return 0;
	}

	LRESULT OnContextMenu(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		CI_LIST_ITEM *pItem = (CI_LIST_ITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

		HMENU hMenu = CreatePopupMenu();

		UINT f = 0;
		if( pItem->Type == 0 )
		{
			if( pItem->Cluster.Location->Lcn.QuadPart == -1 )
				f = MF_DISABLED;
		}
		else
		{
			if( pItem->Cluster.PhysicalOffset == NULL )
				f = MF_DISABLED;
		}
		AppendMenu(hMenu,MF_STRING|f,ID_HEXDUMP,L"Show Cluster in &Dump Window");
		AppendMenu(hMenu,MF_STRING,0,NULL);
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Line Text");

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

		SetMenuDefaultItem(hMenu,ID_HEXDUMP,FALSE);

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,(HWND)wParam,hMenu,pt,0);

		DestroyMenu(hMenu);

		return 0;
	}

	VOID OnOK(HWND hDlg)
	{
		CI_DIALOG_PARAM *pdlgParam = (CI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDOK);
	}

	VOID OnCancel(HWND hDlg)
	{
		CI_DIALOG_PARAM *pdlgParam = (CI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCANCEL);
	}

	VOID OnCmdClose(HWND hDlg)
	{
		CI_DIALOG_PARAM *pdlgParam = (CI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		EndDialog(hDlg,IDCLOSE);
	}

	VOID OnCmdHexDump(HWND hDlg)
	{
		CI_DIALOG_PARAM *pdlgParam = (CI_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return ;

		OpenDump(iItem);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		UINT uCmdId = LOWORD(wParam);
		switch( uCmdId )
		{
			case IDOK:
				OnOK(hDlg);
				break;
			case IDCANCEL:
				OnCancel(hDlg);
				break;
			case IDCLOSE:
				OnCmdClose(hDlg);
				break;
			case ID_EDIT_COPY:
				SetClipboardTextFromListView(m_hWndList,SCTEXT_UNICODE);
				break;
			case ID_HEXDUMP:
				OnCmdHexDump(hDlg);
				break;
			default:
				if( ID_EDIT_COPY_COUMN_FIRST <= uCmdId && uCmdId <= ID_EDIT_COPY_COUMN_LAST )
					SetClipboardTextFromListViewColumn(m_hWndList,SCTEXT_FORMAT_SELECTONLY,(LOWORD(wParam)-ID_EDIT_COPY_COUMN_FIRST));
				break;
		}
		return 0;
	}

	virtual INT_PTR OnCtlColor(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_CTLCOLORBTN:
			case WM_CTLCOLORDLG:
				return (INT_PTR)m_hbrBackground;
			case WM_CTLCOLORSTATIC:
				SetBkMode((HDC)wParam,TRANSPARENT);
				return (INT_PTR)m_hbrHeader;
		}
		return 0;
	}

	//---------------------------------------------------------------------------
	//
	//  DialogProc()
	//
	//  PURPOSE: Dialog Window Procedure
	//
	//---------------------------------------------------------------------------
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch( uMsg )
		{
			case WM_INITDIALOG:
				return OnInitDialog(hDlg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hDlg,wParam,lParam);
			case WM_SIZE:
				return OnSize(hDlg,wParam,lParam);
			case WM_CLOSE:
				return OnClose(hDlg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hDlg,wParam,lParam);
			case WM_NOTIFY:
				return OnNotify(hDlg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hDlg,wParam,lParam);
		}
		return CDialogWindowEx::DlgProc(hDlg,uMsg,wParam,lParam);
	}
};

static FS_CLUSTER_INFORMATION *_CreateClusterInformationBuffer(PCWSTR pszFilePath)
{
	NTSTATUS Status;
	UNICODE_STRING usFileName;
	DWORD dwError;

	if( IsRootDirectory_W(pszFilePath) )
	{
		usFileName.Buffer =  L"";
	}
	else
	{
		SplitPathFileName_W(pszFilePath,NULL,&usFileName);
	}

	PWSTR Path;
	Path = DuplicateString(pszFilePath);
	if( Path == NULL )
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return NULL;
	}

	FS_CLUSTER_INFORMATION *pci = NULL;
	PWSTR RootDirectory=NULL,RootRelativePath=NULL;
	ULONG cchRootDirectory=0,cchRootRelativePath=0;
	PWSTR FileName = NULL;
	RemoveFileSpec(Path);
	SplitRootPath_W(Path,&RootDirectory,&cchRootDirectory,&RootRelativePath,&cchRootRelativePath);
	FreeMemory(Path);

	if( RootDirectory && RootRelativePath )
	{
		HANDLE hRootDirectory = NULL;
		if( OpenRootDirectory(RootDirectory,0,&hRootDirectory) != STATUS_SUCCESS )
		{
			//
			// The Root directory open failed. If so try open directory using
			// full path string without splitting the volume and root relative path.
			//
			FreeMemory(RootRelativePath);
			RootRelativePath = NULL;
			hRootDirectory = NULL;
		}
	
		HANDLE hCurDir;
		if( (Status = OpenFile_W(&hCurDir,hRootDirectory,RootRelativePath,FILE_READ_ATTRIBUTES|SYNCHRONIZE,
							FILE_SHARE_READ|FILE_SHARE_WRITE,FILE_DIRECTORY_FILE)) == STATUS_SUCCESS )
		{
			HANDLE hFile;
			ULONG DesiredAccess = FILE_READ_ATTRIBUTES|SYNCHRONIZE;
			ULONG Option = FILE_OPEN_REPARSE_POINT|FILE_OPEN_FOR_BACKUP_INTENT|FILE_SYNCHRONOUS_IO_NONALERT;
			if( (Status = OpenFile_W(&hFile,hCurDir,usFileName.Buffer,DesiredAccess,FILE_SHARE_READ|FILE_SHARE_WRITE,Option)) == STATUS_SUCCESS )
			{
				dwError = ReadFileClusterInformaion(NULL,hFile,RootDirectory,ClusterInformationAll,&pci,sizeof(pci));
	
				CloseHandle(hFile);
			}
			else
			{
				dwError = NtStatusToDosError(Status);
			}
	
			CloseHandle(hCurDir);
		}
		else
		{
			dwError = NtStatusToDosError(Status);
		}

		if( hRootDirectory )
			CloseHandle(hRootDirectory);
	}
	else
	{
		dwError = ERROR_OUTOFMEMORY;
	}

	FreeMemory(RootRelativePath);
	FreeMemory(RootDirectory);

	SetLastError(dwError);

	return pci;
}

//---------------------------------------------------------------------------
//
//  FileClusterInformationDialog()
//
//  PURPOSE: 
//    File Cluster Location Viewer Dialog.
//
//  PATAMETERS:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
FileClusterInformationDialog(
	HWND hWnd,
	PCWSTR pszFilePath,
	UINT Reserved,
	PVOID Ptr
	)
{
	HRESULT hr;

	WCHAR *pszStreamName = NULL;
	DWORD cchStreamName = 0;

	hr = FileSelectStreamDialog(hWnd,pszFilePath,&pszStreamName,&cchStreamName,FSSDF_MAKEFULLPATH);

	if( hr == S_FALSE )
	{
		return S_FALSE; // user cancel
	}
	else if( hr == S_SSD_NO_STREAM )
	{
		;
	}
	else if( hr == S_SSD_DEFAULT_STREAM_ONLY )
	{
		;
	}
	else if( FAILED(hr) )
	{
		_ErrorMessageBoxEx(hWnd,0,g_pszTitle,NULL,hr,MB_OK|MB_ICONSTOP);
		return hr;
	}

	if( pszStreamName )
	{
		// Remove stream type string (e.g. "::$DATA")
		WCHAR *p;
		p = wcsrchr(pszStreamName,L':');
		if( p )
		{
			*p = L'\0';
			if( *(p - 1) == L':' )
				*(--p) = L'\0';
		}
	}

	FS_CLUSTER_INFORMATION *pci;

	pci = (FS_CLUSTER_INFORMATION *)_CreateClusterInformationBuffer(pszStreamName ? pszStreamName : pszFilePath);
	if( pci == NULL )
	{
		DWORD dwError = GetLastError();

		if( dwError == ERROR_FILE_NOT_FOUND )
			MsgBox(hWnd,L"Cluster not in use.",g_pszTitle,MB_OK|MB_ICONINFORMATION);
		else
			_ErrorMessageBoxEx(hWnd,0,g_pszTitle,NULL,dwError,MB_OK|MB_ICONSTOP);

		return E_FAIL;
	}

	CI_DIALOG_PARAM *pParam = new CI_DIALOG_PARAM;
	if( pParam == NULL )
		return E_OUTOFMEMORY;

	ZeroMemory(pParam,sizeof(CI_DIALOG_PARAM));

	pParam->hWndHost = hWnd;
	pParam->pClusterInfo = pci;

	if( pszStreamName )
		NtPathGetVolumeName(pszStreamName,pParam->szVolumeName,MAX_PATH);
	else
		NtPathGetVolumeName(pszFilePath,pParam->szVolumeName,MAX_PATH);

	CClusterInformationDialog *dlg = new CClusterInformationDialog;
	if( dlg ) 
	{
		PWSTR pszName;
		pszName = (PWSTR)(pszStreamName ? pszStreamName : pszFilePath);
		if( !IsRootDirectory_W(pszName) )
			pszName = wcsrchr(pszName,L'\\');

		if( pszName )
			pszName++;

		dlg->m_pszFileName = pszName;

		dlg->DoModal(hWnd,IDD_FILE_CLUSTER_LAYOUT,(LPARAM)pParam,_GetResourceInstance());

		delete dlg;

		hr = S_OK;
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	delete pParam;

	FreeClusterInformation(pci);

	return hr;
}
