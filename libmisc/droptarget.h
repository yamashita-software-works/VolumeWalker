#pragma once

typedef struct _DRAGEFFECT
{
	WCHAR Message[MAX_PATH];
	WCHAR InsertMessage[MAX_PATH];
} DRAGEFFECT, *PDRAGEFFECT;

interface IDropCallback
{
	virtual HRESULT DropFiles(DROPFILES *pDropFiles,DWORD fKeyState,POINTL pt,DWORD dwEffect) = 0;
	virtual HRESULT DropLongPath(HGLOBAL hGlobal,DWORD fKeyState,POINTL pt,DWORD dwEffect) = 0;
	virtual HRESULT DropShellItemArray(LPIDA pida,DWORD fKeyState,POINTL pt,DWORD dwEffect) = 0;
	virtual HRESULT QueryEnter(IDataObject *pDataObject,DWORD fKeyState,POINTL pt,DWORD *pdwEffect,DRAGEFFECT *pDragEffect) = 0;
	virtual HRESULT QueryOver(DWORD fKeyState,POINTL pt,DWORD *pdwEffect,DRAGEFFECT *pDragEffect) = 0;
	virtual HRESULT DragLeave(VOID) = 0;
	virtual HRESULT QueryDropFormat(IDataObject *pDataObject,CLIPFORMAT *pcfDataType) = 0;
};

class CDropTarget: public IDropTarget
{
	IDataObject *m_pDataObject;
public:
	CDropTarget();
	~CDropTarget();

	STDMETHOD(QueryInterface)(REFIID riid,void **ppvObj);
	STDMETHOD_(ULONG,AddRef)(void);
	STDMETHOD_(ULONG,Release)(void);

	STDMETHOD(DragEnter)(IDataObject * pDataObject,
					     DWORD grfKeyState,
						 POINTL pt,
						 DWORD * pdwEffect);
	STDMETHOD(DragLeave)(void);
	STDMETHOD(DragOver)(DWORD grfKeyState,
						POINTL pt,
						DWORD * pdwEffect );  
	STDMETHOD(Drop)(IDataObject * pDataObject,
					DWORD grfKeyState,
					POINTL pt,
					DWORD * pdwEffect);

	STDMETHOD(RegisterDropWindow)(HWND hWnd,IDropCallback *pCallback);
	STDMETHOD(RevokeDragDrop)();
	STDMETHOD(EnableDrop)(BOOL bEnable);
	STDMETHOD(GetControlFlags)(ULONG *pfFlags);

	SIZE_T GetGlobalSize() { return m_tempGlobalSize; }

private:
	ULONG m_refs;
	HWND m_hwndDropTarget;
	BOOL m_bSupportedFormat;
	ULONG m_fControl;
	IDropTargetHelper *m_pDropTargetHelper;
	IDropCallback *m_pCallback;
	SIZE_T m_tempGlobalSize;
	UINT m_cfShellItemIdList;

protected:
	HRESULT SetDropDescription(IDataObject* pDataObject, DWORD dropEffect, DWORD DragPhase, DRAGEFFECT *pde);
};

#define DDCF_ENABLEMOVE  0x00000001
#define DDCF_STARTSELF   0x00000002
#define DDCF_DISABLEDROP 0x00000004

#define _IS_DISABLE_DROP(f)       (f & DDCF_DISABLEDROP)
#define _IS_ENABLE_MOVE(f)        (f & DDCF_ENABLEMOVE)
#define _IS_DROP_FROM_SELF(f)     (f & DDCF_STARTSELF)
