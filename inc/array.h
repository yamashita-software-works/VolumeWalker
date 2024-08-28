//
// CArray template
//
// NOTE:
// Legacy Code - created in 2001.
// This header is will be obsolete. Use only for backward compatibility.
//
#ifndef __T_ARRAY__
#define __T_ARRAY__

#ifdef _COMMONLIB
#include "mem.h"
#define realloc  _MemReAlloc
#else
	;
#endif

typedef int (__cdecl *PCOMPAREPROC)(const void *elem1, const void *elem2);

template <class T> class CArray
{
	int m_nCount;
	T*  m_element;
public:
	CArray()
	{
		m_element = 0;
		m_nCount = 0;
	}

	~CArray()
	{
		Free();
	}

	int GetCount()
	{
		return m_nCount;
	}

	T GetAt(int i)
	{
		return m_element[i];
	}

	int Add(T p)
	{
		if( (m_element = (T*)realloc(m_element,sizeof(T) * (m_nCount + 1))) != 0 )
		{
			m_element[m_nCount] = p;
			m_nCount++;
			return (m_nCount-1);
		} 
		return -1;
	}

	BOOL Insert(int i,T p)
	{
		if( (m_element = (T*)realloc(m_element,sizeof(T) * (m_nCount + 1))) == 0 )
		{
			DebugBreak();
			return FALSE;
		}

		int cb = (sizeof(T) * (m_nCount + 1));
		MoveMemory(&m_element[i+1],&m_element[i],cb);
		m_element[i] = p;
		return TRUE;
	}

	BOOL Delete(int i)
	{
		if( m_nCount <= 0 || m_nCount <= i )
		{
			ASSERT(FALSE);
			return FALSE;
		}
		int cb = (sizeof(T) * (m_nCount - i - 1));
		MoveMemory(&m_element[i],&m_element[i+1],cb);
		m_nCount--;
		ASSERT(	m_nCount >= 0 );
		if( m_nCount > 0 )
			// If element count is above zero then Shrink memory.
//			realloc(m_element,sizeof(T) * m_nCount);
			// Windows XP heap manager: Possible to the memory block may moving.
			m_element = (T*)realloc(m_element,sizeof(T) * m_nCount);
		return TRUE;
	}

	VOID Free()
	{
		if( m_element != NULL )
		{
			free(m_element);
			m_element = NULL;		
			m_nCount = 0;
		}
	}

	VOID Release()
	{
		delete this;
	}

	BOOL Find(T t)
	{
		int c = GetCount();		
		for(int i = 0; i < c; i++)
		{
			if( m_element[i] == t)
				return TRUE;
		}
		return FALSE;
	}

	void Sort(PCOMPAREPROC compare)
	{
		qsort(m_element,m_nCount,sizeof(T),compare);
	}

	T operator[](int i) const
	{
		ASSERT(FALSE);
		return m_element[i];
	}

	T& operator[](int i)
	{
		return m_element[i];
	}
};

typedef CArray<PTSTR> CPSTRArray;
typedef CArray<ULONG> CULONGArray;

#endif
