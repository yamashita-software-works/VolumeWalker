//
//  2023.05.31 Create
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"
#include "menu.h"

static BOOL _QueryAreEnabledMenuItems(HMENU hMenu,PFNQUERYCOMMANDSTATE pfnUpdateUICallback,LPARAM lParam)
{
	MENUCOMMANDSTATE mcs = {0};
	int i,cMenuItems;

	cMenuItems = GetMenuItemCount(hMenu);

	for(i = 0; i < cMenuItems; i++)
	{
		UINT id;
		UINT state;

		id = GetMenuItemID(hMenu,i);
		state = GetMenuState(hMenu,i,MF_BYPOSITION);

		// NOTE:
		// If the menu item opens a submenu, the low-order byte of 
		// the return value contains the menu flags associated with the item, 
		// and the high-order byte contains the number of items in 
		// the submenu opened by the item. 
		if( state & MF_POPUP )
			state &= 0xFF;

		if( state & MF_SEPARATOR )
			continue;

		if( id == (UINT)-1 )
		{
			//
			// Submenu item, Recursive check.
			//
			HMENU hSubMenu = GetSubMenu(hMenu,i);

			if( _QueryAreEnabledMenuItems(hSubMenu,pfnUpdateUICallback,lParam) )
				return TRUE;
		}
		else
		{
			//
			// Callback check
			//
			ULONG cmdUIState = pfnUpdateUICallback(id,&mcs,lParam);
			if( cmdUIState != -1 )
			{
				if( (cmdUIState & UPDUI_DISABLED) == UPDUI_ENABLED )
				{
					//
					// The enabled menu item found.
					// Therefore, the submenu containing this command must be enabled,
					// and no further processing to check the menu item is necessary.
					//
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

BOOL UpdateUI_QueryAreEnabledSubMenuItems(HMENU hMenu,PFNQUERYCOMMANDSTATE pfnQueryCmdStateCallback,LPARAM lParam)
{
	int i,cMenuItems;

	cMenuItems = GetMenuItemCount(hMenu);

	for(i = 0; i < cMenuItems; i++)
	{
		HMENU hSubMenu;
		hSubMenu = GetSubMenu(hMenu,i);
		if( hSubMenu )
		{
			BOOL bFoundEnableItem;
			bFoundEnableItem = _QueryAreEnabledMenuItems(hSubMenu,pfnQueryCmdStateCallback,lParam);

			EnableMenuItem(hMenu,i,MF_BYPOSITION|(bFoundEnableItem ? MF_ENABLED : MF_GRAYED));
		}
	}

	return TRUE;
}

VOID UpdateUI_MenuItem(HMENU hMenu,PFNQUERYCOMMANDSTATE pfnQueryCmdStateCallback,LPARAM lParam)
{
	UINT fMenuItemState;
	MENUCOMMANDSTATE mcs = {0};
	int i,cMenuItems;

	cMenuItems = GetMenuItemCount(hMenu);

	for(i = 0; i < cMenuItems; i++)
	{
		UINT id = GetMenuItemID(hMenu,i);
		if( id == 0 || id == (UINT)-1 ) // separator or popup submenu
			continue;

		fMenuItemState = pfnQueryCmdStateCallback(id,&mcs,lParam);

		EnableMenuItem(hMenu,i,MF_BYPOSITION | ((fMenuItemState & UPDUI_DISABLED) ? (MF_GRAYED|MF_DISABLED) : MF_ENABLED));
		CheckMenuItem(hMenu,i,MF_BYPOSITION | (fMenuItemState & UPDUI_CHECKED ? MF_CHECKED : MF_UNCHECKED));
		if((fMenuItemState & UPDUI_RADIO) != 0)
			CheckMenuRadioItem(hMenu,mcs.IdFirst,mcs.IdLast,id,MF_BYCOMMAND);
	}

	//
	// Set SubMenu state
	//
	UpdateUI_QueryAreEnabledSubMenuItems(hMenu,pfnQueryCmdStateCallback,lParam);
}
