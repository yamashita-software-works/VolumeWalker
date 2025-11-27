#pragma once

interface IWriteFile
{
	virtual HRESULT Release() = 0;
	virtual HRESULT Create(PCWSTR Filename) = 0;
	virtual HRESULT Close() = 0;
	virtual HRESULT Start(PCWSTR name) = 0;
	virtual HRESULT Commit() = 0;
	virtual HRESULT StartElement(PCWSTR pszLocalName) = 0;
	virtual HRESULT WriteString(PCWSTR pszString) = 0;
	virtual HRESULT EndElement() = 0;
	virtual HRESULT AttributeString(PCWSTR pszLocalName, PCWSTR pszValue) = 0;
};

HRESULT CreateWriter(IWriteFile **ppWriter);
HRESULT DeleteWriter(IWriteFile *pWriter);

