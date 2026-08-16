#pragma once
//*****************************************************************************
//
//  page_radix.h
//
//  PURPOSE: Radix Converter ToolPad Page
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2026-03-26 Created.
//
//*****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//

class CRadixToolPage : public CControlPane
{
	CUILayout m_Layout;

	HWND m_hwndHex;
	HWND m_hwndDec;
	HWND m_hwndBin;

	BOOL m_bInEdit;

public:
	DEFDIALOGRESID(IDD_TOOLPAGE_RADIX)

	CRadixToolPage()
	{
		m_hwndHex = NULL;
		m_hwndDec = NULL;
		m_hwndBin = NULL;
		m_bInEdit = FALSE;
	}

	~CRadixToolPage()
	{
	}

	virtual VOID OnInitPane(PVOID) {}
	virtual VOID OnStartOperation(PVOID) {}
	virtual VOID OnEndOperation(PVOID) {}

	virtual INT_PTR DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch( uMsg )
		{
			case WM_INITDIALOG:
			{
				m_Layout.Initialize(hWnd);			

				m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_TOPRIGHT,IDC_EDIT1,FALSE);
				m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_TOPRIGHT,IDC_EDIT2,FALSE);
				m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_TOPRIGHT,IDC_EDIT3,FALSE);

				m_hwndHex = GetDlgItem(hWnd,IDC_EDIT1);
				m_hwndDec = GetDlgItem(hWnd,IDC_EDIT2);
				m_hwndBin = GetDlgItem(hWnd,IDC_EDIT3);

				SetWindowSubclass(m_hwndDec,&EditBox_DecInputSubclassProc,1,0);
				SetWindowSubclass(m_hwndHex,&EditBox_HexInputSubclassProc,2,0);
				SetWindowSubclass(m_hwndBin,&EditBox_BinInputSubclassProc,3,0);

				return TRUE;
			}
			case WM_DESTROY:
			{
				RemoveWindowSubclass(m_hwndDec,&EditBox_DecInputSubclassProc,1);
				RemoveWindowSubclass(m_hwndHex,&EditBox_HexInputSubclassProc,2);
				RemoveWindowSubclass(m_hwndBin,&EditBox_BinInputSubclassProc,3);
				return 0;
			}
			case WM_SIZE:
			{
				int cx = GET_X_LPARAM(lParam);
				int cy = GET_Y_LPARAM(lParam);
				m_Layout.AdjustControls(cx,cy);
				break;
			}
			case WM_COMMAND:
			{
				switch( HIWORD(wParam) )
				{
					case EN_CHANGE:
						OnEditChange((HWND)lParam);
						break;
				}

				switch( LOWORD(wParam) )
				{
					case IDOK:
						return 0;
				}
			}
		}
		return CControlPane::DlgProc(hWnd,uMsg,wParam,lParam);
	}

private:
	VOID OnEditChange(HWND hwndEdit)
	{
		if( m_bInEdit )
			return ;

		m_bInEdit = TRUE;

		UINT64 val = 0;
		int base = -1;
		if( hwndEdit == m_hwndHex )
		{
			base = 16;
		}
		else if( hwndEdit == m_hwndDec )
		{
			base = 10;
		}
		else if( hwndEdit == m_hwndBin )
		{
			base = 2;
		}
		else
		{
			return ;
		}

		WCHAR sz[64+1];
		int cch;

		cch = GetWindowText(hwndEdit,sz,ARRAYSIZE(sz));

		if( cch == 0 )
		{
			_set_errno(22); // EINVAL
			updateHex(0);
			updateDec(0);
			updateBin(0);
		}
		else
		{
			_set_errno(0);

			val = _wcstoui64(sz,nullptr,base);   // 64bit unsigned, todo:

			switch( base )
			{
				case 16:
					updateDec(val);
					updateBin(val);
					break;
				case 10:
					updateHex(val);
					updateBin(val);
					break;
				case 2:
					updateHex(val);
					updateDec(val);
					break;
			}
		}
		m_bInEdit = FALSE;
	}

	void updateHex(UINT64 val)
	{
		setValue(m_hwndHex,val,16);
	}

	void updateDec(UINT64 val)
	{
		setValue(m_hwndDec,val,10);
	}

	void updateBin(UINT64 val)
	{
		setValue(m_hwndBin,val,2);
	}

	void setValue(HWND hwndEdit,UINT64 val,int Radix)
	{
		WCHAR sz[64+1];

		int errorCode;
		_get_errno( &errorCode );
		if( errorCode != 0 )
		{
			sz[0] = 0;
		}
		else
		{
			_set_errno( 0 );
			_ui64tow_s(val,sz,ARRAYSIZE(sz),Radix);  // 64bit unsigned, todo:
			_get_errno( &errorCode );
			if( errorCode != 0 )
				sz[0] = 0;
		}

		SetWindowText(hwndEdit,sz);
	}
};
