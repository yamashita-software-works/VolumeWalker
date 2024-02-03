#pragma once

#ifndef _WINFSCTL_
#define _WINFSCTL_

#define FSCTL_SET_EXTERNAL_BACKING              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 195, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_GET_EXTERNAL_BACKING              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 196, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_EXTERNAL_BACKING           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 197, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_ENUM_EXTERNAL_BACKING             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 198, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_ENUM_OVERLAY                      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 199, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_ADD_OVERLAY                       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 204, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_REMOVE_OVERLAY                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 205, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_UPDATE_OVERLAY                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 206, METHOD_BUFFERED, FILE_WRITE_DATA)

#endif

#ifndef _NO_NEW_WINDEFS_

#define FSCTL_RKF_INTERNAL                                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 171, METHOD_NEITHER,  FILE_ANY_ACCESS) // Resume Key Filter
#define FSCTL_SCRUB_DATA                                          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 172, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REPAIR_COPIES                                       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 173, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_DISABLE_LOCAL_BUFFERING                             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 174, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_MGMT_LOCK                                       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 175, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_QUERY_DOWN_LEVEL_FILE_SYSTEM_CHARACTERISTICS    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 176, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_ADVANCE_FILE_ID                                     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 177, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_SYNC_TUNNEL_REQUEST                             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 178, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_QUERY_VETO_FILE_DIRECT_IO                       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 179, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_WRITE_USN_REASON                                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 180, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_CONTROL                                         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 181, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_REFS_VOLUME_DATA                                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 182, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_H_BREAKING_SYNC_TUNNEL_REQUEST                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 185, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_STORAGE_CLASSES                               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 187, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_REGION_INFO                                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 188, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_USN_TRACK_MODIFIED_RANGES                           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 189, METHOD_BUFFERED, FILE_ANY_ACCESS) // USN_TRACK_MODIFIED_RANGES
#define FSCTL_QUERY_SHARED_VIRTUAL_DISK_SUPPORT                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 192, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SVHDX_SYNC_TUNNEL_REQUEST                           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 193, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SVHDX_SET_INITIATOR_INFORMATION                     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 194, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_EXTERNAL_BACKING                                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 195, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_GET_EXTERNAL_BACKING                                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 196, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_EXTERNAL_BACKING                             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 197, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_ENUM_EXTERNAL_BACKING                               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 198, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_ENUM_OVERLAY                                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 199, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_ADD_OVERLAY                                         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 204, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_REMOVE_OVERLAY                                      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 205, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_UPDATE_OVERLAY                                      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 206, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_SHUFFLE_FILE                                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 208, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // SHUFFLE_FILE_DATA
#define FSCTL_DUPLICATE_EXTENTS_TO_FILE                           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 209, METHOD_BUFFERED, FILE_WRITE_DATA )
#define FSCTL_SPARSE_OVERALLOCATE                                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 211, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_STORAGE_QOS_CONTROL                                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 212, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_INITIATE_FILE_METADATA_OPTIMIZATION                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 215, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_QUERY_FILE_METADATA_OPTIMIZATION                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 216, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_SVHDX_ASYNC_TUNNEL_REQUEST                          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 217, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_WOF_VERSION                                     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 218, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_HCS_SYNC_TUNNEL_REQUEST                             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 219, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_HCS_ASYNC_TUNNEL_REQUEST                            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 220, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_EXTENT_READ_CACHE_INFO                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 221, METHOD_NEITHER,  FILE_ANY_ACCESS) // VCN_RANGE_INPUT_BUFFER, EXTENT_READ_CACHE_INFO_BUFFER
#define FSCTL_QUERY_REFS_VOLUME_COUNTER_INFO                      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 222, METHOD_NEITHER,  FILE_ANY_ACCESS) // REFS_VOLUME_COUNTER_INFO_INPUT_BUFFER, VOLUME_REFS_INFO_BUFFER
#define FSCTL_CLEAN_VOLUME_METADATA                               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 223, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_INTEGRITY_INFORMATION_EX                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 224, METHOD_BUFFERED, FILE_ANY_ACCESS) // FSCTL_SET_INTEGRITY_INFORMATION_BUFFER_EX
#define FSCTL_SUSPEND_OVERLAY                                     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 225, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_VIRTUAL_STORAGE_QUERY_PROPERTY                      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 226, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_FILESYSTEM_GET_STATISTICS_EX                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 227, METHOD_BUFFERED, FILE_ANY_ACCESS) // FILESYSTEM_STATISTICS_EX
#define FSCTL_QUERY_VOLUME_CONTAINER_STATE                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 228, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_LAYER_ROOT                                      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 229, METHOD_BUFFERED, FILE_ANY_ACCESS) // CONTAINER_ROOT_INFO_INPUT CONTAINER_ROOT_INFO_OUTPUT
#define FSCTL_QUERY_DIRECT_ACCESS_EXTENTS                         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 230, METHOD_NEITHER,  FILE_ANY_ACCESS) // QUERY_DIRECT_ACCESS_EXTENTS
#define FSCTL_NOTIFY_STORAGE_SPACE_ALLOCATION                     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 231, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SSDI_STORAGE_REQUEST                                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 232, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_DIRECT_IMAGE_ORIGINAL_BASE                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 233, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_READ_UNPRIVILEGED_USN_JOURNAL                       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 234,  METHOD_NEITHER, FILE_ANY_ACCESS) // READ_USN_JOURNAL_DATA, USN
#define FSCTL_GHOST_FILE_EXTENTS                                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 235, METHOD_BUFFERED, FILE_WRITE_ACCESS) // FSCTL_GHOST_FILE_EXTENTS_INPUT_BUFFER
#define FSCTL_QUERY_GHOSTED_FILE_EXTENTS                          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 236, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_UNMAP_SPACE                                         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 237, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_HCS_SYNC_NO_WRITE_TUNNEL_REQUEST                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 238, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_START_VIRTUALIZATION_INSTANCE                       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 240, METHOD_BUFFERED, FILE_ANY_ACCESS) // VIRTUALIZATION_INSTANCE_INFO_INPUT, VIRTUALIZATION_INSTANCE_INFO_OUTPUT
#define FSCTL_GET_FILTER_FILE_IDENTIFIER                          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 241, METHOD_BUFFERED, FILE_ANY_ACCESS) // GET_FILTER_FILE_IDENTIFIER_INPUT, GET_FILTER_FILE_IDENTIFIER_OUTPUT
#define FSCTL_STREAMS_QUERY_PARAMETERS                            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 241, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_STREAMS_ASSOCIATE_ID                                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 242, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_STREAMS_QUERY_ID                                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 243, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_RETRIEVAL_POINTERS_AND_REFCOUNT                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 244, METHOD_NEITHER,  FILE_ANY_ACCESS) // STARTING_VCN_INPUT_BUFFER, RETRIEVAL_POINTERS_AND_REFCOUNT_BUFFER
#define FSCTL_QUERY_VOLUME_NUMA_INFO                              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 245, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REFS_DEALLOCATE_RANGES                              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 246, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_REFS_SMR_VOLUME_INFO                          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 247, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_REFS_SMR_VOLUME_GC_PARAMETERS                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 248, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_REFS_FILE_STRICTLY_SEQUENTIAL                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 249, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DUPLICATE_EXTENTS_TO_FILE_EX                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 250, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_QUERY_BAD_RANGES                                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 251, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_DAX_ALLOC_ALIGNMENT_HINT                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 252, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_CORRUPTED_REFS_CONTAINER                     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 253, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SCRUB_UNDISCOVERABLE_ID                             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 254, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NOTIFY_DATA_CHANGE                                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 255, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_START_VIRTUALIZATION_INSTANCE_EX                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 256, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_ENCRYPTION_KEY_CONTROL                              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 257, METHOD_BUFFERED, FILE_ANY_ACCESS)  // protect/unprotect under DPL
#define FSCTL_VIRTUAL_STORAGE_SET_BEHAVIOR                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 258, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_REPARSE_POINT_EX                                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 259, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // REPARSE_DATA_BUFFER_EX
#define FSCTL_REARRANGE_FILE                                      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 264, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // REARRANGE_FILE_DATA
#define FSCTL_VIRTUAL_STORAGE_PASSTHROUGH                         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 265, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_RETRIEVAL_POINTER_COUNT                         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 266, METHOD_NEITHER,  FILE_ANY_ACCESS) // STARTING_VCN_INPUT_BUFFER, RETRIEVAL_POINTER_COUNT
#define FSCTL_ENABLE_PER_IO_FLAGS                                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 267, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_ASYNC_DUPLICATE_EXTENTS_STATUS                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 268, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SMB_SHARE_FLUSH_AND_PURGE                           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 271, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REFS_STREAM_SNAPSHOT_MANAGEMENT                     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 272, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_MANAGE_BYPASS_IO                                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 274, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REFS_DEALLOCATE_RANGES_EX                           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 275, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_CACHED_RUNS_STATE                               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 276, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REFS_SET_VOLUME_COMPRESSION_INFO                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 277, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REFS_QUERY_VOLUME_COMPRESSION_INFO                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 278, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DUPLICATE_CLUSTER                                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 279, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CREATE_LCN_WEAK_REFERENCE                           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 280, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_LCN_WEAK_REFERENCE                           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 281, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_LCN_WEAK_REFERENCE                            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 282, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_LCN_WEAK_REFERENCES                          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 283, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REFS_SET_VOLUME_DEDUP_INFO                          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 284, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REFS_QUERY_VOLUME_DEDUP_INFO                        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 285, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LMR_QUERY_INFO                                      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 286, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//==================== FSCTL_QUERY_REFS_VOLUME_COUNTER_INFO ======================
//
// Structure for FSCTL_QUERY_REFS_VOLUME_COUNTER_INFO
//

typedef struct _REFS_VOLUME_COUNTER_INFO_INPUT_BUFFER {

    BOOLEAN ResetCounters;

} REFS_VOLUME_COUNTER_INFO_INPUT_BUFFER, *PREFS_VOLUME_COUNTER_INFO_INPUT_BUFFER;

typedef struct _VOLUME_REFS_INFO_BUFFER {

    //
    //  These values will not be affected at reset
    //

    LARGE_INTEGER CacheSizeInBytes;
    LARGE_INTEGER AllocatedCacheInBytes;
    LARGE_INTEGER PopulatedCacheInBytes;
    LARGE_INTEGER InErrorCacheInBytes;
    LARGE_INTEGER MemoryUsedForCacheMetadata;
    ULONG CacheLineSize;
    LONG CacheTransactionsOutstanding;
    LONG CacheLinesFree;
    LONG CacheLinesInError;

    //
    //  These values will be affected at reset
    //

    LARGE_INTEGER CacheHitsInBytes;
    LARGE_INTEGER CacheMissesInBytes;
    LARGE_INTEGER CachePopulationUpdatesInBytes;
    LARGE_INTEGER CacheWriteThroughUpdatesInBytes;
    LARGE_INTEGER CacheInvalidationsInBytes;
    LARGE_INTEGER CacheOverReadsInBytes;
    LARGE_INTEGER MetadataWrittenBytes;

    LONG CacheHitCounter;
    LONG CacheMissCounter;
    LONG CacheLineAllocationCounter;
    LONG CacheInvalidationsCounter;
    LONG CachePopulationUpdatesCounter;
    LONG CacheWriteThroughUpdatesCounter;
    LONG MaxCacheTransactionsOutstanding;

//  LONG TimeRequiredForCacheHitIn100us;
//  LONG TimeRequiredForCacheMissIn100us;

    LONG DataWritesReallocationCount;
    LONG DataInPlaceWriteCount;
    LONG MetadataAllocationsFastTierCount;
    LONG MetadataAllocationsSlowTierCount;
    LONG DataAllocationsFastTierCount;
    LONG DataAllocationsSlowTierCount;

    LONG DestagesSlowTierToFastTier;
    LONG DestagesFastTierToSlowTier;
    LONG SlowTierDataFillRatio;
    LONG FastTierDataFillRatio;
    LONG SlowTierMetadataFillRatio;
    LONG FastTierMetadataFillRatio;

    LONG SlowToFastDestageReadLatency;
    LONG SlowToFastDestageReadLatencyBase;

    LONG SlowToFastDestageWriteLatency;
    LONG SlowToFastDestageWriteLatencyBase;

    LONG FastToSlowDestageReadLatency;
    LONG FastToSlowDestageReadLatencyBase;

    LONG FastToSlowDestageWriteLatency;
    LONG FastToSlowDestageWriteLatencyBase;

    LONG SlowTierContainerFillRatio;
    LONG SlowTierContainerFillRatioBase;

    LONG FastTierContainerFillRatio;
    LONG FastTierContainerFillRatioBase;

    LONG TreeUpdateLatency;
    LONG TreeUpdateLatencyBase;

    LONG CheckpointLatency;
    LONG CheckpointLatencyBase;

    LONG TreeUpdateCount;
    LONG CheckpointCount;
    LONG LogWriteCount;
    LONG LogFillRatio;

    LONG ReadCacheInvalidationsForOverwrite;
    LONG ReadCacheInvalidationsForReuse;
    LONG ReadCacheInvalidationsGeneral;

    LONG ReadCacheChecksOnMount;
    LONG ReadCacheIssuesOnMount;

    LONG TrimLatency;
    LONG TrimLatencyBase;

    LONG DataCompactionCount;

    LONG CompactionReadLatency;
    LONG CompactionReadLatencyBase;

    LONG CompactionWriteLatency;
    LONG CompactionWriteLatencyBase;

    LARGE_INTEGER DataInPlaceWriteClusterCount;

    LONG CompactionFailedDueToIneligibleContainer;
    LONG CompactionFailedDueToMaxFragmentation;

    LONG CompactedContainerFillRatio;
    LONG CompactedContainerFillRatioBase;

    LONG ContainerMoveRetryCount;
    LONG ContainerMoveFailedDueToIneligibleContainer;

    LONG CompactionFailureCount;
    LONG ContainerMoveFailureCount;

    LARGE_INTEGER NumberOfDirtyMetadataPages;
    LONG NumberOfDirtyTableListEntries;
    LONG NumberOfDeleteQueueEntries;

} VOLUME_REFS_INFO_BUFFER, *PVOLUME_REFS_INFO_BUFFER;

//
//==================== FSCTL_QUERY_REFS_SMR_VOLUME_INFO =======================
//

#define REFS_SMR_VOLUME_INFO_OUTPUT_VERSION_V0      0
#define REFS_SMR_VOLUME_INFO_OUTPUT_VERSION_V1      1

typedef enum _REFS_SMR_VOLUME_GC_STATE {

    SmrGcStateInactive = 0,
    SmrGcStatePaused = 1,
    SmrGcStateActive = 2,
    SmrGcStateActiveFullSpeed = 3,

} REFS_SMR_VOLUME_GC_STATE, *PREFS_SMR_VOLUME_GC_STATE;

typedef struct _REFS_SMR_VOLUME_INFO_OUTPUT {

    DWORD Version;
    DWORD Flags;

    LARGE_INTEGER SizeOfRandomlyWritableTier;
    LARGE_INTEGER FreeSpaceInRandomlyWritableTier;
    LARGE_INTEGER SizeofSMRTier;
    LARGE_INTEGER FreeSpaceInSMRTier;
    LARGE_INTEGER UsableFreeSpaceInSMRTier;

    REFS_SMR_VOLUME_GC_STATE VolumeGcState;
    DWORD    VolumeGcLastStatus;

    //
    //  Fields added in V1
    //

    DWORD CurrentGcBandFillPercentage;

    DWORDLONG Unused[6];

} REFS_SMR_VOLUME_INFO_OUTPUT, *PREFS_SMR_VOLUME_INFO_OUTPUT;

//
//==================== FSCTL_SET_REFS_SMR_VOLUME_GC_PARAMETERS =======================
//

#define REFS_SMR_VOLUME_GC_PARAMETERS_VERSION_V1    1

typedef enum _REFS_SMR_VOLUME_GC_ACTION {

    SmrGcActionStart = 1,
    SmrGcActionStartFullSpeed = 2,
    SmrGcActionPause = 3,
    SmrGcActionStop = 4,

} REFS_SMR_VOLUME_GC_ACTION, *PREFS_SMR_VOLUME_GC_ACTION;

typedef enum _REFS_SMR_VOLUME_GC_METHOD {

    SmrGcMethodCompaction = 1,
    SmrGcMethodCompression = 2,
    SmrGcMethodRotation = 3,

} REFS_SMR_VOLUME_GC_METHOD, *PREFS_SMR_VOLUME_GC_METHOD;

typedef struct _REFS_SMR_VOLUME_GC_PARAMETERS {

    DWORD Version;
    DWORD Flags;

    REFS_SMR_VOLUME_GC_ACTION Action;
    REFS_SMR_VOLUME_GC_METHOD Method;

    DWORD IoGranularity;
    DWORD CompressionFormat;

    DWORDLONG Unused[8];

} REFS_SMR_VOLUME_GC_PARAMETERS, *PREFS_SMR_VOLUME_GC_PARAMETERS;

//
//==================== STREAMS CONSTANTS =======================
//

#define STREAMS_INVALID_ID                      (0)
#define STREAMS_MAX_ID                          (MAXWORD  )

//
//==================== FSCTL_STREAMS_QUERY_PARAMETERS =======================
//

typedef struct _STREAMS_QUERY_PARAMETERS_OUTPUT_BUFFER {

    DWORD OptimalWriteSize;
    DWORD StreamGranularitySize;
    DWORD StreamIdMin;
    DWORD StreamIdMax;

} STREAMS_QUERY_PARAMETERS_OUTPUT_BUFFER, *PSTREAMS_QUERY_PARAMETERS_OUTPUT_BUFFER;

//
//==================== FSCTL_STREAMS_ASSOCIATE_ID =======================
//

#define STREAMS_ASSOCIATE_ID_CLEAR              (0x1)
#define STREAMS_ASSOCIATE_ID_SET                (0x2)

typedef struct _STREAMS_ASSOCIATE_ID_INPUT_BUFFER {

    DWORD Flags;
    DWORD StreamId;

} STREAMS_ASSOCIATE_ID_INPUT_BUFFER, *PSTREAMS_ASSOCIATE_ID_INPUT_BUFFER;

//
//==================== FSCTL_STREAMS_QUERY_ID =======================
//

typedef struct _STREAMS_QUERY_ID_OUTPUT_BUFFER {

    DWORD StreamId;

} STREAMS_QUERY_ID_OUTPUT_BUFFER, *PSTREAMS_QUERY_ID_OUTPUT_BUFFER;


//
//==================== FSCTL_GET_REFS_VOLUME_DATA ======================
//
// Structures for FSCTL_GET_REFS_VOLUME_DATA.
//

typedef struct {

    ULONG ByteCount;
    ULONG MajorVersion;
    ULONG MinorVersion;

    ULONG BytesPerPhysicalSector;

    LARGE_INTEGER VolumeSerialNumber;
    LARGE_INTEGER NumberSectors;
    LARGE_INTEGER TotalClusters;
    LARGE_INTEGER FreeClusters;
    LARGE_INTEGER TotalReserved;
    ULONG BytesPerSector;
    ULONG BytesPerCluster;
    LARGE_INTEGER MaximumSizeOfResidentFile;

    USHORT FastTierDataFillRatio;               // between 0 and 10000
    USHORT SlowTierDataFillRatio;               // between 0 and 10000

    ULONG DestagesFastTierToSlowTierRate;       // in clusters per second

    USHORT  MetadataChecksumType;

    UCHAR  Reserved0[6];
    LARGE_INTEGER Reserved[8];

} REFS_VOLUME_DATA_BUFFER, *PREFS_VOLUME_DATA_BUFFER;

#endif

#ifndef FILE_DAX_VOLUME
#define FILE_DAX_VOLUME                     0x20000000
#endif

#ifndef FILE_RETURNS_CLEANUP_RESULT_INFO
#define FILE_RETURNS_CLEANUP_RESULT_INFO    0x00000200  
#endif

#ifndef FILE_SUPPORTS_POSIX_UNLINK_RENAME
#define FILE_SUPPORTS_POSIX_UNLINK_RENAME   0x00000400  
#endif

#ifndef FILE_SUPPORTS_SPARSE_VDL
#define FILE_SUPPORTS_SPARSE_VDL            0x10000000
#endif

#ifndef FILE_SUPPORTS_BLOCK_REFCOUNTING
#define FILE_SUPPORTS_BLOCK_REFCOUNTING     0x08000000
#endif

#ifndef FILE_SUPPORTS_GHOSTING
#define FILE_SUPPORTS_GHOSTING              0x40000000
#endif

#pragma pack(1)
typedef struct _FILE_SYSTEM_RECOGNITION_STRUCTURE {
  UCHAR  Jmp[3];
  UCHAR  FsName[8];
  UCHAR  MustBeZero[5];
  ULONG  Identifier;
  USHORT Length;
  USHORT Checksum;
} FILE_SYSTEM_RECOGNITION_STRUCTURE;
#pragma pack()

#ifndef FILE_REMOVABLE_MEDIA

#define FILE_REMOVABLE_MEDIA                     0x00000001
#define FILE_READ_ONLY_DEVICE                    0x00000002
#define FILE_FLOPPY_DISKETTE                     0x00000004
#define FILE_WRITE_ONCE_MEDIA                    0x00000008
#define FILE_REMOTE_DEVICE                       0x00000010
#define FILE_DEVICE_IS_MOUNTED                   0x00000020
#define FILE_VIRTUAL_VOLUME                      0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME           0x00000080
#define FILE_DEVICE_SECURE_OPEN                  0x00000100
#define FILE_CHARACTERISTIC_PNP_DEVICE           0x00000800
#define FILE_CHARACTERISTIC_TS_DEVICE            0x00001000
#define FILE_CHARACTERISTIC_WEBDAV_DEVICE        0x00002000
#define FILE_CHARACTERISTIC_CSV                  0x00010000
#define FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL 0x00020000
#define FILE_PORTABLE_DEVICE                     0x00040000

#endif

#ifndef FILESYSTEM_STATISTICS_TYPE_REFS
#define FILESYSTEM_STATISTICS_TYPE_REFS 4
#endif

typedef struct _FILESYSTEM_STATISTICS_EX {

    USHORT FileSystemType;
    USHORT Version;                     // currently version 1

    ULONG SizeOfCompleteStructure;      // must by a multiple of 64 bytes

    ULONGLONG UserFileReads;
    ULONGLONG UserFileReadBytes;
    ULONGLONG UserDiskReads;
    ULONGLONG UserFileWrites;
    ULONGLONG UserFileWriteBytes;
    ULONGLONG UserDiskWrites;

    ULONGLONG MetaDataReads;
    ULONGLONG MetaDataReadBytes;
    ULONGLONG MetaDataDiskReads;
    ULONGLONG MetaDataWrites;
    ULONGLONG MetaDataWriteBytes;
    ULONGLONG MetaDataDiskWrites;

    //
    //  The file system's private structure is appended here.
    //

} FILESYSTEM_STATISTICS_EX, *PFILESYSTEM_STATISTICS_EX;

typedef struct _NTFS_STATISTICS_WIN8 {

    ULONG LogFileFullExceptions;
    ULONG OtherExceptions;

    //
    // Other meta data io's
    //

    ULONG MftReads;
    ULONG MftReadBytes;
    ULONG MftWrites;
    ULONG MftWriteBytes;
    struct {
        USHORT Write;
        USHORT Create;
        USHORT SetInfo;
        USHORT Flush;
    } MftWritesUserLevel;

    USHORT MftWritesFlushForLogFileFull;
    USHORT MftWritesLazyWriter;
    USHORT MftWritesUserRequest;

    ULONG Mft2Writes;
    ULONG Mft2WriteBytes;
    struct {
        USHORT Write;
        USHORT Create;
        USHORT SetInfo;
        USHORT Flush;
    } Mft2WritesUserLevel;

    USHORT Mft2WritesFlushForLogFileFull;
    USHORT Mft2WritesLazyWriter;
    USHORT Mft2WritesUserRequest;

    ULONG RootIndexReads;
    ULONG RootIndexReadBytes;
    ULONG RootIndexWrites;
    ULONG RootIndexWriteBytes;

    ULONG BitmapReads;
    ULONG BitmapReadBytes;
    ULONG BitmapWrites;
    ULONG BitmapWriteBytes;

    USHORT BitmapWritesFlushForLogFileFull;
    USHORT BitmapWritesLazyWriter;
    USHORT BitmapWritesUserRequest;

    struct {
        USHORT Write;
        USHORT Create;
        USHORT SetInfo;
    } BitmapWritesUserLevel;

    ULONG MftBitmapReads;
    ULONG MftBitmapReadBytes;
    ULONG MftBitmapWrites;
    ULONG MftBitmapWriteBytes;

    USHORT MftBitmapWritesFlushForLogFileFull;
    USHORT MftBitmapWritesLazyWriter;
    USHORT MftBitmapWritesUserRequest;

    struct {
        USHORT Write;
        USHORT Create;
        USHORT SetInfo;
        USHORT Flush;
    } MftBitmapWritesUserLevel;

    ULONG UserIndexReads;
    ULONG UserIndexReadBytes;
    ULONG UserIndexWrites;
    ULONG UserIndexWriteBytes;

    //
    // Additions for NT 5.0
    //

    ULONG LogFileReads;
    ULONG LogFileReadBytes;
    ULONG LogFileWrites;
    ULONG LogFileWriteBytes;

    struct {
        ULONG Calls;                // number of individual calls to allocate clusters
        ULONG Clusters;             // number of clusters allocated
        ULONG Hints;                // number of times a hint was specified

        ULONG RunsReturned;         // number of runs used to satisfy all the requests

        ULONG HintsHonored;         // number of times the hint was useful
        ULONG HintsClusters;        // number of clusters allocated via the hint
        ULONG Cache;                // number of times the cache was useful other than the hint
        ULONG CacheClusters;        // number of clusters allocated via the cache other than the hint
        ULONG CacheMiss;            // number of times the cache wasn't useful
        ULONG CacheMissClusters;    // number of clusters allocated without the cache
    } Allocate;

    //
    //  Additions for Windows 8.1
    //

    ULONG DiskResourcesExhausted;

    //
    //  All future expansion of this structure needs to be in NTFS_STATISTICS_EX starting Windows 10
    //

} NTFS_STATISTICS_WIN8, *PNTFS_STATISTICS_WIN8;

typedef struct _NTFS_STATISTICS_EX {

    ULONG LogFileFullExceptions;
    ULONG OtherExceptions;

    //
    // Other meta data io's
    //

    ULONGLONG MftReads;
    ULONGLONG MftReadBytes;
    ULONGLONG MftWrites;
    ULONGLONG MftWriteBytes;
    struct {
        ULONG Write;
        ULONG Create;
        ULONG SetInfo;
        ULONG Flush;
    } MftWritesUserLevel;

    ULONG MftWritesFlushForLogFileFull;
    ULONG MftWritesLazyWriter;
    ULONG MftWritesUserRequest;

    ULONGLONG Mft2Writes;
    ULONGLONG Mft2WriteBytes;
    struct {
        ULONG Write;
        ULONG Create;
        ULONG SetInfo;
        ULONG Flush;
    } Mft2WritesUserLevel;

    ULONG Mft2WritesFlushForLogFileFull;
    ULONG Mft2WritesLazyWriter;
    ULONG Mft2WritesUserRequest;

    ULONGLONG RootIndexReads;
    ULONGLONG RootIndexReadBytes;
    ULONGLONG RootIndexWrites;
    ULONGLONG RootIndexWriteBytes;

    ULONGLONG BitmapReads;
    ULONGLONG BitmapReadBytes;
    ULONGLONG BitmapWrites;
    ULONGLONG BitmapWriteBytes;

    ULONG BitmapWritesFlushForLogFileFull;
    ULONG BitmapWritesLazyWriter;
    ULONG BitmapWritesUserRequest;

    struct {
        ULONG Write;
        ULONG Create;
        ULONG SetInfo;
        ULONG Flush;
    } BitmapWritesUserLevel;

    ULONGLONG MftBitmapReads;
    ULONGLONG MftBitmapReadBytes;
    ULONGLONG MftBitmapWrites;
    ULONGLONG MftBitmapWriteBytes;

    ULONG MftBitmapWritesFlushForLogFileFull;
    ULONG MftBitmapWritesLazyWriter;
    ULONG MftBitmapWritesUserRequest;

    struct {
        ULONG Write;
        ULONG Create;
        ULONG SetInfo;
        ULONG Flush;
    } MftBitmapWritesUserLevel;

    ULONGLONG UserIndexReads;
    ULONGLONG UserIndexReadBytes;
    ULONGLONG UserIndexWrites;
    ULONGLONG UserIndexWriteBytes;

    //
    // Additions for NT 5.0
    //

    ULONGLONG LogFileReads;
    ULONGLONG LogFileReadBytes;
    ULONGLONG LogFileWrites;
    ULONGLONG LogFileWriteBytes;

    struct {
        ULONG Calls;                    // number of individual calls to allocate clusters
        ULONG RunsReturned;             // number of runs used to satisfy all the requests
        ULONG Hints;                    // number of times a hint was specified
        ULONG HintsHonored;             // number of times the hint was useful
        ULONG Cache;                    // number of times the cache was useful other than the hint
        ULONG CacheMiss;                // number of times the cache wasn't useful

        ULONGLONG Clusters;             // number of clusters allocated
        ULONGLONG HintsClusters;        // number of clusters allocated via the hint
        ULONGLONG CacheClusters;        // number of clusters allocated via the cache other than the hint
        ULONGLONG CacheMissClusters;    // number of clusters allocated without the cache
    } Allocate;

    //
    //  Additions for Windows 8.1
    //

    ULONG DiskResourcesExhausted;

    //
    //  Additions for Windows 10
    //

    ULONGLONG VolumeTrimCount;
    ULONGLONG VolumeTrimTime;
    ULONGLONG VolumeTrimByteCount;

    ULONGLONG FileLevelTrimCount;
    ULONGLONG FileLevelTrimTime;
    ULONGLONG FileLevelTrimByteCount;

    ULONGLONG VolumeTrimSkippedCount;
    ULONGLONG VolumeTrimSkippedByteCount;

    //
    //  Additions for NtfsFillStatInfoFromMftRecord
    //

    ULONGLONG NtfsFillStatInfoFromMftRecordCalledCount;
    ULONGLONG NtfsFillStatInfoFromMftRecordBailedBecauseOfAttributeListCount;
    ULONGLONG NtfsFillStatInfoFromMftRecordBailedBecauseOfNonResReparsePointCount;

} NTFS_STATISTICS_EX, *PNTFS_STATISTICS_EX;
