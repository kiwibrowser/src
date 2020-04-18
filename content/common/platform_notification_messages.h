// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_PLATFORM_NOTIFICATION_MESSAGES_H_
#define CONTENT_COMMON_PLATFORM_NOTIFICATION_MESSAGES_H_

// Messages for platform-native notifications using the Web Notification API.

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include "content/public/common/common_param_traits_macros.h"
#include "content/public/common/notification_resources.h"
#include "content/public/common/platform_notification_data.h"
#include "ipc/ipc_message_macros.h"

#define IPC_MESSAGE_START PlatformNotificationMsgStart

// TODO(https://crbug.com/841329): Delete this legacy IPC code, use a pure
// mojo struct instead from ServiceWorkerEventDispatcher mojo interface.
IPC_ENUM_TRAITS_MAX_VALUE(
    content::PlatformNotificationData::Direction,
    content::PlatformNotificationData::DIRECTION_LAST)

IPC_ENUM_TRAITS_MAX_VALUE(content::PlatformNotificationActionType,
                          content::PLATFORM_NOTIFICATION_ACTION_TYPE_TEXT)

IPC_STRUCT_TRAITS_BEGIN(content::PlatformNotificationAction)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(action)
  IPC_STRUCT_TRAITS_MEMBER(title)
  IPC_STRUCT_TRAITS_MEMBER(icon)
  IPC_STRUCT_TRAITS_MEMBER(placeholder)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::PlatformNotificationData)
  IPC_STRUCT_TRAITS_MEMBER(title)
  IPC_STRUCT_TRAITS_MEMBER(direction)
  IPC_STRUCT_TRAITS_MEMBER(lang)
  IPC_STRUCT_TRAITS_MEMBER(body)
  IPC_STRUCT_TRAITS_MEMBER(tag)
  IPC_STRUCT_TRAITS_MEMBER(image)
  IPC_STRUCT_TRAITS_MEMBER(icon)
  IPC_STRUCT_TRAITS_MEMBER(badge)
  IPC_STRUCT_TRAITS_MEMBER(vibration_pattern)
  IPC_STRUCT_TRAITS_MEMBER(timestamp)
  IPC_STRUCT_TRAITS_MEMBER(renotify)
  IPC_STRUCT_TRAITS_MEMBER(silent)
  IPC_STRUCT_TRAITS_MEMBER(require_interaction)
  IPC_STRUCT_TRAITS_MEMBER(data)
  IPC_STRUCT_TRAITS_MEMBER(actions)
IPC_STRUCT_TRAITS_END()

#endif  // CONTENT_COMMON_PLATFORM_NOTIFICATION_MESSAGES_H_
