// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_DESCRIPTORS_ANDROID_H_
#define CHROME_COMMON_DESCRIPTORS_ANDROID_H_

#include "build/build_config.h"
#include "content/public/common/content_descriptors.h"

enum {
#if defined(OS_ANDROID)
  kAndroidLocalePakDescriptor = kContentIPCDescriptorMax + 1,
  kAndroidSecondaryLocalePakDescriptor,
  kAndroidChrome100PercentPakDescriptor,
  kAndroidUIResourcesPakDescriptor,
  kAndroidMinidumpDescriptor,
#endif
};

#endif  // CHROME_COMMON_DESCRIPTORS_ANDROID_H_
