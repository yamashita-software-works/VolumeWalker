#pragma once

interface __declspec(novtable) IViewBaseWindow
{
	virtual HWND GetHWND() const = 0;
	virtual HRESULT Create(HWND hWndParent,DWORD dwFlags=0,LPARAM lParam=0,HWND *phWnd=NULL)=0;
	virtual HRESULT Destroy() = 0;
	virtual HRESULT InitData(PVOID,LPARAM) = 0;
	virtual HRESULT InitLayout(const RECT *prc) = 0;
	virtual HRESULT SelectView(SELECT_ITEM *Path) = 0;
	virtual HRESULT UpdateData(SELECT_ITEM *Sel) = 0;
	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State) = 0;
	virtual HRESULT InvokeCommand(UINT CmdId) = 0;
	virtual HRESULT PreTranslateMessage(MSG *pmsg) = 0;
	virtual HRESULT GetState(int,ULONG *pul) = 0;
	virtual HRESULT SetState(int,ULONG ul) = 0;
	virtual HRESULT GetString(int,LPWSTR pszString,int cch) = 0;
	virtual HRESULT SetString(int,LPWSTR pszString) = 0;
	virtual HRESULT GetBSTR(int,BSTR *pbsString) = 0;
};

interface __declspec(novtable) IApplicationsReader
{
	virtual ULONG __stdcall AddRef() = 0;
	virtual ULONG __stdcall Release() = 0;
	virtual HRESULT Load(PCWSTR pszFileName) = 0;
	virtual HRESULT GetItemCount(ULONG *pulCount) = 0;
	virtual HRESULT GetCommandLine(ULONG ulIndex,PWSTR pszCommandLine,int cchCommandLine) const = 0;
	virtual HRESULT GetFriendlyName(ULONG ulIndex,PWSTR pszFriendlyName, int cchFriendlyName) const = 0;
	virtual HRESULT GetIconPath(ULONG ulIndex,PWSTR pszIconPath,int cchIconPath) const = 0;
	virtual HRESULT GetExecPath(ULONG ulIndex,PWSTR pszExecPath,int cchExecPath) const = 0;
	virtual HRESULT GetStartupDirectory(ULONG ulIndex,PWSTR pszExecPath,int cchExecPath) const = 0;
	virtual HRESULT GetAppendPath(ULONG ulIndex,PWSTR pszAppendPath,int cchAppendPath) const = 0;
	virtual HRESULT GetType(ULONG ulIndex,PULONG pulType) const = 0;
	virtual HRESULT IsSeparator(ULONG ulIndex) const = 0;
};
