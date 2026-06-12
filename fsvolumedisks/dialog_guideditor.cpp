//*****************************************************************************
//
//  dialog_guideditor.cpp
//
//  PURPOSE: GUID editor dialog.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2025-01-31 Created
//
//*****************************************************************************
#include "stdafx.h"
#include "fsvolumelist.h"
#include "resource.h"
#include "basewindow.h"
#include "uilayout.h"
#include "libntwdk.h"
#include "ntnativehelp.h"
#include "ntvolumenames.h"
#include "ntobjecthelp.h"
#include "ntwin32helper.h"
#include "..\fsfilelib\fsfilelib.h"

typedef struct _GUIDEDIT_DIALOG_PARAM
{
	HWND hWnd;
	GUID Guid;
	GUIDEDITPARAM GuidEditParam;
	DWORD dwFlags;
	WCHAR szGuidString[40];
	WCHAR szVolumeName[MAX_PATH];
} GUIDEDIT_DIALOG_PARAM;

static
LRESULT
CALLBACK
GuidHexEditBoxSubclassProc(
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
		case WM_CHAR:
		{
			INT ch = (INT)wParam;

			if( (ch & ~0xff) != 0 )
				return 0;

			if( isxdigit(ch) )
				break;

			if( ch == VK_BACK )
				break;

			return 0;
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

struct CGUIDEditorDialog : public CDialogWindow
{
	HWND m_hwndEditGuid1;
	HWND m_hwndEditGuid2;
	HWND m_hwndEditGuid3;
	HWND m_hwndEditGuid4;
	HWND m_hwndEditGuid5;

	HFONT m_hFont;

	CGUIDEditorDialog()
	{
		m_hFont = NULL;
	}
	
	~CGUIDEditorDialog()
	{
	}

	void SetEditText(HWND hwndEdit,PCWSTR pszText)
	{
		SendMessage(hwndEdit,EM_SETSEL,(WPARAM)0,(LPARAM)-1);
		SendMessage(hwndEdit,EM_REPLACESEL,0,(LPARAM)pszText);
	}

	PWSTR mid(PCWSTR str,int iFirst,int nCount,PWSTR ret,int cch)
	{
		if( StringCchCopyNW(ret,cch,&str[iFirst],nCount) != S_OK )
			return NULL;
		return ret;
	}

	void SetGuidValume(GUID& Guid)
	{
		WCHAR sz[40];
		StringFromGUID(&Guid,sz,ARRAYSIZE(sz));

		// 0         1    1    2    2    3       3
		// 012345678901234567890123456789012345678
		// {00000000-0000-0000-0000-000000000000}
		WCHAR buf[16];
		mid(sz,1,8,buf,ARRAYSIZE(buf));
		SetEditText(m_hwndEditGuid1,buf);

		mid(sz,10,4,buf,ARRAYSIZE(buf));
		SetEditText(m_hwndEditGuid2,buf);

		mid(sz,15,4,buf,ARRAYSIZE(buf));
		SetEditText(m_hwndEditGuid3,buf);

		mid(sz,20,4,buf,ARRAYSIZE(buf));
		SetEditText(m_hwndEditGuid4,buf);

		mid(sz,25,12,buf,ARRAYSIZE(buf));
		SetEditText(m_hwndEditGuid5,buf);
	}

	BOOL GetGUIDStringFromEditBox(PWSTR pszGuid,int cchGuid)
	{
		WCHAR szGuid1[8+1];
		WCHAR szGuid2[4+1];
		WCHAR szGuid3[4+1];
		WCHAR szGuid4[4+1];
		WCHAR szGuid5[12+1];
		BOOL bResult;

		if( GetWindowText(m_hwndEditGuid1,szGuid1,ARRAYSIZE(szGuid1)) &&
			GetWindowText(m_hwndEditGuid2,szGuid2,ARRAYSIZE(szGuid2)) &&
			GetWindowText(m_hwndEditGuid3,szGuid3,ARRAYSIZE(szGuid3)) &&
			GetWindowText(m_hwndEditGuid4,szGuid4,ARRAYSIZE(szGuid4)) &&
			GetWindowText(m_hwndEditGuid5,szGuid5,ARRAYSIZE(szGuid5)) )
		{
			StringCchPrintf(pszGuid,cchGuid,L"{%s-%s-%s-%s-%s}",
				szGuid1,szGuid2,szGuid3,szGuid4,szGuid5);
			bResult = TRUE;
		}
		else
		{
			bResult = FALSE;
		}
		return bResult;
	}

	ULONG GetHexFromEditBox(HWND hwndEdit)
	{
		WCHAR sz[8+1];
		if( GetWindowText(hwndEdit,sz,ARRAYSIZE(sz)) )
		{
			return wcstoul(sz,NULL,16);
		}
		return 0;
	}

	LONGLONG GetHex64FromEditBox(HWND hwndEdit1,HWND hwndEdit2)
	{
		WCHAR sz1[4+12+1];
		WCHAR sz2[12+1];
		if( GetWindowText(hwndEdit1,sz1,ARRAYSIZE(sz1)) != 0 )
		{
			if( GetWindowText(hwndEdit2,sz2,ARRAYSIZE(sz2)) != 0 )
			{
				StringCchCat(sz1,ARRAYSIZE(sz1),sz2);
				return _wcstoui64(sz1,NULL,16);
			}
		}
		return 0;
	}

	VOID GetGUIDFromEditBox(GUID& Guid)
	{
		Guid.Data1 = GetHexFromEditBox(m_hwndEditGuid1);
		Guid.Data2 = (USHORT)GetHexFromEditBox(m_hwndEditGuid2);
		Guid.Data3 = (USHORT)GetHexFromEditBox(m_hwndEditGuid3);

		LONGLONG n;
		n = GetHex64FromEditBox(m_hwndEditGuid4,m_hwndEditGuid5);

		UCHAR *s = ((UCHAR *)&n);
		UCHAR *d = ((UCHAR *)&Guid.Data4);
		for(int i = 0; i < 8; i++)
		{
			d[i] = s[7-i];
		}
	}

	HWND InitEditBox(UINT idEdit,int cchMax,int id)
	{
		HWND hwndEdit = GetDlgItem(m_hWnd,idEdit);
		if( hwndEdit == NULL )
			return NULL;

		SendMessage(hwndEdit,EM_LIMITTEXT,cchMax,0);

		SetWindowSubclass(hwndEdit,&GuidHexEditBoxSubclassProc,id,0);

		if( m_hFont )
			SendMessage(hwndEdit, WM_SETFONT, (WPARAM)m_hFont, 0); 

		return hwndEdit;
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		GUIDEDIT_DIALOG_PARAM *pdlgParam = (GUIDEDIT_DIALOG_PARAM *)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		_CenterWindow(hDlg,GetActiveWindow());

		m_hFont = GetGlobalFont(hDlg);

		m_hwndEditGuid1 = InitEditBox(IDC_EDIT1, 8, 1);
		m_hwndEditGuid2 = InitEditBox(IDC_EDIT2, 4, 1);
		m_hwndEditGuid3 = InitEditBox(IDC_EDIT3, 4, 1);
		m_hwndEditGuid4 = InitEditBox(IDC_EDIT4, 4, 1);
		m_hwndEditGuid5 = InitEditBox(IDC_EDIT5,12, 1);

		EnableWindow(GetDlgItem(hDlg,IDOK),FALSE);

		if( (pdlgParam->dwFlags & GUIDEF_NO_INITIAL_EDITBOX) == 0 )
		{
			GUID guid = pdlgParam->Guid;
			SetGuidValume( guid );
		}
		else
		{
			EnableWindow( GetDlgItem(hDlg,IDC_RESET), FALSE );
		}

		if( pdlgParam->GuidEditParam.pszMainInstruction )
			SetDlgItemText(hDlg,IDC_TEXT,pdlgParam->GuidEditParam.pszMainInstruction);

		if( pdlgParam->GuidEditParam.pszWindowTitle )
			SetWindowText(hDlg,pdlgParam->GuidEditParam.pszWindowTitle);

		return FALSE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		RemoveWindowSubclass(m_hwndEditGuid1,&GuidHexEditBoxSubclassProc,1);
		RemoveWindowSubclass(m_hwndEditGuid2,&GuidHexEditBoxSubclassProc,1);
		RemoveWindowSubclass(m_hwndEditGuid3,&GuidHexEditBoxSubclassProc,1);
		RemoveWindowSubclass(m_hwndEditGuid4,&GuidHexEditBoxSubclassProc,1);
		RemoveWindowSubclass(m_hwndEditGuid5,&GuidHexEditBoxSubclassProc,1);
		DeleteObject( m_hFont );
		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		EndDialog(hDlg,IDCLOSE);
		return 0;
	}
	
public:
	VOID OnOK(HWND hDlg)
	{
		GUIDEDIT_DIALOG_PARAM *pdlgParam = (GUIDEDIT_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		{
			WCHAR sz[48];
			if( GetGUIDStringFromEditBox(sz,ARRAYSIZE(sz)) )
			{
				GUID guid;
				if( GUIDFromString(sz,&guid) == STATUS_SUCCESS )
				{
					pdlgParam->Guid = guid;
					StringCchCopy(pdlgParam->szGuidString,ARRAYSIZE(pdlgParam->szGuidString),sz);
					EndDialog(hDlg,IDOK);
					return ;
				}
			}
		}

		MessageBeep(-1);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		GUIDEDIT_DIALOG_PARAM *pdlgParam = (GUIDEDIT_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		if( HIWORD(wParam) == EN_UPDATE )
		{
			switch( LOWORD(wParam) )
			{
				case IDC_EDIT1:
				case IDC_EDIT2:
				case IDC_EDIT3:
				case IDC_EDIT4:
				case IDC_EDIT5:
				{
					if( GetWindowTextLength(m_hwndEditGuid1) == 8 && GetWindowTextLength(m_hwndEditGuid2) == 4 &&
						GetWindowTextLength(m_hwndEditGuid3) == 4 && GetWindowTextLength(m_hwndEditGuid4) == 4 &&
						GetWindowTextLength(m_hwndEditGuid5) == 12 )
					{
						EnableWindow( GetDlgItem(hDlg,IDOK), TRUE );

						GUID Guid;
						GetGUIDFromEditBox(Guid);
						if( (pdlgParam->dwFlags & GUIDEF_NO_INITIAL_EDITBOX) == 0 )
							EnableWindow( GetDlgItem(hDlg,IDC_RESET), IsEqualGUID(Guid,pdlgParam->Guid) ? FALSE : TRUE );
					}
					else
					{
						EnableWindow( GetDlgItem(hDlg,IDOK), FALSE );
						if( (pdlgParam->dwFlags & GUIDEF_NO_INITIAL_EDITBOX) == 0 )
							EnableWindow( GetDlgItem(hDlg,IDC_RESET), TRUE );
					}
					break;
				}
			}
		}

		if( HIWORD(wParam) == EN_SETFOCUS )
		{
			SendMessage((HWND)lParam,EM_SETSEL,(WPARAM)-1,(LPARAM)-1);
		}

		switch( LOWORD(wParam) )
		{
			case IDOK:
				OnOK(hDlg);
				break;
			case IDCLOSE:
			case IDCANCEL:
			{
				EndDialog(hDlg,(INT_PTR)LOWORD(wParam));
				break;
			}
			case IDC_RESET:
			{
				SetGuidValume( pdlgParam->Guid );
				break;
			}
			case IDC_CREATE:
			{
				GUID Guid;
				if( CoCreateGuid(&Guid) == S_OK )
				{
					SetGuidValume( Guid );
				}
				break;
			}
		}
		return 0;
	}

	//---------------------------------------------------------------------------
	//
	//  DlgProc()
	//
	//  PURPOSE:
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
			case WM_CLOSE:
				return OnClose(hDlg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hDlg,wParam,lParam);
		}
		return 0;
	}
};

//---------------------------------------------------------------------------
//
//  GUIDEditDialog()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
GUIDEditDialog(
	__in HWND hWnd,
	__in DWORD dwFlags,
	__inout GUID *pGuid,
	__inout_opt PWSTR pszReturnGuidStringBuffer,
	__in_opt int cchReturnGuidStringBuffer,
	__in_opt GUIDEDITPARAM *Param
	)
{
	HRESULT hr;
	GUIDEDIT_DIALOG_PARAM *pParam = new GUIDEDIT_DIALOG_PARAM;
	if( pParam == NULL )
		return E_OUTOFMEMORY;

	ZeroMemory(pParam,sizeof(GUIDEDIT_DIALOG_PARAM));

	if( pGuid )
	{
		pParam->Guid = *pGuid;
	}

	if( Param )
	{
		pParam->GuidEditParam = *Param;
	}

	pParam->dwFlags = dwFlags;

	if( pszReturnGuidStringBuffer )
	{
		hr = StringCchCopy(pParam->szGuidString,ARRAYSIZE(pParam->szGuidString),pszReturnGuidStringBuffer);

		if( hr == S_OK )
		{
			hr = HRESULT_FROM_NT( GUIDFromString( pParam->szGuidString, &pParam->Guid ) );
		}

		if( hr != S_OK )
		{
			delete pParam;
			return hr;
		}
	}

	//
	// Show dialog box
	//
	CGUIDEditorDialog *dlg = new CGUIDEditorDialog;

	if( dlg )
	{
		if( dlg->DoModal(hWnd,IDD_GUID_EDITOR,(LPARAM)pParam,_GetResourceInstance()) == IDOK )
		{
			if( pGuid )
			{
				*pGuid = pParam->Guid;
			}

			if( pszReturnGuidStringBuffer )
			{
				StringCchCopy(pszReturnGuidStringBuffer,cchReturnGuidStringBuffer,pParam->szGuidString);
			}
			hr = S_OK;
		}
		else
		{
			hr = S_FALSE;
		}

		delete dlg;
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	delete pParam;

	return hr;
}
