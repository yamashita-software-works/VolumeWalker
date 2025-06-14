#ifndef _DEF_VOLUME_CONSOLE_ID
#define _DEF_VOLUME_CONSOLE_ID

enum {
	VOLUME_CONSOLE_NONE=0,
	VIEW_CURRENT=VOLUME_CONSOLE_NONE,
	VOLUME_CONSOLE_HOME,
	VOLUME_CONSOLE_ROOT,
	VOLUME_CONSOLE_VOLUMELIST,
	VOLUME_CONSOLE_PHYSICALDRIVELIST,
	VOLUME_CONSOLE_SHADOWCOPYLIST,
	VOLUME_CONSOLE_STORAGEDEVICE,
	VOLUME_CONSOLE_MOUNTEDDEVICE,
	VOLUME_CONSOLE_DOSDRIVELIST,
	VOLUME_CONSOLE_VOLUMEINFORMAION,
	VOLUME_CONSOLE_PHYSICALDRIVEINFORMAION,
	VOLUME_CONSOLE_DISKLAYOUT,
	VOLUME_CONSOLE_FILESYSTEMSTATISTICS,
	VOLUME_CONSOLE_SIMPLEHEXDUMP,
	VOLUME_CONSOLE_DISKPERFORMANCE,
	VOLUME_CONSOLE_VOLUMEMOUNTPOINT,      /* 15 */
	VOLUME_CONSOLE_ENCRYPTIONVOLUME,      /* 16 */
	VOLUME_CONSOLE_RELATIONVIEW,          /* 17 */
/* 18 */
/* 19 */
	VOLUME_CONSOLE_FILTERDRIVER=20,       /* 20 */
	VOLUME_CONSOLE_VOLUMEFILELIST,        /* 21 */
	VOLUME_CONSOLE_MAX_ID,
	VOLUME_CONSOLE_COUNT = VOLUME_CONSOLE_MAX_ID,
};

#endif
