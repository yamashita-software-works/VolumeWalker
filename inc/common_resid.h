// File commands
#define ID_FILE_NEW                     0xE100
#define ID_FILE_OPEN                    0xE101
#define ID_FILE_CLOSE                   0xE102
#define ID_FILE_SAVE                    0xE103
#define ID_FILE_SAVE_AS                 0xE104
#define ID_FILE_UPDATE                  0xE10A
#define ID_FILE_SAVE_COPY_AS            0xE10B

// Edit commands
#define ID_EDIT_CLEAR                   0xE120
#define ID_EDIT_CLEAR_ALL               0xE121
#define ID_EDIT_COPY                    0xE122
#define ID_EDIT_CUT                     0xE123
#define ID_EDIT_FIND                    0xE124
#define ID_EDIT_PASTE                   0xE125
#define ID_EDIT_PASTE_LINK              0xE126
#define ID_EDIT_PASTE_SPECIAL           0xE127
#define ID_EDIT_REPEAT                  0xE128
#define ID_EDIT_REPLACE                 0xE129
#define ID_EDIT_SELECT_ALL              0xE12A
#define ID_EDIT_UNDO                    0xE12B
#define ID_EDIT_REDO                    0xE12C
#define ID_EDIT_DELETE                  ID_EDIT_CLEAR
#define ID_EDIT_FIND_NEXT               ID_EDIT_REPEAT
#define ID_EDIT_FIND_PREVIOUS           0xE12D

// Window commands
#define ID_WINDOW_NEW                   0xE130
#define ID_WINDOW_ARRANGE               0xE131
#define ID_WINDOW_CASCADE               0xE132
#define ID_WINDOW_TILE_HORZ             0xE133
#define ID_WINDOW_TILE_VERT             0xE134
#define ID_WINDOW_SPLIT                 0xE135
#define ATL_IDM_WINDOW_FIRST            0xE130
#define ATL_IDM_WINDOW_LAST             0xE13F
#define ATL_IDM_FIRST_MDICHILD          0xFF00  // window list starts here
#define ATL_IDM_LAST_MDICHILD           0xFFFD
#define ID_WINDOW_TABFIRST              0xFF00	// = ATL_IDM_FIRST_MDICHILD
#define ID_WINDOW_TABLAST               0xFFFD
#define ID_WINDOW_SHOWTABLIST           0xFFFE

#define ID_VIEW_TOOLBAR                 0xE800
#define ID_VIEW_STATUS_BAR              0xE801
#define ID_VIEW_REFRESH                 0xE803
#define ID_VIEW_RIBBON                  0xE804
#define ID_VIEW_UPDATE                  0xE111
