#pragma once
//*****************************************************************************
//
//  page_codetochar.h
//
//  PURPOSE: Character Code to Character Converter ToolPad Page
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

class CCodeToCharacterToolPage : public CControlPane
{
	CUILayout m_Layout;

	HWND m_hwndText;
	HWND m_hwndDump;
	HWND m_hwndCombo;
public:
	DEFDIALOGRESID(IDD_TOOLPAGE_CODE_TO_CHARACTER)

	CCodeToCharacterToolPage()
	{
		m_hwndDump = NULL;
	}

	~CCodeToCharacterToolPage()
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

				SetWindowSubclass(m_hwndText,&EditBox_HexInputSubclassProc,1,EBSPF_ACCEPT_SPACE|EBSPF_ACCEPT_HEX_PREFIX_CHAR);

				m_hwndCombo = GetDlgItem(m_hWnd,IDC_COMBO);

				ComboBox_AddString(m_hwndCombo,L"Unicode");
				ComboBox_AddString(m_hwndCombo,L"UTF-8");
				ComboBox_AddString(m_hwndCombo,L"Shift JIS");
				ComboBox_SetCurSel(m_hwndCombo,0);

				break;
			}
			case WM_DESTROY:
			{
				RemoveWindowSubclass(m_hwndText,&EditBox_HexInputSubclassProc,1);
				return 0;
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
			return;
		}

		PWSTR pwszTextBuf = _MemAllocStringBuffer(cch+1);
		if( pwszTextBuf == NULL )
			return ;

		GetWindowText(hwndEdit,pwszTextBuf,cch+1);

		int cbBinaryBuf = 32768;
		int cchOutBuf = 65536;
		BYTE *BinaryBuf = (BYTE *)_MemAllocZero(cbBinaryBuf);
		WCHAR *pwszOutBuf = _MemAllocStringBuffer(cchOutBuf);
		if( BinaryBuf && pwszOutBuf )
		{
			int cb;
			cb = WideCharTokenToBin(pwszTextBuf,(UCHAR*)BinaryBuf,cbBinaryBuf);
	
			cch = cb / sizeof(WCHAR);
	
			SetWindowText(m_hwndDump,L"");
	

			PWSTR pszEditBoxText = NULL;
			int len;
			int mode = ComboBox_GetCurSel(m_hwndCombo);

			if( mode == 0 ) // unicode
			{
				pszEditBoxText = (PWSTR)BinaryBuf;
				len = cch;
			}
			else if( mode == 1 ) // utf-8
			{
				len = MultiByteToWideChar(CP_UTF8,0,(LPCSTR)BinaryBuf,-1,pwszOutBuf,cchOutBuf);
				pszEditBoxText = pwszOutBuf;
			}
			else if( mode == 2 ) // shiftjis
			{
				len = MultiByteToWideChar(932,0,(LPCSTR)BinaryBuf,-1,pwszOutBuf,cchOutBuf);
				pszEditBoxText = pwszOutBuf;
			}

			if( len && pszEditBoxText )
				Edit_AddText(m_hwndDump,pszEditBoxText);
		}
		_SafeMemFree(BinaryBuf);
		_SafeMemFree(pwszOutBuf);
		_SafeMemFree(pwszTextBuf);
	}

	int WideCharTokenToBin(PWSTR pszInputStringBuf,BYTE *BinaryBuf,int BinaryBufSize)
	{
		UCHAR *pBuf = BinaryBuf;
		wchar_t *token;
		wchar_t *next_token;
		const wchar_t *seps = L"\x20\t;,";
		ULONG lRet;
		int n = 0;
		int f = 0;
		token = wcstok_s(pszInputStringBuf,seps,&next_token);

		while( token != NULL )
        {
			// "0"         -> 00
			// "0x"        -> none
			// "0x1"       -> 1
			// "0x12"      -> 12
			// "0x123"     -> 03 12
			// "0x1234"    -> 34 12
			// "0x12345"   -> 05 34 12
			// "0x123456"  -> 56 34 12
			// "0x1234567" -> 07 56 34 12
			// "12"        -> 12
			// "123"       -> 12 03
			// "1234"      -> 12 34
			// "12345"     -> 12 34 05
			// "123456"    -> 12 34 56

			if( token[0] == L'0' && (token[1] == L'x' || token[1] == L'X' ) )
			{
				token += 2;
				f = 1;
			}
			else
			{
				f = 0;
			}

			if( f == 1 )
			{
				int token_len = (int)wcslen(token);
				int width;

				if( token_len >= 7 )
				{
					// "00000000"
					// "0000000"
					width = 4;
				}
				else if( token_len >= 5 )
				{
					// "000000"
					// "00000"
					width = 3;
				}
				else if( token_len >= 3 )
				{
					// "0000"
					// "000"
					width = 2;
				}
				else
				{
					width = 1;
				}

				if( StringToIntegerW(token,16,&lRet) )
				{
					ULONG mask = 0xff;
					ULONG val;

					do
					{
						if( n >= BinaryBufSize )
							break;

						val = (lRet & mask);
						pBuf[n++] = (UCHAR)val;
 
						lRet >>= 8;
					}
					while( --width > 0 );
				}
			}
			else
			{
				WCHAR byte[2+1];
				wchar_t *p = token;

				while( *p )
				{
					byte[0] = byte[1] = L'0';
					byte[2] = L'\0';
					if( *p != L'\0' )
					{
						byte[0] = *(p++);
						if( *p != L'\0' )
							byte[1] = *(p++);
					}
					else
					{
						byte[0] = *(p++);
					}

					if( StringToIntegerW(byte,16,&lRet) )
					{
						if( n >= BinaryBufSize )
							break;

						pBuf[n++] = (UCHAR)lRet;
					}
				}
			}

			if( n >= BinaryBufSize )
				break;

			token = wcstok_s(NULL,seps,&next_token);
		}

		return n;
	}
};
