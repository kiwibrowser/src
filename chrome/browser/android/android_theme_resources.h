// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_ANDROID_THEME_RESOURCES_H_
#define CHROME_BROWSER_ANDROID_ANDROID_THEME_RESOURCES_H_

// LINK_RESOURCE_ID will use an ID defined by grit, so no-op.
#define LINK_RESOURCE_ID(c_id, java_id)
// For DECLARE_RESOURCE_ID, make an entry in an enum.
#define DECLARE_RESOURCE_ID(c_id, java_id) c_id,

enum {
  // Not used; just provides a starting value for the enum. These must
  // not conflict with IDR_* values, which top out at 2^16 - 1.
  ANDROID_RESOURCE_ID_NONE = 1 << 16,
#include "chrome/browser/android/resource_id.h"
};

#undef LINK_RESOURCE_ID
#undef DECLARE_RESOURCE_ID

#endif  // CHROME_BROWSER_ANDROID_ANDROID_THEME_RESOURCES_H_
