#pragma once
//****************************************************************************
//
//  history.h
//
//  Simple cyclic history list
//
//  Author: YAMASHITA Katsuhiro
//
//  History: 2015.05.13 Created
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//

// e.g.) When back button clicked twice from "<End".
//
//             +-----+ 
// UpperBound >|     |
//             +-----+
//             |     |
//             +-----+
//             |     |< End
//             +-----+
//             |     |
//             +-----+
//             |     |< Cur
//             +-----+
//             |     |
//             +-----+
// LowerBound >|     |< Top
//             +-----+
//
// e.g.) When wrap-around the queue.
//
//             +-----+ 
// UpperBound >|     |
//             +-----+
//             |     |< Top
//             +-----+
//             |     |< Cur < End
//             +-----+
//             |     |
//             +-----+
//             |     |
//             +-----+
//             |     |
//             +-----+
// LowerBound >|     |
//             +-----+
//
typedef struct _HISTORY_ITEM
{
	USHORT Type;
	USHORT State;
	PWSTR pszPath;
	PWSTR pszFileName;
} HISTORY_ITEM;

#define _MAX_HISTORY_ITEM  128

class CHistoryManager 
{
	HISTORY_ITEM *m_pHistoryList;
	HISTORY_ITEM *m_pUpperBound;
	HISTORY_ITEM *m_pLowerBound;
	HISTORY_ITEM *m_pTop;
	HISTORY_ITEM *m_pEnd;
	HISTORY_ITEM *m_pCurrent;

	int m_cMaxHistoryItem;
	int m_cItemCount;

	BOOL m_bWrap;

public:
	CHistoryManager()
	{
		m_pHistoryList = NULL;
		m_cMaxHistoryItem = _MAX_HISTORY_ITEM;
		m_pUpperBound = NULL;
		m_pLowerBound = NULL;
		m_pTop = NULL;
		m_pEnd = NULL;
		m_pCurrent = NULL;
	}

	~CHistoryManager()
	{
		if( m_pHistoryList )
		{
			int i;
			for(i = 0; i < m_cMaxHistoryItem; i++)
			{
				_SafeMemFree(m_pHistoryList[i].pszPath);
				_SafeMemFree(m_pHistoryList[i].pszFileName);
			}
			_SafeMemFree( m_pHistoryList );
		}
	}

	BOOL Initialize(ULONG cHistorySize=_MAX_HISTORY_ITEM)
	{
		m_cMaxHistoryItem = cHistorySize;

		m_pHistoryList = (HISTORY_ITEM *)_MemAllocZero( sizeof(HISTORY_ITEM) * m_cMaxHistoryItem );
		m_pLowerBound = &m_pHistoryList[0];
		m_pUpperBound = &m_pHistoryList[ m_cMaxHistoryItem-1 ];

		m_pTop = m_pLowerBound;
		m_pCurrent = m_pEnd = NULL;
		m_cItemCount = 0;

		m_bWrap = FALSE;

		SetLastError(ERROR_SUCCESS);

		return TRUE;
	}

	ULONG GetHistorySize()
	{
		return m_cMaxHistoryItem;
	}

	HISTORY_ITEM *CurrentPtr()
	{
		return m_pCurrent;
	}

	HISTORY_ITEM *SetCurrentPtr(HISTORY_ITEM *pCur)
	{
		HISTORY_ITEM *pPrev = m_pCurrent;
		m_pCurrent = pCur;
		return pPrev;
	}

	HISTORY_ITEM *ForwardPtr(HISTORY_ITEM *ptr)
	{
		ptr++;
		if( ptr > m_pUpperBound )
			ptr = m_pLowerBound;
		return ptr;
	}

	HISTORY_ITEM *BackwardPtr(HISTORY_ITEM *ptr)
	{
		ptr--;
		if( ptr < m_pLowerBound )
			ptr = m_pUpperBound;
		return ptr;
	}

	HISTORY_ITEM *Wraparound(HISTORY_ITEM *pItem)
	{
		m_cItemCount--;
		m_pTop = ForwardPtr(m_pTop);
		pItem = ForwardPtr(pItem);
		return pItem;
	}

	BOOL Push( PCWSTR pszPath, PCWSTR pszFileName = NULL)
	{
		if( m_pCurrent == NULL )
		{
			m_pCurrent = m_pTop;
		}
		else
		{
			if( m_pCurrent == m_pUpperBound )
				m_bWrap = TRUE;

			if( m_bWrap )
			{
				m_pCurrent = Wraparound(m_pCurrent);
			}
			else
			{
				m_pCurrent = ForwardPtr(m_pCurrent);
			}
		}

		m_pEnd = m_pCurrent;

		_SafeMemFree(m_pCurrent->pszPath);
		_SafeMemFree(m_pCurrent->pszFileName);

		m_pCurrent->Type = 1;
		m_pCurrent->State = 0;
		m_pCurrent->pszPath = _MemAllocString( pszPath );
		if( pszFileName != NULL )
			m_pCurrent->pszFileName = _MemAllocString( pszFileName );
		m_cItemCount++;

		SetLastError(ERROR_SUCCESS);

		return TRUE;
	}

	BOOL SetBackSelectToDirectory(PCWSTR pszDirectory)
	{
		if( m_pCurrent == NULL )
			return FALSE;
		_SafeMemFree(m_pCurrent->pszFileName);
		m_pCurrent->pszFileName = _MemAllocString( pszDirectory );
		return (m_pCurrent->pszFileName != NULL);
	}

	BOOL Back(HISTORY_ITEM *pItem)
	{
		DWORD dwResult = ERROR_SUCCESS;
		if( m_pCurrent == m_pTop )
		{
			dwResult = ERROR_NO_MORE_ITEMS;;
		}
		else
		{
			m_pCurrent = BackwardPtr(m_pCurrent);
		}

		*pItem = *m_pCurrent;

		SetLastError(dwResult);

		return (dwResult == ERROR_SUCCESS);
	}

	BOOL Forward(HISTORY_ITEM *pItem)
	{
		DWORD dwResult = ERROR_SUCCESS;
		if( m_pCurrent == m_pEnd )
		{
			dwResult = ERROR_NO_MORE_ITEMS;
		}
		else
		{
			m_pCurrent = ForwardPtr(m_pCurrent);
		}

		*pItem = *m_pCurrent;

		SetLastError(dwResult);

		return (dwResult == ERROR_SUCCESS);
	}

	BOOL IsEnableBack() { return !IsEmpty() && ( m_pCurrent != m_pTop ); }
	BOOL IsEnableForward() { return !IsEmpty() && ( m_pCurrent != m_pEnd ); }
	BOOL IsEmpty() { return m_cItemCount == 0 ? TRUE : FALSE; }
	BOOL IsHistoryNone() { return m_cItemCount <= 1 ? TRUE : FALSE; }
	int GetItemCount() { return m_cItemCount; }
};
