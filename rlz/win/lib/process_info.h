// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Information about the current process.

#ifndef RLZ_WIN_LIB_PROCESS_INFO_H_
#define RLZ_WIN_LIB_PROCESS_INFO_H_

#include "base/macros.h"

namespace rlz_lib {

class ProcessInfo {
 public:
  // All these functions cache the result after first run.
  static bool IsRunningAsSystem();
  static bool HasAdminRights();  // System / Admin / High Elevation on Vista

 private:
  DISALLOW_COPY_AND_ASSIGN(ProcessInfo);
};  // class
};  // namespace

#endif  // RLZ_WIN_LIB_PROCESS_INFO_H_
