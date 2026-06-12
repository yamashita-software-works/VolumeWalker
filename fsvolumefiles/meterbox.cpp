//****************************************************************************
//
//  meterbox.cpp
//
//  Meter box window.
//
//  Author:  YAMASHITA Katsuhiro
//
//  History: 2017.08.30 Created.
//           2025.06.28 Ported.
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "meterbox.h"

#define CLR_METER                 RGB(80,120,220)
#define CLR_METER_WARNING         RGB(218,38,38)
#define CLR_METER_BASE            RGB(232,232,232)
#define CLR_BORDER                RGB(208,208,208)
#define CLR_METER_TEXT            RGB(255,2552,255)
#define CLR_METER_BASE_TEXT       RGB(0,0,0)

/////////////////////////////////////////////////////////////////////////////
// MeterBox Self Registration

static LRESULT CALLBACK DefMeterBoxProc(HWND,UINT,WPARAM,LPARAM);

void InitMeterBox(HMODULE hModule)
{
	WNDCLASS wndcls = {0};
    wndcls.style       = CS_GLOBALCLASS|CS_OWNDC|CS_VREDRAW|CS_HREDRAW|CS_SAVEBITS|CS_PARENTDC|CS_DBLCLKS;
	wndcls.lpfnWndProc = DefMeterBoxProc;
    wndcls.hInstance   = hModule;
	wndcls.hIcon       = 0;
    wndcls.hCursor     = 0;
    wndcls.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wndcls.lpszMenuName = NULL;
	wndcls.lpszClassName = MTBC_METERBOX_NAME;
	RegisterClass(&wndcls);
}

/////////////////////////////////////////////////////////////////////////////
// Custom control

typedef struct _METERBOXSTRUCT
{
	COLORREF crMeter;
	COLORREF crMeterWarning;
	COLORREF crBackground;
	COLORREF crMeterText;
	COLORREF crBackgroundText;

	HFONT hFont; // Read only, the control does not delete it.
	DWORD dwStyle;
	HBRUSH hbrBack;

	LONGLONG Pos;
	LONGLONG Full;

	double Usage;
} METERBOXSTRUCT;

static void drawMeter(HWND hwnd,HDC hdc,METERBOXSTRUCT *pBox)
{
	RECT rc;
	GetClientRect(hwnd,&rc);

	int cxColumnWidth = rc.right - rc.left;
	int cxIcon = 0;

	RECT rcMeter = rc;

	if( pBox->dwStyle & MTBS_BORDER )
	{
		rcMeter.top    += 1;
		rcMeter.left   += 1;
		rcMeter.right  = rcMeter.left + cxColumnWidth - 2;
		rcMeter.bottom -= 1;
	}

	if( !(pBox->dwStyle & WS_DISABLED) )
	{
		double width = (double)(rcMeter.right - rcMeter.left);
		if( (pBox->Usage * 100.0) == 0.0 )
		{
			width = 0.0;
		}
		else
		{
			width *= pBox->Usage;

			if( width < 2.0 )
				width = 2.0;
		}

		//
		// draw fill meter style
		//
		HBRUSH hbr = CreateSolidBrush( 
						(pBox->Usage * 100.0) < 95.0 ? pBox->crMeter : pBox->crMeterWarning );

		HDC hdcMem1 = CreateCompatibleDC(hdc);
		HDC hdcMem2 = CreateCompatibleDC(hdc);
		HBITMAP hbmp1 = CreateCompatibleBitmap(hdc,rcMeter.right - rcMeter.left,rcMeter.bottom - rcMeter.top);
		HBITMAP hbmp2 = CreateCompatibleBitmap(hdc,rcMeter.right - rcMeter.left,rcMeter.bottom - rcMeter.top);
		HBITMAP hbmpOld1 = (HBITMAP)::SelectObject(hdcMem1,hbmp1);
		HBITMAP hbmpOld2 = (HBITMAP)::SelectObject(hdcMem2,hbmp2);

		// frame
		if( pBox->dwStyle & MTBS_BORDER )
		{
			HBRUSH hbr3 = CreateSolidBrush( CLR_BORDER );
			FillRect(hdc,&rc,hbr3);
			DeleteObject(hbr3);
		}

		RECT box;
		box.left   = 0;
		box.top    = 0;
		box.right  = rcMeter.right - rcMeter.left;
		box.bottom = rcMeter.bottom - rcMeter.top;
				
		HBRUSH hbr2 = CreateSolidBrush( CLR_METER_BASE );
		FillRect(hdcMem1,&box,hbr2);
		DeleteObject(hbr2);

		FillRect(hdcMem2,&box,hbr);


		WCHAR sz[MAX_PATH];

		if( (pBox->Usage * 100.0) == 0.0 )
			wnsprintf(sz,MAX_PATH,L"0%%");
		else if( (pBox->Usage * 100.0) < 1.0 )
			wnsprintf(sz,MAX_PATH,L"< 1%%"); // less then 1%
//			wnsprintf(sz,MAX_PATH,L"%.2f%%",(pBox->Usage * 100.0));
		else
			wnsprintf(sz,MAX_PATH,L"%d%%",(int)(pBox->Usage * 100.0));

		RECT rcText = box;
		rcText.right -= 2;

		HFONT hfontOld;
		if( pBox->hFont )
			hfontOld = (HFONT)SelectObject(hdcMem1,pBox->hFont);
		SetTextColor(hdcMem1,pBox->crBackgroundText);
		SetBkMode(hdcMem1,TRANSPARENT);
		DrawText(hdcMem1,sz,-1,&rcText,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
		if( pBox->hFont )
			SelectObject(hdcMem1,hfontOld);

		if( pBox->hFont )
			hfontOld = (HFONT)SelectObject(hdcMem2,pBox->hFont);
		SetTextColor(hdcMem2,pBox->crMeterText);
		SetBkMode(hdcMem2,TRANSPARENT);
		DrawText(hdcMem2,sz,-1,&rcText,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
		if( pBox->hFont )
			SelectObject(hdcMem2,hfontOld);

		//
		// Draw bitmap by BitBit, mixed meter parts.
		//

		// base
		BitBlt(hdc,
			rcMeter.left,
			rcMeter.top,
			rcMeter.right - rcMeter.left,
			rcMeter.bottom - rcMeter.top,
			hdcMem1,0,0,SRCCOPY);

		// color level
		rcMeter.right = rcMeter.left + (int)width;
		BitBlt(hdc,
			rcMeter.left,
			rcMeter.top,
			rcMeter.right - rcMeter.left,
			rcMeter.bottom - rcMeter.top,
			hdcMem2,0,0,SRCCOPY);

		SelectObject(hdcMem1,hbmpOld1);
		SelectObject(hdcMem2,hbmpOld2);

		DeleteObject(hbmp1);
		DeleteObject(hbmp2);
		DeleteDC(hdcMem1);
		DeleteDC(hdcMem2);
		DeleteObject(hbr);
	}
	else
	{
		FillRect(hdc,&rcMeter,GetSysColorBrush(COLOR_3DSHADOW));
	}
}

static void notifyEvent(HWND hWnd,METERBOXSTRUCT *pBox,UINT uMessage)
{
	HWND hwndParent = GetParent(hWnd);
	USHORT id = (USHORT)GetWindowLong(hWnd,GWL_ID);
	USHORT code;
	switch(uMessage) 
	{
		case WM_LBUTTONDOWN:
			code = MTBN_CLICKED;
			break;
		case WM_LBUTTONDBLCLK:
			code = MTBN_DBLCLK;
			break;
		default:
			return ;
	}
	SendMessage(hwndParent,WM_COMMAND,MAKEWPARAM(id,code),(LPARAM)hWnd);
}

LRESULT CALLBACK DefMeterBoxProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam)
{
	METERBOXSTRUCT *pBox = (METERBOXSTRUCT *)GetWindowLongPtr(hWnd,GWLP_USERDATA);

	switch( uMessage )
	{
		case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT ps;

			//
			// Moving another window over a control can cause parts of 
			// the control to be clipped draw, so we invalidate all it 
			// before do calling BeginPaint().
			//
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);

			hdc = BeginPaint(hWnd,&ps);

			drawMeter(hWnd,hdc,pBox);

			EndPaint(hWnd,&ps);
			break;
		}
		case WM_ERASEBKGND:
		{
			return TRUE;
		}
		case WM_CREATE:
		{
			pBox = (METERBOXSTRUCT *)_MemAllocZero( sizeof(METERBOXSTRUCT) );
			pBox->crMeter = CLR_METER;
			pBox->crBackground = CLR_METER_BASE;
			pBox->crMeterText = CLR_METER_TEXT;
			pBox->crBackgroundText = CLR_METER_BASE_TEXT;
			pBox->crMeterWarning = CLR_METER_WARNING;
			pBox->hFont = NULL;
			pBox->dwStyle = GetWindowLong(hWnd,GWL_STYLE);
			SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pBox);
			break;
		}
		case WM_DESTROY:
		{
			_MemFree( pBox );
			break;
		}
		case WM_STYLECHANGING:
		{
			STYLESTRUCT *p = (STYLESTRUCT*)lParam;
			if( wParam == GWL_STYLE )
			{
				pBox->dwStyle = p->styleNew;
			}
			break;
		}
		case WM_SETFONT:
		{
			pBox->hFont = (HFONT)wParam;
			break;
		}
		case WM_LBUTTONDOWN:
		{
			notifyEvent(hWnd,pBox,uMessage);
			break;
		}
		case WM_LBUTTONDBLCLK:
		{
			notifyEvent(hWnd,pBox,uMessage);
			break;
		}
		case MTBM_SETFULL:
		{
			pBox->Full = ((METERBOX_FULL *)lParam)->Full.QuadPart;
			RedrawWindow(hWnd,NULL,NULL,RDW_UPDATENOW|RDW_INVALIDATE);
			break;
		}
		case MTBM_GETFULL:
		{
			((METERBOX_FULL *)lParam)->Full.QuadPart = pBox->Full;
			break;
		}
		case MTBM_SETPOS:
		{
			pBox->Pos = ((METERBOX_POS *)lParam)->Pos.QuadPart;

			if( pBox->Full != 0 )
			{
				double pct = (double)pBox->Pos / (double)pBox->Full;
				pBox->Usage = pct;
			}

			RedrawWindow(hWnd,NULL,NULL,RDW_UPDATENOW|RDW_INVALIDATE);

			break;
		}
		case MTBM_GETPOS:
		{
			((METERBOX_POS *)lParam)->Pos.QuadPart = pBox->Pos;
			break;
		}
		case MTBM_SETINFO:
		{
			METERBOX_INFO *pInfo = (METERBOX_INFO *)lParam;
			pBox->crMeter = pInfo->crMeter;
			pBox->crBackground = pInfo->crBackground;
			pBox->crMeterText = pInfo->crMeterText;
			pBox->crBackgroundText = pInfo->crBackgroundText;
			pBox->crMeterWarning = pInfo->crMeterWarning;
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
			break;
		}
		case MTBM_GETINFO:
		{
			METERBOX_INFO *pInfo = (METERBOX_INFO *)lParam;
			pInfo->crMeter = pBox->crMeter;
			pInfo->crBackground = pBox->crBackground;
			pInfo->crMeterText = pBox->crMeterText;
			pInfo->crBackgroundText = pBox->crBackgroundText;
			pInfo->crMeterWarning = pBox->crMeterWarning;
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
			break;
		}
	}
	return DefWindowProc(hWnd,uMessage,wParam,lParam);
}
