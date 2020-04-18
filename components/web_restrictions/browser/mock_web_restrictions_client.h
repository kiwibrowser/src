// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEB_RESTRICTIONS_BROWSER_MOCK_WEB_RESTRICTIONS_CLIENT_H_
#define COMPONENTS_WEB_RESTRICTIONS_BROWSER_MOCK_WEB_RESTRICTIONS_CLIENT_H_

#include <jni.h>

#include "base/macros.h"

namespace web_restrictions {

// This is a wrapper for the Java MockWebRestrictionsClient.
class MockWebRestrictionsClient {
 public:
  MockWebRestrictionsClient();

  ~MockWebRestrictionsClient();
};

}  // namespace web_restrictions
#endif  // COMPONENTS_WEB_RESTRICTIONS_BROWSER_MOCK_WEB_RESTRICTIONS_CLIENT_H_
