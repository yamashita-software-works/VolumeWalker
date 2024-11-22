#pragma once 
#include "stdwnd.h"
#include "array.h"

#define TABS_BOTTOM  0x0000
#define TABS_TOP     0x0001
#define TABS_MASK    0x000F

typedef struct _VIEWTABITEM
{
	RECT     rc;
	LPCTSTR  pszText;
	UINT     nCmdID;
} VIEWTABITEM,*PVIEWTABITEM;

class CViewTab : public CStdWnd
{
	static LRESULT CALLBACK WndProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam);
	LRESULT WindowProc(HWND hWnd,UINT uMessage,WPARAM wParam,LPARAM lParam);

	CArray<VIEWTABITEM*> m_tab;

	int   m_cyViewTab;
	int   m_SelectIndex;
	ULONG m_SidePos;

	HWND  m_hwndCmdOwner;

	HFONT m_hFont;
	HBRUSH m_hbrBackColor;

	COLORREF m_crFace;
	COLORREF m_crSelectText;
	COLORREF m_crBack;
	COLORREF m_crText;
	COLORREF m_crShadowBorder;
	COLORREF m_crHilightBorder;
	COLORREF m_crTabShadow;

public:
	CViewTab();
	~CViewTab();

	HWND Create(HWND hwndOwner,HINSTANCE hInst,UINT nID,DWORD dwStyle);
	int  InsertItem(int index,UINT nCmdID,LPCTSTR pszText);
	void RemoveAll();

	void SetSelectItem(int index);
	int  GetSelectItem(void);
	int  CommandIdToIndex(UINT nCmdID);
	void SetColorScheme(COLORREF crFace,COLORREF crBack,
					COLORREF crSelectText,COLORREF crText,
					COLORREF crHilightBorder,COLORREF crShadowBorder,COLORREF crTabShadow);

	inline void SetCmdOwner(HWND hwnd) { m_hwndCmdOwner = hwnd; }
	inline UINT GetCmdID(int index) { return m_tab.GetAt(index)->nCmdID; }
	inline void SetHeight(int cy) { m_cyViewTab = cy; }
	inline int GetHeight(void) { return m_cyViewTab; }
	inline void SetFont(HFONT hFont) {
		if( m_hFont ) DeleteObject( m_hFont );
		m_hFont = (HFONT)hFont;
	}

	int GetWidth();
	int GetItemCount();
	int GetTabRect(int index,RECT *prc);
	int GetTabWidth(int index);
	int GetTabText(int index,LPTSTR pszText,int cchText);

protected:
	void DrawSelectTab(HDC hdc,int index);
	void DrawNormalTab(HDC hdc,int index);
	void DrawTabText(HDC hdc,LPCTSTR pszText,RECT& rc,BOOL bSelect);
	void CalcHeight();
	void UpdateColor();
	void FreeColor();
	void InitDefaultColor();

protected:
	LRESULT OnCreate(HWND hWnd,WPARAM wParam,LPARAM lParam);
	LRESULT OnLButtonDown(HWND hWnd,WPARAM wParam,LPARAM lParam);
	LRESULT OnPaint(HWND hWnd,WPARAM wParam,LPARAM lParam);
	LRESULT OnSize(HWND hWnd,WPARAM wParam,LPARAM lParam);
};
