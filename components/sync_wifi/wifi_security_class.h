// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_WIFI_WIFI_SECURITY_CLASS_H_
#define COMPONENTS_SYNC_WIFI_WIFI_SECURITY_CLASS_H_

#include <string>

#include "build/build_config.h"
#include "components/sync/protocol/sync.pb.h"

namespace sync_wifi {

enum WifiSecurityClass {
  SECURITY_CLASS_INVALID,
  SECURITY_CLASS_NONE,
  SECURITY_CLASS_WEP,
  SECURITY_CLASS_PSK,     // WPA-PSK or RSN-PSK
  SECURITY_CLASS_802_1X,  // WPA-Enterprise or RSN-Enterprise
};

// Returns true iff |security_class| allows passphrases. Note that a
// security class may permit passphrases, without requiring them.
bool WifiSecurityClassSupportsPassphrases(WifiSecurityClass security_class);

// Converts from Chrome Sync's serialized form of a security class, to
// a WifiSecurityClass. Returns the appropriate WifiSecurityClass
// value. If |sync_enum| is unrecognized, returns SECURITY_CLASS_INVALID.
WifiSecurityClass WifiSecurityClassFromSyncSecurityClass(
    sync_pb::WifiCredentialSpecifics_SecurityClass sync_enum);

// Converts from a WifiSecurityClass enum to Chrome Sync's serialized
// form of a security class. Returns the appropriate sync value. If
// |security_class| is unrecognized, or unsupported by Chrome Sync,
// returns sync's SECURITY_CLASS_INVALID.
sync_pb::WifiCredentialSpecifics_SecurityClass
WifiSecurityClassToSyncSecurityClass(WifiSecurityClass security_class);

// Converts from a WifiSecurityClass enum to an onc::wifi::kSecurity
// string value. The resulting string is written to
// |security_class_string|.  Returns false if |security_class| is
// SECURITY_CLASS_INVALID, or unrecognized.
bool WifiSecurityClassToOncSecurityString(WifiSecurityClass security_class,
                                          std::string* security_class_string);

#if defined(OS_CHROMEOS)
// Converts from a Security string presented by the ChromeOS
// connection manager ("Shill") to a WifiSecurityClass enum.  Returns
// the appropriate enum value. If |shill_security| is unrecognized,
// returns SECURITY_CLASS_INVALID.
WifiSecurityClass WifiSecurityClassFromShillSecurity(
    const std::string& shill_security);
#endif  // OS_CHROMEOS

}  // namespace sync_wifi

#endif  // COMPONENTS_SYNC_WIFI_WIFI_SECURITY_CLASS_H_
