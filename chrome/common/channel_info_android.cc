// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/channel_info.h"

#include "base/android/build_info.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "components/version_info/android/channel_getter.h"
#include "components/version_info/version_info.h"

namespace chrome {

std::string GetChannelName() {
  switch (GetChannel()) {
    case version_info::Channel::UNKNOWN: return "unknown";
    case version_info::Channel::CANARY: return "canary";
    case version_info::Channel::DEV: return "dev";
    case version_info::Channel::BETA: return "beta";
    case version_info::Channel::STABLE: return std::string();
  }
  NOTREACHED() << "Unknown channel " << static_cast<int>(GetChannel());
  return std::string();
}

version_info::Channel GetChannel() {
  return version_info::GetChannel();
}

}  // namespace chrome
