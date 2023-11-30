#pragma once

#include "targetver.h"

#undef WIN32_NO_STATUS        // Defines STATUS_XXX in ntddk.
#include <ntstatus.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS       // Does not defines STATUS_XXX in winnt.h
#include <windows.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <virtdisk.h>
#include <winioctl.h>
#include <cfgmgr32.h>
#include <shlobj.h>
#include <winternl.h> // WinSDK 7.1

#define NTSTATUS LONG

#include "mem.h"
