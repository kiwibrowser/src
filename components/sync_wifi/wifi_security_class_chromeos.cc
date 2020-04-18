// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/wifi_security_class.h"

#include "third_party/cros_system_api/dbus/service_constants.h"

namespace sync_wifi {

WifiSecurityClass WifiSecurityClassFromShillSecurity(
    const std::string& shill_security) {
  if (shill_security == shill::kSecurityNone)
    return SECURITY_CLASS_NONE;
  else if (shill_security == shill::kSecurityWep)
    return SECURITY_CLASS_WEP;
  else if (shill_security == shill::kSecurityPsk)
    return SECURITY_CLASS_PSK;
  else if (shill_security == shill::kSecurity8021x)
    return SECURITY_CLASS_802_1X;
  return SECURITY_CLASS_INVALID;
}

}  // namespace sync_wifi
