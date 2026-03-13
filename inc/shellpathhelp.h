#pragma once

class makedllpath
{
	PWSTR m_p;
public:
	makedllpath(PCWSTR Name,PCWSTR Path=NULL)
	{
		WCHAR szPath[MAX_PATH];
		if( Path )
		{
			StrCpyN(szPath,Path,MAX_PATH);
		}
		else
		{
			GetModuleFileName(NULL,szPath,MAX_PATH);
			PathRemoveFileSpec(szPath);
		}
		PathCombine(szPath,szPath,Name);
		m_p = StrDup(szPath);
	}

	~makedllpath()
	{
		LocalFree(m_p);
	}

	PWSTR getpath() const
	{
		if( m_p )
			return m_p;
		return L"";
	}

	operator LPCWSTR() const
	{
		return getpath();
	}

	operator LPWSTR() const
	{
		return getpath();
	}
};

