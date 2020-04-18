// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/app/memory_monitor.h"

#include <dispatch/dispatch.h>
#import <Foundation/NSPathUtilities.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#import "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#import "ios/chrome/browser/crash_report/breakpad_helper.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Delay between each invocations of |UpdateBreakpadMemoryValues|.
const int64_t kMemoryMonitorDelayInSeconds = 30;

// Checks the values of free RAM and free disk space and updates breakpad with
// these values.
void UpdateBreakpadMemoryValues() {
  base::AssertBlockingAllowed();
  const int free_memory =
      static_cast<int>(base::SysInfo::AmountOfAvailablePhysicalMemory() / 1024);
  breakpad_helper::SetCurrentFreeMemoryInKB(free_memory);
  NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                       NSUserDomainMask, YES);
  NSString* value = base::mac::ObjCCastStrict<NSString>([paths lastObject]);
  base::FilePath filePath = base::FilePath(base::SysNSStringToUTF8(value));
  const int free_disk_space =
      static_cast<int>(base::SysInfo::AmountOfFreeDiskSpace(filePath) / 1024);
  breakpad_helper::SetCurrentFreeDiskInKB(free_disk_space);
}

// Invokes |UpdateBreakpadMemoryValues| and schedules itself to be called
// after |kMemoryMonitorDelayInSeconds|.
void AsynchronousFreeMemoryMonitor() {
  UpdateBreakpadMemoryValues();
  base::PostDelayedTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&AsynchronousFreeMemoryMonitor),
      base::TimeDelta::FromSeconds(kMemoryMonitorDelayInSeconds));
}
}  // namespace

void StartFreeMemoryMonitor() {
  base::PostTaskWithTraits(FROM_HERE,
                           {base::MayBlock(), base::TaskPriority::BACKGROUND},
                           base::BindOnce(&AsynchronousFreeMemoryMonitor));
}
