#pragma once

inline int __cdecl DebugPrintfW( WCHAR *Format, ... )
{
	const int _MAX_DEBUG_PRINTF_LENGTH = 1024;
	va_list args;
	va_start(args,Format);
	SIZE_T cch=0;
	PWSTR format = NULL;
	{
		size_t cchMessage = _MAX_DEBUG_PRINTF_LENGTH;
		size_t cbMessageBuffer = cchMessage * sizeof(WCHAR);
		WCHAR *pszMessage;
		pszMessage = (WCHAR *)malloc(cbMessageBuffer);
		if( pszMessage )
		{
			cch = _vsnwprintf_s(pszMessage,cchMessage,_TRUNCATE,Format,args);

			if( cch > 0 )
			{
				OutputDebugStringW(pszMessage);
			}

			free(pszMessage);
		}
	}
	va_end(args);

	return (int)cch;
}
