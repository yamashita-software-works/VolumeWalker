#ifndef _STRING_DEF
#define _STRING_DEF

#define diBase    0
#define diTitle   (diBase+1)
#define diName    (diBase+2)

//
// Common
//
#define diGenericBase                            10
#define diGenericMax                             99

#define diNtfsBase                               100
#define diNtfsMax                                199

#define diUdfBase                                200
#define diUdfMax                                 299

#define diPhysicalDriveBase                      300
#define diPhysicalDriveMax                       399

#define diRefsBase                               400
#define diRefsMax                                499

//
// Volume Information
//
#define diTotalAllocationUnits                   (diGenericBase+1)
#define diAvailableAllocationUnits               (diGenericBase+2)
#define diBytesPerSector                         (diGenericBase+3)
#define diSectorsPerAllocationUnit               (diGenericBase+4)
#define diVolumeLabel                            (diGenericBase+7)
#define diUniqueId                               (diGenericBase+8)
#define diSerialNumber                           (diGenericBase+9)
#define diFileSystem                             (diGenericBase+10)
#define diCreationTime                           (diGenericBase+11)
#define diObjectId                               (diGenericBase+12)
#define diSupportsObjects                        (diGenericBase+13)
#define diDeviceNumber                           (diGenericBase+14)
#define diDirtyBit                               (diGenericBase+15)
#define diRetrievalPointerBase                   (diGenericBase+16)
#define diSize                                   (diGenericBase+17)
#define diFree                                   (diGenericBase+18)
#define diUsage                                  (diGenericBase+19)
#define diNtDeviceName                           (diGenericBase+20)
#define diVolumeGuidName                         (diGenericBase+21)
#define diDriveName                              (diGenericBase+22)

#define diNtfsVolumeSerialNumber                 (diNtfsBase + 0)
#define diNtfsNumberSectors                      (diNtfsBase + 1)
#define diNtfsTotalClusters                      (diNtfsBase + 2)
#define diNtfsFreeClusters                       (diNtfsBase + 3)
#define diNtfsTotalReserved                      (diNtfsBase + 4)   
#define diNtfsBytesPerSector                     (diNtfsBase + 5)
#define diNtfsBytesPerCluster                    (diNtfsBase + 6)
#define diNtfsBytesPerFileRecordSegment          (diNtfsBase + 7)
#define diNtfsClustersPerFileRecordSegment       (diNtfsBase + 8)
#define diNtfsMftValidDataLength                 (diNtfsBase + 9)
#define diNtfsMftStartLcn                        (diNtfsBase + 10)
#define diNtfsMft2StartLcn                       (diNtfsBase + 11)
#define diNtfsMftZoneStart                       (diNtfsBase + 12)
#define diNtfsMftZoneEnd                         (diNtfsBase + 13)

#define diUdfDirectoryCount                      (diUdfBase + 0)
#define diUdfFileCount                           (diUdfBase + 1)
#define diUdfFsFormatVersion                     (diUdfBase + 2)
#define diUdfFsFormatName                        (diUdfBase + 3)
#define diUdfFormatTime                          (diUdfBase + 4) 
#define diUdfLastUpdateTime                      (diUdfBase + 5)
#define diUdfCopyrightInfo                       (diUdfBase + 6)
#define diUdfAbstractInfo                        (diUdfBase + 7)
#define diUdfFormattingImplementationInfo        (diUdfBase + 8) 
#define diUdfLastModifyingImplementationInfo     (diUdfBase + 9)

#define diRefsVersion                            (diRefsBase + 0)
#define diRefsMajorVersion                       (diRefsBase + 1)
#define diRefsMinorVersion                       (diRefsBase + 2)
#define diRefsBytesPerPhysicalSector             (diRefsBase + 3)
#define diRefsVolumeSerialNumber                 (diRefsBase + 4)
#define diRefsNumberSectors                      (diRefsBase + 5)
#define diRefsTotalClusters                      (diRefsBase + 6)
#define diRefsFreeClusters                       (diRefsBase + 7)
#define diRefsTotalReserved                      (diRefsBase + 8)
#define diRefsBytesPerSector                     (diRefsBase + 9)
#define diRefsBytesPerCluster                    (diRefsBase + 10)
#define diRefsMaximumSizeOfResidentFile          (diRefsBase + 11)
#define diRefsFastTierDataFillRatio              (diRefsBase + 12)
#define diRefsSlowTierDataFillRatio              (diRefsBase + 13)
#define diRefsDestagesFastTierToSlowTierRate     (diRefsBase + 14)
#define diRefsMetadataChecksumType               (diRefsBase + 15)

//
// Physical Drive Information
//
#define diPhysicalDriveName                      (diPhysicalDriveBase + 0)
#define diPhysicalDiskSize                       (diPhysicalDriveBase + 1)
#define diPartitionStyle                         (diPhysicalDriveBase + 2)
#define diMBRSignature                           (diPhysicalDriveBase + 3)
#define diGPTDiskId                              (diPhysicalDriveBase + 4)
#define diGPTStartingUsableOffset                (diPhysicalDriveBase + 5)
#define diGPTUsableLength                        (diPhysicalDriveBase + 6)
#define diGPTMaxPartitionCount                   (diPhysicalDriveBase + 7)
#define diMediaType                              (diPhysicalDriveBase + 8)
#define diDiskSize                               (diPhysicalDriveBase + 9)
#define diCylinders                              (diPhysicalDriveBase + 10)
#define diTracksPerCylinder                      (diPhysicalDriveBase + 11)
#define diSectorsPerTrack                        (diPhysicalDriveBase + 12)
#define diDetection                              (diPhysicalDriveBase + 13)
#define diAlignmentBytesPerPhysicalSector        (diPhysicalDriveBase + 14)
#define diAlignmentBytesPerLogicalSector         (diPhysicalDriveBase + 15)
#define diAlignmentBytesOffsetForSectorAlignment (diPhysicalDriveBase + 16)
#define diAlignmentBytesPerCacheLine             (diPhysicalDriveBase + 17)
#define diAlignmentBytesOffsetForCacheAlignment  (diPhysicalDriveBase + 18)
#define diVendorId                               (diPhysicalDriveBase + 19)
#define diProductId                              (diPhysicalDriveBase + 20)
#define diProductRevision                        (diPhysicalDriveBase + 21)

//
// Statistics Definitions
//

//
// FILESYSTEM_STATISTICS/FILESYSTEM_STATISTICS_EX
//
#define diStatisticsBase                     (600)

#define diFileSystemType                     (diStatisticsBase + 0)
#define diStatisticsDataVersion              (diStatisticsBase + 1)
#define diSizeOfCompleteStructure            (diStatisticsBase + 2)
#define diUserFileReadBytes                  (diStatisticsBase + 3)
#define diUserDiskReads                      (diStatisticsBase + 4)
#define diUserFileReads                      (diStatisticsBase + 5)
#define diUserFileWrites                     (diStatisticsBase + 6)
#define diUserFileWriteBytes                 (diStatisticsBase + 7)
#define diUserDiskWrites                     (diStatisticsBase + 8)
#define diMetaDataReads                      (diStatisticsBase + 9)
#define diMetaDataReadBytes                  (diStatisticsBase + 10)
#define diMetaDataDiskReads                  (diStatisticsBase + 11)
#define diMetaDataWrites                     (diStatisticsBase + 12)
#define diMetaDataWriteBytes                 (diStatisticsBase + 13)
#define diMetaDataDiskWrites                 (diStatisticsBase + 14)
#define diStatisticsMaxId                    (diStatisticsBase + 15)

//
// NTFS_STATISTICS/NTFS_STATISTICS_EX
//
#define diStatisticsNtfsBase                 (diStatisticsMaxId + 1)

#define diLogFileFullExceptions              (diStatisticsNtfsBase + 0)
#define diOtherExceptions                    (diStatisticsNtfsBase + 1)
#define diMftReads                           (diStatisticsNtfsBase + 2)
#define diMftReadBytes                       (diStatisticsNtfsBase + 3)
#define diMftWrites                          (diStatisticsNtfsBase + 4)
#define diMftWriteBytes                      (diStatisticsNtfsBase + 5)
#define diMftWritesUserLevel_Write           (diStatisticsNtfsBase + 6)
#define diMftWritesUserLevel_Create          (diStatisticsNtfsBase + 7)
#define diMftWritesUserLevel_SetInfo         (diStatisticsNtfsBase + 8)
#define diMftWritesUserLevel_Flush           (diStatisticsNtfsBase + 9)
#define diMftWritesFlushForLogFileFull       (diStatisticsNtfsBase + 10)
#define diMftWritesLazyWriter                (diStatisticsNtfsBase + 11)
#define diMftWritesUserRequest               (diStatisticsNtfsBase + 12)
#define diMft2Writes                         (diStatisticsNtfsBase + 13)
#define diMft2WriteBytes                     (diStatisticsNtfsBase + 14)
#define diMft2WritesUserLevel_Write          (diStatisticsNtfsBase + 15)
#define diMft2WritesUserLevel_Create         (diStatisticsNtfsBase + 16)
#define diMft2WritesUserLevel_SetInfo        (diStatisticsNtfsBase + 17)
#define diMft2WritesUserLevel_Flush          (diStatisticsNtfsBase + 18)
#define diMft2WritesFlushForLogFileFull      (diStatisticsNtfsBase + 19)
#define diMft2WritesLazyWriter               (diStatisticsNtfsBase + 20)
#define diMft2WritesUserRequest              (diStatisticsNtfsBase + 21)
#define diRootIndexReads                     (diStatisticsNtfsBase + 22)
#define diRootIndexReadBytes                 (diStatisticsNtfsBase + 23)
#define diRootIndexWrites                    (diStatisticsNtfsBase + 24)
#define diRootIndexWriteBytes                (diStatisticsNtfsBase + 25)
#define diBitmapReads                        (diStatisticsNtfsBase + 26)
#define diBitmapReadBytes                    (diStatisticsNtfsBase + 27)
#define diBitmapWrites                       (diStatisticsNtfsBase + 28)
#define diBitmapWriteBytes                   (diStatisticsNtfsBase + 29)
#define diBitmapWritesFlushForLogFileFull    (diStatisticsNtfsBase + 30)
#define diBitmapWritesLazyWriter             (diStatisticsNtfsBase + 31)
#define diBitmapWritesUserRequest            (diStatisticsNtfsBase + 32)
#define diBitmapWritesUserLevel_Write        (diStatisticsNtfsBase + 33)
#define diBitmapWritesUserLevel_Create       (diStatisticsNtfsBase + 34)
#define diBitmapWritesUserLevel_SetInfo      (diStatisticsNtfsBase + 35)
#define diBitmapWritesUserLevel_Flush        (diStatisticsNtfsBase + 36)
#define diMftBitmapReads                     (diStatisticsNtfsBase + 37)
#define diMftBitmapReadBytes                 (diStatisticsNtfsBase + 38)
#define diMftBitmapWrites                    (diStatisticsNtfsBase + 39)
#define diMftBitmapWriteBytes                (diStatisticsNtfsBase + 40)
#define diMftBitmapWritesFlushForLogFileFull (diStatisticsNtfsBase + 41)
#define diMftBitmapWritesLazyWriter          (diStatisticsNtfsBase + 42)
#define diMftBitmapWritesUserRequest         (diStatisticsNtfsBase + 43)
#define diMftBitmapWritesUserLevel_Write     (diStatisticsNtfsBase + 44)
#define diMftBitmapWritesUserLevel_Create    (diStatisticsNtfsBase + 45)
#define diMftBitmapWritesUserLevel_SetInfo   (diStatisticsNtfsBase + 46)
#define diMftBitmapWritesUserLevel_Flush     (diStatisticsNtfsBase + 47)
#define diUserIndexReads                     (diStatisticsNtfsBase + 48)
#define diUserIndexReadBytes                 (diStatisticsNtfsBase + 49)
#define diUserIndexWrites                    (diStatisticsNtfsBase + 50)
#define diUserIndexWriteBytes                (diStatisticsNtfsBase + 51)
#define diLogFileReads                       (diStatisticsNtfsBase + 52)
#define diLogFileReadBytes                   (diStatisticsNtfsBase + 53)
#define diLogFileWrites                      (diStatisticsNtfsBase + 54)
#define diLogFileWriteBytes                  (diStatisticsNtfsBase + 55)
#define diAllocate_Calls                     (diStatisticsNtfsBase + 56)
#define diAllocate_RunsReturned              (diStatisticsNtfsBase + 57)
#define diAllocate_Hints                     (diStatisticsNtfsBase + 58)
#define diAllocate_HintsHonored              (diStatisticsNtfsBase + 59)
#define diAllocate_Cache                     (diStatisticsNtfsBase + 60)
#define diAllocate_CacheMiss                 (diStatisticsNtfsBase + 61)
#define diAllocate_Clusters                  (diStatisticsNtfsBase + 62)
#define diAllocate_HintsClusters             (diStatisticsNtfsBase + 63)
#define diAllocate_CacheClusters             (diStatisticsNtfsBase + 64)
#define diAllocate_CacheMissClusters         (diStatisticsNtfsBase + 65)
#define diDiskResourcesExhausted             (diStatisticsNtfsBase + 67)
#define diVolumeTrimCount                    (diStatisticsNtfsBase + 68)
#define diVolumeTrimTime                     (diStatisticsNtfsBase + 69)
#define diVolumeTrimByteCount                (diStatisticsNtfsBase + 70)
#define diFileLevelTrimCount                 (diStatisticsNtfsBase + 71)
#define diFileLevelTrimTime                  (diStatisticsNtfsBase + 72)
#define diFileLevelTrimByteCount             (diStatisticsNtfsBase + 73)
#define diVolumeTrimSkippedCount             (diStatisticsNtfsBase + 74)
#define diVolumeTrimSkippedByteCount         (diStatisticsNtfsBase + 75)
#define diNtfsFillStatInfoFromMftRecordCalledCount                            (diStatisticsNtfsBase + 76)
#define diNtfsFillStatInfoFromMftRecordBailedBecauseOfAttributeListCount      (diStatisticsNtfsBase + 77)
#define diNtfsFillStatInfoFromMftRecordBailedBecauseOfNonResReparsePointCount (diStatisticsNtfsBase + 78)
#define diMftWritesUserLevel                 (diStatisticsNtfsBase + 79) 
#define diMft2WritesUserLevel                (diStatisticsNtfsBase + 80)
#define diStatisticsNtfsMaxId                (diStatisticsNtfsBase + 81)

//
// FAT_STATISTICS/EXFAT_STATISTICS
//
#define diStatisticsFatBase                  (diStatisticsNtfsMaxId+1)

#define diCreateHits                         (diStatisticsFatBase + 0)
#define diSuccessfulCreates                  (diStatisticsFatBase + 1)
#define diFailedCreates                      (diStatisticsFatBase + 2)
#define diNonCachedReads                     (diStatisticsFatBase + 3)
#define diNonCachedReadBytes                 (diStatisticsFatBase + 4)
#define diNonCachedWrites                    (diStatisticsFatBase + 5)
#define diNonCachedWriteBytes                (diStatisticsFatBase + 6)
#define diNonCachedDiskReads                 (diStatisticsFatBase + 7)
#define diNonCachedDiskWrites                (diStatisticsFatBase + 8)
#define diStatisticsFatMaxId                 (diStatisticsFatBase + 9)

#endif
