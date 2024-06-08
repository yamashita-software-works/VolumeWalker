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
} COLUMN;

typedef struct _COLUMN_TABLE
{
	ULONG cItems;
	COLUMN column[1];
} COLUMN_TABLE;

enum {
	COLUMN_None=0,
    COLUMN_Name,
	COLUMN_CreationTime,
	COLUMN_Size,
	COLUMN_Free,
	COLUMN_Usage,
	COLUMN_UsageRate,
	COLUMN_Format,
	COLUMN_Guid,
	COLUMN_Drive,
	COLUMN_VendorId,
	COLUMN_ProductId,
	COLUMN_PartitionStyle,
	COLUMN_BusType,
	COLUMN_DeviceId,
	COLUMN_Identifier,
	COLUMN_OriginalDevice,
	COLUMN_OriginalVolume,
	COLUMN_SnapshotId,
	COLUMN_SnapshotSetId,
	COLUMN_Attributes,
	COLUMN_VolumeLabel,
	COLUMN_Path,
	COLUMN_Type,
	COLUMN_FileSystemType,
	COLUMN_Flags,
	COLUMN_Address,
	COLUMN_Offset,
	COLUMN_DumpHex,
	COLUMN_DumpChar,
	COLUMN_FilterName,
	COLUMN_FilterInstanceName,
	COLUMN_FilterAltitude,
	COLUMN_FilterVolumeName,
	COLUMN_FilterFrameId,
	COLUMN_InstallDate,
	COLUMN_FirstInstallDate,
	COLUMN_LastArrivalDate,
	COLUMN_LastRemovalDate,
	COLUMN_MaxItem,
	COLUMN_MaxCount=COLUMN_MaxItem,
};

BOOL SaveColumnTable(COLUMN_TABLE *pColTblPtr,PCWSTR,PCWSTR);
BOOL SaveColumns(HWND hWndList,LPCWSTR SectionName);

class CColumnList
{
	COLUMN *m_columns;
	int     m_column_count;
	PWSTR   m_inifile_path;

public:
	CColumnList()
	{
		m_columns = NULL;
		m_column_count = 0;
		m_inifile_path = NULL;
	}

	~CColumnList()
	{
		_SafeMemFree(m_inifile_path);
	}

	void SetDefaultColumns(COLUMN *colmums,int column_count)
	{
		m_columns = colmums;
		m_column_count = column_count;
	}

	int GetDefaultColumnCount()
	{
		return m_column_count;
	}

	int LoadUserDefinitionColumnTable(COLUMN_TABLE **pColTblPtr,LPCWSTR pszSectionName);
	int FreeUserDefinitionColumnTable(COLUMN_TABLE *pColTbl);
	const COLUMN *GetDefaultColumnItemFromId(UINT id);
	const COLUMN *GetDefaultColumnItem(int index);
	BOOL SetIniFilePath(PCWSTR Path);
private:
	int findColumnItem(UINT id);
	BOOL PaeseLine(PWSTR pszLine,COLUMN *pcol);
	DSArray<COLUMN> *GetColumnLayout(PCWSTR pszSectionName);
};
