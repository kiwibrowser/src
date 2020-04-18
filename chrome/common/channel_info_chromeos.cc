// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/channel_info.h"

#include "components/version_info/version_info.h"

namespace chrome {

static version_info::Channel chromeos_channel = version_info::Channel::UNKNOWN;

std::string GetChannelName() {
#if defined(GOOGLE_CHROME_BUILD)
  switch (chromeos_channel) {
    case version_info::Channel::STABLE:
      return "";
    case version_info::Channel::BETA:
      return "beta";
    case version_info::Channel::DEV:
      return "dev";
    case version_info::Channel::CANARY:
      return "canary";
    default:
      return "unknown";
  }
#endif
  return std::string();
}

version_info::Channel GetChannel() {
  return chromeos_channel;
}

void SetChannel(const std::string& channel) {
#if defined(GOOGLE_CHROME_BUILD)
  if (channel == "stable-channel") {
    chromeos_channel = version_info::Channel::STABLE;
  } else if (channel == "beta-channel") {
    chromeos_channel = version_info::Channel::BETA;
  } else if (channel == "dev-channel") {
    chromeos_channel = version_info::Channel::DEV;
  } else if (channel == "canary-channel") {
    chromeos_channel = version_info::Channel::CANARY;
  }
#endif
}

#if defined(GOOGLE_CHROME_BUILD)
std::string GetChannelSuffixForDataDir() {
  // ChromeOS doesn't support side-by-side installations.
  return std::string();
}
#endif  // defined(GOOGLE_CHROME_BUILD)

}  // namespace chrome
