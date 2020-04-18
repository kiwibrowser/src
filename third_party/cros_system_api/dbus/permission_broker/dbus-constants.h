// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_PERMISSION_BROKER_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_PERMISSION_BROKER_DBUS_CONSTANTS_H_

namespace permission_broker {
const char kPermissionBrokerInterface[] = "org.chromium.PermissionBroker";
const char kPermissionBrokerServicePath[] = "/org/chromium/PermissionBroker";
const char kPermissionBrokerServiceName[] = "org.chromium.PermissionBroker";

// Methods
const char kCheckPathAccess[] = "CheckPathAccess";
const char kRequestPathAccess[] = "RequestPathAccess";
const char kOpenPath[] = "OpenPath";
const char kRequestTcpPortAccess[] = "RequestTcpPortAccess";
const char kRequestUdpPortAccess[] = "RequestUdpPortAccess";
const char kReleaseTcpPort[] = "ReleaseTcpPort";
const char kReleaseUdpPort[] = "ReleaseUdpPort";
const char kRequestVpnSetup[] = "RequestVpnSetup";
const char kRemoveVpnSetup[] = "RemoveVpnSetup";
}  // namespace permission_broker

#endif  // SYSTEM_API_DBUS_PERMISSION_BROKER_DBUS_CONSTANTS_H_
