#pragma once

template<class T,int N>
class CSimpleStack
{
	int m_pos;
	int m_max;
	T *m_stack;
public:
	CSimpleStack(bool ErrorAtThrow=false)
	{
		m_pos = 0;
		m_max = N;
		m_stack = new T[N];
		if( m_stack == NULL )
			if( ErrorAtThrow )
				throw (HRESULT)E_OUTOFMEMORY;
			else
				m_max = 0;
	}

	~CSimpleStack()
	{	
		if( m_stack )
			delete[] m_stack;
	}

	T Push(T a)
	{
		if( m_pos < m_max )
		{
			m_stack[m_pos++] = a;
			return a;
		}
		return (T)-1;
	}

	T Pop()
	{
		if( m_pos > 0 )
			return m_stack[--m_pos];
		return (T)-1;
	}

	int GetPos() const
	{
		return m_pos;
	}

	T GetCurrent() const
	{
		if( m_pos > 0 )
			return m_stack[m_pos-1];
		return -1;
	}

	int GetSize()
	{
		return m_pos;
	}
};
