//*****************************************************************************
//*                                                                           *
//*  utilnt.cpp                                                               *
//*                                                                           *
//*  NT native API functions                                                  *
//*                                                                           *
//*  Create: 2024-03-21                                                       *
//*                                                                           *
//*  Author: YAMASHITA Katsuhiro                                              *
//*                                                                           *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                  *
//*  Licensed under the MIT License.                                          *
//*                                                                           *
//*****************************************************************************
#include <ntifs.h>
#include <windef.h>
#include <ntstatus.h>
#include <winerror.h>
#include <strsafe.h>
#include <malloc.h>
#include "libntwdk.h"
#include "ntnativeapi.h"
#include "fsfilelib.h"
#include "..\libntwdk\ntnativeapi.h"
#include "..\libntwdk\ntpathcomponent.h"
#include "ntobjecthelp.h"

//---------------------------------------------------------------------------
//
//  NtPathLookupDeviceNameFromPath()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
EXTERN_C
ULONG
APIENTRY
NtPathLookupDeviceNameFromPath(
	HANDLE *phspa,
    PCWSTR Path,
	ULONG Flags
    )
{
    HANDLE hObjDir;
    LONG Status;
    ULONG Index = 0;
    UNICODE_STRING usNtDeviceName;
	WCHAR NtDeviceName[WIN32_MAX_PATH];
	WCHAR DosDeviceName[WIN32_MAX_PATH];
	UNICODE_STRING usPath;

	if( phspa == NULL || Path == NULL )
		return NtStatusToDosError(STATUS_INVALID_PARAMETER);

	HANDLE hspa = SPtrArray_Create(0);
	if( hspa == NULL )
		return NtStatusToDosError(STATUS_NO_MEMORY);

	RtlInitUnicodeString(&usPath,Path);

    Status = OpenObjectDirectory( 
					(Flags & EDDTNF_LOCAL) ? L"\\??" : L"\\GLOBAL??",
					&hObjDir
					);

    if( Status == STATUS_SUCCESS )
    {
        while( QueryObjectDirectory(hObjDir,&Index,DosDeviceName,WIN32_MAX_PATH,NULL,0) == 0)
        {
			usNtDeviceName.Length = 0;
			usNtDeviceName.MaximumLength = sizeof(NtDeviceName);
			usNtDeviceName.Buffer = NtDeviceName;

			if( QuerySymbolicLinkObject_U(hObjDir,DosDeviceName,&usNtDeviceName,FALSE) == 0 )
			{
				// e.g.
				//
				// path:        "\Device\Foo\Bar\Xxx\Yyy"
				// device name: "\Device\Foo\Bar"
				// -> true
				//
				// path:        "\Device\Foo\Bar\Xxx\Yyy"
				// device name: "\Device\Zzz
				// -> false
				//
				if( usNtDeviceName.Length > 0 )
				{
					if( RtlPrefixUnicodeString(&usNtDeviceName,&usPath,TRUE) )
					{
						PWSTR pwsz = DuplicateString(usNtDeviceName.Buffer);
						if( pwsz )
							SPtrArray_Add(hspa,pwsz);
					}
				}
            }
        }

        CloseObjectDirectory( hObjDir );

		if( SPtrArray_GetCount(hspa) == 0 )
		{
			SPtrArray_Destroy(hspa);
			*phspa = NULL;
			Status = STATUS_OBJECT_NAME_NOT_FOUND;
		}
		else
		{
			*phspa = hspa;
			Status = STATUS_SUCCESS;
		}
    }

    return NtStatusToDosError(Status);
}
