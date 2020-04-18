// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_ANDROID_H_
#define CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_ANDROID_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/browser_process_platform_part_base.h"

class BrowserProcessPlatformPart : public BrowserProcessPlatformPartBase {
 public:
  BrowserProcessPlatformPart();
  ~BrowserProcessPlatformPart() override;

  // Overridden from BrowserProcessPlatformPartBase:
  void AttemptExit() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserProcessPlatformPart);
};

#endif  // CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_ANDROID_H_
