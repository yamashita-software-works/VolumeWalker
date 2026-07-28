#pragma once
//*****************************************************************************
//
//  pane_streacluster.h
//
//  PURPOSE: Stream and Cluster Information Pane.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-04-11 Created
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
// 

#include "simplesplitwindow.h"

class CStreamClusterInfoWindow : 
	public CBaseWindow,
	CSimpleHSplitWindowEx<CStreamClusterInfoWindow>
{
	HWND m_hWndCluster;
	HWND m_hWndStream;
	HWND m_hWndFocus;

	PWSTR m_pszCurPath;
	FILEITEMEX *m_pFIEx;

	double m_save_ratio;
	int m_save_splitpos;
	int m_splitpos;

public:
	CStreamClusterInfoWindow()
	{
		m_hWndCluster = NULL;
		m_hWndStream = NULL;
		m_pszCurPath = NULL;
		m_pFIEx = NULL;
		m_save_ratio = 0.0;
		m_save_splitpos = -1;
		m_splitpos = -1;
		m_ratio = 0.4;
		m_hWndFocus = NULL;
	}

	~CStreamClusterInfoWindow()
	{
		_SafeMemFree(m_pszCurPath);
	}

	void SetRatio(int percent)
	{
		m_ratio = ((double)percent) / 100.0;
	}

protected:
	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		CStreamInformationWindow *pStm = new CStreamInformationWindow;
		m_hWndStream = pStm->Create(hWnd,0,L"",WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);

		CClusterInformationWindow *pCluster = new CClusterInformationWindow;
		m_hWndCluster = pCluster->Create(hWnd,0,L"",WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,WS_EX_CONTROLPARENT);

		CSimpleHSplitWindowEx::SetSplitterValue(&m_splitpos);

		return 0;
	}

	LRESULT OnDestroy(HWND,UINT,WPARAM,LPARAM)
	{
		return 0;
	}

	LRESULT OnSize(HWND,UINT,WPARAM,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		return 0;
	}

	LRESULT OnEraseBkGnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HDC hdc = (HDC)wParam;
		RECT rc;
		GetClientRect(hWnd,&rc);
		FillRect(hdc,&rc,GetSysColorBrush(COLOR_3DFACE));
		return (LRESULT)1;
	}

	LRESULT OnShowWindow(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( wParam ) {
			if(!IsWindowVisible(m_hWndStream)) ShowWindow(m_hWndStream,SW_SHOWNA);
			if(!IsWindowVisible(m_hWndCluster)) ShowWindow(m_hWndCluster,SW_SHOWNA);
		}
		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;
		switch( pnmhdr->code )
		{
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
			case LVN_ITEMCHANGED:
				return OnItemChanged(pnmhdr);
		}
		return 0;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		m_hWndFocus = pnmhdr->hwndFrom;
		return ForwardNotificationToParent(m_hWnd,pnmhdr);
	}

	LRESULT OnItemChanged(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;

		if( pnmlv->uNewState & (LVIS_FOCUSED|LVIS_SELECTED) )
		{
			STREAM_LIST_ITEM *pItem = (STREAM_LIST_ITEM *)pnmlv->lParam;
			if( pItem )
			{
				PWSTR psz = pItem->pStreamInformation->StreamName;

				PWSTR pszStreamName =_MemAllocStringCat(m_pszCurPath,pItem->pStreamInformation->StreamName);

				SendMessage(m_hWndCluster,PPM_SETPATH,(WPARAM)m_pFIEx,(LPARAM)pszStreamName);

				_SafeMemFree(pszStreamName);
			}
		}
		else
		{
			if( (pnmlv->uNewState & LVIS_SELECTED) == 0 && (pnmlv->uOldState & LVIS_SELECTED) != 0 ) {
			if( ListView_GetSelectedCount(pnmlv->hdr.hwndFrom) == 0 )
			{
				SendMessage(m_hWndCluster,PPM_SETPATH,(WPARAM)0,(LPARAM)0);
			}
			}
		}
		return 0;
	}

	LRESULT OnCommand(HWND,UINT,WPARAM wParam,LPARAM lParam)
	{
		if( m_hWndFocus == m_hWndStream )
			SendMessage(m_hWndStream,WM_COMMAND,wParam,lParam);
		else if( m_hWndFocus == m_hWndCluster )
			SendMessage(m_hWndCluster,WM_COMMAND,wParam,lParam);
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_ERASEBKGND:
				return OnEraseBkGnd(hWnd,uMsg,wParam,lParam);
		    case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				return OnSetFocus(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
		    case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_SHOWWINDOW:
				return OnShowWindow(hWnd,uMsg,wParam,lParam);
			case PPM_SETPATH:
				return OnSetPath((PCWSTR)lParam,(FILEITEMEX *)wParam);
			case PPM_CHDIR:
				return OnChDir((PCWSTR)lParam,(FILEITEMEX *)wParam);
			case PPM_INITLAYOUT:
				return OnInitLayout(wParam,lParam);
		}

		LRESULT lResult;
		CSimpleHSplitWindowEx::ProcessWindowMessage(hWnd,uMsg,wParam,lParam,lResult);

		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	void InitSplitPos()
	{
		RECT rc;
		GetClientRect(m_hWnd,&rc);
		int cy = _RECT_HIGHT(rc);
		cy = (int)((double)cy * m_ratio);
		m_splitpos = cy;
		UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),0);
	}

	void DetermineViewStyleByFileSystemFlag(PCWSTR pszFilePath)
	{
		DWORD FileSystemFlags = 0;
		HANDLE hFile;
		NTSTATUS Status;
		UNICODE_STRING RootDir;

		if( pszFilePath == NULL )
			return;

		SplitRootRelativePath(pszFilePath,&RootDir,NULL);

		if( (Status = OpenFile_U(&hFile,NULL,&RootDir,FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,0)) == 0 )
		{
			GetVolumeInformationByHandleW(hFile,nullptr,0,nullptr,nullptr,&FileSystemFlags,nullptr,0);

			CloseHandle(hFile);
		}

		if( FILE_NAMED_STREAMS & FileSystemFlags )
		{
			if( m_save_splitpos == -1 )
			{
				InitSplitPos();
				m_save_splitpos = m_splitpos;
			}

			if( !IsWindowVisibleEx(m_hWndStream) )
			{
				m_ratio = m_save_ratio;
				m_splitpos = m_save_splitpos;

				SetWindowPos(m_hWndStream,NULL,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOREDRAW);

				RECT rc;
				GetClientRect(m_hWnd,&rc);
				UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),0);
			}
		}
		else
		{
			if( IsWindowVisibleEx(m_hWndStream) )
			{
				RECT rc;
				GetClientRect(m_hWnd,&rc);

				ShowWindow(this->m_hWndStream,SW_HIDE);

				m_save_ratio    = m_ratio;
				if( m_splitpos == -1 )
					m_splitpos = (int)((double)_RECT_HIGHT(rc) * CSimpleHSplitWindowEx::m_ratio);
				m_save_splitpos = m_splitpos;

				m_splitpos = 0; // no stream pane area

				UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),TRUE);
			}
		}
	}

	LRESULT OnChDir(PCWSTR pszFilePath,FILEITEMEX *pFIEx)
	{
		DetermineViewStyleByFileSystemFlag(pszFilePath);
		return 0;
	}

	LRESULT OnSetPath(PCWSTR pszFilePath,FILEITEMEX *pFIEx)
	{
		LRESULT lResult = 0;

		if( pszFilePath == NULL )
		{
			SendMessage(m_hWndStream,PPM_SETPATH,(WPARAM)0,(LPARAM)0);
			SendMessage(m_hWndCluster,PPM_SETPATH,(WPARAM)0,(LPARAM)0);
			return 0;
		}

		DetermineViewStyleByFileSystemFlag(pszFilePath);

		_SafeMemFree(m_pszCurPath);
		if( pszFilePath )
			m_pszCurPath = _MemAllocString(pszFilePath);

		m_pFIEx = pFIEx;

		if( IsWindowVisibleEx(m_hWndStream) )
		{
			SendMessage(m_hWndStream,PPM_SETPATH,(WPARAM)pFIEx,(LPARAM)pszFilePath);
			if( SendMessage(m_hWndStream,PPM_SELECTSTREAM,(WPARAM)0,(LPARAM)0) != 0 )
				SendMessage(m_hWndCluster,PPM_SETPATH,(WPARAM)0,(LPARAM)0);
		}
		else
		{
			SendMessage(m_hWndCluster,PPM_SETPATH,(WPARAM)pFIEx,(LPARAM)pszFilePath);
		}

		return lResult;
	}

	LRESULT OnInitLayout(WPARAM wParam,LPARAM lParam)
	{
		PWSTR psz = (PWSTR)lParam;
		if( psz )
			DetermineViewStyleByFileSystemFlag(psz);
		SendMessage(m_hWndStream,PPM_INITLAYOUT,wParam,lParam); // forward child pane
		return 0;
	}

	VOID UpdateLayout(int cx=-1,int cy=-1,BOOL absSplitPos=FALSE)
	{
		HDWP hdwp = BeginDeferWindowPos(2);
		int cySplit;

		if( m_splitpos == -1 && m_save_splitpos == -1 )
		{
			return ;
		}

		if( cx == -1 && cy == -1 )
		{
			RECT rc;
			GetClientRect(m_hWnd,&rc);
			cx = _RECT_WIDTH(rc);
			cy = _RECT_HIGHT(rc);
		}

		if( IsWindowVisibleEx(this->m_hWndStream) )
		{
			if( absSplitPos )
				cySplit = m_splitpos;
			else
				cySplit = m_splitpos = (int)((double)cy * m_ratio);
			int bar = 3;
			if( this->m_hWndStream )
				DeferWindowPos(hdwp,this->m_hWndStream,NULL,0,0,cx,cySplit,SWP_NOZORDER);
			if( this->m_hWndCluster )
				DeferWindowPos(hdwp,this->m_hWndCluster,NULL,0,cySplit+bar,cx,cy-cySplit-bar,SWP_NOZORDER);
		}
		else
		{
			DeferWindowPos(hdwp,this->m_hWndCluster,NULL,0,0,cx,cy,SWP_NOZORDER);
		}
		
		EndDeferWindowPos(hdwp);
	}
};
