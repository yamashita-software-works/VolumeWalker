//****************************************************************************
//
//  longpathbox.cpp
//
//  Long Path Displayes Custom Control.
//
//  Author:  YAMASHITA Katsuhiro
//
//  History: 2015-01-20 Created
//           2024-06-20 Ported
//
//****************************************************************************
#include "stdafx.h"
#include "longpathbox.h"

/////////////////////////////////////////////////////////////////////////////
// LongPathBox Window Class Registration

static LRESULT CALLBACK DefLongPathBoxProc(HWND,UINT,WPARAM,LPARAM);

void InitLongPathBox(HMODULE hModule)
{
	WNDCLASS wndcls = {0};
    wndcls.style         = CS_GLOBALCLASS|CS_OWNDC|CS_VREDRAW|CS_HREDRAW|CS_SAVEBITS|CS_PARENTDC|CS_DBLCLKS;
	wndcls.lpfnWndProc   = DefLongPathBoxProc;
    wndcls.hInstance     = hModule;
	wndcls.hIcon         = 0;
    wndcls.hCursor       = 0;
    wndcls.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wndcls.lpszMenuName  = NULL;
	wndcls.lpszClassName = LPBC_LONGPATHBOX_NAME;
	RegisterClass(&wndcls);
}

/////////////////////////////////////////////////////////////////////////////
// Custom control

typedef struct _LONGPATHBOXSTRUCT
{
	COLORREF crBack;
	COLORREF crText;
	COLORREF crBorder;
	COLORREF crSelect;
	COLORREF crSelectText;
	HFONT hFont; // Read only, the control does not delete it.
	PWSTR pszPath;
	BOOL bFocus;
	DWORD dwStyle;
	HBRUSH hbrBack;
	int xMarginL;
	int xMarginR;
	ULONG State;
} LONGPATHBOXSTRUCT;

static void drawPaint(HWND hwnd,HDC hdc,LONGPATHBOXSTRUCT *pBox)
{
	HDC hdcMem;
	HBITMAP hBmp,hOrgBmp;

	RECT rcClient,rc;
	GetClientRect(hwnd,&rcClient);

	PWSTR pszPath = pBox->pszPath;
	HFONT hFont = pBox->hFont;

	hdcMem = CreateCompatibleDC(hdc);
	hBmp = CreateCompatibleBitmap(hdc,_RECT_WIDTH(rcClient),_RECT_HIGHT(rcClient));

	hOrgBmp = (HBITMAP)SelectObject(hdcMem,hBmp);

	// Base color
	FillRect(hdcMem,&rcClient,pBox->hbrBack);

	// Directory Path Area
	rc = rcClient;
	InflateRect(&rc,0,-2);
	rc.left = 0;
	FillRect(hdcMem,&rc,pBox->hbrBack);

	// Display Path String 
	if( pszPath )
	{
		HFONT hfontOld;
		hfontOld = (HFONT)SelectObject(hdcMem,hFont);

		TEXTMETRIC tm;
		GetTextMetrics(hdcMem,&tm);

		int cyMargin;
		cyMargin = ((rcClient.bottom-rcClient.top)-tm.tmHeight)/2;

		ABC abc;
		int cxEllipsis;
		GetCharABCWidths(hdcMem,L'.',L'.',&abc);
		cxEllipsis = (abc.abcA + abc.abcB + abc.abcC) * 3;

		int cchIndex = 0;
		int cx = 0;
		int cxWidth;

		rc = rcClient;
		cxWidth = (rc.right - rc.left);

		cxWidth -= pBox->xMarginL;
		cxWidth -= pBox->xMarginR;

		BOOL bEllipsis = FALSE;
		PWSTR p = pszPath;
		int cchPath = (int)wcslen( pszPath );
		int cxCurDir = 0;
		int posCurDir = cchPath - 1;
		int cchCurDir = 0;

		if( pBox->dwStyle & LPBS_NO_ELLIPSIS )
		{
			// No ellipsis
			//
			cchIndex = cchPath;
		}
		else if( pBox->dwStyle & LPBS_END_ELLIPSIS )
		{
			// Add an ellipsis at the end.
			//
			while( p[cchIndex] )
			{
				GetCharABCWidths(hdcMem,p[cchIndex],p[cchIndex],&abc);
				cx += (abc.abcA + abc.abcB + abc.abcC);
				if( cx >= cxWidth )
				{
					do
					{
						GetCharABCWidths(hdcMem,p[cchIndex],p[cchIndex],&abc);
						cx -= (abc.abcA + abc.abcB + abc.abcC);
						cchIndex--;
					}
					while( cx > (cxWidth - cxEllipsis) );

					SIZE size;
					GetTextExtentPoint32(hdcMem,pszPath,cchIndex,&size);
					cx = size.cx;

					bEllipsis = TRUE;
					break;
				}
				cchIndex++;
			}
		}
		else
		{
			// Displays the last directory in the path (the current directory) 
			// with an ellipsis between them.
			//

			while( p[cchIndex] )
			{
				GetCharABCWidths(hdcMem,p[cchIndex],p[cchIndex],&abc);
				cx += (abc.abcA + abc.abcB + abc.abcC);
				if( cx > (cxWidth - 2) ) 
				{
					// Get current directory start pos, width and length.
					// "\xxx\yyy\zzz
					//          ^---
					while( p[posCurDir] != L'\\' && posCurDir >= 0)
					{
						GetCharABCWidths(hdcMem,p[posCurDir],p[posCurDir],&abc);
						cxCurDir += (abc.abcA + abc.abcB + abc.abcC);
						posCurDir--;
					}
					GetCharABCWidths(hdcMem,p[posCurDir],p[posCurDir],&abc);
					cxCurDir += (abc.abcA + abc.abcB + abc.abcC);
					cchCurDir = cchPath - posCurDir;

					// Trim trail characters.
					//
					do
					{
						GetCharABCWidths(hdcMem,p[cchIndex],p[cchIndex],&abc);
						cx -= (abc.abcA + abc.abcB + abc.abcC);
						cchIndex--;
					}
					while( (cchIndex >= 0) && (cx > (cxWidth - cxEllipsis - cxCurDir)) );

					if( cchIndex > 0)
					{
						SIZE size;
						GetTextExtentPoint32(hdcMem,pszPath,cchIndex,&size);
						cx = size.cx + 1;
					}
					else
					{
						// If only the current directory ("...\zzz") can be displayed,
						// and the width becomes narrower so that all characters cannot be displayed,
						// the width is calculated so that the end of the text is truncated.
						//
						cx = 0;

						int pos = posCurDir; 
						cxCurDir = 0;
						cchCurDir = 0;
						while( p[pos] )
						{
							GetCharABCWidths(hdcMem,p[pos],p[pos],&abc);
							cxCurDir += (abc.abcA + abc.abcB + abc.abcC);
							if( cxCurDir > (cxWidth + 4) )
							{
								break;
							}
							cchCurDir++;
							pos++;
						}
					}
					bEllipsis = TRUE;
					break;
				}
				cchIndex++;
			}
		}

		if( pBox->dwStyle & LPBS_NO_TEXTSELECTION )
		{
			SetTextColor(hdcMem, pBox->crText);
			SetBkColor(hdcMem, pBox->crBack);
		}
		else
		{
			COLORREF crText;
			COLORREF crBack;

			crBack = pBox->bFocus ? pBox->crSelect : pBox->crBack;
			crText = pBox->bFocus ? pBox->crSelectText : pBox->crText;

			if( pBox->State & LPBSTATE_SELECT && !pBox->bFocus )
				crBack = GetSysColor(COLOR_3DSHADOW);

			SetTextColor(hdcMem, crText);
			SetBkColor(hdcMem, crBack);
		}

		rc.left  += pBox->xMarginL;
		rc.right -= pBox->xMarginR;
		rc.top += cyMargin;
		if( cchIndex > 0 )
			TextOut(hdcMem,rc.left,rc.top,pszPath,cchIndex);
		if( bEllipsis )
		{
			ExtTextOut(hdcMem,rc.left+cx,rc.top,ETO_CLIPPED,&rc,L"...",3,NULL);
			ExtTextOut(hdcMem,rc.left+cx+cxEllipsis,rc.top,ETO_CLIPPED,&rc,&pszPath[posCurDir],cchCurDir,NULL);
		}

		SelectObject(hdcMem,hfontOld);
	}

	// Border
	if( pBox->dwStyle & LPBS_FLAT_BORDER )
	{
		rc = rcClient;
		HBRUSH hbr = CreateSolidBrush( pBox->crBorder );
		FrameRect(hdcMem,&rc,hbr);
		DeleteObject(hbr);
	}

	BitBlt(hdc,
		0,0,
		_RECT_WIDTH(rcClient),
		_RECT_HIGHT(rcClient),
		hdcMem,0,0,SRCCOPY);

	SelectObject(hdcMem,hOrgBmp);

	DeleteObject(hBmp);
	DeleteDC(hdcMem);
}

static void	copyClipboard(HWND hWnd,LONGPATHBOXSTRUCT *pBox)
{
	if( OpenClipboard(hWnd) )
	{
		EmptyClipboard();

		SIZE_T cch = (wcslen(pBox->pszPath) + 1);
		SIZE_T cb = cch * sizeof(WCHAR);

		HGLOBAL hgbl = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,cb);

		if( hgbl )
		{
			PWSTR psz = (PWSTR)GlobalLock(hgbl);

			wcscpy_s(psz,cch,pBox->pszPath);

			GlobalUnlock(hgbl);

			SetClipboardData(CF_UNICODETEXT,hgbl);
		}

		CloseClipboard();
	}

}

static void notifyEvent(HWND hWnd,LONGPATHBOXSTRUCT *pBox,UINT uMessage)
{
	HWND hwndParent = GetParent(hWnd);
	USHORT id = (USHORT)GetWindowLong(hWnd,GWL_ID);
	USHORT code;
	switch(uMessage) 
	{
		case WM_LBUTTONDOWN:
			code = LPBN_CLICKED;
			break;
		case WM_LBUTTONDBLCLK:
			code = LPBN_DBLCLK;
			break;
		case WM_SETFOCUS:
			code = LPBN_SETFOCUS;
			break;
		case WM_KILLFOCUS:
			code = LPBN_KILLFOCUS;
			break;
		default:
			return ;
	}
	SendMessage(hwndParent,WM_COMMAND,MAKEWPARAM(id,code),(LPARAM)hWnd);
}

LRESULT CALLBACK DefLongPathBoxProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam)
{
	LONGPATHBOXSTRUCT *pBox = (LONGPATHBOXSTRUCT *)GetWindowLongPtr(hWnd,GWLP_USERDATA);

	switch( uMessage )
	{
		case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT ps;

			// Do shaking another window on a control can cause parts of the control to be clipped,
			// invalidate it completely before calling BeginPaint().
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);

			hdc = BeginPaint(hWnd,&ps);

			drawPaint(hWnd,hdc,pBox);

			EndPaint(hWnd,&ps);
			break;
		}
		case WM_ERASEBKGND:
		{
			return TRUE;
		}
		case WM_CREATE:
		{
			pBox = (LONGPATHBOXSTRUCT *)_MemAllocZero( sizeof(LONGPATHBOXSTRUCT) );
			pBox->crBack = GetSysColor(COLOR_3DFACE);
			pBox->crText = GetSysColor(COLOR_BTNTEXT);
			pBox->crBorder = GetSysColor(COLOR_3DSHADOW);
			pBox->crSelect = GetSysColor(COLOR_HIGHLIGHT);
			pBox->crSelectText = GetSysColor(COLOR_HIGHLIGHTTEXT);
			pBox->hFont = NULL;
			pBox->pszPath = NULL;
			pBox->hbrBack = CreateSolidBrush( pBox->crBack );
			pBox->dwStyle = GetWindowLong(hWnd,GWL_STYLE);
			if( pBox->dwStyle & LPBS_FLAT_BORDER )
			{
				pBox->xMarginL = 4;
				pBox->xMarginR = 4;
			}
			else
			{
				pBox->xMarginL = 0;
				pBox->xMarginR = 0;
			}
			SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)pBox);
			break;
		}
		case WM_DESTROY:
		{
			DeleteObject(pBox->hbrBack);
			_SafeMemFree(pBox->pszPath);
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
		case LPBM_GETINFO:
		{
			LONGPATHBOXINFO *pInfo = (LONGPATHBOXINFO *)lParam;
			pInfo->crBack = pBox->crBack;
			pInfo->crText = pBox->crText;
			pInfo->crBorder = pBox->crBorder;
			return 0;
		}
		case LPBM_SETINFO:
		{
			LONGPATHBOXINFO *pInfo = (LONGPATHBOXINFO *)lParam;
			pBox->crBack = pInfo->crBack;
			pBox->crText = pInfo->crText;
			pBox->crBorder = pInfo->crBorder;
			if( pBox->hbrBack == NULL )
				DeleteObject( pBox->hbrBack );
			pBox->hbrBack = CreateSolidBrush( pBox->crBack );
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
			return 0;
		}
		case LPBM_SETMARGIN:
		{
			pBox->xMarginL = LOWORD(wParam);
			pBox->xMarginR = HIWORD(wParam);
			return 0;
		}
		case LPBM_SETSTATE:
		{
			pBox->State = LOWORD(wParam);
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
			return 0;
		}
		case LPBM_GETSTATE:
		{
			return (LRESULT)pBox->State;
		}
		case WM_SETTEXT:
		{
			if( lParam )
			{
				_SafeMemFree(pBox->pszPath);
				pBox->pszPath = _MemAllocString((PWSTR)lParam);
				DefWindowProc(hWnd,uMessage,wParam,lParam); // force SetWindowText()
				RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
			}
			return 0;
		}
		case WM_SETFONT:
		{
			pBox->hFont = (HFONT)wParam;
			break;
		}
		case WM_SETFOCUS:
		{
			pBox->bFocus = true;
			notifyEvent(hWnd,pBox,uMessage);
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
			break;
		}
		case WM_KILLFOCUS:
		{
			pBox->bFocus = false;
			notifyEvent(hWnd,pBox,uMessage);
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
			break;
		}
		case WM_LBUTTONDOWN:
		{
			if( (GetWindowLong(hWnd,GWL_STYLE) & LPBS_NO_FOCUS) == 0 )
				SetFocus(hWnd);
			notifyEvent(hWnd,pBox,uMessage);
			break;
		}
		case WM_COPY:
		{
			copyClipboard(hWnd,pBox);
			break;
		}
		case WM_GETDLGCODE:
		{
			return DLGC_WANTCHARS;
		}
		case WM_CHAR:
		{
			if( GetKeyState(VK_CONTROL) < 0)
			{
				if( GetKeyState('C')  < 0 )
					copyClipboard(hWnd,pBox);
			}
			return 0;
		}
		case WM_LBUTTONDBLCLK:
		{
			notifyEvent(hWnd,pBox,uMessage);
			break;
		}
	}
	return DefWindowProc(hWnd,uMessage,wParam,lParam);
}
