// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_TEST_UTILS_H_
#define EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_TEST_UTILS_H_

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {

class Extension;

namespace declarative_net_request {

// Enum specifying the extension load type. Used for parameterized tests.
enum class ExtensionLoadType {
  PACKED,
  UNPACKED,
};

// Returns true if the given extension has a valid indexed ruleset. Should be
// called on a sequence where file IO is allowed.
bool HasValidIndexedRuleset(const Extension& extension,
                            content::BrowserContext* browser_context);

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DECLARATIVE_NET_REQUEST_TEST_UTILS_H_
