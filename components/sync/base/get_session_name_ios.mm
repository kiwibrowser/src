// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/get_session_name_ios.h"

#import <UIKit/UIKit.h>

#include "base/strings/sys_string_conversions.h"

namespace syncer {
namespace internal {

std::string GetComputerName() {
  return base::SysNSStringToUTF8([[UIDevice currentDevice] name]);
}

}  // namespace internal
}  // namespace syncer
