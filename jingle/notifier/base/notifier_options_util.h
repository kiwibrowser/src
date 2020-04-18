// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utility functions for NotifierOptions.

#ifndef JINGLE_NOTIFIER_BASE_NOTIFIER_OPTIONS_UTIL_H_
#define JINGLE_NOTIFIER_BASE_NOTIFIER_OPTIONS_UTIL_H_

#include <string>
#include <vector>

#include "jingle/notifier/base/server_information.h"
#include "third_party/libjingle_xmpp/xmpp/xmppclientsettings.h"

namespace notifier {

struct NotifierOptions;

buzz::XmppClientSettings MakeXmppClientSettings(
    const NotifierOptions& notifier_options,
    const std::string& email, const std::string& token);

ServerList GetServerList(const NotifierOptions& notifier_options);

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_BASE_NOTIFIER_OPTIONS_UTIL_H_
