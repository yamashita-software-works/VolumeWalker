#pragma once
//*****************************************************************************
//*                                                                           *
//*  page_volumefilelist_drag_and_drop.h                                      *
//*                                                                           *
//*  Drag and Drop Handler                                                    *
//*                                                                           *
//*  Author:  YAMASHITA Katsuhiro                                             *
//*                                                                           *
//*  History: 2024-11-02 Created.                                             *
//*                                                                           *
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "dataobject.h"
#include "dropsource.h"
#include "droptarget.h"
#include "clipboardfilecopy.h"

//////////////////////////////////////////////////////////////////////////////

template <class T>
class CDragHandler
{
public:
	LRESULT OnBeginDrag(NMHDR *pnmhdr,HWND hWndList,PCWSTR pszPath)
	{
		LPNMLISTVIEW *pnmlv = (LPNMLISTVIEW *)pnmhdr;

		CFileListCache *pFiles = new CFileListCache;

		int iItem = -1;
		while( (iItem = ListView_GetNextItem(hWndList,iItem,LVNI_SELECTED)) != -1 )
		{
			CFileLvItem *pItem = (CFileLvItem *)ListViewEx_GetItemData(hWndList,iItem);

			ASSERT(pItem != NULL);

			if( wcscmp(pItem->pFI->hdr.FileName,L"..") == 0 )
			{
				continue;
			}

			PWSTR pPath;
			if( pItem->pFI->hdr.Path != NULL )
			{
				pPath = CombinePath(pItem->pFI->hdr.Path,pItem->pFI->hdr.FileName);
			}
			else
			{
				pPath = CombinePath(pszPath,pItem->pFI->hdr.FileName);
			}

			WCHAR szDosPath[MAX_PATH];
			if( PathIsPrefixDosDeviceDrive(pPath) )
				StringCchCopy(szDosPath,MAX_PATH,&pPath[4]);
			else
				NtPathToDosPath(pPath,szDosPath,MAX_PATH);
	
			pFiles->AddFullPathFileNameEx( szDosPath, pPath );
	
			FreeMemory(pPath);
		}

		pFiles->Complete( MAKE_HGLOBAL_HDROP|MAKE_HGLOBAL_IDLIST|MAKE_HGLOBAL_LONGPATH );

		HRESULT hr;

		if( pFiles->GetItemCount() > 0 )
		{
			hr = StartDragDrop(pFiles,DROPEFFECT_COPY|DROPEFFECT_LINK,NULL,0);
		}
		else
		{
			hr = S_FALSE;
		}

		delete pFiles;

		return 0; // no return value
	}
};

//////////////////////////////////////////////////////////////////////////////

template <class T>
class CDropHandler : public IDropCallback
{
protected:
	DWORD m_fDropKeyState;
	CDropTarget *m_pDropTarget;

private:
	HWND GetListViewCtrl()
	{
		T *pThis = static_cast<T*>(this);
		return pThis->GetListView();
	}

	HRESULT GetDropDirectory(HWND hWndList,POINTL pt,PWSTR *pszDropDirectory)
	{
		*pszDropDirectory = NULL;
	
		POINT ptClient = { pt.x, pt.y };
		ScreenToClient(hWndList,&ptClient);
		UINT Flags = LVHT_ONITEM;
		int iItem;
		iItem = ListViewEx_HitTest(hWndList,ptClient,&Flags);
		if( iItem != -1 )
		{
			CFileLvItem *pItem = (CFileLvItem *)ListViewEx_GetItemData(hWndList,iItem);
			if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				*pszDropDirectory = _MemAllocString(pItem->pFI->hdr.FileName);
			}
		}
		return S_OK;
	}

public:
	CDropHandler()
	{
		m_fDropKeyState = 0;
		m_pDropTarget = NULL;
	}

	~CDropHandler()
	{
	}

	HRESULT QueryItemOnPointer(IDataObject *pDataObject,DWORD fKeyState,POINTL pt,DWORD *pdwEffect,DRAGEFFECT *pDragEffect)
	{
		HWND m_hWndList = GetListViewCtrl();

		pDragEffect->Message[0] = L'\0';
		pDragEffect->InsertMessage[0] = L'\0';

		m_fDropKeyState = fKeyState;

		ULONG fControl;
		m_pDropTarget->GetControlFlags(&fControl);

		POINT ptClient = { pt.x, pt.y };
		ScreenToClient(m_hWndList,&ptClient);

		UINT Flags = LVHT_ONITEM;
		int iItem;
		iItem = ListViewEx_HitTest(m_hWndList,ptClient,&Flags);

		ListView_SetItemState(m_hWndList,-1,0,LVIS_DROPHILITED);

		if( iItem != -1 )
		{
			int fSelected = ListView_GetItemState(m_hWndList,iItem,LVIS_SELECTED);
			if( fSelected == 0 )
			{
				CFileLvItem *pItem = (CFileLvItem *)ListViewEx_GetItemData(m_hWndList,iItem);
				if( pItem->pFI->FileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				{
					StringCchPrintf(pDragEffect->Message,ARRAYSIZE(pDragEffect->Message),L"Drop copy to: '%s'",pItem->pFI->hdr.FileName);
					StringCchCopy(pDragEffect->InsertMessage,ARRAYSIZE(pDragEffect->InsertMessage),L"Drag Effect Insert Message");
					ListView_SetItemState(m_hWndList,iItem,LVIS_DROPHILITED,LVIS_DROPHILITED);
					return S_OK; // continue drop operation.
				}
			}
		}

//		StringCchCopy(pDragEffect->Message,ARRAYSIZE(pDragEffect->Message),L"@Placeholder");
//		StringCchCopy(pDragEffect->InsertMessage,ARRAYSIZE(pDragEffect->InsertMessage),L"@Placeholder Insert Message");

		if( _IS_DROP_FROM_SELF(fControl) )
		{
			*pdwEffect = DROPEFFECT_NONE;
			return S_FALSE;
		}

		*pdwEffect = DROPEFFECT_COPY;
		return S_OK;
	}

	HRESULT QueryDataType(IDataObject *pDataObject,CLIPFORMAT *pcfDataType)
	{
		FORMATETC fcLongPath = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		FORMATETC fcHDROP = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		FORMATETC fcShell = { 0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	
		fcLongPath.cfFormat = (CLIPFORMAT)GetLongFilePathClipboardFormat();
		fcHDROP.cfFormat = CF_HDROP;
		fcShell.cfFormat = (CLIPFORMAT)RegisterClipboardFormat( CFSTR_SHELLIDLIST );
	
		HRESULT hr;
	
		hr = pDataObject->QueryGetData(&fcLongPath);
		if( hr == S_OK )
		{
			*pcfDataType = fcLongPath.cfFormat;
			return S_OK;
		}
	
		hr = pDataObject->QueryGetData(&fcHDROP);
		if( hr == S_OK )
		{
			*pcfDataType = fcHDROP.cfFormat;
			return S_OK;
		}
	
		hr = pDataObject->QueryGetData(&fcShell);
		if( hr == S_OK )
		{
			*pcfDataType = fcShell.cfFormat;
			return S_OK;
		}
	
		return E_FAIL;
	}

	HRESULT DropWithCommandMenu(HGLOBAL hGlobal,DROPFILES *pDropFiles,DWORD fKeyState,POINTL pt,DWORD dwEffect)
	{
		return E_NOTIMPL;
	}

	HRESULT CreateDropFileList(DROPFILES *pDropFiles,CFileOperationList **pFileList)
	{
		ULONG cFileCount = 0;
		PWSTR pSrcFile;
		HRESULT hr = E_FAIL;
		WCHAR NtFilePath[MAX_PATH];

		// Count the number of files
		pSrcFile = (LPWSTR)((ULONG_PTR)pDropFiles + pDropFiles->pFiles);
		while( *pSrcFile )
		{
			cFileCount++;
			pSrcFile += (wcslen(pSrcFile) + 1);
		}
	
		if( cFileCount == 0 )
			return E_FAIL;

		CFileOperationList *pFiles = new CFileOperationList(cFileCount);

		cFileCount = 0;
		pSrcFile = (LPWSTR)((ULONG_PTR)pDropFiles + pDropFiles->pFiles);

		while( *pSrcFile )
		{
			if( DosPathToNtDevicePath(pSrcFile,NtFilePath,MAX_PATH,0) )
			{
				pFiles->Add( NtFilePath );
			}
			pSrcFile += (wcslen(pSrcFile) + 1);
		}

		*pFileList = pFiles;

		hr = S_OK;

		return hr;
	}

	HRESULT DropFiles(DROPFILES *pDropFiles,DWORD fKeyState,POINTL pt,DWORD dwEffect)
	{
		HRESULT hr = E_FAIL;
	
		HWND m_hWndList = GetListViewCtrl();

		PWSTR pszDropDirectory=NULL;
		GetDropDirectory(m_hWndList,pt,&pszDropDirectory);
	
		SIZE_T cb = m_pDropTarget->GetGlobalSize();
	
		HGLOBAL hGlobal = GlobalAlloc(GPTR,cb);
	
		if( hGlobal != NULL )
		{
			LPBYTE pb = (LPBYTE)GlobalLock(hGlobal);
	
			if( pb != NULL )
			{
				CopyMemory(pb,(LPBYTE)pDropFiles,cb);
	
				GlobalUnlock(hGlobal);
	
				CFileOperationList *dropFileList = NULL;
				DROPFILES *pSrcDrop;
				HRESULT hr = E_FAIL;
	
				pSrcDrop = (DROPFILES *)GlobalLock(hGlobal);
	
				if( pSrcDrop->fWide )
				{
					if( m_fDropKeyState & MK_RBUTTON )
					{
						hr = DropWithCommandMenu(NULL,pSrcDrop,fKeyState,pt,dwEffect);
					}
	
					hr = CreateDropFileList(pSrcDrop,&dropFileList);
	
					if( hr  == S_OK )
					{
						T *pThis = static_cast<T*>(this);
						dropFileList->SetDestinationDirectory( pThis->GetPath(), pszDropDirectory );
	
						PostMessage(pThis->m_hWnd,PM_FILEOPERATION,0,(LPARAM)dropFileList);
					}
				}
			}
	
			GlobalUnlock(hGlobal);
	
			GlobalFree(hGlobal);
	
			hr = S_OK;
		}
	
		ListView_SetItemState(m_hWndList,-1,0,LVIS_DROPHILITED);
	
		return hr;
	}

	HRESULT DropLongPath(HGLOBAL hGlobal,DWORD fKeyState,POINTL pt,DWORD dwEffect)
	{
		HRESULT hr = E_FAIL;

		HWND m_hWndList = GetListViewCtrl();
	
		PWSTR pszDropDirectory=NULL;
		GetDropDirectory(m_hWndList,pt,&pszDropDirectory);
	
		if( m_fDropKeyState & MK_RBUTTON )
		{
			hr = DropWithCommandMenu(hGlobal,NULL,fKeyState,pt,dwEffect);
		}
		else
		{
			if( dwEffect == DROPEFFECT_MOVE )
			{
				hr = E_FAIL;
			}
			else
			{
				CPopClipboardLongFilePath lfpcbPop;
				PWSTR pszSrcPath = NULL;
				int cFileNameListCount = 0;
	
				if( lfpcbPop.Open(0,hGlobal) )
				{
					FS_LONGFILEPATH_CLIPBOARD_NAME *pName = lfpcbPop.GetTop();
	
					while( pName->Length != 0 )
					{
						cFileNameListCount++;
						pName = lfpcbPop.Next(pName);
					}
	
					CFileOperationList *pFiles = new CFileOperationList( cFileNameListCount);
	
					if( pFiles != NULL )
					{
						pName = lfpcbPop.GetTop();
						while( pName->Length != 0 )
						{
							ULONG fType = FoItemPath;
							if( pName->Flags == FSLCNF_DIRECTORY_PATH )
								fType =FoItemDirectory;
							else if( pName->Flags == FSLCNF_FILENAME )
								fType = FoItemFile;
	
							pFiles->Add( pName->Buffer, fType );
	
							pName = lfpcbPop.Next(pName);
						}
	
						T *pThis = static_cast<T*>(this);
						pFiles->SetDestinationDirectory( pThis->GetPath(), pszDropDirectory );
					}
					lfpcbPop.Close();
	
					T *pThis = static_cast<T*>(this);
					PostMessage(pThis->m_hWnd,PM_FILEOPERATION,0,(LPARAM)pFiles);
				}
			}
		}
	
		GlobalFree(hGlobal);
	
		_MemFree(pszDropDirectory);
	
		ListView_SetItemState(m_hWndList,-1,0,LVIS_DROPHILITED);
	
		return hr;
	}

	HRESULT DropShellItemArray(LPIDA pida,DWORD fKeyState,POINTL pt,DWORD dwEffect)
	{
		return E_NOTIMPL;
	}
	
	HRESULT QueryEnter(IDataObject *pDataObject,DWORD fKeyState,POINTL pt,DWORD *pdwEffect,DRAGEFFECT *pDragEffect)
	{
		return QueryItemOnPointer(pDataObject,fKeyState,pt,pdwEffect,pDragEffect);
	}
	
	HRESULT QueryOver(DWORD fKeyState,POINTL pt,DWORD *pdwEffect,DRAGEFFECT *pDragEffect)
	{
		return QueryItemOnPointer(NULL,fKeyState,pt,pdwEffect,pDragEffect);
	}
	
	HRESULT QueryDropFormat(IDataObject *pDataObject,CLIPFORMAT *pcfDataType)
	{
		return QueryDataType(pDataObject,pcfDataType);
	}
	
	HRESULT DragLeave(VOID)
	{
		T *pThis = static_cast<T*>(this);
		ListView_SetItemState(pThis->GetListView(),-1,0,LVIS_DROPHILITED);
		return S_OK;
	}
};

//////////////////////////////////////////////////////////////////////////////

template <class T>
class CClipboardFileList
{
private:
	HWND GetListViewCtrl()
	{
		T *pThis = static_cast<T*>(this);
		return pThis->GetListView();
	}

public:
	int PushClipboardLongPath()
	{
		int iItem = ListView_GetNextItem(((T*)this)->GetListView(),-1,LVNI_SELECTED);
		if( iItem == -1 )
			return 0;
	
		ULONG cch = 0;
		ULONG cchMax = 0;
	
		CLongFilePathClipboard lfpc;
	
		lfpc.Start( NULL );
	
		iItem = -1;
		while( (iItem = ListView_GetNextItem(((T*)this)->GetListView(),iItem,LVNI_SELECTED)) != -1 )
		{
			CFileLvItem *pItem = (CFileLvItem *)ListViewEx_GetItemData(hWndList,iItem);

			PWSTR pPath = NULL;

			if( pItem->pFI->hdr.Path != NULL )
			{
				pPath = CombinePath(pItem->pFI->hdr.Path,pItem->pFI->hdr.FileName);
			}
			else
			{
				T *pThis = static_cast<T*>(this);
				pPath = CombinePath(pThis->GetPath(),pItem->pFI->hdr.FileName);
			}

			if( pPath )
				lfpc.AddFullPath( pPath );
		}
	
		lfpc.Commit(((T*)this)->m_hWnd);
	
		return 0;
	}

	int PopClipboardLongPath(HWND hwndClipboard,HGLOBAL hGlobal)
	{
		//
		// Make PCWSTR array from CPopClipboardLongFilePath.
		//
		CPopClipboardLongFilePath lfpcbPop;
		PCWSTR *pFileNameList = NULL;
		ULONG cFileNameListCount = 0;
		PWSTR pszSrcPath = NULL;
	
		if( lfpcbPop.Open(hwndClipboard,hGlobal) )
		{
			FS_LONGFILEPATH_CLIPBOARD_NAME *pName = lfpcbPop.GetTop();
	
			cFileNameListCount = 0;
	
			//
			// If include source path, set to pszSrcPath.
			// If not include that, pszSrcPath is still NULL.
			//
			while( pName->Length != 0 )
			{
				if( pszSrcPath == NULL && pName->Flags == FSLCNF_DIRECTORY_PATH )
					pszSrcPath = (PWSTR)_MemAllocString( pName->Buffer );
				else
					cFileNameListCount++;
				pName = lfpcbPop.Next(pName);
			}
			
			CFileOperationList fol(cFileNameListCount); // test
	
			pFileNameList = (PCWSTR*)_MemAllocZero( sizeof(PCWSTR) * cFileNameListCount );
	
			if( pFileNameList != NULL )
			{
				ULONG i = 0;
				pName = lfpcbPop.GetTop();
				while( pName->Length != 0 )
				{
					if( pName->Flags ==	FSLCNF_FILENAME || pName->Flags == FSLCNF_FULLPATH )
						pFileNameList[i++] = pName->Buffer;
	
					fol.Add(pName->Buffer,pName->Flags);
	
					pName = lfpcbPop.Next(pName);
				}
	
				cFileNameListCount = fol.GetItemCount();
			}
	
			lfpcbPop.Close();
		}
	
		_SafeMemFree(pFileNameList);
		_SafeMemFree(pszSrcPath);
	
		return 0;
	}

	int PopClipboardShellItem(HWND hwndClipboard,HGLOBAL hGlobal)
	{
		DWORD dwErrorCode = 0;
	
		if( OpenClipboard(hwndClipboard) )
		{
			HGLOBAL	hSrcData = GetClipboardData( RegisterClipboardFormat( CFSTR_SHELLIDLIST ) );
	
			if( hSrcData )
			{
				LPIDA pSrcDrop = (LPIDA)GlobalLock(hSrcData);
	
				SIZE_T cbGlobalSize = GlobalSize(hSrcData);
	#if 0
				PCWSTR pszDestPath = /* Get destination path */
	
				HRESULT hr;
				hr = ShellFileOperation(_GetMainWnd(),pszDestPath,pSrcDrop);
				if( FAILED(hr) )
				{
					_ErrorMessageBox(_GetMainWnd(),IDS_ERROR_SHELLITEM_COPY_FAILED,CHexString(hr),hr,MB_OK|MB_ICONEXCLAMATION);
				}
	#endif
				GlobalUnlock(hSrcData);
			}
			else
			{
				dwErrorCode = GetLastError();
			}

			CloseClipboard();
		}
		else
		{
			dwErrorCode = GetLastError();
		}
	
		if( dwErrorCode != 0 )
		{
			_ErrorMessageBox(_GetMainWnd(),IDS_ERROR_SHELLITEM_COPY_FAILED,CHexString(dwErrorCode),dwErrorCode,MB_OK|MB_ICONEXCLAMATION);
		}

		return S_OK;
	}

	int PushClipboardHDROP()
	{
		HWND hWndList = ((T*)this)->GetListView();

		int iResult = ERROR_INVALID_FUNCTION;
	
		int iItem = ListView_GetNextItem(hWndList,-1,LVNI_SELECTED);
		if( iItem == -1 )
			return 0;
	
		ULONG cch = 0;
		ULONG cchMax = 0;
	

		CFileListCache *pFiles = new CFileListCache;
	
		iItem = -1;
		while( (iItem = ListView_GetNextItem(hWndList,iItem,LVNI_SELECTED)) != -1 )
		{
			CFileLvItem *pItem = (CFileLvItem *)ListViewEx_GetItemData(hWndList,iItem);
	/*++	CLVItem *pFI = (CLVIten*)m_List.GetItemData(iItem);
	
			if( pFI->Status == 0 )
			{
				lfpc.AddFullPath( pFI->FullPath );
			} --*/
		}
	
		if( pFiles->GetItemCount() > 0 )
		{
			pFiles->Complete(MAKE_HGLOBAL_HDROP|MAKE_HGLOBAL_IDLIST);
	
			IDataObject *pDataObject = new CFilePathDataObject;
	
			FORMATETC formatc   = { 0, NULL,DVASPECT_CONTENT, -1,TYMED_HGLOBAL };
			STGMEDIUM stgmedium = { TYMED_HGLOBAL, { NULL }, NULL };
	
			if( pFiles->GetHDROP() )
			{
				formatc.cfFormat = CF_HDROP;
				stgmedium.hGlobal = pFiles->GetHDROP();
				pDataObject->SetData(&formatc,&stgmedium,FALSE);
			}
	
			if( pFiles->GetPIDL() )
			{
				formatc.cfFormat = (CLIPFORMAT)RegisterClipboardFormat( CFSTR_SHELLIDLIST );
				stgmedium.hGlobal = pFiles->GetPIDL();
				pDataObject->SetData(&formatc,&stgmedium,FALSE);
			}
	
			// Set OLE Clipboard
			OleSetClipboard((IDataObject *)pDataObject);
	
			pDataObject->Release();
	
			iResult = ERROR_SUCCESS;
		}
		else
		{
			iResult = ERROR_NO_MORE_FILES;
		}
	
		delete pFiles;
	
		return iResult;
	}

	int PopClipboardHDROP()
	{
		//
		// Make PCWSTR array from CPopClipboardHDROP.
		//
		CPopClipboardHDROP popcbHDROP;
	
		PCWSTR *pFileNameList = NULL;
		UNICODE_STRING *pusFileNameList = NULL;
	
		ULONG cFileNameListCount = 0;
	
		PWSTR pszSrcPath = NULL;
	
		if( popcbHDROP.Open(((T*)this)->m_hWnd) )
		{
			PCWSTR pszDosPath;
	
			pszDosPath = popcbHDROP.GetTop();
	
			if( pszDosPath != NULL )
			{
				cFileNameListCount = popcbHDROP.GetFileCount();
	
				pFileNameList = (PCWSTR*)_MemAllocZero( sizeof(PCWSTR) * cFileNameListCount );
	
				if( pFileNameList != NULL )
				{
					ULONG i = 0;
					pszDosPath = popcbHDROP.GetTop();
					while( *pszDosPath != L'\0' )
					{
						pFileNameList[i++] = pszDosPath;
						pszDosPath = popcbHDROP.Next(pszDosPath);
					}
				}
	
				_SafeMemFree(pFileNameList);
			}
	
			popcbHDROP.Close();
		}
	
		return 0;
	}

	LRESULT OnPushClipboardFileName(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/)
	{
		EmptyClipboard();
	
		int iStatusHDROP,iStatusLongPath;
	
		iStatusHDROP = PushClipboardHDROP();
	
		iStatusLongPath = PushClipboardLongPath();
	
		return 0;
	}
	
	LRESULT OnPopClipboardFileName()
	{
		if( IsLongFilePathClipboardAvailable() )
			PopClipboardLongPath(((T*)this)->m_hWnd,NULL);
		else if( IsClipboardFormatAvailable(CF_HDROP) )
			PopClipboardHDROP();
		else if( IsClipboardFormatAvailable(RegisterClipboardFormat( CFSTR_SHELLIDLIST )) )
			PopClipboardShellItem(((T*)this)->m_hWnd,NULL);
		return 0;
	}
};
