#pragma once

__forceinline LPARAM TreeView_GetItemData(HWND hwndTV,HTREEITEM hItem) {
    TVITEMEX tviex = {0};
    tviex.mask = TVIF_PARAM;
    tviex.hItem = hItem;
    if( TreeView_GetItem(hwndTV,&tviex) )
        return tviex.lParam;
    return 0;
}

__forceinline BOOL TreeView_GetItemText(HWND hwndTV,HTREEITEM hItem,PTSTR pszText,int cchTextMax) {
    TVITEMEX tviex = {0};
    tviex.mask = TVIF_TEXT;
    tviex.hItem = hItem;
    tviex.cchTextMax = cchTextMax;
    tviex.pszText = pszText;
    return TreeView_GetItem(hwndTV,&tviex);
}

__forceinline BOOL TreeView_SetChildren(HWND hwndTV,HTREEITEM hItem,int cChildren) {
    TVITEMEX tviex = {0};
    tviex.mask = TVIF_CHILDREN;
    tviex.hItem = hItem;
    tviex.cChildren = cChildren;
    return TreeView_SetItem(hwndTV,&tviex);
}

inline LPARAM ListViewEx_GetItemData(HWND hwndLV,int iItem)
{
    LVITEM lvi = {0};
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    ListView_GetItem(hwndLV,&lvi);
    return lvi.lParam;
}

inline BOOL ListViewEx_SetItemData(HWND hwndLV,int iItem,LPARAM lParam)
{
    LVITEM lvi = {0};
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    lvi.lParam = lParam;
    return ListView_SetItem(hwndLV,&lvi);
}

inline BOOL ListViewEx_SetHeaderItemData(HWND hwndLV,int index, LPARAM lParam)
{
    HWND hwndHD = ListView_GetHeader(hwndLV);
    HDITEM hdi = {0};
    hdi.mask = HDI_LPARAM;
    hdi.lParam = lParam;
    return Header_SetItem(hwndHD,index,&hdi);
}

inline LPARAM ListViewEx_GetHeaderItemData(HWND hwndLV,int index)
{
    HWND hwndHD = ListView_GetHeader(hwndLV);
    HDITEM hdi = {0};
    hdi.mask = HDI_LPARAM;
    Header_GetItem(hwndHD,index,&hdi);
    return hdi.lParam;
}

inline int ListViewEx_GetColumnCount(HWND hwndLV)
{
    return Header_GetItemCount(ListView_GetHeader(hwndLV));
}

inline int ListViewEx_GetSubItem(HWND hwndLV,int iColumn)
{
    LVCOLUMN lvi = {0};
    lvi.mask = LVCF_SUBITEM;
    ListView_GetColumn(hwndLV,iColumn,&lvi);
    return lvi.iSubItem;
}

inline void ListViewEx_SetHeaderArrow(HWND hwndLV,int iSubItem,int iDirection)
{
    HWND h = ListView_GetHeader(hwndLV);

    HDITEM hdi = {0};
    DWORD f;
    hdi.mask = HDI_FORMAT;
    SendMessage(h,HDM_GETITEM,iSubItem,(LPARAM)&hdi);

    if( iDirection < 0 )
        f = HDF_SORTDOWN;
    else if( iDirection > 0 )
        f = HDF_SORTUP;
    else
        f = 0;

    hdi.fmt &= ~(HDF_SORTDOWN|HDF_SORTUP);
    hdi.fmt |= f;
    SendMessage(h,HDM_SETITEM,iSubItem,(LPARAM)&hdi);
}

inline int ListViewEx_HitTest(HWND hwndLV,POINT pt, UINT* pFlags)
{
    LVHITTESTINFO hti = {};
    hti.pt = pt;
    int nRes = (int)SendMessage(hwndLV, LVM_HITTEST, (WPARAM)-1, (LPARAM)&hti);
    if (pFlags != NULL)
        *pFlags = hti.flags;
    return nRes;
}

inline int ListViewEx_SubItemHitTest(HWND hwndLV,POINT pt, UINT* pFlags)
{
    LVHITTESTINFO hti = {};
    hti.pt = pt;
    int nRes = (int)SendMessage(hwndLV, LVM_SUBITEMHITTEST, (WPARAM)-1, (LPARAM)&hti);
    if (pFlags != NULL)
        *pFlags = hti.flags;
    return hti.iSubItem;
}

inline BOOL ListViewEx_ClearSelectAll(HWND hwndLV,BOOL bClearFocus=FALSE)
{
    LVITEM lvi = {};
    lvi.stateMask = (LVIS_SELECTED|(bClearFocus ? LVNI_FOCUSED : 0));
    lvi.state = 0;
    return (BOOL)SendMessage(hwndLV,LVM_SETITEMSTATE,(WPARAM)(-1),(LPARAM)&lvi);
}

inline BOOL ListViewEx_AdjustWidthAllColumns(HWND hwndLV,int cx)
{
    HWND hwndHD = ListView_GetHeader(hwndLV);
    if( hwndHD == NULL )
        return FALSE;
    int cHeaders = Header_GetItemCount(hwndHD);
    for(int i = 0; i < cHeaders; i++)
    {
        ListView_SetColumnWidth(hwndLV,i,cx);
    }
    return TRUE;
}

inline VOID ListViewEx_SetTrickColumnZero(HWND hwndLV,BOOL bStart)
{
    // note
    // insert and delete dummy column for column drag and drop layout change.
    // please refer to win32 document.
    if( bStart )
    {
        LVCOLUMN lvc = {0};
        lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_ORDER;
        ListView_InsertColumn(hwndLV,0,&lvc); // insert dummy column
    }
    else
    {
        ListView_DeleteColumn(hwndLV,0); // delete dummy column
    }
}

inline BOOL ListViewEx_InsertColumnText(HWND hwndLV,int iCol,PCWSTR text,int fmt)
{
    LVCOLUMN col = {0};
    col.mask = LVCF_FMT|LVCF_TEXT;
    col.pszText = (LPWSTR)text;
    col.fmt = fmt;
    return (BOOL)SendMessage(hwndLV,LVM_INSERTCOLUMN,iCol,(LPARAM)&col);
}

inline int ListViewEx_GetCurSel(HWND hwndLV)
{
    return (int)ListView_GetNextItem(hwndLV,-1,LVNI_SELECTED|LVNI_FOCUSED);
}

#ifndef LVM_RESETEMPTYTEXT
#define LVM_RESETEMPTYTEXT (LVM_FIRST + 84)
#endif

inline BOOL ListViewEx_ResetEmptyText(HWND hwndLV)
{
    return (BOOL)SendMessage(hwndLV,LVM_RESETEMPTYTEXT,0,0);
}

inline void ListViewEx_InvalidateListItem(HWND hWndList,int iItem)
{
    int i,cColumns;
    cColumns = ListViewEx_GetColumnCount(hWndList);
    for(i = 0; i < cColumns; i++)
        ListView_SetItemText(hWndList,iItem,i,LPSTR_TEXTCALLBACK);

    LVITEM lvi={0};
    lvi.mask      = LVIF_TEXT|LVIF_IMAGE|LVIF_STATE;
    lvi.iItem     = iItem;
    lvi.iImage    = I_IMAGECALLBACK;
    lvi.state     = 0;
    lvi.stateMask = LVIS_OVERLAYMASK;
    lvi.pszText   = LPSTR_TEXTCALLBACK;
    ListView_SetItem(hWndList,&lvi);
}

inline void ListViewEx_InvalidateListColumn(HWND hWndList,int iColumn,BOOL bImages=FALSE,BOOL bRedraw=TRUE)
{
    int i,cItems;
    cItems =  ListView_GetItemCount(hWndList);
    for(i = 0; i < cItems; i++)
    {
        ListView_SetItemText(hWndList,i,iColumn,LPSTR_TEXTCALLBACK);
        if( iColumn == 0 )
        {
            LVITEM lvi={0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.pszText = LPSTR_TEXTCALLBACK;
            if( bImages )
            {
                lvi.mask |= LVIF_IMAGE|LVIF_STATE;
                lvi.iImage = I_IMAGECALLBACK;
                lvi.state = 0;
                lvi.stateMask = LVIS_OVERLAYMASK;
            }
            ListView_SetItem(hWndList,&lvi);
        }
    }
    if( bRedraw )
        ListView_RedrawItems(hWndList,0,cItems-1);
}

inline int ListViewEx_InsertString(HWND hwndLV,int iItem,PCWSTR String)
{
    LVITEM item = {0};
    item.mask = LVIF_TEXT;
	item.iItem = iItem;
    item.pszText = (LPWSTR)String;
    return (int)SendMessage(hwndLV,LVM_INSERTITEM,0,(LPARAM)&item);
}

inline int ListViewEx_InsertStringParam(HWND hwndLV,int iItem,PCWSTR String,LPARAM lParam)
{
    LVITEM item = {0};
    item.mask = LVIF_TEXT|LVIF_PARAM;
	item.iItem = iItem;
    item.pszText = (LPWSTR)String;
	item.lParam = lParam;
    return (int)SendMessage(hwndLV,LVM_INSERTITEM,0,(LPARAM)&item);
}

typedef struct _LVSORTHOSTPARAM
{
    PVOID pFunc;
    int iColumn;
    int iDirection;
} LVSORTHOSTPARAM;

template <class T>
class CListViewCtrlSortHost
{
protected:
    int m_iSortColumn;
    int m_iSortDirection;

public:
    CListViewCtrlSortHost()
    {
        m_iSortColumn = 0;
        m_iSortDirection = 0;
    }

    static int CALLBACK _CompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
    {
        LVSORTHOSTPARAM *p = (LVSORTHOSTPARAM *)lParamSort;

        T* po = (T*)p->pFunc;

        return po->T::CompareProc(lParam1,lParam2,p);
    }
/*
    BOOL SortItemsByIndex(HWND hwndLV,int iColumn)
    {
        LVSORTHOSTPARAM param = { static_cast<T*>(this), iColumn, m_iSortDirection };
        return ListView_SortItemsEx(hwndLV,&_CompareProc,(LPARAM)&param);
    }
*/
    BOOL SortItemsByParam(HWND hwndLV,int iColumn)
    {
        LVSORTHOSTPARAM param = { static_cast<T*>(this), iColumn, m_iSortDirection };
        return ListView_SortItems(hwndLV,&_CompareProc,(LPARAM)&param);
    }

    void DoSort(HWND hwndLV,int iColumn,BOOL bChangeDirection=TRUE)
    {
        // Clear current mark
        //
        ListViewEx_SetHeaderArrow(hwndLV,m_iSortColumn,0);

        // If change the select column, to reset sort direction.
        //
        if( bChangeDirection )
        {
            if( m_iSortColumn != iColumn ) 
                m_iSortDirection = 0;
            else
                m_iSortDirection = !m_iSortDirection;
        }

        // Set current sort mark
        //
        ListViewEx_SetHeaderArrow(hwndLV,iColumn,m_iSortDirection == 0 ? 1 : -1);

        // Do sort
        //
        SortItemsByParam( hwndLV, iColumn );

        // Save current sort index
        //
        m_iSortColumn = iColumn;
    }
};

class CWaitCursor
{
    HCURSOR m_hCursor;
public:
    CWaitCursor()
    {
        m_hCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));
    }

    ~CWaitCursor()
    {
        Resume();
    }

    VOID Resume()
    {
        SetCursor(m_hCursor);
    }
};
