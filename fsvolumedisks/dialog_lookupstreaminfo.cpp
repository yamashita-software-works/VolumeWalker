//*****************************************************************************
//
//  dialog_lookupstreaminfo.cpp
//
//  PURPOSE: Lookup stream name dialog.
//
//  AUTHOR:  YAMASHITA Katsuhiro
//
//  HISTORY: 2024-12-13 Created
//
//*****************************************************************************
#include "stdafx.h"
#include "fsvolumelist.h"
#include "resource.h"
#include "basewindow.h"
#include "uilayout.h"
#include "..\libntwdk\libntwdk.h"
#include "..\libntwdk\ntnativehelp.h"
#include "ntvolumenames.h"
#include "ntobjecthelp.h"
#include "ntwin32helper.h"
#include "..\fsfilelib\fsfilelib.h"

LRESULT
CALLBACK
HexEditBoxSubclassProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
	)
{
	switch( uMsg )
	{
		case WM_CHAR:
		{
			INT ch = (INT)wParam;

			WCHAR sz[64];
			GetWindowText(hWnd,sz,ARRAYSIZE(sz));
			int cch = (int)wcslen(sz);

			// If the prefix "0x" is expected.
			if( cch == 1 && (ch == 'x' || ch == 'X') )
				break;

			if( (ch & ~0xff) != 0 )
				return 0;

			if( isxdigit(ch) )
				break;

			if( ch == VK_BACK )
				break;

			return 0;
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/*++

Flags:

LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_PAGE_FILE
0x00000001
The stream is part of the system pagefile.

LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_DENY_DEFRAG_SET
0x00000002
he stream is locked from defragmentation. The HandleInfo member of 
the MARK_HANDLE_INFO structure for this stream has 
the MARK_HANDLE_PROTECT_CLUSTERS flag set.

LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_FS_SYSTEM_FILE
0x00000004
The stream is part of a file that is internal to the filesystem.

LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_TXF_SYSTEM_FILE
0x00000008
The stream is part of a file that is internal to TxF.

LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_DATA
0x01000000
The stream is part of a $DATA attribute for the file (data stream).

LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_INDEX
0x02000000
The stream is part of the $INDEX_ALLOCATION attribute for the file.

LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_SYSTEM
0x03000000
The stream is part of another attribute for the file. 

--*/

typedef struct _LSFCE_FLAG_TEXT
{
	ULONG Flag;
	PCWSTR pszString;
} LSFCE_FLAG_TEXT;

LSFCE_FLAG_TEXT flag_text[] = 
{
	{LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_PAGE_FILE,        L"Page File"},
	{LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_DENY_DEFRAG_SET,  L"Deny Defrag Set"},
	{LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_FS_SYSTEM_FILE,   L"FS System File"},
	{LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_TXF_SYSTEM_FILE,  L"TxF System File"},
};

LSFCE_FLAG_TEXT attribute_text[] = 
{
	{LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_DATA,        L"Data"},
	{LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_INDEX,       L"Index"},
	{LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_SYSTEM,      L"System"},
};

typedef HRESULT (CALLBACK *STREAM_LOOKUP_CALLBACK)(int,PVOID,ULONG_PTR Context);

#define WM_WORKER_THREAD_EXIT (WM_APP+0)

typedef struct _STREAM_LOOKUP_THREAD_PARAM
{
	PVOID pThis;
	HANDLE VolumeHandle;
	LONGLONG ClusterNumber;
	WCHAR VolumeName[MAX_PATH];
	STREAM_LOOKUP_CALLBACK pfnCallback;
	ULONG_PTR Context;
} STREAM_LOOKUP_THREAD_PARAM;

class CLookupStreamName
{
public:
	HRESULT WorkProc(STREAM_LOOKUP_THREAD_PARAM *psltp)
	{
		BOOL bSuccess;

		DWORD cbOutBuffer = sizeof(LOOKUP_STREAM_FROM_CLUSTER_OUTPUT)
							+ sizeof(LOOKUP_STREAM_FROM_CLUSTER_ENTRY)
							+ 65536;
	
		PBYTE pOutBuffer = (PBYTE)_MemAlloc( cbOutBuffer );
		if( pOutBuffer == NULL )
		{
			CloseHandle(psltp->VolumeHandle);
			return E_OUTOFMEMORY;
		}

		DWORD cb;

		LOOKUP_STREAM_FROM_CLUSTER_INPUT in;
		in.Flags = 0;
		in.NumberOfClusters = 1;
		in.Cluster[0].QuadPart = psltp->ClusterNumber;

		bSuccess = DeviceIoControl(psltp->VolumeHandle,
						FSCTL_LOOKUP_STREAM_FROM_CLUSTER,
						&in,sizeof(in),
						pOutBuffer,cbOutBuffer,
						&cb,NULL);

		if( bSuccess )
		{
			LOOKUP_STREAM_FROM_CLUSTER_OUTPUT *pout = (LOOKUP_STREAM_FROM_CLUSTER_OUTPUT *)pOutBuffer;
	
			psltp->pfnCallback( 0, NULL, psltp->Context );
		
			LOOKUP_STREAM_FROM_CLUSTER_ENTRY *entry = (LOOKUP_STREAM_FROM_CLUSTER_ENTRY *)((ULONG_PTR)pout + pout->Offset);
			for(;;)
			{
				HRESULT hr;
				hr = psltp->pfnCallback( 1, entry, psltp->Context );
	
				if( hr != S_OK )
					break;
	
				if( entry->OffsetToNext == 0)
					break;
	
				entry = (LOOKUP_STREAM_FROM_CLUSTER_ENTRY *)((DWORD_PTR)entry + entry->OffsetToNext);
			}
		}
		else
		{
			DWORD dwError = GetLastError();
			psltp->pfnCallback( 0, NULL, psltp->Context );
			psltp->pfnCallback( 3, (PVOID)GetLastError(), psltp->Context );
		}

		CloseHandle(psltp->VolumeHandle);

		_MemFree(pOutBuffer);

		psltp->pfnCallback( 4, 0, psltp->Context );

		return S_OK;
	}

	static DWORD WINAPI WorkThreadProc(LPVOID lpParameter)
	{
		STREAM_LOOKUP_THREAD_PARAM *psltp = (STREAM_LOOKUP_THREAD_PARAM *)lpParameter;

		((CLookupStreamName *)psltp->pThis)->WorkProc(psltp);

		delete psltp;

		return 0;
	}
};

typedef struct _LOOKUPSTREAM_DIALOG_PARAM
{
	HWND hWnd;
	WCHAR szVolumeName[MAX_PATH];
} LOOKUPSTREAM_DIALOG_PARAM;

struct CLookupStreamNameDialog : public CDialogWindow
{
	HWND m_hWndList;
	HIMAGELIST m_himl;
	PWSTR m_pszVolumeName;
	BOOL m_bRunWorkerThread;

	VOLUME_NAME_STRING_ARRAY *m_pVolumeNames;

	CUILayout m_Layout;

	CLookupStreamNameDialog()
	{
		m_hWndList = NULL;
		m_himl = NULL;
		m_pszVolumeName = NULL;
		m_bRunWorkerThread = FALSE;
	}
	
	~CLookupStreamNameDialog()
	{
		_SafeMemFree(m_pszVolumeName);
	}


	void SetStatusText(PCWSTR pszMsg)
	{
		SendDlgItemMessage(m_hWnd,IDC_EDIT3,EM_SETSEL,(WPARAM)-1,(LPARAM)-1);
		SendDlgItemMessage(m_hWnd,IDC_EDIT3,EM_REPLACESEL,0,(LPARAM)pszMsg);
	}

	void EnableControls(BOOL bEnable)
	{
		UINT ids[] = {
			IDC_EDIT1,
			IDC_EDIT2,
			IDOK,
		};

		for(int i = 0; i < _countof(ids); i++)
		{
			EnableWindow(GetDlgItem(m_hWnd,ids[i]),bEnable);
		}
	}

	void DumpFlagText(ULONG Flags)
	{
		WCHAR sz[100];
		int i;

		for(i = 0; i < _countof(attribute_text); i++)
		{
			if( (attribute_text[i].Flag & LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_MASK) == 
				(Flags & LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_MASK) )
			{
				SetStatusText(L"    ");
	
				StringCchPrintf(sz,ARRAYSIZE(sz),L"0x%08X - ",attribute_text[i].Flag);
				SetStatusText(sz);
	
				SetStatusText(attribute_text[i].pszString);
				SetStatusText(L"\r\n");
				break;
			}
		}

		if( (Flags & ~LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_MASK) != 0 )
		{
			for(i = 0; i < _countof(flag_text); i++)
			{
				if( flag_text[i].Flag & Flags )
				{
					SetStatusText(L"    ");
	
					StringCchPrintf(sz,ARRAYSIZE(sz),L"0x%08X - ",flag_text[i].Flag);
					SetStatusText(sz);
	
					SetStatusText(flag_text[i].pszString);
					SetStatusText(L"\r\n");
				}
			}
		}
	}

	HRESULT CALLBACK MyCallback(LOOKUP_STREAM_FROM_CLUSTER_ENTRY *pEntry)
	{
		WCHAR szMsg[MAX_PATH];

		StringCchPrintf(szMsg,_countof(szMsg),L"Volume Relative Path:\r\n    %s\r\n",pEntry->FileName);
		SetStatusText(szMsg);

		StringCchPrintf(szMsg,_countof(szMsg),L"\r\nLCN:\r\n    0x%I64X\r\n",pEntry->Cluster.QuadPart);
		SetStatusText(szMsg);

		StringCchPrintf(szMsg,_countof(szMsg),L"\r\nFlags:\r\n    0x%08X\r\n",pEntry->Flags);
		SetStatusText(szMsg);

		SetStatusText(L"\r\n");

		DumpFlagText(pEntry->Flags);

		return S_OK;
	}

	HRESULT CALLBACK MyCallbackClear()
	{
		SetDlgItemText(m_hWnd,IDC_EDIT3,L"");
		return S_OK;
	}

	HRESULT CALLBACK MyCallbackSetText(PWSTR pszText)
	{
		SetDlgItemText(m_hWnd,IDC_EDIT3,pszText);
		return S_OK;
	}

	HRESULT CALLBACK MyCallbackError(ULONG_PTR dwError)
	{
// ERROR_FILE_NOT_FOUND
#if 0
		WCHAR szMsg[MAX_PATH];
		StringCchPrintf(szMsg,_countof(szMsg),L"\r\nError %u\r\n",dwError);
		SetStatusText(szMsg);
#else
		PWSTR pMessage;
		_GetSystemErrorMessage((ULONG)dwError,&pMessage);

		WCHAR szBuffer[256];
		FormatNtStatusErrorMessage(pMessage,szBuffer,ARRAYSIZE(szBuffer),0);

		SetStatusText(szBuffer);

		_FreeSystemErrorMessage(pMessage);
#endif
		return S_OK;
	}

	HRESULT CALLBACK MyCallbackExit(ULONG_PTR dwError)
	{
		PostMessage(m_hWnd,WM_WORKER_THREAD_EXIT,0,0);
		return S_OK;
	}

	static HRESULT CALLBACK callback(int code,PVOID ptr,ULONG_PTR Context)
	{
		switch( code )
		{
			case 0: // Clear
				return ((CLookupStreamNameDialog *)Context)->MyCallbackClear();
			case 1: // Progress
				return ((CLookupStreamNameDialog *)Context)->MyCallback((LOOKUP_STREAM_FROM_CLUSTER_ENTRY *)ptr);
			case 2: // Set text
				return ((CLookupStreamNameDialog *)Context)->MyCallbackSetText((PWSTR)ptr);
			case 3: // Error detected
				return ((CLookupStreamNameDialog *)Context)->MyCallbackError((ULONG_PTR)ptr);
			case 4: // Exit thread
				return ((CLookupStreamNameDialog *)Context)->MyCallbackExit((ULONG_PTR)ptr);
		}
		return S_OK;
	}

	INT_PTR OnInitDialog(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		LOOKUPSTREAM_DIALOG_PARAM *pdlgParam = (LOOKUPSTREAM_DIALOG_PARAM *)lParam;
		SetWindowLongPtr(hDlg,DWLP_USER,(LONG_PTR)pdlgParam);

		_CenterWindow(hDlg,GetActiveWindow());

		//
		// Remove the icon, the minimize and the restore buttons 
		// from the window has resizable border.
		//
		DWORD dw = GetWindowLong(hDlg,GWL_EXSTYLE);
		dw |= WS_EX_DLGMODALFRAME;
		SetWindowLong(hDlg,GWL_EXSTYLE,dw);
		if( (HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0) == NULL )
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)NULL);

		//
		// Initialize Layout
		//
		m_Layout.Initialize(hDlg);			
		m_Layout.AnchorControl(CUILayout::AP_TOPLEFT,CUILayout::AP_BOTTOMRIGHT,IDC_EDIT3,FALSE);
		m_Layout.AnchorControl(CUILayout::AP_BOTTOMRIGHT,CUILayout::AP_BOTTOMRIGHT,IDOK,TRUE);

		SetDlgItemText(hDlg,IDC_EDIT1,pdlgParam->szVolumeName);

		SetWindowSubclass(GetDlgItem(hDlg,IDC_EDIT2),&HexEditBoxSubclassProc,0x1,0);

		SetFocus( GetDlgItem(hDlg,IDC_EDIT2) );

		SendDlgItemMessage( hDlg, IDC_EDIT3, WM_SETFONT, (WPARAM)GetGlobalFont(hDlg), 0);

		return FALSE;
	}

	LRESULT OnDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		RemoveWindowSubclass(GetDlgItem(hDlg,IDC_EDIT2),&HexEditBoxSubclassProc,0x1);
		DeleteObject( (HGLOBAL)SendDlgItemMessage( hDlg, IDC_EDIT3, WM_GETFONT, 0, 0) );
		return 0;
	}

	LRESULT OnClose(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		if( m_bRunWorkerThread )
			return 0;
		EndDialog(hDlg,IDCLOSE);
		return 0;
	}
	
	LRESULT OnNcDestroy(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		return 0;
	}

	LRESULT OnSize(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		m_Layout.AdjustControls(cx,cy);
		return 0;
	}

public:
	static DWORD WINAPI WorkThreadProc(LPVOID lpParameter)
	{
		STREAM_LOOKUP_THREAD_PARAM *psltp = (STREAM_LOOKUP_THREAD_PARAM *)lpParameter;

		((CLookupStreamName *)psltp->pThis)->WorkProc(psltp);

		delete psltp;

		return 0;
	}

	HRESULT StartLookup(PCWSTR pszVolumeName,LONGLONG ClusterNumber)
	{
		CWaitCursor wait;

		SetDlgItemText(m_hWnd,IDC_EDIT3,L"");

		LONG ntStatus;
		HANDLE hVolume;
		ntStatus = OpenVolume(pszVolumeName,OPEN_GENERIC_READ|OPEN_READ_DATA,&hVolume);
		if( hVolume == INVALID_HANDLE_VALUE )
		{
			PWSTR pMessage;
			_GetSystemErrorMessage(ntStatus,&pMessage);

			WCHAR szBuffer[256];
			FormatNtStatusErrorMessage(pMessage,szBuffer,ARRAYSIZE(szBuffer),0);

			SetStatusText(szBuffer);

			_FreeSystemErrorMessage(pMessage);
			return E_FAIL;
		}

		m_bRunWorkerThread = TRUE;

		SetStatusText(L"Start lookup...\r\n(This operation may take a long time)");

		EnableControls(FALSE);

		STREAM_LOOKUP_THREAD_PARAM *sltp = new STREAM_LOOKUP_THREAD_PARAM;

		sltp->pThis = this;
		sltp->VolumeHandle = hVolume;
		sltp->ClusterNumber = ClusterNumber;
		StringCchCopy(sltp->VolumeName,_countof(sltp->VolumeName),pszVolumeName);

		sltp->pfnCallback = &callback;
		sltp->Context = (ULONG_PTR)this;

		QueueUserWorkItem(WorkThreadProc,sltp,WT_EXECUTEDEFAULT);

		return 0;
	}

	VOID OnOK(HWND hDlg)
	{
		LOOKUPSTREAM_DIALOG_PARAM *pdlgParam = (LOOKUPSTREAM_DIALOG_PARAM *)GetWindowLongPtr(hDlg,DWLP_USER);

		WCHAR szVolume[MAX_PATH];
		WCHAR szLcn[64];

		GetDlgItemText(hDlg,IDC_EDIT1,szVolume,_countof(szVolume));

		LONGLONG lRet;
		GetDlgItemText(hDlg,IDC_EDIT2,szLcn,_countof(szLcn));
		StrToInt64Ex(szLcn,STIF_SUPPORT_HEX,&lRet);

		StartLookup(szVolume,lRet);
	}

	LRESULT OnCommand(HWND hDlg,WPARAM wParam,LPARAM lParam)
	{
		if( HIWORD(wParam) == EN_UPDATE )
		{
			if( LOWORD(wParam) == IDC_EDIT1 || LOWORD(wParam) == IDC_EDIT2 )
			{
				EnableWindow(GetDlgItem(hDlg,IDOK),
					(GetWindowTextLength(GetDlgItem(hDlg,IDC_EDIT1)) &&
					 GetWindowTextLength(GetDlgItem(hDlg,IDC_EDIT2))) );
			}
		}

		switch( LOWORD(wParam) )
		{
			case IDOK:
				OnOK(hDlg);
				break;
			case IDCLOSE:
			case IDCANCEL:
			{
				if( m_bRunWorkerThread )
					return 0;
				EndDialog(hDlg,(INT_PTR)LOWORD(wParam));
				break;
			}
		}
		return 0;
	}

	//---------------------------------------------------------------------------
	//
	//  DlgProc()
	//
	//  PURPOSE:
	//
	//---------------------------------------------------------------------------
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		switch( uMsg )
		{
			case WM_INITDIALOG:
				return OnInitDialog(hDlg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hDlg,wParam,lParam);
			case WM_NCDESTROY:
				return OnNcDestroy(hDlg,wParam,lParam);
			case WM_SIZE:
				return OnSize(hDlg,wParam,lParam);
			case WM_CLOSE:
				return OnClose(hDlg,wParam,lParam);
			case WM_COMMAND:
				return OnCommand(hDlg,wParam,lParam);
			case WM_WORKER_THREAD_EXIT:
				m_bRunWorkerThread = FALSE;
				EnableControls(TRUE);
				return 0;
		}
		return 0;
	}
};

//---------------------------------------------------------------------------
//
//  LookupStreamNameDialog()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
HRESULT
WINAPI
LookupStreamNameDialog(
	HWND hWnd,
	PWSTR pszVolumeName,
	DWORD dwFlags
	)
{
	HRESULT hr;
	LOOKUPSTREAM_DIALOG_PARAM *pParam = new LOOKUPSTREAM_DIALOG_PARAM;
	if( pParam == NULL )
		return E_OUTOFMEMORY;

	ZeroMemory(pParam,sizeof(LOOKUPSTREAM_DIALOG_PARAM));

	NtPathGetRootDirectory(pszVolumeName,pParam->szVolumeName,MAX_PATH);
	if( pParam->szVolumeName[0] == 0 )
		StringCchCopy(pParam->szVolumeName,MAX_PATH,pszVolumeName);
	RemoveBackslash(pParam->szVolumeName);

	CLookupStreamNameDialog *dlg = new CLookupStreamNameDialog;

	if( dlg )
	{
		if( dlg->DoModal(hWnd,IDD_LOOKUP_STREAM_NAME,(LPARAM)pParam,_GetResourceInstance()) == IDOK )
		{
			hr = S_OK;
		}
		else
		{
			hr = S_FALSE;
		}

		delete dlg;
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	delete pParam;

	return hr;
}
