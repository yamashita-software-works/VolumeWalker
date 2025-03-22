//***************************************************************************
//*                                                                         *
//*  listviewhelper.cpp                                                     *
//*                                                                         *
//*  Create: 2023-11-14                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "libmisc.h"
#include "common_control_helper.h"

//
// ListView Helper
//
UINT
ListViewEx_SimpleContextMenuHandler(
	HWND hWnd,
	HWND hWndList,
	HWND hwndRightClicked, // Reserved
	HMENU hMenu,
	POINT point,
	UINT uFlags
	)
{
	POINT pt = point;
	int iSelItem;

	if( pt.x == -1 && pt.y == -1 )
	{
		// context menu key
		iSelItem = ListView_GetNextItem(hWndList,-1,LVNI_SELECTED|LVNI_FOCUSED);
	}
	else
	{
		// mouse button click
		MapWindowPoints(NULL,hWndList,&pt,1);

		UINT f = 0;
		iSelItem = ListViewEx_HitTest(hWndList,pt,&f);

		if( iSelItem != -1 )
		{
			if( 0x200 & f ) // undocumented flag? Hit test result on Header Controol.
			{
				iSelItem = -1;
			}
		}
	}

	UINT Result=0;
	if( iSelItem != -1 )
	{
		RECT rcItem;
		if( ListView_GetItemRect(hWndList,iSelItem,&rcItem,LVIR_BOUNDS) )
		{
			if( pt.x == -1 && pt.y == -1 )
			{
				pt.x = rcItem.left;
				pt.y = rcItem.bottom;
			}

			MapWindowPoints(hWndList,NULL,&pt,1);

			// Handle to the window that owns the shortcut menu.
			// This window receives all messages from the menu.
			// The window does not receive a WM_COMMAND message from the menu until the function returns. 
			// If you specify TPM_NONOTIFY in the fuFlags parameter, the function does not send messages 
			// to the window identified by hwnd. However, you must still pass a window handle in hwnd. 
			// It can be any window handle from your application. 
			//
			if( hWnd == NULL )
				hWnd = GetActiveWindow();

			Result = TrackPopupMenuEx(hMenu,uFlags,pt.x,pt.y,hWnd,NULL);
		}
	}
	else
	{
		Result = (UINT)-1;
	}

	return Result;
}


int
ListViewEx_SetColumnWidthByHeaderText(
	HWND hwndLV,
	int iColumn,
	DWORD dwFlags
	)
{
	WCHAR sz[MAX_PATH];

    HWND hwndHD = ListView_GetHeader(hwndLV);
	if( hwndHD == NULL )
		return -1;

	HDITEM hdi = {0};
	hdi.mask = HDI_TEXT;
	hdi.pszText = sz;
	hdi.cchTextMax = MAX_PATH;
	Header_GetItem(hwndHD,iColumn,&hdi);

	HDC hdc;
	hdc = GetWindowDC(hwndHD);

	HFONT hFont,hFontOld;
	hFont = (HFONT)SendMessage(hwndHD,WM_GETFONT,0,0);
	hFontOld = (HFONT)SelectObject(hdc,hFont);

	SIZE size = {0};
	GetTextExtentPoint32(hdc,sz,(int)wcslen(sz),&size);

	SelectObject(hdc,hFontOld);

	ReleaseDC(hwndHD,hdc);

	// +------------------+
	// |<-6->|<-cx->|<-6->|
	// +------------------+
	size.cx = size.cx + 6 + 6;
	int cx;
	if( dwFlags & LVEXCHTF_ADJUST_WIDTH_BY_COLUMN_ITEM_TEXT )
	{
		cx = size.cx;
		ListView_SetColumnWidth(hwndLV,iColumn,LVSCW_AUTOSIZE);
		int cxCol = 0;
		int cItems = ListView_GetItemCount(hwndLV);
		for(int i = 0; i < cItems; i++)
		{
	        cxCol = ListView_GetColumnWidth(hwndLV,iColumn);

			if( cxCol > size.cx )
			{
				cx = LVSCW_AUTOSIZE;
				break;
			}
		}
	}
	else
	{
		cx = size.cx;
	}

	ListView_SetColumnWidth(hwndLV,iColumn,cx);

	return cx;
}

int
ListViewEx_SetLastColumnWidth(
	HWND hwndLV,
	DWORD dwFlags
	)
{
    HWND hwndHD = ListView_GetHeader(hwndLV);
    int cHeaders = Header_GetItemCount(hwndHD);
	if( cHeaders == 0 )
		return false;
	return ListViewEx_SetColumnWidthByHeaderText(hwndLV,(cHeaders - 1),dwFlags);
}

int
ListViewEx_GetColumnIndexFromColumnId(
	HWND hwndLV,
	INT_PTR ColumnId
	)
{
    HWND hwndHD = ListView_GetHeader(hwndLV);
	int i,cColumns = Header_GetItemCount(hwndHD);

	for(i = 0; i < cColumns; i++)
	{
		INT_PTR id = (INT_PTR)ListViewEx_GetHeaderItemData(hwndHD,i);
		if( id == ColumnId )
		{
			return i;
		}
	}
	return -1;
}

VOID
DrawListViewColumnMeter(
	HDC hdc,
	HWND hWndList,
	int iItem,
	int iMeterColumn,
	RECT *prcRect,
	HFONT hTextFont,
	double DiskUsage,
	UINT fMeterStyle
	)
{
	RECT rcSubItem;
	ListView_GetSubItemRect(hWndList,iItem,iMeterColumn,LVIR_LABEL,&rcSubItem);

	RECT rcMeter = rcSubItem;
	rcMeter.top    += 2;
	rcMeter.bottom -= 2;
	rcMeter.left   += (iMeterColumn == 0 ? 2 : 6);
	rcMeter.right  -= 6;

	double width = (double)_RECT_WIDTH(rcMeter);
	if( (DiskUsage * 100.0) == 0.0 )
	{
		width = 0.0;
	}
	else
	{
		width *= DiskUsage;

		if( width < 2.0 )
			width = 2.0;
	}

	if( fMeterStyle == 0 )
	{
		//
		// draw skeleton meter style
		//
		COLORREF crText  = RGB(60,72,74);
		COLORREF crMeter = RGB(120,190,250);
		HBRUSH hbr = CreateSolidBrush( crMeter );
		HBRUSH hbrBack = CreateSolidBrush( ListView_GetBkColor(hWndList) );
		// draw meter frame
		FillRect(hdc,&rcMeter,hbrBack);
		FrameRect(hdc,&rcMeter,hbr);

		RECT rcLevel = rcMeter;

		int nWidth = (int)width;
		int nPer = (int)(DiskUsage * 100.0);
		//
		// To prevent looks like the 100% meter view when actually 99%.
		// It looks like that because the frame is drawn.
		//
		if( ((rcMeter.left + nWidth) == (rcLevel.right-1)) && (nPer < 100) )
			rcLevel.right-=2;
		else
			rcLevel.right = rcMeter.left + (int)width;
		FillRect(hdc,&rcLevel,hbr);

		COLORREF crPrev = ::SetTextColor(hdc,crText);
		WCHAR sz[MAX_PATH];

		ListView_GetItemText(hWndList,iItem,iMeterColumn,sz,_countof(sz));

		HFONT hFont;
		if( hTextFont )
			hFont = hTextFont;
		else
			hFont = (HFONT)SendMessage(hWndList,WM_GETFONT,0,0);

		SelectObject(hdc,hFont);
		SetBkMode(hdc,TRANSPARENT);
		RECT rcText = rcMeter;
		rcText.right -= 2;
		DrawText(hdc,sz,-1,&rcText,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
		SetTextColor(hdc,crPrev);

		DeleteObject(hbr);
		DeleteObject(hbrBack);
	}
	else if( fMeterStyle == 2 )
	{
		//
		// draw fill meter style
		//
		COLORREF crMeter = RGB(120,190,250);
		HBRUSH hbr = CreateSolidBrush( crMeter );

		HDC hdcMem1 = CreateCompatibleDC(hdc);
		HDC hdcMem2 = CreateCompatibleDC(hdc);
		HBITMAP hbmp1 = CreateCompatibleBitmap(hdc,_RECT_WIDTH(rcMeter),_RECT_HIGHT(rcMeter));
		HBITMAP hbmp2 = CreateCompatibleBitmap(hdc,_RECT_WIDTH(rcMeter),_RECT_HIGHT(rcMeter));
		HBITMAP hbmpOld1 = (HBITMAP)::SelectObject(hdcMem1,hbmp1);
		HBITMAP hbmpOld2 = (HBITMAP)::SelectObject(hdcMem2,hbmp2);

		RECT box;
		box.left   = 0;
		box.top    = 0;
		box.right  = _RECT_WIDTH(rcMeter);
		box.bottom = _RECT_HIGHT(rcMeter);

		FrameRect(hdcMem1,&box,hbr);
		FillRect(hdcMem2,&box,hbr);

		WCHAR sz[MAX_PATH];

		ListView_GetItemText(hWndList,iItem,iMeterColumn,sz,_countof(sz));

		RECT rcText = box;
		rcText.right -= 2;

		HFONT hFont;
		if( hTextFont )
			hFont = hTextFont;
		else
			hFont = (HFONT)SendMessage(hWndList,WM_GETFONT,0,0);

		HFONT hfontOld;
		hfontOld = (HFONT)SelectObject(hdcMem1,hFont);
		SetTextColor(hdcMem1,RGB(255,255,255));
		SetBkMode(hdcMem1,TRANSPARENT);
		DrawText(hdcMem1,sz,-1,&rcText,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
		SelectObject(hdcMem1,hfontOld);

		hfontOld = (HFONT)SelectObject(hdcMem2,hFont);
		SetTextColor(hdcMem2,RGB(33,33,33));
		SetBkMode(hdcMem2,TRANSPARENT);
		DrawText(hdcMem2,sz,-1,&rcText,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
		SelectObject(hdcMem2,hfontOld);

		//
		// BitBit meter parts bitmap
		//

		// base
		BitBlt(hdc,
			rcMeter.left,
			rcMeter.top,
			_RECT_WIDTH(rcMeter),
			_RECT_HIGHT(rcMeter),
			hdcMem1,0,0,SRCCOPY);

		// color level
		rcMeter.right = rcMeter.left + (int)width;
		BitBlt(hdc,
			rcMeter.left,
			rcMeter.top,
			_RECT_WIDTH(rcMeter),
			_RECT_HIGHT(rcMeter),
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
		//
		// draw fill meter style
		//
		COLORREF crMeter = RGB(80,120,220);
		HBRUSH hbr = CreateSolidBrush( crMeter );

		HDC hdcMem1 = CreateCompatibleDC(hdc);
		HDC hdcMem2 = CreateCompatibleDC(hdc);
		HBITMAP hbmp1 = CreateCompatibleBitmap(hdc,_RECT_WIDTH(rcMeter),_RECT_HIGHT(rcMeter));
		HBITMAP hbmp2 = CreateCompatibleBitmap(hdc,_RECT_WIDTH(rcMeter),_RECT_HIGHT(rcMeter));
		HBITMAP hbmpOld1 = (HBITMAP)::SelectObject(hdcMem1,hbmp1);
		HBITMAP hbmpOld2 = (HBITMAP)::SelectObject(hdcMem2,hbmp2);

		RECT box;
		box.left   = 0;
		box.top    = 0;
		box.right  = _RECT_WIDTH(rcMeter);
		box.bottom = _RECT_HIGHT(rcMeter);

		FillRect(hdcMem1,&box,GetSysColorBrush(COLOR_MENUBAR));
		FillRect(hdcMem2,&box,hbr);

		WCHAR sz[MAX_PATH];

		ListView_GetItemText(hWndList,iItem,iMeterColumn,sz,_countof(sz));

		RECT rcText = box;
		rcText.right -= 2;

		HFONT hFont;
		if( hTextFont )
			hFont = hTextFont;
		else
			hFont = (HFONT)SendMessage(hWndList,WM_GETFONT,0,0);

		HFONT hfontOld;
		hfontOld = (HFONT)SelectObject(hdcMem1,hFont);
		SetTextColor(hdcMem1,RGB(0,0,0));
		SetBkMode(hdcMem1,TRANSPARENT);
		DrawText(hdcMem1,sz,-1,&rcText,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
		SelectObject(hdcMem1,hfontOld);

		hfontOld = (HFONT)SelectObject(hdcMem2,hFont);
		SetTextColor(hdcMem2,RGB(255,255,255));
		SetBkMode(hdcMem2,TRANSPARENT);
		DrawText(hdcMem2,sz,-1,&rcText,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
		SelectObject(hdcMem2,hfontOld);

		//
		// BitBit meter parts bitmap
		//

		// base
		BitBlt(hdc,
			rcMeter.left,
			rcMeter.top,
			_RECT_WIDTH(rcMeter),
			_RECT_HIGHT(rcMeter),
			hdcMem1,0,0,SRCCOPY);

		// color level
		rcMeter.right = rcMeter.left + (int)width;
		BitBlt(hdc,
			rcMeter.left,
			rcMeter.top,
			_RECT_WIDTH(rcMeter),
			_RECT_HIGHT(rcMeter),
			hdcMem2,0,0,SRCCOPY);

		SelectObject(hdcMem1,hbmpOld1);
		SelectObject(hdcMem2,hbmpOld2);

		DeleteObject(hbmp1);
		DeleteObject(hbmp2);

		DeleteDC(hdcMem1);
		DeleteDC(hdcMem2);
		DeleteObject(hbr);
	}
}
