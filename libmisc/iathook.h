// This file contains code from
// https://github.com/stevemk14ebr/PolyHook_2_0/blob/master/sources/IatHook.cpp
// which is licensed under the MIT License.
// See PolyHook_2_0-LICENSE for more information.
//
// 2024-06-24 Modified for Windows 7 WDK/Visual Studio 2010 build environment.
//
#pragma once

#if _MSC_VER <= 1500
#ifndef nullptr
#define nullptr NULL
#endif
#endif

#ifndef DELAYLOAD_IMPORTS_DEFINED
#include "pshpack4.h"

typedef struct _IMAGE_DELAYLOAD_DESCRIPTOR {
    union {
        DWORD AllAttributes;
        struct {
            DWORD RvaBased : 1;             // Delay load version 2
            DWORD ReservedAttributes : 31;
        } DUMMYSTRUCTNAME;
    } Attributes;

    DWORD DllNameRVA;                       // RVA to the name of the target library (NULL-terminate ASCII string)
    DWORD ModuleHandleRVA;                  // RVA to the HMODULE caching location (PHMODULE)
    DWORD ImportAddressTableRVA;            // RVA to the start of the IAT (PIMAGE_THUNK_DATA)
    DWORD ImportNameTableRVA;               // RVA to the start of the name table (PIMAGE_THUNK_DATA::AddressOfData)
    DWORD BoundImportAddressTableRVA;       // RVA to an optional bound IAT
    DWORD UnloadInformationTableRVA;        // RVA to an optional unload info table
    DWORD TimeDateStamp;                    // 0 if not bound,
                                            // Otherwise, date/time of the target DLL

} IMAGE_DELAYLOAD_DESCRIPTOR, *PIMAGE_DELAYLOAD_DESCRIPTOR;

typedef const IMAGE_DELAYLOAD_DESCRIPTOR *PCIMAGE_DELAYLOAD_DESCRIPTOR;

#include "poppack.h"
#endif

template <typename T, typename T1, typename T2>
T RVA2VA(T1 base, T2 rva)
{
	return reinterpret_cast<T>(reinterpret_cast<ULONG_PTR>(base) + rva);
}

template <typename T>
T DataDirectoryFromModuleBase(void *moduleBase, size_t entryID)
{
	PIMAGE_DOS_HEADER dosHdr = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleBase);
	PIMAGE_NT_HEADERS ntHdr = RVA2VA<PIMAGE_NT_HEADERS>(moduleBase, dosHdr->e_lfanew);
	PIMAGE_DATA_DIRECTORY dataDir = ntHdr->OptionalHeader.DataDirectory;
	return RVA2VA<T>(moduleBase, dataDir[entryID].VirtualAddress);
}

__inline PIMAGE_THUNK_DATA FindAddressByName(void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, const char *funcName)
{
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal))
			continue;

		PIMAGE_IMPORT_BY_NAME import = RVA2VA<PIMAGE_IMPORT_BY_NAME>(moduleBase, impName->u1.AddressOfData);
		if ((char *)strcmp((char *)import->Name, funcName) != 0)
			continue;
		return impAddr;
	}
	return nullptr;
}

__inline PIMAGE_THUNK_DATA FindAddressByOrdinal(void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, USHORT ordinal)
{
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal)
			return impAddr;
	}
	return nullptr;
}

__inline PIMAGE_THUNK_DATA FindIatThunkInModule(void *moduleBase, const char *dllName, const char *funcName)
{
	PIMAGE_IMPORT_DESCRIPTOR imports = DataDirectoryFromModuleBase<PIMAGE_IMPORT_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_IMPORT);
	for (; imports->Name; ++imports)
	{
		if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->Name), dllName) != 0)
			continue;

		PIMAGE_THUNK_DATA origThunk = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->OriginalFirstThunk);
		PIMAGE_THUNK_DATA thunk = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->FirstThunk);
		return FindAddressByName(moduleBase, origThunk, thunk, funcName);
	}
	return nullptr;
}

__inline PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void *moduleBase, const char *dllName, const char *funcName)
{
	PIMAGE_DELAYLOAD_DESCRIPTOR imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for (; imports->DllNameRVA; ++imports)
	{
		if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;

		PIMAGE_THUNK_DATA impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
		PIMAGE_THUNK_DATA impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByName(moduleBase, impName, impAddr, funcName);
	}
	return nullptr;
}

__inline PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void *moduleBase, const char *dllName, USHORT ordinal)
{
	PIMAGE_DELAYLOAD_DESCRIPTOR imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for (; imports->DllNameRVA; ++imports)
	{
		if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;

		PIMAGE_THUNK_DATA impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
		PIMAGE_THUNK_DATA impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByOrdinal(moduleBase, impName, impAddr, ordinal);
	}
	return nullptr;
}
