// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_BASE_NOTIFICATION_METHOD_H_
#define JINGLE_NOTIFIER_BASE_NOTIFICATION_METHOD_H_

#include <string>

namespace notifier {

enum NotificationMethod {
  // Old peer-to-peer notification method.  Currently only used for
  // testing.
  NOTIFICATION_P2P,
  // Server-issued notifications.  The default.
  NOTIFICATION_SERVER,
};

extern const NotificationMethod kDefaultNotificationMethod;

std::string NotificationMethodToString(
    NotificationMethod notification_method);

// If the given string is not one of "p2p" or "server", returns
// kDefaultNotificationMethod.
NotificationMethod StringToNotificationMethod(const std::string& str);

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_BASE_NOTIFICATION_METHOD_H_

