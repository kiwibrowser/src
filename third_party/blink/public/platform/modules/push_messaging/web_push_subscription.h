// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PUSH_MESSAGING_WEB_PUSH_SUBSCRIPTION_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PUSH_MESSAGING_WEB_PUSH_SUBSCRIPTION_H_

#include "third_party/blink/public/platform/modules/push_messaging/web_push_subscription_options.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace blink {

struct WebPushSubscription {
  // The |endpoint|, |p256dh| and |auth| must all be unique for each
  // subscription.
  WebPushSubscription(const WebURL& endpoint,
                      bool user_visible_only,
                      const WebString& application_server_key,
                      const WebVector<unsigned char>& p256dh,
                      const WebVector<unsigned char>& auth)
      : endpoint(endpoint), p256dh(p256dh), auth(auth) {
    options.user_visible_only = user_visible_only;
    options.application_server_key = application_server_key;
  }

  WebURL endpoint;
  WebPushSubscriptionOptions options;
  WebVector<unsigned char> p256dh;
  WebVector<unsigned char> auth;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_PUSH_MESSAGING_WEB_PUSH_SUBSCRIPTION_H_
