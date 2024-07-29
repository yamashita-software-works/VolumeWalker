#pragma once

class CMultiSz
{
	PTSTR m_pszTop;
	int   m_BufferSize;

public:
	CMultiSz()
	{
		m_pszTop = NULL;
		m_BufferSize = 0;
	}

	CMultiSz(CMultiSz& msz)
	{
		m_pszTop = (LPTSTR)msz.GetTop();
		int cchBuffer = GetLength();
		m_pszTop = (LPTSTR)_MemAlloc( cchBuffer * sizeof(TCHAR) );
		memcpy(m_pszTop,msz.GetTop(),cchBuffer * sizeof(TCHAR));
		m_BufferSize = cchBuffer;
	}

	CMultiSz(LPCTSTR psz)
	{
		m_pszTop = (LPTSTR)psz;
		int cchBuffer = GetLength();
		m_pszTop = (LPTSTR)_MemAlloc( cchBuffer * sizeof(TCHAR) );
		memcpy(m_pszTop,psz,cchBuffer * sizeof(TCHAR));
		m_BufferSize = cchBuffer;
	}

	CMultiSz& operator =(CMultiSz& msz)
	{
		m_pszTop = (LPTSTR)msz.GetTop();
		int cchBuffer = GetLength();
		m_pszTop = (LPTSTR)_MemAlloc( cchBuffer * sizeof(TCHAR) );
		memcpy(m_pszTop,msz.GetTop(),cchBuffer * sizeof(TCHAR));
		m_BufferSize = cchBuffer;
		return *this;
	}

	~CMultiSz()
	{
		Free();
	}

	void Free()
	{
		if( m_pszTop )
		{
			_MemFree( m_pszTop );
			m_pszTop = NULL;
			m_BufferSize = 0;
		}
	}

	PTSTR Add(PTSTR psz)
	{
		int cch;
		if( psz )
			cch = (int)_tcslen(psz) + 1;
		else
			cch = 1; // only null

		PTSTR pszTemp;
		pszTemp = (PTSTR)_MemReAlloc(m_pszTop, (m_BufferSize + cch + 1) * sizeof(TCHAR));
		if( pszTemp == NULL )
			return m_pszTop;

		m_pszTop = pszTemp;

		if( psz )
			memcpy(&m_pszTop[m_BufferSize],psz,cch*sizeof(TCHAR));

		m_BufferSize += cch;

		m_pszTop[m_BufferSize] = 0;

		return m_pszTop;
	}

	int GetLength()
	{
		LPTSTR psz = m_pszTop;
		int cch = 0;
		int cchMax = 0;
		do
		{
			cch = ((int)_tcslen(psz) + 1);
			cchMax += cch;
			psz += cch;
		}
		while( *psz );
		cchMax++; // multi term null
		return cchMax;
	}

	LPCTSTR GetTop()
	{
		return m_pszTop;
	}

	BOOL Next(LPCTSTR* ppsz)
	{
		*ppsz += (_tcslen(*ppsz) + 1);
		return **ppsz != _T('\0') ? TRUE : FALSE;
	}

	PCTSTR GetNext(PCTSTR psz)
	{
		PCTSTR p = psz + _tcslen(psz);
		if( *p == _T('\0') && *(p + 1) == _T('\0') )
			return (p + 1); // point to multi string teminate null.
		if( *p == _T('\0') )
			return (p + 1); // point to next string top.
		return NULL;        // not possible.
	}

	operator LPCTSTR() const
	{
		return m_pszTop;
	}

	ULONG GetBufferSize()
	{
		return (ULONG)((m_BufferSize+1)*sizeof(TCHAR));
	}

	BOOL IsEmpty()
	{
		if( m_pszTop == NULL )
			return TRUE;
		if( *m_pszTop == _T('\0') )
			return TRUE;
		return FALSE;
	}
};
