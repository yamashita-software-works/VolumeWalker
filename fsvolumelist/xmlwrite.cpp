//***************************************************************************
//*                                                                         *
//*  xmlwriter.cpp                                                          *
//*                                                                         *
//*  Simple XML File Writer                                                 *
//*                                                                         *
//*  Author:  YAMASHITA Katsuhiro                                           *
//*                                                                         *
//*  History: 2025-09-16 Created.                                           *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
// 
#include "stdafx.h"
#include <xmllite.h>
#include "xmlwrite.h"
#pragma comment(lib, "xmllite.lib")

class CXmlWriteStorageDevices : public IWriteFile
{
    IStream    *pOutFileStream;
    IXmlWriter *pXmlWriter;
public:
	CXmlWriteStorageDevices();
	~CXmlWriteStorageDevices();

	HRESULT Release();

	HRESULT Create(PCWSTR Filename);
	HRESULT Close();

	HRESULT Start(PCWSTR name);
	HRESULT Commit();

	HRESULT StartElement(PCWSTR pszLocalName);
	HRESULT WriteString(PCWSTR pszString);
	HRESULT EndElement();

	HRESULT AttributeString(PCWSTR pszLocalName, PCWSTR pszValue);
};

HRESULT CreateWriter(IWriteFile **ppFileWriter)
{
	if( ppFileWriter == NULL )
		return E_INVALIDARG;
	*ppFileWriter = static_cast<IWriteFile *>(new CXmlWriteStorageDevices);
	return *ppFileWriter ? S_OK : E_OUTOFMEMORY;
}

HRESULT DeleteWriter(IWriteFile *pFileWriter)
{
	if( pFileWriter == NULL )
		return E_INVALIDARG;
	pFileWriter->Release();
	return S_OK;
}

CXmlWriteStorageDevices::CXmlWriteStorageDevices()
{
	pOutFileStream = NULL;
	pXmlWriter = NULL;
}

CXmlWriteStorageDevices::~CXmlWriteStorageDevices()
{
	if( pOutFileStream ) {
		pOutFileStream->Release();
		pOutFileStream = NULL;
	}
	if( pXmlWriter ) {
		pXmlWriter->Release();
		pXmlWriter = NULL;
	}
}

HRESULT CXmlWriteStorageDevices::Release()
{
	delete this;
	return S_OK;
}

HRESULT CXmlWriteStorageDevices::Create(PCWSTR Filename)
{
    HRESULT hr;

	ASSERT(pOutFileStream == NULL);
	ASSERT(pXmlWriter == NULL);
	if( pXmlWriter != NULL || pOutFileStream != NULL )
		return E_FAIL;

    if( FAILED(hr = SHCreateStreamOnFile(Filename, STGM_CREATE | STGM_WRITE, &pOutFileStream)) )
    {
        return hr;
    }

    if( FAILED(hr = CreateXmlWriter(__uuidof(IXmlWriter), (void**) &pXmlWriter, NULL)) )
    {
        return hr;
    }

    if( FAILED(hr = pXmlWriter->SetOutput(pOutFileStream)) )
    {
        return hr;
    }

    if( FAILED(hr = pXmlWriter->SetProperty(XmlWriterProperty_Indent, TRUE)) )
    {
        return hr;
    }

    if( FAILED(hr = pXmlWriter->WriteStartDocument(XmlStandalone_Omit)) )
    {
        return hr;
    }

//	if( FAILED(hr = pXmlWriter->WriteProcessingInstruction(L"xml-stylesheet", L"href=\"volumewalker.xsl\" type=\"text/xsl\"")) )
//  {
//     return hr;
//  }

	return hr;
}

HRESULT CXmlWriteStorageDevices::Commit()
{
	HRESULT hr;

    if( FAILED(hr = pXmlWriter->WriteFullEndElement()) )
    {
        return hr;
    }

    if( FAILED(hr = pXmlWriter->WriteEndDocument()) )
    {
        return hr;
    }

    if( FAILED(hr = pXmlWriter->Flush()) )
    {
        return hr;
    }

	return hr;
}

HRESULT CXmlWriteStorageDevices::Close()
{
	return S_OK;
}

HRESULT CXmlWriteStorageDevices::Start(PCWSTR name)
{
	HRESULT hr;
	hr = pXmlWriter->WriteStartElement(NULL, L"root", NULL);
	return hr;
}

HRESULT CXmlWriteStorageDevices::StartElement(PCWSTR pszLocalName)
{
	HRESULT hr;
	hr = pXmlWriter->WriteStartElement(NULL, pszLocalName, NULL);
	return hr;
}

HRESULT CXmlWriteStorageDevices::WriteString(PCWSTR pszString)
{
	HRESULT hr;
	hr = pXmlWriter->WriteString(pszString);
	return hr;
}

HRESULT CXmlWriteStorageDevices::EndElement()
{
	HRESULT hr;
	hr = pXmlWriter->WriteEndElement();
	return hr;
}

HRESULT CXmlWriteStorageDevices::AttributeString(PCWSTR pszLocalName, PCWSTR pszValue)
{
	HRESULT hr;
	hr = pXmlWriter->WriteAttributeString(NULL, pszLocalName, NULL, pszValue);
	return hr;
}
