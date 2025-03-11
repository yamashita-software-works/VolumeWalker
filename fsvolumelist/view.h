#pragma once
//****************************************************************************
//
//  view.h
//
//  Implements the view base window.
//
//  Author: YAMASHITA Katsuhiro
//
//  History: 2023.03.17 Created.
//           2024.12.03 Modified to a non-window class.
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "pagewbdbase.h"
#include "page_volumehome.h"
#include "page_volumebasicinfo.h"
#include "page_physicaldriveinfo.h"
#include "page_disklayout.h"
#include "page_storagedevice.h"
#include "page_mounteddevice.h"
#include "page_volumelist.h"
#include "page_physicaldrivelist.h"
#include "page_shadowcopy.h"
#include "page_dosdrive.h"
#include "page_statistics.h"
#include "page_simplehexdump.h"
#include "page_filterdriver.h"
#include "page_diskperformance.h"
#include "page_volumemountpoint.h"

class CViewBase
{
	CPageWndBase *m_pPage;
	CPageWndBase *m_pPageTable[VOLUME_CONSOLE_MAX_ID];

public:
	HWND m_hWnd;
	HWND GetPageHWND()
	{
		if( m_pPage == NULL )
			return NULL;
		return m_pPage->GetHwnd();
	}

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
			case VOLUME_CONSOLE_DISKPERFORMANCE:
			{
				pNew = GetOrAllocWndObject<CDiskPerformancePage>(nView);
				break;
			}
			case VOLUME_CONSOLE_VOLUMEMOUNTPOINT:
			{
				pNew = GetOrAllocWndObject<CVolumeMountPointPage>(nView);
				break;
			}
			default:
				return NULL;
		}

		ASSERT(pNew != NULL);

		if( pNew )
		{
			pNew->OnInitPage(ptr,0,0);
		}

		return pNew;
	}

	INT _SelectPage(SELECT_ITEM *SelItem)
	{
		CPageWndBase* pNew = NULL;
		BOOL bCreate = FALSE;

		int nView = SelItem->ViewType;

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

		return nView;
	}

public:
	HRESULT InitData(PVOID pSel,LPARAM lParam)
	{
		if( m_pPage )
			return m_pPage->OnInitPage(pSel,0,0);
		return E_FAIL;
	}

	HRESULT InitLayout(const RECT *prc)
	{
		if( m_pPage )
			return m_pPage->OnInitLayout(prc);
		return E_FAIL;
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
			case VOLUME_CONSOLE_DISKPERFORMANCE:
				_SelectPage( SelItem );
				break;
			case VOLUME_CONSOLE_VOLUMEMOUNTPOINT:
				_SelectPage( SelItem );
				break;
			default:
				ASSERT(FALSE);
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
};
