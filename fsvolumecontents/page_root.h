#pragma once
//***************************************************************************
//*                                                                         *
//*  page_root.h                                                            *
//*                                                                         *
//*  Placeholder Page                                                       *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2022-04-04                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
class CRootView : public CPageWndBase
{
	PWSTR m_pszText;
	HFONT m_hFont;
public:
	CRootView()
	{
		m_pszText = NULL;
		m_hFont = NULL;
	}

	~CRootView()
	{
		_SafeMemFree( m_pszText );
		if( m_hFont )
			DeleteObject(m_hFont);
	}

	virtual HRESULT UpdateData(PVOID pv)
	{
		SELECT_ITEM *pFile = (SELECT_ITEM*)pv;

		_SafeMemFree(m_pszText);
		m_pszText = _MemAllocString(pFile->pszPath);

		RedrawWindow(m_hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);

		return S_OK;
	}

	void InitFont()
	{
		if( m_hFont )
		{
			DeleteObject(m_hFont);
			m_hFont = NULL;
		}

		LOGFONT lf;
		SystemParametersInfo(SPI_GETICONTITLELOGFONT,sizeof(LOGFONT),&lf,0);
		m_hFont = CreateFontIndirect(&lf);
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		InitFont();
		return 0;
	}

	LRESULT OnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HDC hdcMem;
		HBITMAP hBmp,hOrgBmp;
		PAINTSTRUCT ps;
		RECT rc;

		GetClientRect(hWnd,&rc);

		HDC hdc = BeginPaint(hWnd,&ps);

		hdcMem = CreateCompatibleDC(hdc);
		hBmp = CreateCompatibleBitmap(hdc,rc.right-rc.left,rc.bottom-rc.top);

		hOrgBmp = (HBITMAP)SelectObject(hdcMem,hBmp);

		HBRUSH hbr = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
		FillRect(hdcMem,&rc,hbr);
		DeleteObject(hbr);

		if( m_pszText )
		{
			HFONT hPrevFont = (HFONT)SelectObject(hdcMem,m_hFont);
			SetBkMode(hdcMem,TRANSPARENT);
			DrawText(hdcMem,m_pszText,-1,&rc,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
			SelectObject(hdcMem,hPrevFont);
		}

		BitBlt(hdc,
			0,0,
			rc.right-rc.left,
			rc.bottom-rc.top,
			hdcMem,0,0,SRCCOPY);

		SelectObject(hdcMem,hOrgBmp);

		DeleteObject(hBmp);
		DeleteDC(hdcMem);

		EndPaint(hWnd,&ps);

		return 0;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_PAINT:
				return OnPaint(hWnd,uMsg,wParam,lParam);
			case WM_ERASEBKGND:
				return (LRESULT)TRUE;
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}
};
