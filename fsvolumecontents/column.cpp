//****************************************************************************
//
//  column.cpp
//
//  listview column manager.
//
//  Auther: YAMASHITA Katsuhiro
//
//  Create: 2023.02.28
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "column.h"

BOOL CColumnList::SetIniFilePath(PCWSTR Path)
{
	_SafeMemFree(m_inifile_path);
	m_inifile_path = _MemAllocString(Path);
	return TRUE;
}

typedef struct _COLUMN_NAME
{
	UINT id;
	PCWSTR Name;
	UINT dummy;
} COLUMN_NAME;

static COLUMN_NAME column_name_map[] = {
	{ COLUMN_Name,           L"Name",                  0},
	{ COLUMN_Extension,      L"Extension",             0},
	{ COLUMN_FileAttributes, L"Attributes",            0},
	{ COLUMN_EndOfFile,      L"EndOfFile",             0},
	{ COLUMN_AllocationSize, L"AllocationSize",        0},
	{ COLUMN_LastWriteTime,  L"LastWriteTime",         0},
	{ COLUMN_CreationTime,   L"CreationTime",          0},
	{ COLUMN_LastAccessTime, L"LastAccessTime",        0},
	{ COLUMN_ChangeTime,     L"ChangeTime",            0},
	{ COLUMN_EaSize,         L"EaSize",                0},
	{ COLUMN_FileId,         L"FileId",                0},
	{ COLUMN_FileIndex,      L"FileIndex",             0},
	{ COLUMN_ShortName,      L"ShortName",             0},
	{ COLUMN_Extension,      L"Extention",             0},
	{ COLUMN_Path,           L"Path",                  0},
	{ COLUMN_Usn,            L"Usn",                   0},
	{ COLUMN_Date,           L"Date",                  0},
	{ COLUMN_Reason,         L"Reason",                0},
	{ COLUMN_Frn,            L"FRN",                   0},
	{ COLUMN_ParentFrn,      L"Parent FRN",            0},
	{ COLUMN_Path,           L"Path",                  0},

};

static UINT NameToId(PCWSTR pszName)
{
	for(int i = 0; i < ARRAYSIZE(column_name_map); i++)
	{
		if( _wcsicmp(pszName,column_name_map[i].Name) == 0 )
			return column_name_map[i].id;
	}
	return 0;
}

static PCWSTR IdToName(UINT Id)
{
	for(int i = 0; i < ARRAYSIZE(column_name_map); i++)
	{
		if( Id == column_name_map[i].id )
			return column_name_map[i].Name;
	}
	return NULL;
}

int CColumnList::findColumnItem(UINT id)
{
	for(int i = 0; i < GetDefaultColumnCount(); i++)
	{
		if(m_columns[i].id == id)
		{
			return i;
		}
	}
	return -1;
}

const COLUMN *CColumnList::GetDefaultColumnItemFromId(UINT id)
{
	int index = findColumnItem(id);
	if( index != -1 )
	{
		return &m_columns[index];
	}
	return NULL;
}

const COLUMN *CColumnList::GetDefaultColumnItem(int index)
{
	if( 0 <= index && index < GetDefaultColumnCount() )
		return &m_columns[index];
	return NULL;
}

BOOL CColumnList::PaeseLine(PWSTR pszLine,COLUMN *pcol)
{
	wchar_t *delim = L",";
	wchar_t *context;
	wchar_t *tok;
	wchar_t *sep;
	UINT id = 0;

	pcol->id = -1;

	PWSTR pLine = _MemAllocString(pszLine);

	sep = wcschr(pLine,L'=');
	if( sep )
	{
		*sep = L'\0';

		StrTrim(pLine,L" \t");

		id = NameToId(pLine);

		if( id != 0 )
		{
			const COLUMN *pdef = GetDefaultColumnItemFromId(id);

			if( pdef )
			{
				pcol->id     = pdef->id;
				pcol->Name   = pdef->Name;
				pcol->cx     = pdef->cx;
				pcol->fmt    = pdef->fmt;
				pcol->iOrder = pdef->iOrder;

				// =width[,reserved]
				tok = wcstok_s(++sep,delim,&context);

				if( tok != NULL )
				{
					pcol->cx = wcstoul(tok,NULL,10);

					while( tok )
					{	
						tok = wcstok_s(NULL,delim,&context); // reserved
					}
				}
			}
		}
	}

	_MemFree(pLine);

	return (id != 0) && (pcol->id != -1);
}

DSArray<COLUMN> *CColumnList::GetColumnLayout(PCWSTR pszSectionName)
{
	WCHAR szIniFileName[MAX_PATH];
	if( m_inifile_path != NULL )
	{
		StringCchCopy(szIniFileName,MAX_PATH,m_inifile_path);
	}
	else
	{
		WCHAR szTemp[MAX_PATH];
		GetModuleFileName(GetModuleHandle(NULL),szTemp,MAX_PATH);

		PathRemoveExtension(szTemp);
		PathAddExtension(szTemp,L".ini");
		StringCchCopy(szIniFileName,MAX_PATH,szTemp);
	}

	if( !PathFileExists(szIniFileName) )
	{
		return NULL;
	}

	DWORD cch = 32768;
	PWSTR buf = new WCHAR[cch];
	DWORD ret;

	ret = GetPrivateProfileSection(pszSectionName,buf,cch,szIniFileName);
	if( ret == 0 )
	{
		delete[] buf;
		return NULL;
	}

	DSArray<COLUMN> *pdsa = new DSArray<COLUMN>;

	pdsa->Create();

	PWSTR p = buf;
	COLUMN col = {0};
	int iOrder = 0;

	while( *p )
	{
		if( PaeseLine(p,&col) )
		{
			COLUMN d;
			int i,cItems = pdsa->GetCount();
			for(i = 0; i < cItems; i++)
			{
				if( pdsa->GetItem(i,&d) )
				{
					if( d.id == col.id )
					{
						break; // already has in array.
					}
				}
			}

			if( i == cItems )
			{
				col.iOrder = iOrder++;
				pdsa->Add(&col);
			}
		}

		p += (wcslen(p) + 1);
	}

	delete[] buf;

	return pdsa;
}

int CColumnList::LoadUserDefinitionColumnTable(COLUMN_TABLE **pColTblPtr,PCWSTR pszSectionName)
{
	int cItems = 0;

	*pColTblPtr = NULL;

	DSArray<COLUMN> *pc = GetColumnLayout(pszSectionName);

	if( pc )
	{
		cItems = pc->GetCount();

		if( cItems > 0 )
		{
			COLUMN_TABLE *pcoltbl = (COLUMN_TABLE *)_MemAllocZero( sizeof(COLUMN_TABLE) + ((cItems - 1) * sizeof(COLUMN)) );

			pcoltbl->cItems = cItems;

			for(UINT i = 0; i < pcoltbl->cItems; i++)
			{
				pc->GetItem(i,&pcoltbl->column[i]);
			}

			*pColTblPtr = pcoltbl;
		}

		delete pc;
	}

	return cItems;
}

int CColumnList::FreeUserDefinitionColumnTable(COLUMN_TABLE *pColTbl)
{
	_SafeMemFree(pColTbl);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
//  Column Save Functions

BOOL SaveColumnTable(COLUMN_TABLE *pColTblPtr,PCWSTR pszSectionName,PCWSTR pszIniFileName)
{
	BOOL bSuccess = FALSE;
	ULONG i;
	PCWSTR pszName;
	WCHAR szInfo[100];
	CMultiSz msz;

	if( pszIniFileName != NULL && *pszIniFileName != L'\0' )
	{
		for(i = 0; i < pColTblPtr->cItems; i++)
		{
			pszName = IdToName(pColTblPtr->column[i].id);

			if( pszName )
			{
				COLUMN *pcol = &pColTblPtr->column[i];

				StringCchPrintf(szInfo,ARRAYSIZE(szInfo),L"%s=%d",pszName,pcol->cx);

				msz.Add(szInfo);
			}
		}

		if( pszSectionName && *pszSectionName != L'\0' )
		{
			if( WritePrivateProfileSection(pszSectionName,msz.GetTop(),pszIniFileName) )
			{
				bSuccess = TRUE;
			}
		}
	}

	return bSuccess;
}

BOOL SaveColumns(HWND hWndList,LPCWSTR SectionName)
{
	BOOL bSuccess;

	int cColumns = ListViewEx_GetColumnCount(hWndList);
	if( cColumns == 0 )
	{
		return FALSE;
	}

	COLUMN_TABLE *pcoltbl = (COLUMN_TABLE *)_MemAllocZero(sizeof(COLUMN_TABLE) + sizeof(COLUMN) * cColumns);
	if( pcoltbl == NULL )
		return FALSE;

	pcoltbl->cItems = cColumns;

	LVCOLUMN lvc = {0};
	lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_ORDER;

	ULONG i;
	for(i = 0; i < pcoltbl->cItems; i++)
	{
		ListView_GetColumn(hWndList,i,&lvc);

		pcoltbl->column[i].cx = lvc.cx;
		pcoltbl->column[i].iOrder = lvc.iOrder;
		pcoltbl->column[i].id = (int)ListViewEx_GetHeaderItemData(hWndList,i);
	}

	bSuccess = SaveColumnTable(pcoltbl,SectionName,NULL);

	_MemFree(pcoltbl);

	return bSuccess;
}
