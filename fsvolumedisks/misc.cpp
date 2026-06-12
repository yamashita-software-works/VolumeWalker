//***************************************************************************
//*                                                                         *
//*  misc.cpp                                                               *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Create: 2024-08-08                                                     *
//*                                                                         *
//***************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"

#pragma comment(lib, "propsys.lib")

//---------------------------------------------------------------------------
//
//  GetBitLockerProtectionFromShellProp()
//
//  PURPOSE: Get BitLocker state from shell Property value.
//
//  NOTE:    Values of System.Volume.BitLockerProtection determined
//           empirically on Windows 10 1909 (10.0.18363.1082):
//
//---------------------------------------------------------------------------

/*++ References

| System.Volume.      | Control Panel                    | manage-bde conversion     | manage-bde     | Get-BitlockerVolume          | Get-BitlockerVolume |
| BitLockerProtection |                                  |                           | protection     | VolumeStatus                 | ProtectionStatus    |
| ------------------- | -------------------------------- | ------------------------- | -------------- | ---------------------------- | ------------------- |
|                   1 | BitLocker on                     | Used Space Only Encrypted | Protection On  | FullyEncrypted               | On                  |
|                   1 | BitLocker on                     | Fully Encrypted           | Protection On  | FullyEncrypted               | On                  |
|                   1 | BitLocker on                     | Fully Encrypted           | Protection On  | FullyEncryptedWipeInProgress | On                  |
|                   2 | BitLocker off                    | Fully Decrypted           | Protection Off | FullyDecrypted               | Off                 |
|                   3 | BitLocker Encrypting             | Encryption In Progress    | Protection Off | EncryptionInProgress         | Off                 |
|                   3 | BitLocker Encryption Paused      | Encryption Paused         | Protection Off | EncryptionSuspended          | Off                 |
|                   4 | BitLocker Decrypting             | Decryption in progress    | Protection Off | DecyptionInProgress          | Off                 |
|                   4 | BitLocker Decryption Paused      | Decryption Paused         | Protection Off | DecryptionSuspended          | Off                 |
|                   5 | BitLocker suspended              | Used Space Only Encrypted | Protection Off | FullyEncrypted               | Off                 |
|                   5 | BitLocker suspended              | Fully Encrypted           | Protection Off | FullyEncrypted               | Off                 |
|                   6 | BitLocker on (Locked)            | Unknown                   | Unknown        | $null                        | Unknown             |
|                   7 |                                  |                           |                |                              |                     |
|                   8 | BitLocker waiting for activation | Used Space Only Encrypted | Protection Off | FullyEncrypted               | Off                 |

--*/

int GetBitLockerProtectionFromShellProp(wchar_t *pszDosDrive)
{
    HRESULT hr;
    IShellItem2 *drive = NULL;
    hr = SHCreateItemFromParsingName(pszDosDrive, NULL, IID_PPV_ARGS(&drive));

    if (SUCCEEDED(hr))
    {
        PROPERTYKEY pKey;
        hr = PSGetPropertyKeyFromName(L"System.Volume.BitLockerProtection", &pKey);
        if (SUCCEEDED(hr))
        {
            PROPVARIANT prop;
            PropVariantInit(&prop);
            hr = drive->GetProperty(pKey, &prop);
            if (SUCCEEDED(hr))
            {
                int status = prop.intVal;

                drive->Release();

                if (status == 1 || status == 3 || status == 5)
                    return status; // Encrypted
                else
                    return status; // not Encrypted
            }
        }
    }

    if (drive)
        drive->Release();

    return -1; // unknown
}
