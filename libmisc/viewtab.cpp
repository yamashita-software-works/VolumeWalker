//****************************************************************************
//*                                                                          *
//*  ViewTab.cpp                                                             *
//*                                                                          *
//*  CLASS:   CViewTab                                                       *
//*                                                                          *
//*  PURPOSE:                                                                *
//*                                                                          *
//*  HISTORY: 2000.11.10 Create                                              *
//*                                                                          *
//*  AUTHOR:  Yamashita Katsuhiro                                            *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "ViewTab.h"

#define CONTAINER_VIEW_EXTRA  8
#define VIEW_THIS             0 
#define CLASS_NANE  TEXT("ViewTab")

#define CX_INNER_MARGIN       (8)
#define CY_INNER_MARGIN       (8)

CViewTab::CViewTab()
{
	m_cyViewTab = 0;
	m_SelectIndex = 0;
	m_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	m_hwndCmdOwner = NULL;
	m_hbrBackColor = NULL;
	InitDefaultColor();
}

CViewTab::~CViewTab()
{
	DeleteObject( m_hFont );

	RemoveAll();

	FreeColor();
}

HWND CViewTab::Create(HWND hwndOwner,HINSTANCE hInst,UINT nID,DWORD dwStyle)
{
	// Register window class
	WNDCLASS wc;

	wc.style         = 0;
	wc.lpszClassName = CLASS_NANE;
	wc.lpfnWndProc   = (WNDPROC)CViewTab::WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = CONTAINER_VIEW_EXTRA;
	wc.hbrBackground = (HBRUSH)NULL;
	wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
	wc.hIcon         = 0;
	wc.hInstance     = hInst;
	wc.lpszMenuName  = 0;
	
	RegisterClass( &wc );

	if( dwStyle & TABS_TOP )
		m_SidePos = TABS_TOP;
	else
		m_SidePos = TABS_BOTTOM;

	dwStyle &= ~TABS_MASK;

	dwStyle |= WS_CHILD;

	m_hWnd = CreateWindowEx( 0,
							 CLASS_NANE,TEXT(""),dwStyle,
							 0,0,0,0,hwndOwner,(HMENU)(ULONG_PTR)nID,hInst,(PVOID)this) ;

	return m_hWnd;
}

LRESULT CALLBACK CViewTab::WndProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam)
{
	CViewTab* pWndProc = (CViewTab*)(LONG_PTR)GetWindowLongPtr(hWnd,VIEW_THIS);
	if( pWndProc == 0 )
	{
		if( uMessage == WM_NCCREATE )
		{
			ASSERT(((LPCREATESTRUCT)lParam)->lpCreateParams != NULL);
			PVOID pThis = (CViewTab*)(((LPCREATESTRUCT)lParam)->lpCreateParams); 
			SetWindowLongPtr(hWnd,VIEW_THIS,(LONG_PTR)pThis);
			pWndProc = (CViewTab*)pThis;
			pWndProc->m_hWnd = hWnd;
		}
	}

	if( pWndProc == 0 )
		return DefWindowProc(hWnd,uMessage,wParam,lParam);

	return pWndProc->WindowProc(hWnd,uMessage,wParam,lParam);
}

LRESULT CViewTab::WindowProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam)
{
	m_hWnd = hWnd;
	switch( uMessage )
	{
		case WM_CREATE:
			return OnCreate(hWnd,wParam,lParam);
		case WM_LBUTTONDOWN:
			return OnLButtonDown(hWnd,wParam,lParam);
		case WM_PAINT:
			return OnPaint(hWnd,wParam,lParam);
		case WM_SIZE:
			return OnSize(hWnd,wParam,lParam);
		case WM_SETFONT:
			SetFont( (HFONT)wParam );
			break;
		case WM_SYSCOLORCHANGE:
			UpdateColor();
			break;
	}

	return DefWindowProc(hWnd,uMessage,wParam,lParam);
}

LRESULT CViewTab::OnCreate(HWND hWnd,WPARAM wParam,LPARAM lParam)
{
	CalcHeight();
	return 0;
}

LRESULT CViewTab::OnSize(HWND hWnd,WPARAM wParam,LPARAM lParam)
{
	InvalidateRect(hWnd,NULL,TRUE);
	UpdateWindow(hWnd);
	return 0;
}

LRESULT CViewTab::OnLButtonDown(HWND hWnd,WPARAM wParam,LPARAM lParam)
{
	int i;
	int c;

	c = m_tab.GetCount();

	POINT pt;
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);

	RECT rc;
	for(i = 0; i < c; i++)
	{
		if( m_tab.GetAt(i) == NULL )
		{
			ASSERT(FALSE);
			break;
		}
		
		rc = m_tab.GetAt(i)->rc;

		if( PtInRect(&rc,pt) )
		{
			SetSelectItem(i);

			SendMessage(m_hwndCmdOwner,WM_COMMAND,MAKEWPARAM((WORD)m_tab.GetAt(i)->nCmdID,0),0);
		}
	}

	return 0;
}

LRESULT CViewTab::OnPaint(HWND hWnd,WPARAM wParam,LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	int i;
	int c;
	RECT rc;

	HDC hdcClient;
	HBITMAP hbmpMem;
	HBITMAP hbmpOld;

	hdcClient = BeginPaint(hWnd,&ps);

	GetClientRect(hWnd,&rc);

	hdc  = CreateCompatibleDC(hdcClient);
	hbmpMem = CreateCompatibleBitmap(hdcClient,rc.right-rc.left,rc.bottom-rc.top);

	hbmpOld = (HBITMAP)SelectObject(hdc,hbmpMem);

	FillRect(hdc,&rc,m_hbrBackColor);

	// draw top line
	HPEN hpen,hpenOld;
	hpen = CreatePen(PS_SOLID,1,m_crShadowBorder);
	hpenOld = (HPEN)SelectObject(hdc,hpen);
	if( m_SidePos == TABS_BOTTOM ) {
		MoveToEx(hdc,rc.right-2,0,0);
		LineTo(hdc,0,0);
	} else {
		MoveToEx(hdc,0,this->m_cyViewTab-1,NULL);
		LineTo(hdc,rc.right,this->m_cyViewTab-1);
	}
	SelectObject(hdc,hpenOld);
	DeleteObject(hpen);

	// draw buttons
	c = m_tab.GetCount();
	for(i = 0; i < c; i++)
	{
		if( i == m_SelectIndex )
			DrawSelectTab(hdc,i);
		else
			DrawNormalTab(hdc,i);
	}

	// copy MemDC to ClientDC
	BitBlt(hdcClient,
			rc.left,rc.top,
			rc.right,rc.bottom,
			hdc,0,0,SRCCOPY);

	SelectObject(hdc,hbmpOld);
	DeleteObject(hbmpMem);
	DeleteDC(hdc);
	EndPaint(hWnd,&ps);
	return 0;
}

void
GetTextSize(
	HWND hwnd,
	HFONT hFont,
	LPCTSTR pszText,
	SIZE *pSize
	)
{
	HDC hdc = GetDC(hwnd);
	HFONT hFontOld;

	hFontOld = (HFONT)SelectObject(hdc,hFont);

	GetTextExtentPoint32(hdc,pszText,lstrlen(pszText),pSize);

	SelectObject(hdc,hFontOld);

	ReleaseDC(hwnd,hdc);
}

void CViewTab::CalcHeight()
{
	TEXTMETRIC tm;
	HDC hdc;
	HFONT hold;
	hdc = GetDC(m_hWnd);
	hold = (HFONT)SelectObject(hdc,m_hFont);
	GetTextMetrics(hdc,&tm);
	SetHeight( tm.tmHeight + 1 + 1 + CY_INNER_MARGIN + CY_INNER_MARGIN ); // border + inner margin
	SelectObject(hdc,hold);
	ReleaseDC(m_hWnd,hdc);
}

void CViewTab::SetSelectItem(int index)
{
	int c = m_tab.GetCount();

	if( m_SelectIndex != index && ((index >= 0) && (index <= c)) )
	{
		m_SelectIndex = index;

		InvalidateRect(m_hWnd,NULL,TRUE);
		UpdateWindow(m_hWnd);
	}
}

int CViewTab::GetSelectItem(void)
{
	return m_SelectIndex;
}

int CViewTab::CommandIdToIndex(UINT nCmdID)
{
	int i,c;

	c = m_tab.GetCount();

	for(i = 0; i < c; i++)
	{
		if( m_tab.GetAt(i)->nCmdID == nCmdID )
			return i;
	}

	return -1;
}

int CViewTab::InsertItem(int index,UINT nCmdID,LPCTSTR pszText)
{
	int c;
	int cxUse = 0;

	c = m_tab.GetCount();

	if( c != 0 )
	{
		cxUse = m_tab.GetAt(c-1)->rc.right;
	}

	RECT rcClient;
	GetClientRect(m_hWnd,&rcClient);

	PVIEWTABITEM pTab = new VIEWTABITEM;

	pTab->pszText = new TCHAR[lstrlen(pszText)+1];
	lstrcpy((LPTSTR)pTab->pszText,pszText);

	SIZE size;
	GetTextSize(m_hWnd,m_hFont,pszText,&size);

	pTab->rc.top    = 0;
	pTab->rc.bottom = m_cyViewTab;
	pTab->rc.left   = cxUse;
	pTab->rc.right  = pTab->rc.left + size.cx + (1 + 1) + (CX_INNER_MARGIN + CX_INNER_MARGIN); // R/L border line, inner margin
	pTab->nCmdID    = nCmdID;

	m_tab.Add( pTab );

	return 0;
}

void CViewTab::RemoveAll()
{
	int i,c;

	c = m_tab.GetCount();

	for(i = 0; i < c; i++)
	{
		PVIEWTABITEM pTab = m_tab.GetAt(i);
		if( pTab->pszText )
			delete[] (void*)pTab->pszText;
		delete pTab;
	}

	m_tab.Free();
}

int CViewTab::GetWidth()
{
	int cx = 0;
	int i;
	for(i = 0; i < GetItemCount(); i++)
	{
		cx += GetTabWidth(i);
	}
	return cx;
}

int CViewTab::GetItemCount()
{
	return m_tab.GetCount();
}

int CViewTab::GetTabRect(int index,RECT *prc)
{
	if( index >= m_tab.GetCount() )
		return -1;

	PVIEWTABITEM p = m_tab.GetAt(index);
	if( p == NULL )
	{
		ASSERT(FALSE);
		return -1;
	}

	*prc = p->rc;

	return index;
}

int CViewTab::GetTabWidth(int index)
{
	RECT rc;
	if( GetTabRect(index,&rc) == -1 )
	{
		return -1;
	}	
	return (rc.right - rc.left);
}

int CViewTab::GetTabText(int index,LPTSTR pszText,int cchText)
{
	if( index >= m_tab.GetCount() )
		return 0;

	PVIEWTABITEM p = m_tab.GetAt(index);
	if( p == NULL )
	{
		ASSERT(FALSE);
		return 0;
	}

	lstrcpyn(pszText,p->pszText,cchText);
	return lstrlen(pszText);
}

void CViewTab::DrawSelectTab(HDC hdc,int index)
{
	PVIEWTABITEM pTab = m_tab.GetAt(index);
	if( pTab == NULL )
	{
		ASSERT(FALSE);
		return;
	}

	RECT rc;
	int cxIcon = 0;

	if( m_SidePos == TABS_BOTTOM )
	{
		// Bottom tab
		rc = pTab->rc;

		HBRUSH hbr = CreateSolidBrush(m_crFace);
		FillRect(hdc,&rc,hbr);
		DeleteObject(hbr);

		// Draw frame line
		//
		HPEN hpenShadow;
		HPEN hpenHilight;
		HPEN hpenOld;

		hpenShadow  = CreatePen(PS_SOLID,1,m_crTabShadow);
		hpenHilight = CreatePen(PS_SOLID,1,m_crHilightBorder);

		hpenOld = (HPEN)SelectObject(hdc,hpenHilight);

		rc = pTab->rc;
	
		MoveToEx(hdc,rc.left,rc.top,NULL);
		LineTo(hdc,rc.left,rc.bottom);

		SelectObject(hdc,hpenShadow);

		MoveToEx(hdc,rc.left+1,rc.bottom-1,NULL);
		LineTo(hdc,rc.right-1,rc.bottom-1);
		LineTo(hdc,rc.right-1,rc.top-1);

		SelectObject(hdc,hpenOld);

		DeleteObject(hpenShadow);
		DeleteObject(hpenHilight);
	}
	else
	{
		// Top Tab
		rc = pTab->rc;

		HBRUSH hbr = CreateSolidBrush(m_crFace);
		FillRect(hdc,&rc,hbr);
		DeleteObject(hbr);

		// Draw frame line
		//
		HPEN hpenShadow;
		HPEN hpenHilight;
		HPEN hpenOld;

		hpenShadow  = CreatePen(PS_SOLID,1,m_crTabShadow);
		hpenHilight = CreatePen(PS_SOLID,1,m_crHilightBorder);

		hpenOld = (HPEN)SelectObject(hdc,hpenHilight);

		// Hilight
		MoveToEx(hdc,rc.left,rc.bottom,NULL);
		LineTo(hdc,rc.left,rc.top);
		LineTo(hdc,rc.right-1,rc.top);
		LineTo(hdc,rc.right-1,rc.top+1);

		// Shadow
		SelectObject(hdc,hpenShadow);
		LineTo(hdc,rc.right-1,rc.bottom);

		SelectObject(hdc,hpenOld);

		DeleteObject(hpenShadow);
		DeleteObject(hpenHilight);
	}

	DrawTabText(hdc,pTab->pszText,rc,TRUE);
}


void CViewTab::DrawNormalTab(HDC hdc,int index)
{
	PVIEWTABITEM pTab = m_tab.GetAt(index);
	if( pTab == NULL )
	{
		ASSERT(FALSE);
		return;
	}

	RECT rc;
	rc = pTab->rc;

	if( m_SidePos == TABS_BOTTOM )
	{
		InflateRect(&rc,0,-1); // override top line
		FillRect(hdc,&rc,m_hbrBackColor);
	}
	else
	{
		InflateRect(&rc,-1,-1);
 		FillRect(hdc,&rc,m_hbrBackColor);
		OffsetRect(&rc,0,+1);
	}

	DrawTabText(hdc,pTab->pszText,rc,FALSE);
}

void CViewTab::DrawTabText(HDC hdc,LPCTSTR pszText,RECT& rc,BOOL bSelect)
{
	HFONT hFont,hOldFont;
	hFont = m_hFont;
	hOldFont = (HFONT)SelectObject(hdc,hFont);	

	COLORREF crBack;
	COLORREF crText;

	if( bSelect )
	{
		OffsetRect(&rc,0,1);
		crBack = m_crFace;
		crText = m_crSelectText;
	}
	else
	{
		crBack = m_crBack;
		crText = m_crText;
	}

	SetBkColor(hdc,crBack);
	SetTextColor(hdc,crText);

	DrawText(hdc,pszText,-1,&rc,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
	SelectObject(hdc,hOldFont);	
}

void CViewTab::UpdateColor()
{
	FreeColor();
	m_hbrBackColor = CreateSolidBrush(m_crBack);
}

void CViewTab::FreeColor()
{
	if( m_hbrBackColor )
		DeleteObject(m_hbrBackColor);
}

void CViewTab::SetColorScheme(
		COLORREF crFace,
		COLORREF crBack,
		COLORREF crSelectText,
		COLORREF crText,
		COLORREF crHilightBorder,
		COLORREF crShadowBorder,
		COLORREF crTabShadow
		)
{
	m_crFace          = crFace;
	m_crBack          = crBack;
	m_crSelectText    = crSelectText;
	m_crText          = crText;
	m_crHilightBorder = crHilightBorder;
	m_crShadowBorder  = crShadowBorder;
	m_crTabShadow     = crTabShadow;

	UpdateColor();
}

void CViewTab::InitDefaultColor()
{
	SetColorScheme(
		GetSysColor(COLOR_3DFACE),
		GetSysColor(COLOR_3DSHADOW),
		GetSysColor(COLOR_BTNTEXT),
		GetSysColor(COLOR_3DHILIGHT),
		GetSysColor(COLOR_3DHILIGHT),
		GetSysColor(COLOR_WINDOWFRAME),
		GetSysColor(COLOR_WINDOWFRAME)
		);
}
