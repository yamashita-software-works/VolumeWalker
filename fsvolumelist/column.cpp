//***************************************************************************
//*                                                                         *
//*  column.cpp                                                             *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2023-02-18                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "column.h"

static PWSTR g_pszFileName = L""; // reserved

typedef struct _COLUMN_NAME
{
	UINT id;
	PCWSTR Name;
	UINT dummy;
} COLUMN_NAME;

static COLUMN_NAME column_name_map[] = {
	{ COLUMN_Name,           L"Name",           0 },
	{ COLUMN_CreationTime,   L"CreationTime",   0 }, 
	{ COLUMN_Size,           L"Size",           0 }, 
	{ COLUMN_Free,           L"Free",           0 }, 
	{ COLUMN_Usage,          L"Useage",         0 }, 
	{ COLUMN_UsageRate,      L"UsageRate",      0 }, 
	{ COLUMN_Format,         L"Format",         0 }, 
	{ COLUMN_Guid,           L"Guid",           0 }, 
	{ COLUMN_Drive,          L"Drive",          0 }, 
	{ COLUMN_VendorId,       L"VendorId",       0 }, 
	{ COLUMN_ProductId,      L"ProductId",      0 }, 
	{ COLUMN_PartitionStyle, L"PartitionStyle", 0 }, 
	{ COLUMN_BusType,        L"BusType",        0 }, 
	{ COLUMN_DeviceId,       L"DeviceId",       0 }, 
	{ COLUMN_Identifier,     L"Identifier",     0 }, 
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

DSArray<COLUMN> *CColumnList::GetColumnLayout()
{
	PCWSTR pszSectionName = L"ColumnLayout";

	WCHAR szIniFileName[MAX_PATH];
	WCHAR szExeFilePath[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL),szExeFilePath,MAX_PATH);
	PathRemoveExtension(szExeFilePath);
	PathAddExtension(szExeFilePath,L".ini");
	if( PathFileExists(szExeFilePath) )
	{
		StringCchCopy(szIniFileName,MAX_PATH,szExeFilePath);
	}
	else
	{
		return NULL;
	}

	DWORD cch = 32768;
	PWSTR buf = new WCHAR[cch+1];
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

	return pdsa;
}

int CColumnList::LoadUserDefinitionColumnTable(COLUMN_TABLE **pColTblPtr)
{
	int cItems = 0;

	*pColTblPtr = NULL;

	DSArray<COLUMN> *pc = GetColumnLayout();

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

int SaveColumnTable(COLUMN_TABLE *pColTblPtr,PCWSTR pszSectionName,PCWSTR)
{
	ULONG i;
	PCWSTR pszName;
	WCHAR szInfo[100];
	CMultiSz msz;

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

#ifdef _DEBUG
{
	PCWSTR psz;
	psz = msz.GetTop();

	while( psz && *psz )
	{
		_TRACE("%S\n",psz);
		msz.Next(&psz);
	}
}
#endif;

	if( pszSectionName && *pszSectionName != 0 )
	{
		WritePrivateProfileSection(pszSectionName,msz.GetTop(),g_pszFileName);
	}

	return 0;
}

BOOL SaveColumns(HWND hWndList,LPCWSTR SectionName)
{
	int cColumns = ListViewEx_GetColumnCount(hWndList);
	COLUMN_TABLE *pcoltbl = (COLUMN_TABLE *)_MemAllocZero(sizeof(COLUMN_TABLE) + sizeof(COLUMN) * cColumns);

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

	SaveColumnTable(pcoltbl,SectionName,NULL);

	_MemFree(pcoltbl);

	return TRUE;
}
