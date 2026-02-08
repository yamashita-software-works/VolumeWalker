#pragma once

class CGuidToString
{
	WCHAR m_szGuid[39];
public:
	CGuidToString()
	{
		m_szGuid[0] = 0;
	}

	CGuidToString( GUID& Guid )
	{
		StringFromGUID(&Guid,m_szGuid,ARRAYSIZE(m_szGuid));
	}

	~CGuidToString()
	{
	}

	operator PWSTR() const
	{
		return (PWSTR)m_szGuid;
	}

	operator PCWSTR() const
	{
		return (PCWSTR)m_szGuid;
	}
};
