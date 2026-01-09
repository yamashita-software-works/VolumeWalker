//***************************************************************************
//*                                                                         *
//*  datatarget.cpp                                                         *
//*                                                                         *
//*  CLASS:    CDropTarget                                                  *
//*                                                                         *
//*  PURPOSE:  IDropTarget implementation.                                  *
//*                                                                         *
//*  AUTHOR:   YAMASHITA Katsuhiro                                          *
//*                                                                         *
//*  HISTORY:  2011-12-01,2015-08-18 - Created                              *
//*                                                                         *
//***************************************************************************
//   Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//   Licensed under the MIT License.
#include "stdafx.h"
#include "droptarget.h"
#include "clipboardfilecopy.h"

DWORD CheckPreferredEffect(IDataObject *pdto)
{
    DWORD dwRet = 0;
    FORMATETC fe = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT),
                    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgm = {0};

    if (SUCCEEDED(pdto->GetData(&fe, &stgm)))
    {
        if ((stgm.tymed & TYMED_HGLOBAL) && GlobalSize(stgm.hGlobal) >= sizeof(DWORD))
        {
            DWORD *pdw = (DWORD*)GlobalLock(stgm.hGlobal);
            if( pdw  )
                dwRet = *pdw;

            GlobalUnlock(stgm.hGlobal);
        }
    }

    ReleaseStgMedium(&stgm);

    return dwRet;
}

DWORD CheckPerformedEffect(IDataObject *pdto)
{
    DWORD dwRet = 0;
    FORMATETC fe = {(CLIPFORMAT)RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT),
                    NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgm = {0};

    if (SUCCEEDED(pdto->GetData(&fe, &stgm)))
    {
        if ((stgm.tymed & TYMED_HGLOBAL) && GlobalSize(stgm.hGlobal) >= sizeof(DWORD))
        {
            DWORD *pdw = (DWORD*)GlobalLock(stgm.hGlobal);
            if( pdw  )
                dwRet = *pdw;

            GlobalUnlock(stgm.hGlobal);
        }
    }

    ReleaseStgMedium(&stgm);

    return dwRet;
}

HRESULT SetPerformedDropEffect(IDataObject *pDataObject,DWORD dwEffect)
{
    // CFSTR_PERFORMEDDROPEFFECT
    //
    // This format identifier is used by the target to inform the data object through its 
    // IDataObject::SetData method of the outcome of a data transfer. The data is an STGMEDIUM structure 
    // that contains a global memory object. The structure's hGlobal member points to a DWORD 
    // set to the appropriate DROPEFFECT value, normally DROPEFFECT_MOVE or DROPEFFECT_COPY.
    // This format is normally used when the outcome of an operation can be either move or copy,
    // such as in an optimized move or delete-on-paste operation. It provides a reliable way 
    // for the target to tell the data object what actually happened. It was introduced because
    // the value of pdwEffect returned by DoDragDrop did not reliably indicate which operation 
    // had taken place. The CFSTR_PERFORMEDDROPEFFECT format is the reliable way to indicate that
    // an unoptimized move has taken place.
    FORMATETC feP = { (CLIPFORMAT)RegisterClipboardFormat( CFSTR_PERFORMEDDROPEFFECT ), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgmS = {0};
    stgmS.tymed = TYMED_HGLOBAL;
    stgmS.hGlobal = GlobalAlloc(GHND,sizeof(DWORD));
    LPDWORD pdw = (LPDWORD)::GlobalLock(stgmS.hGlobal);
    *pdw = dwEffect;
    GlobalUnlock(stgmS.hGlobal);
    return pDataObject->SetData(&feP,&stgmS,TRUE);
}

HRESULT SetPasteSucceeded(IDataObject *pDataObject,DWORD dwEffect)
{
    FORMATETC feS = { (CLIPFORMAT)RegisterClipboardFormat( CFSTR_PASTESUCCEEDED ), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgmS = {0};
    stgmS.tymed = TYMED_HGLOBAL;
    stgmS.hGlobal = GlobalAlloc(GHND,sizeof(DWORD));
    LPDWORD pdw = (LPDWORD)::GlobalLock(stgmS.hGlobal);
    *pdw = dwEffect;
    GlobalUnlock(stgmS.hGlobal);
    return pDataObject->SetData(&feS,&stgmS,TRUE); 
}

//////////////////////////////////////////////////////////////////////////////

CDropTarget::CDropTarget()
{
    m_pDropTargetHelper = NULL;
    m_pCallback = NULL;
    m_hwndDropTarget = NULL;
    m_fControl = DDCF_ENABLEMOVE;
    m_refs = 0;
    m_bSupportedFormat = FALSE;
    m_pDataObject = NULL; // current drop operation's data
    m_cfShellItemIdList = RegisterClipboardFormat( CFSTR_SHELLIDLIST );
}

CDropTarget::~CDropTarget()
{
}

STDMETHODIMP CDropTarget::QueryInterface(REFIID iid, void FAR* FAR* ppv) 
{
    if(iid == IID_IUnknown || iid == IID_IDropTarget)
    {
        *ppv = this;
        ++m_refs;
        return NOERROR;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDropTarget::AddRef(void)
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG) CDropTarget::Release(void)
{
    if(--m_refs == 0)
    {
        delete this;
        return 0;
    }
    return m_refs;
}  

HRESULT CDropTarget::RegisterDropWindow(HWND hWnd,IDropCallback *pCallback)
{
    ASSERT(m_pDropTargetHelper == NULL);
    ASSERT(m_hwndDropTarget == NULL);

    m_hwndDropTarget = hWnd;
    m_pCallback = pCallback;

    CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_INPROC_SERVER,
                     IID_IDropTargetHelper,(LPVOID*)&m_pDropTargetHelper);

    return RegisterDragDrop(m_hwndDropTarget,static_cast<IDropTarget*>(this));
}

HRESULT CDropTarget::RevokeDragDrop()
{
    return ::RevokeDragDrop(m_hwndDropTarget);
}

HRESULT CDropTarget::EnableDrop(BOOL bDrop)
{
    if( bDrop )
        m_fControl &= ~DDCF_DISABLEDROP;
    else
        m_fControl |= DDCF_DISABLEDROP;
    return S_OK;
}

#if 0
HRESULT CDropTarget::SetControlFlags(ULONG fFlags)
{
    m_fControl = fFlags;
    return S_OK;
}
#endif

HRESULT CDropTarget::GetControlFlags(ULONG *pfFlags)
{
    *pfFlags = m_fControl;
    return S_OK;
}

HRESULT CDropTarget::SetDropDescription(IDataObject* pDataObject, DWORD dropEffect, DWORD DragPhase, DRAGEFFECT *pde )
{
    HRESULT hr = E_FAIL;

    PCWSTR strMessage = pde->Message;
    PCWSTR strInsert = pde->InsertMessage;

    FORMATETC format = { 
        (CLIPFORMAT) RegisterClipboardFormat(CFSTR_DROPDESCRIPTION), 
        NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    STGMEDIUM storageMedium = { 0 };
    storageMedium.tymed = TYMED_HGLOBAL;
    storageMedium.hGlobal = GlobalAlloc(GHND, sizeof(DROPDESCRIPTION));
    if (storageMedium.hGlobal)
    {
        DROPDESCRIPTION* pDD = (DROPDESCRIPTION*) ::GlobalLock(storageMedium.hGlobal);

        pDD->type = (DROPIMAGETYPE) dropEffect;
        if (strMessage)
            lstrcpyn(pDD->szMessage, strMessage, MAX_PATH);
        if (strInsert)
            lstrcpyn(pDD->szInsert, strInsert, MAX_PATH);

        GlobalUnlock(storageMedium.hGlobal);
        pDataObject->SetData(&format, &storageMedium, TRUE);

        GlobalFree(storageMedium.hGlobal);
    }

    return hr;
}

HRESULT CDropTarget::DragEnter(IDataObject *pDataObject,DWORD fKeyState,POINTL pt,DWORD *pdwEffect)
{
    DRAGEFFECT de = {0};

    m_bSupportedFormat = FALSE;

    if( m_fControl & DDCF_DISABLEDROP )
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

#if 1
    IEnumFORMATETC *penumfmt;
    BOOL bFind = FALSE;
    FORMATETC formatetc;
    ULONG ulFetched;
    pDataObject->EnumFormatEtc(DATADIR_GET,&penumfmt);
    while ( S_OK == penumfmt->Next(1,&formatetc,&ulFetched) )
    {
        if( formatetc.cfFormat == CF_HDROP ||
            formatetc.cfFormat == m_cfShellItemIdList || 
            formatetc.cfFormat == GetLongFilePathClipboardFormat() )
        {
            bFind = TRUE;
            break;
        }
    }
    penumfmt->Release();
    if( !bFind )
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }
#else
    FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    if( pDataObject->QueryGetData(&fe) != S_OK )
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }
#endif
    m_bSupportedFormat = TRUE;

    if (m_pDropTargetHelper)
    {
        // Use the helper if we have one.
        POINT ptPos = { pt.x, pt.y };
        m_pDropTargetHelper->DragEnter(m_hwndDropTarget, pDataObject, &ptPos, *pdwEffect);
    }

    if( _IS_ENABLE_MOVE(m_fControl) && GetKeyState(VK_SHIFT) < 0 )
        *pdwEffect = DROPEFFECT_MOVE;
    else
        *pdwEffect = DROPEFFECT_COPY;

    if( m_pCallback )
    {
        if( m_pCallback->QueryEnter(pDataObject,fKeyState,pt,pdwEffect,&de) != S_OK )
        {
            return S_OK;
        }
    }

    m_pDataObject = pDataObject;
    SetDropDescription(pDataObject, *pdwEffect, 0, &de);

    return S_OK;
}

HRESULT CDropTarget::DragOver(DWORD fKeyState,POINTL pt,DWORD *pdwEffect)
{
    DRAGEFFECT de = {0};

    if( _IS_DISABLE_DROP(m_fControl) )
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    if (m_pDropTargetHelper)
    {
        // Use the helper if we have one.
        POINT ptPos = { pt.x, pt.y };
        m_pDropTargetHelper->DragOver(&ptPos, *pdwEffect);
    }

    if( m_bSupportedFormat )
    {
        // Checks the item under the pointer and processes it if necessary.
        // Returns DROPEFFECT_NONE if it cannot be dropped.
        if( _IS_ENABLE_MOVE(m_fControl) && GetKeyState(VK_SHIFT) < 0 )
            *pdwEffect = DROPEFFECT_MOVE;
        else
            *pdwEffect = DROPEFFECT_COPY;
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    if( m_pCallback->QueryOver(fKeyState,pt,pdwEffect,&de) != S_OK )
    {
        return S_OK;
    }

    if( m_pDataObject )
        SetDropDescription(m_pDataObject, *pdwEffect, 1, &de);

    return S_OK;
}

HRESULT CDropTarget::DragLeave(void)
{
    if( m_pDropTargetHelper )
    {
        m_pDropTargetHelper->DragLeave();
    }

    m_pCallback->DragLeave();

    m_pDataObject = NULL;

    return S_OK;
}

HRESULT CDropTarget::Drop(IDataObject *pDataObject,DWORD fKeyState,POINTL pt,DWORD *pdwEffect)
{
    HRESULT hr;

    if( _IS_DISABLE_DROP(m_fControl) )
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    if (m_pDropTargetHelper)
    {
        POINT ptPos = { pt.x, pt.y };
        m_pDropTargetHelper->Drop(pDataObject, &ptPos, *pdwEffect);
    }

    //
    // Support Formats
    //
    CLIPFORMAT cfHDROP,cfLongPath,cfShellItem;
    cfHDROP     = CF_HDROP;
    cfLongPath  = (CLIPFORMAT)GetLongFilePathClipboardFormat();
    cfShellItem = (CLIPFORMAT)m_cfShellItemIdList;

    //
    // Drop Target Format
    //
    CLIPFORMAT cfDropUsing = cfLongPath; // FSDirWalker's default
    if( m_pCallback )
    {
        if( m_pCallback->QueryDropFormat(pDataObject,&cfDropUsing) != S_OK )
        {
            ;
        }
    }

    FORMATETC fe = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stgm;

    if( cfDropUsing == cfLongPath )
    {
        //
        // Long Path Format
        //
        fe.cfFormat = cfLongPath;

        if( SUCCEEDED(pDataObject->GetData(&fe,&stgm)) )
        {
            if( stgm.hGlobal != NULL )
            {
                if( _IS_ENABLE_MOVE(m_fControl) && (fKeyState & MK_SHIFT) && (*pdwEffect & DROPEFFECT_MOVE) )
                    *pdwEffect = DROPEFFECT_MOVE;
                else
                    *pdwEffect = DROPEFFECT_COPY;

                if( m_pCallback )
                    m_pCallback->DropLongPath(stgm.hGlobal,fKeyState,pt,*pdwEffect);

                ReleaseStgMedium(&stgm);

                return S_OK;
            }
        }
    }

    if( cfDropUsing == cfHDROP )
    {
        //
        // CF_HDROP
        //
        fe.cfFormat = cfHDROP;

        if( SUCCEEDED(pDataObject->GetData(&fe,&stgm)) )
        {
            if( stgm.hGlobal != NULL )
            {
                if( _IS_ENABLE_MOVE(m_fControl) && (fKeyState & MK_SHIFT) && (*pdwEffect & DROPEFFECT_MOVE) )
                    *pdwEffect = DROPEFFECT_MOVE;
                else
                    *pdwEffect = DROPEFFECT_COPY;

                HGLOBAL hSrcData = stgm.hGlobal;
                DROPFILES *pSrcDrop;
                ULONG cFileCount = 0;

                pSrcDrop = (DROPFILES *)GlobalLock(hSrcData);

                if( pSrcDrop->fWide )
                {
                    if( m_pCallback )
                    {
                        m_tempGlobalSize = GlobalSize(hSrcData);
                        m_pCallback->DropFiles(pSrcDrop,fKeyState,pt,*pdwEffect);
                        m_tempGlobalSize = 0;                   
                    }
                }

                GlobalUnlock(hSrcData);

                ReleaseStgMedium(&stgm);

#if 0 
				// NOTE:
				// Currently there is no way to notify here of the success or 
				// failure of the copy, or if the user canceled it.
                if( *pdwEffect == DROPEFFECT_MOVE )
                {
                    // Remove from drop source (e.g. Windows Explorer)
                    //
                    SetPerformedDropEffect(pDataObject,*pdwEffect);
                }
#endif
                return S_OK;
            }
        }
    }

    if( cfDropUsing == cfShellItem )
    {
        //
        // CFSTR_SHELLIDLIST
        //
#if 0
        // The target extracts the CFSTR_PREFERREDDROPEFFECT data. 
        // If it is set to only DROPEFFECT_MOVE, the target can either 
        // do an optimized move or simply copy the data.
        FORMATETC fePreferredDropEffect = { (CLIPFORMAT)RegisterClipboardFormat( CFSTR_PREFERREDDROPEFFECT ), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        if( SUCCEEDED(pDataObject->GetData(&fePreferredDropEffect,&stgm)) )
        {
            if( stgm.tymed == TYMED_HGLOBAL  && stgm.hGlobal != NULL )
            {
                LPDWORD pdw = (LPDWORD)GlobalLock(stgm.hGlobal);
                if( *pdw & DROPEFFECT_MOVE )
                {
                    __noop; // optimized move or simply copy the data.
                }
                GlobalUnlock(stgm.hGlobal);
            }
            ReleaseStgMedium(&stgm);
        }
#endif
        fe.cfFormat = cfShellItem;

        if( SUCCEEDED(pDataObject->GetData(&fe,&stgm)) )
        {
            if( stgm.hGlobal != NULL )
            {
                if( _IS_ENABLE_MOVE(m_fControl) && (fKeyState & MK_SHIFT) && (*pdwEffect & DROPEFFECT_MOVE) )
                    *pdwEffect = DROPEFFECT_MOVE;
                else
                    *pdwEffect = DROPEFFECT_COPY;

                HGLOBAL hSrcData = stgm.hGlobal;

                LPIDA pSrcDrop = (LPIDA)GlobalLock(hSrcData);

                if( m_pCallback )
                {
                    m_tempGlobalSize = GlobalSize(hSrcData);
                    hr = m_pCallback->DropShellItemArray(pSrcDrop,fKeyState,pt,*pdwEffect);
                    m_tempGlobalSize = 0;                   
                }

                GlobalUnlock(hSrcData);

                ReleaseStgMedium(&stgm);
#if 0
                if( hr == S_OK && *pdwEffect == DROPEFFECT_MOVE )
                {
                    // NOTE:
                    // Move operation. Be careful because the original file will be deleted!
                    //
                    SetPasteSucceeded(DataObject,DROPEFFECT_MOVE);
                    SetPerformedDropEffect(pDataObject,DROPEFFECT_MOVE);
                }
#endif
                return S_OK;
            }
        }
    }

    *pdwEffect = DROPEFFECT_NONE;

    return S_OK;
}
