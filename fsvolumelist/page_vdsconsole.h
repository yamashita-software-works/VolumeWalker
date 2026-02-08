#pragma once
//****************************************************************************
//*                                                                          *
//*  page_vdsconsole.h                                                       *
//*                                                                          *
//*  VDS Information Console Host Page                                       *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2025-09-25 Created.                                            *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "vdshelp.h"
#include "common.h"
#include "pagewbdbase.h"
#include "vdshelp.h"
#include "page_vdsdisks.h"
#include "simpletoolbar.h"

class CVDSConsolePage : public CPageWndBase
{
	CVDSDataManager *m_pVDB;
	
	enum {
		ID_VDS_DISK_CONSOLE,
	};

	enum {
		VDS_PAGE_NONE=0,
		VDS_PAGE_PHYSICALDISK,
		VDS_PAGE_MAX,
	};

	CPageWndBase *m_pPage;
	CPageWndBase *m_pPageTable[ VDS_PAGE_MAX ];

	CSimpleToolbar m_Toolbar;
	HWND m_hWndToolBar;

	DWORD m_dwErrorCode;

public:
	CVDSConsolePage()
	{
		m_pVDB = NULL;
		m_pPage = NULL;
		memset(m_pPageTable,0,sizeof(m_pPageTable));
		m_dwErrorCode = 0;
	}

	~CVDSConsolePage()
	{
	}

	template <class T>
	CPageWndBase *GetOrAllocWndObject(int wndId)
	{
		if( wndId >= VOLUME_CONSOLE_MAX_ID || wndId < 0 )
			return NULL;

		CPageWndBase *pobj;
		if( m_pPageTable[ wndId ] == NULL )
		{
			pobj = (CPageWndBase*)new T ;
			m_pPageTable[ wndId ] = pobj;
			pobj->Create(m_hWnd,wndId,nullptr,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,WS_EX_CONTROLPARENT);
		}
		else
		{
			pobj = NULL;
		}
		return pobj;
	}

	virtual HRESULT OnInitPage(PVOID ptr,DWORD,PVOID)
	{
		SELECT_ITEM *SelectItem = (SELECT_ITEM *)ptr;
		m_pPageTable[VDS_PAGE_PHYSICALDISK] = GetOrAllocWndObject<CVDSDisksPage>(VDS_PAGE_PHYSICALDISK);// currently, physical drive page only.
		m_pPageTable[VDS_PAGE_PHYSICALDISK]->OnInitPage(nullptr,0,nullptr);
		ChangePage(VDS_PAGE_PHYSICALDISK);
		return S_OK;
	}

	virtual HRESULT OnInitLayout(const RECT *prc)
	{
		if(	m_pPage )
		{
			RECT rc;
			GetClientRect(m_pPage->m_hWnd,&rc);
			m_pPage->OnInitLayout(&rc);
		}
		return S_OK;
	}

	virtual HRESULT OnDestroyPage(PVOID)
	{
		return S_OK;
	}

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		FreeVDB();
		return 0;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		PAINTSTRUCT ps;

		HDC hdc = BeginPaint(hWnd,&ps);

		RECT rc;
		GetClientRect(hWnd,&rc);
		FillRect(hdc,&rc,GetSysColorBrush(COLOR_WINDOW));

		if( m_dwErrorCode != 0 )
		{
			HFONT hFont = GetIconFont();
			HFONT hfontOld;
	
			PWSTR pMessage;
			if( HRESULT_CODE(m_dwErrorCode) == ERROR_ACCESS_DENIED )
				pMessage = StrDup(L"Access denied.\nTo get full-information, please run under administrator mode.");
			else
				WinGetErrorMessage(m_dwErrorCode,&pMessage);
	
			hfontOld = (HFONT)SelectObject(hdc,hFont);
	
			SetBkMode(hdc,TRANSPARENT);

			RECT rcText = rc;
			DrawText(hdc,pMessage,-1,&rcText,DT_VCENTER|DT_CENTER|DT_CALCRECT);
			rc.top = ((rc.bottom - rc.top) - (rcText.bottom - rcText.top))/2;
			DrawText(hdc,pMessage,-1,&rc,DT_CENTER);
	
			SelectObject(hdc,hfontOld);
	
			WinFreeErrorMessage(pMessage);
		}

		EndPaint(hWnd,&ps);

		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return 0;
	}

	LRESULT OnQueryMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// LOWORD(wParam) UI_QUERY_*
		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;

		switch( pnmhdr->code )
		{
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
		}
		return 0;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		pnmhdr->hwndFrom = m_hWnd;
		pnmhdr->idFrom = GetWindowLong(m_hWnd,GWL_ID);
		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)pnmhdr);
		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_PAINT:
				return OnPaint(hWnd,uMsg,wParam,lParam);
			case WM_ERASEBKGND:
				return 1;
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
			case WM_QUERY_MESSAGE:
				return OnQueryMessage(hWnd,uMsg,wParam,lParam);
			case WM_COMMAND:
				InvokeCommand(LOWORD(wParam));
				return 0;
			case PM_FINDITEM:
				if( m_pPage )
					return SendMessage(m_pPage->m_hWnd,uMsg,wParam,lParam);
				break;
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	VOID UpdateLayout(int cx,int cy)
	{
		if( m_pPage )
		{
			SetWindowPos(m_pPage->m_hWnd,NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOCOPYBITS);
			RedrawWindow(m_hWnd,NULL,NULL,RDW_INVALIDATE);
		}
	}

	VOID ChangePage(int nPage)
	{
		ASSERT( VDS_PAGE_NONE < nPage && nPage < VDS_PAGE_MAX );
		if( m_pPage != m_pPageTable[nPage] )
		{
			CPageWndBase *pNextPage = m_pPageTable[ nPage ];
			CPageWndBase *pPrevPage;

			pPrevPage = m_pPage;
			m_pPage = pNextPage;

			RECT rc;
			GetClientRect(m_hWnd,&rc);
			UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc));

			ShowWindow(m_pPage->GetHwnd(),SW_SHOW);
			if( pPrevPage )
				ShowWindow(pPrevPage->GetHwnd(),SW_HIDE);
		}
	}

	HRESULT FillItems(SELECT_ITEM *)
	{
		CWaitCursor cur;

		HRESULT hr = E_FAIL;

		FreeVDB();

		if( m_pPage )
		{
			CVDSDataManager *pVDB = new CVDSDataManager;
			if( pVDB )
			{
				hr = pVDB->Initialize();

				if( hr == S_OK )
				{
					hr = pVDB->EnumVDSItems();
				}

				if( hr == S_OK )
				{
					m_pVDB = pVDB;

					switch( m_pPage->GetWndId() )
					{
						case VDS_PAGE_PHYSICALDISK:
						{
							SELECT_ITEM sel = {0};
							sel.pszPhysicalDrive = (PWSTR)pVDB;
							hr = ((CVDSDisksPage*)m_pPage)->FillItems(&sel);
							break;
						}
					}
				}
				else
				{
					FreeVDB();
				}
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}

			if( FAILED(hr) )
			{
				m_dwErrorCode = hr;
				EnableWindow(m_pPage->GetHwnd(),FALSE);
				ShowWindow(m_pPage->GetHwnd(),SW_HIDE);
			}
			else
			{
				m_dwErrorCode = 0;
			}
		}
		return hr;
	}

	VOID FreeVDB()
	{
		if( m_pVDB )
		{
			delete m_pVDB;
			m_pVDB = NULL;
		}
	}

	virtual HRESULT UpdateData(PVOID pFile)
	{
		return FillItems((SELECT_ITEM*)pFile);
	}

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		if( m_pPage )
		{
			return m_pPage->QueryCmdState(CmdId,State);
		}
		return S_FALSE;
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		switch( CmdId )
		{
			case ID_VDS_DISK_CONSOLE:
				ChangePage(VDS_PAGE_PHYSICALDISK);
				break;
			case ID_VIEW_REFRESH:
				UpdateData(NULL);
				break;
			default:
				if( m_pPage )
					return m_pPage->InvokeCommand(CmdId);
		}
		return S_OK;
	}
};
