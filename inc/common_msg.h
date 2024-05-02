#ifndef _PRIVATE_COMMON_MESSAGE_
#define _PRIVATE_COMMON_MESSAGE_

#define PM_BASE (WM_APP+0x6600)

//
// PM_FINDITEM
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_FINDITEM (PM_BASE+10)

#define FIND_QUERYOPENDIALOG  0
#define FIND_OPENDIALOG       1
#define FIND_CLOSEDIALOG      2
#define FIND_SEARCH           3
#define FIND_SEARCH_NEXT      4

//
// PM_GETCURDIR
//   wParam  -
//   lParam  -
//   lResult -
//
#define PM_GETCURDIR (PM_BASE+20)


#endif