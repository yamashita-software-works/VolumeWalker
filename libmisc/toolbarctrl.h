#if !defined(_TOOLBARCTRL)
#define _TOOLBARCTRL

#include "stdwnd.h"

class CToolBarCtrl : public CStdWnd
{
public:
	CToolBarCtrl()
	{
	}

	~CToolBarCtrl()
	{
	}

	void SetButtonStructSize(int nSize)
	{ 
		::SendMessage(m_hWnd, TB_BUTTONSTRUCTSIZE, nSize, 0 );
	}

	BOOL AddButtons( int nNumButtons, LPTBBUTTON lpButtons )
	{
		return (BOOL)SendMessage(m_hWnd,TB_ADDBUTTONS,(WPARAM)(UINT)nNumButtons,(LPARAM)(LPTBBUTTON)lpButtons);
	}

	int AddStrings( LPCTSTR lpszStrings )
	{
		return (int)SendMessage(m_hWnd,TB_ADDSTRING,(WPARAM)0,(LPARAM)lpszStrings);
	}

	BOOL SetButtonSize( int dxButton, int dyButton )
	{
		return (BOOL)SendMessage(m_hWnd,TB_SETBUTTONSIZE,(WPARAM)0,MAKELONG(dxButton, dyButton));
	}

	BOOL SetBitmapSize( int dxButton, int dyButton )
	{
		return (BOOL)SendMessage(m_hWnd,TB_SETBITMAPSIZE,(WPARAM)0,MAKELONG(dxButton, dyButton));
	}

	int GetButtonCount()
	{
		return (int)SendMessage(m_hWnd, TB_BUTTONCOUNT, 0, 0L);
	}

	inline DWORD GetButtonSize() const
	{
		return (DWORD)SendMessage(m_hWnd,TB_GETBUTTONSIZE,0,0L);
	}

	BOOL MapAccelerator(TCHAR chAccel, UINT* pIDBtn)
	{
		return (BOOL)SendMessage(m_hWnd, TB_MAPACCELERATOR, (WPARAM)chAccel, (LPARAM)pIDBtn);
	}

	int HitTest(LPPOINT ppt)
	{
		return (int)SendMessage(m_hWnd, TB_HITTEST, 0, (LPARAM)ppt);
	}

	int GetHotItem()
	{
		return (int)SendMessage(m_hWnd, TB_GETHOTITEM, 0, 0);
	}

	int SetHotItem(int nHot)
	{
		return (int)SendMessage(m_hWnd, TB_SETHOTITEM, nHot, 0);
	}

	BOOL GetRect(int nID, LPRECT lpRect)
	{
		return (BOOL)SendMessage(m_hWnd, TB_GETRECT, nID, (LPARAM)lpRect);
	}

	ULONG GetItemData(int nID)
	{
		TBBUTTONINFO tbbi;
		tbbi.cbSize = sizeof(TBBUTTONINFO);
		tbbi.dwMask = TBIF_LPARAM;
		tbbi.lParam = 0;
		SendMessage(m_hWnd,TB_GETBUTTONINFO,(WPARAM)(INT)nID,(LPARAM)(LPTBBUTTONINFO)&tbbi);
		return (ULONG)tbbi.lParam;
	}

	BOOL GetItemRect(int nIndex, LPRECT lpRect) const
	{
		return (BOOL)SendMessage(m_hWnd, TB_GETITEMRECT, nIndex, (LPARAM)lpRect);
	}

	BOOL GetMaxSize(SIZE *size)
	{
		return (BOOL)SendMessage(m_hWnd,TB_GETMAXSIZE,0,(LPARAM)size);
	}

	int GetItemText(int idButton,LPTSTR pszText)
	{
		return (int)SendMessage(m_hWnd,TB_GETBUTTONTEXT,(WPARAM)idButton,(LPARAM)pszText);
	}

	UINT GetCmdId(int Index)
	{
		TBBUTTON tb = {0};
		SendMessage(m_hWnd,TB_GETBUTTON,(WPARAM)Index,(LPARAM)&tb);
		return tb.idCommand;
	}

	void SetText(int nID,LPTSTR pszText)
	{
		TBBUTTONINFO tbbi;
		tbbi.cbSize = sizeof(tbbi);
		tbbi.dwMask = TBIF_TEXT|TBIF_STYLE;
		SendMessage(m_hWnd,TB_GETBUTTONINFO,nID,(LPARAM)&tbbi);

		tbbi.pszText = pszText;
		tbbi.cchText = lstrlen(pszText);
		tbbi.fsStyle |= TBSTYLE_AUTOSIZE;
		SendMessage(m_hWnd,TB_SETBUTTONINFO,nID,(LPARAM)&tbbi);
    }

	inline void AutoSize()
	{
		::SendMessage(m_hWnd, TB_AUTOSIZE, 0, 0L);
	}

	inline DWORD SetExtendedStyle(DWORD dwExStyle)
	{
		ASSERT(IsWindow(m_hWnd));
		return (DWORD)SendMessage(m_hWnd, TB_SETEXTENDEDSTYLE, 0, dwExStyle);
	}

	int AddBitmap(HINSTANCE hInst,UINT nID,int cImages)
	{
		TBADDBITMAP tba;
		tba.hInst = hInst;
		tba.nID = nID;
		return (int)SendMessage(m_hWnd,TB_ADDBITMAP,cImages,(LPARAM)(LPTBADDBITMAP)&tba);
	}

	BOOL DeleteButton(int nIndex)
	{ ASSERT(IsWindow(m_hWnd)); return (BOOL) SendMessage(m_hWnd, TB_DELETEBUTTON, nIndex, 0); }

	UINT CommandToIndex(UINT nID) const
	{ ASSERT(IsWindow(m_hWnd)); return (UINT) SendMessage(m_hWnd, TB_COMMANDTOINDEX, nID, 0L); }

	BOOL InsertButton(int nIndex, LPTBBUTTON lpButton)
	{ ASSERT(IsWindow(m_hWnd)); return (BOOL)SendMessage(m_hWnd, TB_INSERTBUTTON, nIndex, (LPARAM)lpButton); }

	HIMAGELIST SetImageList(HIMAGELIST himl)
	{ ASSERT(IsWindow(m_hWnd)); return (HIMAGELIST)SendMessage(m_hWnd,TB_SETIMAGELIST,0,(LPARAM)himl); }

	int GetState(int nID) const
	{ ASSERT(::IsWindow(m_hWnd)); return (int)SendMessage(m_hWnd,TB_GETSTATE,nID,0L); }

	DWORD GetStyle() const
	{ ASSERT(::IsWindow(m_hWnd)); return (DWORD)SendMessage(m_hWnd,TB_GETSTYLE,0,0L); }

	BOOL CToolBarCtrl::EnableButton(int nID, BOOL bEnable)
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, TB_ENABLEBUTTON, nID, MAKELPARAM(bEnable, 0)); }

	BOOL CToolBarCtrl::CheckButton(int nID, BOOL bCheck)
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, TB_CHECKBUTTON, nID, MAKELPARAM(bCheck, 0)); }

	BOOL CToolBarCtrl::PressButton(int nID, BOOL bPress)
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, TB_PRESSBUTTON, nID, MAKELPARAM(bPress, 0)); }

	BOOL CToolBarCtrl::HideButton(int nID, BOOL bHide)
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, TB_HIDEBUTTON, nID, MAKELPARAM(bHide, 0)); }

	BOOL CToolBarCtrl::Indeterminate(int nID, BOOL bIndeterminate)
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, TB_INDETERMINATE, nID, MAKELPARAM(bIndeterminate, 0)); }

	BOOL CToolBarCtrl::IsButtonEnabled(int nID) const
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, TB_ISBUTTONENABLED, nID, 0); }

	BOOL CToolBarCtrl::IsButtonChecked(int nID) const
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, TB_ISBUTTONCHECKED, nID, 0); }

	BOOL CToolBarCtrl::IsButtonPressed(int nID) const
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, TB_ISBUTTONPRESSED, nID, 0); }

	BOOL CToolBarCtrl::IsButtonHidden(int nID) const
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, TB_ISBUTTONHIDDEN, nID, 0); }

	BOOL CToolBarCtrl::IsButtonIndeterminate(int nID) const
	{ ASSERT(::IsWindow(m_hWnd)); return (BOOL) ::SendMessage(m_hWnd, TB_ISBUTTONINDETERMINATE, nID, 0); }
};
#endif

