#pragma once

#include "basewindow.h"

class CPageWndBase : public CBaseWindow
{
public:
	CPageWndBase()
	{
	}

	~CPageWndBase()
	{
	}

	virtual HRESULT OnInitPage(PVOID,DWORD,PVOID)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT OnInitLayout(const RECT *prc)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT OnDestroyPage(PVOID)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT UpdateData(PVOID)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT PreTranslateMessage(MSG *pMsg)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT GetString(int,LPWSTR psz,int cch)
	{
		return E_NOTIMPL;
	}
};
