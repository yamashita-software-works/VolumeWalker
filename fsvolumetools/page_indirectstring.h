#pragma once
//*****************************************************************************
//
//  page_indirectstring.h
//
//  PURPOSE: SHLoadIndirectString Function call.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-10-04 Created.
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//

class CIndirectStringPage : public CControlPane
{
	CUILayout m_Layout;

	HWND m_hwndInput;
	HWND m_hwndOutput;
	HWND m_hwndStatus;

	typedef struct {
		PCWSTR Title;
		int nUnitConvType;
	} COMBOBOXITEM;

public:
	DEFDIALOGRESID(IDD_TOOLPAGE_INDIRECT_STRING)

	CIndirectStringPage()
	{
	}

	~CIndirectStringPage()
	{
	}

	virtual VOID OnInitPane(PVOID) {}
	virtual VOID OnStartOperation(PVOID) {}
	virtual VOID OnEndOperation(PVOID) {}

	virtual INT_PTR DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( uMsg )
		{
			case WM_INITDIALOG:
			{
				m_hwndInput  = GetDlgItem(hWnd,IDC_EDIT1);
				m_hwndOutput = GetDlgItem(hWnd,IDC_EDIT2);
				m_hwndStatus = GetDlgItem(hWnd,IDC_EDIT3);
				m_Layout.Initialize(hWnd);			
				m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_TOPRIGHT,IDC_EDIT1,FALSE);
				m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_TOPRIGHT,IDC_EDIT2,FALSE);
				m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_TOPRIGHT,IDC_EDIT3,FALSE);
				break;
			}
			case WM_DESTROY:
			{
				break;
			}
			case WM_SIZE:
			{
				int cx = GET_X_LPARAM(lParam);
				int cy = GET_Y_LPARAM(lParam);
				m_Layout.AdjustControls(cx,cy);
				break;
			}
			case WM_COMMAND:
			{
				switch( LOWORD(wParam) )
				{
					case IDOK:
						OnUpdateOutput((HWND)lParam);
						return TRUE;
				}
			}
		}
		return CControlPane::DlgProc(hWnd,uMsg,wParam,lParam);
	}

private:
	VOID OnUpdateOutput(HWND hwndEdit)
	{
		WCHAR szBuf[MAX_PATH];
		WCHAR szOutput[MAX_PATH];
		WCHAR szStatus[MAX_PATH];

		if( GetWindowText(m_hwndInput,szBuf,MAX_PATH) > 0 )
		{
			szOutput[0] = 0;
			szStatus[0] = 0;

			HRESULT hr;
			if( (hr = SHLoadIndirectString(szBuf,szOutput,MAX_PATH,NULL)) != S_OK )
			{
				PWSTR pMessage = NULL;
				DWORD dwError;

				dwError = GetLastError();

				WinGetSystemErrorMessage(dwError != 0 ? dwError : hr,&pMessage);

				if( pMessage )
				{
					if( dwError != 0 )
						StringCchPrintf(szStatus,MAX_PATH,L"%s (Error: %u)",pMessage,dwError);
					else
						StringCchPrintf(szStatus,MAX_PATH,L"%s (Error: 0x%08X)",pMessage,hr);
				}
				else
				{
					StringCchPrintf(szStatus,MAX_PATH,L"Error: 0x%08X,%u",hr,dwError);
				}

				WinFreeErrorMessage(pMessage);
			}
			else
			{
				StringCchCopy(szStatus,MAX_PATH,L"Succeeded.");
			}

			SetWindowText(m_hwndOutput,szOutput);
			SetWindowText(m_hwndStatus,szStatus);
		}
		else
		{
			SetWindowText(m_hwndOutput,L"");
			SetWindowText(m_hwndStatus,L"");
		}
	}
};

