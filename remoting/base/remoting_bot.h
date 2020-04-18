// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_BASE_REMOTING_BOT_H_
#define REMOTING_BASE_REMOTING_BOT_H_

#include <string>

namespace remoting {

// The JID of the remoting bot.
const char kRemotingBotJid[] = "remoting@bot.talk.google.com";

#if !defined(NDEBUG)
// The JID of the remoting bot test instance.
const char kRemotingTestBotJid[] = "remoting-test@bot.talk.google.com";
#endif  // !defined(NDEBUG)

// Returns true if |input| is a valid bot JID.
bool IsValidBotJid(const std::string& input);

}  // namespace remoting

#endif  // REMOTING_BASE_REMOTING_BOT_H_