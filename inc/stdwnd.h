//
// CStdWnd class
//
// NOTE:
// Legacy Code - created in 2001.
// This header is will be obsolete. Use only for backward compatibility.
//
#if !defined( _STDWND )
#define _STDWND

class CStdWnd
{
public:
	HWND m_hWnd;

	CStdWnd()
	{
		m_hWnd = NULL;
	}

	virtual ~CStdWnd()
	{
	}

	inline void Attach(HWND hWnd)
	{
		m_hWnd = hWnd;
	}

	inline DWORD GetStyle()
	{
		return GetWindowLong(m_hWnd,GWL_STYLE);
	}

	inline void SetStyle(DWORD dwStyle)
	{
		SetWindowLong(m_hWnd,GWL_STYLE,dwStyle);
	}

	inline void Redraw(BOOL bDraw)
	{
		::SendMessage(m_hWnd,WM_SETREDRAW,(WPARAM)bDraw,0);
	}

	inline void SetFont(HFONT hFont)
	{
		::SendMessage(m_hWnd,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
	}

	inline HFONT GetFont(void)
	{
		return (HFONT)::SendMessage(m_hWnd,WM_GETFONT,0,0);
	}

	inline UINT GetWindowId()
	{
		return (UINT)GetWindowLong(m_hWnd,GWL_ID);
	}

	inline DWORD GetExStyle()
	{
		return GetWindowLong(m_hWnd,GWL_EXSTYLE);
	}

	inline void SetExStyle(DWORD dwStyle)
	{
		SetWindowLong(m_hWnd,GWL_EXSTYLE,dwStyle);
	}

	inline BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0)
	{
		DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
		DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
		if(dwStyle == dwNewStyle)
			return FALSE;

		::SetWindowLong(m_hWnd, GWL_STYLE, dwNewStyle);
		if(nFlags != 0)
		{
			::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
				SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
		}

		return TRUE;
	}
	inline BOOL ModifyStyleEx(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0)
	{
		DWORD dwStyle = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
		DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
		if(dwStyle == dwNewStyle)
			return FALSE;

		::SetWindowLong(m_hWnd, GWL_EXSTYLE, dwNewStyle);
		if(nFlags != 0)
		{
			::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
				SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
		}

		return TRUE;
	}
};

#endif