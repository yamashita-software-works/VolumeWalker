#ifndef _NTFILEID_
#define _NTFILEID_

//  We cant define FILE_ID_128 as a union of the UCHAR[16] with two LONGLONGs because that would
//  impose an alignment requirement that wouldn't otherwise exist.  That would change the in-memory
//  layout of structures that already embed FILE_ID_128 and/or make their accesses unaligned.
typedef struct _FILE_ID_128 {                               // winnt
    UCHAR Identifier[16];                                   // winnt
} FILE_ID_128, *PFILE_ID_128;                               // winnt

#define FILE_ID_IS_INVALID(FID) ((FID).QuadPart == FILE_INVALID_FILE_ID)

#define FILE_ID_128_IS_INVALID(FID128) (((FID128).Identifier[0] == (UCHAR)-1) &&    \
                                        ((FID128).Identifier[1] == (UCHAR)-1) &&    \
                                        ((FID128).Identifier[2] == (UCHAR)-1) &&    \
                                        ((FID128).Identifier[3] == (UCHAR)-1) &&    \
                                        ((FID128).Identifier[4] == (UCHAR)-1) &&    \
                                        ((FID128).Identifier[5] == (UCHAR)-1) &&    \
                                        ((FID128).Identifier[6] == (UCHAR)-1) &&    \
                                        ((FID128).Identifier[7] == (UCHAR)-1) &&    \
                                        ((FID128).Identifier[8] == (UCHAR)-1) &&    \
                                        ((FID128).Identifier[9] == (UCHAR)-1) &&    \
                                        ((FID128).Identifier[10] == (UCHAR)-1) &&   \
                                        ((FID128).Identifier[11] == (UCHAR)-1) &&   \
                                        ((FID128).Identifier[12] == (UCHAR)-1) &&   \
                                        ((FID128).Identifier[13] == (UCHAR)-1) &&   \
                                        ((FID128).Identifier[14] == (UCHAR)-1) &&   \
                                        ((FID128).Identifier[15] == (UCHAR)-1))

#define MAKE_INVALID_FILE_ID_128(FID128) {  \
    ((FID128).Identifier[0] = (UCHAR)-1);   \
    ((FID128).Identifier[1] = (UCHAR)-1);   \
    ((FID128).Identifier[2] = (UCHAR)-1);   \
    ((FID128).Identifier[3] = (UCHAR)-1);   \
    ((FID128).Identifier[4] = (UCHAR)-1);   \
    ((FID128).Identifier[5] = (UCHAR)-1);   \
    ((FID128).Identifier[6] = (UCHAR)-1);   \
    ((FID128).Identifier[7] = (UCHAR)-1);   \
    ((FID128).Identifier[8] = (UCHAR)-1);   \
    ((FID128).Identifier[9] = (UCHAR)-1);   \
    ((FID128).Identifier[10] = (UCHAR)-1);  \
    ((FID128).Identifier[11] = (UCHAR)-1);  \
    ((FID128).Identifier[12] = (UCHAR)-1);  \
    ((FID128).Identifier[13] = (UCHAR)-1);  \
    ((FID128).Identifier[14] = (UCHAR)-1);  \
    ((FID128).Identifier[15] = (UCHAR)-1);  \
}

typedef enum _FS_FILE_ID_TYPE
{ 
	FsFileIdType          = 0,
	FsObjectIdType        = 1,
	FsExtendedFileIdType  = 2,
	FsMaximumFileIdType
} FS_FILE_ID_TYPE, *PFS_FILE_ID_TYPE;

typedef struct
{
	ULONG dwSize;
	FS_FILE_ID_TYPE Type;
	union
	{
		LARGE_INTEGER FileId;
		GUID          ObjectId;
		FILE_ID_128   ExtendedFileId;
	};
} FS_FILE_ID_DESCRIPTOR, *PFS_FILE_ID_DESCRIPTOR;

#endif
