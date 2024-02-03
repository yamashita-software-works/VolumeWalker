#ifndef _LISTHELP_
#define _LISTHELP_

//
// GetDisp Column Hanlder
//
template< class T > struct COLUMN_HANDLER_PROC
{
	LRESULT (T::*pfn)(UINT,NMLVDISPINFO*);
};

template< class T > struct COLUMN_HANDLER_DEF
{
	int colid;
	LRESULT (T::*pfn)(UINT,NMLVDISPINFO*);
};

#define COL_HANDLER_MAP_DEF(colid,pfn) { colid,pfn }

//
// Compare Column Handler
//
template< class T, class C > struct COMPARE_HANDLER_PROC
{
	int (T::*proc)(C *p1,C *p2, const void *p);
};

template< class T, class C > struct COMPARE_HANDLER_PROC_DEF
{
	int colid;
	int (T::*proc)(C *p1,C *p2, const void *p);
};

#define _COMP(n1,n2)  (n1 < n2 ? -1 : n1 > n2 ? 1 : 0)

// compare helper
	_inline bool _compare_pointer_nullstring(PCWSTR psz1,PCWSTR psz2,int direction,int& result)
	{
		if( psz1 != NULL && psz2 == NULL )
			return result = (-1 * direction), true;
		else if( psz1 == NULL && psz2 != NULL )
			return result = (1  * direction), true;
		else if( psz1 == NULL && psz2 == NULL )
			return result = 0, true;

		if( *psz1 != L'\0' && *psz2 == L'\0' )
			return result = (-1 * direction), true;
		else if( *psz1 == L'\0' && *psz2 != L'\0' )
			return result = (1  * direction), true;

		return false;
	}

	_inline int _compare_pointer_string(PCWSTR psz1,PCWSTR psz2,int direction)
	{
		int result;
		if( _compare_pointer_nullstring(psz1,psz2,direction,result) )
			return result;
		return StrCmp(psz1,psz2);
	}

	_inline int _compare_pointer_string_logical(PCWSTR psz1,PCWSTR psz2,int direction)
	{
		int result;
		if( _compare_pointer_nullstring(psz1,psz2,direction,result) )
			return result;
		return StrCmpLogicalW(psz1,psz2);
	}

	_inline bool _compare_pointer_ansi_nullstring(PCSTR psz1,PCSTR psz2,int direction,int& result)
	{
		if( psz1 != NULL && psz2 == NULL )
			return result = (-1 * direction), true;
		else if( psz1 == NULL && psz2 != NULL )
			return result = (1  * direction), true;
		else if( psz1 == NULL && psz2 == NULL )
			return result = 0, true;

		if( *psz1 != '\0' && *psz2 == '\0' )
			return result = (-1 * direction), true;
		else if( *psz1 == '\0' && *psz2 != '\0' )
			return result = (1  * direction), true;

		return false;
	}

	_inline int _compare_pointer_ansi_string(PCSTR psz1,PCSTR psz2,int direction)
	{
		int result;
		if( _compare_pointer_ansi_nullstring(psz1,psz2,direction,result) )
			return result;
		return StrCmpA(psz1,psz2);
	}

template <class T>struct SORT_PARAM
{
	T* pThis;
	UINT id;
	int direction;
	int directory_align;
};
#define _COMP(n1,n2)  (n1 < n2 ? -1 : n1 > n2 ? 1 : 0)
#define _IS_DIRECTORY(d) (d & FILE_ATTRIBUTE_DIRECTORY)

#endif
