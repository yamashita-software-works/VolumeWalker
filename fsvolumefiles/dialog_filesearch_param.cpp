//*****************************************************************************
//
//  dialog_filesearch_param.cpp
//
//  PURPOSE: Implements for file search parameters dialog.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-04-18 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 
#include "stdafx.h"
#include "libntwdk.h"
#include "ntnativeapi.h"
#include "ntobjecthelp.h"
#include "ntwin32helper.h"
#include "filelist_filesearch.h"
#include "resource.h"

namespace SearchParameterDialog
{
	static FS_SELECTED_FILELIST *pFileList = NULL;

	enum {
		ID_GROUP_WIN32_ATTRIBUTES = 1,
		ID_GROUP_OTHERS,
	};

	typedef struct _GROUP_ITEM
	{
		int idGroup;
		UINT idGroupTitle;
		PCWSTR Text;
	} GROUP_ITEM;

	HWND hwndDialog;
	HWND hwndEditName;
	HWND hwndEditPath;
	HWND hwndAttrList;
	HWND hwndDtpLastWriteFrom;
	HWND hwndDtpLastWriteTo;
	HWND hwndDtpCreationFrom;
	HWND hwndDtpCreationTo;
	HWND hwndDtpLastAccessFrom;
	HWND hwndDtpLastAccessTo;
	HWND hwndDtpChangeFrom;
	HWND hwndDtpChangeTo;
	HWND hwndEndOfFileFrom;
	HWND hwndEndOfFileTo;
	HWND hwndAllocationSizeFrom;
	HWND hwndAllocationSizeTo;
	HWND hwndMaxItemCound;

	SEARCH_PARAMETER *SearchParam;

	typedef struct _DEF_CTRLMAP {
		UINT mask;
		UINT checkBox;
		UINT CtrlId[2];
	} DEF_CTRLMAP;

	BOOL EnableCheckBoxWithCtrl(UINT checkBoxId)
	{
		DEF_CTRLMAP idmap[] = {
			{0, IDC_CHECK1,{IDC_LIST,IDC_CHECK8}},
			{0, IDC_CHECK2,{IDC_DATETIMEPICKER1,IDC_DATETIMEPICKER2}},
			{0, IDC_CHECK3,{IDC_DATETIMEPICKER3,IDC_DATETIMEPICKER4}},
			{0, IDC_CHECK4,{IDC_DATETIMEPICKER5,IDC_DATETIMEPICKER6}},
			{0, IDC_CHECK5,{IDC_DATETIMEPICKER7,IDC_DATETIMEPICKER8}},
			{0, IDC_CHECK6,{IDC_EDIT2,IDC_EDIT3}},
			{0, IDC_CHECK7,{IDC_EDIT4,IDC_EDIT5}},
		};
		for(int i = 0; i < _countof(idmap); i++)
		{
			if( idmap[i].checkBox == checkBoxId )
			{
				BOOL bEnable = (IsDlgButtonChecked(hwndDialog,idmap[i].checkBox) == BST_CHECKED);

				if( idmap[i].CtrlId[0] )
					EnableWindow( GetDlgItem(hwndDialog,idmap[i].CtrlId[0]), bEnable );

				if( idmap[i].CtrlId[1] )
					EnableWindow( GetDlgItem(hwndDialog,idmap[i].CtrlId[1]), bEnable );

				return TRUE;
			}
		}
		return FALSE;
	}

	inline VOID EnableOKbutton(HWND hwndDlg)
	{
		EnableWindow(GetDlgItem(hwndDlg,IDOK),(GetWindowTextLength(hwndEditName) != 0));
	}

	int InsertGroup(HWND hWndList,int iGroupId,LPCWSTR pszHeaderText,int iImage=I_IMAGENONE,BOOL fCollapsed=FALSE,LPCWSTR pszSubTitle=NULL)
	{
		LVGROUP group = {0};

		group.cbSize      = sizeof(LVGROUP);
		group.mask        = LVGF_GROUPID|LVGF_TITLEIMAGE|LVGF_HEADER|LVGF_STATE;
		group.iTitleImage = iImage;
		group.pszHeader   = (LPWSTR)pszHeaderText;
		group.uAlign      = LVGA_HEADER_LEFT;
		group.iGroupId    = iGroupId;
		group.state       = LVGS_COLLAPSIBLE | (fCollapsed ? LVGS_COLLAPSED : 0);

		if( pszSubTitle )
		{
			group.mask |= LVGF_SUBTITLE;
			group.pszSubtitle = (LPWSTR)pszSubTitle;
		}

		return (int)ListView_InsertGroup(hWndList,-1,(PLVGROUP)&group);
	}

	void InitGroup()
	{
		GROUP_ITEM Group[] = {
			{ ID_GROUP_WIN32_ATTRIBUTES, 0, L"Attributes" },
			{ ID_GROUP_OTHERS,           0, L"Others"  },
		};
		int cGroupItem = ARRAYSIZE(Group);

		for(int i = 0; i < cGroupItem; i++)
		{
			InsertGroup(hwndAttrList,Group[i].idGroup,Group[i].Text);
		}
	}

	int Insert(HWND hWndList,int iGroupId,PCWSTR pszText,LPARAM lParam=0,int iIndent=0)
	{
		int iItem = ListView_GetItemCount(hWndList);

		LVITEM lvi = {0};
		lvi.mask     = LVIF_TEXT|LVIF_INDENT|LVIF_PARAM|LVIF_GROUPID;
		lvi.iItem    = iItem;
		lvi.iImage   = I_IMAGENONE;
		lvi.iIndent  = iIndent;
		lvi.lParam   = (LPARAM)lParam;
		lvi.iGroupId = iGroupId;
		lvi.pszText  = (PWSTR)pszText;

		return ListView_InsertItem(hWndList,&lvi);
	}

#define _DEF_ATTRIB_FLAG(f,d) { f, L#f, d }

	VOID InitWin32Attributes()
	{
		struct {
			DWORD Flag;
			PWSTR FlagName;
			PCWSTR FlagDescription;
		} attrDef[] = {
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_READONLY,              L"ReadOnly"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_HIDDEN,                L"Hidden"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_SYSTEM,                L"System"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_DIRECTORY,             L"Directory"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_ARCHIVE,               L"Archive"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_DEVICE,                L"Device"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_NORMAL,                L"Normal"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_TEMPORARY,             L"Temporary"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_SPARSE_FILE,           L"Sparse File"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_REPARSE_POINT,         L"Reparse Point"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_COMPRESSED,            L"Compressed"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_OFFLINE,               L"Offline"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,   L"Noy Content Indexed"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_ENCRYPTED,             L"Encrypted"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_VIRTUAL,               L"Virtual"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_NO_SCRUB_DATA,         L"No Scrub Data"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_INTEGRITY_STREAM,      L"Integrity Stream"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_EA,                    L"EA"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_PINNED,                L"Pinned"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_UNPINNED,              L"Uppinned"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_RECALL_ON_OPEN,        L"Recall on Open"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS, L"Recall on Data Access"),
			_DEF_ATTRIB_FLAG( FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL,   L"Strictly Sequential"),
		};

		for( int i = 0; i < _countof(attrDef); i++)
		{
			WCHAR sz[MAX_PATH];
			StringCchPrintf(sz,MAX_PATH,L"%s",attrDef[i].FlagName);

			Insert(hwndAttrList,ID_GROUP_WIN32_ATTRIBUTES,sz,attrDef[i].Flag);
		}
	}

	VOID SetFileAttributesCheckBox(DWORD dwAttribute,BOOL bCheck=TRUE)
	{
		int iItem,cItems;
		cItems = ListView_GetItemCount(hwndAttrList);

		for(iItem = 0; iItem < cItems; iItem++)
		{
			if( ListViewEx_GetItemData(hwndAttrList,iItem) & dwAttribute )
			{
				ListViewEx_SetCheckState(hwndAttrList,iItem,bCheck);
				break;
			}
		}
	}

	VOID SetFileAttributesList(SEARCH_PARAMETER *SearchParam)
	{
		int i;
		for(i = 0; i < 32; i++)
		{
			if( SearchParam->FileAttributes & (0x1 << i) )
			{
				SetFileAttributesCheckBox((DWORD)(0x1 << i));
			}
		}
	}

	VOID InitAttributeList(HWND hDlg)
	{
		_EnableVisualThemeStyle(hwndAttrList);

		ListView_SetExtendedListViewStyle(hwndAttrList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT|LVS_EX_JUSTIFYCOLUMNS|LVS_EX_AUTOSIZECOLUMNS|LVS_EX_CHECKBOXES);

		LVCOLUMN lvc = {0};
		lvc.mask    = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
		lvc.fmt     = LVCFMT_LEFT;
		lvc.cx      = 32767;
		lvc.pszText = L"";
		lvc.iOrder  = 0;
		ListView_InsertColumn(hwndAttrList,0,&lvc);

		InitGroup();

		InitWin32Attributes();
	}

	VOID InitDateTimePickerFormat(HWND hwndDTPFrom,HWND hwndDTPTo)
	{
		WCHAR sz[256];
		WCHAR szDateFmt[64];
		WCHAR szTimeFmt[64];
		GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SSHORTDATE,szDateFmt,_countof(szDateFmt));
		GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_STIMEFORMAT,szTimeFmt,_countof(szTimeFmt));
		wnsprintf(sz,_countof(sz),_T("%s %s"),szDateFmt,szTimeFmt);
		DateTime_SetFormat(hwndDTPFrom,sz);
		DateTime_SetFormat(hwndDTPTo,sz);
	}

	VOID SetDateTimePicker(HWND hwndDTPFrom,HWND hwndDTPTo,LARGE_INTEGER *pliFrom,LARGE_INTEGER *pliTo)
	{
		SYSTEMTIME st;

		TimeIntegerToLocalSystemTime(pliFrom,&st);
		DateTime_SetSystemtime(hwndDTPFrom,GDT_VALID,&st);

		TimeIntegerToLocalSystemTime(pliTo,&st);
		DateTime_SetSystemtime(hwndDTPTo,GDT_VALID,&st);
	}

	VOID InitDateTimePickers(SEARCH_PARAMETER *SearchParam)
	{
		InitDateTimePickerFormat(hwndDtpLastWriteFrom,hwndDtpLastWriteTo);
		InitDateTimePickerFormat(hwndDtpCreationFrom,hwndDtpCreationTo);
		InitDateTimePickerFormat(hwndDtpLastAccessFrom,hwndDtpLastAccessTo);
		InitDateTimePickerFormat(hwndDtpChangeFrom,hwndDtpChangeTo);

 		SetDateTimePicker(hwndDtpLastWriteFrom,hwndDtpLastWriteTo,
				&SearchParam->DateTime.LastWrite.From,&SearchParam->DateTime.LastWrite.To);

		SetDateTimePicker(hwndDtpCreationFrom,hwndDtpCreationTo,
				&SearchParam->DateTime.Creation.From,&SearchParam->DateTime.Creation.To);

		SetDateTimePicker(hwndDtpLastAccessFrom,hwndDtpLastAccessTo,
				&SearchParam->DateTime.LastAccess.From,&SearchParam->DateTime.LastAccess.To);

		SetDateTimePicker(hwndDtpChangeFrom,hwndDtpChangeTo,
				&SearchParam->DateTime.Change.From,&SearchParam->DateTime.Change.To);
	}

	VOID SetSizeText(HWND hwndFrom,HWND hwndTo,LARGE_INTEGER *pliFrom,LARGE_INTEGER *pliTo)
	{
		WCHAR sz[64];

		StringCchPrintf(sz,ARRAYSIZE(sz),L"%I64d",pliFrom->QuadPart);
		SetWindowText(hwndFrom,sz);

		StringCchPrintf(sz,ARRAYSIZE(sz),L"%I64d",pliTo->QuadPart);
		SetWindowText(hwndTo,sz);
	}

	VOID InitFileSize(SEARCH_PARAMETER *SearchParam)
	{
		SetSizeText(hwndEndOfFileFrom,hwndEndOfFileTo,&SearchParam->EndOfFile.From,&SearchParam->EndOfFile.To);
		SetSizeText(hwndAllocationSizeFrom,hwndAllocationSizeTo,&SearchParam->EndOfFile.From,&SearchParam->EndOfFile.To);
	}

	VOID InitMaxFoundItemCount(SEARCH_PARAMETER *SearchParam)
	{
		WCHAR sz[64];

		StringCchPrintf(sz,ARRAYSIZE(sz),L"%u",SearchParam->MaxFoundItemCount);
		SetWindowText(hwndMaxItemCound,sz);
	}

	struct {
		UINT id;
		SEARCH_FLAGS Flags;
	} chkbox[] = {
		// LowPart,HighPart
		{IDC_CHECK1,SEARCH_FLAG_ATTRIBUTE},
		{IDC_CHECK2,SEARCH_FLAG_DATETIME_LASTWRITE},
		{IDC_CHECK3,SEARCH_FLAG_DATETIME_CREATION},
		{IDC_CHECK4,SEARCH_FLAG_DATETIME_LASTACCESS},
		{IDC_CHECK5,SEARCH_FLAG_DATETIME_CHANGE},
		{IDC_CHECK6,SEARCH_FLAG_ENDOFFILE},
		{IDC_CHECK7,SEARCH_FLAG_ALLOCATIONSIZE},
		{IDC_CHECK8,SEARCH_FLAG_ATTR_MATCH_WHOLE_BITS},
		{IDC_CHECK9,SEARCH_FLAG_ALT_STREAM},
	};

	VOID SetCompareFlag(SEARCH_FLAGS CompareFlags)
	{
		DWORD dwFlags = 0;
		for(int i = 0; i < _countof(chkbox); i++)
		{
			if( chkbox[i].Flags & CompareFlags )
			{
				CheckDlgButton(hwndDialog,chkbox[i].id,BST_CHECKED);
			}
		}
	}

	DWORD GetCompareFlag()
	{
		DWORD dwFlags = 0;
		for(int i = 0; i < _countof(chkbox); i++)
		{
			BOOL bEnable = (IsDlgButtonChecked(hwndDialog,chkbox[i].id) == BST_CHECKED);
			if( bEnable )
			{
				dwFlags |= chkbox[i].Flags;
			}
		}

		return dwFlags;
	}

	INT_PTR OnInitDialog(HWND hDlg,UINT,WPARAM,LPARAM lParam)
	{
		_CenterWindow(hDlg,GetActiveWindow());

		hwndDialog = hDlg;

		SearchParam = (SEARCH_PARAMETER *)lParam;

		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)SearchParam);

		hwndEditPath           = GetDlgItem(hDlg,IDC_EDIT6); // reserved
		hwndEditName           = GetDlgItem(hDlg,IDC_EDIT1);
		hwndAttrList           = GetDlgItem(hDlg,IDC_LIST);
		hwndDtpLastWriteFrom   = GetDlgItem(hDlg,IDC_DATETIMEPICKER1);
		hwndDtpLastWriteTo     = GetDlgItem(hDlg,IDC_DATETIMEPICKER2);
		hwndDtpCreationFrom    = GetDlgItem(hDlg,IDC_DATETIMEPICKER3);
		hwndDtpCreationTo      = GetDlgItem(hDlg,IDC_DATETIMEPICKER4);
		hwndDtpLastAccessFrom  = GetDlgItem(hDlg,IDC_DATETIMEPICKER5);
		hwndDtpLastAccessTo    = GetDlgItem(hDlg,IDC_DATETIMEPICKER6);
		hwndDtpChangeFrom      = GetDlgItem(hDlg,IDC_DATETIMEPICKER7);
		hwndDtpChangeTo        = GetDlgItem(hDlg,IDC_DATETIMEPICKER8);
		hwndEndOfFileFrom      = GetDlgItem(hDlg,IDC_EDIT2);
		hwndEndOfFileTo        = GetDlgItem(hDlg,IDC_EDIT3);
		hwndAllocationSizeFrom = GetDlgItem(hDlg,IDC_EDIT4);
		hwndAllocationSizeTo   = GetDlgItem(hDlg,IDC_EDIT5);
		hwndMaxItemCound       = GetDlgItem(hDlg,IDC_EDIT7);

		InitAttributeList(hDlg);
		SetFileAttributesList(SearchParam);

		InitDateTimePickers(SearchParam);

		InitFileSize(SearchParam);

		InitMaxFoundItemCount(SearchParam);

		SetCompareFlag(SearchParam->CompareFlag);

		UINT id[] = {IDC_CHECK1,IDC_CHECK2,IDC_CHECK3,IDC_CHECK4,IDC_CHECK5,IDC_CHECK6,IDC_CHECK7};
		for( int i = 0; i < _countof(id); i++ )
		{
			EnableCheckBoxWithCtrl(id[i]);
		}

		if( SearchParam->Name[0] )
			SetWindowText(hwndEditName,SearchParam->Name);

		// Search target path
		SetWindowText(hwndEditPath,pFileList->FileListBuffer->File[0].Name);

		EnableOKbutton(hDlg);

		return TRUE;
	}

	DWORD GetFileAttributeFlags()
	{
		DWORD dwFileAttributes = 0;
		int i,cItems;

		cItems = ListView_GetItemCount(hwndAttrList);
		for(i = 0; i < cItems; i++)
		{
			if( ListViewEx_GetCheckState(hwndAttrList,i) )
			{
				dwFileAttributes |= ListViewEx_GetItemData(hwndAttrList,i);
			}
		}

		return dwFileAttributes;
	}

	BOOL GetSizeValue(HWND hwndFrom,HWND hwndTo,SEARCH_RANGE_VALUE *Size)
	{
		WCHAR szBuf[100];

		GetWindowText(hwndFrom,szBuf,ARRAYSIZE(szBuf));
		if( StrToInt64Ex(szBuf,STIF_SUPPORT_HEX,NULL) )
		{
			StrToInt64Ex(szBuf,STIF_SUPPORT_HEX,&Size->From.QuadPart);
		}
		else
		{
			return FALSE;
		}

		GetWindowText(hwndTo,szBuf,ARRAYSIZE(szBuf));
		if( StrToInt64Ex(szBuf,STIF_SUPPORT_HEX,NULL) )
		{
			StrToInt64Ex(szBuf,STIF_SUPPORT_HEX,&Size->To.QuadPart);
		}
		else
		{
			return FALSE;
		}

		return TRUE;
	}

	BOOL GetDateTime(HWND hwndFrom,HWND hwndTo,SEARCH_RANGE_VALUE *DateTime)
	{
		SYSTEMTIME st;

		DateTime_GetSystemtime(hwndFrom,&st);
		LocalSystemTimeToTimeInteger(&st,&DateTime->From);

		DateTime_GetSystemtime(hwndTo,&st);
		LocalSystemTimeToTimeInteger(&st,&DateTime->To);

		return TRUE;
	}

	BOOL GetMaxFoundItemCount(HWND hwndEdit,ULONG *pulCount)
	{
		WCHAR szBuf[100];
		int n;

		GetWindowText(hwndEdit,szBuf,ARRAYSIZE(szBuf));

		if( StrToIntEx(szBuf,STIF_SUPPORT_HEX,&n) )
		{
			*pulCount = n;
			return TRUE;
		}

		return FALSE;
	}

	VOID OnOK(HWND hwndDlg)
	{
		SEARCH_PARAMETER *psp = (SEARCH_PARAMETER *)GetWindowLongPtr(hwndDlg,DWLP_USER);

		GetWindowText(hwndEditName,psp->Name,ARRAYSIZE(psp->Name));

		psp->FileAttributes = GetFileAttributeFlags();

		GetSizeValue(hwndEndOfFileFrom,hwndEndOfFileTo,&psp->EndOfFile);
		GetSizeValue(hwndAllocationSizeFrom,hwndAllocationSizeTo,&psp->AllocateSize);

		GetDateTime(hwndDtpLastWriteFrom,hwndDtpLastWriteTo,&psp->DateTime.LastWrite);
		GetDateTime(hwndDtpCreationFrom,hwndDtpCreationTo,&psp->DateTime.Creation);
		GetDateTime(hwndDtpLastAccessFrom,hwndDtpLastAccessTo,&psp->DateTime.LastAccess);
		GetDateTime(hwndDtpChangeFrom,hwndDtpChangeTo,&psp->DateTime.Change);

		psp->CompareFlag = GetCompareFlag();

		GetMaxFoundItemCount(hwndMaxItemCound,&psp->MaxFoundItemCount);

		EndDialog(hwndDlg,IDOK);
	}

	INT_PTR OnCommand(HWND hDlg,UINT,WPARAM wParam,LPARAM lParam)
	{
		if( HIWORD(wParam) == BN_CLICKED )
		{
			if( EnableCheckBoxWithCtrl(LOWORD(wParam)) )
				return 0;
		}

		if( HIWORD(wParam) == EN_CHANGE && (HWND)lParam == hwndEditName )
		{
			EnableOKbutton(hDlg);
			return 0;
		}

		switch( LOWORD(wParam) )
		{
			case IDOK:
				OnOK(hDlg);
				break;
			case IDCANCEL:
				EndDialog(hDlg,LOWORD(wParam));
				break;
		}
		return 0;
	}

	INT_PTR
	CALLBACK
	DlgProc(
		HWND hwndDlg,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam
		)
	{
		switch( uMsg )
		{
			case WM_INITDIALOG:
				return OnInitDialog(hwndDlg,uMsg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hwndDlg,uMsg,wParam,lParam);
			case WM_CLOSE:
				EndDialog(hwndDlg,IDCLOSE);
				break;
		}
		return 0;
	}
};

//-----------------------------------------------------------------------------
//
//  SearchParamDialog()
//
//  PURPOSE: File Search Parameter Input Dialog.
//
//-----------------------------------------------------------------------------
HRESULT SearchParamDialog(HWND hWnd,FS_SELECTED_FILELIST *FileList,SEARCH_PARAMETER *SearchParam)
{
	if( FileList == NULL || SearchParam == NULL )
		return E_INVALIDARG;

	if( FileList->FileListBuffer == NULL )
		return E_INVALIDARG;

	SearchParameterDialog::pFileList = FileList;

	INT_PTR iRet;
	iRet = DialogBoxParam(_GetResourceInstance(),MAKEINTRESOURCE(IDD_SEARCH_PARAMETER),
							hWnd,SearchParameterDialog::DlgProc,(LPARAM)SearchParam);

	if (iRet == -1)
	{
    	return HRESULT_FROM_WIN32(GetLastError());
	}

	return (iRet == IDOK) ? S_OK : S_FALSE;
}
