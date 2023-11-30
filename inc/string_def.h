#ifndef _STRING_DEF
#define _STRING_DEF

#define diBase    0
#define diTitle   (diBase+1)
#define diName    (diBase+2)

//
// Common
//
#define diGenericBase                            10
#define diNtfsBase                               100
#define diUdfBase                                200
#define diPhysicalDriveBase                      300

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

#endif
