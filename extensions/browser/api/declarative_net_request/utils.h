// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_UTILS_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_UTILS_H_

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace base {
class ListValue;
}  // namespace base

namespace extensions {
class Extension;
struct InstallWarning;

namespace declarative_net_request {

// Indexes and persists |rules| for |extension|. In case of an error, returns
// false and populates |error|. On success, returns |ruleset_checksum|, which
// is a checksum of the persisted indexed ruleset file. |ruleset_checksum| must
// not be null.
// Note: This must be called on a thread where file IO is allowed.
bool IndexAndPersistRules(const base::ListValue& rules,
                          const Extension& extension,
                          std::string* error,
                          std::vector<InstallWarning>* warnings,
                          int* ruleset_checksum);

// Returns true if |data| and |size| represent a valid data buffer containing
// indexed ruleset data with |expected_checksum|.
bool IsValidRulesetData(const uint8_t* data,
                        size_t size,
                        int expected_checksum);

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_UTILS_H_
