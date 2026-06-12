// 2024-03-31
#pragma once

class CSimpleToolbar
{
public:
	HWND m_hWnd;
	HWND CreateSimpleToolbar(HWND hWndParent,TBBUTTON *tbButtons,int cButtons,DWORD dwStyle=0,DWORD dwExtendedStyle=TBSTYLE_EX_MIXEDBUTTONS)
	{
		const int cyMargin = 16;
		const int xPadding = 16;
		const int yPadding = 16;
		const HIMAGELIST himl = NULL; // todo: currently text toolbar only.
		PCWSTR pszTheme;
		int cyText = -1; // todo:

		if( dwStyle == 0 )
		{
			dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NODIVIDER | CCS_TOP | 
					TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_AUTOSIZE | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT;
		}

	    HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, dwStyle, 0, 0, 0, 0,
								hWndParent, NULL, _GetResourceInstance(), NULL);

	    if (hWndToolbar == NULL)
		{
			m_hWnd = NULL;
			return NULL;
		}

		if( _IsDarkModeEnabled() )
			pszTheme = L"DarkMode";
		else
			pszTheme = L"Toolbar";

		SendMessage(hWndToolbar,CCM_SETVERSION, (WPARAM) 6, 0); 
		SendMessage(hWndToolbar,TB_SETEXTENDEDSTYLE,0,dwExtendedStyle);
		SendMessage(hWndToolbar,TB_SETWINDOWTHEME,0,(LPARAM)pszTheme);
		SendMessage(hWndToolbar,TB_SETIMAGELIST, 0, (LPARAM)himl);
		SendMessage(hWndToolbar,TB_SETPADDING,DPI_SIZE_CX(xPadding),DPI_SIZE_CY(yPadding));
		SendMessage(hWndToolbar,TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 
		SendMessage(hWndToolbar,TB_ADDBUTTONS, (WPARAM)cButtons, (LPARAM)tbButtons);

		if( cyText == -1 )
		{
			HDC hdc;
			hdc = GetDC(hWndParent);
			TEXTMETRIC tm;
			GetTextMetrics(hdc,&tm);
			cyText = tm.tmHeight;
			ReleaseDC(hWndParent,hdc);
		}

		DWORD dw = (DWORD)::SendMessage(hWndToolbar,TB_GETBUTTONSIZE,0,0);
		SendMessage(hWndToolbar,TB_SETBUTTONSIZE,0,MAKELONG(LOWORD(dw),cyText+cyMargin));
		SendMessage(hWndToolbar,TB_AUTOSIZE,0,0);

	    // Tell the toolbar to resize itself, and show it.
		ShowWindow(hWndToolbar, TRUE);

		m_hWnd = hWndToolbar;

		return hWndToolbar;
	}
};