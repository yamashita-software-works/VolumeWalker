//****************************************************************************
//
// xmlapplications.h
//
// Author:  YAMASHITA Katshhiro
//
// Hisroty: 2017-06-19 18:04 Created.
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#pragma once 

#include "interface.h"
#include "simplevalarray.h"
#include "xmldochelp.h"

//////////////////////////////////////////////////////////////////////////////
// Applications Reader

typedef struct _FS_APPLICATION_ITEM
{
	ULONG Type;
	GUID Guid; /* unused */
	PCWSTR FriendlyName;
	PCWSTR CommandLine;
	PCWSTR IconPath;
	PCWSTR ExecPath;
	PCWSTR Directory;
	PCWSTR AppPath;
	PCWSTR Parameters;
	PCWSTR AppendPath;
} FS_APPLICATION_ITEM;

class CFSApplocationItem : public FS_APPLICATION_ITEM
{
public:
	CFSApplocationItem()
	{
		Type = 0;
		ZeroMemory(&Guid,sizeof(Guid));
		FriendlyName = NULL;
		CommandLine = NULL;
		IconPath = NULL;
		ExecPath = NULL;
		Directory = NULL;
		AppPath = NULL;
		Parameters = NULL;
		AppendPath = NULL;
	}

	~CFSApplocationItem()
	{
		_SafeMemFree(FriendlyName);
		_SafeMemFree(CommandLine);
		_SafeMemFree(IconPath);
		_SafeMemFree(ExecPath);
		_SafeMemFree(Directory);
		_SafeMemFree(AppPath);
		_SafeMemFree(Parameters);
		_SafeMemFree(AppendPath);
	}

	VOID SetFriendlyName(PCWSTR psz)
	{
		_SafeMemFree(FriendlyName);
		if( psz != NULL )
			FriendlyName = _MemAllocString(psz);
	}

	VOID SetCommandLine(PCWSTR psz)
	{
#if 0
		_SafeMemFree(CommandLine);
		_SafeMemFree(ExecPath);
		if( psz != NULL )
		{
			CommandLine = _MemAllocString(psz);

			CStringBuffer buf(MAX_PATH);
			if( buf.BufferPointer() != NULL  )
			{
				ExpandEnvironmentStrings(psz,buf,MAX_PATH);

				PathRemoveArgs(buf);
				PathUnquoteSpaces(buf);

				ExecPath = _MemAllocString(buf);
			}
		}
#else
		_SafeMemFree(CommandLine);
		if( psz != NULL )
			CommandLine = _MemAllocString(psz);
#endif
	}

	VOID SetIconPath(PCWSTR psz)
	{
		_SafeMemFree(IconPath);
		if( psz != NULL )
			IconPath = _MemAllocString(psz);
	}

	VOID SetStartupDirectory(PCWSTR psz)
	{
		_SafeMemFree(Directory);
		if( psz != NULL )
			Directory = _MemAllocString(psz);
	}

	VOID SetAppPath(PCWSTR psz)
	{
		_SafeMemFree(AppPath);
		if( psz != NULL )
			AppPath = _MemAllocString(psz);
	}

	VOID SetParameters(PCWSTR psz)
	{
		_SafeMemFree(Parameters);
		if( psz != NULL )
			Parameters = _MemAllocString(psz);
	}

	VOID SetAppendPath(PCWSTR psz)
	{
		_SafeMemFree(AppendPath);
		if( psz != NULL )
			AppendPath = _MemAllocString(psz);
	}

	VOID MakeExecPath()
	{
		_SafeMemFree(ExecPath);

		CStringBuffer buf(MAX_PATH);
		if( buf.BufferPointer() == NULL )
			return ;

		//
		// Command line is precedence.
		//
		if( CommandLine != NULL )
		{
			ExpandEnvironmentStrings(CommandLine,buf,MAX_PATH);

			PathRemoveArgs(buf);
			PathUnquoteSpaces(buf);

			ExecPath = _MemAllocString(buf);
		}
		else if( AppPath != NULL )
		{
			if( PathIsFileSpec(AppPath) )
			{
				DWORD cchStartupDir = MAX_PATH * 2;
				WCHAR *StartupDir = new WCHAR[cchStartupDir];
				if( StartupDir )
				{
					// Find out from "App Paths" registration name
					if( GetPathFromAppPaths(AppPath,buf,MAX_PATH,TRUE,StartupDir,cchStartupDir,TRUE) == 0 )
					{
						// Find out from $PATH
						StringCchCopy(buf,MAX_PATH,AppPath);
						PathFindOnPath(buf,NULL);
					}
					else
					{
						// If startup directory not specified then does set.
						if( Directory == NULL )
						{
							Directory = _MemAllocString(StartupDir);
						}
					}
					delete[] StartupDir;
				}
			}
			else
			{
				ExpandEnvironmentStrings(AppPath,buf,MAX_PATH);
				PathUnquoteSpaces(buf);
			}

			ExecPath = _MemAllocString(buf);
		}
	}

	VOID MakeCommandLine()
	{
		_SafeMemFree(CommandLine);

		CStringBuffer buf(MAX_PATH);
		if( buf.BufferPointer() == NULL )
			return ;

		WCHAR *parameters = new WCHAR[MAX_PATH];

		memset(parameters,0,sizeof(MAX_PATH)*sizeof(WCHAR));

		if( Parameters )
		{
			ExpandEnvironmentStrings(Parameters,parameters,MAX_PATH);
		}

		if( ExecPath )
		{
			if( *parameters != 0 )
				StringCchPrintf(buf,MAX_PATH,L"\"%s\" %s",ExecPath,parameters);
			else
				StringCchPrintf(buf,MAX_PATH,L"\"%s\"",ExecPath);
		}

		if( parameters )
			delete[] parameters;

		CommandLine = _MemAllocString(buf);
	}

	PCWSTR PtrFriendlyName() const { return FriendlyName;	}
	PCWSTR PtrCommandLine() const { return CommandLine;	}
	PCWSTR PtrIconPath() const { return IconPath;	}
	PCWSTR PtrExecPath()  const { return ExecPath; }
	PCWSTR PtrStartupDirectory() const { return Directory; }
	PCWSTR PtrAppPath() const { return AppPath; }
	PCWSTR PtrParameters() const { return Parameters; }
	PCWSTR PtrAppendPath() const { return AppendPath; }
};

class CApplocationPointerArray : protected CValArray<CFSApplocationItem *>
{
public:
	CApplocationPointerArray()
	{
	}

	~CApplocationPointerArray()
	{
		clear();
		RemoveAll();
	}

	int size()
	{
		return GetSize();
	}


	CFSApplocationItem *operator[](int iIndex) const
	{
		return (GetBuffer())[iIndex];
	}

	int add(CFSApplocationItem *pItem)
	{
		return Add(pItem);
	}

	void clear()
	{
		int i,c;
		c = GetSize();
		for(i = 0; i < c; i++)
		{
			delete (*this)[i];
		}
	}
};

class CXMLApplicationsReader : public IApplicationsReader
{
	CApplocationPointerArray m_apps;

	XML_ERROR_INFO m_xmlerr;

public:
	ULONG __stdcall AddRef()
	{
		return 0;
	}

	ULONG __stdcall Release()
	{
		delete this;
		return 0;
	}

	HRESULT GetItemCount(ULONG *pulCount)
	{
		*pulCount = (ULONG)m_apps.size();
		return S_OK;
	}

	HRESULT GetCommandLine(ULONG ulIndex,PWSTR pszCommandLine,int cchCommandLine) const
	{
		if( m_apps[(int)ulIndex]->PtrCommandLine() == NULL )
		{
			*pszCommandLine = L'\0';
			return S_FALSE;
		}
		return StringCchCopy(pszCommandLine,cchCommandLine,m_apps[(int)ulIndex]->PtrCommandLine());
	}

	HRESULT GetFriendlyName(ULONG ulIndex,PWSTR pszFriendlyName, int cchFriendlyName) const
	{
		if( m_apps[(int)ulIndex]->PtrFriendlyName() == NULL )
		{
			*pszFriendlyName = L'\0';
			return S_FALSE;
		}
		return StringCchCopy(pszFriendlyName,cchFriendlyName,m_apps[(int)ulIndex]->PtrFriendlyName());
	}

	HRESULT GetIconPath(ULONG ulIndex,PWSTR pszIconPath,int cchIconPath) const
	{
		if( m_apps[(int)ulIndex]->PtrIconPath() == NULL )
		{
			*pszIconPath = L'\0';
			return S_FALSE;
		}
		return StringCchCopy(pszIconPath,cchIconPath,m_apps[(int)ulIndex]->PtrIconPath());
	}

	HRESULT GetExecPath(ULONG ulIndex,PWSTR pszExecPath,int cchExecPath) const
	{
		if( m_apps[(int)ulIndex]->PtrExecPath() == NULL )
		{
			*pszExecPath = L'\0';
			return S_FALSE;
		}
		return StringCchCopy(pszExecPath,cchExecPath,m_apps[(int)ulIndex]->PtrExecPath());
	}

	HRESULT GetStartupDirectory(ULONG ulIndex,PWSTR pszDirectory,int cchDirectory) const
	{
		if( m_apps[(int)ulIndex]->PtrStartupDirectory() == NULL )
		{
			*pszDirectory = L'\0';
			return S_FALSE;
		}
		return StringCchCopy(pszDirectory,cchDirectory,m_apps[(int)ulIndex]->PtrStartupDirectory());
	}

	HRESULT GetAppendPath(ULONG ulIndex,PWSTR pszAppendPath,int cchAppendPath) const
	{
		if( m_apps[(int)ulIndex]->PtrAppendPath() == NULL )
		{
			*pszAppendPath = L'\0';
			return S_FALSE;
		}
		return StringCchCopy(pszAppendPath,cchAppendPath,m_apps[(int)ulIndex]->PtrAppendPath());
	}

	HRESULT GetType(ULONG ulIndex,PULONG pulType) const
	{
		*pulType = m_apps[ulIndex]->Type;
		return S_OK;
	}

	HRESULT IsSeparator(ULONG ulIndex) const
	{
		return m_apps[ulIndex]->Type == 0 ? S_FALSE : S_OK;
	}

public:
	CXMLApplicationsReader()
	{
		memset(&m_xmlerr,0,sizeof(m_xmlerr));
	}

	~CXMLApplicationsReader()
	{
	}

	HRESULT _GetAttributeBSTR(IXMLDOMNamedNodeMap *pnodemap,PWSTR TagName,BSTR *psbResult)
	{
		HRESULT hr = E_FAIL;
		IXMLDOMNode *pnattr = NULL;

		BSTR bsTag = SysAllocString( TagName );

		if( pnodemap->getNamedItem(bsTag,&pnattr) == S_OK )
		{
			hr = pnattr->get_text(psbResult);
			pnattr->Release();
			hr = S_OK;
		}

		SysFreeString(bsTag);

		return hr;
	}

	HRESULT Load(PCWSTR pszFilename)
	{
		HRESULT hr;

		ATLASSERT(pszFilename != NULL);

		if( pszFilename == NULL )
		{
			return E_INVALIDARG;
		}

		// Initialize XML parser
		//
		IXMLDOMDocument *pXMLParseDoc = NULL;
		hr = CoCreateInstance(CLSID_DOMDocument,NULL,CLSCTX_INPROC_SERVER,
						IID_IXMLDOMDocument,(LPVOID *)&pXMLParseDoc);

		if( FAILED(hr) )
		{
			return (DWORD)hr;
		}

		CComVariant varFile;
		varFile = pszFilename;

		VARIANT_BOOL vbSuccessful = 0;

		hr = pXMLParseDoc->load(varFile,&vbSuccessful);

		if( hr != S_OK )
		{
			LONG line,linePos;
			CComBSTR reason;
			IXMLDOMParseError *pParsingErr = NULL;
			memset(&m_xmlerr,0,sizeof(m_xmlerr));
			hr = pXMLParseDoc->get_parseError(&pParsingErr);
			if( pParsingErr )
			{
				pParsingErr->get_line(&line);
				pParsingErr->get_linepos(&linePos);
				pParsingErr->get_reason(&reason);
				pParsingErr->get_errorCode(&hr);
				pParsingErr->Release();

				m_xmlerr.Line = line;
				m_xmlerr.LinePos = linePos;
				m_xmlerr.hr = hr;
				StringCchCopy(m_xmlerr.szReason,_countof(m_xmlerr.szReason),reason);
			}
			else
			{
				m_xmlerr.hr = hr;
			}
		}
		else
		{
			IXMLDOMNode *nodeRoot = NULL;
			IXMLDOMNode *nodeChild = NULL;
			IXMLDOMNode *nodeNext = NULL;

			// to select applications root "Applications" node.
			hr = pXMLParseDoc->selectSingleNode( L"Applications", &nodeRoot );

			if( hr == S_OK )
			{
				// loop on "Application" nodes
				hr = nodeRoot->get_firstChild( &nodeChild  );

				while( hr == S_OK )
				{
					BSTR s;
					nodeChild->get_nodeName( &s );

					if( _wcsicmp(s,L"Application") == 0 )
					{
/*++
						IXMLDOMNamedNodeMap *attr;
						if( nodeChild->get_attributes( &attr ) == S_OK )
						{
							CComBSTR bsPath;
							hr = _GetAttributeBSTR(attr,L"WinVer",&bsPath);
							if( hr == S_OK )
							{
								nWinVer = _wtoi(bsPath.m_str);
							}
							attr->Release();
						}
--*/
						GetApplicaionInformation(nodeChild);
					}
					else if( _wcsicmp(s,L"Separator") == 0 )
					{
						InsertSeparator();
					}

					hr = nodeChild->get_nextSibling( &nodeNext );
					nodeChild->Release();

					nodeChild = nodeNext;
				}

				nodeRoot->Release();
				hr = S_OK;
			}
		}

		pXMLParseDoc->Release();

		return hr;
	}
protected:
	HRESULT GetApplicaionInformation( IXMLDOMNode *nodeApp )
	{
		HRESULT hr;
		IXMLDOMNode *node = NULL;
#if 0
		hr = nodeApp->selectSingleNode( L"CommandLine", &node );
		if( hr != S_OK )
		{
			return S_FALSE;
		}

		CComBSTR CommandLine;
		hr = node->get_text( &CommandLine );
		node->Release();

		if( hr != S_OK )
		{
			return S_FALSE;
		}
#else
		CComBSTR CommandLine;
		CComBSTR AppPath;

		node = NULL;

		hr = nodeApp->selectSingleNode( L"CommandLine", &node );
		if( hr == S_OK )
		{
			hr = node->get_text( &CommandLine );
			node->Release();

			if( hr != S_OK )
			{
				return S_FALSE;
			}
		}
		else
		{
			hr = nodeApp->selectSingleNode( L"AppPath", &node );
			if( hr == S_OK )
			{
				hr = node->get_text( &AppPath );
				node->Release();

				if( hr != S_OK )
				{
					return S_FALSE;
				}
			}
		}
#endif

		CFSApplocationItem *pItem = new CFSApplocationItem;
		if( pItem == NULL )
		{
			return E_OUTOFMEMORY;
		}

		pItem->SetCommandLine( CommandLine );

		hr = nodeApp->selectSingleNode( L"FriendlyName", &node );
		if( hr == S_OK )
		{
			CComBSTR FriendlyName;
			if( node->get_text( &FriendlyName ) == S_OK )
				pItem->SetFriendlyName( FriendlyName );
			node->Release();
		}

		hr = nodeApp->selectSingleNode( L"IconPath", &node );
		if( hr == S_OK )
		{
			CComBSTR IconPath;
			if( node->get_text( &IconPath ) == S_OK )
				pItem->SetIconPath( IconPath );
			node->Release();
		}

		hr = nodeApp->selectSingleNode( L"StartupDirectory", &node );
		if( hr == S_OK )
		{
			CComBSTR CurDir;
			if( node->get_text( &CurDir ) == S_OK )
				pItem->SetStartupDirectory( CurDir );
			node->Release();
		}

		hr = nodeApp->selectSingleNode( L"AppPath", &node );
		if( hr == S_OK )
		{
			CComBSTR CurDir;
			if( node->get_text( &CurDir ) == S_OK )
				pItem->SetAppPath( CurDir );
			node->Release();
		}

		hr = nodeApp->selectSingleNode( L"Parameters", &node );
		if( hr == S_OK )
		{
			CComBSTR CurDir;
			if( node->get_text( &CurDir ) == S_OK )
				pItem->SetParameters( CurDir );
			node->Release();
		}

		hr = nodeApp->selectSingleNode( L"AppendPath", &node );
		if( hr == S_OK )
		{
			CComBSTR AppendPath;
			if( node->get_text( &AppendPath ) == S_OK )
				pItem->SetAppendPath( AppendPath );
			node->Release();
		}

		pItem->MakeExecPath();

		if( pItem->PtrCommandLine() == NULL )
			pItem->MakeCommandLine();

		m_apps.add( pItem );

		return S_OK;
	}

	HRESULT InsertSeparator()
	{
		CFSApplocationItem *pItem = new CFSApplocationItem;
		if( pItem == NULL )
		{
			return E_OUTOFMEMORY;
		}

		pItem->Type = 0x1;

		m_apps.add( pItem );

		return S_OK;
	}
};

#if _ENABLE_EXTERNAL_TOOLS

//
// External Toolsmenu implement
//
class CExternalToolsMenu : public IExternalTools
{
	LONG m_ref;
	HIMAGELIST m_himlSmall;
	HIMAGELIST m_himlNormal;
	PWSTR m_pszDefFile;

	typedef struct _EXTERNAL_TOOLS_MENU_ITEM
	{
		PWSTR DisplayName;
		PWSTR Path;
		PWSTR Directory;
		int indexImage;
		BOOL bSeparator;
	} EXTERNAL_TOOLS_MENU_ITEM;

	CValArray<EXTERNAL_TOOLS_MENU_ITEM> m_menuItem;

public:
	CExternalToolsMenu()
	{
		m_himlSmall = NULL;
		m_himlNormal = NULL;
		m_pszDefFile = NULL;
		m_ref = 0;
	}

	~CExternalToolsMenu()
	{
		clearImageList();
		clearAllItems();
		ImageList_Destroy(m_himlSmall);
		ImageList_Destroy(m_himlNormal);
		_SafeMemFree(m_pszDefFile);
	}

	BOOL Load(PCWSTR pszFilename)
	{
		if( m_pszDefFile == NULL )
		{
			WCHAR szPath[MAX_PATH];

			if( PathFileExists(szPath) )
			{
				m_pszDefFile = _MemAllocString(szPath);
			}
			else
			{
#if 0
				// locale specified
				// "<exelocation>\config\<LangId>\"
				WCHAR szLcid[8+8+1];
#if 0
				StringCchPrintf(szLcid,_countof(szLcid),L"config\\%u",_GetLangId());
#else
				StringCchPrintf(szLcid,_countof(szLcid),L"%u",_GetLangId());
#endif
				GetModuleFileName(NULL,szPath,MAX_PATH);
				PathRemoveFileSpec(szPath);
				PathCombine(szPath,szPath,szLcid);
				PathCombine(szPath,szPath,pszFilename);
				if( PathFileExists(szPath) )
				{
					m_pszDefFile = _MemAllocString(szPath);
				}
				else
				{
					// config directory
					// "<exelocation>\config"
					GetModuleFileName(NULL,szPath,MAX_PATH);
					PathRemoveFileSpec(szPath);
					PathCombine(szPath,szPath,L"config");
					PathCombine(szPath,szPath,pszFilename);
					if( PathFileExists(szPath) )
					{
						m_pszDefFile = _MemAllocString(szPath);
					}
				}
#endif
			}
		}

		//
		// External tools cannot be used.
		//
		if( m_pszDefFile == NULL )
			return FALSE;

		//
		// Create imagelist for external tools
		//
		if( m_himlSmall == NULL )
		{
			m_himlSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CXSMICON),ILC_COLOR32|ILC_MASK,16,8);
		}

		if( m_himlNormal == NULL )
		{
			m_himlNormal = ImageList_Create(GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CXICON),ILC_COLOR32|ILC_MASK,16,8);
		}

		//
		// Create external tool list
		//
		createToolList();

		return TRUE;
	}

protected:
	VOID clearImageList()
	{
		ImageList_RemoveAll(m_himlSmall);
		ImageList_RemoveAll(m_himlNormal);
	}

	VOID clearAllItems()
	{
		int i,c;
		c = m_menuItem.GetSize();
		for(i = 0; i < c; i++)
		{
			EXTERNAL_TOOLS_MENU_ITEM& mdmi = m_menuItem[i];
			_SafeMemFree(mdmi.DisplayName);
			_SafeMemFree(mdmi.Path);
			_SafeMemFree(mdmi.Directory);
		}
		m_menuItem.RemoveAll();
	}

	VOID createToolList()
	{
		clearImageList();

		// Load items
		//
		IApplicationsReader *pApps;

		pApps = static_cast<IApplicationsReader *>(new CXMLApplicationsReader);

		HRESULT hr;
		hr = pApps->Load( m_pszDefFile );
		if( hr != S_OK )
		{
			pApps->Release();
			return;
		}

		ULONG i;
		ULONG c = 0;
		WCHAR szBuffer[MAX_PATH];
		SHFILEINFO si={0};

		pApps->GetItemCount(&c);

		for(i = 0; i < c; i++)
		{
			EXTERNAL_TOOLS_MENU_ITEM mdmi;

			mdmi.bSeparator = (pApps->IsSeparator(i) == S_OK) ? TRUE : FALSE;

			//
			// application name
			//
			hr = pApps->GetFriendlyName(i,szBuffer,MAX_PATH);
			if( hr == S_OK )
			{
				mdmi.DisplayName = _MemAllocString( szBuffer );
			}
			else
			{
				pApps->GetExecPath(i,szBuffer,MAX_PATH);
				mdmi.DisplayName = _MemAllocString( PathFindFileName( szBuffer ) );
			}

			//
			// command line
			//
			pApps->GetCommandLine(i,szBuffer,MAX_PATH);
			mdmi.Path = _MemAllocString( szBuffer  );

			//
			// startup directory
			//
			if( pApps->GetStartupDirectory(i,szBuffer,MAX_PATH) == S_OK )
				mdmi.Directory = _MemAllocString( szBuffer  );
			else
				mdmi.Directory = NULL;

			// app icon 
			mdmi.indexImage = I_IMAGENONE;

			if( pApps->GetIconPath(i,szBuffer,MAX_PATH) == S_OK )
			{
				mdmi.indexImage = GetIcon(szBuffer);
			}

			if( mdmi.indexImage == I_IMAGENONE )
			{
				// exec icon
				if( pApps->GetExecPath(i,szBuffer,MAX_PATH) == S_OK )
				{
					if( SHGetFileInfo( szBuffer, 0, &si, sizeof(si), SHGFI_ICON|SHGFI_SMALLICON|SHGFI_SHELLICONSIZE) )
					{
						mdmi.indexImage = ImageList_AddIcon( m_himlSmall, si.hIcon );

						DestroyIcon(si.hIcon);
					}
					if( SHGetFileInfo( szBuffer, 0, &si, sizeof(si), SHGFI_ICON|SHGFI_SHELLICONSIZE) )
					{
						mdmi.indexImage = ImageList_AddIcon( m_himlNormal, si.hIcon );

						DestroyIcon(si.hIcon);
					}
				}
			}

			m_menuItem.Add(mdmi);
		}

		pApps->Release();
	}

	int GetIcon(PCWSTR pszIconLocation)
	{
		HICON hIcon = NULL;
		HICON hIconSmall = NULL;
		WCHAR szIconPath[MAX_PATH];
		WCHAR szTemp[MAX_PATH];

		SHLoadIndirectString(pszIconLocation,szTemp,MAX_PATH,NULL);
		ExpandEnvironmentStrings(szTemp,szIconPath,MAX_PATH);
		int iIconIndex = PathParseIconLocation(szIconPath);

		if( szIconPath[0] != L'\0' )
		{
			UINT cIcons;
			cIcons = ExtractIconEx(
						szIconPath,
						-1,NULL,NULL,1);

			if( cIcons == 1 )
			{
				ExtractIconEx(
					szIconPath,
					0,&hIcon,&hIconSmall,1);
			}
			else
			{
				ExtractIconEx(
					szIconPath,
					iIconIndex,&hIcon,&hIconSmall,1);
			}
		}

		int iImage;
		if( hIcon || hIconSmall )
		{
			iImage = ImageList_AddIcon( m_himlSmall, hIconSmall );
			DestroyIcon(hIconSmall);

			iImage = ImageList_AddIcon( m_himlNormal, hIcon );
			DestroyIcon(hIcon);
		}
		else
		{
			iImage = I_IMAGENONE;
		}

		return iImage;
	}

	BOOL CheckPath(PCWSTR pszIn,PWSTR pszOut)
	{
		WCHAR szTemp[MAX_PATH];

		StringCchCopy(szTemp,MAX_PATH,pszIn);

		PathRemoveArgs(szTemp);
		PathRemoveBlanks(szTemp);
		PathUnquoteSpaces(szTemp);
		PathIsFileSpec(szTemp);
		PathIsExe(szTemp);

		return TRUE;
	}

public:
	//
	// IExternalTools
	//
	ULONG __stdcall AddRef()
	{
		return ++m_ref;
	}

	ULONG __stdcall Release()
	{
		ASSERT(m_ref != 0);
		ULONG r = --m_ref;
		if( m_ref == 0 )
			delete this;
		return r;
	}

	HRESULT GetCount(PINT pulToolCount) const
	{
		*pulToolCount = m_menuItem.GetSize();
		return S_OK;
	}

	HRESULT GetIcon(INT Index,INT Size,HICON *phIcon) const
	{
		int iIndex = m_menuItem[Index].indexImage;
		if( iIndex >= 0 )
		{
			*phIcon = ImageList_GetIcon( (Size == 32) ? m_himlNormal : m_himlSmall,iIndex,ILD_NORMAL);
			return S_OK;
		}
		return E_FAIL;
	}

	PCWSTR GetName(int Index) const 
	{
		return (PCWSTR)m_menuItem[Index].DisplayName;
	}

	PCWSTR GetPath(int Index) const
	{
		return (PCWSTR)m_menuItem[Index].Path;
	}

	PCWSTR GetStartupDirectory(int Index) const
	{
		return (PCWSTR)m_menuItem[Index].Directory;
	}

	BOOL IsSeparator(int iIndex) const 
	{
		return m_menuItem[iIndex].bSeparator;
	}
};
#endif