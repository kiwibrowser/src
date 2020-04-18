// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_restrictions/browser/mock_web_restrictions_client.h"

#include "jni/MockWebRestrictionsClient_jni.h"

namespace web_restrictions {

MockWebRestrictionsClient::MockWebRestrictionsClient() {
  Java_MockWebRestrictionsClient_registerAsMockForTesting(
      base::android::AttachCurrentThread());
}

MockWebRestrictionsClient::~MockWebRestrictionsClient() {
  Java_MockWebRestrictionsClient_unregisterAsMockForTesting(
      base::android::AttachCurrentThread());
}

}  // namespace web_restrictions

