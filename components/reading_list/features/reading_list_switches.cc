// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/reading_list/features/reading_list_switches.h"

#include "base/command_line.h"
#include "build/build_config.h"
#include "components/reading_list/features/reading_list_buildflags.h"

namespace reading_list {
namespace switches {
bool IsReadingListEnabled() {
  return BUILDFLAG(ENABLE_READING_LIST);
}
}  // namespace switches
}  // namespace reading_list
