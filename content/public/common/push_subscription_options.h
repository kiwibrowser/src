// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_PUSH_SUBSCRIPTION_OPTIONS_H_
#define CONTENT_PUBLIC_COMMON_PUSH_SUBSCRIPTION_OPTIONS_H_

#include <string>

#include "content/common/content_export.h"

namespace content {

// Structure to hold the options provided from the web app developer as
// part of asking for a new push subscription.
struct CONTENT_EXPORT PushSubscriptionOptions {
  PushSubscriptionOptions() {}
  ~PushSubscriptionOptions() {}

  // Whether or not the app developer agrees to provide user visible
  // notifications whenever they receive a push message.
  bool user_visible_only = false;

  // The unique identifier of the application service which is used to
  // verify the push message before delivery. This could either be an ID
  // assigned by the developer console or the app server's public key.
  std::string sender_info;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_CONTENT_PUSH_SUBSCRIPTION_OPTIONS_H_
