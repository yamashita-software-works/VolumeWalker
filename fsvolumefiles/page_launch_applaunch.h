#pragma once
//*****************************************************************************
//
//  page_launch_applaunch.h
//
//  PURPOSE: External Tool/Application Launch Page
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-05-13 Created.
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "libntwdk.h"
#include "ntnativeapi.h"
#include "externaltools.h"
#include "runhelp.h"

typedef struct _LAUNCHPAD_PAGE_PARAM
{
	HWND hWnd;
} LAUNCHPAD_PAGE_PARAM;

class LaunchBoxItem 
{
private:
	PWSTR pszName;
	PWSTR pszDescriptionText;
	PWSTR pszPath;
	PWSTR pszStartupDirectory;

public:
	LaunchBoxItem()
	{
		pszName = NULL;
		pszDescriptionText = NULL;
		pszPath = NULL;
		pszStartupDirectory = NULL;
	}

	~LaunchBoxItem()
	{
		_SafeMemFree(pszName);
		_SafeMemFree(pszDescriptionText);
		_SafeMemFree(pszPath);
		_SafeMemFree(pszStartupDirectory);
	}

	void SetName(PCWSTR Name)
	{
		pszName = _MemAllocString(Name);
	}

	void SetDescriptionText(PCWSTR DescriptionText)
	{
		pszDescriptionText = _MemAllocString(DescriptionText);
	}

	void SetPath(PCWSTR Path)
	{
		pszPath = _MemAllocString(Path);
	}

	void SetStartupDirectory(PCWSTR StartupDirectory)
	{
		if( StartupDirectory )
			pszStartupDirectory = _MemAllocString(StartupDirectory);
	}

	PWSTR GetDescriptionText() const
	{
		return pszDescriptionText;
	}

	PWSTR GetName() const
	{
		return pszName;
	}

	PWSTR GetPath() const
	{
		return pszPath;
	}

	PWSTR GetStartupDirectory() const
	{
		return pszStartupDirectory;
	}
};

class CAppLaunchPage : public CLaunchPadPage
{
	HWND m_hWndList;
	HIMAGELIST m_himlNormal;
	HIMAGELIST m_himlSmall;
	CUILayout m_Layout;

public:
	DEFDIALOGRESID(IDD_LAUNCHPAD_PAGE_APPLAUNCH)

	CAppLaunchPage()
	{
		m_hWndList = NULL;
		m_himlSmall = NULL;
		m_himlNormal = NULL;
	}

	~CAppLaunchPage()
	{
		ASSERT( m_himlSmall == NULL );
		ASSERT( m_himlNormal == NULL );
	}

	virtual VOID OnInitPage(PVOID)
	{
	}

	VOID FillItems()
	{
		CWaitCursor wait;

		IExternalTools *pTools;
		if( CreateExternalTools( &pTools ) != S_OK )
		{
			return;
		}

		SetRedraw(m_hWndList,FALSE);

		int i,cItems;
		pTools->GetCount(&cItems);

		PCWSTR psz;
		HICON hIcon;
		int iImage;
		for(i = 0; i < cItems; i++)
		{
			psz = pTools->GetName(i);

			if( psz == NULL || *psz == 0 )
				continue;

			hIcon = NULL;

			if( pTools->GetIcon(i,32,&hIcon) != S_OK )
			{
				LPITEMIDLIST pidl;
				WCHAR szPath[MAX_PATH];
				StringCchCopy(szPath,MAX_PATH,pTools->GetPath(i));
				PathUnquoteSpaces(szPath);
				pidl = ILCreateFromPath(szPath);
				if( pidl )
				{
					SHFILEINFO sfi = {0};
					SHGetFileInfo((LPCWSTR)pidl,0,&sfi,sizeof(sfi),SHGFI_PIDL|SHGFI_ICON|SHGFI_LARGEICON|SHGFI_TYPENAME|SHGFI_DISPLAYNAME);
					hIcon = sfi.hIcon;
					ILFree(pidl);
				}
				else
				{
					hIcon = (HICON)LoadImage(NULL,IDI_ERROR,IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_SHARED);
				}
			}

			iImage = ImageList_AddIcon(m_himlNormal,hIcon);

			LaunchBoxItem *pItem  = new LaunchBoxItem;

			if( pItem )
			{
				pItem->SetName( pTools->GetName(i) );
				pItem->SetDescriptionText( pTools->GetPath(i) );
				pItem->SetPath( pTools->GetPath(i) );
				pItem->SetStartupDirectory( pTools->GetStartupDirectory(i) );

				InsertItem(m_hWndList,-1,iImage,pItem);
			}

			DestroyIcon(hIcon);
		}

		pTools->Release();

		SetRedraw(m_hWndList,TRUE);
	}

	int InsertItem(HWND hWndList,int iItem,int iImage, LaunchBoxItem *pItem)
	{
		if( iItem == -1 )
			iItem = ListView_GetItemCount(hWndList);

		LVITEM lvi = {0};
		lvi.mask     = LVIF_TEXT|LVIF_IMAGE|LVIF_INDENT|LVIF_PARAM;
		lvi.iItem    = iItem;
		lvi.iImage   = iImage;
		lvi.iIndent  = 1;
		lvi.lParam   = (LPARAM)pItem;
		lvi.pszText  = (PWSTR)LPSTR_TEXTCALLBACK;

		iItem = ListView_InsertItem(hWndList,&lvi);
		ListView_SetItemText(hWndList,iItem,1,LPSTR_TEXTCALLBACK);

		return iItem;
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		LAUNCHPAD_PAGE_PARAM *pdlgParam = (LAUNCHPAD_PAGE_PARAM *)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		//
		// Initialize Layout
		//
		m_Layout.Initialize(hDlg);			
		m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_BOTTOMRIGHT,IDC_LIST,FALSE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMRIGHT,CUILayout::AP_BOTTOMRIGHT,IDOK,TRUE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMLEFT,CUILayout::AP_BOTTOMRIGHT,IDC_EDIT1,TRUE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMLEFT,CUILayout::AP_BOTTOMRIGHT,IDC_EDIT2,TRUE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMLEFT,CUILayout::AP_BOTTOMLEFT,IDC_TEXT1,TRUE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMLEFT,CUILayout::AP_BOTTOMLEFT,IDC_TEXT2,TRUE);

		//
		// Initialize ListView
		//
		m_hWndList = GetDlgItem(hDlg,IDC_LIST);

		_EnableVisualThemeStyle(m_hWndList);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_INFOTIP);

		LVCOLUMN lvc;
		lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = 120;
		lvc.pszText = L"Name";
		ListView_InsertColumn(m_hWndList,0,&lvc);

		lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT;
		lvc.cx = 240;
		lvc.pszText = L"Description";
		ListView_InsertColumn(m_hWndList,1,&lvc);

		m_himlSmall = ImageList_Create(
							GetSystemMetrics(SM_CXSMICON),
							GetSystemMetrics(SM_CYSMICON),
							ILC_COLOR32 | ILC_MASK,
							8, 0);

		m_himlNormal = ImageList_Create(
							GetSystemMetrics(SM_CXICON),
							GetSystemMetrics(SM_CYICON),
							ILC_COLOR32 | ILC_MASK,
							8, 0);

		ListView_SetImageList(m_hWndList,m_himlSmall,LVSIL_SMALL);
		ListView_SetImageList(m_hWndList,m_himlNormal,LVSIL_NORMAL);

		FillItems();

		ListView_SetView(m_hWndList,LV_VIEW_ICON);

		SendDlgItemMessage(hDlg,IDC_EDIT2,EM_LIMITTEXT,MAX_PATH,0);

		EnableWindow(GetDlgItem(m_hWnd,IDOK),FALSE);

		return TRUE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		LAUNCHPAD_PAGE_PARAM *pdlgParam = (LAUNCHPAD_PAGE_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		if( m_himlSmall ) {
			if( ImageList_Destroy(m_himlSmall) )
				m_himlSmall = NULL;
		}
		if( m_himlNormal ) {
			if( ImageList_Destroy(m_himlNormal) )
				m_himlNormal = NULL;
		}
		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		LAUNCHPAD_PAGE_PARAM *pdlgParam = (LAUNCHPAD_PAGE_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
		return 0;
	}
	
	LRESULT OnSize(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		m_Layout.AdjustControls(cx,cy);
		return 0;
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;

		LaunchBoxItem *pItem = (LaunchBoxItem *)pdi->item.lParam;

		switch( pdi->item.iSubItem )
		{
			case 0:
				StringCchCopy(pdi->item.pszText,pdi->item.cchTextMax,pItem->GetName());
				break;
			case 1:
				StringCchCopy(pdi->item.pszText,pdi->item.cchTextMax,pItem->GetDescriptionText());
				break;
		}
		return 0;	
	}

	LRESULT OnItemChanged(LPNMLISTVIEW pnmlv)
	{
		if( (pnmlv->uNewState & (LVIS_FOCUSED|LVIS_SELECTED)) == (LVIS_FOCUSED|LVIS_SELECTED) )
		{
			SetStatusBox((LaunchBoxItem *)pnmlv->lParam);
			EnableWindow(GetDlgItem(m_hWnd,IDOK),TRUE);
		}
		else
		{
			SetStatusBox(NULL);
			EnableWindow(GetDlgItem(m_hWnd,IDOK),FALSE);
		}

		return 0;
	}

	LRESULT OnItemActivate(LPNMLISTVIEW pnmlv)
	{
		OnOK(m_hWnd);
		return 0;
	}

	LRESULT OnGetInfoTip(LPNMLVGETINFOTIP pnmit)
	{
		LaunchBoxItem *pItem = (LaunchBoxItem *)ListViewEx_GetItemData(m_hWndList,pnmit->iItem);

		if( pnmit->dwFlags & LVGIT_UNFOLDED )
		{
			StringCchCopy(pnmit->pszText,pnmit->cchTextMax, pItem->GetDescriptionText());
		}
		else
		{
			StringCchCat(pnmit->pszText,pnmit->cchTextMax, L"\n\n");
			StringCchCat(pnmit->pszText,pnmit->cchTextMax, pItem->GetDescriptionText());
		}

		return 0;
	}

	LRESULT OnDeleteItem(NMLISTVIEW *pnmlv)
	{
		LaunchBoxItem *pItem = (LaunchBoxItem *)pnmlv->lParam;
		delete pItem;
		return 0;
	}

	LRESULT OnGetEmptyMarkup(NMLVEMPTYMARKUP *pnmlvem)
	{
		pnmlvem->dwFlags = EMF_CENTERED;
		StringCchCopy(pnmlvem->szMarkup,L_MAX_URL_LENGTH,L"No Apps.");
		return TRUE;
	}

	LRESULT OnNotify(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		switch( ((NMHDR *)lParam)->code )
		{
			case LVN_GETDISPINFO:
				return OnGetDispInfo((NMHDR*)lParam);
			case LVN_ITEMCHANGED:
				return OnItemChanged((LPNMLISTVIEW)lParam);
			case LVN_ITEMACTIVATE:
				return OnItemActivate((LPNMLISTVIEW)lParam);
			case LVN_GETINFOTIP:
				return OnGetInfoTip((LPNMLVGETINFOTIP)lParam);
			case LVN_DELETEITEM:
				return OnDeleteItem((LPNMLISTVIEW)lParam);
			case LVN_GETEMPTYMARKUP:
				return OnGetEmptyMarkup((NMLVEMPTYMARKUP*)lParam);
		}
		return 0;
	}

	VOID OnOK(HWND hDlg)
	{
		LAUNCHPAD_PAGE_PARAM *pdlgParam = (LAUNCHPAD_PAGE_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return ;

		LaunchBoxItem *pItem = (LaunchBoxItem *)ListViewEx_GetItemData(m_hWndList,iItem);

		ASSERT(pItem != NULL);

		WCHAR szCmdLine[MAX_PATH];
		WCHAR szStartupDir[MAX_PATH];
		WCHAR szExecPath[MAX_PATH];

		ZeroMemory(szCmdLine,sizeof(szCmdLine));
		ZeroMemory(szStartupDir,sizeof(szStartupDir));
		ZeroMemory(szExecPath,sizeof(szExecPath));

		if( pItem->GetStartupDirectory() )
			StringCchCopy(szStartupDir,sizeof(szStartupDir),pItem->GetStartupDirectory());

		ExpandEnvironmentStrings(pItem->GetPath(),szExecPath,MAX_PATH);
		PathRemoveArgs(szExecPath);
		PathUnquoteSpaces(szExecPath);

		ExpandEnvironmentStrings(pItem->GetPath(),szCmdLine,MAX_PATH);
		PWSTR pArgs = PathGetArgs(szCmdLine);

		WCHAR *pszAdditionalArg = NULL;
		WCHAR *pszAdditionalArgTemp = NULL;
		if( GetWindowTextLength(GetDlgItem(hDlg,IDC_EDIT2)) > 0 )
		{
			pszAdditionalArg = new WCHAR[MAX_PATH];
			pszAdditionalArgTemp = new WCHAR[MAX_PATH];
			if( pszAdditionalArg && pszAdditionalArgTemp )
			{
				ZeroMemory(pszAdditionalArg,sizeof(WCHAR)*MAX_PATH);
				ZeroMemory(pszAdditionalArgTemp,sizeof(WCHAR)*MAX_PATH);

				GetDlgItemText(hDlg,IDC_EDIT2,pszAdditionalArgTemp,MAX_PATH);

				if( *pArgs != 0 )
				{
					StringCchCopy(pszAdditionalArg,MAX_PATH,pArgs);
					StringCchCat(pszAdditionalArg,MAX_PATH,L" "); // arg sep
				}

				size_t cch = wcslen(pszAdditionalArg);

				ExpandEnvironmentStrings(pszAdditionalArgTemp,&pszAdditionalArg[cch],MAX_PATH);

				pArgs = pszAdditionalArg;

				delete[] pszAdditionalArgTemp;
			}
		}

		{
			SHELLEXECUTEINFO sei = {0};
			sei.cbSize       = sizeof(sei);
			sei.lpFile       = szExecPath;
			sei.lpParameters = pArgs;
			sei.lpDirectory  = szStartupDir;
			sei.lpVerb       = L"open";
			sei.nShow        = SW_SHOWNORMAL;
			BOOL bSuccess;
			bSuccess = ShellExecuteEx( &sei );
		}

		if( pszAdditionalArg )
			delete[] pszAdditionalArg;
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		switch( LOWORD(wParam) )
		{
			case IDOK:
				OnOK(hDlg);
				break;
			case IDCLOSE:
			case IDCANCEL:
				EndDialog(hDlg,(INT_PTR)LOWORD(wParam));
				break;
		}
		return 0;
	}

	VOID SetStatusBox(LaunchBoxItem *pItem)
	{
		if( pItem == NULL )
		{
			SetDlgItemText(m_hWnd,IDC_EDIT1,L"");
			SetDlgItemText(m_hWnd,IDC_EDIT2,L"");
		}
		else
		{
			SetDlgItemText(m_hWnd,IDC_EDIT1,pItem->GetPath());
		}
	}

	virtual INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
				SetWindowLongPtr(hDlg,DWLP_MSGRESULT,OnNotify(hDlg,wParam,lParam));
				return 0;
		}
		return CLaunchPadPage::DlgProc(hDlg,uMsg,wParam,lParam);
	}
};
