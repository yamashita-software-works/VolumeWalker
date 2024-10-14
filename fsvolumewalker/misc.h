// 2024-09-16
#pragma once

LONG WINAPI GetComputerInformation(ULONG InfoClass, UINT Flags, PVOID Buffer, ULONG BufferLength);

#define CIF_REG_WOW64_64KEY  (0x1)
#define CIF_REG_WOW64_32KEY  (0x2)

HICON GetAppropriateVolumeIconByPath(PCWSTR pszPath);
