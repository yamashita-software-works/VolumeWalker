#pragma once
//
// Minifilter driver manager helper
//
#include <fltuser.h>  // for FltMgr

EXTERN_C
PCSTR
WINAPI
GetFilterFileSystemTypeString(
	INT Type
	);

EXTERN_C
HRESULT
WINAPI
FindFirstVolumeInstance(
	PCWSTR pszVolumeName,
	INSTANCE_INFORMATION_CLASS dwInformationClass,
	LPVOID *lpReturnedBuffer,
	LPHANDLE lpVolumeInstanceFind
	);

EXTERN_C
HRESULT
WINAPI
FindNextVolumeInstance(
	HANDLE hVolumeInstanceFind,
	INSTANCE_INFORMATION_CLASS dwInformationClass,
	LPVOID *lpReturnedBuffer
	);
