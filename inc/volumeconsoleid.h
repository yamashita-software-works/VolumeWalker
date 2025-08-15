#ifndef _DEF_VOLUME_CONSOLE_ID
#define _DEF_VOLUME_CONSOLE_ID

enum {
    VOLUME_CONSOLE_NONE=0,
    VIEW_CURRENT=VOLUME_CONSOLE_NONE,          /* 0 */
    VOLUME_CONSOLE_HOME,                       /* 1 */
    VOLUME_CONSOLE_ROOT,                       /* 2 */
    VOLUME_CONSOLE_VOLUMELIST,                 /* 3 */
    VOLUME_CONSOLE_PHYSICALDRIVELIST,          /* 4 */
    VOLUME_CONSOLE_SHADOWCOPYLIST,             /* 5 */
    VOLUME_CONSOLE_STORAGEDEVICE,              /* 6 */
    VOLUME_CONSOLE_MOUNTEDDEVICE,              /* 7 */
    VOLUME_CONSOLE_DOSDRIVELIST,               /* 8 */
    VOLUME_CONSOLE_VOLUMEINFORMAION,           /* 9 */
    VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION,    /* 10 */
    VOLUME_CONSOLE_DISKLAYOUT,                 /* 11 */
    VOLUME_CONSOLE_FILESYSTEMSTATISTICS,       /* 12 */
    VOLUME_CONSOLE_SIMPLEHEXDUMP,              /* 13 */
    VOLUME_CONSOLE_DISKPERFORMANCE,            /* 14 */
    VOLUME_CONSOLE_VOLUMEMOUNTPOINT,           /* 15 */
    VOLUME_CONSOLE_ENCRYPTIONVOLUME,           /* 16 */
    VOLUME_CONSOLE_RELATIONVIEW,               /* 17 */
                                               /* 18 */
                                               /* 19 */
    VOLUME_CONSOLE_FILTERDRIVER=20,            /* 20 */
    VOLUME_CONSOLE_VOLUMEFILELIST,             /* 21 */
    VOLUME_CONSOLE_VOLUMEFILEMANAGER=VOLUME_CONSOLE_VOLUMEFILELIST, /* 21 */
    VOLUME_CONSOLE_VOLUMEFILESEARCHRESULT,     /* 22 */
    VOLUME_CONSOLE_MAX_ID,
    VOLUME_CONSOLE_COUNT = VOLUME_CONSOLE_MAX_ID,
};

#endif
