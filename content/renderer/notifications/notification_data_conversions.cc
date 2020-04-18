// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/notifications/notification_data_conversions.h"

#include <stddef.h>

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "third_party/blink/public/platform/modules/notifications/web_notification_action.h"
#include "third_party/blink/public/platform/url_conversion.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_vector.h"

using blink::WebNotificationData;
using blink::WebString;

namespace content {

WebNotificationData ToWebNotificationData(
    const PlatformNotificationData& platform_data) {
  WebNotificationData web_data;
  web_data.title = WebString::FromUTF16(platform_data.title);

  switch (platform_data.direction) {
    case PlatformNotificationData::DIRECTION_LEFT_TO_RIGHT:
      web_data.direction = WebNotificationData::kDirectionLeftToRight;
      break;
    case PlatformNotificationData::DIRECTION_RIGHT_TO_LEFT:
      web_data.direction = WebNotificationData::kDirectionRightToLeft;
      break;
    case PlatformNotificationData::DIRECTION_AUTO:
      web_data.direction = WebNotificationData::kDirectionAuto;
      break;
  }

  web_data.lang = WebString::FromUTF8(platform_data.lang);
  web_data.body = WebString::FromUTF16(platform_data.body);
  web_data.tag = WebString::FromUTF8(platform_data.tag);
  web_data.image = blink::WebURL(platform_data.image);
  web_data.icon = blink::WebURL(platform_data.icon);
  web_data.badge = blink::WebURL(platform_data.badge);
  web_data.vibrate = platform_data.vibration_pattern;
  web_data.timestamp = platform_data.timestamp.ToJsTime();
  web_data.renotify = platform_data.renotify;
  web_data.silent = platform_data.silent;
  web_data.require_interaction = platform_data.require_interaction;
  web_data.data = platform_data.data;
  blink::WebVector<blink::WebNotificationAction> resized(
      platform_data.actions.size());
  web_data.actions.Swap(resized);
  for (size_t i = 0; i < platform_data.actions.size(); ++i) {
    switch (platform_data.actions[i].type) {
      case PLATFORM_NOTIFICATION_ACTION_TYPE_BUTTON:
        web_data.actions[i].type = blink::WebNotificationAction::kButton;
        break;
      case PLATFORM_NOTIFICATION_ACTION_TYPE_TEXT:
        web_data.actions[i].type = blink::WebNotificationAction::kText;
        break;
      default:
        NOTREACHED() << "Unknown platform data type: "
                     << platform_data.actions[i].type;
    }
    web_data.actions[i].action =
        WebString::FromUTF8(platform_data.actions[i].action);
    web_data.actions[i].title =
        WebString::FromUTF16(platform_data.actions[i].title);
    web_data.actions[i].icon = blink::WebURL(platform_data.actions[i].icon);
    web_data.actions[i].placeholder =
        WebString::FromUTF16(platform_data.actions[i].placeholder);
  }

  return web_data;
}

}  // namespace content
