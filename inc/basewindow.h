#pragma once

#define PROP_NAME_WNDOBJ  L"WndObjectPtr"
#define WC_CLASSNAME      L"BaseWindow"

inline PVOID GetBaseWindowObject(HWND hWnd)
{
	return (PVOID)GetProp(hWnd,PROP_NAME_WNDOBJ);
}

inline BOOL AttachBaseWindowObject(HWND hWnd,PVOID pwnd)
{
	return SetProp(hWnd,PROP_NAME_WNDOBJ,pwnd);
}

inline BOOL DetachBaseWindowObject(HWND hWnd)
{
	return SetProp(hWnd,PROP_NAME_WNDOBJ,NULL);
}

class CWindowHandle
{
public:
	HWND m_hWnd;

	HWND GetHwnd() const 
	{
		return m_hWnd;
	}

	CWindowHandle()
	{
	}

	CWindowHandle(HWND hWnd)
	{
		m_hWnd = hWnd;
	}
};

class CBaseWindow : public CWindowHandle
{
public:
	CBaseWindow()
	{
		m_hWnd = NULL;
	}

	virtual ~CBaseWindow()
	{
		
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}

	static LRESULT CALLBACK _wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( uMsg == WM_NCCREATE )
		{
			CREATESTRUCT*pcs = (CREATESTRUCT*)lParam;
			CBaseWindow *pc = (CBaseWindow *)pcs->lpCreateParams;
			pc->m_hWnd = hWnd;
			AttachBaseWindowObject(hWnd,pc);
			return DefWindowProc(hWnd,uMsg,wParam,lParam);
		}

		CBaseWindow *pwnd = (CBaseWindow *)GetBaseWindowObject(hWnd);

		LRESULT lResult = 0;
		if( pwnd )
		{
			lResult = pwnd->WndProc(hWnd,uMsg,wParam,lParam);

			if( uMsg == WM_NCDESTROY )
			{
				//pwnd->PostNcDestroy();
				delete pwnd;
				DetachBaseWindowObject(hWnd);
			}
		}

		return lResult;
	}

	static BOOL RegisterClass(HINSTANCE hInstance)
	{
		WNDCLASS wndClass;
	    wndClass.style         = 0;
		wndClass.cbClsExtra    = 0;
	    wndClass.cbWndExtra    = sizeof(LONG_PTR) + DLGWINDOWEXTRA;
		wndClass.hCursor       = LoadCursor(NULL,IDC_ARROW);
	    wndClass.hInstance     = hInstance;
		wndClass.lpfnWndProc   = CBaseWindow::_wndProc;
	    wndClass.hIcon         = NULL;
		wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	    wndClass.lpszMenuName  = NULL;
		wndClass.lpszClassName = WC_CLASSNAME;

		if(!::RegisterClass(&wndClass))
		{
			return FALSE;
	    }
		return TRUE;
	}

	virtual HWND Create(HWND hWnd,UINT id=0,PCWSTR pszTitle=NULL,DWORD dwStyle=WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,DWORD dwExStyle=0)
	{
		HINSTANCE hInstance;
	    hInstance = GETINSTANCE(hWnd);

		CBaseWindow::RegisterClass(hInstance);

		HWND hwnd = CreateWindowEx(dwExStyle,WC_CLASSNAME,pszTitle,dwStyle,0,0,0,0,hWnd,(HMENU)id,hInstance,(LPVOID)this);
		if( hwnd == NULL )
			return NULL;

		return hwnd;
	}
};
