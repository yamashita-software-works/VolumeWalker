#ifndef __STRINGBUFFER__
#define __STRINGBUFFER__
/*++

  CStringBuffer -- C/C++ string buffer operation helper

  2000.10.26 Create

  YAMASHITA Katsuhiro

--*/

#ifndef ASSERT
#define ASSERT(a)
#endif

class CStringBuffer
{
	TCHAR *m_str;
	int    m_cch;

	HINSTANCE m_hResourceModule;

#ifdef _MBCS
	WCHAR  *m_wstr;
#endif

public:
	void Init()
	{
		m_str = NULL;
		m_cch  = 0;
#ifdef _MBCS
		m_wstr = NULL;
#endif
		m_hResourceModule = NULL;
	}

	void Free()
	{
		if( m_str )
		{
			delete[] m_str;
			m_str = NULL;
#ifdef _MBCS
			delete[] m_wstr;
			m_wstr = NULL;
#endif
		}
		m_cch = 0;
	}

	CStringBuffer()
	{
		Init();
	}

	CStringBuffer(int id)
	{
		Init();
		Load(id);
	}

	CStringBuffer(LPTSTR psz)
	{
		Init();
		Copy(psz);
	}

	~CStringBuffer()
	{
		Free();
	}

	int Load(int id)
	{
		Free();
		m_cch  = 1024; // default size
		m_str = new TCHAR[m_cch];
		return LoadString(m_hResourceModule,id,m_str,m_cch);
	}

	int Load(int id,HINSTANCE hInst)
	{
		Free();
		m_cch  = 1024; // default size
		m_str = new TCHAR[m_cch];
		return LoadString(hInst,id,m_str,m_cch);
	}

	int	Copy(PTSTR psz)
	{
		Free();
		m_cch = lstrlen(psz) + 1;
		m_str = new TCHAR[m_cch];
		lstrcpy(m_str,psz);
		return m_cch;
	}

	void Alloc(int cch)
	{
		m_cch = cch;
		m_str = new TCHAR[m_cch];
		memset(m_str,0,m_cch * sizeof(TCHAR));
	}

	int	SetText(PTSTR psz)
	{
		return Copy(psz); 
	}

	operator LPCTSTR() const
	{
		return m_str;
	}

	operator LPTSTR() const
	{
		return m_str;
	}

#ifdef _MBCS
	operator LPCWSTR()
	{
		ASSERT( m_str != NULL );
		if( m_wstr == NULL )
		{
			m_wstr = new WCHAR[ m_cch ];
			MultiByteToWideChar(CP_OEMCP,0,m_str,-1,m_wstr,m_cch);
		}
		return m_wstr;
	}
	operator LPWSTR()
	{
		ASSERT( m_str != NULL );
		if( m_wstr == NULL )
		{
			m_wstr = new WCHAR[ m_cch ];
			MultiByteToWideChar(CP_OEMCP,0,m_str,-1,m_wstr,m_cch);
		}
		return m_wstr;
	}
#endif

	TCHAR operator[](int n) const
	{
		return m_str[n];
	}

	const LPCTSTR operator=(LPCTSTR p)
	{
		return m_str;
	}

	const LPTSTR operator&()
	{
		return m_str;
	}

	// CStringBuffer = CStringBuffer;
	const CStringBuffer& operator=(CStringBuffer& str)
	{
		Copy(str);
		return *this;
	}

	inline int Length()
	{
		return (int)_tcslen(m_str);
	}

	inline int BufferSize()
	{
		return m_cch * sizeof(TCHAR);
	}

//++050313
	inline int BufferLength()
	{
		return m_cch;
	}
//--050313

	inline LPTSTR Tail()
	{
		return &m_str[ Length() ];
	}

	inline LPTSTR CharFind(TCHAR ch)
	{
		return _tcschr(m_str,ch);
	}

	inline LPTSTR ReplaceChar(TCHAR chFrom,TCHAR chTo)
	{
		PTSTR p;
		p = _tcschr(m_str,chFrom);
		if( p != NULL )
			*p = chTo;
		return p;
	}
	inline LPTSTR Append(LPTSTR psz)
	{
		int cch;
		cch = (int)_tcslen(psz) + m_cch + 1;

		TCHAR *p = new TCHAR[cch]; 

		ASSERT(p != NULL);
		if( p == NULL )
			return m_str;

		_tcscpy(p,m_str);
		_tcscat(p,psz);

		delete[] m_str;

		m_str = p;
		m_cch = cch; 

		return m_str;
	}
};

#if 0
//
// Multibyte String Helper Class
//

class CMultiByteString
{
	PSTR m_psz;
public:
	CMultiByteString()
	{
		m_psz = NULL;
	}

	CMultiByteString(int id)
	{
		m_psz = new CHAR[256];
		LoadStringA(m_hResourceModule, id, m_psz, 256 );
	}

	CMultiByteString(PWSTR pszWideChar)
	{
		int cb = WideCharToMultiByte(CP_OEMCP,0,pszWideChar,-1,0,0,NULL,NULL);
		m_psz = new CHAR[cb];
		if( m_psz == NULL )
			throw E_OUTOFMEMORY;
		WideCharToMultiByte(CP_OEMCP,0,pszWideChar,-1,m_psz,cb,NULL,NULL);
	}

	~CMultiByteString()
	{
		if( m_psz != NULL )
			delete[] m_psz;
	}

	char *GetString()
	{
		return m_psz;
	}

	operator char*()
	{
		return m_psz;
	}

	char* operator()(void)const
	{
		return m_psz;
	}
};
#endif

//
// Formated string helper
//

class CStringFormat
{
	PTSTR m_psz;
public:
	CStringFormat()
	{
		m_psz = new TCHAR[1024]; // wsprintfÇ™1KBÇ‹Ç≈ÇÃêßå¿Ç™Ç†ÇËÅAÇªÇÍÇÃñºécÅB
	}
	~CStringFormat()
	{
		delete[] m_psz;
	}

	int Format( PTSTR Format, ...  )
	{
		va_list args;
		va_start(args, Format);

		int iRet;
		ASSERT(m_psz != NULL);

		iRet = _vsntprintf(m_psz, 1024, Format, args);

		va_end(args);
		return iRet;
	}

	PTSTR GetString() const
	{
		return m_psz;
	}

	operator LPCTSTR() const
	{
		return m_psz;
	}

	operator LPTSTR() const
	{
		return m_psz;
	}
};

#endif