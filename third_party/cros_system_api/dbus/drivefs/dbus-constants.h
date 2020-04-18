// Copyright 2018 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_DRIVEFS_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_DRIVEFS_DBUS_CONSTANTS_H_

namespace drivefs {

const char kDriveFileStreamInterface[] = "org.chromium.DriveFileStream";
const char kDriveFileStreamServicePath[] = "/org/chromium/DriveFileStream";
const char kDriveFileStreamServiceName[] = "org.chromium.DriveFileStream";

const char kDriveFileStreamOpenIpcChannelMethod[] = "OpenIpcChannel";

}  // namespace drivefs


#endif  // SYSTEM_API_DBUS_DRIVEFS_DBUS_CONSTANTS_H_
