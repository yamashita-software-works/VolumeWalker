#pragma once

template<class T>
class PtrArray
{
	HDPA    m_hdpa;
	HANDLE  m_hHeap;

public:
	PtrArray()
	{
		m_hdpa = NULL;
		m_hHeap = NULL;
	}

	~PtrArray()
	{
		if(m_hdpa != NULL)
		{
			Destroy();
		}
	}

	BOOL Create(int cpGrow=8)
	{
		m_hdpa = DPA_CreateEx(cpGrow,m_hHeap);
		return m_hdpa ? TRUE : FALSE;
	}

	BOOL Destroy()
	{
		if( DPA_Destroy(m_hdpa) )
		{
			m_hdpa = NULL;
			return TRUE;
		}
		return FALSE;
	}

	INT GetCount()
	{
		return DPA_GetPtrCount(m_hdpa);
	}

	T** GetData()
	{
		return (T**)DPA_GetPtrPtr(m_hdpa);
	}

	INT Add(T* pitem)
	{
		return DPA_AppendPtr(m_hdpa,pitem);
	}

	INT Insert(INT iIndex,T* ptr)
	{
		return DPA_InsertPtr(m_hdpa,iIndex,ptr);
	}

	T* Get(INT iIndex)
	{
		return (T*)DPA_GetPtr(m_hdpa,iIndex);
	}

	INT GetIndex(const T *ptr)
	{
		return DPA_GetPtrIndex(m_hdpa,ptr);
	}

	BOOL DeleteAll()
	{
		return DPA_DeleteAllPtrs(m_hdpa);
	}

	T* Delete(int iIndex)
	{
		return (T *)DPA_DeletePtr(m_hdpa);
    }
};
