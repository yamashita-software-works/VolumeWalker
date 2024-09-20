#include "stdafx.h"

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

static DWORD dwOSVersion = 0;

DWORD GetOSVersion()
{
	return dwOSVersion;
}

VOID InitOSVersion()
{
	OSVERSIONINFO osver = {0};
	osver.dwOSVersionInfoSize = sizeof(osver);
	GetVersionEx(&osver);
	dwOSVersion = ((osver.dwMajorVersion << 8) | (osver.dwMinorVersion));
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		InitOSVersion();
		_MemInit();
		break;
	case DLL_PROCESS_DETACH:
		_MemEnd();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
