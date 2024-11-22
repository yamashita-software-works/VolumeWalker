//
// https://github.com/ysc3839/win32-darkmode
// 2024-06-24 Modified for Windows 7 WDK/Visual Studio 2010 build environment.
//
#include "stdafx.h"
#include "darkmode.h"

#if _MSC_VER <= 1500
#ifndef nullptr
#define nullptr NULL
#endif
#endif

typedef void (WINAPI *fnRtlGetNtVersionNumbers)(LPDWORD major, LPDWORD minor, LPDWORD build);
typedef BOOL (WINAPI *fnSetWindowCompositionAttribute)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
typedef bool (WINAPI *fnShouldAppsUseDarkMode)();                                             // ordinal 132
typedef bool (WINAPI *fnAllowDarkModeForWindow)(HWND hWnd, bool allow);                       // ordinal 133
typedef bool (WINAPI *fnAllowDarkModeForApp)(bool allow);                                     // ordinal 135, in 1809
typedef void (WINAPI *fnFlushMenuThemes)();                                                   // ordinal 136
typedef void (WINAPI *fnRefreshImmersiveColorPolicyState)();                                  // ordinal 104
typedef bool (WINAPI *fnIsDarkModeAllowedForWindow)(HWND hWnd);                               // ordinal 137
typedef bool (WINAPI *fnGetIsImmersiveColorUsingHighContrast)(IMMERSIVE_HC_CACHE_MODE mode);  // ordinal 106
typedef HTHEME(WINAPI *fnOpenNcThemeData)(HWND hWnd, LPCWSTR pszClassList);                   // ordinal 49
typedef bool (WINAPI *fnShouldSystemUseDarkMode)();                                           // ordinal 138
typedef PreferredAppMode (WINAPI *fnSetPreferredAppMode)(PreferredAppMode appMode);           // ordinal 135, in 1903
typedef bool (WINAPI *fnIsDarkModeAllowedForApp)();                                           // ordinal 139

fnSetWindowCompositionAttribute        _SetWindowCompositionAttribute = nullptr;
fnShouldAppsUseDarkMode                _ShouldAppsUseDarkMode = nullptr;
fnAllowDarkModeForWindow               _AllowDarkModeForWindow = nullptr;
fnAllowDarkModeForApp                  _AllowDarkModeForApp = nullptr;
fnFlushMenuThemes                      _FlushMenuThemes = nullptr;
fnRefreshImmersiveColorPolicyState     _RefreshImmersiveColorPolicyState = nullptr;
fnIsDarkModeAllowedForWindow           _IsDarkModeAllowedForWindow = nullptr;
fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast = nullptr;
fnOpenNcThemeData                      _OpenNcThemeData = nullptr;
fnShouldSystemUseDarkMode              _ShouldSystemUseDarkMode = nullptr;
fnSetPreferredAppMode                  _SetPreferredAppMode = nullptr;

static bool g_darkModeSupported = false;
static bool g_darkModeEnabled = false;
static DWORD g_buildNumber = 0;

bool _IsDarkModeSupported() { return g_darkModeSupported; }

bool AllowDarkModeForWindow(HWND hWnd, bool allow)
{
	if (g_darkModeSupported)
		return _AllowDarkModeForWindow(hWnd, allow);
	return false;
}

bool IsHighContrast()
{
	HIGHCONTRASTW highContrast = { sizeof(highContrast) };
	if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE))
		return highContrast.dwFlags & HCF_HIGHCONTRASTON;
	return false;
}

void RefreshTitleBarThemeColor(HWND hWnd)
{
	BOOL dark = FALSE;
	if (_IsDarkModeAllowedForWindow(hWnd) &&
		_ShouldAppsUseDarkMode() &&
		!IsHighContrast())
	{
		dark = TRUE;
	}
	if (g_buildNumber < 18362)
		SetPropW(hWnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
	else if (_SetWindowCompositionAttribute)
	{
		WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
		_SetWindowCompositionAttribute(hWnd, &data);
	}
}

bool IsColorSchemeChangeMessage(LPARAM lParam)
{
	bool is = false;
	if (lParam && CompareStringOrdinal(reinterpret_cast<LPCWCH>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
	{
		_RefreshImmersiveColorPolicyState();
		is = true;
	}
	_GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
	return is;
}

bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
{
	if (message == WM_SETTINGCHANGE)
		return IsColorSchemeChangeMessage(lParam);
	return false;
}

void AllowDarkModeForApp(bool allow)
{
	if (_AllowDarkModeForApp)
		_AllowDarkModeForApp(allow);
	else if (_SetPreferredAppMode)
		_SetPreferredAppMode(allow ? AllowDark : Default);
}

static HTHEME __stdcall _my_hook(HWND hWnd, LPCWSTR classList)
{
	if (wcscmp(classList, L"ScrollBar") == 0)
	{
		hWnd = nullptr;
		classList = L"Explorer::ScrollBar";
	}
	return _OpenNcThemeData(hWnd, classList);
};

static void FixDarkScrollBar()
{
	HMODULE hComctl = LoadLibraryExW(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hComctl)
	{
		PIMAGE_THUNK_DATA addr = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
		if (addr)
		{
			DWORD oldProtect;
			if (VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
			{
#if 0
				auto MyOpenThemeData = [](HWND hWnd, LPCWSTR classList) -> HTHEME {
					if (wcscmp(classList, L"ScrollBar") == 0)
					{
						hWnd = nullptr;
						classList = L"Explorer::ScrollBar";
					}
					return _OpenNcThemeData(hWnd, classList);
				};
				addr->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(MyOpenThemeData));
				VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
#else
				addr->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(&_my_hook));
				VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
#endif
			}
		}
	}
}

HRESULT InitDarkMode()
{
	HRESULT hr = E_FAIL;
	fnRtlGetNtVersionNumbers RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
	if (RtlGetNtVersionNumbers)
	{
		DWORD major, minor;
		RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
		g_buildNumber &= ~0xF0000000;
		if( major == 10 && minor == 0 )
		{
			HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			if (hUxtheme)
			{
				_OpenNcThemeData = reinterpret_cast<fnOpenNcThemeData>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49)));
				_RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
				_GetIsImmersiveColorUsingHighContrast = reinterpret_cast<fnGetIsImmersiveColorUsingHighContrast>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(106)));
				_ShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
				_AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));

				fnSetPreferredAppMode ord135;
				(FARPROC&)ord135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
				if (g_buildNumber < 18362)
					_AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(ord135);
				else
					_SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(ord135);

				_FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
				_IsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));
				_SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

				if (_OpenNcThemeData &&
					_RefreshImmersiveColorPolicyState &&
					_ShouldAppsUseDarkMode &&
					_AllowDarkModeForWindow &&
					(_AllowDarkModeForApp || _SetPreferredAppMode) &&
					_FlushMenuThemes &&
					_IsDarkModeAllowedForWindow)
				{
					g_darkModeSupported = true;
					hr = S_OK;
				}
			}
		}
	}

	return hr;
}

HRESULT EnableDarkMode(BOOL bEnable)
{
	if (_OpenNcThemeData &&
		_RefreshImmersiveColorPolicyState &&
		_ShouldAppsUseDarkMode &&
		_AllowDarkModeForWindow &&
		(_AllowDarkModeForApp || _SetPreferredAppMode) &&
		_FlushMenuThemes &&
		_IsDarkModeAllowedForWindow )
	{
	   	AllowDarkModeForApp(bEnable ? true : false);
		_RefreshImmersiveColorPolicyState();
		g_darkModeEnabled = _ShouldAppsUseDarkMode() && !IsHighContrast();
		FixDarkScrollBar();
	}

	return S_OK;
}

bool _IsDarkModeEnabled()
{
	return g_darkModeEnabled;
}
