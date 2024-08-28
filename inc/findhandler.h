#pragma once
//
// Common Find Handler Helper for ListView
//
template <class T>
class CFindHandler
{
	int m_iStartFindItem;
	int m_iFirstMatchItem;
	int m_iLastMatchItem;
	BOOL m_bSimpleScan;

protected:
	CFindHandler()
	{
		m_iStartFindItem = 0;
		m_iFirstMatchItem = 0;
		m_iLastMatchItem = 0;
		m_bSimpleScan = FALSE;
	}

	LRESULT OnFindItem(HWND,UINT,WPARAM wParam,LPARAM lParam)
	{
		T *p = static_cast<T*>(this);
		HWND hWndList = p->GetListView();

		LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;
		switch( LOWORD(wParam) )
		{
			case FIND_QUERYOPENDIALOG:
				return 0; // 0:accept 1:prevent

			case FIND_CLOSEDIALOG:
				m_iStartFindItem = -1;
				break;

			case FIND_SEARCH:
				m_iStartFindItem = ListViewEx_GetCurSel(hWndList);
				if( m_iStartFindItem == -1 )
					m_iStartFindItem = 0;
				SearchItem(m_iStartFindItem,lpfr->lpstrFindWhat,
						(BOOL) (lpfr->Flags & FR_DOWN), 
						(BOOL) (lpfr->Flags & FR_MATCHCASE)); 
				break;
			case FIND_SEARCH_NEXT:
			{
				int cItems = ListView_GetItemCount(hWndList);
				m_iStartFindItem = ListViewEx_GetCurSel(hWndList);
				if( m_iStartFindItem == -1 )
					m_iStartFindItem = 0;
				else
				{
					if( m_bSimpleScan )
					{
						m_iStartFindItem = m_iStartFindItem + ((lpfr->Flags & FR_DOWN) ? 1 : -1);

						if( m_iStartFindItem <= 0 )
							m_iStartFindItem = cItems - 1;
						else if( m_iStartFindItem >= cItems )
							m_iStartFindItem = 0;
					}
					else
					{
						m_iStartFindItem = ListView_GetNextItem(hWndList,m_iStartFindItem,
							LVNI_VISIBLEORDER|((lpfr->Flags & FR_DOWN) ? 0 : LVNI_PREVIOUS) );

						if( m_iStartFindItem == -1 )
						{
							if( !(lpfr->Flags & FR_DOWN) )
							{
								m_iStartFindItem = GetLastVisibleItem(hWndList);
								if( m_iStartFindItem == -1 )
									m_iStartFindItem = cItems - 1;
							}
							else if( lpfr->Flags & FR_DOWN )
							{
								m_iStartFindItem = GetFirstVisibleItem(hWndList);
							}
						}
					}
				}

				SearchItem(m_iStartFindItem,lpfr->lpstrFindWhat,
						(BOOL) (lpfr->Flags & FR_DOWN), 
						(BOOL) (lpfr->Flags & FR_MATCHCASE)); 

				break;
			}
		}
		return 0;
	}
private:
	int SearchItem(int iFindStartItem,PWSTR pszFindText,BOOL Down,BOOL MatchCase)
	{
		T *p = static_cast<T*>(this);
		HWND hWndList = p->GetListView();

		int iItem,col,cItems,cColumns;

		const int cchText = MAX_PATH;
		WCHAR szText[cchText];

		cItems = ListView_GetItemCount(hWndList);
		cColumns = ListViewEx_GetColumnCount(hWndList);

		iItem = iFindStartItem;

		for(;;)
		{
			for(col = 0; col < cColumns; col++)
			{
				ListView_GetItemText(hWndList,iItem,col,szText,cchText);

				if( StrStrI(szText,pszFindText) != 0 )
				{
					ListViewEx_ClearSelectAll(hWndList,TRUE);
					ListView_SetItemState(hWndList,iItem,LVNI_SELECTED|LVNI_FOCUSED,LVNI_SELECTED|LVNI_FOCUSED);
					ListView_EnsureVisible(hWndList,iItem,FALSE);
					goto __found;
				}
			}

			if( m_bSimpleScan )
			{
				Down ? iItem++ : iItem--;
				// lap around
				if( iItem >= cItems )
					iItem = 0;
				else if( iItem < 0 )
					iItem = cItems - 1;
			}
			else
			{
				iItem = ListView_GetNextItem(hWndList,iItem,
						LVNI_VISIBLEORDER|(Down ? 0 : LVNI_PREVIOUS) );
				if( Down && iItem == -1 )
				{
					iItem = GetFirstVisibleItem(hWndList);
				}
				else if( !Down && iItem == -1 )
				{
					iItem = GetLastVisibleItem(hWndList);
					if( iItem == -1 )
						iItem = cItems - 1;
				}
			}

			if( iItem == iFindStartItem )
			{
				MessageBeep(MB_ICONSTOP);
				iItem = -1;
				break; // not found
			}
		}

 __found:
		return iItem;
	}

	int GetLastVisibleItem(HWND hWndList)
	{
		if( m_bSimpleScan )
			return (ListView_GetItemCount(hWndList) - 1); // can also be -1
		int iLastItem = 0,iItem = 0;
		for(;;)
		{
			iItem = ListView_GetNextItem(hWndList,iLastItem,LVNI_VISIBLEORDER);
			if( iItem == -1 )
			{
				return iLastItem;
			}
			iLastItem = iItem;
		}
		return -1;
	}

	int GetFirstVisibleItem(HWND hWndList)
	{
		if( m_bSimpleScan )
			return 0;
		return ListView_GetNextItem(hWndList,-1,LVNI_VISIBLEORDER);
	}
};
