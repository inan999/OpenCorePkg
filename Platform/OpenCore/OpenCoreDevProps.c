/** @file
  OpenCore driver.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenCore.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDevicePropertyLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcStringLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathPropertyDatabase.h>

VOID
OcLoadDevPropsSupport (
  IN OC_GLOBAL_CONFIG    *Config
  )
{
  EFI_STATUS                                  Status;
  UINT32                                      DeviceIndex;
  UINT32                                      PropertyIndex;
  EFI_DEVICE_PATH_PROPERTY_DATABASE_PROTOCOL  *PropertyDatabase;
  OC_ASSOC                                    *PropertyMap;
  CHAR8                                       *AsciiDevicePath;
  CHAR16                                      *UnicodeDevicePath;
  CHAR8                                       *AsciiProperty;
  CHAR16                                      *UnicodeProperty;
  EFI_DEVICE_PATH_PROTOCOL                    *DevicePath;
  UINTN                                       OriginalSize;

  PropertyDatabase = OcDevicePathPropertyInstallProtocol (Config->DeviceProperties.ReinstallProtocol);
  if (PropertyDatabase == NULL) {
    DEBUG ((DEBUG_ERROR, "OC: Device property database protocol is missing\n"));
    return;
  }

  for (DeviceIndex = 0; DeviceIndex < Config->DeviceProperties.Block.Count; ++DeviceIndex) {
    AsciiDevicePath   = OC_BLOB_GET (Config->DeviceProperties.Block.Keys[DeviceIndex]);

    UnicodeDevicePath = AsciiStrCopyToUnicode (AsciiDevicePath, 0);
    DevicePath        = NULL;

    if (UnicodeDevicePath != NULL) {
      DevicePath = ConvertTextToDevicePath (UnicodeDevicePath);
      FreePool (UnicodeDevicePath);
    }

    if (DevicePath == NULL) {
      DEBUG ((DEBUG_WARN, "OC: Failed to parse %a device path\n", AsciiDevicePath));
      continue;
    }

    for (PropertyIndex = 0; PropertyIndex < Config->DeviceProperties.Block.Values[DeviceIndex]->Count; ++PropertyIndex) {
      AsciiProperty       = OC_BLOB_GET (Config->DeviceProperties.Block.Values[DeviceIndex]->Values[PropertyIndex]);
      UnicodeProperty = AsciiStrCopyToUnicode (AsciiProperty, 0);

      if (UnicodeProperty == NULL) {
        DEBUG ((DEBUG_WARN, "OC: Failed to convert %a property\n", AsciiProperty));
        continue;
      }

      Status = PropertyDatabase->RemoveProperty (
        PropertyDatabase,
        DevicePath,
        UnicodeProperty
        );

      DEBUG ((
        EFI_ERROR (Status) && Status != EFI_NOT_FOUND ? DEBUG_WARN : DEBUG_INFO,
        "OC: Removing devprop %a:%a - %r\n",
        AsciiDevicePath,
        AsciiProperty,
        Status
        ));

      FreePool (UnicodeProperty);
    }

    FreePool (DevicePath);
  }

  for (DeviceIndex = 0; DeviceIndex < Config->DeviceProperties.Add.Count; ++DeviceIndex) {
    PropertyMap       = Config->DeviceProperties.Add.Values[DeviceIndex];
    AsciiDevicePath   = OC_BLOB_GET (Config->DeviceProperties.Add.Keys[DeviceIndex]);
    UnicodeDevicePath = AsciiStrCopyToUnicode (AsciiDevicePath, 0);
    DevicePath        = NULL;

    if (UnicodeDevicePath != NULL) {
      DevicePath = ConvertTextToDevicePath (UnicodeDevicePath);
      FreePool (UnicodeDevicePath);
    }

    if (DevicePath == NULL) {
      DEBUG ((DEBUG_WARN, "OC: Failed to parse %a device path\n", AsciiDevicePath));
      continue;
    }

    for (PropertyIndex = 0; PropertyIndex < PropertyMap->Count; ++PropertyIndex) {
      AsciiProperty   = OC_BLOB_GET (PropertyMap->Keys[PropertyIndex]);
      UnicodeProperty = AsciiStrCopyToUnicode (AsciiProperty, 0);

      if (UnicodeProperty == NULL) {
        DEBUG ((DEBUG_WARN, "OC: Failed to convert %a property\n", AsciiProperty));
        continue;
      }

      OriginalSize = 0;
      Status = PropertyDatabase->GetProperty (
        PropertyDatabase,
        DevicePath,
        UnicodeProperty,
        NULL,
        &OriginalSize
        );

      DEBUG ((
        DEBUG_INFO,
        "OC: Getting devprop %a:%a - %r (will skip on too small)\n",
        AsciiDevicePath,
        AsciiProperty,
        Status
        ));

      if (Status != EFI_BUFFER_TOO_SMALL) {
        Status = PropertyDatabase->SetProperty (
          PropertyDatabase,
          DevicePath,
          UnicodeProperty,
          OC_BLOB_GET (PropertyMap->Values[PropertyIndex]),
          PropertyMap->Values[PropertyIndex]->Size
          );

        DEBUG ((
          EFI_ERROR (Status) ? DEBUG_WARN : DEBUG_INFO,
          "OC: Setting devprop %a:%a - %r\n",
          AsciiDevicePath,
          AsciiProperty,
          Status
          ));
      }

      FreePool (UnicodeProperty);
    }

    FreePool (DevicePath);
  }
}
