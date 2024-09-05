#pragma once
//
// Simple Array Class
//
template<class T>
class CValArray
{
	int m_cCount;
	ULONG m_cbBufferSize;
	ULONG m_cbGrowSize;
	T* m_pBuffer;

	T* _Alloc(SIZE_T cb)
	{
		return (T*)_MemAlloc(cb);
	}

	T* _Realloc(T *p,SIZE_T cb)
	{
		return (T*)_MemReAlloc(p,cb);
	}

	VOID _Free(T *val)
	{
		_MemFree(val);
	}

	T* _ReallocBuffer(T* val)
	{
		m_cbBufferSize += m_cbGrowSize;
		return _Realloc(val,m_cbBufferSize);
	}

public:
	CValArray(ULONG cGrowCount=1)
	{
		m_cCount = 0;
		m_cbGrowSize = cGrowCount * sizeof(T);
		m_cbBufferSize = 1 * sizeof(T);
		m_pBuffer = _Alloc(m_cbBufferSize);
	}

	CValArray(ULONG cCount,ULONG cGrowCount=1)
	{
		m_cCount = 0;
		m_cbGrowSize = cGrowCount * sizeof(T);
		m_cbBufferSize = cCount * sizeof(T);
		m_pBuffer = _Alloc(m_cbBufferSize);
	}

	~CValArray()
	{
		if( m_pBuffer )
		{
			_Free(m_pBuffer);
			m_pBuffer = NULL;
		}
	}

	int GetCount() const
	{
		return m_cCount;
	}

	int GetSize() const
	{
		return GetCount();
	}

    T& operator[](int index) const
    {
		return m_pBuffer[index];
    }

	int Add(T val)
	{
		if( (m_cCount * sizeof(T)) >= m_cbBufferSize )
		{
			T* pBuffer = _ReallocBuffer(m_pBuffer);
			if( pBuffer == NULL )
				return -1;
			m_pBuffer = pBuffer;
		}

		m_pBuffer[ m_cCount++ ] = val;

		return (m_cCount-1);
	}

	int Delete(int i)
	{
        if( i >= m_cCount )
			return -1;
        if( 0 == m_cCount )
			return -1;

		memmove(&m_pBuffer[i],&m_pBuffer[i+1],((m_cCount-i-1)*sizeof(T)));
		m_cCount--;

		m_cbBufferSize -= sizeof(T);
		m_pBuffer = _Realloc(m_pBuffer,m_cbBufferSize);

		return i;
	}

	int DeleteAll()
	{
		m_cbBufferSize = 0;
		m_cCount = 0;
		_Free( m_pBuffer );
		m_pBuffer = NULL;
		return 0;
	}

	int RemoveAll()
	{
		return DeleteAll();
	}

	T* GetBuffer() const
	{
		return m_pBuffer;
	}

	ULONG GetBufferSize() const
	{
		return m_cbBuffer;
	}
};

template <class T>
class CSimpleValArray : public CValArray<T> { };
