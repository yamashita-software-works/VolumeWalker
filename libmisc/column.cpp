//****************************************************************************
//
//  column.cpp
//
//  listview column manager.
//
//  Author: YAMASHITA Katsuhiro
//
//  Create: 2023-02-18
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
	if( Path )
		m_inifile_path = _MemAllocString(Path);
	return TRUE;
}

PCWSTR CColumnList::GetIniFilePath()
{
	if( m_inifile_path != NULL )
		return m_inifile_path;
#if 0 // pending
	WCHAR szTemp[MAX_PATH];
	GetModuleFileName(GetModuleHandle(NULL),szTemp,MAX_PATH);

	WCHAR szIniFileName[MAX_PATH];
	PathRemoveExtension(szTemp);
	PathAddExtension(szTemp,L".ini");
	StringCchCopy(szIniFileName,MAX_PATH,szTemp);

	if( !PathFileExists(szIniFileName) )
	{
		return NULL;
	}

	SetIniFilePath(szIniFileName);
#endif
	return m_inifile_path;
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
				pcol->field  = pdef->field;
				pcol->iOrder = pdef->iOrder;

				// =width[,columnfieldtype]
				tok = wcstok_s(++sep,delim,&context);

				if( tok != NULL )
				{
					pcol->cx = wcstoul(tok,NULL,10);

					while( tok )
					{	
						tok = wcstok_s(NULL,delim,&context); // ",field"
						if( tok )
							pcol->field = wcstoul(tok,NULL,10);
					}
				}
			}
		}
	}

	_MemFree(pLine);

	return (id != 0) && (pcol->id != -1);
}

DSArray<COLUMN> *CColumnList::GetColumnLayout(PCWSTR pszSectionName,PCWSTR pszSectionText,int cbSectionText)
{
	DWORD cch = 32768;
	PWSTR buf = new WCHAR[cch];
	DWORD ret = 0;
	PCWSTR szIniFileName;

	if( pszSectionName != NULL )
	{
		szIniFileName = GetIniFilePath();
		if( szIniFileName == NULL || *szIniFileName == L'\0' )
		{
			delete[] buf;
			return NULL;
		}
		ret = GetPrivateProfileSection(pszSectionName,buf,cch,szIniFileName);
	}
	else
	{
		if( pszSectionText != NULL && cbSectionText != 0 )
		{
			memcpy(buf,pszSectionText,cbSectionText);
			ret = cbSectionText/sizeof(WCHAR);
		}
	}

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

LARGE_INTEGER CColumnList::GetSortInfo(PWSTR sz)
{
	LARGE_INTEGER liRet = {0,0};
	PWSTR pSep;
	pSep = wcschr(sz,L',');
	if( pSep )
	{
		*pSep = L'\0';

		if( *(++pSep) != L'\0' )
		{
			UINT id  = NameToId(sz);
			if( id != 0 )
			{
				int iDirection = _wtoi(pSep);
#if 0
				if( iDirection == -1 || iDirection == 0 || iDirection == 1 )
				{
					liRet.HighPart = iDirection;
					liRet.LowPart = id;
				}
#else
				if( iDirection == 0 || iDirection == 1 )
				{
					switch( iDirection )
					{
						case 0: liRet.HighPart = 1; break;
						case 1: liRet.HighPart = -1; break;
					}
					liRet.LowPart  = id;
				}
#endif
			}
		}
	}

	return liRet;
}

LARGE_INTEGER CColumnList::GetColumnSortInfo(PCWSTR pszSectionName)
{
	PCWSTR szIniFileName;
	WCHAR sz[MAX_PATH];
	LARGE_INTEGER liRet = {0};

	szIniFileName = GetIniFilePath();

	GetPrivateProfileString(pszSectionName,L"SortColumn",L"",sz,MAX_PATH,szIniFileName);
	return GetSortInfo(sz);
}

LARGE_INTEGER CColumnList::GetColumnSortInfoFromText(PCWSTR pszText)
{
	WCHAR sz[MAX_PATH];
	StringCchCopy(sz,MAX_PATH,pszText);
	return GetSortInfo(sz);
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

int CColumnList::LoadUserDefinitionColumnTableFromText(COLUMN_TABLE **pColTblPtr,PCWSTR pszText,int cbText)
{
	int cItems = 0;

	*pColTblPtr = NULL;

	DSArray<COLUMN> *pc = GetColumnLayout(NULL,pszText,cbText);

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

BOOL CColumnList::SaveColumnTable(COLUMN_TABLE *pColTblPtr,PCWSTR pszSectionName,PCWSTR pszIniFileName,INT idSort,INT sortDirection)
{
	BOOL bSuccess = FALSE;
	ULONG i;
	PCWSTR pszName;
	WCHAR szInfo[100];
	CMultiSz msz;

	if( pszIniFileName == NULL || *pszIniFileName == L'\0' )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	try
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
			if( WritePrivateProfileSection(pszSectionName,msz.GetTop(),pszIniFileName) == 0 )
			{
				throw GetLastError();
			}
		}

		if( idSort != -1 )
		{
			pszName = IdToName(idSort);
		
			if( pszName )
			{
				StringCchPrintf(szInfo,ARRAYSIZE(szInfo),L"%s,%d",pszName,sortDirection);

				if( WritePrivateProfileString(pszSectionName,L"SortColumn",szInfo,pszIniFileName) == 0 )
				{
					throw GetLastError();
				}
			}
		}

		bSuccess = TRUE;
	}
	catch( DWORD err )
	{
		SetLastError(err);
	}

	return bSuccess;
}

BOOL CColumnList::MakeColumnString(COLUMN_TABLE *pColTblPtr,INT idSort,INT sortDirection,PWSTR *ppszColumns,PWSTR *ppszSortColumn)
{
	BOOL bSuccess = FALSE;
	ULONG i;
	PCWSTR pszName;
	WCHAR szInfo[100];
	CMultiSz msz;

	try
	{
		if( ppszColumns )
		{
			for(i = 0; i < pColTblPtr->cItems; i++)
			{
				pszName = IdToName(pColTblPtr->column[i].id);
	
				if( pszName )
				{
					COLUMN *pcol = &pColTblPtr->column[i];
	
					if( pcol->field )
						StringCchPrintf(szInfo,ARRAYSIZE(szInfo),L"%s=%d,%d",pszName,pcol->cx,pcol->field);
					else
						StringCchPrintf(szInfo,ARRAYSIZE(szInfo),L"%s=%d",pszName,pcol->cx);
	
					msz.Add(szInfo);
				}
			}
	
			PWSTR pszBuf = (PWSTR)CoTaskMemAlloc( msz.GetBufferSize() );
			if( pszBuf == NULL )
				throw ERROR_NOT_ENOUGH_MEMORY;

			ZeroMemory(pszBuf,msz.GetBufferSize());
	
			PCWSTR psz = msz.GetTop();
			if( psz && *psz )
			{
				for(;;)
				{
					StringCbCat(pszBuf,msz.GetBufferSize(),psz);
					if( msz.Next(&psz) == FALSE )
						break;
					StringCbCat(pszBuf,msz.GetBufferSize(),L";");
				}
			}
	
			*ppszColumns = pszBuf;
		}

		if( idSort != -1 && ppszSortColumn )
		{
			*ppszSortColumn = L'\0';

			pszName = IdToName(idSort);
		
			if( pszName )
			{
				if( StringCchPrintf(szInfo,ARRAYSIZE(szInfo),L"%s,%d",pszName,sortDirection) == S_OK )
				{
					SIZE_T cch = (wcslen(szInfo) + 1);
					*ppszSortColumn = (PWSTR)CoTaskMemAlloc( cch * sizeof(WCHAR) );
					if( *ppszSortColumn == NULL )
						throw ERROR_NOT_ENOUGH_MEMORY;
					StringCchCopy(*ppszSortColumn,cch,szInfo);
				}
			}
		}

		bSuccess = TRUE;
	}
	catch( LONG err )
	{
		SetLastError(err);
	}

	return bSuccess;
}

#if 0
BOOL CColumnList::SaveColumns(HWND hWndList,LPCWSTR SectionName)
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

	bSuccess = SaveColumnTable(pcoltbl,SectionName,GetIniFilePath(),-1,-1);

	_MemFree(pcoltbl);

	return bSuccess;
}
#endif

int ColumnList_GetMszColumnStringSizeCch(PCWSTR pmsz)
{
	int cchTotal = 0;
	int cch = 0;
	PCWSTR p = pmsz;
	while( *p )
	{
		cch = ((int)wcslen(p) + 1);
		p += cch;
		cchTotal += cch;
	}
	cchTotal++; // double terminate null
	return cchTotal;
}

int ColumnList_GetMszColumnStringSizeCb(PCWSTR pmsz)
{
	int cch = ColumnList_GetMszColumnStringSizeCch(pmsz);
	return (cch * sizeof(WCHAR));
}

PWSTR ColumnList_MszColumnStringReplaceChar(PWSTR psz)
{
	PWSTR p = psz;
	while( *p )
	{
		if( *p == L'|' ||  *p == L';' )
		{
			*p = L'\0';	
		}
		p++;
	}
	return psz;
}

PWSTR ColumnList_ConvertAllocSzToMszColumnString(PCWSTR psz)
{
	SIZE_T cch = wcslen(psz);
	PWSTR pszAlloc = (PWSTR)_MemAllocZero( (cch + 1) * sizeof(WCHAR) ); // double terminate null
	if( pszAlloc == NULL )
		return NULL;
	memcpy(pszAlloc,psz,cch*sizeof(WCHAR));
	return ColumnList_MszColumnStringReplaceChar(pszAlloc);
}

BOOL ColumnList_FreeString(PWSTR psz)
{
	if( psz == NULL )
		return FALSE;
	_MemFree(psz);
	return TRUE;
}
