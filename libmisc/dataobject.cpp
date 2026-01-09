//***************************************************************************
//*                                                                         *
//*  dataobject.cpp                                                         *
//*                                                                         *
//*  CLASS:    CFilePathDataObject                                          *
//*                                                                         *
//*  PURPOSE:  IDataObject implementation.                                  *
//*                                                                         *
//*  AUTHOR:   YAMASHITA Katsuhiro                                          *
//*                                                                         *
//*  HISTORY:  2015-08-18 - Created                                         *
//*                                                                         *
//***************************************************************************
//   Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//   Licensed under the MIT License.
#include "stdafx.h"
#include "libmisc.h"
#include "dataobject.h"
#include "clipboardfilecopy.h"
#include "..\libntwdk\ntpathcomponent.h"

#ifndef _DEBUG_TRACE_
#define _DEBUG_TRACE_ 0
#endif

//////////////////////////////////////////////////////////////////////////////
//
//	CIEnumFORMATETC
//
//  PURPOSE: 
//

BOOL _IsEqualFORMATETC( FORMATETC *pformatetc1, FORMATETC *pformatetc2 )
{
#if _DEBUG_TRACE_
	_TRACE("%u=%u\n%u=%u\n%u=%u\n\n",
			pformatetc1->cfFormat,pformatetc2->cfFormat,
			pformatetc1->dwAspect,pformatetc2->dwAspect,
			pformatetc1->tymed,pformatetc2->tymed);
#endif
	return (pformatetc1->cfFormat == pformatetc2->cfFormat);
}

class CIEnumFORMATETC : public IEnumFORMATETC
{
	ULONG m_ref;
	int m_iIndex;
	int m_cFormatc;
	FORMATETC *m_aFormatc;

public:
	CIEnumFORMATETC()
	{
		m_ref = 0;
		m_iIndex = 0;
		m_cFormatc = 0;
		m_aFormatc = NULL;
	}

	~CIEnumFORMATETC()
	{
		ASSERT(m_ref == 0);
		if(m_aFormatc != NULL)
			delete[] m_aFormatc;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void **ppvObject)
	{
		*ppvObject = this;
		AddRef(); // bugbug:20180625
		return 0;
	}

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		m_ref++;
		return m_ref;
	}

	ULONG STDMETHODCALLTYPE Release(void)
	{
		ULONG n;
		n = --m_ref;
		if( m_ref == 0 )
			delete this;
		return n;
	}

	HRESULT STDMETHODCALLTYPE Next(ULONG celt,FORMATETC* rgelt,ULONG * pceltFetched)
	{
		if( m_iIndex >= m_cFormatc )
			return S_FALSE;

		*rgelt = m_aFormatc[m_iIndex++]; 

		if( pceltFetched )
			*pceltFetched  = 1;

		return S_OK;
	}
 
	HRESULT STDMETHODCALLTYPE Skip(ULONG celt)
	{
		return E_NOTIMPL;
	}
 
	HRESULT STDMETHODCALLTYPE Reset(void)
	{
		m_iIndex = 0;
		return S_OK;
	}	

	HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC **ppenum)
	{
		return E_NOTIMPL;
	}

	VOID SetAvailableFormat(int cFormatc,FORMATETC *aFormatc)
	{
		ASSERT( m_aFormatc == NULL );

		m_cFormatc = cFormatc;
		m_aFormatc = new FORMATETC[cFormatc];
		memcpy(m_aFormatc,aFormatc,cFormatc * sizeof(FORMATETC));
	}
};

//////////////////////////////////////////////////////////////////////////////
//
//	CFilePathDataObject::CFilePathDataObject()
//
//  PURPOSE: 
//

CFilePathDataObject::CFilePathDataObject()
{
	m_ref = 1;
	ZeroMemory(&m_sdt,sizeof(m_sdt));

	FORMATETC formatetc = { 0, NULL,DVASPECT_CONTENT, -1,TYMED_HGLOBAL };
	STGMEDIUM stgmedium = { TYMED_HGLOBAL, { NULL }, NULL };

	m_sdt[cfHDROP].cf = CF_HDROP;
	m_sdt[cfHDROP].formatetc = formatetc;
	m_sdt[cfHDROP].stgmedium = stgmedium;

	m_sdt[cfLONGPATH].cf = (CLIPFORMAT)RegisterClipboardFormat( CFWSTR_LONGFILEPATHCLIPBOARDFORMAT );
	m_sdt[cfLONGPATH].formatetc = formatetc;
	m_sdt[cfLONGPATH].stgmedium = stgmedium;

	m_sdt[cfIDLIST].cf = (CLIPFORMAT)RegisterClipboardFormat( CFSTR_SHELLIDLIST );
	m_sdt[cfIDLIST].formatetc = formatetc;
	m_sdt[cfIDLIST].stgmedium = stgmedium;
}

CFilePathDataObject::~CFilePathDataObject()
{
	ULONG i;
	for(i = 0; i < cfMAX_SIZE; i++)
	{
		if( m_sdt[i].stgmedium.hGlobal != NULL )
		{
			HGLOBAL h = GlobalFree(m_sdt[i].stgmedium.hGlobal);
			ASSERT(h == NULL);
			m_sdt[i].stgmedium.hGlobal = NULL;
		}
	}
}

STDMETHODIMP CFilePathDataObject::QueryInterface(REFIID riid,LPVOID *ppvObj)
{
	if( riid == IID_IUnknown || riid == IID_IDataObject )
	{
		AddRef();
		*ppvObj = this;
		return NOERROR;
	}

	// unkown interface requested
	*ppvObj = NULL;
	return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CFilePathDataObject::AddRef()
{
#if _DEBUG_TRACE_
	_TRACE("DO(0x%08X) ADD : %d -> %d\n",(ULONG_PTR)this,m_ref,m_ref+1);
#endif
	return ++m_ref;
}

STDMETHODIMP_(ULONG) CFilePathDataObject::Release()
{
#if _DEBUG_TRACE_
	_TRACE("DO(0x%08X) REL : %d -> %d\n",(ULONG_PTR)this,m_ref,m_ref-1);
#endif
	if( --m_ref == 0)
	{
		delete this;
		return 0;
	}

	return m_ref;
}

//////////////////////////////////////////////////////////////////////////////
//
//	CFilePathDataObject::QueryGetData()
//
//  PURPOSE: 
//

STDMETHODIMP CFilePathDataObject::QueryGetData(LPFORMATETC pformatetc)
{
	ULONG i;
	for(i = 0; i < cfMAX_SIZE; i++)
	{
		if( m_sdt[i].flags != 0 )
		{
			if( _IsEqualFORMATETC( &m_sdt[i].formatetc, pformatetc) )
			{
				return S_OK;
			}
		}
	}

#ifdef _DEBUG
	//CF_HDROP(15)
	//CF_UNICODETEXT(13)
	//CF_TEXT(1)
	WCHAR sz[260] = {0};
	GetClipboardFormatName(pformatetc->cfFormat,sz,260);
	_TRACE("cfFormat=%u (%S)\n",pformatetc->cfFormat,sz);
#endif

	return DV_E_FORMATETC;
}

HRESULT CFilePathDataObject::EnumFormatEtc(DWORD dwDirection,LPENUMFORMATETC *ppenumFormatEtc)
{
	HRESULT hr = E_NOTIMPL;
	int cFormatc;

	*ppenumFormatEtc = NULL;
	cFormatc = 0;

	if( dwDirection == DATADIR_GET )
	{
		CIEnumFORMATETC *pEnum = new CIEnumFORMATETC;

		FORMATETC formatetc[] ={ 0, NULL, DVASPECT_CONTENT, -1,TYMED_HGLOBAL };
		FORMATETC aFormatc[cfMAX_SIZE];

		if( m_sdt[cfHDROP].flags != 0 )
		{
			memcpy(&aFormatc[cFormatc],formatetc,sizeof(formatetc));
			aFormatc[cFormatc++].cfFormat = m_sdt[cfHDROP].cf;
		}
		if( m_sdt[cfLONGPATH].flags != 0 )
		{
			memcpy(&aFormatc[cFormatc],formatetc,sizeof(formatetc));
			aFormatc[cFormatc++].cfFormat = m_sdt[cfLONGPATH].cf;
		}
		if( m_sdt[cfIDLIST].flags != 0 )
		{
			memcpy(&aFormatc[cFormatc],formatetc,sizeof(formatetc));
			aFormatc[cFormatc++].cfFormat = m_sdt[cfIDLIST].cf;
		}

		pEnum->AddRef();
		pEnum->SetAvailableFormat( cFormatc, aFormatc );

		*ppenumFormatEtc = pEnum;

		if( *ppenumFormatEtc == NULL )
			hr = E_OUTOFMEMORY;
		else
			hr =  S_OK;
	}

	return hr;
}

HRESULT CFilePathDataObject::SetData(LPFORMATETC pformatetc,STGMEDIUM *pmedium,BOOL fRelease)
{
	// NOTE: Supports CF_HDROP and CFSTR_SHELLIDLIST only
	{
		HRESULT hr = E_FAIL;

		if( (pformatetc->cfFormat == CF_HDROP) &&
			(pformatetc->dwAspect == DVASPECT_CONTENT) &&
			(pformatetc->tymed == TYMED_HGLOBAL) )
		{
			ULONG n = cfHDROP;

			ASSERT(m_sdt[n].stgmedium.hGlobal == NULL);

			m_sdt[n].flags = 0x1;
			m_sdt[n].formatetc = *pformatetc;
			m_sdt[n].stgmedium = *pmedium;

			PVOID pOrg = GlobalLock( pmedium->hGlobal );
			if( pOrg != NULL )
			{
				SIZE_T cb = GlobalSize( pmedium->hGlobal );

				HGLOBAL hDup = GlobalAlloc(GHND,cb);
				if( hDup != NULL )
				{
					PVOID pDup;
					pDup = GlobalLock(hDup);
					CopyMemory(pDup,pOrg,cb);
					GlobalUnlock(hDup);

					m_sdt[n].stgmedium.hGlobal = hDup;
					hr = S_OK;
				}

				GlobalUnlock( pmedium->hGlobal );

				if( fRelease )
				{
					GlobalFree( pmedium->hGlobal );
				}
			}

			return hr;
		}

		if( (pformatetc->cfFormat == m_sdt[cfLONGPATH].cf) &&
			(pformatetc->dwAspect == m_sdt[cfLONGPATH].formatetc.dwAspect) &&
			(pformatetc->tymed == m_sdt[cfLONGPATH].formatetc.tymed) )
		{
			ULONG n = cfLONGPATH;

			ASSERT(m_sdt[n].stgmedium.hGlobal == NULL);

			m_sdt[n].flags = 0x1;
			m_sdt[n].formatetc = *pformatetc;
			m_sdt[n].stgmedium = *pmedium;

			PVOID pOrg = GlobalLock( pmedium->hGlobal );
			if( pOrg != NULL )
			{
				SIZE_T cb = GlobalSize( pmedium->hGlobal );

				HGLOBAL hDup = GlobalAlloc(GHND,cb);
				if( hDup != NULL )
				{
					PVOID pDup;
					pDup = GlobalLock(hDup);
					CopyMemory(pDup,pOrg,cb);
					GlobalUnlock(hDup);

					m_sdt[n].stgmedium.hGlobal = hDup;
					hr = S_OK;
				}

				GlobalUnlock( pmedium->hGlobal );

				if( fRelease )
				{
					GlobalFree( pmedium->hGlobal );
				}
			}

			return hr;
		}

		if( (pformatetc->cfFormat == m_sdt[cfIDLIST].cf) &&
			(pformatetc->dwAspect == m_sdt[cfIDLIST].formatetc.dwAspect) &&
			(pformatetc->tymed == m_sdt[cfIDLIST].formatetc.tymed) )
		{
			ULONG n = cfIDLIST;

			ASSERT(m_sdt[n].stgmedium.hGlobal == NULL);

			m_sdt[n].flags = 0x1;
			m_sdt[n].formatetc = *pformatetc;
			m_sdt[n].stgmedium = *pmedium;

			PVOID pOrg = GlobalLock( pmedium->hGlobal );
			if( pOrg != NULL )
			{
				SIZE_T cb = GlobalSize( pmedium->hGlobal );

				HGLOBAL hDup = GlobalAlloc(GHND,cb);
				if( hDup != NULL )
				{
					PVOID pDup;
					pDup = GlobalLock(hDup);
					CopyMemory(pDup,pOrg,cb);
					GlobalUnlock(hDup);

					m_sdt[n].stgmedium.hGlobal = hDup;
					hr = S_OK;
				}

				GlobalUnlock( pmedium->hGlobal );

				if( fRelease )
				{
					GlobalFree( pmedium->hGlobal );
				}
			}

			return hr;
		}
	}

	return DV_E_FORMATETC;
}

HRESULT CFilePathDataObject::GetData(LPFORMATETC pformatetc,LPSTGMEDIUM pmedium)
{
	pmedium->tymed = TYMED_NULL;
	pmedium->pUnkForRelease = NULL;
	pmedium->hGlobal = NULL;

	ULONG i;

	for(i = 0; i < cfMAX_SIZE; i++)
	{
		if( _IsEqualFORMATETC( &m_sdt[i].formatetc, pformatetc) )
		{
			pmedium->tymed          = m_sdt[i].stgmedium.tymed;
			pmedium->pUnkForRelease = m_sdt[i].stgmedium.pUnkForRelease;
			pmedium->hGlobal        = OleDuplicateData(m_sdt[i].stgmedium.hGlobal,m_sdt[i].formatetc.cfFormat,0);
			return S_OK;
		}
	}

	return DV_E_FORMATETC;
}

//////////////////////////////////////////////////////////////////////////////
//
//  File list managemant class
//

HRESULT CFileListCache::FreeFileList()
{
	int i;
	int n = m_FileList.GetSize();

	if( n == 0 )
		return S_FALSE;

	for( i = 0; i < n; i++)
	{
		_SafeMemFree( m_FileList[i]->Buffer );
		_SafeMemFree( m_FileList[i]->NtBuffer );
		_MemFree( m_FileList[i] );
	}

	m_FileList.RemoveAll();

	_SafeMemFree(m_pszNtPath);
	_SafeMemFree(m_pszDosPath);

	return S_OK;
}

HRESULT CFileListCache::SetBasePath(PCWSTR pszNtPath,PCWSTR pszDosPath)
{
	if( pszNtPath )
		m_pszNtPath = _MemAllocString(pszNtPath);

	if( pszDosPath )
		m_pszDosPath = _MemAllocString(pszDosPath);

	return S_OK;
}

HRESULT CFileListCache::AddFileName(PCWSTR Filename)
{
	FILENAME_STRING *p = (FILENAME_STRING *)_MemAllocZero(sizeof(FILENAME_STRING));
	p->FStrType = FSTR_TYPE_FILENAME;
	p->Length = (ULONG)wcslen(Filename);
	p->Buffer = _MemAllocString((PWSTR)Filename);
	m_FileList.Add( p );
	return S_OK;
}

HRESULT CFileListCache::AddFullPathFileName(PCWSTR FullPath)
{
	FILENAME_STRING *p = (FILENAME_STRING *)_MemAllocZero(sizeof(FILENAME_STRING));
	p->FStrType = FSTR_TYPE_FULLPATH;
	p->Length = (ULONG)wcslen(FullPath);
	p->Buffer = _MemAllocString((PWSTR)FullPath);
	m_FileList.Add( p );
	return S_OK;
}

HRESULT CFileListCache::AddFullPathFileNameEx(PCWSTR DosFullPath,PCWSTR NtFullPath)
{
	FILENAME_STRING *p = (FILENAME_STRING *)_MemAllocZero(sizeof(FILENAME_STRING));
	p->FStrType = FSTR_TYPE_FULLPATH;

	p->Length = (ULONG)wcslen(DosFullPath);
	p->Buffer = _MemAllocString((PWSTR)DosFullPath);

	p->NtLength = (ULONG)wcslen(NtFullPath);
	p->NtBuffer = _MemAllocString((PWSTR)NtFullPath);

	m_FileList.Add( p );
	return S_OK;
}

HRESULT CFileListCache::Complete(UINT MakeFlags)
{
	//
	// Create drop data for CF_HDROP.
	//
	if( MakeFlags & MAKE_HGLOBAL_HDROP )
		MakeHDROPFileList();

	//
	// Create drop data for ITEMIDLIST.
	//
	if( MakeFlags & MAKE_HGLOBAL_IDLIST )
		MakeIDLFileList();

	//
	// Create drop data for ITEMIDLIST(Relative).
	//
	if( MakeFlags & MAKE_HGLOBAL_IDLIST_RELATIVE )
		MakeIDLFileListRelativePath();

	//
	// Create drop data for LongPath.
	//
	if( MakeFlags & MAKE_HGLOBAL_LONGPATH )
		MakeLongPathFileList();
	else if( MakeFlags & MAKE_HGLOBAL_LONGPATH_RELATIVE ) // exclusive with MAKE_HGLOBAL_LONGPATH.
		MakeLongPathRelativeFileList();

	return S_OK;
}

//----------------------------------------------------------------------------
//
//  MakeHDROPFileList()
//
//----------------------------------------------------------------------------
HRESULT CFileListCache::MakeHDROPFileList()
{
	HRESULT hr = S_OK;

	DROPFILES *p;
	ULONG cbBufferLength = 0;
	int i,cFileCount;

	cFileCount = m_FileList.GetSize();

	if( cFileCount == 0 )
		return S_FALSE;

	int cchDosPath = 0;
	if( m_pszDosPath != NULL )
		cchDosPath = (int)wcslen(m_pszDosPath) + 1; // length with separator '\'.

	for(i = 0; i < cFileCount; i++)
	{
		if( m_FileList[i]->FStrType == FSTR_TYPE_FULLPATH )
			cbBufferLength += (((ULONG)wcslen(m_FileList[i]->Buffer) + 1) * sizeof(WCHAR));
		else
			cbBufferLength += (((ULONG)cchDosPath + (ULONG)wcslen(m_FileList[i]->Buffer) + 1) * sizeof(WCHAR));
	}

	cbBufferLength += sizeof(WCHAR);

	if( m_hGlobalHDROP != NULL )
		GlobalFree((HGLOBAL)m_hGlobalHDROP);

	m_hGlobalHDROP = GlobalAlloc(GHND,sizeof(DROPFILES) + cbBufferLength);
	if( m_hGlobalHDROP == NULL )
	{
		return E_OUTOFMEMORY;
	}

	p = (DROPFILES *)GlobalLock(m_hGlobalHDROP);

	GetCursorPos(&p->pt);
	p->fNC = TRUE;
	p->fWide = TRUE;

	//
	// Set the pointer to a position in the file name list.
	//
	PWSTR psz;
	p->pFiles = sizeof(DROPFILES);
	psz = (LPWSTR)(((LPBYTE)p) + sizeof(DROPFILES));

	for(i = 0; i < cFileCount; i++)
	{
		//
		// For drives, if the full path length exceeds MAX_PATH-1, it will not be stored.
		//
		if( m_FileList[i]->FStrType == FSTR_TYPE_FULLPATH )
		{
#if 0
			PWSTR pszPath = m_FileList[i]->Buffer;
			if( FsPathGetPathType(pszPath) == FSPT_DEVICE )
			{
				CNtPathBuffer DosPath;
				if( FsNtPathNameToDosPathName(m_FileList[i]->Buffer,DosPath,DosPath.GetBufferLength()) )
				{
					if( wcslen(DosPath) < MAX_PATH )
						wcscat(psz,DosPath.c_str());
					else
						continue;
				}
				else
				{
					continue;
				}
			}
			else
			{
				if( wcslen(pszPath) < MAX_PATH )
					wcscat(psz,pszPath);
				else
					continue;
			}
#else
			PWSTR pszNtPath  = m_FileList[i]->NtBuffer;
			PWSTR pszDosPath = m_FileList[i]->Buffer;
			if( pszNtPath != NULL && pszDosPath == NULL )
			{
				continue;
			}
			else
			{
				if( wcslen(pszDosPath) < MAX_PATH )
					wcscat(psz,pszDosPath);
				else
					continue;
			}
#endif
		}
		else if( m_FileList[i]->FStrType == FSTR_TYPE_FILENAME )
		{
			//
			// Relative Path
			//
			if( m_pszDosPath == NULL )
				continue;
			wcscat(psz,m_pszDosPath);
			wcscat(psz,L"\\");
			wcscat(psz,m_FileList[i]->Buffer);
		}		
		else
		{
			; // unrecognized type
		}

		psz += (wcslen(psz) + 1);
	}

	// double null terminate
	*psz = L'\0';

	GlobalUnlock(m_hGlobalHDROP);

	return hr;
}

//----------------------------------------------------------------------------
//
//  MakeLongPathFileList()
//
//  NOTE:
//  A proprietary format for store NT device path.
//
//----------------------------------------------------------------------------
HRESULT CFileListCache::MakeLongPathFileList()
{
	HRESULT hr = E_FAIL;
	int i,cFileNameListCount;

	cFileNameListCount = m_FileList.GetSize();

	if( cFileNameListCount == 0 )
		return S_FALSE;

	CLongFilePathClipboard lfpc;

	WCHAR *ntSrcPath = new WCHAR[_NT_PATH_FULL_LENGTH];

	if( m_pszNtPath )
	{
		wcscpy(ntSrcPath,m_pszNtPath);
	}
	else
	{
		*ntSrcPath = L'\0';
	}

	if( cFileNameListCount > 0 )
	{
		BOOL bIsFullPath = ntSrcPath[0] == 0 ? TRUE : FALSE;

		lfpc.Start( bIsFullPath ? NULL : ntSrcPath );

		for(i = 0; i < cFileNameListCount; i++)
		{
			if( bIsFullPath )
				lfpc.AddFullPath( m_FileList[i]->NtBuffer );
			else
				lfpc.AddFile( PathFindFileName(m_FileList[i]->Buffer) );
		}

		if( lfpc.Commit(NULL,TRUE) )
		{
			m_hGlobalLongPath = lfpc.GetHandle();
			hr = S_OK;
		}
	}

	delete[] ntSrcPath;

	return hr;
}

//----------------------------------------------------------------------------
//
//  MakeLongPathRelativeFileList()
//
//  NOTE:
//  A proprietary format for store NT device path, accept relative path.
//
//----------------------------------------------------------------------------
HRESULT CFileListCache::MakeLongPathRelativeFileList()
{
	HRESULT hr = E_FAIL;
	int i,cFileNameListCount;

	cFileNameListCount = m_FileList.GetSize();

	if( cFileNameListCount == 0 )
		return S_FALSE;

	CLongFilePathClipboard lfpc;

	WCHAR *ntSrcPath = new WCHAR[_NT_PATH_FULL_LENGTH];

	if( m_pszNtPath )
	{
		wcscpy(ntSrcPath,m_pszNtPath);

		if( cFileNameListCount > 0 )
		{
			BOOL bIsFullPath = (ntSrcPath[0] == 0 ? TRUE : FALSE);

			lfpc.Start( bIsFullPath ? NULL : ntSrcPath );

			for(i = 0; i < cFileNameListCount; i++)
			{
				if( bIsFullPath )
					lfpc.AddFullPath( m_FileList[i]->Buffer );
				else
					lfpc.AddFile( PathFindFileName(m_FileList[i]->Buffer) );
			}

			if( lfpc.Commit(NULL,TRUE) )
			{
				m_hGlobalLongPath = lfpc.GetHandle();
				hr = S_OK;
			}
		}
	}

	delete[] ntSrcPath;

	return hr;
}

HRESULT CFileListCache::MakeIDLFileList()
{
	ASSERT( m_hGlobalIDL == NULL );

	// CIDA Structure
	// An array of offsets, relative to the beginning of this structure. The array contains
	// cidl+1 elements. The first element of aoffset contains an offset to the fully-qualified 
	// PIDL of a parent folder. If this PIDL is empty, the parent folder is the desktop. 
	// Each of the remaining elements of the array contains an offset to one of the PIDLs 
	// to be transferred. All of these PIDLs are relative to the PIDL of the parent folder. 

	HRESULT hr = E_FAIL;
	LPITEMIDLIST *pIdlArray;
	ULONG cbBufferLength = 0;
	int i,cFileListSize,cItemCount;

	__try 
	{
		cFileListSize = m_FileList.GetSize();

		cItemCount = cFileListSize + 1;  // +1, for the Parent directory

		pIdlArray = new LPITEMIDLIST[ cItemCount ];

		if( pIdlArray == NULL )
			__leave;

		ZeroMemory(pIdlArray,sizeof(LPITEMIDLIST) * cItemCount);

		//
		// Create IDL for the source directory path.
		//
		WCHAR szParentDirPath[MAX_PATH];

		if( m_pszDosPath == NULL || *m_pszDosPath == L'\0' )
		{
			// Desktop root
			PIDLIST_ABSOLUTE pidlDesktop = NULL;
			if( SHGetSpecialFolderLocation(NULL,CSIDL_DESKTOP,&pidlDesktop) != S_OK )
				__leave;

			pIdlArray[0] = ILClone(pidlDesktop);

			CoTaskMemFree(pidlDesktop);
		}
		else
		{
			// Specified root path
			StringCchCopy(szParentDirPath,MAX_PATH,m_pszDosPath);
			pIdlArray[0] = ILCreateFromPathW( m_pszDosPath );
		}

		//
		// cItemValid is the number of valid PIDLs (including parent paths) 
		// that could be stored in pIdlArray.
		//
		int cItemValid = 1;

		//
		// Add filename
		//
		LPITEMIDLIST pNameItem;
		WCHAR szFilePath[MAX_PATH];
		for(i = 0; i < cFileListSize; i++)
		{
			if( m_FileList[i]->FStrType == FSTR_TYPE_FILENAME )
			{
				if( PathCombine(szFilePath,szParentDirPath,m_FileList[i]->Buffer) != NULL )
				{
					pNameItem = ILCreateFromPathW( szFilePath );
					if( pNameItem )
					{
						pIdlArray[cItemValid++] = ILClone( ILFindLastID(pNameItem) );
						ILFree(pNameItem);
					}
				}
			}
			else if( m_FileList[i]->FStrType == FSTR_TYPE_FULLPATH )
			{
				pIdlArray[cItemValid++] = ILCreateFromPathW( m_FileList[i]->Buffer );
			}
		}

		//
		// If no files are added and the list only contains parent directory paths,
		// the process will be canceled.
		//
		if( cItemValid == 1 )
		{
			__leave;
		}

		//
		// We calculate memory size of required.
		//
		for(i = 0; i < cItemValid; i++)
		{
			cbBufferLength += ILGetSize( pIdlArray[i] );
		}

		//
		// Allocate memory
		// CIDA
		//   UINT cidl;        // number of relative IDList
		//   UINT aoffset[];   // [0]: folder IDList, [1]-[cidl]: item IDList
		//   IDL idl[];
		//
		m_hGlobalIDL = GlobalAlloc(GHND,sizeof(CIDA) + ((cItemValid-1) * sizeof(UINT)) + cbBufferLength);
		if(m_hGlobalIDL == NULL)
		{
			__leave;
		}

		CIDA *pcida;
		pcida = (CIDA *)GlobalLock(m_hGlobalIDL);

		// CAUTION!!
		// CIDA.cidl is Number of PIDLs that are being transferred,
		// NOT counting the parent folder.
		pcida->cidl = cItemValid - 1;

		//
		// Append to end of the memory buffer that variable length IDL data, 
		// location to pointed by the aoffset[n]. 
		//
		// ex) cItemCount==3
		//
		// |<-------------------------------cbBufferLength-------------------------------->|
		// |<------------------ CIDA ------------------->|
	 	// | cidl | aoffset[0] | aoffset[1] | aoffset[2] | Data    | Data      | Data      |
		//              |            |           |       ^         ^           ^
		//              +--------------------------------+         |           |
		//                           |           |                 |           |
		//                           +-----------------------------+           |
		//                                       |                             |
		//                                       +-----------------------------+
		//
		LPBYTE pData = (LPBYTE)(&pcida->aoffset[cItemValid]);

		UINT_PTR cbSize;

		for(i = 0; i < cItemValid; i++)
		{
			pNameItem = pIdlArray[i];
			cbSize = ILGetSize(pNameItem);
			CopyMemory(pData,pNameItem,cbSize);
			pcida->aoffset[i] = (UINT)((UINT_PTR)pData - (UINT_PTR)pcida);
			pData += cbSize; 
		}

		GlobalUnlock(m_hGlobalIDL);

		hr = S_OK;
	}
	__finally
	{
		if( pIdlArray )
		{
			int i;
			for( i = 0; i < cItemCount; i++ )
			{
				ILFree( pIdlArray[i] );
			}
			delete[] pIdlArray;
		}
	}

	return hr;
}

HRESULT CFileListCache::MakeIDLFileListRelativePath()
{
	ASSERT( m_hGlobalIDL == NULL );

	//
	// If the DOS path is not set, no processing.
	//
	if( m_pszDosPath == NULL || *m_pszDosPath == L'\0' )
		return S_FALSE; 

	// CIDA Structure
	// An array of offsets, relative to the beginning of this structure. The array contains
	// cidl+1 elements. The first element of aoffset contains an offset to the fully-qualified 
	// PIDL of a parent folder. If this PIDL is empty, the parent folder is the desktop. 
	// Each of the remaining elements of the array contains an offset to one of the PIDLs 
	// to be transferred. All of these PIDLs are relative to the PIDL of the parent folder. 

	HRESULT hr = E_FAIL;
	LPITEMIDLIST *pIdlArray;
	ULONG cbBufferLength = 0;
	int i,cFileListSize,cItemCount;

	__try 
	{
		cFileListSize = m_FileList.GetSize();

		cItemCount = cFileListSize + 1;  // increment count one for parent directory.

		pIdlArray = new LPITEMIDLIST[ cItemCount ];

		if( pIdlArray == NULL )
			__leave;

		ZeroMemory(pIdlArray,sizeof(LPITEMIDLIST) * cItemCount);

		//
		// Create IDL for copy source directory.
		//
		WCHAR szParentDirPath[MAX_PATH];
		StringCchCopy(szParentDirPath,MAX_PATH,m_pszDosPath);
		pIdlArray[0] = ILCreateFromPathW( m_pszDosPath );

		//
		// cItemValid is the number of valid PIDLs (including parent paths) 
		// that could be stored in pIdlArray.
		//
		int cItemValid = 0;
		if( !ILIsEmpty( pIdlArray[cItemValid] ) )
			cItemValid++;
		else
			__leave;

		//
		// The ITEMIDLIST is handled up to MAX_PATH, so the buffer length set be MAX_PATH.
		//		
		WCHAR szFilePath[MAX_PATH];
		for(i = 0; i < cFileListSize; i++)
		{
			// TODO:
			// The FStrType should be using a relative path, but as this is
			// not currently supported, FSTR_TYPE_FULLPATH is used.
			//
			// The processing root path indicated by m_pszDosPath is compared
			// with the full path of each file to obtain a relative IDTEMIDLIST,
			// which is then used as CIDA data.
			//
			if( m_FileList[i]->FStrType == FSTR_TYPE_FULLPATH )
			{
				if( StringCchCopy(szFilePath,MAX_PATH,m_FileList[i]->Buffer) == S_OK )
				{
					LPITEMIDLIST pidlFilePath;
					pidlFilePath = ILCreateFromPathW(szFilePath);
					if( pidlFilePath )
					{
						// Returns a pointer to the child's simple ITEMIDLIST structure
						// if pidlChild is a child of pidlParent. The returned structure
						// consists of pidlChild, minus the SHITEMID structures that 
						// make up pidlParent. 
						// Returns NULL if pidlChild is not a child of pidlParent.
						//
						// The returned pointer is a pointer into the existing parent structure. 
						// It is an alias for pidlChild. No new memory is allocated in association 
						// with the returned pointer. It is not the caller's responsibility to free
						// the returned value.
						PUIDLIST_RELATIVE pidlRelative;
						pidlRelative = ILFindChild(pIdlArray[0],pidlFilePath);

						pIdlArray[cItemValid++] = ILClone( pidlRelative );

						ILFree(pidlFilePath);
					}
				}
			}
		}

		//
		// If no files are added and the list only contains parent directory paths, 
		// the process will be canceled.
		//
		if( cItemValid == 1 )
		{
			__leave;
		}

		//
		// We calculate memory size of required.
		//
		for(i = 0; i < cItemValid; i++)
		{
			cbBufferLength += ILGetSize( pIdlArray[i] );
		}

		//
		// Allocate memory
		// CIDA
		//   UINT cidl;        // number of relative IDList
		//   UINT aoffset[];   // [0]: folder IDList, [1]-[cidl]: item IDList
		//   IDL idl[];
		//
		m_hGlobalIDL = GlobalAlloc(GHND,sizeof(CIDA) + ((cItemValid-1) * sizeof(UINT)) + cbBufferLength);
		if(m_hGlobalIDL == NULL)
		{
			__leave;
		}

		CIDA *pcida;
		pcida = (CIDA *)GlobalLock(m_hGlobalIDL);

		// CAUTION!!
		// CIDA.cidl is Number of PIDLs that are being transferred,
		// NOT counting the parent folder.
		pcida->cidl = cItemValid - 1;

		//
		// Append to end of the memory buffer that variable length IDL data, 
		// location to pointed by the aoffset[n]. 
		//
		// ex) cItemCount==3
		//
		// |<-------------------------------cbBufferLength-------------------------------->|
		// |<------------------ CIDA ------------------->|
	 	// | cidl | aoffset[0] | aoffset[1] | aoffset[2] | Data    | Data      | Data      |
		//              |            |           |       ^         ^           ^
		//              +--------------------------------+         |           |
		//                           |           |                 |           |
		//                           +-----------------------------+           |
		//                                       |                             |
		//                                       +-----------------------------+
		//
		LPBYTE pData = (LPBYTE)(&pcida->aoffset[cItemValid]);

		UINT_PTR cbSize;
		LPITEMIDLIST pNameItem;

		for(i = 0; i < cItemValid; i++)
		{
			pNameItem = pIdlArray[i];
			cbSize = ILGetSize(pNameItem);
			CopyMemory(pData,pNameItem,cbSize);
			pcida->aoffset[i] = (UINT)((UINT_PTR)pData - (UINT_PTR)pcida);
			pData += cbSize; 
		}

		GlobalUnlock(m_hGlobalIDL);

		hr = S_OK;
	}
	__finally
	{
		if( pIdlArray )
		{
			int i;
			for( i = 0; i < cItemCount; i++ )
			{
				ILFree( pIdlArray[i] );
			}
			delete[] pIdlArray;
		}
	}

	return hr;
}
