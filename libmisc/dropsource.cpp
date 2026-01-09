//***************************************************************************
//*                                                                         *
//*  datasource.cpp                                                         *
//*                                                                         *
//*  CLASS:    CDropSource                                                  *
//*                                                                         *
//*  PURPOSE:  IDropSource implementation.                                  *
//*                                                                         *
//*  AUTHOR:   YAMASHITA Katsuhiro                                          *
//*                                                                         *
//*  HISTORY:  2006-09-02,2015-08-18 - Created                              *
//*                                                                         *
//***************************************************************************
//   Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//   Licensed under the MIT License.
#include "stdafx.h"
#include "dropsource.h"
#include "dataobject.h"
#include "clipboardfilecopy.h"

// msdn support information: Q203327
// SUMMARY:
// When an application is serving as a file drag-and-drop source,
// it is necessary for the source's IDataObject to support multiple
// clipboard formats to allow the file object to be dropped on a shortcut. 
// MORE INFORMATION:
// When a file object is dropped on a shortcut to an application, 
// the application that the shortcut refers to is launched with the dropped file name 
// supplied on the application's command line. To allow this to work correctly, 
// the drag-and-drop source's IDataObject must support both the CF_HDROP and
// the CFSTR_SHELLIDLIST clipboard formats.

HRESULT WINAPI StartDragDrop(CFileListCache *pFiles,DWORD dwEnableEffect,DWORD *pdwResultEffect,DWORD OptionFlags)
{
    HRESULT hr = E_FAIL;

    IDataObject *pDataObject = new CFilePathDataObject;
    if( pDataObject == NULL )
        return E_FAIL;

    FORMATETC formatc   = { 0, NULL,DVASPECT_CONTENT, -1,TYMED_HGLOBAL };
    STGMEDIUM stgmedium = { TYMED_HGLOBAL, { NULL }, NULL };

    if( pFiles->GetHDROP() )
    {
        formatc.cfFormat = CF_HDROP;
        stgmedium.hGlobal = pFiles->GetHDROP();
        pDataObject->SetData(&formatc,&stgmedium,FALSE);
    }

    if( pFiles->GetLongPath() )
    {
        formatc.cfFormat = (CLIPFORMAT)GetLongFilePathClipboardFormat();
        stgmedium.hGlobal = pFiles->GetLongPath();
        pDataObject->SetData(&formatc,&stgmedium,FALSE);
    }

    if( pFiles->GetPIDL() )
    {
        formatc.cfFormat = (CLIPFORMAT)(CLIPFORMAT)RegisterClipboardFormat( CFSTR_SHELLIDLIST );
        stgmedium.hGlobal = pFiles->GetPIDL();
        pDataObject->SetData(&formatc,&stgmedium,FALSE);
    }

    IDropSource *pDropSource = new CDropSource;

    if (pDropSource)
    {
        DWORD dwResultEffect;

        hr = DoDragDrop(pDataObject,
                   pDropSource,
                   dwEnableEffect,
                   &dwResultEffect);     

        if( pdwResultEffect )
            *pdwResultEffect = dwResultEffect;

        pDropSource->Release();
    }
    else
    {
        hr = E_FAIL;        
    }

    pDataObject->Release();

    return hr;
}

//---------------------------------------------------------------------
//                    IUnknown Methods
//---------------------------------------------------------------------

STDMETHODIMP CDropSource::QueryInterface(REFIID iid, void FAR* FAR* ppv) 
{
    if(iid == IID_IUnknown || iid == IID_IDropSource)
    {
        *ppv = this;
        ++m_refs;
        return NOERROR;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDropSource::AddRef(void)
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG) CDropSource::Release(void)
{
    if(--m_refs == 0)
    {
        delete this;
        return 0;
    }
    return m_refs;
}  

//---------------------------------------------------------------------
//                    CDropSource Constructor
//---------------------------------------------------------------------
 
CDropSource::CDropSource()
{
    m_refs = 1;  
}

//---------------------------------------------------------------------
//                    IDropSource Methods
//---------------------------------------------------------------------  

STDMETHODIMP CDropSource::QueryContinueDrag(BOOL fEscapePressed, 
                                            DWORD dwKeyState)
{  
    if (fEscapePressed)
    {
        // Escape key pressed, exit process.
        //
        return DRAGDROP_S_CANCEL;
    }
    else if (!(dwKeyState & MK_LBUTTON) && !(dwKeyState & MK_RBUTTON))
    {
        // NOTE:
        // Avoids the phenomenon that the mouse cursor for the operation
        // specified by dwEnableEffect of DoDragDrop is displayed when the mouse cursor
        // is moved over the window after dropping to another window 
        // (while a long copy operation is being performed in that window or 
        // a confirmation dialog is being displayed).
        HCURSOR h = LoadCursor(NULL,IDC_NO);
        SetCursor( h );

        return DRAGDROP_S_DROP;
    }
    else
    {
        return NOERROR;                  
    }
}

STDMETHODIMP CDropSource::GiveFeedback(DWORD dwEffect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}
