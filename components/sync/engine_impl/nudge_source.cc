// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/nudge_source.h"

#include "base/logging.h"

namespace syncer {

#define ENUM_CASE(x) \
  case x:            \
    return #x;       \
    break

const char* GetNudgeSourceString(NudgeSource nudge_source) {
  switch (nudge_source) {
    ENUM_CASE(NUDGE_SOURCE_UNKNOWN);
    ENUM_CASE(NUDGE_SOURCE_NOTIFICATION);
    ENUM_CASE(NUDGE_SOURCE_LOCAL);
    ENUM_CASE(NUDGE_SOURCE_LOCAL_REFRESH);
  }
  NOTREACHED();
  return "";
}

#undef ENUM_CASE

}  // namespace syncer
