// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_MAC_H_
#define CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_MAC_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/apps/app_shim/app_shim_host_manager_mac.h"
#include "chrome/browser/browser_process_platform_part_base.h"

class BrowserProcessPlatformPart : public BrowserProcessPlatformPartBase {
 public:
  BrowserProcessPlatformPart();
  ~BrowserProcessPlatformPart() override;

  // Overridden from BrowserProcessPlatformPartBase:
  void StartTearDown() override;
  void AttemptExit() override;
  void PreMainMessageLoopRun() override;

  AppShimHostManager* app_shim_host_manager();

 private:
  // Hosts the IPC channel factory that App Shims connect to on Mac.
  scoped_refptr<AppShimHostManager> app_shim_host_manager_;

  DISALLOW_COPY_AND_ASSIGN(BrowserProcessPlatformPart);
};

#endif  // CHROME_BROWSER_BROWSER_PROCESS_PLATFORM_PART_MAC_H_
