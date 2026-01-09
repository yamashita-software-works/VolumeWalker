#pragma once

#include "DataObject.h"

class CDropSource : public IDropSource
{
public:    
    CDropSource();

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    /* IDropSource methods */
    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);
 
private:
    ULONG m_refs;
};  

HRESULT
WINAPI
StartDragDrop(
	CFileListCache *pFiles,
	DWORD dwEnableEffect,
	DWORD *pdwResultEffect,
	DWORD OptionFlags
	);
