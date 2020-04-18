// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_BASE_LOGGING_H_
#define REMOTING_BASE_LOGGING_H_

#include "base/logging.h"

namespace remoting {

// Chromoting host code should use HOST_LOG instead of LOG(INFO) to bypass the
// CheckSpamLogging presubmit check. This won't spam chrome output because it
// runs in the chromoting host processes.
// In the future we may also consider writing to a log file instead of the
// console.
#define HOST_LOG LOG(INFO)
#define HOST_DLOG DLOG(INFO)

}  // namespace remoting

#endif  // REMOTING_BASE_LOGGING_H_
