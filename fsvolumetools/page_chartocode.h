#pragma once
//*****************************************************************************
//
//  page_chartocode.h
//
//  PURPOSE: Character to Character Code Converter ToolPad Page
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-03-26 Created.
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//

class CCharacterToCodeToolPage : public CControlPane
{
	CUILayout m_Layout;

	HWND m_hwndText;
	HWND m_hwndDump;
	HWND m_hwndCombo;
public:
	DEFDIALOGRESID(IDD_TOOLPAGE_CHARACTER_TO_CODE)

	CCharacterToCodeToolPage()
	{
		m_hwndDump = NULL;
	}

	~CCharacterToCodeToolPage()
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
				m_Layout.Initialize(hWnd);			
				m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_TOPRIGHT,IDC_EDIT1,FALSE);
				m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_BOTTOMRIGHT,IDC_EDIT2,FALSE);
				m_Layout.AnchorControl(CUILayout::AP_TOPRIGHT,CUILayout::AP_TOPRIGHT,IDC_COMBO,FALSE);

				m_hwndText = GetDlgItem(m_hWnd,IDC_EDIT1);
				m_hwndDump = GetDlgItem(m_hWnd,IDC_EDIT2);
				m_hwndCombo = GetDlgItem(m_hWnd,IDC_COMBO);

				ComboBox_AddString(m_hwndCombo,L"Unicode");
				ComboBox_AddString(m_hwndCombo,L"UTF-8");
				ComboBox_AddString(m_hwndCombo,L"Shift JIS");
				ComboBox_SetCurSel(m_hwndCombo,0);

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
				if( LOWORD(wParam) == IDC_EDIT1 )
				{
					switch( HIWORD(wParam) )
					{
						case EN_CHANGE:
							OnEditChange((HWND)lParam);
							break;
					}
				}
				else if( LOWORD(wParam) == IDC_COMBO )
				{
					switch( HIWORD(wParam) )
					{
						case CBN_SELCHANGE:
							OnEditChange((HWND)m_hwndText);
							break;
					}
				}
			}
		}
		return CControlPane::DlgProc(hWnd,uMsg,wParam,lParam);
	}

private:
	VOID OnEditChange(HWND hwndEdit)
	{
		int cch = GetWindowTextLength( hwndEdit );
		if( cch == 0 )
		{
			// todo:
			return;
		}

		PWSTR buf = _MemAllocStringBuffer(cch);

		GetWindowText(hwndEdit,buf,cch+1);

		SetWindowText(m_hwndDump,L"");

		CHAR asz[8];
		WCHAR codeChar[8];
		WCHAR tmp[8];

		int mode = ComboBox_GetCurSel(m_hwndCombo);

		for(int i = 0; i < cch; i++)
		{
			WCHAR ch = buf[i];
			if( mode == 0 )
			{
				if( 1 )
					StringCchPrintf(codeChar,ARRAYSIZE(codeChar),L"0x%04X",ch);
				else
					StringCchPrintf(codeChar,ARRAYSIZE(codeChar),L"%02X%02X",(UCHAR)(ch & 0x00ff),(UCHAR)(ch>>8));
			}
			else if( mode == 1 )
			{
				memset(asz,0,sizeof(asz));
				int len = WideCharToMultiByte(CP_UTF8,0,&ch,1,asz,ARRAYSIZE(asz),NULL,NULL);

				memset(codeChar,0,sizeof(codeChar));
				for(int u=0; u <len; u++) {
					StringCchPrintf(tmp,ARRAYSIZE(tmp),L"%02X",(UCHAR)asz[u]);
					StringCchCat(codeChar,ARRAYSIZE(codeChar),tmp);
				}
			}
			else if( mode == 2 )
			{
				asz[0] = asz[1] = asz[2] = 0;
				WideCharToMultiByte(932,0,&ch,1,asz,ARRAYSIZE(asz),NULL,NULL);

				if( IsDBCSLeadByte( asz[0] ) )
					StringCchPrintf(codeChar,ARRAYSIZE(codeChar),L"%02X%02X",(UCHAR)asz[0],(UCHAR)asz[1]);
				else
					StringCchPrintf(codeChar,ARRAYSIZE(codeChar),L"%02X",asz[0]);
			}

			Edit_AddText(m_hwndDump,codeChar);

			if( i < (cch-1) )
				Edit_AddText(m_hwndDump,L" ");
			
		}

		_MemFree(buf);
	}
};
