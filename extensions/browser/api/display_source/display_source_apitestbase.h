// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DISPLAY_SOURCE_DISPLAY_SOURCE_APITESTBASE_H_
#define EXTENSIONS_BROWSER_API_DISPLAY_SOURCE_DISPLAY_SOURCE_APITESTBASE_H_

#include <memory>
#include <string>

#include "content/public/test/test_utils.h"
#include "extensions/browser/api/display_source/display_source_connection_delegate.h"
#include "extensions/browser/api/display_source/display_source_connection_delegate_factory.h"

namespace extensions {

// This functions sets up a mock connection delegate which
// simulates having of one sink device from the beginning with the properties:
// name is "sink 1", id is '1', auth.method is 'PIN', PIN value is '1234'.
// Calling of "StartWatchingAvailableSinks" will add one more sink device,
// its properties are: name is "sink 2", id is '2', auth.method is 'PBC'.
void InitMockDisplaySourceConnectionDelegate(content::BrowserContext* profile);

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DISPLAY_SOURCE_DISPLAY_SOURCE_APITESTBASE_H_
