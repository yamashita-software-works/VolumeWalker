//****************************************************************************
//
// XML Document Helper Functions
//
// Author:  YAMASHITA Katsuhiro
//
// History: 2020-06-08 Created.
//
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#pragma once

inline IXMLDOMNode *_XMLAddRootNode(IXMLDOMDocument *pDoc,PCWSTR NodeName,PCWSTR /*Version*/)
{
	VARIANT vtTemp;
	vtTemp.vt   = VT_I2;
	vtTemp.iVal = NODE_ELEMENT;

	IXMLDOMNode *pNode;
	pDoc->createNode(vtTemp, (BSTR)NodeName, L"", &pNode);
	pDoc->appendChild(pNode,NULL);

#if 0
	if( Version )
	{
		IXMLDOMElement*  pXMLElement;
		if( SUCCEEDED(pDoc->get_documentElement(&pXMLElement)) )
		{
			CComBSTR bstrAttr(L"Version");
			CComVariant varAttr;
			varAttr = Version;
			pXMLElement->setAttribute(bstrAttr,varAttr);
			pXMLElement->Release();
		}
	}
#endif
	return pNode;
}

inline IXMLDOMNode *_XMLAddNode(IXMLDOMDocument *pDoc,IXMLDOMNode *pParent,PCWSTR NodeName)
{
	VARIANT vtTemp;
	vtTemp.vt   = VT_I2;
	vtTemp.iVal = NODE_ELEMENT;

	BSTR bstrNodeName = SysAllocString( NodeName );
	IXMLDOMNode *pNode;
	pDoc->createNode(vtTemp, bstrNodeName, L"", &pNode);
	pParent->appendChild(pNode,NULL);
	SysFreeString(bstrNodeName);

	return pNode;
}

inline IXMLDOMElement *_XMLAddElement(IXMLDOMDocument *pDoc,IXMLDOMNode *pNode,PCWSTR pszTagName,PCWSTR pszBuffer)
{
	IXMLDOMElement *pElement = NULL;

	CComBSTR bstrTag( pszTagName );
	CComBSTR bstrString( pszBuffer );

	if( SUCCEEDED(pDoc->createElement(bstrTag,&pElement)) )
	{
		pElement->put_text(bstrString);

		pNode->appendChild(pElement,NULL);
	}

	return pElement;
}

inline void _XMLAddString(IXMLDOMDocument *pDoc,IXMLDOMNode *pNode,WCHAR *psz)
{
	IXMLDOMText *pText;

	if( SUCCEEDED(pDoc->createTextNode(psz,&pText)) )
	{
		pNode->appendChild(pText,NULL);
		pText->Release();
	}
}

inline BOOL _XMLSetElementLine(IXMLDOMDocument *pDoc,IXMLDOMNode *pNode,PCWSTR pszTagName,PCWSTR pszBuffer,int nIndentTab,BOOL NewLine)
{
	BOOL bSuccess = FALSE;

	while( nIndentTab-- )
		_XMLAddString(pDoc,pNode,L"\t");

	IXMLDOMElement *pElement = NULL;
	CComBSTR bstrTag( pszTagName );
	CComBSTR bstrString( pszBuffer );

	if( SUCCEEDED(pDoc->createElement(bstrTag,&pElement)) )
	{
		pElement->put_text(bstrString);

		pNode->appendChild(pElement,NULL);
		pElement->Release();

		bSuccess = TRUE;
	}

	if( NewLine )
		_XMLAddString(pDoc,pNode,L"\n");

	return bSuccess;
}

inline HRESULT _XMLGetAttributeBSTR(IXMLDOMNamedNodeMap *pnodemap,PWSTR TagName,BSTR *psbResult)
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

inline HRESULT _XMLGetAttributeString(IXMLDOMNamedNodeMap *pnodemap,LPCWSTR TagName,PWSTR String,int cch)
{
	IXMLDOMNode *attrItem;
	HRESULT hr;

	*String = L'\0';

	CComBSTR Tag(TagName);
	hr = pnodemap->getNamedItem( Tag, &attrItem );
	if( hr == S_OK )
	{
		CComBSTR bs;
		if( (hr = attrItem->get_text(&bs)) == S_OK )
		{
			StringCchCopy(String,cch,bs);
		}
		attrItem->Release();
		return hr;
	}
	return hr;
}

inline void _cdecl _XMLSetFormatElement(IXMLDOMDocument *pDoc,IXMLDOMNode *pNode,PWSTR pszTagName,LPCWSTR pszFormat, ...)
{
	va_list args;
	va_start(args, pszFormat);

	int nBuf;
	WCHAR szBuffer[1024];
	size_t cch = sizeof(szBuffer) / sizeof(WCHAR);

	nBuf = _vsnwprintf_s(szBuffer, cch, pszFormat, args);
	if( nBuf != -1 )
	{
		IXMLDOMElement *pElement = _XMLAddElement(pDoc,pNode,pszTagName,szBuffer);
		if( pElement != NULL )
			pElement->Release();
	}

	va_end(args);
}

inline ULONG _XMLGetNodeCount( IXMLDOMNode *nodeParent )
{
	ULONG cCount = 0;

	IXMLDOMNode *node,*nextNode;

	nodeParent->get_firstChild( &nextNode );

	if( nextNode )
	{
		do
		{
			cCount++;
			node = nextNode;
			node->get_nextSibling( &nextNode );
			node->Release();
		}
		while( nextNode );
	}
	return cCount;
}

typedef struct _XML_ERROR_INFO
{
	LONG Line;
	LONG LinePos;
	HRESULT hr;
	WCHAR szReason[2048];
} XML_ERROR_INFO;

//
// XML Document Helper Wrapper Class
//
class CXMLDocument
{
public:
	IXMLDOMDocument* m_pDoc;
	IXMLDOMNode *m_pRoot;

	CXMLDocument()
	{
		m_pRoot = NULL;
		m_pDoc = NULL;
	}

	~CXMLDocument()
	{
		if( m_pRoot )
			m_pRoot->Release();
		if( m_pDoc )
			m_pDoc->Release();
	}

	IXMLDOMNode *AddRootNode(PCWSTR NodeName,PCWSTR Version)
	{
		return _XMLAddRootNode(m_pDoc,NodeName,Version);
	}

	IXMLDOMNode *AddSubNode(IXMLDOMNode *pParent,PCWSTR NodeName)
	{
		return _XMLAddNode(m_pDoc,pParent,NodeName);
	}

	void NewLine(IXMLDOMNode *pNode)
	{
		_XMLAddString(m_pDoc,pNode,L"\n");
	}

	void Tab(IXMLDOMNode *pNode,int nTab=1)
	{
		while( nTab-- )
			_XMLAddString(m_pDoc,pNode,L"\t");
	}

	HRESULT Write(PCWSTR Filename)
	{
		HRESULT hr;

		// XMLDocs
		//
		CComVariant varFile(Filename);

		hr = m_pDoc->save(varFile);

		return hr;
	}
};
