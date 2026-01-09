#pragma once

typedef struct _SUPPORT_DATA_TYPE
{
	CLIPFORMAT cf;
	ULONG flags;
	FORMATETC formatetc;
	STGMEDIUM stgmedium;
} SUPPORT_DATA_TYPE;

enum {
	cfHDROP = 0,
	cfLONGPATH,
	cfIDLIST,
	cfFILECONTENTS,
	cfFILEDESCRIPTOR,
	cfMAX_SIZE,
};

class CFilePathDataObject : public IDataObject
{
	int m_ref;
	SUPPORT_DATA_TYPE m_sdt[cfMAX_SIZE];

public:
	CFilePathDataObject();
	~CFilePathDataObject();

public:
	STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppvObj);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

	STDMETHODIMP DAdvise( FORMATETC FAR* pFormatetc, DWORD advf,
						  LPADVISESINK pAdvSink, DWORD FAR* pdwConnection )
	{
		return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
	}

	STDMETHODIMP DUnadvise( DWORD dwConnection )
	{
		return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
	}

	STDMETHODIMP EnumDAdvise( LPENUMSTATDATA FAR* ppenumAdvise )
	{
		return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
	}

	STDMETHODIMP GetCanonicalFormatEtc( LPFORMATETC pformatetc,LPFORMATETC pformatetcOut )
	{
		pformatetcOut->ptd = NULL;
		return ResultFromScode(E_NOTIMPL);
	}

	STDMETHODIMP GetDataHere(LPFORMATETC pformatetc,LPSTGMEDIUM ppmeduim)
	{
		SCODE sc = E_NOTIMPL;
		return ResultFromScode(sc);
	}

	STDMETHODIMP QueryGetData(LPFORMATETC pformatetc );
	STDMETHODIMP EnumFormatEtc( DWORD dwDirection,LPENUMFORMATETC FAR* ppenumFormatEtc);
	STDMETHODIMP GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium );
	STDMETHODIMP SetData(LPFORMATETC pformatetc,STGMEDIUM *pmedium,BOOL fRelease);
};

//
// File list cache manager class
//

#define MAKE_HGLOBAL_HDROP              0x00000001
#define MAKE_HGLOBAL_IDLIST             0x00000002
#define MAKE_HGLOBAL_LONGPATH           0x00000004
#define MAKE_HGLOBAL_IDLIST_RELATIVE    0x00000008
#define MAKE_HGLOBAL_ALL                0x0000000F
#define MAKE_HGLOBAL_LONGPATH_RELATIVE  0x00000010

#define FSTR_TYPE_FILENAME         0x0
#define FSTR_TYPE_FULLPATH         0x2

class CFileListCache
{
public:
	typedef struct _FILENAME_STRING {
		ULONG FStrType;
		ULONG Length;
		PWSTR Buffer;
		ULONG NtLength;
		PWSTR NtBuffer;
	} FILENAME_STRING;

private:
	CValArray<FILENAME_STRING*> m_FileList;
	HGLOBAL m_hGlobalHDROP;
	HGLOBAL m_hGlobalLongPath;
	HGLOBAL m_hGlobalIDL;
	HRESULT MakeHDROPFileList();
	HRESULT MakeIDLFileList();
	HRESULT MakeIDLFileListRelativePath();
	HRESULT MakeLongPathFileList();
	HRESULT MakeLongPathRelativeFileList();

	PWSTR m_pszNtPath;
	PWSTR m_pszDosPath;

public:
	CFileListCache()
	{
		m_hGlobalHDROP = NULL;
		m_hGlobalLongPath = NULL;
		m_hGlobalIDL = NULL;
		m_pszNtPath = NULL;
		m_pszDosPath = NULL;
	}

	~CFileListCache()
	{
		FreeFileList();

		if( m_hGlobalHDROP )
		{
			GlobalFree( m_hGlobalHDROP );
		}
		if( m_hGlobalLongPath )
		{
			GlobalFree( m_hGlobalLongPath );
		}
	}

	HRESULT SetBasePath(PCWSTR pszNtPath,PCWSTR pszDosPath);
	HRESULT AddFileName(PCWSTR Filename);
	HRESULT AddFullPathFileName(PCWSTR FullPath);
	HRESULT AddFullPathFileNameEx(PCWSTR NtFullPath,PCWSTR DosFullPath);
	HRESULT Complete(UINT MakeFlags);
	HRESULT FreeFileList();
	LONG GetItemCount() { return m_FileList.GetSize(); }

	HGLOBAL GetHDROP() { return m_hGlobalHDROP; }
	HGLOBAL GetLongPath() { return m_hGlobalLongPath; }
	HGLOBAL GetPIDL() { return m_hGlobalIDL; }
};
