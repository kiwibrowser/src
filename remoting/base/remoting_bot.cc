// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/base/remoting_bot.h"

namespace remoting {

bool IsValidBotJid(const std::string& input) {
  if (input == kRemotingBotJid) {
    return true;
  }
#if !defined(NDEBUG)
  if (input == kRemotingTestBotJid) {
    return true;
  }
#endif  // !defined(NDEBUG)
  return false;
}

}  // namespace remoting