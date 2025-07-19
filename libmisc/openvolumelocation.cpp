//***************************************************************************
//*                                                                         *
//*  openvolumelocation.cpp                                                 *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2024-08-08,2024-12-12                                          *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "libmisc.h"
#include "runhelp.h"

HRESULT
WINAPI
OpenVolumeLocationByShell(
	HWND hWnd,
	UINT Open,
	PCWSTR pszDosDrive,
	PCWSTR pszVolumeGuid
	)
{
    WCHAR szVolume[MAX_PATH];
    HRESULT hr;
    BOOL bAdmin = FALSE;

    if( Open & OpenVolumeLocationWithAdmin )
    {
        bAdmin = TRUE;
        Open &= ~OpenVolumeLocationWithAdmin;
    }

    switch( Open )
    {
        case 0:
        {
            szVolume[0] = L'\0';

            if( pszDosDrive == NULL || *pszDosDrive == L'\0' )
            {
                if( pszVolumeGuid != NULL && *pszVolumeGuid != L'\0' )
                    hr = StringCchPrintf(szVolume,MAX_PATH,L"\\\\?\\%s\\",pszVolumeGuid);
                else
                    hr = HRESULT_FROM_WIN32(ERROR_NO_VOLUME_ID);
            }
            else if( pszDosDrive != NULL || *pszDosDrive != L'\0' )
            {
				hr = StringCchCopy(szVolume,MAX_PATH,pszDosDrive);
				PathAddBackslash(szVolume);
            }
            else
            {
                hr = E_INVALIDARG;
            }

            if( hr == S_OK )
            {
                SHELLEXECUTEINFO sei = {0};
                sei.cbSize = sizeof(sei);
                sei.fMask = 0;
                sei.lpFile = szVolume;
                sei.lpDirectory = NULL;
                sei.lpParameters = NULL;
                sei.lpVerb = L"open";
                sei.nShow  = SW_SHOWNORMAL;
                if( ShellExecuteEx( &sei ) )
                    hr = S_OK;
                else
                    hr = HRESULT_FROM_WIN32( GetLastError() );
            }
            break;
        }
        case 1:
        {
            CRunHelp run;
            if(pszDosDrive && *pszDosDrive != L'\0' )
                hr = run.RunCommandPrompt(pszDosDrive, bAdmin );
            else
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE);
            break;
        }
        case 2:
        {
            CRunHelp run;
            if(pszDosDrive && *pszDosDrive != L'\0' )
                hr = run.RunPowerShell(pszDosDrive, bAdmin );
            else
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE);
            break;
        }
        case 3:
        {
            if(pszDosDrive && *pszDosDrive != L'\0' )
                hr = OpenTerminal( NULL,pszDosDrive );
            else
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE);
            break;
        }
        case 4:
        {
            if(pszDosDrive && *pszDosDrive != L'\0' )
            {
                WCHAR szBashPath[MAX_PATH];
                if( GetBashExePath(szBashPath,MAX_PATH) )
                {
                    SHELLEXECUTEINFO sei = {0};
                    sei.cbSize = sizeof(sei);
                    sei.lpFile = szBashPath;
                    sei.lpDirectory = pszDosDrive;
                    sei.lpParameters = NULL;
                    sei.lpVerb = L"open";
                    sei.hwnd = hWnd;
                    sei.nShow  = SW_SHOWNORMAL;
                    if( ShellExecuteEx( &sei ) )
                        hr = S_OK;
                    else
                        hr = HRESULT_FROM_WIN32( GetLastError() );
                }
                else
                {
                    hr = E_NOTIMPL;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE);
            }
            break;
        }
    }

    if( hr != S_OK )
    {
        WCHAR buf[MAX_PATH];
        StringCchPrintf(buf,MAX_PATH,L"0x%08X",hr);
        _ErrorMessageBoxEx(hWnd,0,L"Open Location",L"",HRESULT_CODE(hr),MB_OK|MB_ICONEXCLAMATION);
    }

    return hr;
}
