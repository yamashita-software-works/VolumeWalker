#pragma once

class CSimpleSplitWindow : public CBaseWindow
{
public:
	BOOL m_bCapture;
	INT  m_xSplitPos;
	double m_ratio;

	virtual VOID UpdateLayout(int cx,int cy,BOOL absPos=FALSE)
	{
	}

	CSimpleSplitWindow()
	{
		m_bCapture = FALSE;
		m_xSplitPos = 0;
		double m_ratio = 0.0;
	}

	VOID SetSplitPos(int pos)
	{
		m_xSplitPos = pos;

		RECT rc;
		GetClientRect(m_hWnd,&rc);
		m_ratio = ((double)m_xSplitPos / (double)_RECT_WIDTH(rc));
	}

	int GetSplitPos() const
	{
		return m_xSplitPos;
	}

	BOOL IsOverSplitter(int x) const
	{
		return (((m_xSplitPos-4) <= x) && (x <= (m_xSplitPos+4)));
	}

	VOID SetPosAndCheckLimit(int x)
	{
#define SPLIT_LIMIT 32
		RECT rc;
		GetClientRect(m_hWnd,&rc);

		if( x < SPLIT_LIMIT ) 
			m_xSplitPos = SPLIT_LIMIT;
		else if( x > (_RECT_WIDTH(rc) - SPLIT_LIMIT) )
			m_xSplitPos = (_RECT_WIDTH(rc) - SPLIT_LIMIT);
		else
			m_xSplitPos = x;
	}

	LRESULT OnMouseMove(HWND hWnd,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		if( !m_bCapture )
		{
			if( IsOverSplitter(x)  )
			{
				SetCursor( LoadCursor(NULL,MAKEINTRESOURCE(IDC_SIZEWE)) );
			}
		}
		else
		{
			// moving splitter
			SetPosAndCheckLimit(x);

			RECT rc;
			GetClientRect(hWnd,&rc);
			UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),TRUE);
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
		}
		return 0;
	}

	LRESULT OnLButtonDown(HWND hWnd,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		if( IsOverSplitter(x)  )
		{
			// start move splitter
			SetCursor( LoadCursor(NULL,MAKEINTRESOURCE(IDC_SIZEWE)) );

			m_xSplitPos = x;

			m_bCapture = TRUE;
			SetCapture(hWnd);
		}

		return 0;
	}

	LRESULT OnLButtonUp(HWND hWnd,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		if( m_bCapture )
		{
			// determine split position
			SetPosAndCheckLimit(x);

			RECT rc;
			GetClientRect(hWnd,&rc);
			m_ratio = ((double)m_xSplitPos / (double)_RECT_WIDTH(rc));

			m_bCapture = FALSE;
			ReleaseCapture();
		}

		return 0;
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
	    {
			case WM_MOUSEMOVE:
				return OnMouseMove(hWnd,uMsg,wParam,lParam);
			case WM_LBUTTONDOWN:
				return OnLButtonDown(hWnd,uMsg,wParam,lParam);
			case WM_LBUTTONUP:
				return OnLButtonUp(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}
};

//
// Simple Vertical Splitter Window
//
template <class T>
class CSimpleVSplitWindowEx
{
	INT  *m_pxSplitPos;
	BOOL m_bHandled;
protected:
	BOOL m_bCapture;
	double m_ratio;
public:
	virtual VOID UpdateLayout(int cx,int cy,BOOL absPos=FALSE)
	{
	}

	CSimpleVSplitWindowEx()
	{
		m_bCapture = FALSE;
		m_pxSplitPos = nullptr;
		double m_ratio = 0.0;
		m_bHandled = FALSE;
	}

	BOOL IsMessageHandled() const
	{
		return m_bHandled;
	}

	VOID SetSplitterValue(INT *pv)
	{
		m_pxSplitPos = pv;
	}

	INT *GetSplitterValuePtr() const
	{
		return m_pxSplitPos;
	}

	VOID SetSplitPos(int pos)
	{
		if( m_pxSplitPos != nullptr ) {
			*m_pxSplitPos = pos;

			RECT rc;
			T *pThis = static_cast<T*>(this);
			GetClientRect(pThis->m_hWnd,&rc);
			m_ratio = ((double)*m_pxSplitPos / (double)_RECT_WIDTH(rc));
		}
	}

	int GetSplitPos() const
	{
		if( m_pxSplitPos )
			return *m_pxSplitPos;
		return -1;
	}

	virtual BOOL IsOverSplitterV(int x,int y)
	{
		if( m_pxSplitPos )
			return (((*m_pxSplitPos-4) <= x) && (x <= (*m_pxSplitPos+4)));
		return FALSE;
	}

	VOID SetPosAndCheckLimit(int x)
	{
		if( m_pxSplitPos )
		{
#define SPLIT_LIMIT 32
			RECT rc;
			T *pThis = static_cast<T*>(this);
			GetClientRect(pThis->m_hWnd,&rc);

			if( x < SPLIT_LIMIT ) 
				*m_pxSplitPos = SPLIT_LIMIT;
			else if( x > (_RECT_WIDTH(rc) - SPLIT_LIMIT) )
				*m_pxSplitPos = (_RECT_WIDTH(rc) - SPLIT_LIMIT);
			else
				*m_pxSplitPos = x;
		}
	}

	LRESULT OnMouseMove(HWND hWnd,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		if( !m_bCapture )
		{
			if( IsOverSplitterV(x,y)  )
			{
				SetCursor( LoadCursor(NULL,MAKEINTRESOURCE(IDC_SIZEWE)) );
				m_bHandled = true;
			}
		}
		else
		{
			// moving splitter
			SetPosAndCheckLimit(x);

			RECT rc;
			GetClientRect(hWnd,&rc);
			UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),TRUE);
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);

			m_bHandled = true;
		}
		return 0;
	}

	LRESULT OnLButtonDown(HWND hWnd,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		if( IsOverSplitterV(x,y)  )
		{
			ASSERT(m_pxSplitPos != NULL);

			// start move splitter
			SetCursor( LoadCursor(NULL,MAKEINTRESOURCE(IDC_SIZEWE)) );

			*m_pxSplitPos = x;

			m_bCapture = TRUE;
			SetCapture(hWnd);

			m_bHandled = true;

			EnterSplitterDrag();
		}

		return 0;
	}

	LRESULT OnLButtonUp(HWND hWnd,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		if( m_bCapture )
		{
			ASSERT(m_pxSplitPos != NULL);

			// determine split position
			SetPosAndCheckLimit(x);

			RECT rc;
			GetClientRect(hWnd,&rc);
			m_ratio = ((double)*m_pxSplitPos / (double)_RECT_WIDTH(rc));

			m_bCapture = FALSE;
			ReleaseCapture();

			LeaveSplitterDrag();

			m_bHandled = true;
		}

		return 0;
	}

	virtual void EnterSplitterDrag() {}
	virtual void LeaveSplitterDrag() {}

	virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT &lResult)
	{
		m_bHandled = false;

		lResult = 0;

		switch(uMsg)
	    {
			case WM_MOUSEMOVE:
				lResult = OnMouseMove(hWnd,uMsg,wParam,lParam);
				break;
			case WM_LBUTTONDOWN:
				lResult = OnLButtonDown(hWnd,uMsg,wParam,lParam);
				break;
			case WM_LBUTTONUP:
				lResult = OnLButtonUp(hWnd,uMsg,wParam,lParam);
				break;
		}
		return m_bHandled;
	}
};

//
// Simple Horizontal Splitter Window
//
template <class T>
class CSimpleHSplitWindowEx
{
	INT  *m_pySplitPos;
	BOOL m_bHandled;
protected:
	BOOL m_bCapture;
	double m_ratio;
public:
	virtual VOID UpdateLayout(int cx,int cy,BOOL absPos=FALSE)
	{
	}

	CSimpleHSplitWindowEx()
	{
		m_bCapture = FALSE;
		m_pySplitPos = nullptr;
		double m_ratio = 0.0;
		m_bHandled = FALSE;
	}

	BOOL IsMessageHandled() const
	{
		return m_bHandled;
	}

	VOID SetSplitterValue(INT *pv)
	{
		m_pySplitPos = pv;
	}

	INT *GetSplitterValuePtr() const
	{
		return m_pySplitPos;
	}

	VOID SetSplitPos(int pos)
	{
		if( m_pySplitPos != nullptr ) {
			*m_pySplitPos = pos;

			RECT rc;
			GetClientRect(m_hWnd,&rc);
			m_ratio = ((double)*m_pySplitPos / (double)_RECT_HIGHT(rc));
		}
	}

	int GetSplitPos() const
	{
		if( m_pySplitPos )
			return *m_pySplitPos;
		return -1;
	}

	virtual BOOL IsOverSplitterH(int x,int y)
	{
		if( m_pySplitPos )
			return (((*m_pySplitPos-4) <= y) && (y <= (*m_pySplitPos+4)));
		return FALSE;
	}

	VOID SetPosAndCheckLimit(int y)
	{
		if( m_pySplitPos )
		{
#define SPLIT_LIMIT 32
			RECT rc;
			T *pThis = static_cast<T*>(this);
			GetClientRect(pThis->m_hWnd,&rc);

			if( y < SPLIT_LIMIT ) 
				*m_pySplitPos = SPLIT_LIMIT;
			else if( y > (_RECT_WIDTH(rc) - SPLIT_LIMIT) )
				*m_pySplitPos = (_RECT_WIDTH(rc) - SPLIT_LIMIT);
			else
				*m_pySplitPos = y;
		}
	}

	LRESULT OnMouseMove(HWND hWnd,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		if( !m_bCapture )
		{
			if( IsOverSplitterH(x,y)  )
			{
				SetCursor( LoadCursor(NULL,MAKEINTRESOURCE(IDC_SIZENS)) );
				m_bHandled = true;
			}
		}
		else
		{
			// moving splitter
			SetPosAndCheckLimit(y);

			RECT rc;
			GetClientRect(hWnd,&rc);
			UpdateLayout(_RECT_WIDTH(rc),_RECT_HIGHT(rc),TRUE);
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);

			m_bHandled = true;
		}
		return 0;
	}

	LRESULT OnLButtonDown(HWND hWnd,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		if( IsOverSplitterH(x,y)  )
		{
			ASSERT(m_pySplitPos != NULL);

			// start move splitter
			SetCursor( LoadCursor(NULL,MAKEINTRESOURCE(IDC_SIZENS)) );

			*m_pySplitPos = x;

			m_bCapture = TRUE;
			SetCapture(hWnd);

			m_bHandled = true;

			EnterSplitterDrag();
		}

		return 0;
	}

	LRESULT OnLButtonUp(HWND hWnd,UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		int x = GET_X_LPARAM(lParam);
		int y = GET_Y_LPARAM(lParam);

		if( m_bCapture )
		{
			ASSERT(m_pySplitPos != NULL);

			// determine split position
			SetPosAndCheckLimit(y);

			RECT rc;
			GetClientRect(hWnd,&rc);
			m_ratio = ((double)*m_pySplitPos / (double)_RECT_HIGHT(rc));

			m_bCapture = FALSE;
			ReleaseCapture();

			LeaveSplitterDrag();

			m_bHandled = true;
		}

		return 0;
	}

	virtual void EnterSplitterDrag() {}
	virtual void LeaveSplitterDrag() {}

	virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT &lResult)
	{
		m_bHandled = false;

		lResult = 0;

		switch(uMsg)
	    {
			case WM_MOUSEMOVE:
				lResult = OnMouseMove(hWnd,uMsg,wParam,lParam);
				break;
			case WM_LBUTTONDOWN:
				lResult = OnLButtonDown(hWnd,uMsg,wParam,lParam);
				break;
			case WM_LBUTTONUP:
				lResult = OnLButtonUp(hWnd,uMsg,wParam,lParam);
				break;
		}
		return m_bHandled;
	}
};
