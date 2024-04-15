//****************************************************************************
//
//  dialogs.cpp
//
//  Implements dialog procedures.
//
//  Auther: YAMASHITA Katsuhiro
//
//  Create: 2024.04.05
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "resource.h"
#include "dialogs.h"

typedef  struct _DISK_VOLUME_SECTOR_CLUSTER_LOCATION_DIALOG_PARAM
{
	HWND hwndOwner;
	DISK_VOLUME_SECTOR_CLUSTER_LOCATION dvscl;
} DISK_VOLUME_SECTOR_CLUSTER_LOCATION_DIALOG_PARAM;

/////////////////////////////////////////////////////////////////////////////////
// Sector/Cluster 

#define MAX_ADDRESS_LENGTH 64

LRESULT
CALLBACK
EditSubclassWindow(
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

			WCHAR sz[64];
			GetWindowText(hWnd,sz,ARRAYSIZE(sz));
			int cch = (int)wcslen(sz);

			if( cch == 1 && (ch == 'x' || ch == 'X') )
				break;

			if( isxdigit(ch) )
				break;

			if( ch == VK_BACK )
				break;

			return 0;
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static INT_PTR CALLBACK GotoLocationDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			SetWindowLongPtr(hDlg,DWLP_USER,lParam);
			DISK_VOLUME_SECTOR_CLUSTER_LOCATION_DIALOG_PARAM *param = (DISK_VOLUME_SECTOR_CLUSTER_LOCATION_DIALOG_PARAM *)lParam;
			DISK_VOLUME_SECTOR_CLUSTER_LOCATION *p = &param->dvscl;

			_CenterWindow(hDlg,param->hwndOwner);

			SetWindowText(hDlg,(p->Flags & DVL_LOC_PHYSICAL_DISK) ? L"Sector" : L"Cluster");

			SetDlgItemText(hDlg,IDC_RADIO1,(p->Flags & DVL_LOC_PHYSICAL_DISK) ? L"Sector &Number" : L"LC&N");

			CheckRadioButton(hDlg,IDC_RADIO1,IDC_RADIO2,(p->Flags & DVL_LOC_SECTOR_NUMBER ) ? IDC_RADIO1 : IDC_RADIO2);

			SendDlgItemMessage(hDlg,IDC_EDIT,EM_SETLIMITTEXT,MAX_ADDRESS_LENGTH,0);

			WCHAR sz[MAX_ADDRESS_LENGTH];
			if( p->Flags & DVL_HEX )
				StringCchPrintf(sz,ARRAYSIZE(sz),L"0x%I64x",p->Location.QuadPart);
			else
				StringCchPrintf(sz,ARRAYSIZE(sz),L"%I64d",p->Location.QuadPart);
			
			SetDlgItemText(hDlg,IDC_EDIT,sz);

			SetWindowSubclass(GetDlgItem(hDlg,IDC_EDIT),&EditSubclassWindow,0x1,0);

			return (INT_PTR)TRUE;
		}
		case WM_DESTROY:
		{
			RemoveWindowSubclass(GetDlgItem(hDlg,IDC_EDIT),&EditSubclassWindow,0x1);
			break;
		}
		case WM_COMMAND:
		{
			if( LOWORD(wParam) == IDOK )
			{
				DISK_VOLUME_SECTOR_CLUSTER_LOCATION_DIALOG_PARAM *param = (DISK_VOLUME_SECTOR_CLUSTER_LOCATION_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);
				DISK_VOLUME_SECTOR_CLUSTER_LOCATION *p = &param->dvscl;

				p->Flags &= ~DVL_LOC_NUMBER_MASK;
				if( IsDlgButtonChecked(hDlg,IDC_RADIO1) == BST_CHECKED )
					p->Flags |= DVL_LOC_SECTOR_NUMBER;

				int cchBuffer = MAX_ADDRESS_LENGTH;
				WCHAR szBuffer[MAX_ADDRESS_LENGTH];
				GetDlgItemText(hDlg,IDC_EDIT,szBuffer,cchBuffer);

				LONGLONG val;
				StrToInt64ExW(szBuffer,STIF_SUPPORT_HEX,&val);

				p->Location.QuadPart = val;

				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			if( LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDCLOSE  )
			{
				EndDialog(hDlg, LOWORD(wParam));
			}
			break;
		}
	}

	return (INT_PTR)FALSE;
}

HRESULT GotoDialog(HWND hWnd,PDISK_VOLUME_SECTOR_CLUSTER_LOCATION pLoc)
{
	HRESULT hr = E_FAIL;

	if( pLoc == NULL )
		return E_INVALIDARG;

	DISK_VOLUME_SECTOR_CLUSTER_LOCATION_DIALOG_PARAM loc = {0};
	loc.hwndOwner = hWnd;
	loc.dvscl.Flags = pLoc->Flags;
	loc.dvscl.Location = pLoc->Location;
	loc.dvscl.SectorClusterSize = pLoc->SectorClusterSize;

	if( DialogBoxParamW(_GetResourceInstance(), MAKEINTRESOURCE(IDD_GOTO_SECTOR),
			hWnd, &GotoLocationDlgProc,(LPARAM)&loc) == IDOK )
	{
		pLoc->Location.QuadPart = loc.dvscl.Location.QuadPart;
		pLoc->Flags = loc.dvscl.Flags;
		hr = S_OK;
	}
	else
	{
		hr = S_FALSE;
	}

	return hr;
}
