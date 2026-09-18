#pragma once

#define $UNUSED                          (0x0)
#define $STANDARD_INFORMATION            (0x10)
#define $ATTRIBUTE_LIST                  (0x20)
#define $FILE_NAME                       (0x30)
#define $OBJECT_ID                       (0x40)
#define $SECURITY_DESCRIPTOR             (0x50)
#define $VOLUME_NAME                     (0x60)
#define $VOLUME_INFORMATION              (0x70)
#define $DATA                            (0x80)
#define $INDEX_ROOT                      (0x90)
#define $INDEX_ALLOCATION                (0xA0)
#define $BITMAP                          (0xB0)
#define $SYMBOLIC_LINK                   (0xC0)
#define $EA_INFORMATION                  (0xD0)
#define $EA                              (0xE0)
#define $FIRST_USER_DEFINED_ATTRIBUTE    (0x100)
#define $END                             (0xFFFFFFFF)

#define FILE_NAME_NTFS                   (0x01)
#define FILE_NAME_DOS                    (0x02)

#pragma pack(8)

typedef struct _MULTI_SECTOR_HEADER {
    UCHAR  Signature[4];
    USHORT UpdateSequenceArrayOffset;
    USHORT UpdateSequenceArraySize;
} MULTI_SECTOR_HEADER, *PMULTI_SECTOR_HEADER; // sizeof == 0x8

typedef struct _MFT_SEGMENT_REFERENCE {
	union {
		struct {
			ULONG  SegmentNumberLowPart;
			USHORT SegmentNumberHighPart;
			USHORT SequenceNumber;
		};
		LONGLONG QuadPart;
	};
} MFT_SEGMENT_REFERENCE, *PMFT_SEGMENT_REFERENCE;

typedef MFT_SEGMENT_REFERENCE FILE_REFERENCE, *PFILE_REFERENCE;;

typedef USHORT UPDATE_SEQUENCE_NUMBER, *PUPDATE_SEQUENCE_NUMBER;
typedef LARGE_INTEGER LSN, *PLSN;
typedef LONGLONG VCN;
typedef UPDATE_SEQUENCE_NUMBER UPDATE_SEQUENCE_ARRAY[1];
typedef UPDATE_SEQUENCE_ARRAY *PUPDATE_SEQUENCE_ARRAY;

typedef struct _FILE_RECORD_SEGMENT_HEADER {
  MULTI_SECTOR_HEADER   MultiSectorHeader;     // +0x00  8bytes
  ULONGLONG             Usn;                   // +0x08  8bytes
  USHORT                SequenceNumber;        // +0x0A  2bytes
  USHORT                LinkCount;             // +0x0C  2bytes
  USHORT                FirstAttributeOffset;  // +0x0E  2bytes
  USHORT                Flags;                 // +0x10  2bytes
  ULONG                 ByteInUse;             // +0x12  4bytes
  ULONG                 BytesAllocated;        // +0x14  4bytes
  FILE_REFERENCE        BaseFileRecordSegment; // +0x16  8bytes
  USHORT                NextAttributeNumber;   // +0x1E  2bytes
  UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;   // array USHORT[1] 
} FILE_RECORD_SEGMENT_HEADER, *PFILE_RECORD_SEGMENT_HEADER; // sizeof==48byte

#define FILE_RECORD_SEGMENT_IN_USE   0x0001
#define FILE_FILE_NAME_INDEX_PRESENT 0x0002

// DeletedFile       = 0x0000
// ExistingFile      = 0x0001
// DeletedDirectory  = 0x0002
// ExistingDirectory = 0x0003

typedef ULONG ATTRIBUTE_TYPE_CODE;

typedef struct _ATTRIBUTE_RECORD_HEADER {
    ATTRIBUTE_TYPE_CODE TypeCode;           // +0x00
    ULONG               RecordLength;       // +0x04
    UCHAR               FormCode;           // +0x08
    UCHAR               NameLength;         // +0x09
    USHORT              NameOffset;         // +0x0A
    USHORT              Flags;              // +0x0C
    USHORT              Instance;           // +0x0E
    union {
        struct {
            ULONG    ValueLength;           // +0x10 The size of the attribute value, in bytes.
            USHORT   ValueOffset;           // +0x14 The offset to the value from the start of the attribute record, in bytes.
            USHORT   ResidentFlags;         // +0x16
        } Resident;
        struct {
            VCN      LowestVcn;             // +0x10
            VCN      HighestVcn;            // +0x18
            USHORT   MappingPairsOffset;    // +0x20
            UCHAR    CompressionUnit;       // +0x22
            UCHAR    Reserved[5];           // +0x23
            LONGLONG AllocatedLength;       // +0x28
            LONGLONG FileSize;              // +0x30
            LONGLONG ValidDataLength;       // +0x38
            LONGLONG TotalAllocated;        // +0x40
        } Nonresident;
    } Form;
} ATTRIBUTE_RECORD_HEADER, *PATTRIBUTE_RECORD_HEADER; // sizeof==72bytes

#define RESIDENT_FORM     0x00
#define NONRESIDENT_FORM  0x01

#define ATTRIBUTE_FLAG_COMPRESSION_MASK 0x00FF
#define ATTRIBUTE_FLAG_SPARSE           0x8000
#define ATTRIBUTE_FLAG_ENCRYPTED        0x4000

#define FILE_ATTRIBUTE_FILENAMESDIRECTORIES 0x10000000
#define FILE_ATTRIBUTE_SECUTIRYDESCRIPTORS  0x20000000

/*++
     Name   FileAttribute
    '$I30'  0x10000000    Filenames Directories
    '$SDH'  0x20000000    Security Descriptors  $Secure
    '$SII'                Security Ids          $Secure
    '$O'                  Object Ids            $ObjId
    '$O'                  Owner Ids             $Quota
    '$Q'                  Quotas                $Quota
    '$R'                  Reparse Points        $Reparse
--*/

typedef struct _STANDARD_INFORMATION {
    LONGLONG CreationTime;                  // +0x00
    LONGLONG LastModificationTime;          // +0x08
    LONGLONG LastChangeTime;                // +0x10
    LONGLONG LastAccessTime;                // +0x18
    ULONG FileAttributes;                   // +0x20
    ULONG MaximumVersions;                  // +0x24
    ULONG VersionNumber;                    // +0x28
    ULONG ClassId;                          // +0x2c
    ULONG OwnerId;                          // +0x30
    ULONG SecurityId;                       // +0x34   
    ULONGLONG QuotaCharged;                 // +0x38
    ULONGLONG Usn;                          // +0x40  
} STANDARD_INFORMATION;                     // sizeof==72bytes

typedef struct _BASIC_INFORMATION {
    LONGLONG CreationTime;                  // +0x00
    LONGLONG LastModificationTime;          // +0x08
    LONGLONG LastChangeTime;                // +0x10
    LONGLONG LastAccessTime;                // +0x18
    LONGLONG AllocatedLength;               // +0x20
    LONGLONG FileSize;                      // +0x28
    ULONG FileAttributes;                   // +0x30
    USHORT PackedEaSize;                    // +0x34
    USHORT Reserved;                        // +0x36
} BASIC_INFORMATION, *PBASIC_INFORMATION;   // sizeof==0x038

typedef struct _FILE_NAME {
    ULONGLONG ParentDirectory;              // +0x00
    BASIC_INFORMATION BasicInfo;            // +0x08
    UCHAR FileNameLength;                   // +0x40
    UCHAR Flags;                            // +0x41
    WCHAR FileName[1];                      // +0x42
} FILE_NAME, *PFILE_NAME;

typedef struct _INDEX_HEADER {
    ULONG FirstIndexEntry;                  // +0x00
    ULONG FirstFreeByte;                    // +0x04
    ULONG BytesAvailable;                   // +0x08
    UCHAR Flags;                            // +0x0C
    UCHAR Reserved[3];                      // +0x0D
} INDEX_HEADER,*PINDEX_HEADER;              // sizeof==16

typedef struct _INDEX_ROOT {
    ULONG IndexedAttributeType;             // +0x00
    ULONG CollationRule;                    // +0x04
    ULONG BytesPerIndexBuffer;              // +0x08
    UCHAR BlocksPerIndexBuffer;             // +0x0C
    UCHAR Reserved[3];                      // +0x0D
    INDEX_HEADER IndexHeader;               // +0x010
} INDEX_ROOT,*PINDEX_ROOT;                  // sizeof==32

typedef struct _INDEX_ENTRY {
    union {
        FILE_REFERENCE FileReference;       // +0x00
        struct 
        {
            USHORT DataOffset;              // +0x00  
            USHORT DataLength;              // +0x02
            ULONG  ReservedForZero;         // +0x04
        };
   } Form;
   USHORT Length;                           // +0x08
   USHORT AttributeLength;                  // +0x0A
   USHORT Flags;                            // +0x0C
   USHORT Reserved;                         // +0x0E
   FILE_NAME Name;                          // +0x10
} INDEX_ENTRY, *PINDEX_ENTRY;               // sizeof==16+sizeof(FILE_NAME)

#define INDEX_ENTRY_NODE                 (0x0001)
#define INDEX_ENTRY_END                  (0x0002)

typedef struct _INDEX_ALLOCATION
{    
    MULTI_SECTOR_HEADER MultiSectorHeader;  // +0x00
    LSN Lsn;                                // +0x08
    VCN ThisBlock;                          // +0x10
    INDEX_HEADER IndexHeader;               // +0x18
    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray; // +0x028
} INDEX_ALLOCATION,*PINDEX_ALLOCATION;

typedef struct _ATTRIBUTE_LIST_ENTRY {
	ATTRIBUTE_TYPE_CODE   AttributeTypeCode;
	USHORT                RecordLength;
	UCHAR                 AttributeNameLength;
	UCHAR                 AttributeNameOffset;
	VCN                   LowestVcn;
	MFT_SEGMENT_REFERENCE SegmentReference;
	USHORT                Reserved;
	WCHAR                 AttributeName[1];
} ATTRIBUTE_LIST_ENTRY, *PATTRIBUTE_LIST_ENTRY;

#pragma pack()

inline ULONG RunLength(PUCHAR run)
{
    return(*run & 0x0f) + ((*run >> 4) & 0x0f) + 1;
}

inline LONGLONG RunLCN(PUCHAR run)
{
   UCHAR n1 = *run & 0x0f;
   UCHAR n2 = (*run >> 4) & 0x0f;
   LONGLONG lcn = (n2 == 0) ? 0 : (CHAR)(run[n1 + n2]);
   LONG i = 0;
   for (i = n1 + n2 - 1; i > n1; i--)
       lcn = (lcn << 8) + run[i];
   return lcn;
}

inline LONGLONG RunCount(PUCHAR run)
{
    UCHAR n = *run & 0xf;
    LONGLONG count = 0;
    ULONG i = 0;
    for (i = n; i > 0; i--)
        count = (count << 8) + run[i];
    return count;
}

#define _ADDOFFSET(p,off)  (((PUCHAR)p)+((ULONG)off))
