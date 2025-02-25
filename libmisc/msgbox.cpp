//***************************************************************************
//*                                                                         *
//*  msgbox.cpp                                                             *
//*                                                                         *
//*  Create: 2023-09-25                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "libmisc.h"
#include "stringbuffer.h"

static HHOOK g_hMsgHook  = NULL;
static HWND  g_hwndOwner = NULL;
static UINT  g_disableButton  = 0;
static PWSTR g_pszMsgCaption = NULL;

HRESULT
WINAPI
SetMessageBoxCaption(
	PCWSTR Caption
	)
{
	if( Caption == NULL )
	{
		_SafeMemFree(g_pszMsgCaption);
		return S_OK;
	}
	g_pszMsgCaption = _MemAllocString( Caption );
	if( g_pszMsgCaption == NULL )
		return E_OUTOFMEMORY;
	return S_OK;
}

static struct {
	UINT flag;
	UINT id;
} disableBtnMap[] = {
	{ _MB_DISABLE_CONTINUE, IDCONTINUE },
	{ _MB_DISABLE_RETRY,    IDRETRY },
	{ _MB_DISABLE_TRYAGAIN, IDTRYAGAIN },
	{ _MB_DISABLE_CANCEL,   IDCANCEL },
};

static LRESULT CALLBACK MessageBoxFilter(int code,WPARAM wParam,LPARAM lParam)
{
	if( code >= 0 )
	{
		CWPRETSTRUCT *pmsg = (CWPRETSTRUCT *)lParam;
		if( pmsg->message == WM_INITDIALOG )
		{
			HWND hwndOwner = g_hwndOwner;
			if( hwndOwner == NULL )
				hwndOwner = GetWindow(pmsg->hwnd,GW_OWNER);

			_CenterWindow(pmsg->hwnd,hwndOwner);

			if( g_disableButton != 0 )
			{
				HWND hwndBtn;
				int i;
				for(i = 0; i < _countof(disableBtnMap); i++)
				{
					if( (disableBtnMap[i].flag & g_disableButton) != 0 )
					{
						hwndBtn = GetDlgItem(pmsg->hwnd,disableBtnMap[i].id);
						if( hwndBtn != NULL )
							EnableWindow(hwndBtn,FALSE);
					}
				}
			}
		}

		return 0;
	}
	return CallNextHookEx(g_hMsgHook, code, wParam, lParam);
}

int
WINAPI
MsgBoxIcon(
	HWND hwndOwner,
	LPCTSTR pszText,
	LPCTSTR pszCaption,
	UINT flags,
	UINT idDisableButton,
	HINSTANCE hInstance,
	LPCWSTR lpszIcon
	)
{
	int n;
	g_hwndOwner = hwndOwner;

	if( hwndOwner != NULL )
	{
		g_hMsgHook = SetWindowsHookEx(WH_CALLWNDPROCRET,
						MessageBoxFilter, NULL, ::GetCurrentThreadId());
	}

	if( pszCaption == NULL )
	{
		if( g_pszMsgCaption )
			pszCaption = (LPTSTR)g_pszMsgCaption;
		else
			pszCaption = TEXT("");
	}

	g_disableButton = idDisableButton;

	MSGBOXPARAMS mbp = {0};
	mbp.cbSize = sizeof(mbp);
	mbp.hwndOwner = hwndOwner;
    mbp.lpszText = pszText;
    mbp.lpszCaption = pszCaption;
	mbp.dwStyle = flags|MB_USERICON;
	mbp.hInstance = hInstance;
    mbp.lpszIcon = lpszIcon;
	n = MessageBoxIndirect(&mbp);

	if( hwndOwner != NULL )
	{
		UnhookWindowsHookEx(g_hMsgHook);
	}

	g_hwndOwner = NULL;

	return n;
}

int
WINAPI
MsgBoxEx(
	HWND hwndOwner,
	LPCTSTR pszText,
	LPCTSTR pszCaption,
	UINT flags,
	UINT idDisableButton
	)
{
	CStringBuffer sCaption(256);
	int n;
	g_hwndOwner = hwndOwner;

	if( hwndOwner != NULL )
	{
		g_hMsgHook = SetWindowsHookEx(WH_CALLWNDPROCRET,
						MessageBoxFilter, NULL, ::GetCurrentThreadId());
	}

	if( pszCaption == NULL )
	{
		if( g_pszMsgCaption )
			sCaption = g_pszMsgCaption;
	}
	else
	{
		if( IS_INTRESOURCE(pszCaption) )
		{
			LoadString(_libmisc_get_resource_handle(),(UINT)pszCaption,sCaption,sCaption.GetBufferSize());
		}
		else
		{
			sCaption = pszCaption;
		}
	}

	CStringBuffer *psText = NULL;
	if( IS_INTRESOURCE(pszText) )
	{
		psText = new CStringBuffer(2048);
		LoadString(_libmisc_get_resource_handle(),(UINT)pszText,*psText,psText->GetBufferSize());
	}

	g_disableButton = idDisableButton;

	n = MessageBox(hwndOwner,psText ? *psText : pszText,sCaption,flags);

	if( hwndOwner != NULL )
	{
		UnhookWindowsHookEx(g_hMsgHook);
	}

	if( psText )
		delete psText;

	g_hwndOwner = NULL;

	return n;
}

int
WINAPI
MsgBox(
	HWND hwnd,
	LPCTSTR pszText,
	LPCTSTR pszCaption,
	UINT flags
	)
{
	return MsgBoxEx(hwnd,pszText,pszCaption,flags,0);
}

int
WINAPI
MsgBox(
	HWND hwnd,
	LPCTSTR pszText,
	UINT flags
	)
{
	return MsgBoxEx(hwnd,pszText,NULL,flags,0);
}

int
WINAPI
MsgBox(
	HWND hwnd,
	UINT idString,
	UINT flags
	)
{
	return MsgBoxEx(hwnd,MAKEINTRESOURCE(idString),NULL,flags,0);
}

int
WINAPI
MsgBox(
	HWND hwnd,
	UINT idString,
	UINT idCaption,
	UINT flags
	)
{
	return MsgBoxEx(hwnd,MAKEINTRESOURCE(idString),MAKEINTRESOURCE(idCaption),flags,0);
}

EXTERN_C
INT
WINAPI
_ErrorMessageBoxEx(
	HWND hWnd,
	UINT_PTR idString,
	PCWSTR pszCaption,
	PCWSTR pszMessage,
	ULONG Status,
	ULONG Flags
	)
{
	int iRet = 0;

	if( idString != 0 )
	{
		const int cch = 1024;
		WCHAR *szMsg = _MemAllocStringBuffer(cch);
		WCHAR *szMsg2 = _MemAllocStringBuffer(cch);

		if( szMsg == NULL || szMsg2 == NULL )
		{
			_SafeMemFree(szMsg);
			_SafeMemFree(szMsg2);
			return -1;
		}

		if( IS_INTRESOURCE(idString) )
			LoadString(LIBMISC::g_ResourceInstance,(UINT)idString,szMsg,cch);
		else
			wcsncpy_s(szMsg,cch,(WCHAR *)idString,cch-1);

		PWSTR pMessage = NULL;

		if( Status != (ULONG)-1 )
		{
			_GetSystemErrorMessageEx(Status,&pMessage,LIBMISC::g_dwLanguageId);

			if( pMessage )
			{
				// NT system error message character convert
				// { } -> []
				if( Status >= 0xC0000000 )
				{
					PWSTR p = pMessage;
					while( *p != L'\0' )
					{
						if( *p == L'{' ) *p = L'[';
						else if( *p == L'}' ) *p = L']';
						else if( *p == L'\r' ) *p = L'\0';
						else if( *p == L'\n' ) *p = L'\0';
						p++;
					}
				}
				else
				{
					PWSTR p = pMessage;
					while( *p != L'\0' )
					{
						if( *p == L'\r' ) *p = L'\0';
						else if( *p == L'\n' ) *p = L'\0';
						p++;
					}
				}

				PWSTR p = pMessage;
				while( *p != L'\0' )
				{
					if( *p == L'%' )
					{
						if( iswdigit( *(p+1) ) )
						{
							PWSTR pNewText = (PWSTR)LocalAlloc(LPTR, wcslen(pMessage) + MAX_PATH + 1 );

							va_list param[1];
							param[0] = (va_list)pszMessage;

							va_list *Arguments = param;
							FormatMessage(FORMAT_MESSAGE_FROM_STRING,pMessage,0,0,pNewText,260,(va_list*)&Arguments);

							LocalFree((HLOCAL)pMessage);
							pMessage = pNewText;

							break;
						}
					}
					p++;
				}

				va_list param[2];
				param[0] = (va_list)pszMessage;
				param[1] = (va_list)pMessage;

				va_list *Arguments = param;

				FormatMessage(FORMAT_MESSAGE_FROM_STRING,szMsg,0,0,szMsg2,cch,(va_list*)&Arguments);

				_FreeSystemErrorMessage(pMessage);

				if( Flags == 0 )
					Flags = MB_OK|MB_ICONEXCLAMATION;
			}
			else
			{
				wcscpy_s(szMsg2,cch,L"Error"); // fatal error 
			}
		}
		else
		{
			va_list param[2];
			param[0] = (va_list)pszMessage;
			param[1] = (va_list)L"";

			va_list *Arguments = param;

			FormatMessage(FORMAT_MESSAGE_FROM_STRING,szMsg,0,0,szMsg2,cch,(va_list*)&Arguments);

			_FreeSystemErrorMessage(pMessage);
		}

		iRet = MsgBox(hWnd,szMsg2,pszCaption,Flags);

		_SafeMemFree(szMsg);
		_SafeMemFree(szMsg2);
	}
	else
	{
		PWSTR pMessage = NULL;
		_GetSystemErrorMessageEx(Status,&pMessage,LIBMISC::g_dwLanguageId);

		iRet = MsgBox(hWnd,pMessage,pszCaption,Flags);

		_FreeSystemErrorMessage(pMessage);
	}
	return iRet;
}

EXTERN_C
INT
WINAPI
_ErrorMessageBox(
	HWND hWnd,
	UINT_PTR idString,
	PCWSTR pszFile, 
	ULONG Status,
	ULONG Flags
	)
{
	return _ErrorMessageBoxEx(hWnd,idString,NULL,pszFile,Status,Flags);
}

EXTERN_C
INT
WINAPI
_ErrorMessageBoxEx2(
	HWND hWnd,
	PCWSTR pszLayout,
	PCWSTR pszCaption,
	PCWSTR pszMessage,
	PCWSTR pszReserved,
	ULONG Status,
	ULONG FormatFlags,
	ULONG Flags
	)
{
	int iRet = 0;

	if( pszLayout != 0 )
	{
		const int cch = 1024;
		WCHAR *szMsg2 = _MemAllocStringBuffer(cch);

		if( szMsg2 == NULL )
		{
			_SafeMemFree(szMsg2);
			return -1;
		}

		PWSTR SystemErrorMessage = NULL;
		_GetSystemErrorMessageEx(Status,&SystemErrorMessage,GetUserDefaultLangID());

		if( SystemErrorMessage )
		{
			// NT system error message character convert
			// { } -> []
			if( Status >= 0xC0000000 )
			{
				PWSTR p = SystemErrorMessage;
				while( *p != L'\0' )
				{
					if( *p == L'{' ) *p = L'[';
					else if( *p == L'}' ) *p = L']';
					else if( *p == L'\r' ) *p = L'\0';
					else if( *p == L'\n' ) *p = L'\0';
					p++;
				}
			}
			else
			{
				PWSTR p = SystemErrorMessage;
				while( *p != L'\0' )
				{
					if( *p == L'\r' ) *p = L'\0';
					else if( *p == L'\n' ) *p = L'\0';
					p++;
				}
			}

			PWSTR p = SystemErrorMessage;
			while( *p != L'\0' )
			{
				if( *p == L'%' )
				{
					// "%1,%2...%9,%0,%10..." any digit ok.
					if( iswdigit( *(p+1) ) )
					{
						DWORD cchNew = (DWORD)(wcslen(SystemErrorMessage) + wcslen(pszMessage) + 1);

						PWSTR pNewText = (PWSTR)LocalAlloc(LPTR, cchNew * sizeof(WCHAR) );

						va_list param[1];
						param[0] = (va_list)pszMessage;

						va_list *Arguments = param;
						FormatMessage(FORMAT_MESSAGE_FROM_STRING,SystemErrorMessage,0,0,pNewText,cchNew,(va_list*)&Arguments);

						LocalFree((HLOCAL)SystemErrorMessage);
						SystemErrorMessage = pNewText;

						break;
					}
				}
				p++;
			}
		}
		else
		{
			if( pszReserved )
				SystemErrorMessage = StrDup( pszReserved );
			else
				SystemErrorMessage = StrDup( L"" );
		}

		va_list param[2];
		param[0] = (va_list)pszMessage;
		param[1] = (va_list)SystemErrorMessage;

		va_list *Arguments = param;

		FormatMessage(FORMAT_MESSAGE_FROM_STRING,pszLayout,0,0,szMsg2,cch,(va_list*)&Arguments);

		LocalFree((HLOCAL)SystemErrorMessage);

		if( Flags == 0 )
			Flags = MB_OK|MB_ICONEXCLAMATION;

		iRet = MsgBox(hWnd,szMsg2,pszCaption,Flags);

		_SafeMemFree(szMsg2);
	}
	else
	{
		PWSTR SystemErrorMessage = NULL;
		_GetSystemErrorMessageEx(Status,&SystemErrorMessage,GetUserDefaultLangID());

		iRet = MsgBox(hWnd,SystemErrorMessage,pszCaption,Flags);

		_FreeSystemErrorMessage(SystemErrorMessage);
	}

	return iRet;
}

EXTERN_C
INT
CDECL
_ErrorPrintfMessageBox(
	HWND hwndOwner,
	LPCWSTR Title,
	LPCWSTR LayoutString,
	LONG code,
	UINT uType,
	LPCWSTR FormatMessage,
	...
	)
{
	int cchMessage = 2048;
	int cch = 0;
	int nRet = 0;
	WCHAR *szMessage = (WCHAR *)LocalAlloc( LPTR, cchMessage * sizeof(WCHAR) );
	if( szMessage == NULL )
		return 0;

	va_list args;
	va_start(args,FormatMessage);

	PWSTR pszTitle;
	PWSTR pszLayout;
	PWSTR pszFormat;
	pszTitle  = _AllocLoadString(Title);
	pszLayout = _AllocLoadString(LayoutString);
	pszFormat = _AllocLoadString(FormatMessage);

	if( pszTitle && pszLayout && pszFormat )
	{
		cch = _vsnwprintf_s(szMessage,cchMessage,_TRUNCATE,pszFormat,args);

		// Layout parameter:
		//  %1 szMessage
		//  %2 system message
		nRet = _ErrorMessageBoxEx2(hwndOwner,pszLayout,pszTitle,szMessage,NULL,code,0,uType);

		va_end(args);
	}

	LocalFree(szMessage);

	_SafeMemFree(pszTitle);
	_SafeMemFree(pszLayout);
	_SafeMemFree(pszFormat);

	return nRet;
}

#if (_WIN32_WINNT >= 0x600)

EXTERN_C
HRESULT
WINAPI
VerificationMessageBox( 
	HWND hwnd,
	HINSTANCE hRes,
	PWSTR pszTitle,
	PWSTR pszMainInstruction,
	PWSTR pszContent,
	PWSTR pszVerificationText,
	TASKDIALOG_BUTTON *buttons,
	int buttonsCount,
	int nDefaultButton,
	int *pSelectedButton,
	int *pVerificationFlag
	)
{
	TASKDIALOGCONFIG tc = {0};
	tc.cbSize              = sizeof(tc);
	tc.dwFlags             = TDF_POSITION_RELATIVE_TO_WINDOW;
	tc.hwndParent          = hwnd;
	tc.hInstance           = hRes;
	tc.pszWindowTitle      = pszTitle;
	tc.pszMainInstruction  = pszMainInstruction;
	tc.pszContent          = pszContent;
	tc.pszVerificationText = pszVerificationText;
	tc.nDefaultButton      = nDefaultButton;
	tc.pButtons            = buttons;
	tc.cButtons            = buttonsCount;

	if( *pVerificationFlag != 0 )
		tc.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;

	return TaskDialogIndirect(&tc, pSelectedButton, NULL, pVerificationFlag);
}

#endif
