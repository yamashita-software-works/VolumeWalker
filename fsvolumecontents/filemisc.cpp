//***************************************************************************
//*                                                                         *
//*  filemisc.cpp                                                           *
//*                                                                         *
//*  File Miscellaneous Functions                                           *
//*                                                                         *
//*  Create: 2024-02-04                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include "stdafx.h"
#include "..\libntwdk\libntwdk.h"
#include "..\libntwdk\ntnativeapi.h"
#include "..\libntwdk\ntobjecthelp.h"
#include "filemisc.h"

//---------------------------------------------------------------------------
//
//  GetAttributeString()
//
//  PURPOSE:
//
//---------------------------------------------------------------------------
static struct {
	DWORD AttributeFlag;
	TCHAR AttributeChar;
} attributes_char[] = {
	{FILE_ATTRIBUTE_DIRECTORY,            L'd'},
	{FILE_ATTRIBUTE_READONLY,             L'r'},
	{FILE_ATTRIBUTE_HIDDEN,               L'h'},
	{FILE_ATTRIBUTE_SYSTEM,               L's'},
	{FILE_ATTRIBUTE_ARCHIVE,              L'a'},
	{FILE_ATTRIBUTE_ENCRYPTED,            L'e'},
	{FILE_ATTRIBUTE_COMPRESSED,           L'c'},
	{FILE_ATTRIBUTE_REPARSE_POINT,        L'l'},
	{FILE_ATTRIBUTE_SPARSE_FILE,          L'p'},
	{FILE_ATTRIBUTE_TEMPORARY,            L't'},
	{FILE_ATTRIBUTE_VIRTUAL,              L'v'},
	{FILE_ATTRIBUTE_OFFLINE,              L'o'},
	{FILE_ATTRIBUTE_DEVICE,               L'D'}, // 'x'
	{FILE_ATTRIBUTE_NORMAL,               L'n'},
	{FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,  L'i'},
// Win8/Windows Server 2012
	{FILE_ATTRIBUTE_NO_SCRUB_DATA,        L'X'}, // 'u'->'X'
	{FILE_ATTRIBUTE_INTEGRITY_STREAM,     L'V'}, // 'g'->'V'
// Win10
	{FILE_ATTRIBUTE_EA,                   L'E'},
	{FILE_ATTRIBUTE_PINNED,               L'P'},
	{FILE_ATTRIBUTE_UNPINNED,             L'U'},
	{FILE_ATTRIBUTE_RECALL_ON_OPEN,       L'R'},
	{FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS,L'D'},
	{FILE_ATTRIBUTE_STRICTLY_SEQUENTIAL,  L'Q'},
};

EXTERN_C
BOOL
APIENTRY
GetAttributeString(
	DWORD Attributes,
	LPWSTR String,
	int cchString
	)
{
	int i,c;

	c = ARRAYSIZE(attributes_char);

	for(i = 0; i < c; i++)
	{
		if( Attributes & attributes_char[i].AttributeFlag )
		{
			*String++ = attributes_char[i].AttributeChar;
		}
		else
		{
			//*String++ = _T('-');
		}
	}

	*String = L'\0';

	return TRUE;
}
