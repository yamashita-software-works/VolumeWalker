#pragma once

#ifdef _WINDOWS
#ifndef _WINTERNL_
typedef struct _UNICODE_STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
#endif

#define NTAPI __stdcall
#define NTSTATUS LONG

EXTERN_C
ULONG
NTAPI
RtlNtStatusToDosError(
    IN NTSTATUS  Status
	); 

#endif
