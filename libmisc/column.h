#pragma once

#include "dsarray.h"
#include "multisz.h"

typedef struct _COLUMN
{
	UINT id;
	PWSTR Name;
	int iOrder;
	int cx;
	int fmt;
	ULONG field;
} COLUMN;

typedef struct _COLUMN_TABLE
{
	ULONG cItems;
	COLUMN column[1];
} COLUMN_TABLE;

typedef struct _COLUMN_NAME
{
	UINT id;
	PCWSTR Name;
	UINT dummy;
} COLUMN_NAME;

class CColumnList
{
	COLUMN *m_columns;
	int     m_column_count;
	PWSTR   m_inifile_path;

	COLUMN_NAME *m_column_name_map;
	int m_column_name_map_size;

public:
	CColumnList()
	{
		m_columns = NULL;
		m_column_count = 0;
		m_inifile_path = NULL;
		m_column_name_map = NULL;
		m_column_name_map_size = 0;
	}

	~CColumnList()
	{
		_SafeMemFree(m_inifile_path);
		_SafeMemFree(m_column_name_map);
	}

	void SetDefaultColumns(COLUMN *colmums,int column_count)
	{
		m_columns = colmums;
		m_column_count = column_count;
	}

	int GetDefaultColumnCount() const
	{
		return m_column_count;
	}

	VOID SetColumnNameMap(int NamesCount, COLUMN_NAME *ColumnNames)
	{
		m_column_name_map = new COLUMN_NAME[ NamesCount ];
		if( m_column_name_map == NULL )
			return ;
		m_column_name_map_size = NamesCount;
		memcpy(m_column_name_map,ColumnNames,NamesCount * sizeof(COLUMN_NAME));
	}

	int LoadUserDefinitionColumnTable(COLUMN_TABLE **pColTblPtr,LPCWSTR pszSectionName);
	int LoadUserDefinitionColumnTableFromText(COLUMN_TABLE **pColTblPtr,PCWSTR pszText,int cbText);
	int FreeUserDefinitionColumnTable(COLUMN_TABLE *pColTbl);
	const COLUMN *GetDefaultColumnItemFromId(UINT id);
	const COLUMN *GetDefaultColumnItem(int index);
	BOOL SetIniFilePath(PCWSTR Path);
	LARGE_INTEGER GetColumnSortInfo(PCWSTR pszSectionName);
	LARGE_INTEGER GetColumnSortInfoFromText(PCWSTR pszText);
	BOOL SaveColumnTable(COLUMN_TABLE *pColTblPtr,PCWSTR pszSectionName,PCWSTR pszIniFileName,INT idSort,INT sortDirection);
	BOOL MakeColumnString(COLUMN_TABLE *pColTblPtr,INT idSort,INT sortDirection,PWSTR *ppszColumns,PWSTR *ppszSortColumn);
#if 0
	BOOL SaveColumns(HWND hWndList,LPCWSTR SectionName);
#endif
	PCWSTR GetIniFilePath();
private:
	int findColumnItem(UINT id);
	BOOL PaeseLine(PWSTR pszLine,COLUMN *pcol);
	DSArray<COLUMN> *GetColumnLayout(PCWSTR pszSectionName,PCWSTR pszSectionText=NULL,int cbSectionText=0);
	LARGE_INTEGER GetSortInfo(PWSTR sz);

	UINT NameToId(PCWSTR pszName)
	{
		for(int i = 0; i < m_column_name_map_size; i++)
		{
			if( _wcsicmp(pszName,m_column_name_map[i].Name) == 0 )
				return m_column_name_map[i].id;
		}
		return 0;
	}

	PCWSTR IdToName(UINT Id)
	{
		for(int i = 0; i < m_column_name_map_size; i++)
		{
			if( Id == m_column_name_map[i].id )
				return m_column_name_map[i].Name;
		}
		return NULL;
	}
};
