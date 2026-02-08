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

inline PVOID DetachBaseWindowObject(HWND hWnd)
{
	return RemoveProp(hWnd,PROP_NAME_WNDOBJ);
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
				delete pwnd;
				DetachBaseWindowObject(hWnd);
			}
		}

		return lResult;
	}

	static BOOL RegisterClass(HINSTANCE hInstance)
	{
		WNDCLASS wndClass;
	    wndClass.style         = CS_DBLCLKS;
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

#define DEFDIALOGRESID(id) UINT_PTR GetResId() const { return id; }

class CDialogWindow : public CWindowHandle
{
	typedef struct _INITDLGPARAM {
		CDialogWindow *pThis;
		LPARAM lParam;
	} INITDLGPARAM;

protected:
	int m_dialogmode;

public:
	CDialogWindow()
	{
		m_hWnd = NULL;
		m_dialogmode = -1;
	}

	virtual ~CDialogWindow()
	{
	}

	virtual UINT_PTR GetResId() const { return 0; }

	virtual INT_PTR DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return FALSE;
	}

	static INT_PTR CALLBACK _dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( uMsg == WM_INITDIALOG )
		{
			INITDLGPARAM *pidp = (INITDLGPARAM *)lParam;
			CDialogWindow *pdlg = pidp->pThis;
			pdlg->m_hWnd = hDlg;
			AttachBaseWindowObject(hDlg,pdlg);
			return pdlg->DlgProc(hDlg,uMsg,wParam,pidp->lParam);
		}

		CDialogWindow *pwnd = (CDialogWindow *)GetBaseWindowObject(hDlg);

		LRESULT lResult = 0;
		if( pwnd )
		{
			lResult = pwnd->DlgProc(hDlg,uMsg,wParam,lParam);

			if( uMsg == WM_NCDESTROY )
			{
				DetachBaseWindowObject(hDlg);
			}
		}
		return lResult;
	}

	virtual INT_PTR DoModal(HWND hWnd,UINT_PTR idRes,LPARAM lParam=0,HINSTANCE hInstance=NULL)
	{
		m_dialogmode = 0;

		if( hInstance == NULL )
			hInstance = GETINSTANCE(hWnd);

		INITDLGPARAM idp = {0};
		idp.pThis  = this;
		idp.lParam = lParam;
		return DialogBoxParam(hInstance,(LPCWSTR)idRes,hWnd,&_dlgProc,(LPARAM)&idp);
	}

	virtual HWND Create(HWND hWnd,UINT_PTR idRes,LPARAM lParam=0,HINSTANCE hInstance=NULL)
	{
		m_dialogmode = 1;

		if( hInstance == NULL )
			hInstance = GETINSTANCE(hWnd);

		INITDLGPARAM idp = {0};
		idp.pThis  = this;
		idp.lParam = lParam;
		return CreateDialogParamW(hInstance,(LPCWSTR)idRes,hWnd,&_dlgProc,(LPARAM)&idp);
	}
};

class CDialogWindowEx : public CDialogWindow
{
protected:
	HBRUSH m_hbrBackground;
	HBRUSH m_hbrHeader;
	int m_cyHeaderHight;
public:
	CDialogWindowEx()
	{
		m_hbrBackground = NULL;
		m_hbrHeader = NULL;
		m_cyHeaderHight = 0;
	}

	void OnInitDialog(HWND hDlg)
	{
		m_hbrHeader = CreateSolidBrush(RGB(247,247,247));     // todo:
		m_hbrBackground = CreateSolidBrush(RGB(255,255,255)); // todo:
	}

	void OnDestory(HWND hDlg)
	{
		if( m_hbrBackground )
			DeleteObject(m_hbrBackground);
		if( m_hbrHeader )
			DeleteObject(m_hbrHeader);
	}

	virtual INT_PTR OnCtlColor(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_CTLCOLORBTN:
			case WM_CTLCOLORDLG:
			case WM_CTLCOLORSTATIC:
			case WM_CTLCOLOREDIT:
				return (INT_PTR)m_hbrBackground;
		}
		return 0;
	}

	virtual INT_PTR OnEraseBkGnd(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HDC hdc = (HDC)wParam;
		RECT rc;
		GetClientRect(hDlg,&rc);
		FillRect(hdc,&rc,m_hbrBackground);
		if( m_cyHeaderHight > 0 )
		{
			rc.bottom = m_cyHeaderHight;
			FillRect(hdc,&rc,m_hbrHeader);
		}
		return (INT_PTR)TRUE;
	}

	virtual INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( uMsg )
		{
			case WM_CTLCOLORBTN:
			case WM_CTLCOLORDLG:
			case WM_CTLCOLORSTATIC:
			case WM_CTLCOLOREDIT:
				return OnCtlColor(hDlg,uMsg,wParam,lParam);
			case WM_ERASEBKGND:
				return OnEraseBkGnd(hDlg,uMsg,wParam,lParam);
		}
		return 0;
	}
};
