#pragma once

#include "basewindow.h"

class CConsoleWindow : public CBaseWindow
{
protected:
	HWND m_hWndCtrlFocus;
public:
	CConsoleWindow()
	{
		m_hWndCtrlFocus = NULL;
	}

	virtual ~CConsoleWindow()
	{
	}

	virtual VOID InitData(PCWSTR)
	{
	}

	virtual VOID InitDataEx(PVOID,ULONG)
	{
	}

	virtual VOID InitLayout(const RECT *prcDesktopWorkArea)
	{
	}
};
