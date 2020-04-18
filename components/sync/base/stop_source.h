// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_STOP_SOURCE_H_
#define COMPONENTS_SYNC_BASE_STOP_SOURCE_H_

namespace syncer {

// Enumerate the main sources that can turn off sync. This enum is used to
// back a UMA histogram and should be treated as append-only.
//
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.components.sync
enum StopSource {
  PROFILE_DESTRUCTION,   // The user destroyed the profile.
  SIGN_OUT,              // The user signed out of Chrome.
  BIRTHDAY_ERROR,        // A dashboard stop-and-clear on the server.
  CHROME_SYNC_SETTINGS,  // The on/off switch in settings for mobile Chrome.
  ANDROID_CHROME_SYNC,   // Android's sync setting for Chrome.
  ANDROID_MASTER_SYNC,   // Android's master sync setting.
  STOP_SOURCE_LIMIT,
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_BASE_STOP_SOURCE_H_
