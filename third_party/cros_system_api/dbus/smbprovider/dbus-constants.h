// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_SMBPROVIDER_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_SMBPROVIDER_DBUS_CONSTANTS_H_

namespace smbprovider {

// General
const char kSmbProviderInterface[] = "org.chromium.SmbProvider";
const char kSmbProviderServicePath[] = "/org/chromium/SmbProvider";
const char kSmbProviderServiceName[] = "org.chromium.SmbProvider";

// Methods
const char kMountMethod[] = "Mount";
const char kUnmountMethod[] = "Unmount";
const char kReadDirectoryMethod[] = "ReadDirectory";
const char kGetMetadataEntryMethod[] = "GetMetadataEntry";
const char kOpenFileMethod[] = "OpenFile";
const char kCloseFileMethod[] = "CloseFile";
const char kReadFileMethod[] = "ReadFile";
const char kDeleteEntryMethod[] = "DeleteEntry";
const char kCreateFileMethod[] = "CreateFile";
const char kTruncateMethod[] = "Truncate";
const char kWriteFileMethod[] = "WriteFile";
const char kCreateDirectoryMethod[] = "CreateDirectory";
const char kMoveEntryMethod[] = "MoveEntry";
const char kCopyEntryMethod[] = "CopyEntry";
const char kGetDeleteListMethod[] = "GetDeleteList";
const char kGetSharesMethod[] = "GetShares";
const char kRemountMethod[] = "Remount";
const char kSetupKerberosMethod[] = "SetupKerberos";

}  // namespace smbprovider

#endif  // SYSTEM_API_DBUS_SMBPROVIDER_DBUS_CONSTANTS_H_
