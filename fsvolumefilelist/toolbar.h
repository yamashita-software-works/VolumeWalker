// 2024-03-31,2025-11-06
#pragma once

class CSimpleToolbar
{
public:
	HWND m_hWnd;

public:
	HWND CreateSimpleToolbar(HWND hWndParent,TBBUTTON *tbButtons,int cButtons,DWORD dwFlags=0/*reserved*/)
	{
		const BOOL Transparent = TRUE;
		const int cyMargin = 16;
		const int xPadding = 16;
		const int yPadding = 16;
		const HIMAGELIST himl = NULL; // todo: currently text toolbar only.
		PCWSTR pszTheme = L"Toolbar";
		int cyText = -1; // todo:

	    HWND hWndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
		    WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
			CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE |
			TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | (Transparent ? TBSTYLE_TRANSPARENT : 0),
			0, 0, 0, 0,
	        hWndParent, NULL, _GetResourceInstance(), NULL);

	    if (hWndToolbar == NULL)
		{
			return NULL;
		}

		SendMessage(hWndToolbar,CCM_SETVERSION, (WPARAM) 6, 0); 
		SendMessage(hWndToolbar,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS|(Transparent ? 0 : TBSTYLE_EX_DOUBLEBUFFER));
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