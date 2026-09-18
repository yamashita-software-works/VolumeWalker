#pragma once
//*****************************************************************************
//
//  page_datetime.h
//
//  PURPOSE: Date Time ToolPad Page
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-01-19 Created.
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//

#ifndef _DTH_1MS_UNITS_100NS
#define _DTH_1MS_UNITS_100NS  (10000)
#endif

class CDateTimeToolPage : public CControlPane
{
	CUILayout m_Layout;

	HWND m_hwndDTC;
	HWND m_hwndMCC;
	HWND m_hwndHexLocal;
	HWND m_hwndHexUTC;
	HWND m_hwndHexMSDOS;
	HWND m_hwndHexUNIX;
	HWND m_hwndFATDate;
	HWND m_hwndFATTime;
	HWND m_hwndFATDateTime;
	HWND m_hwndMSec;
	HWND m_hwnd100ns;

	LARGE_INTEGER m_liCurrentDateTime;

public:
	DEFDIALOGRESID(IDD_TOOLPAGE_DATETIME)

	CDateTimeToolPage()
	{
		m_hwndDTC = NULL;
		m_hwndMCC = NULL;
		m_hwndHexLocal = NULL;
		m_hwndHexUTC = NULL;
		m_hwndHexMSDOS = NULL;
		m_hwndHexUNIX = NULL;
		m_hwndFATDate = NULL;
		m_hwndFATTime = NULL;
		m_hwndFATDateTime = NULL;
		m_hwndMSec = NULL;
		m_hwnd100ns = NULL;
		m_liCurrentDateTime.QuadPart = 0;
	}

	~CDateTimeToolPage()
	{
	}

	virtual VOID OnInitPane(PVOID) {}
	virtual VOID OnStartOperation(PVOID) {}
	virtual VOID OnEndOperation(PVOID) {}

	void _SetCurrentAbsTime(LARGE_INTEGER& li)
	{
		m_liCurrentDateTime = li;
	}

	void _SetFileTimeToCurrentAbsTime(FILETIME& ft)
	{
		m_liCurrentDateTime.LowPart = ft.dwLowDateTime;
		m_liCurrentDateTime.HighPart = ft.dwHighDateTime;
	}

	void _SystemTimeToCurrentAbsTime(SYSTEMTIME& st)
	{
		DWORD n = _GetAbsTimeLessThen1msPart(m_liCurrentDateTime);
		SystemTimeToFileTime(&st,(FILETIME*)&m_liCurrentDateTime);
		m_liCurrentDateTime.QuadPart = _SetAbsTimeLessThen1msPart64(m_liCurrentDateTime.QuadPart,n);
	}

	void _AdjustFileTime100nsByCurrentAbsTime(FILETIME& ft)
	{
		DWORD n = _GetAbsTimeLessThen1msPart(m_liCurrentDateTime);
		_SetAbsTimeLessThen1msPartFileTime(ft,n);
	}

	void SetTimeBinBox(HWND hwndEdit,ULONG64 val,BOOL Radix=16)
	{
		WCHAR sz[32];
		if( Radix == 16 )
			StringCchPrintf(sz,ARRAYSIZE(sz),L"%I64X",val);
		else
			StringCchPrintf(sz,ARRAYSIZE(sz),L"%I64u",val);
		SetWindowText(hwndEdit,sz);
	}

	void UpdateTimeBinBoxes(SYSTEMTIME *pst)
	{
		FILETIME ftLocal;
		FILETIME ftSystem;

		int Radix = IsHexAbsoluteTime() ? 16 : 10;

		//
		// SYSTEMTIME to FILETIME
		//
		if( pst )
		{
			// The *pst parameter will be passed by local time.
			SystemTimeToFileTime(pst,&ftLocal);

			_AdjustFileTime100nsByCurrentAbsTime(ftLocal);

			// Revert localtime to system time
			LocalFileTimeToFileTime(&ftLocal,&ftSystem);
		}
		else
		{
			// use m_liCurrentDateTime, not SYSTEMTIME
			LargeIntegerToFileTime(m_liCurrentDateTime,ftLocal);
			LocalFileTimeToFileTime(&ftLocal,&ftSystem);
		}

		//
		// FILETIME to LARGE_INTEGER
		//
		LARGE_INTEGER liSystem;
		LARGE_INTEGER liLocal;

		FileTimeToLargeInteger(ftSystem,liSystem);
		FileTimeToLargeInteger(ftLocal,liLocal);
	
		SetTimeBinBox(m_hwndHexLocal,liLocal.QuadPart,Radix);
		SetTimeBinBox(m_hwndHexUTC,liSystem.QuadPart,Radix);

	    ULONG ElapsedSeconds;
		if( RtlTimeToSecondsSince1980(&liLocal,&ElapsedSeconds) )
		{
			SetTimeBinBox(m_hwndHexMSDOS,ElapsedSeconds,Radix);
		}
		else
		{
			SetWindowText(m_hwndHexMSDOS,L"-");
		}

		if( RtlTimeToSecondsSince1970(&liLocal,&ElapsedSeconds) )
		{
			SetTimeBinBox(m_hwndHexUNIX,ElapsedSeconds,Radix);
		}
		else
		{
			SetWindowText(m_hwndHexUNIX,L"-");
		}

		SetDosDateTime(ftLocal);
	}

	void SetDosDateTime(FILETIME& ftLocal)
	{
		const int Radix = 16;
		WORD FatDate;
		WORD FatTime;
		if( !FileTimeToDosDateTime(&ftLocal,&FatDate,&FatTime) )
		{
			SetWindowText(m_hwndFATDate,L"-");
			SetWindowText(m_hwndFATTime,L"-");
			SetWindowText(m_hwndFATDateTime,L"-");
			return ;
		}
	
		SetTimeBinBox(m_hwndFATDate,FatDate,Radix);
		SetTimeBinBox(m_hwndFATTime,FatTime,Radix);

		DOSFATDATE dfd;
		DOSFATTIME dft;

		dfd.w = FatDate;
		dft.w = FatTime;
	
		WCHAR szDosDateTime[64];
	
		if( IsDosDateTimeRawData() )
		{
			StringCchPrintf(szDosDateTime,ARRAYSIZE(szDosDateTime),L"y=%d m=%d d=%d , h=%d m=%d s=%d",
					dfd.year,
					dfd.month,
					dfd.day,
					dft.hour,
					dft.minute,
					dft.second);
		}
		else
		{
			SYSTEMTIME st = {0};
			st.wDay    = dfd.day;
			st.wMonth  = dfd.month;
			st.wYear   = dfd.year + 1980;
			st.wHour   = dft.hour;
			st.wMinute = dft.minute;
			st.wSecond = dft.second * 2;
			WCHAR szDate[16];
			WCHAR szTime[16];

			GetDateFormat(LOCALE_USER_DEFAULT,0,&st,NULL,szDate,ARRAYSIZE(szDate));
			GetTimeFormat(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT,&st,NULL,szTime,ARRAYSIZE(szTime));
	
			StringCchPrintf(szDosDateTime,ARRAYSIZE(szDosDateTime),L"%s %s",szDate,szTime);
		}
	
		SetWindowText(m_hwndFATDateTime,szDosDateTime);
	}

	void SetTextMilliseconds(SYSTEMTIME *pst)
	{
		WCHAR sz[3+1];
		StringCchPrintf(sz,ARRAYSIZE(sz),L"%u",pst->wMilliseconds);
		SetWindowText(m_hwndMSec,sz);
	}

	void SetText100ns(LARGE_INTEGER& li)
	{
		WCHAR sz[4+1];
		int n = (int)(li.QuadPart % _DTH_1MS_UNITS_100NS);
		StringCchPrintf(sz,ARRAYSIZE(sz),L"%u",n);
		SetWindowText(m_hwnd100ns,sz);
	}

	int GetAbsoluteTimeEditBoxCurrentRadix()
	{
		return IsHexAbsoluteTime() ? 16 : 10;
	}

	BOOL IsHexAbsoluteTime()
	{
		return (IsDlgButtonChecked(m_hWnd,IDC_CHECK1) == BST_CHECKED);
	}

	BOOL IsDosDateTimeRawData()
	{
		return (IsDlgButtonChecked(m_hWnd,IDC_CHECK2) == BST_CHECKED);
	}

	VOID ConvertEditBoxHexDec(HWND hwndEdit,int Radix)
	{
		INT64 val;
		WCHAR sz[128];
		GetWindowText(hwndEdit,sz,ARRAYSIZE(sz));

		if( Radix == 16 ) // Dec to Hex
		{
			val = _wcstoi64(sz,nullptr,10);
			SetTimeBinBox(hwndEdit,val,16);
		}
		else if( Radix == 10 ) // Hex to Dec
		{
			val = _wcstoi64(sz,nullptr,16);
			SetTimeBinBox(hwndEdit,val,10);
		}
	}

	void OnUpdateMilliseconds()
	{
		WCHAR sz[4];
		GetWindowText(m_hwndMSec,sz,ARRAYSIZE(sz));
		int val = _wtoi(sz);

		SYSTEMTIME st;
		DateTime_GetSystemtime(m_hwndDTC, &st);
		st.wMilliseconds = (WORD)val;		
		DateTime_SetSystemtime(m_hwndDTC,GDT_VALID,&st);

		UpdateTimeBinBoxes(&st);
	}

	void OnUpdate100ns()
	{
		WCHAR sz[5];
		GetWindowText(m_hwnd100ns,sz,ARRAYSIZE(sz));
		int val = _wtoi(sz);

		m_liCurrentDateTime.QuadPart = _SetAbsTimeLessThen1msPart64(m_liCurrentDateTime.QuadPart,val);

		UpdateTimeBinBoxes(NULL);
	}

	void OnOK()
	{
		HWND hwndFocusedEditBox = GetFocus();
		if( hwndFocusedEditBox == m_hwndHexLocal )
		{
			WCHAR sz[32] = {};
			GetWindowText(m_hwndHexLocal,sz,ARRAYSIZE(sz));
			LARGE_INTEGER liRet;

			int radix = GetAbsoluteTimeEditBoxCurrentRadix();
			if( sz[0] == L'0' && (sz[1] == L'x' || sz[1] == L'X') )
				radix = 16;
			liRet.QuadPart = _wcstoi64(sz,nullptr,radix);

			FILETIME ft;
			SYSTEMTIME st;
			ft.dwHighDateTime = liRet.HighPart;
			ft.dwLowDateTime  = liRet.LowPart;

			FileTimeToSystemTime(&ft,&st);

			_SetFileTimeToCurrentAbsTime(ft);
			DateTime_SetSystemtime(m_hwndDTC,GDT_VALID,&st);
			MonthCal_SetCurSel(m_hwndMCC,&st);
			SetTextMilliseconds(&st);
			SetText100ns(m_liCurrentDateTime);
		}
		else if( hwndFocusedEditBox == m_hwndHexUTC )
		{
			WCHAR sz[32] = {};
			GetWindowText(m_hwndHexUTC,sz,ARRAYSIZE(sz));
			LARGE_INTEGER liRet;

			int radix = GetAbsoluteTimeEditBoxCurrentRadix();
			if( sz[0] == L'0' && (sz[1] == L'x' || sz[1] == L'X') )
				radix = 16;
			liRet.QuadPart = _wcstoi64(sz,nullptr,radix);

			FILETIME ft;
			SYSTEMTIME st;
			ft.dwHighDateTime = liRet.HighPart;
			ft.dwLowDateTime  = liRet.LowPart;

			FileTimeToLocalFileTime(&ft,&ft);
			FileTimeToSystemTime(&ft,&st);

			_SetFileTimeToCurrentAbsTime(ft);
			DateTime_SetSystemtime(m_hwndDTC,GDT_VALID,&st);
			MonthCal_SetCurSel(m_hwndMCC,&st);
			SetTextMilliseconds(&st);
			SetText100ns(m_liCurrentDateTime);
		}
	}

	virtual INT_PTR DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( uMsg )
		{
			case WM_INITDIALOG:
			{
				SYSTEMTIME st = {};
				//st.wYear   = 2026;
				//st.wMonth  = 1;
				//st.wDay    = 31;
				//st.wHour   = 1;
				//st.wMinute = 2;
				//st.wSecond = 34;

				m_Layout.Initialize(hWnd);			
				m_Layout.AnchorControl(CUILayout::AP_BOTTOMRIGHT,CUILayout::AP_BOTTOMRIGHT,IDOK,FALSE);
				m_Layout.AnchorControl(CUILayout::AP_BOTTOMRIGHT,CUILayout::AP_BOTTOMRIGHT,IDCLOSE,FALSE);

				m_hwndDTC = GetDlgItem(hWnd,IDC_DATETIMEPICKER);
				m_hwndMCC = GetDlgItem(hWnd,IDC_MONTHCALENDAR);
				m_hwndHexLocal = GetDlgItem(hWnd,IDC_EDIT3);
				m_hwndHexUTC = GetDlgItem(hWnd,IDC_EDIT4);
				m_hwndHexMSDOS = GetDlgItem(hWnd,IDC_EDIT5);
				m_hwndHexUNIX = GetDlgItem(hWnd,IDC_EDIT6);
				m_hwndFATDate = GetDlgItem(hWnd,IDC_EDIT7);
				m_hwndFATTime = GetDlgItem(hWnd,IDC_EDIT8);
				m_hwndFATDateTime= GetDlgItem(hWnd,IDC_EDIT9);

				m_hwndMSec = GetDlgItem(hWnd,IDC_EDIT1);
				m_hwnd100ns = GetDlgItem(hWnd,IDC_EDIT2);
 
				WCHAR sz[64];
				WCHAR szDateFmt[28];
				WCHAR szTimeFmt[28];
				GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SSHORTDATE,szDateFmt,_countof(szDateFmt));
				GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_STIMEFORMAT,szTimeFmt,_countof(szTimeFmt));
				wnsprintf(sz,_countof(sz),_T("%s %s"),szDateFmt,szTimeFmt);
				DateTime_SetFormat(m_hwndDTC,sz);

				MonthCal_SetColor(m_hwndMCC,MCSC_BACKGROUND,RGB(223,223,223));

				CheckDlgButton(m_hWnd,IDC_CHECK1,BST_CHECKED);

				Edit_LimitText(m_hwndMSec,3);
				Edit_LimitText(m_hwnd100ns,4);

				Edit_LimitText(m_hwndFATDate,4);
				Edit_LimitText(m_hwndFATTime,4);

				{
					//MonthCal_SetCurSel(m_hwndMCC, &st);              //@@ if user specified
					//DateTime_SetSystemtime(m_hwndDTC,GDT_VALID,&st); //@@ if user specified

					DateTime_GetSystemtime(m_hwndDTC, &st);
					_SystemTimeToCurrentAbsTime(st);
					UpdateTimeBinBoxes(&st);
					SetTextMilliseconds(&st);
					SetText100ns(m_liCurrentDateTime);
				}
				break;
			}
			case WM_SIZE:
			{
				int cx = GET_X_LPARAM(lParam);
				int cy = GET_Y_LPARAM(lParam);
				m_Layout.AdjustControls(cx,cy);
				break;
			}
			case WM_NOTIFY:
			{
				NMHDR *pnmhdr = (NMHDR *)lParam;
				LONG_PTR lResult = 0;

				if( pnmhdr->idFrom == IDC_MONTHCALENDAR )
				{
					NMSELCHANGE *pnmsc = (NMSELCHANGE *)pnmhdr;
					switch( pnmhdr->code )
					{
						case MCN_SELCHANGE:
						{
							SYSTEMTIME st;
							DateTime_GetSystemtime(m_hwndDTC, &st);
							pnmsc->stSelStart.wHour = st.wHour; 
							pnmsc->stSelStart.wMinute = st.wMinute;
							pnmsc->stSelStart.wSecond = st.wSecond;
							pnmsc->stSelStart.wMilliseconds = st.wMilliseconds;
							DateTime_SetSystemtime(m_hwndDTC,GDT_VALID,&pnmsc->stSelStart);
							_SystemTimeToCurrentAbsTime(st);
							UpdateTimeBinBoxes(&pnmsc->stSelStart);
							SetTextMilliseconds(&pnmsc->stSelStart);
							SetText100ns(m_liCurrentDateTime);
							break;
						}
						case MCN_SELECT:
						{
							if( GetFocus() != pnmsc->nmhdr.hwndFrom )
								SetFocus( pnmsc->nmhdr.hwndFrom );
							NMSELCHANGE *pnmsc = (NMSELCHANGE *)pnmhdr;
							SetFocus( pnmsc->nmhdr.hwndFrom );
							break;
						}
					}
				}
				else if( pnmhdr->idFrom == IDC_DATETIMEPICKER )
				{
					switch( pnmhdr->code )
					{
						case DTN_DATETIMECHANGE:
						{
							NMDATETIMECHANGE *pnmsc = (NMDATETIMECHANGE *)pnmhdr;
							MonthCal_SetCurSel(m_hwndMCC,&pnmsc->st);
							_SystemTimeToCurrentAbsTime(pnmsc->st);
							UpdateTimeBinBoxes(&pnmsc->st);
							SetTextMilliseconds(&pnmsc->st);
							SetText100ns(m_liCurrentDateTime);
							break;
						}
					}
				}
				SetWindowLongPtr(hWnd,DWLP_MSGRESULT,lResult);
				return 0;
			}
			case WM_COMMAND:
			{
				switch( LOWORD(wParam) )
				{
					case IDOK:
						OnOK();
						break;
					case IDC_EDIT1: // msec box
						if( HIWORD(wParam) == EN_UPDATE )
						{
							OnUpdateMilliseconds();
							return TRUE;
						}
						break;
					case IDC_EDIT2: // 100nsec box
						if( HIWORD(wParam) == EN_UPDATE )
						{
							OnUpdate100ns();
							return TRUE;
						}
						break;
					case IDC_CHECK1:
					{
						int Radix = GetAbsoluteTimeEditBoxCurrentRadix();
						ConvertEditBoxHexDec(m_hwndHexLocal,Radix);
						ConvertEditBoxHexDec(m_hwndHexUTC,Radix);
						ConvertEditBoxHexDec(m_hwndHexMSDOS,Radix);
						ConvertEditBoxHexDec(m_hwndHexUNIX,Radix);
						break;
					}
					case IDC_CHECK2:
					{
						SYSTEMTIME st;
						FILETIME ft;
						DateTime_GetSystemtime(m_hwndDTC, &st);
						SystemTimeToFileTime(&st,&ft);
						SetDosDateTime(ft);
						break;
					}
					case IDC_BUTTON1:
					{
						FILETIME ft,ftLocal;
						SYSTEMTIME st;
						GetSystemTimeAsFileTime(&ft);
						FileTimeToLocalFileTime(&ft,&ftLocal);
						FileTimeToSystemTime(&ftLocal,&st);
						_SetFileTimeToCurrentAbsTime(ftLocal);
						UpdateTimeBinBoxes(NULL);

						MonthCal_SetCurSel(m_hwndMCC, &st);
						DateTime_SetSystemtime(m_hwndDTC,GDT_VALID,&st);
						UpdateTimeBinBoxes(&st);
						SetTextMilliseconds(&st);
						SetText100ns(m_liCurrentDateTime);
						break;
					}
				}
			}
		}
		return CControlPane::DlgProc(hWnd,uMsg,wParam,lParam);
	}
};
