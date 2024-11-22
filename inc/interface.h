#pragma once

interface __declspec(novtable) IViewBaseWindow
{
	virtual HWND GetHWND() const = 0;
	virtual HRESULT Create(HWND hWndParent,HWND *phWnd=NULL) = 0;
	virtual HRESULT Destroy() = 0;
	virtual HRESULT InitData(PVOID,LPARAM) = 0;
	virtual HRESULT InitLayout(const RECT *prc) = 0;
	virtual HRESULT SelectView(SELECT_ITEM *Path) = 0;
	virtual HRESULT UpdateData(SELECT_ITEM *Sel) = 0;
	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State) = 0;
	virtual HRESULT InvokeCommand(UINT CmdId) = 0;
	virtual HRESULT GetState(int,ULONG *pul) = 0;
	virtual HRESULT SetState(int,ULONG ul) = 0;
	virtual HRESULT PreTranslateMessage(MSG *pmsg) = 0;
};
