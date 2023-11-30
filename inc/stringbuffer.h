#pragma once

//
// Simple String Buffer
//
template <typename T>
class CStringBufferT
{
	T *m_pStr;
	int m_cch;
public:
	CStringBufferT(int cch) throw(...)
	{
		m_pStr = (T*)_MemAllocZero(  (cch + 1) * sizeof(T) ); // alloc with terminate null
		if( m_pStr == NULL )
			throw ERROR_NOT_ENOUGH_MEMORY;
		m_cch = cch; // terminate null not include
	}

	~CStringBufferT()
	{
		if( m_pStr )
			_MemFree( m_pStr );
	}

	T *BufferPointer() const
	{
		return m_pStr;
	}

	T *c_str() const
	{
		return m_pStr;
	}

	T *csz() const
	{
		return m_pStr;
	}

	BOOL IsEmpty() const
	{
		return (m_pStr == NULL || *m_pStr == 0);
	}

	int GetBufferSize() const
	{
		return m_cch;
	}

	int GetBufferSizeByte() const
	{
		return (m_cch * sizeof(T));
	}

	int GetLength() const
	{
		return (int)_getlen(m_pStr);
	}

	void Empty()
	{
		if( m_pStr )
		{
			memset(m_pStr,0,GetBufferSizeByte());
		}
	}

	operator LPCTSTR() const
	{
		return m_pStr;
	}

	operator LPTSTR() const 
	{
		return m_pStr;
	}

	CStringBufferT<T>& operator =(const T* str) throw(...)
	{
		_scopy(m_pStr,str);
		return *this;
	}

	CStringBufferT<T>& operator +=(const T* str) throw(...)
	{
		_scat(m_pStr,str);
		return *this;
	}

	CStringBufferT<T>& operator +=(T* str) throw(...)
	{
		_scat(m_pStr,str);
		return *this;
	}

	const T *CopyCbText( const T *pszText, int cbText )
	{
		if( m_pStr == NULL )
			return NULL;
		int cb = min(cbText,(int)(m_cch/sizeof(T)));
		memcpy(m_pStr,pszText,cb);
		m_pStr[(cb/sizeof(T))] = 0;
		return m_pStr;
	}

	const T *CopyCchText( const T *pszText, int cchText )
	{
		if( m_pStr == NULL )
			return NULL;
		int cch = min(cchText,m_cch);
		memcpy(m_pStr,pszText,cch*sizeof(T));
		m_pStr[cch] = 0;
		return m_pStr;
	}

private:
	int _getlen(const T *psz) const
	{
		int cch = 0;
		while( *psz++ )
			cch++;
		return cch;
	}

	T *_scopy(T *pdst,const T *psrc)
	{
		if( m_cch > 0 )
		{
			int cch = 0;
			while( *psrc )
			{
				*pdst++ = *psrc++;
				cch++;
				if( cch == m_cch )
					break;
			}
		}
		return pdst;
	}

	T *_scat(T *pdst,const T *psz)
	{
		int cch1 = _getlen(pdst);
		int cch2 = _getlen(psz);
		if( (cch1 + cch2) <= m_cch )
		{
			_scopy(&pdst[cch1],psz);
		}
		return pdst;
	}
};

typedef CStringBufferT<TCHAR>  CStringBuffer;
typedef CStringBufferT<WCHAR>  CStringBufferW;
typedef CStringBufferT<CHAR>   CStringBufferA;

