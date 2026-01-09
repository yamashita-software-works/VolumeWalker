#pragma once
// 2015.02.04 - Created.
#include "simplevalarray.h"
#include "debug.h"

//
// Long File Path
//

#define CFWSTR_LONGFILEPATHCLIPBOARDFORMAT L"FSLongFilePathClipboard"

ULONG
WINAPI
InitializeLongPathClipboard(
	VOID
	);

BOOL
WINAPI
IsLongFilePathClipboardAvailable(
	VOID
	);

UINT
WINAPI
GetLongFilePathClipboardFormat(
	VOID
	);

#pragma pack(2)
typedef struct _FS_LONGFILEPATH_CLIPBOARD_NAME
{
	ULONG Length;
	ULONG Flags;
	WCHAR Buffer[1];
} FS_LONGFILEPATH_CLIPBOARD_NAME;
#pragma pack()

#define FSLCNF_FILENAME       0x0
#define FSLCNF_DIRECTORY_PATH 0x1
#define FSLCNF_FULLPATH       0x2

class CLongFilePathClipboard
{
	CValArray< FS_LONGFILEPATH_CLIPBOARD_NAME* > m_list;
	HGLOBAL m_hGlobal;
public:
	CLongFilePathClipboard();
	~CLongFilePathClipboard();

	BOOL Start(PCWSTR DirectoryName);
	BOOL AddFile(PCWSTR DirectoryName);
	BOOL AddFullPath(PCWSTR FileFullPath);

	BOOL Commit(HWND hWnd,BOOL bAllocHandle=FALSE);

	HGLOBAL GetHandle() { return m_hGlobal; }

private:
	VOID Clear();
};

//
// LargeDepth LongPath from clipboard
//
class CPopClipboardLongFilePath
{
	HGLOBAL m_hgbl;
	FS_LONGFILEPATH_CLIPBOARD_NAME *m_pTop;
	BOOL m_bClipboard;
public:
	CPopClipboardLongFilePath()
	{
		m_hgbl = NULL;
		m_pTop = NULL;
		m_bClipboard = FALSE;
	}

	FS_LONGFILEPATH_CLIPBOARD_NAME *GetTop()
	{
		ASSERT(m_pTop != NULL);
		return (FS_LONGFILEPATH_CLIPBOARD_NAME *)m_pTop;
	}

	FS_LONGFILEPATH_CLIPBOARD_NAME *Next(FS_LONGFILEPATH_CLIPBOARD_NAME *pName)
	{
		ASSERT(pName != NULL);
		return (FS_LONGFILEPATH_CLIPBOARD_NAME *)(((ULONG_PTR)pName) + pName->Length);
	}

	BOOL Open(HWND hWnd,HGLOBAL hGlobal)
	{
		if( hWnd != NULL )
		{
			if( OpenClipboard(hWnd) )
			{
				m_hgbl = GetClipboardData( GetLongFilePathClipboardFormat() );
				m_pTop = (FS_LONGFILEPATH_CLIPBOARD_NAME *)GlobalLock(m_hgbl);
				m_bClipboard = TRUE;
				return TRUE;
			}
		}
		if( hGlobal != NULL )
		{
			m_hgbl = hGlobal;
			m_pTop = (FS_LONGFILEPATH_CLIPBOARD_NAME *)GlobalLock(m_hgbl);
			return TRUE;
		}
		return FALSE;
	}

	BOOL Close()
	{
		GlobalUnlock(m_hgbl);
		if( m_bClipboard )
			CloseClipboard();
		return TRUE;
	}
};

//
// HDROP from clipboard
//
class CPopClipboardHDROP
{
	HGLOBAL m_hgbl;
	DROPFILES *m_pDropFiles;
	LONG m_cFileCount;
public:
	CPopClipboardHDROP()
	{
		m_hgbl = NULL;
		m_pDropFiles = NULL;
		m_cFileCount = -1;
	}

	PCWSTR GetTop()
	{
		ASSERT(m_hgbl!=NULL);
		DROPFILES *p = (DROPFILES *)GlobalLock(m_hgbl);
		if( p != NULL )
		{		
			return (PCWSTR)(((ULONG_PTR)p) + p->pFiles);
		}
		return NULL;
	}

	PCWSTR Next(PCWSTR psz)
	{
		ASSERT(psz!=NULL);
		return (PCWSTR)(psz + (wcslen(psz)+1));
	}

	BOOL Open(HWND hWnd)
	{
		if( OpenClipboard(hWnd) )
		{
			m_hgbl = GetClipboardData(CF_HDROP);
			m_pDropFiles = (DROPFILES *)GlobalLock(m_hgbl);
			return TRUE;
		}
		return FALSE;
	}

	BOOL Close()
	{
		GlobalUnlock(m_hgbl);
		CloseClipboard();
		return TRUE;
	}

	ULONG GetFileCount()
	{
		if( m_cFileCount != -1 )
			return m_cFileCount;
		if( m_hgbl == NULL )
			return 0;

		DROPFILES *p = (DROPFILES *)GlobalLock(m_hgbl);
		if( p != NULL )
		{
			m_cFileCount = 0;
			PWSTR pszName =	(PWSTR)(((ULONG_PTR)p) + p->pFiles);
			while( *pszName )
			{
				 m_cFileCount++;

				pszName += (wcslen(pszName) + 1);
			}
			GlobalUnlock(m_hgbl);
		}
		return m_cFileCount;
	}
};

//
// CFSTR_SHELLIDLIST from clipboard
//

// This format identifier is used when transferring the locations of one or more existing
// namespace objects. It is used in much the same way as CF_HDROP, but it contains PIDLs 
// instead of file system paths. Using PIDLs allows the CFSTR_SHELLIDLIST format to handle 
// virtual objects as well as file system objects. The data is an STGMEDIUM structure that
// contains a global memory object. The structure's hGlobal member points to a CIDA structure.
// 
// The aoffset member of the CIDA structure is an array containing offsets to the beginning of
// the ITEMIDLIST structure for each PIDL that is being transferred. To extract a particular PIDL,
// first determine its index. Then, add the aoffset value that corresponds to that index to 
// the address of the CIDA structure.
//
// The first element of aoffset contains an offset to the fully qualified PIDL of a parent folder. 
// If this PIDL is empty, the parent folder is the desktop. Each of the remaining elements of
// the array contains an offset to one of the PIDLs to be transferred. All of these PIDLs
// are relative to the PIDL of the parent folder.
// 

// The following two macros can be used to retrieve PIDLs from a CIDA structure. 
// The first takes a pointer to the structure and retrieves the PIDL of the parent folder. 
// The second takes a pointer to the structure and retrieves one of the other PIDLs, 
// identified by its zero-based index.
//
// Note:  The value that is returned by these macros is a pointer to the PIDL's 
// ITEMIDLIST structure. Since these structures vary in length, you must determine 
// the end of the structure by walking through each of the ITEMIDLIST structure's 
// SHITEMID structures until you reach the two-byte NULL that marks the end. 
//
#define _HIDA_GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define _HIDA_GetPIDLItem(pida,i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])
/*
typedef struct _IDA {
  UINT cidl;
  UINT aoffset[1];
} CIDA, *LPIDA;
*/
class CPopClipboardSHELLIDLIST
{


};


//
// NtFileDateTime
//

#define CFWSTR_FILEDATETIMECLIPBOARDFORMAT L"FSNTFileDateTime"

ULONG
WINAPI
InitializeNtFileDateTimeClipboard(
	VOID
	);

BOOL
WINAPI
IsNtFileDateTimeClipboardAvailable(
	VOID
	);

UINT
WINAPI
GetNtFileDateTimeClipboardFormat(
	VOID
	);

#pragma pack(4)
typedef struct _FS_FILE_DATETIME_CLIPBOARD
{
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
} FS_FILE_DATETIME_CLIPBOARD;
#pragma pack()
