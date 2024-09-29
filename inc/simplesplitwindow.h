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
