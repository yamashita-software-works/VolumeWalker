#pragma once
//***************************************************************************
//*                                                                         *
//*  page_disklayout.h                                                      *
//*                                                                         *
//*  Disk Layout Page                                                       *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2023-08-14                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include <diskguid.h>
#include "diskfindvolume.h"
#include "findhandler.h"

#define _IT_ITEM        0x0
#define _IT_GAP         0x1
#define _IT_REMAINING   0x2
#define _IT_EXTENDED    0x3

class CDiskLayoutView :
	public CPageWndBase,
	public CFindHandler<CDiskLayoutView>
{
	HWND m_hWndList;
	CPhysicalDriveInformation *m_PDInfo;
	PARTITION_STYLE m_mode;
	CDiskFindVolume m_findVolume;
	HFONT m_hFont;
	PWSTR m_pszPhysicalDrive;

	typedef struct _LAYOUTITEM {
		UINT ItemType;
		LARGE_INTEGER StartOffset;
	} LAYOUTITEM;

public:
	CDiskLayoutView()
	{
		m_hWndList = NULL;
		m_PDInfo = NULL;
		m_pszPhysicalDrive = NULL;
		m_hFont = NULL;
	}

	~CDiskLayoutView()
	{
	}

	HWND GetListView() const { return m_hWndList; }

	LRESULT OnCreate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		InitList(hWnd);
		return 0;
	}

	LRESULT OnSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int cx = GET_X_LPARAM(lParam);
		int cy = GET_Y_LPARAM(lParam);
		UpdateLayout(cx,cy);
		return 0;
	}

	LRESULT OnDestroy(HWND hWnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if( m_hFont )
			DeleteObject(m_hFont);

		_SafeMemFree(m_pszPhysicalDrive);

		if( m_PDInfo )
		{
			delete m_PDInfo;
			m_PDInfo = NULL;
		}
		return 0;
	}

	LRESULT OnContextMenu(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);
		if( iItem == -1 )
			return 0;

		LAYOUTITEM *pli = (LAYOUTITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

		HMENU hMenu = CreatePopupMenu();
		AppendMenu(hMenu,MF_STRING,ID_EDIT_COPY,L"&Copy Text");
		AppendMenu(hMenu,MF_STRING,0,0);
		AppendMenu(hMenu,MF_STRING,ID_HEXDUMP,L"Sector &Dump");

		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ListViewEx_SimpleContextMenuHandler(NULL,m_hWndList,(HWND)wParam,hMenu,pt,0);

		DestroyMenu(hMenu);

		return 0;
	}

	LRESULT OnNotify(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		NMHDR *pnmhdr = (NMHDR *)lParam;

		switch( pnmhdr->code )
		{
			case NM_CUSTOMDRAW:
				return OnCustomDraw(pnmhdr);
			case LVN_GETDISPINFO:
				return OnGetDispInfo(pnmhdr);
			case LVN_DELETEITEM:
				return OnDeleteItem(pnmhdr);
			case NM_SETFOCUS:
				return OnNmSetFocus(pnmhdr);
		}
		return 0;
	}

	LRESULT OnNmSetFocus(NMHDR *pnmhdr)
	{
		SendMessage(GetParent(m_hWnd),WM_NOTIFY,0,(LPARAM)pnmhdr);
		return 0;
	}

	LRESULT OnDeleteItem(NMHDR *pnmhdr)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)pnmhdr;
		if( pnmlv->lParam )
			delete (LAYOUTITEM *)pnmlv->lParam;
		return 0;
	}

	LRESULT OnGetDispInfo(NMHDR *pnmhdr)
	{
		NMLVDISPINFO *pdi = (NMLVDISPINFO*)pnmhdr;
		return 0;	
	}

	virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
			case WM_SETFOCUS:
				SetFocus(m_hWndList);
				break;
			case WM_SIZE:
				return OnSize(hWnd,uMsg,wParam,lParam);
			case WM_NOTIFY:
				return OnNotify(hWnd,uMsg,wParam,lParam);
			case WM_CREATE:
				return OnCreate(hWnd,uMsg,wParam,lParam);
			case WM_DESTROY:
				return OnDestroy(hWnd,uMsg,wParam,lParam);
			case WM_CONTEXTMENU:
				return OnContextMenu(hWnd,uMsg,wParam,lParam);
			case PM_FINDITEM:
				return CFindHandler<CDiskLayoutView>::OnFindItem(hWnd,uMsg,wParam,lParam);
		}
		return CBaseWindow::WndProc(hWnd,uMsg,wParam,lParam);
	}

public:
	VOID FillDriveLayout(DRIVE_LAYOUT_INFORMATION_EX *pdli,DWORD dwDriveNumber,LARGE_INTEGER liDriveLength)
	{
		m_findVolume.Enum();

		if( pdli->PartitionStyle == PARTITION_STYLE_MBR )
		{
			FillMBRPartitionInfo(pdli,dwDriveNumber,liDriveLength);
		}
		else if( pdli->PartitionStyle == PARTITION_STYLE_GPT )
		{
			FillGPRPartitionInfo(pdli,dwDriveNumber,liDriveLength);
		}
		else
		{
			; // RAW
		}

		m_findVolume.Free();
	}

	int InsertLine(int iItem,int indent,PCWSTR psz,LONGLONG Offset,ULONG ItemType=0)
	{
		LVITEM lvi = {0};
		lvi.mask = LVIF_TEXT|LVIF_PARAM|LVIF_INDENT;
		lvi.iItem = iItem;
		lvi.iIndent = indent * 8;
		lvi.pszText = (PWSTR)psz;

#if 0
		if( ItemType != 0 )
			lvi.lParam = ItemType;
#else
		LAYOUTITEM *pli = new LAYOUTITEM;
		pli->ItemType = ItemType;
		pli->StartOffset.QuadPart = Offset;
		lvi.lParam = (LPARAM)pli;
#endif

		return ListView_InsertItem(m_hWndList,&lvi);
	}

	//
	// Fill MBR partition information
	//
	void FillMBRPartitionInfo(DRIVE_LAYOUT_INFORMATION_EX *pdli,DWORD dwDriveNumber,LARGE_INTEGER& liDriveLength)
	{
		WCHAR szText[MAX_PATH];
		WCHAR szHeader[MAX_PATH];
		DWORD i;
		int iItem;
		CSimpleStack<int,256> stack;
		LONGLONG LastNextOffset = 0;
		int pos;

		for(i = 0; i < pdli->PartitionCount; i++)
		{
			iItem = ListView_GetItemCount(m_hWndList);

		    DWORD PartitionNumber = pdli->PartitionEntry[i].PartitionNumber;

			if( pdli->PartitionEntry[i].Mbr.PartitionType != 0 )
			{
				if( PartitionNumber > 0 )
				{
					StringCchPrintf(szHeader,MAX_PATH,L"Partition%d",PartitionNumber);
				}
				else
				{
					if( pdli->PartitionEntry[i].Mbr.PartitionType == PARTITION_EXTENDED )
					{
						StringCchPrintf(szHeader,MAX_PATH,L"Extended Partition");
					}
					else if( pdli->PartitionEntry[i].Mbr.PartitionType == PARTITION_XINT13_EXTENDED ) 
					{
						StringCchPrintf(szHeader,MAX_PATH,L"XINT13 Extended Partition");
					}
					else
					{
						StringCchPrintf(szHeader,MAX_PATH,L"Partition 0x%02X",pdli->PartitionEntry[i].Mbr.PartitionType);
					}
				}

				int indent = stack.GetSize();

				if( i > 0 )
				{
					// Find previous valid partition record
					int iPrevValidIndex;
					for(iPrevValidIndex=i-1; pdli->PartitionEntry[iPrevValidIndex].Mbr.PartitionType==0; iPrevValidIndex--) { ; }

					ASSERT( iPrevValidIndex >= 0 );

					LONGLONG PreviousStartOffset = pdli->PartitionEntry[iPrevValidIndex].StartingOffset.QuadPart;
					LONGLONG PreviousNextOffset = (pdli->PartitionEntry[iPrevValidIndex].StartingOffset.QuadPart+pdli->PartitionEntry[iPrevValidIndex].PartitionLength.QuadPart);
					LONGLONG StartOffset = pdli->PartitionEntry[i].StartingOffset.QuadPart;

					if( PreviousNextOffset == StartOffset )
					{
						// adjacent
						//
						// |================| Disk
						// |----|             Partition#1
						//      |-----------| Partition#2
					}
					else if( PreviousNextOffset < StartOffset )
					{
						// gap
						//
						// |================| Disk
						// |---|              Partition#1
						//      <-->          GAP
						//          |-------| Partition#2
						InsertLine(iItem,0,L"",PreviousNextOffset,_IT_GAP);

						StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",PreviousNextOffset);
						ListView_SetItemText(m_hWndList,iItem,1,szText);

						StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",StartOffset);
						ListView_SetItemText(m_hWndList,iItem,2,szText);

						StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",(StartOffset-PreviousNextOffset));
						ListView_SetItemText(m_hWndList,iItem,3,szText);

						_FormatByteSize((StartOffset-PreviousNextOffset),szText,ARRAYSIZE(szText));
						ListView_SetItemText(m_hWndList,iItem,4,szText);

						iItem++;
					}
					else if( PreviousNextOffset > StartOffset )
					{
						// including me
						if( StartOffset == PreviousStartOffset )
						{
							// |==========| Disk
							// |----|       Partition
							;
						}
						else
						{
							// |==========| Disk
							//     |----|   Partition

							// |==========| Disk
							//     |------| Partition
							InsertLine(iItem,0,L"",PreviousStartOffset,_IT_GAP); // includeing gap

							StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",PreviousStartOffset);
							ListView_SetItemText(m_hWndList,iItem,1,szText);

							StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",StartOffset);
							ListView_SetItemText(m_hWndList,iItem,2,szText);

							StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",(StartOffset-PreviousStartOffset));
							ListView_SetItemText(m_hWndList,iItem,3,szText);

							_FormatByteSize((StartOffset-PreviousStartOffset),szText,ARRAYSIZE(szText));
							ListView_SetItemText(m_hWndList,iItem,4,szText);

							iItem++;
						}
					}
				}
				else
				{
					LastNextOffset = (pdli->PartitionEntry[i].StartingOffset.QuadPart+pdli->PartitionEntry[i].PartitionLength.QuadPart);
				}

				iItem = InsertLine(iItem,indent,szHeader,pdli->PartitionEntry[i].StartingOffset.QuadPart);

				if( (pdli->PartitionEntry[i].Mbr.PartitionType == PARTITION_EXTENDED) || 
				    (pdli->PartitionEntry[i].Mbr.PartitionType == PARTITION_XINT13_EXTENDED)  )
				{
					stack.Push(i);
				}

				LONGLONG NextOffset = (pdli->PartitionEntry[i].StartingOffset.QuadPart+pdli->PartitionEntry[i].PartitionLength.QuadPart);

				LastNextOffset = NextOffset;

				int pos = 1;

				StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",pdli->PartitionEntry[i].StartingOffset.QuadPart);
				ListView_SetItemText(m_hWndList,iItem,pos++,szText);

				StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",(pdli->PartitionEntry[i].StartingOffset.QuadPart+pdli->PartitionEntry[i].PartitionLength.QuadPart));
				ListView_SetItemText(m_hWndList,iItem,pos++,szText);

				StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",pdli->PartitionEntry[i].PartitionLength.QuadPart);
				ListView_SetItemText(m_hWndList,iItem,pos++,szText);

				_FormatByteSize(pdli->PartitionEntry[i].PartitionLength.QuadPart,szText,ARRAYSIZE(szText));
				ListView_SetItemText(m_hWndList,iItem,pos++,szText);

				if( PartitionNumber > 0 )
				{
					int iExtPartIndex = stack.GetCurrent();
					if( NextOffset == (pdli->PartitionEntry[iExtPartIndex].StartingOffset.QuadPart+pdli->PartitionEntry[iExtPartIndex].PartitionLength.QuadPart) )
					{
						stack.Pop(); // restore extended partition
					}
				}

				PCWSTR pszVolumeName;
				WCHAR szDrive[4];

				pszVolumeName = NULL;
				szDrive[0] = 0;

				if(	pdli->PartitionEntry[i].PartitionLength.QuadPart != 0 )
				{
					if( m_findVolume.Find( dwDriveNumber,
										   pdli->PartitionEntry[i].StartingOffset,
										   pdli->PartitionEntry[i].PartitionLength,
										   pszVolumeName) )
					{
						GetVolumeDosName(pszVolumeName,szDrive,ARRAYSIZE(szDrive));
					}

					if( pszVolumeName != NULL )
						ListView_SetItemText(m_hWndList,iItem,pos++,(PWSTR)pszVolumeName);

					if( szDrive[0] != 0 )
						ListView_SetItemText(m_hWndList,iItem,pos++,szDrive);
				}

				if( (pdli->PartitionEntry[i].Mbr.PartitionType == PARTITION_EXTENDED) || 
				    (pdli->PartitionEntry[i].Mbr.PartitionType == PARTITION_XINT13_EXTENDED)  )
				{
//					ListViewEx_SetItemData(m_hWndList,iItem,3);
					LAYOUTITEM *pli = (LAYOUTITEM *)ListViewEx_GetItemData(m_hWndList,iItem);
					pli->ItemType = _IT_EXTENDED;
//					ListViewEx_SetItemData(m_hWndList,iItem,(LPARAM)pli);
				}
			}
		}

		//
		// Insert as remaining length if not value zero.
		//
		LONGLONG remainder = liDriveLength.QuadPart - LastNextOffset;
		if( remainder > 0 )
		{
			iItem = ListView_GetItemCount(m_hWndList);
			InsertLine(iItem,0,L"",LastNextOffset,_IT_REMAINING);
			pos = 1;
			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",LastNextOffset);
			ListView_SetItemText(m_hWndList,iItem,pos++,szText);

			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",liDriveLength.QuadPart);
			ListView_SetItemText(m_hWndList,iItem,pos++,szText);

			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",remainder);
			ListView_SetItemText(m_hWndList,iItem,pos++,szText);
			_FormatByteSize(remainder,szText,ARRAYSIZE(szText));
			ListView_SetItemText(m_hWndList,iItem,pos++,szText);

		}

		//
		// Total Length
		//
		{
			iItem = ListView_GetItemCount(m_hWndList);
			InsertLine(iItem,0,L"Toltal Length",-1);
			pos = 3;
			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",liDriveLength.QuadPart);
			ListView_SetItemText(m_hWndList,iItem,pos++,szText);
			_FormatByteSize(liDriveLength.QuadPart,szText,ARRAYSIZE(szText));
			ListView_SetItemText(m_hWndList,iItem,pos++,szText);
		}
	}

	//
	// Fill GPT partition information
	//
	VOID FillGPRPartitionInfo(DRIVE_LAYOUT_INFORMATION_EX *pdli,DWORD dwDriveNumber,LARGE_INTEGER liDriveLength)
	{
		DWORD i;
		int iItem;
		WCHAR szText[MAX_PATH];
		LONGLONG LastNextOffset = 0;
		int indent = 0;

		for(i = 0; i < pdli->PartitionCount; i++)
		{
			iItem = ListView_GetItemCount(m_hWndList);

			if( i > 0 )
			{
				// Find previous valid partition record
				int iPrevValidIndex = i - 1;

				ASSERT( iPrevValidIndex >= 0 );

				LONGLONG PreviousStartOffset = pdli->PartitionEntry[iPrevValidIndex].StartingOffset.QuadPart;
				LONGLONG PreviousNextOffset = (pdli->PartitionEntry[iPrevValidIndex].StartingOffset.QuadPart+pdli->PartitionEntry[iPrevValidIndex].PartitionLength.QuadPart);
				LONGLONG StartOffset = pdli->PartitionEntry[i].StartingOffset.QuadPart;

				if( PreviousNextOffset == StartOffset )
				{
					// ok
					//
					// |================|
					// |----|
					//      |-----------|
				}
				else if( PreviousNextOffset < StartOffset )
				{
					// gap
					//
					// |================|
					// |---|
					//      <-->           GAP
					//          |-------|
					InsertLine(iItem,indent,L"",PreviousNextOffset,_IT_GAP);

					StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",PreviousNextOffset);//PreviousStartOffset); BUGBUG?20231009
					ListView_SetItemText(m_hWndList,iItem,1,szText);

					StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",StartOffset);
					ListView_SetItemText(m_hWndList,iItem,2,szText);

					StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",(StartOffset-PreviousNextOffset));
					ListView_SetItemText(m_hWndList,iItem,3,szText);

					_FormatByteSize((StartOffset-PreviousNextOffset),szText,ARRAYSIZE(szText));
					ListView_SetItemText(m_hWndList,iItem,4,szText);

					iItem++;
				}
				else if( PreviousNextOffset > StartOffset )
				{
					// including me
					if( StartOffset == PreviousStartOffset )
					{
						// ok
						// |==========|
						// |----|
						;
					}
					else
					{
						// |==========|
						//     |----|

						// |==========|
						//     |------|
						StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",PreviousStartOffset);
						ListView_SetItemText(m_hWndList,iItem,1,szText);

						StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",StartOffset);
						ListView_SetItemText(m_hWndList,iItem,2,szText);

						StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",(StartOffset-PreviousStartOffset));
						ListView_SetItemText(m_hWndList,iItem,3,szText);

						_FormatByteSize((StartOffset-PreviousStartOffset),szText,ARRAYSIZE(szText));
						ListView_SetItemText(m_hWndList,iItem,4,szText);

						iItem++;
					}
				}
			}
			else
			{
				LastNextOffset = (pdli->PartitionEntry[i].StartingOffset.QuadPart+pdli->PartitionEntry[i].PartitionLength.QuadPart);
			}

		    DWORD PartitionNumber = pdli->PartitionEntry[i].PartitionNumber;
			StringCchPrintf(szText,ARRAYSIZE(szText),L"Partition#%d",PartitionNumber);
			iItem = InsertLine(iItem,indent,szText,pdli->PartitionEntry[i].StartingOffset.QuadPart);

			LONGLONG NextOffset = (pdli->PartitionEntry[i].StartingOffset.QuadPart+pdli->PartitionEntry[i].PartitionLength.QuadPart);

			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",pdli->PartitionEntry[i].StartingOffset.QuadPart);
			ListView_SetItemText(m_hWndList,iItem,1,szText);

#if _ENABLE_NEXT_COLUMN
			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",(pdli->PartitionEntry[i].StartingOffset.QuadPart+pdli->PartitionEntry[i].PartitionLength.QuadPart));
			ListView_SetItemText(m_hWndList,iItem,2,szText);
#endif
			LastNextOffset = (pdli->PartitionEntry[i].StartingOffset.QuadPart+pdli->PartitionEntry[i].PartitionLength.QuadPart);

			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",pdli->PartitionEntry[i].PartitionLength.QuadPart);
			ListView_SetItemText(m_hWndList,iItem,3,szText);

			_FormatByteSize(pdli->PartitionEntry[i].PartitionLength.QuadPart,szText,ARRAYSIZE(szText));
			ListView_SetItemText(m_hWndList,iItem,4,szText);

			ZeroMemory(szText,sizeof(szText));
			GetGPTPartitionTypeString(pdli->PartitionEntry[i].Gpt.PartitionType,szText,_countof(szText));

			if( !IsEqualGUID(pdli->PartitionEntry[i].Gpt.PartitionType,PARTITION_ENTRY_UNUSED_GUID) )
			{
				PWSTR p = pdli->PartitionEntry[i].Gpt.Name[0] != L'\0' ? pdli->PartitionEntry[i].Gpt.Name : L"NONE";
				ListView_SetItemText(m_hWndList,iItem,7,p);
			}

			PCWSTR pszVolumeName;
			WCHAR szDrive[4];

			pszVolumeName = NULL;
			szDrive[0] = 0;

			if(	pdli->PartitionEntry[i].PartitionLength.QuadPart != 0 )
			{
				if( m_findVolume.Find( dwDriveNumber,
									   pdli->PartitionEntry[i].StartingOffset,
									   pdli->PartitionEntry[i].PartitionLength,
									   pszVolumeName) )
				{
					GetVolumeDosName(pszVolumeName,szDrive,ARRAYSIZE(szDrive));
				}

				if( pszVolumeName != NULL )
					ListView_SetItemText(m_hWndList,iItem,5,(PWSTR)pszVolumeName);

				if( szDrive[0] != 0 )
					ListView_SetItemText(m_hWndList,iItem,6,szDrive);
			}
		}

		//
		// Insert remaining length if not value zero.
		//
		LONGLONG remainder = liDriveLength.QuadPart - LastNextOffset;
		if( remainder > 0 )
		{
			iItem = ListView_GetItemCount(m_hWndList);
			InsertLine(iItem,0,L"",LastNextOffset,_IT_REMAINING);

			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",LastNextOffset);
			ListView_SetItemText(m_hWndList,iItem,1,szText);

			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",liDriveLength.QuadPart);
			ListView_SetItemText(m_hWndList,iItem,2,szText);

			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",remainder);
			ListView_SetItemText(m_hWndList,iItem,3,szText);

			_FormatByteSize(remainder,szText,ARRAYSIZE(szText));
			ListView_SetItemText(m_hWndList,iItem,4,szText);
		}

		//
		// Total Length
		//
		{
			iItem = ListView_GetItemCount(m_hWndList);
			InsertLine(iItem,0,L"Toltal Length",-1);
			int pos = 3;
			StringCchPrintf(szText,ARRAYSIZE(szText),L"0x%I64X",liDriveLength.QuadPart);
			ListView_SetItemText(m_hWndList,iItem,pos++,szText);
			_FormatByteSize(liDriveLength.QuadPart,szText,ARRAYSIZE(szText));
			ListView_SetItemText(m_hWndList,iItem,pos++,szText);
		}
	}

	LRESULT OnCustomDraw(LPNMHDR pnmh)
	{
		LRESULT lResult = 0;
		NMLVCUSTOMDRAW* pcd = (NMLVCUSTOMDRAW* )pnmh;

		if( pcd->nmcd.dwDrawStage == CDDS_PREPAINT )
		{
			return CDRF_NOTIFYITEMDRAW;
		}

		if( pcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT )
		{
			LAYOUTITEM *pli = (LAYOUTITEM *)pcd->nmcd.lItemlParam;

			if( pli ) // for 32bit BUGBUG
			{
				if( pli->ItemType == _IT_EXTENDED )
				{
					return CDRF_DODEFAULT;
				}

				if( pli->ItemType != 0 )
				{
					pcd->clrTextBk = RGB(246,246,246);
					pcd->clrText = RGB(152,152,152);
					return CDRF_NEWFONT;
				}
			}
		}

		return CDRF_DODEFAULT;
	}


	VOID SetMode( PARTITION_STYLE mode )
	{
		m_mode = mode;
	}

	VOID UpdateLayout(int cx,int cy,BOOL bColumnAdjust=FALSE)
	{
		if( m_hWndList )
		{
			SetRedraw(m_hWndList,FALSE);
			SetWindowPos(m_hWndList,NULL,0,0,cx,cy,SWP_NOMOVE|SWP_NOZORDER);
			SetRedraw(m_hWndList,TRUE);
		}
	}

	void UpdateContents(CPhysicalDriveInformation *pvPDInfoBuffer)
	{
		CPhysicalDriveInformation *pPDInfo = (CPhysicalDriveInformation *)pvPDInfoBuffer;

		FillDriveLayout(pPDInfo->pDriveLayout,pPDInfo->m_dwDiskNumber,pPDInfo->pGeometry->DiskSize);

		ListView_SetColumnWidth(m_hWndList,0,LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hWndList,1,LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hWndList,2,LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hWndList,3,LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hWndList,4,LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hWndList,5,LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hWndList,6,LVSCW_AUTOSIZE_USEHEADER);
		ListView_SetColumnWidth(m_hWndList,7,LVSCW_AUTOSIZE_USEHEADER);
	}

	void InitColumns()
	{
		int col = 0;
		ListViewEx_InsertColumnText(m_hWndList, col++, L"Partition",   LVCFMT_LEFT );
		ListViewEx_InsertColumnText(m_hWndList, col++, L"Start",       LVCFMT_RIGHT );
		ListViewEx_InsertColumnText(m_hWndList, col++, L"Next Extent", LVCFMT_RIGHT );
		ListViewEx_InsertColumnText(m_hWndList, col++, L"Length",      LVCFMT_RIGHT );
		ListViewEx_InsertColumnText(m_hWndList, col++, L"Length(Hint)",LVCFMT_RIGHT );
		ListViewEx_InsertColumnText(m_hWndList, col++, L"Device",      LVCFMT_LEFT );
		ListViewEx_InsertColumnText(m_hWndList, col++, L"Drive",       LVCFMT_LEFT );
		ListViewEx_InsertColumnText(m_hWndList, col++, L"Type",        LVCFMT_LEFT );
	}

	HRESULT InitList(HWND hWnd)
	{
		m_hWndList = CreateWindowEx(0,WC_LISTVIEW, 
                              L"", 
                              WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER, 
                              0,0,0,0,
                              hWnd,
                              (HMENU)0,
                              GetModuleHandle(NULL), 
                              NULL); 

		_EnableVisualThemeStyle(m_hWndList);

		ListView_SetExtendedListViewStyle(m_hWndList,LVS_EX_DOUBLEBUFFER|LVS_EX_FULLROWSELECT);

		HIMAGELIST himl;
		himl = ImageList_Create(
					1, // 1pixcel width
					_DPI_Adjust_Y( GetSystemMetrics(SM_CYSMICON) + 8 ),
					ILC_COLOR32|ILC_MASK,1,1);
		ListView_SetImageList(m_hWndList,himl,LVSIL_SMALL);

		// set fixed width font
		{
			HDC hdc;
			hdc = GetWindowDC(hWnd);

			LOGFONT lf = {0};
			lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
			lf.lfCharSet = ANSI_CHARSET;
			StringCchCopy(lf.lfFaceName,_countof(lf.lfFaceName),L"Consolas");

			m_hFont = CreateFontIndirect( &lf );

			SendMessage(m_hWndList,WM_SETFONT,(WPARAM)m_hFont,0);

			ReleaseDC(hWnd,hdc);
		}

		InitColumns();

		return S_OK;
	}

	HRESULT GetData(PCWSTR pszPhysicalDrive,CPhysicalDriveInformation **pPDInfo)
	{
		HRESULT hr = E_FAIL;
		DWORD dwDriveNumber;

		dwDriveNumber = StringFindNumber(pszPhysicalDrive);

		CPhysicalDriveInformation *pdi = new CPhysicalDriveInformation;

		if( pdi->OpenDisk(pszPhysicalDrive,dwDriveNumber) == S_OK )
		{
			pdi->GetGeometry();
			pdi->GetDriveLayout();
			pdi->GetDeviceIdDescriptor();
			pdi->GetDetectSectorSize();
			*pPDInfo = pdi;
			hr = S_OK;
		}

		return hr;
	}

	virtual HRESULT UpdateData(PVOID pData)
	{
		SELECT_ITEM *pSel = (SELECT_ITEM *)pData;

		LoadFltLibDll(NULL);

		HRESULT hr;
		CPhysicalDriveInformation *pdi = NULL;

		_SafeMemFree(m_pszPhysicalDrive);
		m_pszPhysicalDrive = _MemAllocString(pSel->pszPhysicalDrive);

		hr = GetData( m_pszPhysicalDrive, &pdi );

		if( SUCCEEDED(hr) )
		{
			SetRedraw(m_hWndList,FALSE);

			ListView_DeleteAllItems(m_hWndList);

			if( m_PDInfo )
				delete m_PDInfo;

			m_PDInfo = pdi;

			UpdateContents(pdi);

			SetRedraw(m_hWndList,TRUE);
		}

		UnloadFltLibDll(NULL);

		return hr;
	}

	virtual HRESULT QueryCmdState(UINT CmdId,UINT *State)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
			case ID_HEXDUMP:
				*State = ListView_GetSelectedCount(m_hWndList) ? UPDUI_ENABLED : UPDUI_DISABLED;
				return S_OK;
			case ID_VIEW_REFRESH:
			case ID_EDIT_FIND:
			case ID_EDIT_FIND_NEXT:
			case ID_EDIT_FIND_PREVIOUS:
				*State = UPDUI_ENABLED;
				return S_OK;
		}
		return S_FALSE;
	}

	virtual HRESULT InvokeCommand(UINT CmdId)
	{
		switch( CmdId )
		{
			case ID_EDIT_COPY:
				OnCmdEditCopy();
				break;
			case ID_HEXDUMP:
				OnCmdHexDump();
				break;
			case ID_VIEW_REFRESH:
				OnCmdRefresh();
				break;
		}
		return S_OK;
	}

	void OnCmdEditCopy()
	{
		if( GetKeyState(VK_SHIFT) < 0 )
		{
			SetClipboardTextFromListViewColumn(m_hWndList,SCTEXT_FORMAT_SELECTONLY,1);
		}
		else
		{
			SetClipboardTextFromListView(m_hWndList,SCTEXT_UNICODE);
		}
	}

	void OnCmdHexDump()
	{
		int iItem = ListViewEx_GetCurSel(m_hWndList);

		ASSERT( iItem != -1 );

		LAYOUTITEM *pli = (LAYOUTITEM *)ListViewEx_GetItemData(m_hWndList,iItem);

		if( pli )
		{
			OpenConsole_SendMessage(VOLUME_CONSOLE_SIMPLEHEXDUMP,m_pszPhysicalDrive,pli->StartOffset.QuadPart);
		}
	}

	void OnCmdRefresh()
	{
		SELECT_ITEM sel = {0};
		sel.pszPath = m_pszPhysicalDrive;
		UpdateData(&sel);
	}
};
