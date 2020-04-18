// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_BASE_LOGGING_H_
#define COMPONENTS_SYNC_BASE_LOGGING_H_

#include "base/logging.h"

// TODO(akalin): This probably belongs in base/ somewhere.

namespace base {
class Location;
}  // namespace base

namespace syncer {

bool VlogIsOnForLocation(const base::Location& from_here, int verbose_level);

}  // namespace syncer

#define VLOG_LOC_STREAM(from_here, verbose_level)                     \
  logging::LogMessage(from_here.file_name(), from_here.line_number(), \
                      -verbose_level)                                 \
      .stream()

#define DVLOG_LOC(from_here, verbose_level)              \
  LAZY_STREAM(VLOG_LOC_STREAM(from_here, verbose_level), \
              DCHECK_IS_ON() &&                          \
                  (VLOG_IS_ON(verbose_level) ||          \
                   ::syncer::VlogIsOnForLocation(from_here, verbose_level)))

#endif  // COMPONENTS_SYNC_BASE_LOGGING_H_
