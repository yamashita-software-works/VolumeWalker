//****************************************************************************
//
//  view.cpp
//
//  Implements the view base window.
//
//  Auther: YAMASHITA Katsuhiro
//
//  Create: 2023.03.17
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "resource.h"
#include "fsvolumelist.h"
#include "view.h"

class CViewBase : 
	public CBaseWindow,
	public IViewBaseWindow
{
	CPageWndBase *m_pPage;
	CPageWndBase *m_pPageTable[VOLUME_CONSOLE_MAX_ID];

public:
	CViewBase()
	{
		m_hWnd = NULL;
		m_pPage = NULL;
		memset(m_pPageTable,0,sizeof(m_pPageTable));
	}

	virtual ~CViewBase()
	{
	}

	VOID UpdateLayout(int cx=0,int cy=0)
	{
		if( cx == 0 && cy == 0 )
		{
			RECT rc;
			GetClientRect(m_hWnd,&rc); // todo: GetClientSize
			cx = rc.right - rc.left;
			cy = rc.bottom - rc.top;
		}

		if( m_pPage && m_pPage->GetHwnd() )
			SetWindowPos(m_pPage->GetHwnd(),NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOMOVE|SWP_FRAMECHANGED);
	}

	LRESULT OnCreate(HWND hWnd,UINT,WPARAM,LPARAM lParam)
	{
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd,UINT,WPARAM,LPARAM)
	{
		return 0;
	}

	LRESULT OnSize(HWND,UINT,WPARAM,LPARAM lParam)
	{
		UpdateLayout(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		return 0;
	}

	LRESULT OnSetFocus(HWND,UINT,WPARAM,LPARAM lParam)
	{
		if( m_pPage )
			SetFocus(m_pPage->GetHwnd());
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

	template <class T>
	CPageWndBase *GetOrAllocWndObject(int wndId)
	{
		ASSERT( wndId >= 0 );
		ASSERT( wndId < VOLUME_CONSOLE_MAX_ID );

		if( wndId >= VOLUME_CONSOLE_MAX_ID || wndId < 0 )
			return NULL;

		CPageWndBase *pobj;
		if( m_pPageTable[ wndId ] == NULL )
		{
			pobj = (CPageWndBase*)new T ;
			m_pPageTable[ wndId ] = pobj;
			pobj->Create(m_hWnd,wndId,0,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,WS_EX_CONTROLPARENT);
		}
		else
		{
			pobj = NULL;
		}
		return pobj;
	}

	CPageWndBase *_CreatePage(int nView,PVOID ptr)
	{
		CPageWndBase* pNew = NULL;

		switch( nView )
		{
			case VOLUME_CONSOLE_HOME:
			{
				pNew = GetOrAllocWndObject<CVolumeHomeView>(nView);
				break;
			}
			case VOLUME_CONSOLE_VOLUMEINFORMAION:
			{
				pNew = GetOrAllocWndObject<CVolumeBasicInfoView>(nView);
				break;
			}
			case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
			{
				pNew = GetOrAllocWndObject<CPhysicalDiskInfoView>(nView);
				break;
			}
			case VOLUME_CONSOLE_DISKLAYOUT:
			{
				pNew = GetOrAllocWndObject<CDiskLayoutView>(nView);
				break;
			}
			case VOLUME_CONSOLE_STORAGEDEVICE:
			{
				pNew = GetOrAllocWndObject<CStorageDevicePage>(nView);
				break;
			}
			case VOLUME_CONSOLE_MOUNTEDDEVICE:
			{
				pNew = GetOrAllocWndObject<CMountedDevicePage>(nView);
				break;
			}
			case VOLUME_CONSOLE_VOLUMELIST:
			{
				pNew = GetOrAllocWndObject<CVolumeListPage>(nView);
				break;
			}
			case VOLUME_CONSOLE_PHYSICALDRIVELIST:
			{
				pNew = GetOrAllocWndObject<CPhysicalDriveListPage>(nView);
				break;
			}
			case VOLUME_CONSOLE_SHADOWCOPYLIST:
			{
				pNew = GetOrAllocWndObject<CVolumeShadowCopyListPage>(nView);
				break;
			}
			case VOLUME_CONSOLE_DOSDRIVELIST:
			{
				pNew = GetOrAllocWndObject<CDosDriveListPage>(nView);
				break;
			}
			case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
			{
				pNew = GetOrAllocWndObject<CFileSystemStatisticsPage>(nView);
				break;
			}
			case VOLUME_CONSOLE_SIMPLEHEXDUMP:
			{
				pNew = GetOrAllocWndObject<CSimpleHexDumpPage>(nView);
				break;
			}
			case VOLUME_CONSOLE_FILTERDRIVER:
			{
				pNew = GetOrAllocWndObject<CFilterDriverPage>(nView);
				break;
			}
			default:
				return NULL;
		}

		ASSERT(pNew != NULL);

		if( pNew )
		{
			pNew->OnInitPage(ptr);
		}

		return pNew;
	}

	INT _SelectPage(SELECT_ITEM *SelItem)
	{
		CPageWndBase* pNew = NULL;
		BOOL bCreate = FALSE;

		int nView = SelItem->View;

		ASSERT( nView >= 0 );
		ASSERT( nView < VOLUME_CONSOLE_MAX_ID );

		if( nView >= VOLUME_CONSOLE_MAX_ID || nView < 0 )
			return NULL;

		if( m_pPageTable[ nView ] == NULL )
		{
			pNew = _CreatePage(nView,SelItem);
		}
		else
		{
			pNew = m_pPageTable[ nView ];
		}

		if( pNew == NULL )
			return nView;

		if( m_pPage == pNew )
		{
			return nView;
		}
	
		EnableWindow(pNew->m_hWnd,TRUE);
		ShowWindow(pNew->m_hWnd,SW_SHOWNA);

		if( m_pPage )
		{
			ShowWindow(m_pPage->m_hWnd,SW_HIDE);
			EnableWindow(m_pPage->m_hWnd,FALSE);
		}

		m_pPage = pNew;
		UpdateLayout();

		return nView;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_SETFOCUS:
				return OnSetFocus(hWnd,uMsg,wParam,lParam);
		    case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			// Forward Parent Window
			case WM_CONTROL_MESSAGE:
				return SendMessage(GetParent(m_hWnd),WM_CONTROL_MESSAGE,wParam,lParam);
			case WM_NOTIFY_MESSAGE:
				return SendMessage(GetParent(m_hWnd),WM_NOTIFY_MESSAGE,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

	//
	// IViewBaseWindow
	//
public:
	HWND GetHWND() const
	{
		return m_hWnd;
	}

	HRESULT Create(HWND hWnd,HWND *phWndFileList=NULL)
	{
		HWND hwnd = CBaseWindow::Create(hWnd,0,0,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,WS_EX_CONTROLPARENT);
		if( phWndFileList )
			*phWndFileList = hwnd;

		return S_OK;
	}

	HRESULT Destroy()
	{
		return E_NOTIMPL;
	}

	virtual HRESULT InitData(PVOID pSel,LPARAM lParam)
	{
		ASSERT( m_pPage != NULL );
		return m_pPage->OnInitPage(pSel);
	}

	HRESULT InitLayout(const RECT *prc)
	{
		if( m_pPage )
			m_pPage->OnInitLayout(prc);
		return E_NOTIMPL;
	}

	HRESULT SelectView(SELECT_ITEM *SelItem) 
	{
		ASSERT(SelItem != NULL);

		switch( SelItem->ViewType )
		{
			case VOLUME_CONSOLE_HOME:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_VOLUMEINFORMAION:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_DISKLAYOUT:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_STORAGEDEVICE:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_MOUNTEDDEVICE:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_VOLUMELIST:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_PHYSICALDRIVELIST:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_SHADOWCOPYLIST:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_DOSDRIVELIST:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_FILESYSTEMSTATISTICS:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_SIMPLEHEXDUMP:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_FILTERDRIVER:
				_SelectPage( SelItem );
				break;
			default:
				return E_FAIL;
		}

		return S_OK;
	}

	virtual HRESULT UpdateData(SELECT_ITEM *pSel)
	{
		ASSERT( m_pPage != NULL );
		return m_pPage->UpdateData(pSel);
	}

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		ASSERT( m_pPage != NULL );
		return m_pPage->QueryCmdState(CmdId,State);
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		ASSERT( m_pPage != NULL );
		return m_pPage->InvokeCommand(CmdId);
	}

	virtual HRESULT GetState(int,ULONG *pul)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT PreTranslateMessage(MSG *msg)
	{
		return E_NOTIMPL;
	}
};

//////////////////////////////////////////////////////////////////////////////

//
//  C style functions
//

HRESULT ViewBase_CreateObject(HINSTANCE hInstance,IViewBaseWindow **pObject)
{
	CViewBase *pWnd = new CViewBase;

	CViewBase::RegisterClass(hInstance);

	*pObject = static_cast<IViewBaseWindow *>(pWnd);

	return S_OK;
}
