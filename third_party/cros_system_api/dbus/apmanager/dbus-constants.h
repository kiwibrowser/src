// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSTEM_API_DBUS_APMANAGER_DBUS_CONSTANTS_H_
#define SYSTEM_API_DBUS_APMANAGER_DBUS_CONSTANTS_H_

namespace apmanager {
const char kServiceName[] = "org.chromium.apmanager";
const char kManagerInterface[] = "org.chromium.apmanager.Manager";
const char kManagerPath[] = "/manager";
const char kManagerError[] = "org.chromium.apmanager.Manager.Error";
const char kServiceInterface[] = "org.chromium.apmanager.Service";
const char kServiceError[] = "org.chromium.apmanager.Service.Error";
const char kConfigInterface[] = "org.chromium.apmanager.Config";
const char kConfigError[] = "org.chromium.apmanager.Config.Error";
const char kClientInterface[] = "org.chromium.apmanager.Client";
const char kDeviceInterface[] = "org.chromium.apmanager.Device";

// Manager Methods.
const char kCreateServiceMethod[] = "CreateService";
const char kRemoveServiceMethod[] = "RemoveService";

// Config Properties.
const char kConfigSSIDProperty[] = "Ssid";
const char kConfigInterfaceNameProperty[] = "InterfaceName";
const char kConfigSecurityModeProperty[] = "SecurityMode";
const char kConfigPassphraseProperty[] = "Passphrase";
const char kConfigHwModeProperty[] = "HwMode";
const char kConfigOperationModeProperty[] = "OperationMode";
const char kConfigChannelProperty[] = "Channel";
const char kConfigHiddenNetworkProperty[] = "HiddenNetwork";
const char kConfigBridgeInterfaceProperty[] = "BridgeInterface";
const char kConfigServerAddressIndexProperty[] = "ServerAddressIndex";

// Security modes.
const char kSecurityModeNone[] = "none";
const char kSecurityModeRSN[] = "rsn";

// Hardware modes.
const char kHwMode80211a[] = "802.11a";
const char kHwMode80211b[] = "802.11b";
const char kHwMode80211g[] = "802.11g";
const char kHwMode80211n[] = "802.11n";
const char kHwMode80211ac[] = "802.11ac";

// Operation modes.
const char kOperationModeServer[] = "server";
const char kOperationModeBridge[] = "bridge";

// D-Bus error codes.
const char kErrorInternalError[] =
    "org.chromium.apmanager.Error.InternalError";
const char kErrorInvalidArguments[] =
    "org.chromium.apmanager.Error.InvalidArguments";
const char kErrorInvalidConfiguration[] =
    "org.chromium.apmanager.Error.InvalidConfiguration";
const char kErrorSuccess[] = "org.chromium.apmanager.Error.Success";

}  // namespace apmanager

#endif  // SYSTEM_API_DBUS_APMANAGER_DBUS_CONSTANTS_H_
