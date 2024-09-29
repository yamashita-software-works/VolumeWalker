#pragma once

//////////////////////////////////////////////////////////////////////////////
// LongPathBox Custom Control

typedef struct _LONGPATHBOXINFO
{
	COLORREF crBack;
	COLORREF crText;
	COLORREF crBorder;
} LONGPATHBOXINFO;

#define LPBM_SETINFO         (WM_USER+101)
#define LPBM_GETINFO         (WM_USER+102)
#define LPBM_SETMARGIN       (WM_USER+103)
#define LPBM_SETSTATE        (WM_USER+104)
#define LPBM_GETSTATE        (WM_USER+105)

#define LPBS_END_ELLIPSIS     0x00000001
#define LPBS_NO_ELLIPSIS      0x00000020
#define LPBS_NO_FOCUS         0x00000040
#define LPBS_FLAT_BORDER      0x00000080
#define LPBS_NO_TEXTSELECTION 0x00000100

#define LPBN_CLICKED          0x1
#define LPBN_DBLCLK           0x2
#define LPBN_SETFOCUS         0x3
#define LPBN_KILLFOCUS        0x4

#define LPBSTATE_SELECT       0x01 

#define LPBC_LONGPATHBOX_NAME L"LongPathBox"

//////////////////////////////////////////////////////////////////////////////
// CLongPathBox Window

#ifdef _USEWTL
class CLongPathBox : public CWindow
{
public:
	CLongPathBox()
	{
	}

	virtual ~CLongPathBox()
	{
	}

	int GetInfo(LONGPATHBOXINFO *pInfo) { return (int)::SendMessage(m_hWnd,LPBM_GETINFO,0,(LPARAM)pInfo); }
	int SetInfo(LONGPATHBOXINFO *pInfo) { return (int)::SendMessage(m_hWnd,LPBM_SETINFO,0,(LPARAM)pInfo); }

	void SetColorEx(COLORREF crBack,COLORREF crText,COLORREF crBorder)
	{
		LONGPATHBOXINFO info;
		::SendMessage(m_hWnd,LPBM_GETINFO,0,(LPARAM)&info);
		info.crBack = crBack;
		info.crText = crText;
		info.crBorder = crBorder;
		::SendMessage(m_hWnd,LPBM_SETINFO,0,(LPARAM)&info);
	}

	void SetColor(COLORREF crBack,COLORREF crText)
	{
		SetColorEx(crBack,crText,GetSysColor(COLOR_3DSHADOW));
	}

	void SetBackColor(COLORREF crBack)
	{
		LONGPATHBOXINFO info;
		::SendMessage(m_hWnd,LPBM_GETINFO,0,(LPARAM)&info);
		info.crBack = crBack;
		::SendMessage(m_hWnd,LPBM_SETINFO,0,(LPARAM)&info);
	}

	void SetTextColor(COLORREF crText)
	{
		LONGPATHBOXINFO info;
		::SendMessage(m_hWnd,LPBM_GETINFO,0,(LPARAM)&info);
		info.crText = crText;
		::SendMessage(m_hWnd,LPBM_SETINFO,0,(LPARAM)&info);
	}

	void SetBorderColor(COLORREF crBorder)
	{
		LONGPATHBOXINFO info;
		::SendMessage(m_hWnd,LPBM_GETINFO,0,(LPARAM)&info);
		info.crBorder = crBorder;
		::SendMessage(m_hWnd,LPBM_SETINFO,0,(LPARAM)&info);
	}
};
#endif