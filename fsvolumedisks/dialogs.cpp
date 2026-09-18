//****************************************************************************
//
//  dialogs.cpp
//
//  Implements dialog procedures.
//
//  Author: YAMASHITA Katsuhiro
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

//////////////////////////////////////////////////////////////////////////////
// Goto Sector/Cluster Dialog

typedef  struct _DISK_VOLUME_SECTOR_CLUSTER_LOCATION_DIALOG_PARAM
{
	HWND hwndOwner;
	DISK_VOLUME_SECTOR_CLUSTER_LOCATION dvscl;
} DISK_VOLUME_SECTOR_CLUSTER_LOCATION_DIALOG_PARAM;

#define MAX_ADDRESS_LENGTH 64

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

			EnableWindow(GetDlgItem(hDlg,IDOK),FALSE);

			WCHAR sz[MAX_ADDRESS_LENGTH];
			if( p->Flags & DVL_HEX )
				StringCchPrintf(sz,ARRAYSIZE(sz),L"0x%I64x",p->Location.QuadPart);
			else
				StringCchPrintf(sz,ARRAYSIZE(sz),L"%I64d",p->Location.QuadPart);
			
			SetDlgItemText(hDlg,IDC_EDIT,sz);

			return (INT_PTR)TRUE;
		}
		case WM_DESTROY:
		{
			break;
		}
		case WM_COMMAND:
		{
			if( HIWORD(wParam) == EN_UPDATE )
			{
				int cch = GetWindowTextLength(GetDlgItem(hDlg,IDC_EDIT));
				EnableWindow(GetDlgItem(hDlg,IDOK),cch != 0);
				break;
			}

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

				StrTrim(szBuffer,L" \t");

				LONGLONG val;
				if( StrToInt64ExW(szBuffer,STIF_SUPPORT_HEX,&val) )
				{
					p->Location.QuadPart = val;

					EndDialog(hDlg, LOWORD(wParam));
				}
				else
				{
					MessageBeep(-1);
				}
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
