#pragma once
//*****************************************************************************
//
//  page_unit.h
//
//  PURPOSE: Numeric unit converter.
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

enum {
BYTESIZE64,
BYTESIZE64_ROUND_TO_NEAREST_DISPLAYED_DIGIT,
BYTESIZE64_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS,
BYTESIZE64_KB,
BYTESIZE32,
TIMEINTERVAL32,
TIMESECTO100NS,
TIMESECTOMSEC,
TIMEFILETIME,
TIMEFILETIMELOCAL,
};

// LLONG_MAX
// "9223372036854775807" + margin
// e.g. _I128_MAX
// "170141183460469231731687303715884105727"
#define EDIT_INPUT_MAXLENGTH  (20+8)

class CUnitToolPage : public CControlPane
{
	CUILayout m_Layout;

	BOOL m_bInEdit;

	HWND m_hwndInput;
	HWND m_hwndOutput;
	HWND m_hwndCB;

	typedef struct {
		PCWSTR Title;
		int nUnitConvType;
	} COMBOBOXITEM;

public:
	DEFDIALOGRESID(IDD_TOOLPAGE_UNIT)

	CUnitToolPage()
	{
		m_bInEdit = FALSE;
	}

	~CUnitToolPage()
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
				m_hwndCB     = GetDlgItem(hWnd,IDC_COMBO); 
				m_hwndInput  = GetDlgItem(hWnd,IDC_EDIT1);
				m_hwndOutput = GetDlgItem(hWnd,IDC_EDIT2);
				m_Layout.Initialize(hWnd);			
				m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_TOPRIGHT,IDC_EDIT1,FALSE);
				m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_TOPRIGHT,IDC_EDIT2,FALSE);
				InitComoboBox();
				ComboBox_SetCurSel(m_hwndCB,0);
				Edit_LimitText(m_hwndInput,EDIT_INPUT_MAXLENGTH);
#if 0
				SetWindowSubclass(m_hwndInput,&EditBox_NumberInputSubclassProc,1,0);
#endif
				break;
			}
			case WM_DESTROY:
			{
#if 0
				RemoveWindowSubclass(m_hwndInput,&EditBox_NumberInputSubclassProc,1);
#endif
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
				switch( HIWORD(wParam) )
				{
					case EN_CHANGE:
					case CBN_SELCHANGE:
						OnEditChange((HWND)lParam);
						break;
				}

				switch( LOWORD(wParam) )
				{
					case IDOK:
						return TRUE;
				}
			}
		}
		return CControlPane::DlgProc(hWnd,uMsg,wParam,lParam);
	}

private:
	VOID InitComoboBox()
	{
		static COMBOBOXITEM cbi[] = {
			{L"LONG64 - StrFormatByteSize64",           BYTESIZE64},
			{L"LONG64 - StrFormatByteSizeEx(Round)",    BYTESIZE64_ROUND_TO_NEAREST_DISPLAYED_DIGIT},
			{L"LONG64 - StrFormatByteSizeEx(Truncate)", BYTESIZE64_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS},
			{L"LONG64 - StrFormatKBSize",               BYTESIZE64_KB},
			{L"ULONG32 - StrFormatByteSizeA",           BYTESIZE32},
			{L"ULONG32 - StrFromTimeInterval(msec)",    TIMEINTERVAL32},
			{L"LONG64 - Second to 100ns units",         TIMESECTO100NS},
			{L"LONG64 - Second to Milliseconds",        TIMESECTOMSEC},
			{L"LONG64 - File Time",                     TIMEFILETIME},
			{L"LONG64 - File Time (Local)",             TIMEFILETIMELOCAL},
		};

		for(int i = 0; i < _countof(cbi); i++)
		{
			ComboBox_AddString(m_hwndCB,cbi[i].Title);
			ComboBox_SetItemData(m_hwndCB,i,cbi[i].nUnitConvType);
		}
	}

	int SelectComboBox(int UnitConvType,BOOL bSelect=true)
	{
		int i,c;
		c = ComboBox_GetCount(m_hwndCB);
		for(i = 0; i < c; i++)
		{
			COMBOBOXITEM *pItem = (COMBOBOXITEM *)ComboBox_GetItemData(m_hwndCB,i);

			if( pItem->nUnitConvType == UnitConvType )
			{
				if( bSelect )
					ComboBox_SetCurSel(m_hwndCB,i);
				return i;
			}
		}
		return -1;
	}

	VOID OnEditChange(HWND hwndEdit)
	{
		const int BUFSIZE=128;
		WCHAR szBuf[BUFSIZE];
		WCHAR szText[BUFSIZE];
		CHAR aszText[BUFSIZE];
		LONGLONG n;
		LARGE_INTEGER li;

		if( m_bInEdit )
			return ;

		m_bInEdit = TRUE;

		UINT nUnitConvType = (UINT)ComboBox_GetCurSel(m_hwndCB);

		if( GetWindowText(m_hwndInput,szBuf,BUFSIZE) > 0 )
		{
			if( StrToInt64Ex(szBuf,STIF_SUPPORT_HEX,&n) )
			{
				switch( nUnitConvType )
				{
					case BYTESIZE64:
						if( StrFormatByteSize64(n,szText,BUFSIZE) )
							SetWindowText(m_hwndOutput,szText);
						break;
					case BYTESIZE64_ROUND_TO_NEAREST_DISPLAYED_DIGIT:
						StrFormatByteSizeEx(n,SFBS_FLAGS_ROUND_TO_NEAREST_DISPLAYED_DIGIT,szText,BUFSIZE);
						SetWindowText(m_hwndOutput,szText);
						break;
					case BYTESIZE64_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS:
						StrFormatByteSizeEx(n,SFBS_FLAGS_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS,szText,BUFSIZE);
						SetWindowText(m_hwndOutput,szText);
						break;
					case BYTESIZE64_KB:
						StrFormatKBSize(n,szText,BUFSIZE);
						SetWindowText(m_hwndOutput,szText);
						break;
					case BYTESIZE32:
						li.QuadPart = n;
						StrFormatByteSizeA(li.LowPart,aszText,BUFSIZE);
						SetWindowTextA(m_hwndOutput,aszText);
						break;
					case TIMEINTERVAL32:
					{
						li.QuadPart = n;
					    int digits = 3;
						StrFromTimeInterval(szText,BUFSIZE,li.LowPart,digits);
						SetWindowText(m_hwndOutput,szText);
						break;
					}
					case TIMESECTO100NS:
					{
						const int _1sec_100ns = 10000000;
						StringCchPrintf(szText,BUFSIZE,L"%I64d",(n * _1sec_100ns));
						SetWindowText(m_hwndOutput,szText);
						break;
					}
					case TIMESECTOMSEC:
					{
						const int _1sec_msec = 1000;
						StringCchPrintf(szText,BUFSIZE,L"%I64d",(n * _1sec_msec));
						SetWindowText(m_hwndOutput,szText);
						break;
					}
					case TIMEFILETIME:
					{
						_GetDateTimeStringEx2(n,szText,BUFSIZE,NULL,NULL,TRUE,TRUE);
						SetWindowText(m_hwndOutput,szText);
						break;
					}
					case TIMEFILETIMELOCAL:
					{
						li.QuadPart = n;
						FileTimeToLocalFileTime((FILETIME*)&li,(FILETIME*)&li);
						_GetDateTimeStringEx2(li.QuadPart,szText,BUFSIZE,NULL,NULL,TRUE,TRUE);
						SetWindowText(m_hwndOutput,szText);
						break;
					}
				}
			}
			else
			{
				SetWindowText(m_hwndOutput,L"");
			}
		}
		else
		{
			SetWindowText(m_hwndOutput,L"");
		}

		m_bInEdit = FALSE;
	}
};

