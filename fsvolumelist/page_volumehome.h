#pragma once
//***************************************************************************
//*                                                                         *
//*  page_volumehome.h                                                      *
//*                                                                         *
//*  Page Tempate / Dummy Page                                              *
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
class CVolumeHomeView : public CPageWndBase
{
public:
	CVolumeHomeView()
	{
	}

	~CVolumeHomeView()
	{
	}

	LRESULT OnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd,&ps);
		RECT rc;
		GetClientRect(hWnd,&rc);
		FillRect(hdc,&rc,GetSysColorBrush(COLOR_3DFACE));
		EndPaint(hWnd,&ps);
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_CREATE:
				SetWindowText(hWnd,L"");
				return 0;
			case WM_PAINT:
				return OnPaint(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}
};
