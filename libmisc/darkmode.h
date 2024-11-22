//
// https://github.com/ysc3839/win32-darkmode
// 2024-06-24 Modified for Windows 7 WDK/Visual Studio 2010 build environment.
//
#pragma once
#include "IatHook.h"

#define LOAD_LIBRARY_SEARCH_SYSTEM32 (0x00000800)

enum IMMERSIVE_HC_CACHE_MODE
{
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
};

enum PreferredAppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

enum WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
};

typedef void (WINAPI *fnRtlGetNtVersionNumbers)(LPDWORD major, LPDWORD minor, LPDWORD build);
typedef BOOL (WINAPI *fnSetWindowCompositionAttribute)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
typedef bool (WINAPI *fnShouldAppsUseDarkMode)();                                                 // ordinal 132
typedef bool (WINAPI *fnAllowDarkModeForWindow)(HWND hWnd, bool allow);                           // ordinal 133
typedef bool (WINAPI *fnAllowDarkModeForApp)(bool allow);                                         // ordinal 135, in 1809
typedef void (WINAPI *fnFlushMenuThemes)();                                                       // ordinal 136
typedef void (WINAPI *fnRefreshImmersiveColorPolicyState)();                                      // ordinal 104
typedef bool (WINAPI *fnIsDarkModeAllowedForWindow)(HWND hWnd);                                   // ordinal 137
typedef bool (WINAPI *fnGetIsImmersiveColorUsingHighContrast)(IMMERSIVE_HC_CACHE_MODE mode);      // ordinal 106
typedef HTHEME(WINAPI *fnOpenNcThemeData)(HWND hWnd, LPCWSTR pszClassList);                       // ordinal 49
typedef bool (WINAPI *fnShouldSystemUseDarkMode)();                                               // ordinal 138
typedef PreferredAppMode (WINAPI *fnSetPreferredAppMode)(PreferredAppMode appMode);               // ordinal 135, in 1903
typedef bool (WINAPI *fnIsDarkModeAllowedForApp)();                                               // ordinal 139

bool AllowDarkModeForWindow(HWND hWnd, bool allow);
void RefreshTitleBarThemeColor(HWND hWnd);
bool IsHighContrast();
bool IsColorSchemeChangeMessage(LPARAM lParam);
bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam);
void AllowDarkModeForApp(bool allow);
void FixDarkScrollBar();
bool _IsDarkModeSupported();
bool _IsDarkModeEnabled();
HRESULT InitDarkMode();
HRESULT EnableDarkMode(BOOL bEnable);
